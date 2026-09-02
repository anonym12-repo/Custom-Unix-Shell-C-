/*
 * terminal.c
 *
 * Puts the terminal into "raw mode" so the shell can read input
 * one keystroke at a time instead of waiting for a full line from
 * the kernel's line discipline. This is what lets us support
 * Up/Down history recall and Left/Right cursor movement while the
 * user is typing a command.
 *
 * This file previously lived inside main.c; it was pulled out
 * because terminal/line-editing is a distinct responsibility from
 * "run the shell's main loop".
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>

#include "terminal.h"
#include "shell.h"
#include "history.h"
#include "builtin.h"   /* for shell_prompt, used when redrawing the line */

/* Saved terminal settings so we can restore them on exit from raw mode. */
static struct termios orig_termios;

/*
 * Switches the terminal into raw mode:
 *   - ICANON off  -> input is available byte-by-byte, not line-by-line
 *   - ECHO off    -> we handle echoing ourselves (so we can redraw the
 *                    line cleanly during history recall / editing)
 *   - ISIG off    -> Ctrl-C etc. are delivered as ordinary bytes rather
 *                    than generating signals while we're reading a line
 */
static void enable_raw_mode(void)
{
    struct termios raw;

    tcgetattr(STDIN_FILENO, &orig_termios);
    raw = orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO | ISIG);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

/* Restores the terminal settings saved by enable_raw_mode(). */
static void disable_raw_mode(void)
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

/*
 * Reads a single raw byte from stdin, retrying on EINTR (e.g. if a
 * SIGCHLD arrives from a reaped background job while we're waiting
 * on input).
 *
 * Returns the byte read, or 0 on EOF/error.
 */
static char read_char(void)
{
    char c;
    ssize_t n;

    while (1)
    {
        n = read(STDIN_FILENO, &c, 1);
        if (n == 1)
            return c;
        if (n == 0)
            return 0;
        if (n < 0 && errno == EINTR)
            continue;
        return 0;
    }
}

/*
 * Redraws the current line from the start of the prompt: clears to
 * end-of-line and repositions the cursor at `pos` characters into
 * `line`. Centralising this avoids repeating the same three printf
 * calls at every edit site below.
 */
#include <sys/ioctl.h>

static int last_render_rows = 1;   /* rows used by the previous redraw */

static int get_term_width(void)
{
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0)
        return ws.ws_col;
    return 80;   /* sane fallback if ioctl fails */
}

static void redraw_line(const char *line, int pos)
{
    int prompt_len  = strlen(shell_prompt);
    int term_width  = get_term_width();
    int total_len   = prompt_len + (int)strlen(line);
    int cursor_off  = prompt_len + pos;

    /* Ceiling division that matches terminals' deferred-wrap behavior:
     * writing exactly term_width chars fills one row without wrapping
     * yet, so total_len == term_width must still report 1 row. */
    int total_rows = (total_len == 0) ? 1 : (total_len - 1) / term_width + 1;
    int cursor_row = (cursor_off == 0) ? 0 : (cursor_off - 1) / term_width;
    int cursor_col = (cursor_off == 0) ? 0 : (cursor_off - 1) % term_width + 1;

    if (last_render_rows > 1)
        printf("\033[%dA", last_render_rows - 1);
    printf("\r");
    printf("\033[J");

    printf("%s%s", shell_prompt, line);

    int rows_below_cursor = (total_rows - 1) - cursor_row;
    if (rows_below_cursor > 0)
        printf("\033[%dA", rows_below_cursor);
    printf("\r");
    if (cursor_col > 0)
        printf("\033[%dC", cursor_col);

    fflush(stdout);
    last_render_rows = total_rows;
}

/*
 * Reads one line of input with basic line editing:
 *   - printable characters are inserted at the cursor
 *   - Backspace deletes the character before the cursor
 *   - Left/Right arrows move the cursor
 *   - Up/Down arrows recall previous/next history entries
 *   - Enter finishes the line
 *   - Ctrl-D on an otherwise-empty read signals EOF
 *
 * `line` must be a buffer of at least `max_len` bytes; it is left
 * NUL-terminated on both the success and EOF paths.
 */
