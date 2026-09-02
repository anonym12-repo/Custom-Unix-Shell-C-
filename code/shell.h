#ifndef SHELL_H
#define SHELL_H

#include <stddef.h>

/* ==========================================================
 * shell.h
 *
 * Shared constants and the core Command data structure used
 * by every module in the shell. Keeping all size limits here
 * (rather than scattered across individual .c files) means
 * there is a single place to look when tuning capacity.
 * ==========================================================
 */

/* Maximum number of command-line arguments supported by a single Command. */
#define MAX_ARGS 1000

/* Maximum length of an input command line. */
#define MAX_LINE 4096

/* Maximum number of commands chained on one line via ';' or '&'. */
#define MAX_COMMANDS 100

/* Maximum number of commands in a single pipeline (cmd1 | cmd2 | ...). */
#define MAX_PIPELINE 20

/* Maximum length of the shell prompt string, e.g. set via the
 * "prompt" builtin. */
#define PROMPT_MAX 100

/*
 * Structure representing a single command entered by the user.
 *
 * com_pathname : Name/path of the executable.
 * argc         : Number of arguments.
 * argv         : Array of argument strings (NULL terminated).
 * redirect_in  : File for stdin redirection (NULL if none)
 * redirect_out : File for stdout redirection (NULL if none)
 * redirect_err : File for stderr redirection (NULL if none)
 * append_out   : 1 if output redirection is append (>>)
 * append_err   : 1 if error redirection is append (2>>)
 * suffix       : Command separator that followed this command
 *                (';', '&', '|', or '\0' if none/end of line)
 * background   : 1 if this command should run as a background job
 */
typedef struct
{
    char *com_pathname;
    int argc;
    char *argv[MAX_ARGS + 1];
    char *redirect_in;
    char *redirect_out;
    char *redirect_err;
    int append_out;
    int append_err;
    char suffix;
    int background;
} Command;

#endif
