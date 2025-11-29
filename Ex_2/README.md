File Descriptors (FDs).
0: Standard Input (Keyboard)
1: Standard Output (Screen)
2: Standard Error (Screen)

A queue was used for input and output filenames for its FIFO properties

1. The Structures
***ParsedCommand***
```cpp
struct ParsedCommand {
    std::vector<std::string> args;      // The actual command + arguments
    std::string inputFile;               // File for input redirection (<)
    std::string outputFile;              // File for output redirection (> or >>)
    bool appendMode = false;             // false for >, true for >>
};
```
Think of it as: A single command with all its metadata
***ParsedPipeline***
```cpp
struct ParsedPipeline {
    std::vector<ParsedCommand> commands; // List of all commands
    bool hasPipes = false;               // Is this a pipeline?
};
```
Think of it as: The complete command line, possibly with multiple commands


## Examples
echo "Zebra" > inputFile.txt
echo "Banana" >> inputFile.txt
"Elephant" >> inputFile.txt
cat inputFile.txt
\\ > "Zebra"
\\ > "Banana"
\\ > "Elephant"
sort < inputFile.txt > sortedFile.txt
cat sortedFile.txt
\\ > "Banana"
\\ > "Elephant"
\\ > "Zebra"

### Input Manipulation
parseCommand and splitByPipes 
// Input string:
"ls -la | grep txt | wc -l"

// After parseCommand (1D vector):
["ls", "-la", "|", "grep", "txt", "|", "wc", "-l"]

// After splitByPipes (2D vector):
[
    ["ls", "-la"],      // Command 1
    ["grep", "txt"],    // Command 2
    ["wc", "-l"]        // Command 3
]

### Piping Examples
echo "hello world" | tr 'a-z' 'A-Z' | rev
-> "DLROW OLLEH"