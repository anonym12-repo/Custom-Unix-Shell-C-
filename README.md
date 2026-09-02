# Unix Shell in C

A lightweight Unix shell implemented from scratch in **C**, designed to demonstrate process management, command parsing, inter-process communication, file I/O, signal handling, and Unix system programming.

The project implements a command-line interpreter capable of launching external programs, managing processes, handling shell built-ins, expanding wildcards, executing commands sequentially or concurrently, redirecting standard streams, and constructing multi-stage pipelines.

---

## Overview

This project explores how a Unix shell works internally by implementing the core mechanisms normally provided by shells such as Bash.

Rather than relying on an existing shell through functions such as `system()`, commands are interpreted and executed directly using Unix/POSIX system calls and standard C libraries.

The shell follows a modular architecture separating:

* Command-line parsing
* Built-in command handling
* Process creation and execution
* Signal management
* Wildcard expansion
* Job execution
* I/O redirection
* Pipeline management
* Command history

The result is a compact but functional Unix shell that demonstrates how user commands are translated into processes and how those processes communicate with the operating system.

---

## Features

### Command Execution

* Execute external Unix commands
* Support commands available through the user's `PATH`
* Support command-line arguments
* Support long command lines with many arguments
* Use `fork()` to create child processes
* Use `execvp()` to replace child processes with requested programs
* Use `waitpid()` for process synchronization

Example:

```bash
% ls -l
% cat file.txt
% grep "error" logfile.txt
```

---

### Shell Built-ins

The shell provides built-in commands that execute directly within the shell process.

| Command   | Description                           |
| --------- | ------------------------------------- |
| `pwd`     | Display the current working directory |
| `cd`      | Change the current working directory  |
| `prompt`  | Change the shell prompt               |
| `history` | Display previously entered commands   |
| `exit`    | Terminate the shell                   |

Example:

```bash
% pwd
/home/user/project

% cd ..
% pwd
/home/user

% prompt shell$
shell$ 
```

`cd` is implemented as a shell built-in so that changing directories affects the shell process itself rather than a temporary child process.

---

### Command Parsing

The shell contains a dedicated command parser rather than relying entirely on simple whitespace tokenisation.

The parser supports:

* Multiple spaces between arguments
* Single-quoted strings
* Double-quoted strings
* Escaped characters
* Arguments containing spaces
* Arguments containing shell-special characters
* NULL-terminated argument arrays suitable for `execvp()`

Example:

```bash
% echo "hello world"
% echo 'hello world'
% echo hello\ world
```

Special characters can also be escaped when they should be interpreted literally.

---

### Wildcard Expansion

Filename wildcard expansion is supported using the POSIX `glob()` functionality.

Supported wildcard characters include:

* `*`
* `?`

Example:

```bash
% ls *.c
% ls file?.txt
```

A wildcard pattern is expanded into matching filenames before the command is executed.

If no matching files are found, the original argument can be retained rather than silently discarded.

---

### Sequential Execution

Commands can be separated using `;`.

The shell waits for each command to complete before starting the next command.

```bash
% sleep 2 ; echo done
```

Execution order:

```text
sleep 2
    ↓
command completes
    ↓
echo done
```

This demonstrates process synchronisation using `waitpid()`.

---

### Background / Concurrent Execution

Commands followed by `&` can execute in the background.

```bash
% sleep 10 & echo "Shell is still responsive"
```

The shell does not wait for the background process before accepting the next command.

This allows multiple processes to execute concurrently.

---

### Input and Output Redirection

The shell supports standard Unix stream redirection.

#### Standard input

```bash
% cat < input.txt
```

#### Standard output

```bash
% ls > files.txt
```

#### Standard error

```bash
% ls /nonexistent 2> error.txt
```

Redirection is implemented using file descriptors together with system calls such as:

```text
open()
dup2()
close()
```

The child process configures its standard streams before calling `execvp()`.

---

### Pipelines

Commands can be connected using the pipe operator `|`.

```bash
% ls | grep ".c"
```

The output of the first process becomes the input of the second process.

The shell uses Unix pipes and file descriptor duplication to connect processes:

```text
Process 1
   |
   | stdout
   v
+-------+
| pipe  |
+-------+
   |
   | stdin
   v
Process 2
```

---

### Multi-Stage Pipelines

The pipeline implementation supports multiple commands in a single job.

Example:

```bash
% cat file.txt | grep error | sort | uniq
```

The resulting process structure is conceptually:

```text
cat
 |
 v
grep
 |
 v
sort
 |
 v
uniq
```

Each process is connected using a separate Unix pipe.

This demonstrates practical use of:

