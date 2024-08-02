#!/bin/bash

# Configuration
PROGRAM_NAME="joefetch"
SOURCE_FILE="joefetch.c"
EXE_DIR="$HOME/bin"
EXE_PATH="$EXE_DIR/$PROGRAM_NAME"

mkdir -p "$EXE_DIR"

echo "Compiling $SOURCE_FILE..."
gcc -o "$EXE_PATH" "$SOURCE_FILE"

if [ $? -eq 0 ]; then
    echo "Compilation successful."

    if echo "$PATH" | grep -q "$EXE_DIR"; then
        echo "Executable is already in PATH. Updated the executable."
    else
        echo "Adding $EXE_DIR to PATH..."
        echo "export PATH=\"\$PATH:$EXE_DIR\"" >> "$HOME/.bashrc"
        echo "export PATH=\"\$PATH:$EXE_DIR\"" >> "$HOME/.zshrc"
        source "$HOME/.bashrc"
        source "$HOME/.zshrc"
        echo "$EXE_DIR added to PATH. Please restart your terminal or run 'source ~/.bashrc' and 'source ~/.zshrc' to update the PATH."
    fi
    if command -v "$PROGRAM_NAME" &> /dev/null; then
        echo "Running $PROGRAM_NAME..."
        "$PROGRAM_NAME"
    else
        echo "Error: $PROGRAM_NAME not found in PATH."
    fi
else
    echo "Compilation failed."
fi