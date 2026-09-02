/*
 * main.c
 *
 * Entry point of the ICT374 shell.
 *
 * This file is intentionally small: it owns only the top-level
 * read -> expand-history -> split -> parse -> execute loop. Each
 * step is delegated to the module that actually owns that
 * responsibility:
 *
 *   terminal.c  - raw-mode line editing / history navigation
 *   history.c   - "!!" / "!n" / "!prefix" expansion, storage
 *   parser.c    - splitting a line into Commands, tokenising each
 *                 one, and splitting pipelines
 *   builtin.c   - built-in commands (cd, pwd, exit, prompt, history)
 *   execute.c   - forking/exec'ing external commands and pipelines
 *   signals.c   - shell-level signal handling
 */
#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "shell.h"
#include "terminal.h"
#include "parser.h"
#include "execute.h"
#include "builtin.h"
#include "signals.h"
#include "history.h"

/*
 * Runs a single already-parsed Command: tries the built-in table
 * first, and falls back to launching it as an external process.
 */
static void run_command(Command *cmd)
{
    if (!execute_builtin(cmd))
        execute_single_command(cmd, cmd->background);
}

/*
 * Runs one Command that has already been through split_commands().
 * If it turns out to contain a pipeline ("cmd1 | cmd2 | ..."), it
 * is split into pipeline stages and handed to execute_pipeline();
 * otherwise it's run directly via run_command().
 */
static void run_line_command(Command *cmd)
{
    if (has_pipeline(cmd))
    {
        Command *pipeline_cmds[MAX_PIPELINE];
        int pipe_count = split_pipeline(cmd, pipeline_cmds);

        if (pipe_count > 0)
        {
            execute_pipeline(pipeline_cmds, pipe_count);
            for (int j = 0; j < pipe_count; j++)
                free(pipeline_cmds[j]);
        }
    }
    else
    {
        run_command(cmd);
    }
}

int main(void)
{
    char line[MAX_LINE];
    int num_cmds;
    int read_result;

    /* Allocate command array on the heap to avoid stack overflow
     * when MAX_COMMANDS is large. Each Command contains a 1001‑entry
     * argv array, so the whole block can be several hundred KB.
     * This memory is reused for every command line. */
    Command *cmds = malloc(MAX_COMMANDS * sizeof(Command));
    if (!cmds) {
        perror("malloc");
        exit(1);
    }

    setup_shell_signals();

    while (1)
    {
        printf("%s", shell_prompt);
        fflush(stdout);

        read_result = read_command_with_history(line, sizeof(line));
        if (read_result == 0)
        {
            /* EOF (Ctrl-D) or a read error: exit the shell. */
            printf("\n");
            break;
        }

        if (line[0] == '\n' || line[0] == '\0')
            continue;

        /* "!!" / "!n" / "!prefix": expand in place before doing
         * anything else, so it's the expanded command text (not
         * the "!..." shorthand) that gets added to history and
         * executed. If expansion fails (no matching entry), an
         * error is already printed, so skip straight to the next
         * prompt rather than trying to run "!..." as a literal
         * command. */
        if (line[0] == '!')
        {
            if (!expand_history_reference(line))
                continue;
        }

        add_history(line);

        /* split_commands() zeroes each Command it fills, so we don't
         * need to clear the whole array. It uses parse_command() which
         * also zeroes the Command before filling it. */
        num_cmds = split_commands(line, cmds);

        for (int i = 0; i < num_cmds; i++)
            run_line_command(&cmds[i]);
    }

    free(cmds);
    return 0;
}