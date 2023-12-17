##!/bin/bash

# Check if an argument is provided
if [ $# -ne 1 ]; then
    echo "Usage: $0 <path/to/portal.z64>"
    exit 1
fi

# Get the provided binary path from the argument
BINARY_PATH="$1"

# Check if the file exists
if [ -f "$BINARY_PATH" ]; then
    # Calculate the size of the binary
    size=$(stat -c%s "$BINARY_PATH")

    # Pad zeros until the size is a multiple of 512
    while [ $((size % 512)) -ne 0 ]; do
        printf '\000' >> "$BINARY_PATH"
        size=$((size + 1))
    done
else
    echo "File not found: $BINARY_PATH"
fi