* `pipe()`
* `fork()`
* `dup2()`
* `close()`
* `execvp()`
* `waitpid()`

---

### Command History

The shell maintains previously entered commands.

Supported history functionality includes:

```bash
% history
```

Commands can also be recalled using history expansion mechanisms such as:

```bash
% !!
% !12
% !ls
```

where applicable in the implemented version.

The history system maintains command strings and searches previous entries when a history request is made.

---

### Arrow-Key History

Interactive history can also support recalling previous commands using terminal arrow keys.

For example:

```text
↑
```

recalls the previous command, allowing the user to press Enter and execute it again.

This functionality requires handling terminal input at a lower level rather than relying solely on normal canonical line input.

---

### Signal Handling

The shell handles interactive signals so that pressing:

```text
Ctrl-C
Ctrl-\
Ctrl-Z
```

does not terminate or suspend the shell itself.

The shell configures signal handlers appropriately while child processes restore the default signal behaviour.

This provides different signal behaviour between:

```text
Shell process
     |
     | fork()
     v
Child process
```

The shell remains interactive while commands executed by children retain normal Unix signal behaviour.

---

### Zombie Process Prevention

Background processes can terminate independently of the shell.

To prevent terminated child processes from remaining as zombies, the shell handles `SIGCHLD` and repeatedly calls:

```c
waitpid(-1, &status, WNOHANG);
```

until there are no more terminated children to reclaim.

This demonstrates practical process lifecycle management.

---

## Architecture

The shell is divided into separate modules to keep parsing, execution, built-ins, and signal management independent.

```text
                    ┌──────────────────┐
                    │      main.c      │
                    │  Shell Loop      │
                    └────────┬─────────┘
                             │
                             v
                    ┌──────────────────┐
                    │    Parser        │
                    │    parser.c      │
                    └────────┬─────────┘
                             │
                ┌────────────┴────────────┐
                │                         │
                v                         v
       ┌─────────────────┐       ┌─────────────────┐
       │ Built-in Handler│       │ Command Executor│
       │   builtin.c     │       │   execute.c     │
       └─────────────────┘       └────────┬────────┘
                                          │
                    ┌─────────────────────┼───────────────────┐
                    │                     │                   │
                    v                     v                   v
               fork/exec             pipes/redirection   wildcards
                    │
                    v
             ┌──────────────┐
             │ Child Process│
             └──────────────┘

                    ┌─────────────────┐
                    │ Signal Handling │
                    │   signals.c     │
                    └─────────────────┘
```

---

## Process Execution Model

For an external command, the shell follows the standard Unix process model:

```text
User enters command
        |
        v
Command parser
        |
        v
Wildcard expansion
        |
        v
      fork()
       /   \
      /     \
 Parent     Child
   |          |
   |          +--> configure signals
   |          |
   |          +--> configure redirection/pipes
   |          |
   |          +--> execvp()
   |                 |
   |                 v
   |             Program runs
   |
   +--> waitpid() when foreground
```

This separates the shell process from the program being executed while allowing the shell to control process synchronisation.

---

## Key Unix/POSIX Concepts Demonstrated

This project provides practical implementation experience with:

### Process Management

* `fork()`
* `execvp()`
* `waitpid()`
* Process parent/child relationships
* Foreground and background processes
* Zombie process prevention

### Inter-Process Communication

* Anonymous pipes
* File descriptor management
* Process-to-process communication

### File I/O

* File descriptors
* `open()`
* `close()`
* `dup2()`
* Standard input/output/error streams

### Signal Processing

* `SIGINT`
* `SIGQUIT`
* `SIGTSTP`
* `SIGCHLD`
* Signal handler installation
* Parent/child signal behaviour

### Parsing

* Tokenisation
* Quoting
* Escape sequences
* Special operators
* Command structures
* Argument arrays

### Filesystem Interaction

* `getcwd()`
* `chdir()`
* `HOME` environment variable
* Filename pattern expansion using `glob()`

---

## Project Structure

A typical source tree is organised as follows:

```text
.
├── main.c
├── shell.h
│
├── parser.c
├── parser.h
│
├── execute.c
├── execute.h
│
├── builtin.c
├── builtin.h
│
├── signals.c
├── signals.h
│
├── history.c
├── history.h
│
├── Makefile
│
└── README.md
```

Additional modules can be added as the shell's parser and execution engine are extended.

---

## Building

The project uses GCC and a Makefile.

### Build

```bash
make
```

This produces the shell executable:

```text
shell
```

### Clean Build Files

```bash
make clean
```

### Manual Compilation

The project can also be compiled using GCC:

