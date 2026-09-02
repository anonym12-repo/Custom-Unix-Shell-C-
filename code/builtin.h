#ifndef BUILTIN_H
#define BUILTIN_H

#include "shell.h"

/* Current shell prompt displayed before each command. Shared with
 * terminal.c, which redraws it while editing the current line. */
extern char shell_prompt[PROMPT_MAX];

/*
 * Executes a shell built-in command (exit, pwd, cd, prompt, history).
 *
 * Returns:
 *   1 if the command was a built-in (and was handled).
 *   0 otherwise, so the caller knows to fall back to execute_command().
 */
int execute_builtin(Command *cmd);

#endif
