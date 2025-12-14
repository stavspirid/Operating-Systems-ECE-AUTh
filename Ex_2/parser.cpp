#include "parser.hpp"

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
