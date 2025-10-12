#!/bin/bash
set -e

# Check if input file is provided
if [ $# -eq 0 ]; then
    echo "Usage: $0 <input_file>"
    echo "Example: $0 Test/comprehensive_test.li"
    exit 1
fi

INPUT_FILE="$1"
LOG_FILE="run.log"

# Check if input file exists
if [ ! -f "$INPUT_FILE" ]; then
    echo "[ERROR] Input file not found: $INPUT_FILE"
    exit 1
fi

# Find LinhApp executable
if [ -f build/Debug/LinhApp ]; then
    EXE_PATH="./build/Debug/LinhApp"
elif [ -f build/LinhApp ]; then
    EXE_PATH="./build/LinhApp"
elif [ -f build/Release/LinhApp ]; then
    EXE_PATH="./build/Release/LinhApp"
else
    echo "[ERROR] LinhApp not found in build folders!"
    exit 1
fi

echo "Running: $EXE_PATH $INPUT_FILE"
echo "Saving output to: $LOG_FILE"
echo

# Run the application and save output to log file
"$EXE_PATH" "$INPUT_FILE" > "$LOG_FILE" 2>&1

echo
echo "=== Execution completed. Check $LOG_FILE for output ==="
