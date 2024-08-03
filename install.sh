#!/bin/bash

# Configuration
PROGRAM_NAME="joefetch"
SOURCE_FILE="joefetch.c"
EXE_DIR="$HOME/bin"
EXE_PATH="$EXE_DIR/$PROGRAM_NAME"
ASCII_FILE="ascii.txt"
CONFIG_FILE="config.h"
mkdir -p "$EXE_DIR"
echo "Compiling $SOURCE_FILE..."
gcc -o "$EXE_PATH" "$SOURCE_FILE" -O3 -flto -march=native -ftree-vectorize -fomit-frame-pointer -funroll-loops

if [ $? -eq 0 ]; then
    echo "Compilation successful."
    if [ -f "$ASCII_FILE" ]; then
        cp "$ASCII_FILE" "$EXE_DIR/"
        echo "Copied $ASCII_FILE to $EXE_DIR."
    else
        echo "Warning: $ASCII_FILE not found. Skipping copy."
    fi

    if [ -f "$CONFIG_FILE" ]; then
        cp "$CONFIG_FILE" "$EXE_DIR/"
        echo "Copied $CONFIG_FILE to $EXE_DIR."
    else
        echo "Warning: $CONFIG_FILE not found. Skipping copy."
    fi
    if echo "$PATH" | grep -q "$EXE_DIR"; then
        echo "Executable directory is already in PATH."
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
