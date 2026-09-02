/*
 * execute.c
 *
 * Executes external commands entered into the shell: performs
 * wildcard expansion, sets up I/O redirection, and forks/execvp's
 * either a single command or a pipeline of commands.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <glob.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

#include "signals.h"
#include "execute.h"
#include "parser.h"

/* Heap-duplicates a string. Used when expanding wildcards, since
 * glob() results need to outlive the glob_t they came from. */
static char *copy_string(const char *src)
{
    char *dest = malloc(strlen(src) + 1);
    if (dest != NULL)
        strcpy(dest, src);
    return dest;
}

/*
 * Expands any argv entries containing shell wildcard characters
 * ('*' or '?') into the list of matching filenames via glob(3).
 * Entries with no wildcard characters, and wildcard entries that
 * match nothing (glob() fails), are passed through unchanged - the
 * latter matches typical shell behaviour of leaving an unmatched
 * pattern as a literal argument.
 */
static void expand_wildcards(Command *cmd)
{
    char *newargv[MAX_ARGS + 1];
    int newargc = 0;
    int i;

    for (i = 0; i < cmd->argc; i++)
    {
        if (cmd->argv[i] != NULL && strpbrk(cmd->argv[i], "*?") != NULL)
        {
            glob_t results;

            if (glob(cmd->argv[i], 0, NULL, &results) == 0)
            {
                size_t j;
                for (j = 0; j < results.gl_pathc && newargc < MAX_ARGS; j++)
                    newargv[newargc++] = copy_string(results.gl_pathv[j]);
                globfree(&results);
            }
            else
            {
                newargv[newargc++] = cmd->argv[i];
            }
        }
        else
        {
            newargv[newargc++] = cmd->argv[i];
        }
    }

    newargv[newargc] = NULL;
    cmd->argc = newargc;

    for (i = 0; i <= newargc; i++)
        cmd->argv[i] = newargv[i];

    if (cmd->argc > 0)
        cmd->com_pathname = cmd->argv[0];
}

/*
 * Applies this command's I/O redirections (< > >> 2> 2>>) to the
 * calling process's standard file descriptors. Must only be called
 * in a child process after fork(), before execvp() - it exits the
 * child directly on failure since there is no reasonable way to
 * "return an error" once stdin/stdout/stderr may already be
 * half-redirected.
 */
static void setup_redirections(Command *cmd)
{
    if (cmd->redirect_in != NULL)
    {
        int fd = open(cmd->redirect_in, O_RDONLY);
        if (fd < 0)
        {
            perror("open input");
            exit(1);
        }
        if (dup2(fd, STDIN_FILENO) < 0)
        {
            perror("dup2 stdin");
            exit(1);
        }
        close(fd);
    }

    if (cmd->redirect_out != NULL)
    {
        int flags = O_WRONLY | O_CREAT;
        int fd;

        flags |= cmd->append_out ? O_APPEND : O_TRUNC;

        fd = open(cmd->redirect_out, flags, 0644);
        if (fd < 0)
        {
            perror("open output");
            exit(1);
        }
        if (dup2(fd, STDOUT_FILENO) < 0)
        {
            perror("dup2 stdout");
            exit(1);
        }
        close(fd);
    }

    if (cmd->redirect_err != NULL)
    {
        int flags = O_WRONLY | O_CREAT;
        int fd;

        flags |= cmd->append_err ? O_APPEND : O_TRUNC;

        /* NOTE: this previously opened cmd->redirect_out here by
         * mistake, which meant "2> errfile" silently redirected
         * stderr to the stdout target instead. Fixed to open
         * cmd->redirect_err, and to use append_err (not append_out)
         * so "2>>" appends independently of ">>". */
        fd = open(cmd->redirect_err, flags, 0644);
        if (fd < 0)
        {
            perror("open error");
            exit(1);
        }
        if (dup2(fd, STDERR_FILENO) < 0)
        {
            perror("dup2 stderr");
            exit(1);
        }
        close(fd);
    }
}

/* Simple incrementing job counter for background job messages
 * ("[1] 12345", "[2] 12388", ...). This shell doesn't maintain a
 * full job table (no `jobs` builtin, no re-attaching to a
 * background job by number), so this counter exists purely to
 * make the bracketed number meaningful instead of just repeating
 * the PID. */
static int next_job_number = 1;

/*
 * Executes a single (non-piped) command: expands wildcards, forks,
 * and in the child restores default signal handling, applies
 * redirections, and execvp()s. The parent either waits for the
 * child (foreground) or reports its PID and returns immediately
 * (background).
 *
 * Returns the child's wait status on foreground completion, 0 for
 * a background launch, or -1 on fork/wait failure.
 */
