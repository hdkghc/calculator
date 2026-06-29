#! /usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$SCRIPT_DIR"
TEST_DIR="$ROOT_DIR/test"
BIN_DIR="$TEST_DIR/bin"

# Source files to link
SRC_DIR="$ROOT_DIR/src"
SRC_FILES=$(find "$SRC_DIR" -name '*.cpp' ! -name 'main.cpp' -print0 | xargs -0)

mkdir -p "$BIN_DIR"

# Collect all test cpp files
TEST_FILES=()
while IFS= read -r -d '' file; do
    TEST_FILES+=("$file")
done < <(find "$TEST_DIR" -maxdepth 1 -name '*.cpp' -print0)

if [[ ${#TEST_FILES[@]} -eq 0 ]]; then
    echo "No test cpp files found in $TEST_DIR"
    exit 0
fi

# Display selection menu
echo "Found test files:"
for i in "${!TEST_FILES[@]}"; do
    filename=$(basename "${TEST_FILES[$i]}")
    echo "  $((i+1)). $filename"
done
echo "  a. All files"
echo "  q. Quit"
echo
read -p "Select file(s) to compile (e.g., '1 2 ... ${#TEST_FILES[@]}' or '1-${#TEST_FILES[@]}' or 'a' for all): " selection

# Function to compile a single file
compile_file() {
    local src="$1"
    local exe="$BIN_DIR/$(basename "${src%.cpp}")"
    echo "Compiling $src -> $exe"
    g++ -std=c++17 -O0 -g -Wall -Wextra \
        -I"$ROOT_DIR/inc" \
        -I"$TEST_DIR" \
        "$src" \
        $SRC_FILES \
        -o "$exe"
}

# Parse selection
if [[ "$selection" == "q" || "$selection" == "Q" ]]; then
    echo "Quitting."
    exit 0
fi

if [[ "$selection" == "a" || "$selection" == "A" ]]; then
    # Compile all files
    for src in "${TEST_FILES[@]}"; do
        compile_file "$src"
    done
else
    # Parse ranges and individual numbers
    selected_indices=()
    IFS=' ,' read -ra parts <<< "$selection"
    for part in "${parts[@]}"; do
        if [[ "$part" =~ ^([0-9]+)-([0-9]+)$ ]]; then
            # Range
            start="${BASH_REMATCH[1]}"
            end="${BASH_REMATCH[2]}"
            for ((i=start; i<=end; i++)); do
                if [[ $i -ge 1 && $i -le ${#TEST_FILES[@]} ]]; then
                    selected_indices+=($((i-1)))
                else
                    echo "Warning: Index $i out of range (1-${#TEST_FILES[@]})"
                fi
            done
        elif [[ "$part" =~ ^[0-9]+$ ]]; then
            # Single number
            if [[ $part -ge 1 && $part -le ${#TEST_FILES[@]} ]]; then
                selected_indices+=($((part-1)))
            else
                echo "Warning: Index $part out of range (1-${#TEST_FILES[@]})"
            fi
        fi
    done

    # Remove duplicates and sort
    if [[ ${#selected_indices[@]} -gt 0 ]]; then
        selected_indices=($(printf "%s\n" "${selected_indices[@]}" | sort -nu))
        
        # Compile selected files
        for idx in "${selected_indices[@]}"; do
            compile_file "${TEST_FILES[$idx]}"
        done
    else
        echo "No valid selections made."
        exit 1
    fi
fi

echo "Finished compiling test cpp files to $BIN_DIR."