```bash
gcc -Wall -Wextra -std=c11 -o shell \
    main.c parser.c execute.c builtin.c signals.c history.c
```

Compiler warnings are enabled to help identify potential implementation problems during development.

---

## Running

Start the shell with:

```bash
./shell
```

You should then see the shell prompt:

```text
% 
```

Example session:

```text
% pwd
/home/user/unix-shell

% ls
Makefile
builtin.c
execute.c
main.c
parser.c
signals.c

% echo "Hello from the shell"
Hello from the shell

% exit
```

---

## Example Commands

### Built-ins

```bash
pwd
cd ..
prompt my-shell$
history
exit
```

### Wildcards

```bash
ls *.c
ls *.h
```

### Sequential commands

```bash
sleep 2 ; echo finished
```

### Background commands

```bash
sleep 10 & echo "ready"
```

### Redirection

```bash
ls > files.txt
cat < files.txt
ls /invalid-path 2> errors.txt
```

### Simple pipeline

```bash
ls | grep ".c"
```

### Multi-stage pipeline

```bash
cat file.txt | grep error | sort | uniq
```

### Combined operations

```bash
sleep 2 & ls > files.txt ; echo done
```

---

## Testing

The shell was designed to be tested at multiple levels, from individual commands to complex command lines combining several shell operators.

Testing covers areas such as:

* Basic command execution
* Built-in commands
* Argument parsing
* Quoted arguments
* Escaped characters
* Wildcard expansion
* Sequential execution
* Background execution
* Input/output/error redirection
* Single pipelines
* Multi-stage pipelines
* Command history
* Signal handling
* Zombie process prevention
* Complex combinations of shell operators
* Robustness against invalid commands and input

Example combined test:

```bash
% sleep 2 & ls > files.txt ; echo done
done

% cat files.txt
...
```

This verifies that background execution, output redirection, and sequential execution can coexist within the same command line.

---

## Design Decisions

### Modular Implementation

Functionality is separated into independent modules instead of placing the entire shell implementation inside `main.c`.

This makes the project easier to:

* Debug
* Extend
* Test
* Maintain
* Understand

### Direct Process Control

External programs are executed using Unix process primitives rather than delegating command execution to another shell.

The shell therefore directly controls:

```text
fork → configure process → exec
```

This provides practical experience with how shells interact with the operating system.

### Dedicated Parser

The parser processes the command line character-by-character so that it can distinguish between:

```text
ordinary characters
quoted characters
escaped characters
special operators
whitespace
```

This provides considerably more control than a simple `strtok()`-based implementation.

---

## Error Handling

The shell reports errors from important system operations such as:

* `fork()`
* `execvp()`
* `chdir()`
* `getcwd()`
* File operations
* Process synchronisation

For example, attempting to execute an unavailable command results in an execution error rather than silently failing.

---

## Limitations and Future Improvements

Potential future improvements include:

* More complete POSIX shell grammar
* Environment-variable expansion
* Command substitution
* Append redirection (`>>`)
* File descriptor duplication
* More complete quoting rules
* Improved terminal line editing
* Persistent history between sessions
* Job control with process groups
* `fg` / `bg` functionality
* Better error reporting
* Improved memory management for dynamically expanded arguments
* More comprehensive automated testing

The project intentionally focuses on implementing the core mechanisms of a Unix shell rather than reproducing every feature of Bash.

---

## Skills Demonstrated

This project demonstrates practical experience in:

**C Programming**

* Modular C development
* Pointers and arrays
* Structures
* Dynamic memory
* String manipulation
* Error handling
* Header/source organisation

**Operating Systems**

* Process creation
* Process lifecycle management
* Parent/child relationships
* Process synchronisation
* Signals
* File descriptors
* Inter-process communication

**Unix Systems Programming**

* `fork()`
* `execvp()`
* `waitpid()`
* `pipe()`
* `dup2()`
* `open()`
* `close()`
* `glob()`
* `signal()`

**Software Engineering**

* Modular architecture
* Separation of concerns
* Make-based builds
* Defensive programming
* Testing and debugging
* Code documentation
* Command-line software design

---

## Why This Project Matters

A shell sits directly at the boundary between a user and the operating system.

Building one from scratch provides a practical understanding of concepts that are often abstract when studied independently:

```text
User Input
    ↓
Parsing
    ↓
Process Creation
    ↓
File Descriptors
    ↓
IPC / Pipes
    ↓
Signals
    ↓
Process Synchronisation
    ↓
Program Execution
```

Instead of simply using Unix commands, this project implements the mechanisms required to interpret and execute those commands.

---

## License

This project is provided for educational and portfolio purposes.
