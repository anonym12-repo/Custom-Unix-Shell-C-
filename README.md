# Custom Unix Shell (C)

A POSIX-style interactive Unix shell written from scratch in C as part of an
operating systems course assignment (ICT374, Assignment 2 — Part 1). It
implements a raw-mode line editor with history navigation, quote/escape-aware
parsing, wildcard expansion, I/O redirection, pipelines, background jobs, and
signal handling — all built directly on `fork()`/`execvp()` rather than
wrapping another shell.

## Features

- **Line editing & history** — raw-mode terminal input with Left/Right cursor
  movement, Backspace, and Up/Down arrow recall through the last 100 commands.
  Supports multi-line redraw when a command line exceeds the terminal width.
- **History expansion** — `!!` repeats the last command, `!n` repeats command
  number `n`, `!prefix` repeats the most recent command starting with
  `prefix`. The expanded text (not the shorthand) is echoed and stored in
  history.
- **Parsing** — a character-by-character state machine (not `strtok()`) that
  correctly handles single quotes (literal), double quotes (with `\n`, `\t`,
  `\\`, `\"` escapes), backslash escaping outside quotes, `$VAR` environment
  variable expansion, and whitespace tokenization.
- **Built-in commands** — `cd`, `pwd`, `exit [status]`, `prompt [text]`,
  `history`. Built-ins run in the shell process itself since they need to
  mutate shell state (e.g. `cd`'s working directory).
- **External commands** — resolved via `PATH` using `fork()` + `execvp()`.
- **Wildcard expansion** — `*` and `?` patterns expanded with `glob()`; an
  unmatched pattern is passed through literally, matching standard shell
  behavior.
- **I/O redirection** — `<`, `>`, `>>`, `2>`, `2>>`, composable with pipelines.
- **Pipelines** — arbitrary-length `cmd1 | cmd2 | ... | cmdN` chains, each
  stage its own process with pipes wired via `dup2()`.
- **Sequencing & background jobs** — `;` runs commands sequentially; `&` runs
  a command in the background and immediately prints its PID.
- **Signal handling** — the shell ignores `SIGINT`/`SIGQUIT`/`SIGTSTP` so
  Ctrl-C/Ctrl-\/Ctrl-Z don't kill the shell itself; child processes restore
  default signal handling before `execvp()`. A `SIGCHLD` handler reaps
  terminated children with `waitpid(WNOHANG)` to prevent zombies.

## Skills demonstrated

This project involved building a non-trivial systems program from the ground
up rather than assembling from libraries, which exercised a range of
low-level and software-engineering skills:

- **Systems programming in C** — direct use of POSIX APIs (`fork`, `execvp`,
  `waitpid`, `pipe`, `dup2`, `glob`, `termios`, `sigaction`) to implement
  process creation, inter-process communication, and terminal control
  without higher-level abstractions.
- **Operating systems concepts applied in practice** — process lifecycle and
  process images, file descriptor management, signal delivery and handling,
  and zombie-process reaping — concepts implemented and debugged directly
  rather than only studied theoretically.
- **Parsing & compiler-adjacent design** — a hand-written character-by-character
  state machine for tokenizing shell input (quoting, escaping, variable
  expansion), the same class of technique used in lexers/tokenizers more
  broadly.
- **Concurrency & IPC** — multi-process pipelines with correct pipe
  file-descriptor lifecycle management (closing unused ends so EOF
  propagates), and safe concurrent child-process reaping via signal handlers.
- **Debugging low-level, non-deterministic systems** — diagnosing and fixing
  race-condition-prone terminal redraw bugs, signal-handling edge cases, and
  memory issues in a program where bugs often only manifest interactively.
- **Modular software architecture** — decomposing a single-purpose program
  into independently testable modules (parsing, execution, I/O, signals,
  history, terminal handling) connected through a shared data structure
  (`Command`), rather than one monolithic file.
- **Memory management in C** — manual allocation/lifetime management for
  heap-allocated command buffers and `glob()`-expanded strings, including
  identifying and documenting a known leak rather than leaving it silent.
- **Build tooling** — a hand-written `Makefile` with explicit dependency
  tracking per translation unit, compiling warning-free under
  `-Wall -Wextra`.
- **Technical documentation** — a self-diagnosis writeup evaluating what was
  implemented, what was out of scope, and the trade-offs behind key design
  decisions (e.g. state machine vs. `strtok()`, `fork`/`exec` vs. `system()`).

These are the same fundamentals (memory safety, concurrency, IPC, debugging
close to the OS) that underpin performance-sensitive backend systems, ML
infrastructure/runtime work, and other systems-adjacent AI engineering roles.

## Project structure

The shell is split into modules by responsibility:

| File | Responsibility |
|---|---|
| `main.c` | Entry point; owns the read → expand-history → split → parse → execute loop |
| `shell.h` | Shared constants (`MAX_ARGS`, `MAX_LINE`, `MAX_COMMANDS`, `MAX_PIPELINE`, `PROMPT_MAX`) and the `Command` struct |
| `terminal.c` / `terminal.h` | Raw-mode terminal input, line editing, cursor/redraw logic |
| `parser.c` / `parser.h` | Tokenizing a line into `Command`s: quoting, escaping, `$VAR` expansion, redirection tokens, `;`/`&` splitting, `\|` pipeline splitting |
| `builtin.c` / `builtin.h` | Built-in command implementations (`cd`, `pwd`, `exit`, `prompt`, `history`) |
| `execute.c` / `execute.h` | Wildcard expansion, redirection setup, `fork`/`execvp` for single commands and pipelines |
| `signals.c` / `signals.h` | Shell-level signal handling and zombie reaping |
| `history.c` / `history.h` | Command history storage, retrieval, and `!`-expansion |
| `Makefile` | Builds all modules into the `shell` executable |

## Building

```bash
make        # builds the `shell` executable
make clean  # removes object files and the executable
```

Requires `gcc` and a POSIX environment (developed and tested on Linux/WSL).
Compiles cleanly with `-Wall -Wextra -std=c11`.

## Running

```bash
./shell
```

You'll be dropped into an interactive prompt (`% ` by default). Example
session:

```
% ls *.c | grep parser
parser.c
% echo hello > out.txt
% cat out.txt
hello
% sleep 5 &
[1] 12345
% history
1  ls *.c | grep parser
2  echo hello > out.txt
3  cat out.txt
4  sleep 5 &
% !!
history
...
% exit 0
```

## Known limitations

This was built as a course assignment with a defined scope, so some
shell features found in bash/zsh are intentionally out of scope:

- No `jobs`/`fg`/`bg` job control — background and stopped processes aren't
  tracked beyond a startup message.
- History is in-memory only (capped at 100 entries) and is not persisted
  across sessions.
- Only arrow keys and Backspace are handled during line editing; Delete,
  Home, End, etc. are ignored.
- No aliases, no script/batch mode (`-c` flag or shell scripts), no here-docs
  (`<<`).
- Strings produced by `glob()` during wildcard expansion aren't explicitly
  freed before `execvp()` (negligible for short-lived commands, since the
  child's address space is replaced immediately after).

## Design notes

- **State-machine parser over `strtok()`**: `strtok()` splits on every
  delimiter uniformly, so it can't distinguish a space that separates
  arguments from a space inside a quoted string. The state machine tracks
  quote/escape context explicitly, which is what lets `"hello world"` parse
  as a single token.
- **`glob()` over hand-rolled wildcard matching**: delegates pattern
  matching, hidden-file handling, and filesystem edge cases to a
  battle-tested libc function instead of reimplementing them.
- **`fork()`/`execvp()` over `system()`**: gives direct control over file
  descriptors before the new process image loads, which is what makes
  redirection and pipelines possible without shelling out to another shell.
- **Raw terminal mode**: canonical mode only delivers input line-by-line,
  which can't support arrow-key history recall or in-line cursor editing.
  Raw mode delivers keystrokes immediately, at the cost of the shell having
  to handle its own echoing and redrawing.
