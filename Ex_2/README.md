File Descriptors (FDs).
0: Standard Input (Keyboard)
1: Standard Output (Screen)
2: Standard Error (Screen)

A queue was used for input and output filenames for its FIFO properties


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