#!/bin/bash

# Check if two arguments are provided
if [ $# -ne 2 ]; then
    echo "Usage: $0 <directory1> <directory2>"
    exit 1
fi

# Assign arguments to variables
dir1="$1"
dir2="$2"

# Check if both arguments are directories
if [ ! -d "$dir1" ] || [ ! -d "$dir2" ]; then
    echo "Error: Both arguments must be valid directories"
    exit 1
fi

# Find common files
common_files=$(comm -12 <(ls "$dir1" | sort) <(ls "$dir2" | sort))

# Perform diff on common files
for file in $common_files; do
    if [ -f "$dir1/$file" ] && [ -f "$dir2/$file" ]; then
        echo "Diffing $file:"
        diff "$dir1/$file" "$dir2/$file"
        echo "------------------------"
    fi
done