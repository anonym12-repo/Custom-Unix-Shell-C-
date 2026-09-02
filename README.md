# 🐚 Unix Shell (C, Systems Programming)

### Modular Command-Line Shell with Process Management, Pipelines, Redirection & Job Control

[![C](https://img.shields.io/badge/C-Systems%20Programming-A8B9CC.svg)](#tech-stack)
[![POSIX](https://img.shields.io/badge/POSIX-Unix%20APIs-555555.svg)](#tech-stack)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](#license)

## 📌 Overview

A modular Unix-style command-line shell built in **C** using POSIX system calls and low-level process-management concepts. The shell provides an interactive environment for executing external programs while supporting built-in commands, command parsing, wildcard expansion, process creation, signal handling, background execution, input/output/error redirection, pipelines, and command history.

The project is designed around the core mechanisms behind Unix shells rather than relying on an existing shell framework. Commands are parsed into structured representations, processes are created using `fork()`, programs are launched with `execvp()`, pipelines are connected using `pipe()`, and file descriptors are redirected using `dup2()`.

## 🎯 Relevant Skills Demonstrated

* **Unix process management:** creates child processes with `fork()`, replaces child processes with external programs using `execvp()`, and synchronizes foreground processes using `waitpid()`.
* **Systems-level programming:** works directly with POSIX system calls including `fork()`, `execvp()`, `waitpid()`, `pipe()`, file-descriptor operations, and signal handling rather than relying on high-level process abstractions.
* **Command parsing:** converts raw command-line input into structured arguments while supporting whitespace, single/double quotes, and escape characters.
* **Process communication:** implements pipelines using Unix pipes so the standard output of one process can become the standard input of another.
* **File descriptor management:** supports standard input, output, and error redirection using low-level file operations and `dup2()`.
* **Signal handling:** separates shell and child-process signal behaviour and handles `SIGCHLD` to prevent terminated background processes from becoming zombies.
* **Background execution:** supports concurrent jobs using `&`, allowing the shell to remain responsive while a child process continues running.
* **Wildcard expansion:** uses the POSIX `glob()` functionality to expand patterns such as `*` and `?` before command execution.
* **Shell history:** maintains previously entered commands and supports history retrieval through the `history` command and prefix-based history execution.
* **Modular design:** separates parsing, execution, built-in commands, signal management, and shell definitions into independent source/header modules.

## ⚙️ How It Works

**Flow:** `User Input → Command Parsing → Operator/Argument Processing → Process Creation → Redirection/Pipeline Setup → Program Execution → Process Management`

* The **parser** converts raw input into a structured `Command` containing the executable path, argument count, and argument vector. It handles quoted strings, escaped characters, and whitespace.
* The **execution layer** expands wildcard expressions before creating the process and uses `fork()` to create a child process.
* In the **child process**, appropriate signal behaviour is restored and the requested program is launched using `execvp()`.
* For **foreground commands**, the parent waits for the child to finish using `waitpid()`.
* For **background commands**, the shell returns immediately to the prompt while the child continues executing.
* **Redirection** modifies standard file descriptors so commands can read from files or write output/error streams to files.
* **Pipelines** create multiple processes and connect their standard input/output streams through Unix pipes.
* **Signal handling** allows the interactive shell to remain protected from terminal signals while child processes receive normal Unix signal behaviour.
* **History management** stores previous commands and allows users to retrieve and re-execute commands without typing them again.

## 🔧 Supported Features

| Feature              | Description                                                                  |   |
| -------------------- | ---------------------------------------------------------------------------- | - |
| Built-in commands    | `cd`, `pwd`, `prompt`, `exit`                                                |   |
| External commands    | Execute programs available through the system `PATH`                         |   |
| Sequential execution | Run multiple commands using `;`                                              |   |
| Background execution | Run commands concurrently using `&`                                          |   |
| Input redirection    | Redirect standard input using `<`                                            |   |
| Output redirection   | Redirect standard output using `>`                                           |   |
| Error redirection    | Redirect standard error using `2>`                                           |   |
| Pipelines            | Connect commands using `                                                     | ` |
| Long pipelines       | Support multiple commands connected through several pipes                    |   |
| Wildcards            | Expand `*` and `?` patterns                                                  |   |
| Command history      | Display and reuse previously executed commands                               |   |
| Arrow-key history    | Navigate previously entered commands using terminal history controls         |   |
| Signal handling      | Manage terminal signals and child-process termination                        |   |
| Zombie prevention    | Reap terminated child processes using `SIGCHLD` handling                     |   |
| Complex commands     | Combine sequential/background jobs with pipelines, redirection and wildcards |   |

## 🛠️ Tech Stack

`C` · POSIX System Calls · `fork()` · `execvp()` · `waitpid()` · `pipe()` · `dup2()` · `open()` · `glob()` · Unix Signals · File Descriptors · Process Management · Command Parsing · Make

## 📁 Repository Structure

```text
├── src/
│   ├── main.c          # Shell entry point and interactive command loop
│   ├── shell.h         # Shared shell constants and command structure
│   ├── parser.c        # Command-line parsing and argument construction
│   ├── parser.h
│   ├── execute.c       # Process creation, execution and command handling
│   ├── execute.h
│   ├── builtin.c       # Built-in shell commands
│   ├── builtin.h
│   ├── signals.c       # Signal configuration and child-process handling
│   └── signals.h
├── tests/
│   ├── basic/          # Basic command execution tests
│   ├── redirection/    # Input/output/error redirection tests
│   ├── pipelines/      # Pipeline execution tests
│   ├── history/        # Command history tests
│   └── complex/        # Combined feature tests
├── Makefile
├── README.md
└── .gitignore
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

The shell can be run in a Unix/Linux environment or through a Linux environment such as WSL on Windows.

## 💻 Example Usage

### Basic Commands

```bash
% pwd
% ls
% cd /tmp
% prompt my-shell
```

### Sequential Execution

```bash
% pwd ; ls ; echo done
```

Commands are executed in order, with the shell continuing to the next command after each foreground process completes.

### Background Execution

```bash
% sleep 5 &
% echo "Shell remains responsive"
```

The background process continues running while the shell immediately accepts another command.

### Redirection

```bash
% ls > files.txt
% cat < files.txt
% command 2> errors.txt
```

### Pipelines

```bash
% ls | grep ".c"
```

Multiple commands can also be connected:

```bash
% cat file.txt | grep "error" | sort
```

### Wildcard Expansion

```bash
% ls *.c
% echo *.txt
```

Wildcard patterns are expanded before the command is executed.

### History

```bash
% history
% !ls
```

The history functionality allows previously entered commands to be displayed and reused.

### Combined Commands

```bash
% sleep 2 & ls > files.txt ; echo done
```

This demonstrates how background execution, output redirection, and sequential execution can be combined within a single command line.

## 🧪 Testing

The shell is tested progressively from individual features to combined command scenarios.

Examples include:

```text
Basic command execution
Built-in commands
Quoted and escaped arguments
Sequential execution (;)
Background execution (&)
Input/output/error redirection
Single pipelines
Multi-stage pipelines
Wildcard expansion
Command history
Arrow-key history
Multiple jobs in one command line
Combined pipelines, redirection and wildcards
Interrupted system calls
Zombie-process prevention
```

Testing complex combinations is particularly important because multiple features interact through process creation, file descriptors, signals, and command parsing.

## 🧠 Design Decisions

* **Modular source files:** parsing, execution, built-ins, and signal management are separated to keep responsibilities focused and make the code easier to extend.
* **POSIX process model:** external programs are executed through the traditional Unix `fork()` → `execvp()` process model.
* **Structured commands:** parsed input is represented using a `Command` structure rather than passing raw strings directly to the execution layer.
* **Parent/child signal separation:** the interactive shell and spawned child processes use different signal behaviour so terminal interrupts can be handled appropriately.
* **File-descriptor based I/O:** redirection and pipelines operate through Unix file descriptors, reflecting how real command-line shells connect processes.
* **Zombie prevention:** terminated child processes are reaped so that background execution does not accumulate zombie processes.

## 🔮 Future Work

* Improve the parser into a dedicated shell grammar capable of handling more complex quoting and operator combinations.
* Add stronger job-control support, including process groups and foreground/background job management.
* Improve error reporting for malformed operators, missing files, invalid pipelines, and unsupported command syntax.
* Add automated regression tests for command parsing and execution.
* Improve memory management around dynamically expanded wildcard arguments.
* Add persistent history storage between shell sessions.
* Support additional shell features such as environment-variable expansion and more advanced command substitution.

## 📄 License

This project is available under the MIT License.

---

*This project demonstrates practical systems-programming concepts through the implementation of a Unix-style command-line shell in C.*
