/*
 * parser.c
 *
 * Converts a raw command line entered by the user into one or more
 * Command structures suitable for execution.
 *
 * Supports:
 *   - quoted strings (single and double)
 *   - escaped characters
 *   - whitespace tokenisation
 *   - command separators (; & to sequence, | to pipe)
 *   - redirections (< > 2> and >> 2>>)
 *   - environment variable expansion ($VAR)
 *
 * See parser.h for how the pieces fit together: split_commands()
 * splits on ';'/'&', parse_command() tokenises each resulting
 * piece, and has_pipeline()/split_pipeline() further split a
 * command on '|' into pipeline stages.
 */

#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>      /* for getenv */

#include "parser.h"


/*
 * Expand environment variables in a token.
 * If token starts with '$', replace with getenv() value.
 */
static void expand_env_var(char *token)
{
    if (token == NULL || token[0] != '$')
        return;
    const char *var = token + 1;
    char *val = getenv(var);
    if (val != NULL)
    {
        strncpy(token, val, MAX_LINE - 1);
        token[MAX_LINE - 1] = '\0';
    }
}

/*
 * Parses a command line entered by the user.
 */
int parse_command(char *line, Command *cmd)
{
    char token[MAX_LINE];
    int argc = 0;
    int pos = 0;

    int in_single = 0;
    int in_double = 0;
    int escape = 0;

    memset(cmd, 0, sizeof(Command));

    for (int i = 0;; i++)
    {
        char c = line[i];

        /* ---------- End of input ---------- */
        if (c == '\0' || c == '\n')
        {
            if (pos > 0)
            {
                token[pos] = '\0';
                expand_env_var(token);
                cmd->argv[argc++] = strdup(token);
            }
            break;
        }

        /* ---------- Escaped character ---------- */
        if (escape)
        {
            token[pos++] = c;
            escape = 0;
            continue;
        }

        /* ---------- Inside single quotes ---------- */
        if (in_single)
        {
            if (c == '\'')
            {
                in_single = 0;
            }
            else
            {
                token[pos++] = c;
            }
            continue;
        }

        /* ---------- Inside double quotes ---------- */
        if (in_double)
        {
            if (c == '"')
            {
                in_double = 0;
                continue;
            }

            if (c == '\\')
            {
                char next = line[i + 1];

                switch (next)
                {
                    case 'n':
                        token[pos++] = '\n';
                        i++;
                        break;

                    case 't':
                        token[pos++] = '\t';
                        i++;
                        break;

                    case 'r':
                        token[pos++] = '\r';
                        i++;
                        break;

                    case '\\':
                        token[pos++] = '\\';
                        i++;
                        break;

                    case '"':
                        token[pos++] = '"';
                        i++;
                        break;

                    case '$':
                        token[pos++] = '$';
                        i++;
                        break;

                    default:
                        token[pos++] = '\\';
                        break;
                }

                continue;
            }

            token[pos++] = c;
            continue;
        }

        /* ---------- Outside quotes ---------- */

        if (c == '\\')
        {
            escape = 1;
            continue;
        }

        if (c == '\'')
        {
            in_single = 1;
            continue;
        }

        if (c == '"')
        {
            in_double = 1;
            continue;
        }

        /* ---------- Whitespace ---------- */

        if (isspace((unsigned char)c))
        {
            if (pos > 0)
            {
                token[pos] = '\0';
                expand_env_var(token);
                cmd->argv[argc++] = strdup(token);
                pos = 0;
            }
            continue;
        }

        /* ---------- Redirection operators ---------- */

        if (c == '<' || c == '>')
        {
            if (pos > 0)
            {
                token[pos] = '\0';
                expand_env_var(token);
                cmd->argv[argc++] = strdup(token);
                pos = 0;
            }

            if (c == '<')
            {
                cmd->argv[argc++] = strdup("<");
                continue;
            }

            if (c == '>')
            {
                if (line[i + 1] == '>')
                {
                    cmd->argv[argc++] = strdup(">>");
                    i++;
                }
                else
                {
                    cmd->argv[argc++] = strdup(">");
                }

                continue;
            }
        }

        /* ---------- stderr redirection ---------- */

        if (c == '2' && line[i + 1] == '>')
        {
            if (pos > 0)
            {
                token[pos] = '\0';
                expand_env_var(token);
                cmd->argv[argc++] = strdup(token);
                pos = 0;
            }

            if (line[i + 2] == '>')
            {
                cmd->argv[argc++] = strdup("2>>");
                i += 2;
            }
            else
            {
                cmd->argv[argc++] = strdup("2>");
                i++;
            }

            continue;
        }

        /* ---------- Normal character ---------- */

        token[pos++] = c;

        if (argc >= MAX_ARGS - 1)
            break;

        if (pos >= MAX_LINE - 1)
            break;
    }

    cmd->argv[argc] = NULL;
    cmd->argc = argc;

    if (argc > 0)
        cmd->com_pathname = cmd->argv[0];
    else
        cmd->com_pathname = NULL;

    parse_redirections(cmd);
    cleanup_argv(cmd);

    return cmd->argc;
}

