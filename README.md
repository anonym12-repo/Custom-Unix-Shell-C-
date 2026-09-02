# 🐚 Unix Shell (C, Systems Programming)

### Interactive POSIX-Style Shell with Process Management, Pipelines, Redirection, Wildcards & Command History

[![C](https://img.shields.io/badge/C-Systems%20Programming-A8B9CC.svg)](#tech-stack)
[![POSIX](https://img.shields.io/badge/POSIX-Unix%20APIs-555555.svg)](#tech-stack)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](#license)

## 📌 Overview

A modular interactive Unix shell implemented in **C** using low-level POSIX system calls and terminal interfaces. The shell provides an interactive command-line environment capable of executing external programs while supporting built-in commands, Unix-style command parsing, wildcard expansion, sequential and concurrent execution, input/output/error redirection, pipelines, command history, terminal history navigation, and signal handling.

Rather than invoking an existing shell through a high-level mechanism such as `system()`, the implementation manages the underlying Unix process model directly using `fork()`, `execvp()`, `waitpid()`, pipes, file descriptors, and signals.

The shell is designed around a **modular architecture**, separating command parsing, execution, built-in commands, signal handling, terminal interaction, and command history into dedicated modules.

## 🎯 Relevant Skills Demonstrated

* **Unix process management:** creates processes with `fork()`, launches external programs using `execvp()`, and manages foreground/background processes using `waitpid()`.
* **POSIX systems programming:** works directly with process, file-descriptor, pipe, signal, and terminal APIs rather than relying on an existing shell implementation.
* **Command-line parsing:** uses a character-by-character parser to recognise tokens, whitespace, quotes, escape sequences, operators, redirections, pipelines, and command separators.
* **Unix-style quoting and escaping:** supports single-quoted strings, double-quoted strings, and backslash escaping so special characters can be passed as literal arguments.
* **Command composition:** supports `;` for sequential execution and `&` for concurrent/background execution, including multiple jobs on a single command line.
* **Pipelines:** connects multiple processes using Unix pipes, supporting both simple pipelines and longer multi-stage pipelines.
* **File descriptor redirection:** redirects standard input, standard output, and standard error using `<`, `>`, and `2>`.
* **Wildcard expansion:** expands `*` and `?` filename patterns using the C `glob()` facility before command execution.
* **Command history:** stores previously entered commands and supports `history`, `!!`, numbered history such as `!12`, and prefix-based history such as `!ls`.
* **Interactive terminal handling:** supports Up/Down arrow navigation and command-line editing through terminal input handling.
* **Signal management:** protects the shell from terminal-generated `SIGINT`, `SIGQUIT`, and `SIGTSTP` while restoring normal signal behaviour for child processes.
* **Zombie-process prevention:** uses a `SIGCHLD` handler and `waitpid(..., WNOHANG)` to reap terminated child processes.
* **Robust error handling:** invalid commands, failed directory changes, missing redirection files, invalid history references, and malformed input are handled without terminating the shell.

## ⚙️ How It Works

**Flow:** `Terminal Input → History Expansion → Command Parsing → Job/Pipeline Processing → Process Creation → Redirection/Pipes → Program Execution → Process Management`

* The **main shell loop** displays the configurable prompt, reads interactive input, processes history references, stores commands in history, and coordinates execution.
* The **parser** processes input character-by-character and converts the command line into structured commands. It recognises whitespace, quoted strings, escaped characters, command separators, pipelines, redirections, and environment-variable expansion.
* A command line can contain multiple **jobs**, separated using `;` and `&`. A `;` causes the shell to wait before continuing, while `&` allows the preceding job to execute in the background.
* Each **job** can contain one or more commands connected through `|`. The shell creates the required processes and connects their standard streams using pipes.
* **Redirection** is configured before execution by opening the requested files and replacing the relevant standard file descriptors with `dup2()`.
* **Wildcard expansion** occurs before execution, allowing patterns such as `*.c` and `f?.txt` to be converted into matching filenames.
* **Built-in commands** such as `cd`, `pwd`, `prompt`, `history`, and `exit` execute within the shell process because they need to modify or access the shell's own state.
* **External commands** are executed in child processes using the `fork()` → `execvp()` process model.
* **Signal handling** keeps the interactive shell alive when terminal signals are generated while allowing foreground child processes to receive normal Unix signal behaviour.
* The `SIGCHLD` handler continuously reaps terminated children using `waitpid()` with `WNOHANG`, preventing completed background processes from remaining as zombies.

## 🔧 Supported Features

### Built-in Commands

| Command         | Purpose                                |
| --------------- | -------------------------------------- |
| `prompt <text>` | Changes the shell prompt               |
| `pwd`           | Displays the current working directory |
| `cd <path>`     | Changes the shell's working directory  |
| `cd`            | Returns to the user's home directory   |
| `history`       | Displays previously entered commands   |
| `exit`          | Terminates the shell                   |

### Command Processing

| Feature                  | Description                                                   |
| ------------------------ | ------------------------------------------------------------- |
| External commands        | Executes arbitrary programs using `execvp()`                  |
| Sequential execution `;` | Executes jobs sequentially and waits for foreground jobs      |
| Background execution `&` | Executes jobs concurrently without blocking the shell         |
| Input redirection `<`    | Redirects standard input from a file                          |
| Output redirection `>`   | Redirects standard output to a file                           |
| Error redirection `2>`   | Redirects standard error to a file                            |
| Simple pipelines `\|`    | Connects the output of one process to another                 |
| Long pipelines           | Supports multiple pipe stages within a single job             |
| Wildcard `*`             | Matches multiple characters in filenames                      |
| Wildcard `?`             | Matches a single character in filenames                       |
| Quoted strings           | Preserves spaces and special characters inside quotes         |
| Backslash escaping       | Allows special characters to be interpreted literally         |
| Environment variables    | Supports variable expansion such as `$HOME`                   |
| Complex command lines    | Combines multiple jobs, pipelines, redirections and wildcards |

### Interactive Features

| Feature             | Description                                                 |
| ------------------- | ----------------------------------------------------------- |
| Command history     | Stores commands entered during the session                  |
| `!!`                | Repeats the most recently executed command                  |
| `!N`                | Re-executes a command by its history number                 |
| `!string`           | Re-executes the most recent command beginning with a prefix |
| Up Arrow            | Navigates backward through command history                  |
| Down Arrow          | Navigates forward through command history                   |
| Backspace           | Removes characters during interactive input                 |
| Configurable prompt | Allows the prompt string to be changed at runtime           |

### Process & Signal Management

* Shell ignores `SIGINT` (`Ctrl-C`)
* Shell ignores `SIGQUIT` (`Ctrl-\`)
* Shell ignores `SIGTSTP` (`Ctrl-Z`)
* Child processes restore default signal behaviour
* Background children are reaped through `SIGCHLD`
* Foreground and background execution are handled independently
* Interrupted input/system calls are handled so the shell remains responsive
* Terminated background processes do not accumulate as zombies

## 🛠️ Tech Stack

`C` · POSIX APIs · `fork()` · `execvp()` · `waitpid()` · `pipe()` · `dup2()` · `open()` · `glob()` · `signal handling` · `termios` · `file descriptors` · `process management` · `command parsing` · `Make`

## 📁 Repository Structure

```text
├── code/
│   ├── main.c          # Shell entry point and main command loop
│   ├── shell.h         # Shared constants and core command structures
│   ├── parser.c        # Command-line parsing and command decomposition
│   ├── parser.h        # Parser interface
│   ├── execute.c       # Process creation, execution, redirection and pipelines
│   ├── execute.h       # Execution interface
│   ├── builtin.c       # Built-in shell commands
│   ├── builtin.h       # Built-in command interface
│   ├── signals.c       # Signal configuration and child-process reaping
│   ├── signals.h       # Signal handling interface
│   ├── terminal.c      # Interactive terminal input and arrow-key handling
│   ├── terminal.h      # Terminal interface
│   ├── history.c       # Command history storage and retrieval
│   └── history.h       # History interface
├── Makefile
└── doc
```

## 🚀 Running the Project

### Build

```bash
make
```

### Run

```bash
./shell
```

### Clean Build Files

```bash
make clean
```

The shell is intended to run in a Unix/Linux environment and can also be used through WSL on Windows.

The project is compiled using GCC with warnings enabled:

```bash
gcc -Wall -Wextra ...
```

## 💻 Example Usage

### Basic Commands

```bash
% pwd
% ls
% cd /tmp
% pwd
```

### Configurable Prompt

```bash
% prompt myshell$
myshell$ pwd
```

The new prompt remains active until it is changed again.

### Sequential Execution

```bash
% sleep 2 ; echo done
```

The shell waits for `sleep` to complete before executing `echo done`.

Multiple commands can be chained:

```bash
% echo one ; echo two ; echo three
```

### Background Execution

```bash
% sleep 5 &
% echo "Shell remains responsive"
```

The shell does not wait for the background process before accepting the next command.

Background and foreground jobs can also be combined:

```bash
% sleep 10 & ls
```

### Redirection

```bash
% ls -lt > files.txt
% cat < files.txt
% ls /does-not-exist 2> errors.txt
```

Input, output, and error streams can be redirected independently.

Multiple redirections can be combined:

```bash
% sort < infile > sorted_out 2> sort_err
```

### Pipelines

```bash
% ls -lt | wc -l
```

Longer pipelines are supported:

```bash
% cat /etc/passwd | cut -d: -f1 | sort | head -5
```

And:

```bash
% ls -la | grep "^-" | awk '{print $NF}' | sort | uniq
```

### Wildcards

```bash
% ls *.c
% ls f?.txt
```

Wildcard expressions are expanded before the command is executed. If a pattern has no matches, the original pattern is passed to the command rather than silently removed.

### Command History

```bash
% pwd
% ls
% echo test
% history
```

Previous commands can be re-executed:

```bash
% !!
% !3
% !ls
```

The shell also supports navigating through previous commands using the **Up** and **Down** arrow keys.

### Complex Commands

The shell supports multiple jobs and operators within a single command line:

```bash
% sleep 1 & echo A ; echo B & sleep 1 ; echo C
```

Features can also be combined across jobs:

```bash
% ls *.c > filelist.txt ; cat filelist.txt | wc -l & echo done
```

For example:

```bash
% sleep 2 & ls > files.txt ; echo done
```

Here, `sleep 2` runs in the background, `ls` redirects its output to `files.txt`, and `echo done` executes after the required foreground job completes.

## 🧪 Testing

The shell was tested from individual functionality through to combined and stress scenarios.

Testing covers:

```text
Basic command execution
Repeated command execution
Built-in commands
Prompt modification
Working-directory navigation
Invalid directory handling
Variable whitespace
Quoted strings
Escaped characters
Long command lines
Wildcard expansion using *
Wildcard expansion using ?
Wildcard patterns with no matches
Sequential execution (;)
Background execution (&)
Standard input redirection (<)
Standard output redirection (>)
Standard error redirection (2>)
Combined redirections
Simple pipelines
Multi-stage pipelines
Command history
!! history execution
!N history execution
!string prefix history
Arrow-key history navigation
Mixed ; and & command lines
Complex pipelines with redirection
Wildcard + pipeline combinations
Wildcard + redirection combinations
Background pipelines
Interrupted/slow system calls
Ctrl-C / SIGINT
Ctrl-\ / SIGQUIT
Ctrl-Z / SIGTSTP
Zombie-process reaping
Invalid commands
Invalid history references
Missing input files
Repeated error recovery
Long filenames
Deep directory navigation
Extended stress testing
```

The testing strategy specifically checks both **individual features and interactions between features**, since pipelines, redirection, background execution, signals, and parsing all depend on correct process and file-descriptor management.

## 🧠 Design Decisions

* **Modular architecture:** parsing, execution, built-ins, terminal handling, history, and signals are implemented as separate modules. This keeps each component focused and makes future changes easier.
* **Character-by-character parser:** command syntax is processed explicitly rather than relying on a simple whitespace split. This allows quoted strings, escaped characters, operators, redirections, and pipelines to be recognised correctly.
* **`fork()` + `execvp()` execution model:** external commands run as independent child processes while the shell remains responsible for process coordination.
* **Pipes for inter-process communication:** pipeline stages communicate directly through Unix pipes rather than temporary files.
* **File-descriptor redirection:** `dup2()` is used to replace standard input/output/error streams inside child processes.
* **`glob()` for wildcard expansion:** filename patterns are expanded by the shell before the command is passed to `execvp()`.
* **Dedicated terminal handling:** raw/non-canonical terminal input enables individual keystrokes to be processed for interactive history navigation.
* **Signal separation:** the shell ignores terminal-generated job-control signals while children restore the default behaviour, allowing commands such as `sleep` to respond normally to `Ctrl-C`, `Ctrl-\`, and `Ctrl-Z`.
* **`SIGCHLD` reaping:** completed background processes are collected using `waitpid(..., WNOHANG)` so they do not remain as zombie processes.
* **`SA_RESTART` for interrupted input:** signal handling is configured so input operations remain reliable when child-process signals occur while the shell is waiting for user input.

## 🔮 Future Work

* **Job control:** maintain a job table and add commands such as `jobs`, `fg`, and `bg` for managing background and suspended processes.
* **Persistent history:** save command history between shell sessions instead of keeping it only in memory.
* **Larger history buffer:** replace the fixed-size history storage with a dynamically managed or circular buffer.
* **Extended terminal editing:** add support for keys such as Delete, Home, End, Ctrl-A, and Ctrl-E.
* **Memory management improvements:** explicitly track dynamically allocated wildcard-expansion strings to prevent memory accumulation during long sessions.
* **Batch/script mode:** allow commands to be supplied through a file or command-line option rather than requiring an interactive terminal.
* **Additional shell functionality:** extend the parser and execution model with more advanced Unix shell features while preserving the modular architecture.



*An implementation-focused exploration of Unix process management, command parsing, inter-process communication, file descriptors, signals, and interactive terminal programming in C.*
