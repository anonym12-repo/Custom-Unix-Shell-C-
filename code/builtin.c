/*
 * builtin.c
 *
 * Implements shell built-in commands. Built-ins run directly in
 * the shell process (no fork()) because they need to affect the
 * shell's own state - e.g. "cd" has to change the shell's current
 * directory, not a child's, and "exit" has to terminate the shell
 * itself.
 *
 * Supported built-ins: exit, pwd, cd, prompt, history
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "builtin.h"
#include "history.h"

/* Default shell prompt. Defined here (not just declared) since
 * this is the one place that owns the storage; builtin.h exposes
 * it via extern for terminal.c to read when redrawing the line. */
char shell_prompt[PROMPT_MAX] = "% ";

/*
 * Executes built-in shell commands without creating a child
 * process.
 *
 * Returns:
 *   1 if cmd->argv[0] named a built-in (whether or not it
 *     succeeded - failures are reported via perror()).
 *   0 if cmd->argv[0] is not a recognised built-in, so the caller
 *     should treat this as an external command instead.
 */
int execute_builtin(Command *cmd)
{
    if (cmd->argv[0] == NULL)
        return 0;

    if (strcmp(cmd->argv[0], "exit") == 0)
    {
        /* Honour "exit N" if a status code was given, defaulting
         * to 0 for a bare "exit". */
        int status = (cmd->argc >= 2) ? atoi(cmd->argv[1]) : 0;
        exit(status);
    }

    if (strcmp(cmd->argv[0], "pwd") == 0)
    {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL)
            printf("%s\n", cwd);
        else
            perror("pwd");
        return 1;
    }

    if (strcmp(cmd->argv[0], "cd") == 0)
    {
        char *path;

        if (cmd->argc == 1)
            path = getenv("HOME");
        else
            path = cmd->argv[1];

        if (path != NULL && chdir(path) != 0)
            perror("cd");
        return 1;
    }

    if (strcmp(cmd->argv[0], "prompt") == 0)
    {
        if (cmd->argc >= 2)
        {
            /* Leave room for the trailing space and NUL we append
             * below: sizeof(shell_prompt) - 2 bytes of the user's
             * text, plus ' ' plus '\0'. */
            strncpy(shell_prompt, cmd->argv[1], sizeof(shell_prompt) - 2);
            shell_prompt[sizeof(shell_prompt) - 2] = '\0';
            strcat(shell_prompt, " ");
        }
        else
        {
            /* No argument: reset to default prompt. */
            strcpy(shell_prompt, "% ");
        }
        return 1;
    }

    if (strcmp(cmd->argv[0], "history") == 0)
    {
        print_history();
        return 1;
    }

    return 0;
}
