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
 * - Job control (fg, bg, jobs, CTRL+Z)
 * 
 * Author: TinyShell Project
 * Platform: Linux (POSIX-compliant systems)
 */

#include "tinyshell.hpp"
#include "utils.hpp"
#include "parser.hpp"
#include "jobs.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

// Global variables for job management
std::vector<Job> jobTable;
int nextJobId = 1;
volatile sig_atomic_t job_status_changed = 0;
pid_t shell_pgid;
int shell_terminal;
bool shell_is_interactive;

ParsedCommand::ParsedCommand(){}
ParsedPipeline::ParsedPipeline(){}

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

void setupRedirections(const ParsedCommand& cmd) {
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
    
    // Handle error redirection
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
}

// Signal handler for SIGCHLD - handles child process state changes
void sigchld_handler(int sig) {
    int saved_errno = errno;
    pid_t pid;
    int status;
    
    // Loop to handle all pending child status changes
    while (1) {
        // Retry waitpid if interrupted by another signal
        do {
            errno = 0;
            pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED);
        } while (pid < 0 && errno == EINTR);
        
        // No more children to reap
        if (pid <= 0) {
            errno = saved_errno;
            return;
        }
        
        Job* job = getJobByPgid(pid);
        
        if (job) {
            if (WIFEXITED(status) || WIFSIGNALED(status)) {
                job->state = DONE;
                job_status_changed = 1;
            } else if (WIFSTOPPED(status)) {
                job->state = STOPPED;
                job_status_changed = 1;
            } else if (WIFCONTINUED(status)) {
                job->state = RUNNING;
                job_status_changed = 1;
            }
        }
    }
}

// Check and print job status changes (called from main loop)
void check_job_status_changes() {
    if (!job_status_changed) {
        return;
    }
    
    job_status_changed = 0;
    
    // Check all jobs for status changes
    for (auto it = jobTable.begin(); it != jobTable.end(); ) {
        if (it->state == DONE && !it->notified) {
            // Print completion message in bash format (only once)
            std::cout << "[" << it->jobId << "]";
            if (it->is_current) {
                std::cout << "+";
            } else {
                std::cout << " ";
            }
            std::cout << " Done        " << it->command << std::endl;
            it->notified = true;
            it = jobTable.erase(it);
        } else {
            ++it;
        }
    }
}

// Signal handler for SIGTSTP (CTRL+Z)
void sigtstp_handler(int sig) {
    // Do nothing - let the foreground process group handle it
}

// Signal handler for SIGINT (CTRL+C)
void sigint_handler(int sig) {
    // Do nothing - let the foreground process group handle it
}

// Initialize shell - MUST be called before any job control operations
void init_shell() {
    shell_terminal = STDIN_FILENO;
    shell_is_interactive = isatty(shell_terminal);
    
    if (shell_is_interactive) {
        // Loop until we are in the foreground
        while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
            kill(-shell_pgid, SIGTTIN);
        
        // Setup signal handlers
        signal(SIGINT, sigint_handler);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTSTP, sigtstp_handler);
        signal(SIGTTIN, SIG_IGN);
        signal(SIGTTOU, SIG_IGN);
        signal(SIGCHLD, sigchld_handler);
        
        // Put shell in its own process group
        shell_pgid = getpid();
        if (setpgid(shell_pgid, shell_pgid) < 0) {
            perror("Couldn't put the shell in its own process group");
            exit(1);
        }
        
        // Take control of the terminal
        tcsetpgrp(shell_terminal, shell_pgid);
    }
}

