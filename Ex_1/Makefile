# TinyShell Makefile
# Simple build system for the TinyShell project

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -pedantic
DEBUGFLAGS = -g -O0
RELEASEFLAGS = -O2

# Target executable
TARGET = tinyshell
SOURCE = tinyshell.cpp

# Default target: build release version
all: release

# Release build (optimized)
release: $(SOURCE)
	@echo "Building TinyShell (Release)..."
	$(CXX) $(CXXFLAGS) $(RELEASEFLAGS) -o $(TARGET) $(SOURCE)
	@echo "Build complete! Run with: ./$(TARGET)"

# Debug build (with debug symbols)
debug: $(SOURCE)
	@echo "Building TinyShell (Debug)..."
	$(CXX) $(CXXFLAGS) $(DEBUGFLAGS) -o $(TARGET) $(SOURCE)
	@echo "Debug build complete! Run with: ./$(TARGET)"

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -f $(TARGET)
	@echo "Clean complete!"

# Run the shell after building
run: release
	@echo "Starting TinyShell..."
	./$(TARGET)

# Install to /usr/local/bin (requires sudo)
install: release
	@echo "Installing TinyShell to /usr/local/bin..."
	sudo cp $(TARGET) /usr/local/bin/
	@echo "Installation complete! You can now run 'tinyshell' from anywhere."

# Uninstall from /usr/local/bin
uninstall:
	@echo "Uninstalling TinyShell..."
	sudo rm -f /usr/local/bin/$(TARGET)
	@echo "Uninstall complete!"

# Help message
help:
	@echo "TinyShell Build System"
	@echo "======================"
	@echo ""
	@echo "Available targets:"
	@echo "  make          - Build release version (default)"
	@echo "  make release  - Build optimized release version"
	@echo "  make debug    - Build debug version with symbols"
	@echo "  make clean    - Remove build artifacts"
	@echo "  make run      - Build and run TinyShell"
	@echo "  make install  - Install to /usr/local/bin (requires sudo)"
	@echo "  make uninstall- Remove from /usr/local/bin (requires sudo)"
	@echo "  make help     - Show this help message"

# Phony targets (not actual files)
.PHONY: all release debug clean run install uninstall help
