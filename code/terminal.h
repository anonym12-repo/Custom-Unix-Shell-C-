#ifndef TERMINAL_H
#define TERMINAL_H

/*
 * terminal.h
 *
 * Raw-mode terminal input handling: reads a line of input while
 * supporting left/right cursor movement, backspace, and up/down
 * arrow-key navigation through command history.
 */

/*
 * Reads a single line of input into `line` (a buffer of at least
 * `max_len` bytes), echoing characters and honouring arrow keys /
 * backspace as the user types. Uses the shell's history module to
 * support Up/Down recall while editing.
 *
 * Returns:
 *   1 if a line was successfully read (terminated by Enter).
 *   0 on EOF (Ctrl-D) or a read error, in which case `line` should
 *     be treated as invalid/empty by the caller.
 */
int read_command_with_history(char *line, int max_len);

#endif
