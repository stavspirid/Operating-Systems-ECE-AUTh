# TinyShell
_TinyShell_ is a very simple (hence tiny) Unix shell designed during the course Operating Systems at ECE AUTh winter semester 2025-6.

This is **version 3.0** of *TinyShell*. Learn more about what's new on the *Upgrades and Updates* section.
## Quick Start Guide
Clone the repository from GitHub (skip this step if already downloaded):
```shell
git clone https://github.com/stavspirid/Operating-Systems-ECE-AUTh.git
```
Run the `Makefile` to build the _TinyShell_ executable:
```shell
make
```
Checkout the `Makefile` documentation or run `make help` for other ways to build and install the program.

Now you can run _TinyShell_ like this inside the current directory:
```shell
./tinyshell
```
All commands (without `sudo` needs) in `$PATH` can now be run inside the _TinyShell_ like this:
```shell
ls -la          // Will list the names of all files inside current Unix directory
mkdir test_dir  // Will create a directory named "test_dir"
```

You can exit the _TinyShell_ be pressing `Ctrl + D` or by typing `exit`.
## Requirements
- _Compiler_: g++ with C++11 support
- _Platform_: Linux or WSL
- _Build Tool_: GNU Make
## Documentation
C++ was used for this project so an `std::vector` can be utilized to store the shell's input and an `std::string` can be used for text handling. C would be faster, simpler and maybe easier to debug since it is very close to POSIX APIs but since this is an educational project, this choice was not that important.

_TinyShell_ showcases how shells work under the hood by implementing core functionality such as forking processes, executing binaries, redirection, piping and managing child process lifecycle.

Information about every function and struct can be found in the header files (`*.hpp`).

### Module Responsibilities
| Module                 | Responsibility                          |
| ---------------------- | --------------------------------------- |
| **Parsing**            | `tokenize()`, `parseCommandLine()`      |
| **Path Resolution**    | `findInPath()`                          |
| **Execution**          | `executeCommand()`, `executePipeline()` |
| **Process Management** | `fork()`, `execve()`, `waitpid()`       |
| **I/O Redirection**    | `open()`, `dup2()`, `close()`           |
| **Piping**             | `pipe()`, file descriptor management    |
| **Job Control**        | `addJob()`, `removeJob()`, `getJob()`, `printJobs()`, job table management |
| **Signal Handling**    | `sigchld_handler()`, `sigtstp_handler()`, `sigint_handler()`                |
| **Built-in Commands**  | `builtin_fg()`, `builtin_bg()`, `builtin_jobs()`                            |
| **Shell Initialization** | `init_shell()`, `check_job_status_changes()`                              |


### Build from Source
```C
// Build the release version:
make
// Or build debug version:
make debug
// Build and immediately run TinyShell
make run
// Run this to remove build artifacts
make clean
// Install TinyShell to `/usr/local/bin` so can be ran from everywhere (requires sudo)
make install
// Remove from `/usr/local/bin` (requires sudo)
make uninstall
// Display available targets and usage
make help
```

If installed with `make install`, _TinyShell_ can be run from anywhere with just `tinyshell`

## Upgrades and Updates
### **Version 3**
#### **Job Control**
Full job control system implementation allowing users to manage multiple processes simultaneously. Commands can now run in the background and be controlled with built-in shell commands.

Key Features:
- **Background Execution (`&`)**: Launch processes in the background by appending `&` to any command
- **Job Table Management**: Track all running, stopped, and completed jobs with unique job IDs
- **Job States**: Jobs can be `Running`, `Stopped`, or `Done`

#### **Built-in Commands**
Three new built-in commands for job control:
- **`jobs`**: Display all current jobs with their status, job ID, and command
- **`fg [job_id]`**: Bring a background or stopped job to the foreground
- **`bg [job_id]`**: Resume a stopped job in the background

If no job ID is specified for `fg` or `bg`, the most recent job is used.

#### **Signal Handling**
Advanced signal handling for proper process control:
- **SIGCHLD**: Automatically detects when child processes change state (exit, stop, continue)
- **SIGTSTP (Ctrl+Z)**: Suspend the foreground process and move it to the background
- **SIGINT (Ctrl+C)**: Terminate the foreground process without affecting the shell

#### **Process Groups**
Implementation of process group management for proper terminal control:
- Each pipeline creates its own process group
- Shell maintains separate process groups for foreground and background jobs
- Terminal control is properly transferred between shell and foreground jobs

