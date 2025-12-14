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

#include <string>
#include <vector>
#include <array>

// ANSI color codes
#define COLOR_PROMPT "\033[1;32m"
#define COLOR_RESET "\033[0m"
#define COLOR_ERROR "\033[1;31m"
#define COLOR_INFO "\033[1;36m"

/**
 * Structure representing a single parsed command with redirections
 */
struct ParsedCommand {
    std::vector<std::string> args;	// Command and arguments
    std::string inputFile;			// Input redirection file (<)
    std::string outputFile;			// Output redirection file (> or >>)
	std::string errorFile;        	// For stderr redirection (2>)
    bool appendMode = false;		// true for >>, false for >
	bool appendErrorMode = false; 	// For stderr append (2>>)
    
    ParsedCommand();
};

/**
 * Structure representing a complete pipeline
 */
struct ParsedPipeline {
    std::vector<ParsedCommand> commands;// All commands in the pipeline
    bool hasPipes = false;				// true if pipeline contains pipes
    
    ParsedPipeline();
};

/**
 * Parse command line into tokens
 * 
 * @param line Input command line string
 * @return Vector of tokens
 */
std::vector<std::string> tokenize(const std::string& line);

/**
 * Parse tokens into pipeline with redirections
 * 
 * @param tokens Vector of tokens from tokenize()
 * @return Parsed pipeline structure
 */
ParsedPipeline parseCommandLine(const std::vector<std::string>& tokens);

/**
 * Search for executable in PATH environment variable
 * 
 * @param command Command name to search for
 * @return Full path to executable, or empty string if not found
 */
std::string findInPath(const std::string& command);

/**
 * Convert vector of strings to C-style argv array
 * 
 * @param args Vector of argument strings
 * @return Dynamically allocated argv array (must be freed with freeArgv)
 */
char** vectorToArgv(const std::vector<std::string>& args);

/**
 * Free memory allocated for argv array
 * 
 * @param argv Array to free
 * @param size Number of elements in array
 */
void freeArgv(char** argv, size_t size);

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