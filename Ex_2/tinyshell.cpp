/*
 * TinyShell - A Simple Unix Shell
 * 
 * A lightweight educational shell that demonstrates basic Unix process management
 * using POSIX system calls (fork, execve, waitpid).
 * 
 * Features:
 * - Interactive command prompt
 * - Command parsing with argument support
 * - PATH-based executable search
 * - Process creation and execution
 * - Exit code reporting
 * - EOF and 'exit' command support
 * 
 * Author: TinyShell Project
 * Platform: Linux (POSIX-compliant systems)
 */

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

// ANSI color codes for better visual feedback
#define COLOR_PROMPT "\033[1;32m"  // Green
#define COLOR_RESET "\033[0m"
#define COLOR_ERROR "\033[1;31m"   // Red
#define COLOR_INFO "\033[1;36m"    // Cyan

/**
 * Parse a command line into tokens (arguments)
 * 
 * @param line The input command line string
 * @return Vector of argument strings
 */
std::vector<std::string> parseCommand(const std::string& command) {
    std::vector<std::string> tokens;
    std::istringstream iss(command);
    std::string token;
    
    while (iss >> token) {
        tokens.push_back(token);
    }
    
    return tokens;
}

/**
 * Search for an executable in the PATH environment variable
 * 
 * @param command The command name to search for
 * @return Full path to the executable, or empty string if "Not Found"
 */
std::string findInPath(const std::string& command) {
	// Handle command calls with specific paths
    // If command contains '/', it's already a path
    if (command.find('/') != std::string::npos) {
		// Check if command exists and user has permission to execute it
        if (access(command.c_str(), X_OK) == 0) {
            return command;
        }
        return "";
    }
    
    // Get PATH environment variable
    const char* pathEnv = getenv("PATH");
    if (!pathEnv) {
        return "";
    }
    
    std::string pathStr(pathEnv);
    std::istringstream pathStream(pathStr);
    std::string dir;
    
    // Search each directory in PATH
	// Each directory is split by ':'
    while (std::getline(pathStream, dir, ':')) {
        if (dir.empty()) continue;
        
		// Add command to current path
        std::string fullPath = dir + "/" + command;
        
        // Check if file exists and is executable
        if (access(fullPath.c_str(), X_OK) == 0) {
            return fullPath;
        }
    }
    
    return "";	// Default
}

/**
 * Convert vector of strings to array of C-style strings for execve
 * 
 * @param args Vector of argument strings
 * @return Dynamically allocated array of char pointers (must be freed by caller)
 */
char** vectorToArgv(const std::vector<std::string>& args) {
    char** argv = new char*[args.size() + 1];
    
    for (size_t i = 0; i < args.size(); ++i) {
        argv[i] = new char[args[i].length() + 1];
        strcpy(argv[i], args[i].c_str());
    }
    
    argv[args.size()] = nullptr;  // NULL-terminate the array
    return argv;
}

/**
 * Free memory allocated for argv array
 * 
 * @param argv Array of C-style strings
 * @param size Number of elements in the array
 */
void freeArgv(char** argv, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        delete[] argv[i];
    }
    delete[] argv;
}

/**
 * Execute a command using fork and execve
 * 
 * @param args Vector of command arguments (first element is the command)
 * @return Exit code of the executed command, or -1 on error
 */