int read_command_with_history(char *line, int max_len)
{
    int pos = 0;
    int history_pos = get_history_count();
    char c;
    int done = 0;
    int escape_seq = 0;
    int bracket = 0;

    /* Discard any stale input that may have accumulated before this call. */
    tcflush(STDIN_FILENO, TCIFLUSH);

    line[0] = '\0';
    last_render_rows = 1;
    enable_raw_mode();

    while (!done)
    {
        c = read_char();

        /* Read failure or unexpected EOF mid-line: treat as shell EOF. */
        if (c == 0)
        {
            disable_raw_mode();
            tcflush(STDIN_FILENO, TCIFLUSH);
            return 0;
        }

        /* ---- Start of an ANSI escape sequence (arrow keys etc.) ---- */
        if (escape_seq == 0 && c == 27)
        {
            escape_seq = 1;
            bracket = 0;
            continue;
        }
        if (escape_seq == 1 && c == '[')
        {
            bracket = 1;
            continue;
        }
        if (escape_seq == 1 && bracket == 1)
        {
            if (c == 'A') /* Up arrow: recall older history entry */
            {
                if (history_pos > 0)
                {
                    history_pos--;
                    const char *cmd = get_history_command(history_pos + 1);
                    if (cmd)
                    {
                        strcpy(line, cmd);
                        pos = strlen(line);
                        redraw_line(line, pos);
                    }
                }
            }
            else if (c == 'B') /* Down arrow: recall newer history entry */
            {
                int hist_count = get_history_count();
                if (history_pos < hist_count - 1)
                {
                    history_pos++;
                    const char *cmd = get_history_command(history_pos + 1);
                    if (cmd)
                    {
                        strcpy(line, cmd);
                        pos = strlen(line);
                        redraw_line(line, pos);
                    }
                }
                else if (history_pos == hist_count - 1)
                {
                    /* Moved past the newest entry: back to a blank line. */
                    history_pos = hist_count;
                    line[0] = '\0';
                    pos = 0;
                    redraw_line(line, pos);
                }
            }
            else if (c == 'C') /* Right arrow */
            {
                int len = strlen(line);
                if (pos < len)
                {
                    pos++;
                    printf("\033[C");
                    fflush(stdout);
                }
            }
            else if (c == 'D') /* Left arrow */
            {
                if (pos > 0)
                {
                    pos--;
                    printf("\033[D");
                    fflush(stdout);
                }
            }
            /* Any other sequence (Home/End/Delete/...) is ignored. */

            escape_seq = 0;
            bracket = 0;
            continue;
        }
        if (escape_seq)
        {
            /* Incomplete/unrecognised escape sequence: discard it. */
            escape_seq = 0;
            bracket = 0;
            continue;
        }

        /* ---- Ordinary (non-escape) key handling ---- */
        if (c == '\n' || c == '\r')   /* Enter key */
        {
            printf("\n");
            done = 1;
            /* Flush any extra characters (e.g., a stray \n after \r) immediately. */
            tcflush(STDIN_FILENO, TCIFLUSH);
        }
        else if (c == 127 || c == 8) /* Backspace */
        {
            if (pos > 0)
            {
                int len = strlen(line);
                for (int i = pos; i < len; i++)
                    line[i - 1] = line[i];
                line[len - 1] = '\0';
                pos--;
                redraw_line(line, pos);
            }
        }
        else if (c == 4) /* Ctrl-D: EOF */
        {
            printf("\n");
            disable_raw_mode();
            tcflush(STDIN_FILENO, TCIFLUSH);
            return 0;
        }
        else if (c >= 32 && c < 127) /* Printable character: insert at cursor */
        {
            int len = strlen(line);
            if (len < max_len - 1)
            {
                for (int i = len; i > pos; i--)
                    line[i] = line[i - 1];
                line[pos] = c;
                line[len + 1] = '\0';
                pos++;
                redraw_line(line, pos);
            }
        }
        /* Other control characters are ignored. */
    }

    disable_raw_mode();
    tcflush(STDIN_FILENO, TCIFLUSH);
    return 1;
}