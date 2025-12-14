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
 * - Piping support
 * - Input/Output redirection
 * 
 * Author: TinyShell Project
 * Platform: Linux (POSIX-compliant systems)
 */

#include "tinyshell.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

ParsedCommand::ParsedCommand(){}
ParsedPipeline::ParsedPipeline(){}

std::vector<std::string> tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string token;
    
    while (iss >> token) {
        tokens.push_back(token);
    }
    
    return tokens;
}

ParsedPipeline parseCommandLine(const std::vector<std::string>& tokens) {
    ParsedPipeline result;
    ParsedCommand currentCmd;
    
    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i] == "|") {
            if (!currentCmd.args.empty()) {
                result.commands.push_back(currentCmd);
                result.hasPipes = true;
            }
            currentCmd = ParsedCommand();
        }
        else if (tokens[i] == ">") {	// Redirect Output
            if (i + 1 < tokens.size()) {
                currentCmd.outputFile = tokens[++i];
                currentCmd.appendMode = false;
            }
        }
        else if (tokens[i] == ">>") {	// Redirect and Append Output
            if (i + 1 < tokens.size()) {
                currentCmd.outputFile = tokens[++i];
                currentCmd.appendMode = true;
            }
        }
        else if (tokens[i] == "<") {	// Redirect for Input
            if (i + 1 < tokens.size()) {
                currentCmd.inputFile = tokens[++i];
            }
        }
        else if (tokens[i] == "2>") {	// Redirect Error Output
            if (i + 1 < tokens.size()) {
                currentCmd.errorFile = tokens[++i];
                currentCmd.appendErrorMode = false;
            }
        }
        else if (tokens[i] == "2>>") {	// Redirect and Append Error Output
            if (i + 1 < tokens.size()) {
                currentCmd.errorFile = tokens[++i];
                currentCmd.appendErrorMode = true;
            }
        }
        else {
            currentCmd.args.push_back(tokens[i]);
        }
    }
    
    if (!currentCmd.args.empty()) {
        result.commands.push_back(currentCmd);
    }
    
    return result;
}

std::string findInPath(const std::string& command) {
    if (command.find('/') != std::string::npos) {
        if (access(command.c_str(), X_OK) == 0) {
            return command;
        }
        return "";
    }
    
    const char* pathEnv = getenv("PATH");
    if (!pathEnv) return "";
    
    std::string pathStr(pathEnv);
    std::istringstream pathStream(pathStr);
    std::string dir;
    
    while (std::getline(pathStream, dir, ':')) {
        if (dir.empty()) continue;
        
        std::string fullPath = dir + "/" + command;
        if (access(fullPath.c_str(), X_OK) == 0) {
            return fullPath;
        }
    }
    
    return "";
}

char** vectorToArgv(const std::vector<std::string>& args) {
    char** argv = new char*[args.size() + 1];
    
    for (size_t i = 0; i < args.size(); ++i) {
        argv[i] = new char[args[i].length() + 1];
        strcpy(argv[i], args[i].c_str());
    }
    
    argv[args.size()] = nullptr;
    return argv;
}

void freeArgv(char** argv, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        delete[] argv[i];
    }
    delete[] argv;
}

int executeCommand(const ParsedCommand& cmd) {
    if (cmd.args.empty()) return 0;
    
    std::string execPath = findInPath(cmd.args[0]);
    if (execPath.empty()) {
        std::cerr << COLOR_ERROR << "tinyshell: command not found: " 
                  << cmd.args[0] << COLOR_RESET << "\n";
        return 127;
    }
    
    char** argv = vectorToArgv(cmd.args);
    pid_t pid = fork();
    
    if (pid < 0) {
        std::cerr << COLOR_ERROR << "tinyshell: fork failed" << COLOR_RESET << "\n";
        freeArgv(argv, cmd.args.size());
        return -1;
    }
    else if (pid == 0) {
        // Handle input redirection
        if (!cmd.inputFile.empty()) {	// If "<"
            int fd = open(cmd.inputFile.c_str(), O_RDONLY);
            if (fd < 0) {
                std::cerr << COLOR_ERROR << "tinyshell: cannot open input file\n" 
                          << COLOR_RESET;
                exit(1);
            }
            dup2(fd, STDIN_FILENO);	// Redirect stdin
            close(fd);
        }
        
        // Handle output redirection
        if (!cmd.outputFile.empty()) {	// If ">" or ">>"
			// Append or Truncate based on append flag of current command
            int flags = O_WRONLY | O_CREAT | (cmd.appendMode ? O_APPEND : O_TRUNC);
            int fd = open(cmd.outputFile.c_str(), flags, 0644);
            if (fd < 0) {
                std::cerr << COLOR_ERROR << "tinyshell: cannot open output file\n" 
                          << COLOR_RESET;
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);	// Redirect stdout
            close(fd);
        }
		
		if (!cmd.errorFile.empty()) {
            int flags = O_WRONLY | O_CREAT | (cmd.appendErrorMode ? O_APPEND : O_TRUNC);
            int fd = open(cmd.errorFile.c_str(), flags, 0644);
            if (fd < 0) {
                std::cerr << COLOR_ERROR << "tinyshell: cannot open error file\n" 
                          << COLOR_RESET;
                exit(1);
            }
            dup2(fd, STDERR_FILENO);	// Redirect stderr
            close(fd);
        }
        
        execve(execPath.c_str(), argv, environ);
        std::cerr << COLOR_ERROR << "tinyshell: execve failed" << COLOR_RESET << "\n";
        exit(1);
    }
    else {
        int status;
        waitpid(pid, &status, 0);
        freeArgv(argv, cmd.args.size());
        
        if (WIFEXITED(status)) {
            int exitCode = WEXITSTATUS(status);
            if (exitCode != 0) {
                std::cout << COLOR_INFO << "[Process exited with code: " 
                          << exitCode << "]" << COLOR_RESET << "\n";
            }
            return exitCode;
        }
        else if (WIFSIGNALED(status)) {
            int signal = WTERMSIG(status);
            std::cout << COLOR_ERROR << "[Process terminated by signal: " 
                      << signal << "]" << COLOR_RESET << "\n";
            return 128 + signal;
        }
    }
    
    return 0;
}