int executeCommand(const std::vector<std::string>& args) {
    if (args.empty()) {
        return 0;
    }
    
    // Handle custom 'exit' command
    if (args[0] == "exit") {
        std::cout << "Exiting TinyShell...\n";
        exit(0);	// System call - succesful termination
    }
	
	std::vector<std::string> cmdArgs;
	std::queue<std::string> input;
	std::queue<std::string> output;
	bool appendMode = false;
	
	for (size_t i = 0; i < args.size(); i++) {
		if (args[i] == ">") {
			if (i+1 < args.size() {
				output.push(args[i+1]);	// Set next argument as output
				appendMode = false;
				i++; // Skip filename argument
			} else {
				std:cerr << COLOR_ERROR << "tinyshell: syntax error near unexpected token `newline'" << COLOR_RESET << "\n";
                return -1;
			}
		}
		else if (args [i] = ">>") {
			if (i+1 < args.size() {
				output.push(args[i+1]);	// Set next argument as output
				appendMode = true;
				i++; // Skip filename argument
			} else {
				std:cerr << COLOR_ERROR << "tinyshell: syntax error near unexpected token `newline'" << COLOR_RESET << "\n";
                return -1;
			}
		}
		else if (args[i] == "<") {
			if (i+1 < args.size() {
				input.push(args[i+1]);	// Set next argument as input
				i++; // Skip filename argument
			} else {
				std:cerr << COLOR_ERROR << "tinyshell: syntax error near unexpected token `newline'" << COLOR_RESET << "\n";
                return -1;
			}
		} else {
			cmdArgs.push_back(args[i]);	// Treat all other arguments as commands
		}
	}
	
	if (cmdArgs.empty()) {
		return 0;
	}
	
    
    // Find the executable in PATH
    std::string execPath = findInPath(cmdArgs[0]);
    
	// If command does not exist
    if (execPath.empty()) {
        std::cerr << COLOR_ERROR << "tinyshell: command not found: " 
                  << cmdArgs[0] << COLOR_RESET << std::endl;
        return 127;		// Unix conversion for "command not found"
    }
    
    // Convert arguments to C-style array to use execve()
    char** argv = vectorToArgv(cmdArgs);
    
    // Fork a child process
    pid_t pid = fork();
    
    if (pid < 0) {
        // Fork failed
        std::cerr << COLOR_ERROR << "tinyshell: fork failed" 
                  << COLOR_RESET << std::endl;
        freeArgv(argv, cmdArgs.size());
        return -1;
    }
    else if (pid == 0) {
		
		// Handle input (<)
		if (!input.empty()) {
			int fd = open(input.front().c_str(),O_RDONLY, 0644);
			if (fd < 0) {
				perror("tinyshell: input error");
				exit 1;
			}
			input.pop();
			// Replace std input with file input
			dup2(fd, STDIN_FILENO);
			close(fd);
		}
		
		// Handle output (> or >>)
		if (!output.empty()) {
			int fd = open(output.front().c_str(), O_WRONLY|O_CREAT|O_TRUNC, 0644);
			if (fd < 0) {
				perror("tinyshell: input error");
				exit 1;
			}
			output.pop();
			// Replace std output with file output
			dup2(fd, STDOUT_FILENO);
			close(fd);
		}
		
		
		
        // Child process: execute the command
        execve(execPath.c_str(), argv, environ);
        
        // If execve returns, an error occurred
        std::cerr << COLOR_ERROR << "tinyshell: execve failed for " 
                  << cmdArgs[0] << COLOR_RESET << std::endl;
        freeArgv(argv, cmdArgs.size());
        exit(1);
    }
    else {
        // Parent process: wait for child to complete
        int status;
        waitpid(pid, &status, 0);
        
        freeArgv(argv, cmdArgs.size());
        
        // Extract and return exit code
        if (WIFEXITED(status)) {	// Child exited normally
            int exitCode = WEXITSTATUS(status);	// Check for success (0)
            if (exitCode != 0) {	// Unsuccessful process
                std::cout << COLOR_INFO << "[Process exited with code: " 
                          << exitCode << "]" << COLOR_RESET << std::endl;
            }
            return exitCode;
        }
        else if (WIFSIGNALED(status)) {	// Child extied abnormally
            int signal = WTERMSIG(status);
            std::cout << COLOR_ERROR << "[Process terminated by signal: " 
                      << signal << "]" << COLOR_RESET << std::endl;
            return 128 + signal;
        }
    }
    
    return 0;
}

/**
 * Display the shell prompt
 */
void displayPrompt() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        std::cout << COLOR_PROMPT << "tinyshell:" << cwd << "$ " 
                  << COLOR_RESET;
    }
	else {
        std::cout << COLOR_PROMPT << "tinyshell$ " << COLOR_RESET;
    }
    std::cout.flush();
}

/**
 * Main shell loop
 */
int main() {
    std::string line;
    
    std::cout << "=======================================  _____ _____ _____           _____ _____ _____ _____ \n";
    std::cout << "  Welcome to TinyShell                  |   __|     |   __|   ___   |  _  |  |  |_   _|  |  |\n";
    std::cout << "  Type 'exit' or press Ctrl+D to quit   |   __|   --|   __|  |___|  |     |  |  | | | |     |\n";
    std::cout << "======================================= |_____|_____|_____|         |__|__|_____| |_| |__|__|\n\n";

    while (true) {
        displayPrompt();
        
        // Read input line
        if (!std::getline(std::cin, line)) {
            // EOF detected (Ctrl+D)
            std::cout << "\nExiting TinyShell...\n";
            break;
        }
        
        // Skip empty lines
        if (line.empty() || line.find_first_not_of(" \t") == std::string::npos) {
            continue;
        }
        
        // Parse and execute command
        std::vector<std::string> args = parseCommand(line);
        if (!args.empty()) {
            executeCommand(args);
        }
    }
    
    return 0;
}
