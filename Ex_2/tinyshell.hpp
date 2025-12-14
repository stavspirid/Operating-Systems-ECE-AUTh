/*
 * TinyShell - Header File
 * 
 * Declarations for TinyShell functions and structures
 * 
 * Author: TinyShell Project
 * Platform: Linux (POSIX-compliant systems)
 */

#ifndef TINYSHELL_HPP
#define TINYSHELL_HPP

#include "parser.hpp"
#include <string>
#include <vector>
#include <array>

// ANSI color codes
#define COLOR_PROMPT "\033[1;32m"
#define COLOR_RESET "\033[0m"
#define COLOR_ERROR "\033[1;31m"
#define COLOR_INFO "\033[1;36m"

/**
 * Search for executable in PATH environment variable
 * 
 * @param command Command name to search for
 * @return Full path to executable, or empty string if not found
 */
std::string findInPath(const std::string& command);

/**
 * Setup input/output/error redirections for a command
 * 
 * @param cmd Parsed command structure
 */
void setupRedirections(const ParsedCommand& cmd);

/**
 * Execute a single command with redirections
 * 
 * @param cmd Parsed command structure
 * @return Exit code of command
 */
int executeCommand(const ParsedCommand& cmd);

/**
 * Execute a pipeline of commands
 * 
 * @param pipeline Vector of parsed commands
 * @return Exit code of last command in pipeline
 */
int executePipeline(const std::vector<ParsedCommand>& pipeline);

/**
 * Display the shell prompt
 */
void displayPrompt();

#endif // TINYSHELL_HPP