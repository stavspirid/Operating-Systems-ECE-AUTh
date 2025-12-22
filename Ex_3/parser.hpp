#ifndef PARSER_HPP
#define PARSER_HPP
#include <string>
#include <vector>
#include <sstream>

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
    bool isBackground = false;      // true if command is to be run in background (&)
    
    ParsedCommand();
};

/**
 * Structure representing a complete pipeline
 */
struct ParsedPipeline {
    std::vector<ParsedCommand> commands;// All commands in the pipeline
    bool hasPipes = false;				// true if pipeline contains pipes
    bool isBackground = false;          // true if pipeline is to be run in background (&)
    
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

#endif // PARSER_HPP