// Built-in: fg command
int builtin_fg(const std::vector<std::string>& args) {
    Job* job = nullptr;
    
    if (args.size() > 1) {
        // Parse job specification: %N or just N
        std::string jobSpec = args[1];
        int jobId;
        
        if (jobSpec[0] == '%') {
            jobId = atoi(jobSpec.c_str() + 1);
        } else {
            jobId = atoi(jobSpec.c_str());
        }
        
        job = getJob(jobId);
        if (!job) {
            std::cerr << COLOR_ERROR << "tinyshell: fg: %" << jobId 
                      << ": no such job" << COLOR_RESET << "\n";
            return 1;
        }
    } else {
        // Get most recent job
        job = getMostRecentJob();
        if (!job) {
            std::cerr << COLOR_ERROR << "tinyshell: fg: current: no such job" 
                      << COLOR_RESET << "\n";
            return 1;
        }
    }
    
    // Mark this job as current
    markJobAsCurrent(job->jobId);
    
    // Print the command being foregrounded
    std::cout << job->command << std::endl;
    
    // Give terminal control to the job
    tcsetpgrp(shell_terminal, job->pgid);
    
    // Continue the job if it was stopped
    if (job->state == STOPPED) {
        kill(-job->pgid, SIGCONT);
    }
    
    job->state = RUNNING;
    
    // Wait for job to complete or stop
    int status;
    pid_t pid;
    while ((pid = waitpid(-job->pgid, &status, WUNTRACED)) > 0) {
        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            // Job terminated
            removeJob(job->jobId);
            break;
        } else if (WIFSTOPPED(status)) {
            // Job stopped (CTRL+Z) - print in bash format
            job->state = STOPPED;
            std::cout << "\n[" << job->jobId << "]";
            if (job->is_current) {
                std::cout << "+";
            } else {
                std::cout << " ";
            }
            std::cout << " Stopped         " << job->command << std::endl;
            break;
        }
    }
    
    // Restore terminal control to shell
    tcsetpgrp(shell_terminal, shell_pgid);
    
    return 0;
}

// Built-in: bg command
int builtin_bg(const std::vector<std::string>& args) {
    Job* job = nullptr;
    
    if (args.size() > 1) {
        // Parse job specification: %N or just N
        std::string jobSpec = args[1];
        int jobId;
        
        if (jobSpec[0] == '%') {
            jobId = atoi(jobSpec.c_str() + 1);
        } else {
            jobId = atoi(jobSpec.c_str());
        }
        
        job = getJob(jobId);
        if (!job) {
            std::cerr << COLOR_ERROR << "tinyshell: bg: %" << jobId 
                      << ": no such job" << COLOR_RESET << "\n";
            return 1;
        }
    } else {
        // Get most recent job
        job = getMostRecentJob();
        if (!job) {
            std::cerr << COLOR_ERROR << "tinyshell: bg: current: no such job" 
                      << COLOR_RESET << "\n";
            return 1;
        }
    }
    
    if (job->state != STOPPED) {
        std::cerr << COLOR_ERROR << "tinyshell: bg: job " << job->jobId 
                  << " already in background" << COLOR_RESET << "\n";
        return 1;
    }
    
    // Mark this job as current
    markJobAsCurrent(job->jobId);
    
    // Print in bash format: [1]+ command &
    std::cout << "[" << job->jobId << "]";
    if (job->is_current) {
        std::cout << "+";
    } else {
        std::cout << " ";
    }
    std::cout << " " << job->command << " &" << std::endl;
    
    // Continue the job in background
    kill(-job->pgid, SIGCONT);
    job->state = RUNNING;
    
    return 0;
}

// Built-in: jobs command
int builtin_jobs() {
    printJobs();
    return 0;
}