int execute_single_command(Command *cmd, int background)
{
    pid_t pid;

    if (cmd->argv[0] == NULL)
        return 0;

    expand_wildcards(cmd);

    pid = fork();

    if (pid < 0)
    {
        perror("fork");
        return -1;
    }

    if (pid == 0)
    {
        restore_default_signals();   /* child gets default signal handling */
        setup_redirections(cmd);
        execvp(cmd->argv[0], cmd->argv);
        fprintf(stderr, "%s: command not found\n", cmd->argv[0]);
        exit(1);
    }

    if (!background)
    {
        int status = 0;
        while (1)
        {
            pid_t result = waitpid(pid, &status, WUNTRACED);
            if (result == -1)
            {
                if (errno == EINTR)
                    continue;
                if (errno == ECHILD)
                    break;          /* already reaped by SIGCHLD */
                perror("waitpid");
                return -1;
            }
            if (WIFSTOPPED(status))
            {
                /* Child stopped by Ctrl-Z (SIGTSTP) – return to prompt. */
                printf("[%d] stopped\n", pid);
                return 0;
            }
            if (WIFEXITED(status) || WIFSIGNALED(status))
            {
                /* Child terminated normally or by a signal. */
                return status;
            }
            /* Unexpected waitpid result – just loop. */
        }
        return status;
    }
    else
    {
        printf("[%d] %d\n", next_job_number++, pid);
        return 0;
    }
}

/*
 * Executes a pipeline of `num_cmds` commands connected by pipes.
 * Each stage is forked, has its stdin/stdout wired to the
 * appropriate pipe end, gets its own redirections and wildcard
 * expansion applied, and then execvp()s. The parent closes all
 * pipe ends and waits for every stage to finish.
 *
 * Falls back to execute_single_command() when given just one
 * command, so callers can use this uniformly. Returns 0 on success
 * or -1 if pipe()/fork() fails partway through setup.
 */
int execute_pipeline(Command *cmds[], int num_cmds)
{
    int pipes[MAX_PIPELINE - 1][2];
    pid_t pids[MAX_PIPELINE];
    int i;

    if (num_cmds <= 0)
        return -1;

    if (num_cmds == 1)
        return execute_single_command(cmds[0], 0);

    for (i = 0; i < num_cmds - 1; i++)
    {
        if (pipe(pipes[i]) < 0)
        {
            perror("pipe");
            return -1;
        }
    }

    for (i = 0; i < num_cmds; i++)
    {
        pids[i] = fork();

        if (pids[i] < 0)
        {
            perror("fork");
            return -1;
        }

        if (pids[i] == 0)
        {
            restore_default_signals();
            expand_wildcards(cmds[i]);

            /* Wire stdin to the previous stage's read end, and
             * stdout to this stage's write end, except at the
             * pipeline's two ends. */
            if (i > 0)
            {
                if (dup2(pipes[i - 1][0], STDIN_FILENO) < 0)
                {
                    perror("dup2 stdin");
                    exit(1);
                }
            }
            if (i < num_cmds - 1)
            {
                if (dup2(pipes[i][1], STDOUT_FILENO) < 0)
                {
                    perror("dup2 stdout");
                    exit(1);
                }
            }

            /* Every pipe fd (both ends, all stages) must be closed
             * in the child once dup2() has copied the ones it
             * actually needs - otherwise stages downstream never
             * see EOF on their stdin because a write end is still
             * open somewhere. */
            for (int j = 0; j < num_cmds - 1; j++)
            {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            setup_redirections(cmds[i]);

            if (cmds[i]->argv[0] != NULL)
            {
                execvp(cmds[i]->argv[0], cmds[i]->argv);
                fprintf(stderr, "%s: command not found\n", cmds[i]->argv[0]);
            }
            exit(1);
        }
    }

    /* Parent: close every pipe fd (it doesn't read/write any of
     * them) and wait for all stages to finish. */
    for (i = 0; i < num_cmds - 1; i++)
    {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    for (i = 0; i < num_cmds; i++)
    {
        int status = 0;
        while (waitpid(pids[i], &status, 0) < 0)
        {
            if (errno == ECHILD)
            {
                /* See the matching comment in execute_single_command():
                 * the SIGCHLD handler may have already reaped this
                 * stage. */
                break;
            }
            if (errno != EINTR)
            {
                perror("waitpid");
                return -1;
            }
        }
    }

    return 0;
}

/*
 * Convenience wrapper: executes a single command, honouring its
 * own `background` flag.
 */
void execute_command(Command *cmd)
{
    execute_single_command(cmd, cmd->background);
}
