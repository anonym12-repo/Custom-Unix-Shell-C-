#ifndef HISTORY_H
#define HISTORY_H

#include "shell.h"

/* Maximum number of commands to remember */
#define MAX_HISTORY 100

/*
 * Add a command to history
 */
void add_history(const char *cmd);

/*
 * Get the nth command from history (1-indexed)
 */
const char *get_history_command(int n);

/*
 * Get the most recent command starting with a string
 */
const char *get_history_by_prefix(const char *prefix);

/*
 * Print the history
 */
void print_history(void);

/*
 * Get the last command in history
 */
const char *get_last_command(void);

/*
 * Get the previous command (for arrow up)
 */
const char *get_prev_history(void);

/*
 * Get the next command (for arrow down)
 */
const char *get_next_history(void);

/*
 * Reset history navigation index
 */
void reset_history_index(void);

/*
 * Get the current history count
 */
int get_history_count(void);

/*
 * Expands a history reference in place.
 *
 * If `line` begins with '!', this looks up the matching history
 * entry ("!!", "!n", or "!prefix") and overwrites `line` (a buffer
 * of at least MAX_LINE bytes) with the expanded command text,
 * printing it to stdout the way an interactive shell echoes
 * history expansions.
 *
 * Returns:
 *   1 if `line` started with '!' and was successfully expanded -
 *     the caller should proceed to add it to history and execute it.
 *   0 if `line` did not start with '!' (nothing to expand), or if
 *     it did but no matching history entry was found. In the latter
 *     case an error message has already been printed, and `line`
 *     is left as-is; the caller should skip execution rather than
 *     try to run the unexpanded "!..." text as a literal command.
 */
int expand_history_reference(char *line);

#endif