int executeCommand(const ParsedCommand& cmd) {
    if (cmd.args.empty()) return 0;
    
    // Check for built-in commands
    if (cmd.args[0] == "jobs") {
        return builtin_jobs();
    } else if (cmd.args[0] == "fg") {   
        return builtin_fg(cmd.args);
    } else if (cmd.args[0] == "bg") {
        return builtin_bg(cmd.args);
    }
    
    std::string execPath = findInPath(cmd.args[0]);
    if (execPath.empty()) {
        std::cerr << COLOR_ERROR << "tinyshell: command not found: " 
                  << cmd.args[0] << COLOR_RESET << "\n";
        return 127;
    }
    
    char** argv = vectorToArgv(cmd.args);
    pid_t pid = fork();
    
    if (pid < 0) {
        std::cerr << COLOR_ERROR << "tinyshell: fork failed" 
                  << COLOR_RESET << "\n";
        freeArgv(argv, cmd.args.size());
        return -1;
    }
    else if (pid == 0) {
        // Child Process
        
        // Put child in its own process group
        pid_t child_pgid = getpid();
        setpgid(0, child_pgid);
        
        // If foreground, give terminal control to child
        if (!cmd.isBackground) {
            tcsetpgrp(shell_terminal, child_pgid);
        }
        
        // Set default signal handlers for child
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);
        
        // Handle redirections
        setupRedirections(cmd);
        
        execve(execPath.c_str(), argv, environ);
        std::cerr << COLOR_ERROR << "tinyshell: execve failed" 
                  << COLOR_RESET << "\n";
        exit(1);
    }
    else {
        // Parent Process
        
        // Put child in its own process group
        setpgid(pid, pid);
        
        if (cmd.isBackground) {
            // Background execution
            std::vector<pid_t> pids;
            pids.push_back(pid);
            
            // Build full command string with arguments
            std::string fullCommand;
            for (size_t i = 0; i < cmd.args.size(); i++) {
                if (i > 0) fullCommand += " ";
                fullCommand += cmd.args[i];
            }
            
            addJob(pid, fullCommand, RUNNING, pids);
            freeArgv(argv, cmd.args.size());
        } else {
            // Foreground execution
            // Give terminal to child
            tcsetpgrp(shell_terminal, pid);
            
            int status;
            pid_t wait_result;
            
            // Wait for child
            while ((wait_result = waitpid(pid, &status, WUNTRACED)) > 0) {
                if (WIFEXITED(status)) {
                    int exitCode = WEXITSTATUS(status);
                    if (exitCode != 0) {
                        std::cout << COLOR_INFO << "[Process exited with code: " 
                                << exitCode << "]" << COLOR_RESET << "\n";
                    }
                    break;
                } else if (WIFSIGNALED(status)) {
                    int signal = WTERMSIG(status);
                    std::cout << COLOR_ERROR << "[Process terminated by signal: " 
                            << signal << "]" << COLOR_RESET << "\n";
                    break;
                } else if (WIFSTOPPED(status)) {
                    // Process was stopped (CTRL+Z)
                    std::vector<pid_t> pids;
                    pids.push_back(pid);
                    
                    // Build full command string with arguments
                    std::string fullCommand;
                    for (size_t i = 0; i < cmd.args.size(); i++) {
                        if (i > 0) fullCommand += " ";
                        fullCommand += cmd.args[i];
                    }
                    
                    addJob(pid, fullCommand, STOPPED, pids);
                    
                    // Get the job we just added to print status
                    Job* job = getJobByPgid(pid);
                    if (job) {
                        std::cout << "\n[" << job->jobId << "]";
                        if (job->is_current) {
                            std::cout << "+";
                        } else {
                            std::cout << " ";
                        }
                        std::cout << " Stopped         " << job->command << std::endl;
                    }
                    break;
                }
            }
            
            // Restore terminal control to shell
            tcsetpgrp(shell_terminal, shell_pgid);
            freeArgv(argv, cmd.args.size());
        }
    }
    
    return 0;
}