#### **Job Status Notifications**
Automatic notifications when background jobs:
- Complete successfully: `[job_id]+ Done    command`
- Stop execution: `[job_id]+ Stopped    command`
- The `+` indicator marks the current (most recent) job



---
### **Version 2**
#### **Redirection**
5 methods of redirection were implemented in the latest version (v2.0).
- `>` : Redirect Output
- `>>` : Redirect and Append Output
- `<` : Redirect for Input
- `2>` : Redirect Error Output
- `2>>` : Redirect and Append Error Output
#### **Piping**
Implementation of single and multi-stage piping. A combination of *Redirection* and *Piping* is now available.
#### **File Descriptors**
FDs were utilized throughout the latest version to offer file management through commands. 
0: Standard Input (Keyboard)
1: Standard Output (Screen)
2: Standard Error (Screen)

#### **Input Manipulation (Update)**
A new way to manipulate command line input is implemented using two new structures and refactoring previous codebase.
- `ParsedCommand`:
	Used to find commands, arguments and all redirections in a line.
	Think of it as: A single command with all its metadata
- `ParsedPipeline`:
	Used to split piped commands into simple commands.
	Think of it as: The complete command line, possibly with multiple commands

Redirection and Piping recognition happens in the `parseCommandLine` function.
Each command with its arguments is stored in a different vector so if piping is used, sequential execution and output pipe can be conducted.

`executeCommand` is the function that gets called if no pipes are detected in the command.
`executePipeline` is the function that gets called if there is at least one pipe in the command.
#### **Redirection Handler (v2.1)**
Implemented a seperate redirection handler for both command and pipeline execution so "Single Source of Truth" principle is followed. 

## Examples

### Redirection
```bash
echo "Zebra" > inputFile.txt
echo "Banana" >> inputFile.txt
echo "Elephant" >> inputFile.txt
cat inputFile.txt
```
Will result in:
```
=== inputfile.txt contents ===
> "Zebra"
> "Banana"
> "Elephant"
```
Extra step:
```bash
sort < inputFile.txt > sortedFile.txt
cat sortedFile.txt
```
Will result in:
```
=== inputfile.txt contents ===
> "Banana"
> "Elephant"
> "Zebra"
```
### Piping
Visualization of `parseCommandLine`:
```bash
// Input string:
"ls -la | grep txt | wc -l"
```

```bash
// After `parseCommandLine` (2D vector):
[
    ["ls", "-la"],      // Command 1
    ["grep", "txt"],    // Command 2
    ["wc", "-l"]        // Command 3
]
```
### Piping Examples
```bash
echo "hello world" | tr 'a-z' 'A-Z' | rev
```
-> `"DLROW OLLEH"`
### Piping + Direction
```bash
echo "hello world" | tr 'a-z' 'A-Z' | rev > file1.txt
```
-> file1.txt: `"DLROW OLLEH"`

### Error Redirection Handling
```bash
ls nonexistent 2> errors.txt
cat file.txt 2>> errors.txt
gcc program.c 2>> errors.txt
cat errors.txt
```
-> `errors.txt`:
```
ls: cannot access 'nonexistent': No such file or directory
cat: file.txt: No such file or directory
cc1: fatal error: program.c: No such file or directory
compilation terminated.
```
### Background Execution Tests 
```bash
tinyshell> sleep 30 &
[1] 12345
tinyshell> ps
tinyshell> jobs
[1]+ Running    sleep 30 &
```

### Signal Handling Tests
```bash
# Test Ctrl-C (SIGINT)
tinyshell> sleep 100
^C
tinyshell> # Should return to prompt

# Test Ctrl-Z (SIGTSTP)
tinyshell> sleep 100
^Z
[1]+ Stopped    sleep 100
```

### Job Control Command Tests
```bash
# Test fg command
tinyshell> sleep 100
^Z
[1]+ Stopped    sleep 100
tinyshell> fg %1
sleep 100

# Test bg command
tinyshell> bg %1
[1]+ Running    sleep 100 &
```

### Multiple Jobs Management
```bash
tinyshell> sleep 20 &
[1] 12345
tinyshell> sleep 30 &
[2] 12346
tinyshell> sleep 40 &
[3] 12347
tinyshell> jobs
[1]- Running    sleep 20 &
[2]- Running    sleep 30 &
[3]+ Running    sleep 40 &
```

## Project Limitations
_TinyShell_ does not currently support these listed functionalities:
- `cd` command
- Command history (using arrows)
- Tab completion