int executePipeline(const std::vector<ParsedCommand>& pipeline) {
    int numCmds = pipeline.size();
    std::vector<std::array<int, 2>> pipefds(numCmds - 1);	// [0]:read [1]:write
    
    // Create all pipes
    for (int i = 0; i < numCmds - 1; i++) {
        if (pipe(pipefds[i].data()) == -1) {
            std::cerr << COLOR_ERROR << "tinyshell: pipe failed" << COLOR_RESET << "\n";
            return -1;
        }
    }
    
    // Fork and execute each command
    for (int i = 0; i < numCmds; i++) {
        pid_t pid = fork();
        
        if (pid < 0) {
            std::cerr << COLOR_ERROR << "tinyshell: fork failed" << COLOR_RESET << "\n";
            return -1;
        }
        else if (pid == 0) {	// Child Process
            // Setup pipes			
			// If not first command: read from previous pipe
            if (i > 0) {
                dup2(pipefds[i-1][0], STDIN_FILENO);
            }
            
            // If not last command: write to current pipe
            if (i < numCmds - 1) {
                dup2(pipefds[i][1], STDOUT_FILENO);
            }
            
            // Close all pipe fds
            for (int j = 0; j < numCmds - 1; j++) {
                close(pipefds[j][0]);
                close(pipefds[j][1]);
            }
            
            // Handle input redirection
            if (!pipeline[i].inputFile.empty()) {
                int fd = open(pipeline[i].inputFile.c_str(), O_RDONLY);
                if (fd < 0) {
                    std::cerr << COLOR_ERROR << "tinyshell: cannot open input file\n" 
                              << COLOR_RESET;
                    exit(1);
                }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }
            
            // Handle output redirection
            if (!pipeline[i].outputFile.empty()) {
                int flags = O_WRONLY | O_CREAT | (pipeline[i].appendMode ? O_APPEND : O_TRUNC);
                int fd = open(pipeline[i].outputFile.c_str(), flags, 0644);
                if (fd < 0) {
                    std::cerr << COLOR_ERROR << "tinyshell: cannot open output file\n" 
                              << COLOR_RESET;
                    exit(1);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }
            
            // Find and execute
            std::string execPath = findInPath(pipeline[i].args[0]);
            if (execPath.empty()) {
                std::cerr << COLOR_ERROR << "tinyshell: command not found: " 
                          << pipeline[i].args[0] << COLOR_RESET << "\n";
                exit(127);
            }
            
            char** argv = vectorToArgv(pipeline[i].args);
            execve(execPath.c_str(), argv, environ);	// Execute
            
			// If command does not exist
            std::cerr << COLOR_ERROR << "tinyshell: execve failed" << COLOR_RESET << "\n";
            exit(1);
        }
    }
    
    // Parent: close all pipes
    for (int i = 0; i < numCmds - 1; i++) {
        close(pipefds[i][0]);
        close(pipefds[i][1]);
    }
    
    // Wait for all children
    for (int i = 0; i < numCmds; i++) {
        wait(NULL);
    }
    
    return 0;
}

void displayPrompt() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        std::cout << COLOR_PROMPT << "tinyshell:" << cwd << "$ " << COLOR_RESET;
    } else {
        std::cout << COLOR_PROMPT << "tinyshell$ " << COLOR_RESET;
    }
    std::cout.flush();
}

int main() {
    std::string line;
    
    std::cout << "=======================================  _____ _____ _____           _____ _____ _____ _____ \n";
    std::cout << "  Welcome to TinyShell                  |   __|     |   __|   ___   |  _  |  |  |_   _|  |  |\n";
    std::cout << "  Type 'exit' or press Ctrl+D to quit   |   __|   --|   __|  |___|  |     |  |  | | | |     |\n";
    std::cout << "======================================= |_____|_____|_____|         |__|__|_____| |_| |__|__|\n\n";

    while (true) {
        displayPrompt();
        
        if (!std::getline(std::cin, line)) {
            std::cout << "\nExiting TinyShell...\n";
            break;
        }
        
        if (line.empty() || line.find_first_not_of(" \t") == std::string::npos) {
            continue;
        }
        
        // Parse command line
        std::vector<std::string> tokens = tokenize(line);
        if (tokens.empty()) continue;
        
        ParsedPipeline pipeline = parseCommandLine(tokens);
        if (pipeline.commands.empty()) continue;
        
        // Check for exit command
        for (const auto& cmd : pipeline.commands) {
            if (!cmd.args.empty() && cmd.args[0] == "exit") {
                std::cout << "Exiting TinyShell...\n";
                return 0;
            }
        }
        
        // Execute
        if (!pipeline.hasPipes) {
            executeCommand(pipeline.commands[0]);
        } else {
            executePipeline(pipeline.commands);
        }
    }
    
    return 0;
}