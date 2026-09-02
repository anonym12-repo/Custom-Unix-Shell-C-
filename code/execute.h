#ifndef EXECUTE_H
#define EXECUTE_H

#include "shell.h"

/*
 * Executes an external command by performing wildcard expansion,
 * creating a child process and invoking execvp().
 */
void execute_command(Command *cmd);

/*
 * Executes a single command with optional background execution
 */
int execute_single_command(Command *cmd, int background);

/*
 * Executes a pipeline of commands
 */
int execute_pipeline(Command *cmds[], int num_cmds);

#endif