int executePipeline(const std::vector<ParsedCommand>& pipeline) {
    int numCmds = pipeline.size();
    std::vector<std::array<int, 2>> pipefds(numCmds - 1);
    std::vector<pid_t> pids;
    bool isBackground = pipeline[0].isBackground;
    
    // Create all pipes
    for (int i = 0; i < numCmds - 1; i++) {
        if (pipe(pipefds[i].data()) == -1) {
            std::cerr << COLOR_ERROR << "tinyshell: pipe failed" << COLOR_RESET << "\n";
            return -1;
        }
    }
    
    pid_t pgid = 0;
    
    // Fork and execute each command
    for (int i = 0; i < numCmds; i++) {
        pid_t pid = fork();
        
        if (pid < 0) {
            std::cerr << COLOR_ERROR << "tinyshell: fork failed" << COLOR_RESET << "\n";
            return -1;
        }
        else if (pid == 0) {
            // Child Process
            
            // Set process group
            if (i == 0) {
                // First process becomes group leader
                setpgid(0, 0);
                if (!isBackground) {
                    tcsetpgrp(shell_terminal, getpid());
                }
            } else {
                // Join the group
                setpgid(0, pgid);
            }
            
            // Set default signal handlers
            signal(SIGINT, SIG_DFL);
            signal(SIGQUIT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            signal(SIGTTIN, SIG_DFL);
            signal(SIGTTOU, SIG_DFL);
            signal(SIGCHLD, SIG_DFL);
            
            // Setup pipes
            if (i > 0) {
                dup2(pipefds[i-1][0], STDIN_FILENO);
            }
            
            if (i < numCmds - 1) {
                dup2(pipefds[i][1], STDOUT_FILENO);
            }
            
            // Close all pipe fds
            for (int j = 0; j < numCmds - 1; j++) {
                close(pipefds[j][0]);
                close(pipefds[j][1]);
            }
            
            // Handle redirections
            setupRedirections(pipeline[i]);
            
            // Find and execute
            std::string execPath = findInPath(pipeline[i].args[0]);
            if (execPath.empty()) {
                std::cerr << COLOR_ERROR << "tinyshell: command not found: " 
                          << pipeline[i].args[0] << COLOR_RESET << "\n";
                exit(127);
            }
            
            char** argv = vectorToArgv(pipeline[i].args);
            execve(execPath.c_str(), argv, environ);
            
            std::cerr << COLOR_ERROR << "tinyshell: execve failed" << COLOR_RESET << "\n";
            exit(1);
        }
        else {
            // Parent process
            if (i == 0) {
                pgid = pid;
            }
            setpgid(pid, pgid);
            pids.push_back(pid);
        }
    }
    
    // Parent: close all pipes
    for (int i = 0; i < numCmds - 1; i++) {
        close(pipefds[i][0]);
        close(pipefds[i][1]);
    }
    
    // Build command string for job
    std::string cmdString;
    for (size_t i = 0; i < pipeline.size(); i++) {
        if (i > 0) cmdString += " | ";
        cmdString += pipeline[i].args[0];
    }
    
    if (isBackground) {
        // Background execution
        addJob(pgid, cmdString, RUNNING, pids);
    } else {
        // Foreground execution
        tcsetpgrp(shell_terminal, pgid);
        
        int status;
        pid_t wait_result;
        bool pipeline_stopped = false;
        
        // Wait for all children
        for (int i = 0; i < numCmds; i++) {
            while ((wait_result = waitpid(-pgid, &status, WUNTRACED)) > 0) {
                if (WIFEXITED(status) || WIFSIGNALED(status)) {
                    continue;
                } else if (WIFSTOPPED(status)) {
                    // Pipeline was stopped
                    addJob(pgid, cmdString, STOPPED, pids);
                    
                    // Get the job we just added to print status
                    Job* job = getJobByPgid(pgid);
                    if (job) {
                        std::cout << "\n[" << job->jobId << "]";
                        if (job->is_current) {
                            std::cout << "+";
                        } else {
                            std::cout << " ";
                        }
                        std::cout << " Stopped         " << job->command << std::endl;
                    }
                    pipeline_stopped = true;  // Set flag
                    break;
                }
            }
            if (pipeline_stopped) {
                break;
            }
        }
        
        // Restore terminal control
        tcsetpgrp(shell_terminal, shell_pgid);
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
    
    // CRITICAL: Initialize shell BEFORE anything else
    init_shell();
    
    std::cout << "=======================================  _____ _____ _____           _____ _____ _____ _____ \n";
    std::cout << "  Welcome to TinyShell                  |   __|     |   __|   ___   |  _  |  |  |_   _|  |  |\n";
    std::cout << "  Type 'exit' or press Ctrl+D to quit   |   __|   --|   __|  |___|  |     |  |  | | | |     |\n";
    std::cout << "======================================= |_____|_____|_____|         |__|__|_____| |_| |__|__|\n\n";

    while (true) {
        // Check for job status changes before prompt
        check_job_status_changes();
        
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
        
        // Propagate background flag to all commands in pipeline
        if (pipeline.isBackground) {
            for (auto& cmd : pipeline.commands) {
                cmd.isBackground = true;
            }
        }
        
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