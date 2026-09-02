/*
 * history.c
 *
 * Implements command history for the shell.
 * Supports:
 *   - history built-in command
 *   - !n to repeat command n
 *   - !! to repeat last command
 *   - !string to repeat last command starting with string
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "history.h"

static char history[MAX_HISTORY][MAX_LINE];
static int history_count = 0;
static int history_index = 0;

/*
 * Add a command to history
 */
void add_history(const char *cmd)
{
    if (cmd == NULL || cmd[0] == '\0')
        return;
    
    /* Don't add duplicate of last command */
    if (history_count > 0 && strcmp(history[history_count - 1], cmd) == 0)
        return;
    
    /* Shift if full */
    if (history_count >= MAX_HISTORY)
    {
        for (int i = 0; i < MAX_HISTORY - 1; i++)
            strcpy(history[i], history[i + 1]);
        history_count = MAX_HISTORY - 1;
    }
    
    strcpy(history[history_count], cmd);
    history_count++;
    history_index = history_count;
}

/*
 * Get the nth command from history (1-indexed)
 */
const char *get_history_command(int n)
{
    if (n <= 0 || n > history_count)
        return NULL;
    return history[n - 1];
}

/*
 * Get the most recent command starting with a string
 */
const char *get_history_by_prefix(const char *prefix)
{
    if (prefix == NULL || *prefix == '\0')
        return get_last_command();
    
    for (int i = history_count - 1; i >= 0; i--)
    {
        if (strncmp(history[i], prefix, strlen(prefix)) == 0)
            return history[i];
    }
    return NULL;
}

/*
 * Print the history
 */
void print_history(void)
{
    for (int i = 0; i < history_count; i++)
        printf("%d  %s\n", i + 1, history[i]);
}

/*
 * Get the last command in history
 */
const char *get_last_command(void)
{
    if (history_count == 0)
        return NULL;
    return history[history_count - 1];
}

/*
 * Get the previous command (for arrow up)
 */
const char *get_prev_history(void)
{
    if (history_count == 0)
        return NULL;
    
    if (history_index > 0)
        history_index--;
    
    return history[history_index];
}

/*
 * Get the next command (for arrow down)
 */
const char *get_next_history(void)
{
    if (history_count == 0)
        return NULL;
    
    if (history_index < history_count - 1)
        history_index++;
    else
        history_index = history_count;
    
    if (history_index >= history_count)
        return NULL;
    
    return history[history_index];
}

/*
 * Reset history navigation index
 */
void reset_history_index(void)
{
    history_index = history_count;
}

/*
 * Get the current history count
 */
int get_history_count(void)
{
    return history_count;
}

/*
 * Expands a "!" history reference in place. See history.h for the
 * full contract. This used to live in main.c as
 * handle_history_command(); it was moved here because history.c is
 * the module that owns the history data, so the code that
 * interprets "!!" / "!n" / "!prefix" against that data belongs
 * alongside it rather than in the main loop.
 */
int expand_history_reference(char *line)
{
    const char *cmd = NULL;

    if (line[0] != '!')
        return 0;

    if (strcmp(line, "!!") == 0)
    {
        cmd = get_last_command();
        if (cmd == NULL)
        {
            printf("No commands in history\n");
            return 0;
        }
    }
    else if (line[1] >= '0' && line[1] <= '9')
    {
        int n = atoi(line + 1);
        cmd = get_history_command(n);
        if (cmd == NULL)
        {
            printf("No such command in history\n");
            return 0;
        }
    }
    else
    {
        cmd = get_history_by_prefix(line + 1);
        if (cmd == NULL)
        {
            printf("No command starts with '%s'\n", line + 1);
            return 0;
        }
    }

    strncpy(line, cmd, MAX_LINE - 1);
    line[MAX_LINE - 1] = '\0';
    printf("%s\n", line);
    return 1;
}