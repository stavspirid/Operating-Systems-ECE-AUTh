# TinyShell
*TinyShell* is a very simple (and tiny) Unix shell designed during the course Operating Systems at ECE AUTh winter semester 2025-6.
## Quick Start Guide
Clone the repository from GitHub and go to the `Ex_1` directory (skip this step if already downloaded):
```shell
git clone https://github.com/stavspirid/Operating-Systems-ECE-AUTh.git
cd Ex_1
```

Run the `Makefile` to build the *TinyShell* executable:
```shell
make
```
Checkout the `Makefile` documentation or run `make help` for other ways to build and install the program.

Now you can run *TinyShell* like this inside the current directory:
```shell
./tinyshell
```

All commands (without `sudo` needs) in `$PATH` can now be run inside the *TinyShell* like this:
```shell
ls -la          // Will list the names of all files inside current Unix directory
mkdir test_dir  // Will create a directory named "test_dir"
```

You can exit the *TinyShell* be pressing `Ctrl + D` or by typing `exit`.

## Requirements 
- *Compiler*: g++ with C++11 support
- *Platform*: Linux or WSL
- *Build Tool*: GNU Make

## Documentation
C++ was used for this project so an `std::vector` can be utilized to store the shell's input and an `std::string` can be used for text handling.
C would be faster, simpler and maybe easier to debug since it is very close to POSIX APIs but since this is an educational project, this choice was not that important.

*TinyShell* showcases how shells work under the hood by implementing core functionality such as forking processes, executing binaries, and managing child process lifecycle.

### Build from Source
```shell
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

### Source Code
*TinyShell* implements a simple Read-Eval-Print Loop (REPL) with the following components:
- `parseCommand`: turns the shell's input into a vector of tokens (string) with the first one being a command and each one of the rest being an argument  
- `findInPath`: Searches `$PATH` environment variable to find executable binaries with the same name as the command. If the command is a path, it searches for this exact path to find an executable binary. In case a specific binary needs some not yet granted permissions, the command cannot run (returns empty string).
- `executeCommand`: Forks child processes and executes commands via `execve()`. It is designed to return POSIX type error codes in case of an unsuccessful process or an abnormal exit.
- `vectorToArgv`, `freeArgv`: Are used for memory management. The convert C++ input vector into simple 2D char pointers that are recognized by `execve()`

## Project Limitations
*TinyShell* does not currently support these listed functionalities:
- Piping (`|`)
- Redirection (`>`, `<`, `>>`)
- `cd` command
- Command history (using arrows)
- Tab completion
