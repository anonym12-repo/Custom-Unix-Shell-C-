#ifndef PARSER_H
#define PARSER_H

#include "shell.h"

/*
 * parser.h
 *
 * Everything involved in turning one raw line of user input into
 * one or more Command structures ready for execution:
 *
 *   raw line
 *     -> split_commands()   splits on ';' and '&' (sequencing)
 *       -> parse_command()  tokenises a single command's text
 *            (handles quoting, escapes, $VAR expansion,
 *             and redirection tokens via parse_redirections())
 *     -> has_pipeline() / split_pipeline()
 *                            further splits a single Command on '|'
 *                            into a pipeline of Commands
 */

/*
 * Tokenises a single command's text into a Command structure:
 * handles single/double quoting, backslash escapes, whitespace
 * splitting, $VAR expansion, and redirection operators
 * (< > >> 2> 2>>). Also invokes parse_redirections() and
 * cleanup_argv() internally, so on return `cmd` is fully populated
 * and ready to execute.
 *
 * Returns the number of argv entries parsed (equal to cmd->argc).
 */
int parse_command(char *line, Command *cmd);

/*
 * Splits a raw input line into one or more Commands on the ';' and
 * '&' separators, respecting single/double quotes and backslash
 * escapes so that separators inside quoted strings are not treated
 * as command boundaries. Each resulting Command is fully parsed via
 * parse_command().
 *
 * `cmds` must point to an array of at least MAX_COMMANDS Commands.
 * Returns the number of commands found.
 */
int split_commands(char *line, Command *cmds);

/*
 * Returns 1 if `cmd`'s argv contains a "|" token (i.e. it is really
 * a pipeline that needs to be split further), 0 otherwise.
 */
int has_pipeline(Command *cmd);

/*
 * Splits a single Command whose argv contains "|" tokens into a
 * sequence of heap-allocated Commands representing each stage of
 * the pipeline. Redirections attached to the original command are
 * routed to the correct end of the pipeline (input redirection to
 * the first stage, output/error redirection to the last stage).
 *
 * `pipeline_cmds` must point to an array of at least MAX_PIPELINE
 * Command* entries. The caller is responsible for freeing each
 * allocated Command once the pipeline has been executed.
 *
 * Returns the number of pipeline stages found, or -1 on error
 * (e.g. a leading/trailing/doubled '|').
 */
int split_pipeline(Command *cmd, Command *pipeline_cmds[]);

/*
 * Parse redirection operators from argv, storing the target
 * filenames in cmd->redirect_in / redirect_out / redirect_err and
 * removing the operator/filename tokens from argv.
 */
void parse_redirections(Command *cmd);

/*
 * Remove NULL entries from argv left behind by parse_redirections()
 * so argv is a properly NULL-terminated, contiguous array again.
 */
void cleanup_argv(Command *cmd);

/*
 * Displays the contents of a parsed command. Used for debugging.
 */
void print_command(Command *cmd);

#endif