/*
 * Splits a raw input line into one or more Commands on the ';' and
 * '&' separators. A working copy of `line` is made so the caller's
 * original buffer (e.g. one that also gets pushed onto history) is
 * left untouched.
 *
 * Quote and escape state is tracked across the whole line (not
 * reset per-token) so that a separator inside a quoted string,
 * e.g. echo "a;b" ; echo c
 * is not mistaken for a command boundary.
 */
int split_commands(char *line, Command *cmds)
{
    int num_cmds = 0;
    char *line_copy;
    char *p;
    char *cmd_start;
    char separator;
    int in_quotes = 0;
    char quote_char = 0;
    int escape = 0;

    line_copy = strdup(line);
    if (line_copy == NULL)
        return 0;

    cmd_start = line_copy;
    p = line_copy;

    while (*p != '\0')
    {
        if (escape)
        {
            escape = 0;
            p++;
            continue;
        }
        if (*p == '\\')
        {
            escape = 1;
            p++;
            continue;
        }
        if (*p == '"' || *p == '\'')
        {
            if (!in_quotes)
            {
                in_quotes = 1;
                quote_char = *p;
            }
            else if (*p == quote_char)
            {
                in_quotes = 0;
                quote_char = 0;
            }
            p++;
            continue;
        }
        if (in_quotes)
        {
            p++;
            continue;
        }
        if (*p == ';' || *p == '&')
        {
            separator = *p;
            *p = '\0';

            /* Trim surrounding whitespace from this command's text. */
            char *token = cmd_start;
            while (*token == ' ' || *token == '\t')
                token++;
            char *end = token + strlen(token) - 1;
            while (end > token && (*end == ' ' || *end == '\t'))
                *end-- = '\0';

            if (strlen(token) > 0 && num_cmds < MAX_COMMANDS)
            {
                memset(&cmds[num_cmds], 0, sizeof(Command));
                parse_command(token, &cmds[num_cmds]);
                if (separator == '&')
                {
                    cmds[num_cmds].suffix = '&';
                    cmds[num_cmds].background = 1;
                }
                else
                {
                    cmds[num_cmds].suffix = ';';
                    cmds[num_cmds].background = 0;
                }
                num_cmds++;
            }
            p++;
            cmd_start = p;
            continue;
        }
        p++;
    }

    /* Final command after the last separator (if any text remains). */
    if (cmd_start != NULL && *cmd_start != '\0')
    {
        char *token = cmd_start;
        while (*token == ' ' || *token == '\t')
            token++;
        char *end = token + strlen(token) - 1;
        while (end > token && (*end == ' ' || *end == '\t'))
            *end-- = '\0';

        if (strlen(token) > 0 && num_cmds < MAX_COMMANDS)
        {
            memset(&cmds[num_cmds], 0, sizeof(Command));
            parse_command(token, &cmds[num_cmds]);
            cmds[num_cmds].suffix = '\0';
            cmds[num_cmds].background = 0;
            num_cmds++;
        }
    }

    free(line_copy);
    return num_cmds;
}

/*
 * Returns 1 if `cmd`'s argv contains a literal "|" token.
 */
int has_pipeline(Command *cmd)
{
    for (int i = 0; i < cmd->argc; i++)
        if (cmd->argv[i] != NULL && strcmp(cmd->argv[i], "|") == 0)
            return 1;
    return 0;
}

/*
 * Splits a single Command containing "|" tokens into a sequence of
 * heap-allocated Commands, one per pipeline stage. Redirections
 * that were attached to the original (unsplit) command are routed
 * to the correct stage:
 *   - redirect_in  -> first stage only
 *   - redirect_out -> last stage only
 *   - redirect_err -> first stage
 *     (this mirrors the original assignment's behaviour; stderr in
 *      a pipeline is a common source of ambiguity in real shells
 *      too, so this is a deliberate simplification, not a bug)
 */
