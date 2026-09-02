/*
 * signals.c
 *
 * Handles signal processing for the shell.
 * The shell ignores interactive signals such as
 * Ctrl-C, Ctrl-\ and Ctrl-Z, while child processes
 * restore the default behaviour.
 */

#define _POSIX_C_SOURCE 200809L

#include <signal.h>
#include <sys/wait.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include "signals.h"
/*
 * Reclaims terminated child processes.
 *
 * The handler repeatedly calls waitpid() with WNOHANG
 * to ensure all zombie processes are removed.
 * This prevents zombie processes from accumulating.
 */
static void sigchld_handler(int sig)
{
    (void)sig;

    int status;
    pid_t pid;

    /* Reap all dead children */
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        /* optional debug:
        printf("[reaped %d]\n", pid);
        */
    }

    /* pid == 0 -> no more children
       pid == -1 -> either no children or error (ignore ECHILD) */
}

/*
 * Installs all signal handlers required by the shell.
 */
void setup_shell_signals(void)
{
    struct sigaction sa;

    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);

    /* IMPORTANT FLAGS */
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    /* SA_RESTART = restart syscalls like read()
       SA_NOCLDSTOP = don't trigger SIGCHLD on Ctrl-Z stops */

    if (sigaction(SIGCHLD, &sa, NULL) == -1)
        perror("sigaction(SIGCHLD)");

    /* Shell ignores interactive signals */
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
}

/*
 * Restores default signal behaviour for child processes.
 * Called in child processes before execvp().
 */
void restore_default_signals(void)
{
    signal(SIGINT, SIG_DFL);   /* Ctrl-C: terminate child */
    signal(SIGQUIT, SIG_DFL);  /* Ctrl-\: terminate with core */
    signal(SIGTSTP, SIG_DFL);  /* Ctrl-Z: suspend child (default) */
    signal(SIGCHLD, SIG_DFL);
    signal(SIGTTOU, SIG_DFL);
    signal(SIGTTIN, SIG_DFL);
}

/*
 * Reap any remaining zombie processes.
 * This can be called periodically as a safety net.
 */
void reap_zombies(void)
{
    int status;
    pid_t pid;
    
    while (1)
    {
        pid = waitpid(-1, &status, WNOHANG);
        if (pid <= 0)
            break;
        /* Reap zombie */
    }
}