int split_pipeline(Command *cmd, Command *pipeline_cmds[])
{
    int num_cmds = 0;
    int start = 0;

    for (int i = 0; i < cmd->argc; i++)
    {
        if (cmd->argv[i] != NULL && strcmp(cmd->argv[i], "|") == 0)
        {
            if (i > start)
            {
                pipeline_cmds[num_cmds] = malloc(sizeof(Command));
                if (pipeline_cmds[num_cmds] == NULL)
                    return -1;
                memset(pipeline_cmds[num_cmds], 0, sizeof(Command));
                pipeline_cmds[num_cmds]->argc = i - start;
                for (int j = 0; j < i - start; j++)
                    pipeline_cmds[num_cmds]->argv[j] = cmd->argv[start + j];
                pipeline_cmds[num_cmds]->argv[i - start] = NULL;
                pipeline_cmds[num_cmds]->com_pathname = pipeline_cmds[num_cmds]->argv[0];
                num_cmds++;
                start = i + 1;
            }
            else
            {
                /* Leading or doubled '|' with no command before it. */
                return -1;
            }
        }
    }

    if (start < cmd->argc)
    {
        pipeline_cmds[num_cmds] = malloc(sizeof(Command));
        if (pipeline_cmds[num_cmds] == NULL)
            return -1;
        memset(pipeline_cmds[num_cmds], 0, sizeof(Command));
        pipeline_cmds[num_cmds]->argc = cmd->argc - start;
        for (int j = 0; j < pipeline_cmds[num_cmds]->argc; j++)
            pipeline_cmds[num_cmds]->argv[j] = cmd->argv[start + j];
        pipeline_cmds[num_cmds]->argv[pipeline_cmds[num_cmds]->argc] = NULL;
        pipeline_cmds[num_cmds]->com_pathname = pipeline_cmds[num_cmds]->argv[0];

        pipeline_cmds[num_cmds]->redirect_out = cmd->redirect_out;
        pipeline_cmds[num_cmds]->append_out = cmd->append_out;

        /* num_cmds > 0 guards against dereferencing pipeline_cmds[0]
         * before it exists; in practice split_pipeline() is only
         * called after has_pipeline() confirms a '|' is present, so
         * this stage is never the very first Command allocated. */
        if (cmd->redirect_err != NULL && num_cmds > 0)
        {
            pipeline_cmds[0]->redirect_err = cmd->redirect_err;
            pipeline_cmds[0]->append_err = cmd->append_err;
        }

        if (num_cmds == 0)
        {
            /* Only one stage after all (no real '|' found before it). */
            pipeline_cmds[num_cmds]->redirect_in = cmd->redirect_in;
        }
        else
        {
            pipeline_cmds[0]->redirect_in = cmd->redirect_in;
            pipeline_cmds[num_cmds]->redirect_in = NULL;
        }
        num_cmds++;
    }

    return num_cmds;
}

/*
 * Parse redirection operators from the argument list
 */
void parse_redirections(Command *cmd)
{
    int i = 0;
    while (i < cmd->argc)
    {
        if (cmd->argv[i] == NULL)
            break;

        if (strcmp(cmd->argv[i], "<") == 0 && i + 1 < cmd->argc)
        {
            cmd->redirect_in = cmd->argv[i + 1];
            cmd->argv[i] = cmd->argv[i + 1] = NULL;
            i += 2;
        }
        else if ((strcmp(cmd->argv[i], ">") == 0 || strcmp(cmd->argv[i], ">>") == 0) && i + 1 < cmd->argc)
        {
            cmd->redirect_out = cmd->argv[i + 1];
            cmd->append_out = (strcmp(cmd->argv[i], ">>") == 0);
            cmd->argv[i] = cmd->argv[i + 1] = NULL;
            i += 2;
        }
        else if ((strcmp(cmd->argv[i], "2>") == 0 || strcmp(cmd->argv[i], "2>>") == 0) && i + 1 < cmd->argc)
        {
            cmd->redirect_err = cmd->argv[i + 1];
            cmd->append_err = (strcmp(cmd->argv[i], "2>>") == 0);
            cmd->argv[i] = cmd->argv[i + 1] = NULL;
            i += 2;
        }
        else
        {
            i++;
        }
    }
}

/*
 * Clean up argv by removing NULL entries from redirections
 */
void cleanup_argv(Command *cmd)
{
    int i;
    int new_argc = 0;
    char *new_argv[MAX_ARGS + 1];
    
    for (i = 0; i < cmd->argc; i++)
    {
        if (cmd->argv[i] != NULL)
        {
            new_argv[new_argc++] = cmd->argv[i];
        }
    }
    
    new_argv[new_argc] = NULL;
    
    /* Copy back */
    for (i = 0; i <= new_argc; i++)
        cmd->argv[i] = new_argv[i];
    
    cmd->argc = new_argc;
    
    /* Update com_pathname */
    if (new_argc > 0)
        cmd->com_pathname = cmd->argv[0];
    else
        cmd->com_pathname = NULL;
}

/*
 * Prints the parsed command structure for debugging
 */
void print_command(Command *cmd)
{
    int i;

    fprintf(stderr, "----- Command Dump -----\n");
    fprintf(stderr, "pathname: %s\n", cmd->com_pathname ? cmd->com_pathname : "(null)");
    fprintf(stderr, "argc: %d\n", cmd->argc);
    
    for (i = 0; i < cmd->argc; i++)
        fprintf(stderr, "argv[%d] = [%s]\n", i, cmd->argv[i] ? cmd->argv[i] : "(null)");
    
    fprintf(stderr, "redirect_in: %s\n", cmd->redirect_in ? cmd->redirect_in : "NULL");
    fprintf(stderr, "redirect_out: %s\n", cmd->redirect_out ? cmd->redirect_out : "NULL");
    fprintf(stderr, "redirect_err: %s\n", cmd->redirect_err ? cmd->redirect_err : "NULL");
    fprintf(stderr, "suffix: '%c'\n", cmd->suffix ? cmd->suffix : ' ');
    fprintf(stderr, "background: %d\n", cmd->background);
    fprintf(stderr, "------------------------\n");
}

