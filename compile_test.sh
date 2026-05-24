#! /usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$SCRIPT_DIR"
TEST_DIR="$ROOT_DIR/test"
BIN_DIR="$TEST_DIR/bin"

mkdir -p "$BIN_DIR"

for src in "$TEST_DIR"/*.cpp; do
  if [[ ! -e "$src" ]]; then
    continue
  fi

  exe="$BIN_DIR/$(basename "${src%.cpp}")"
  echo "Compiling $src -> $exe"
  g++ -std=c++17 -O2 -Wall -Wextra -I"$ROOT_DIR/inc" -I"$TEST_DIR" "$src" -o "$exe"
done

echo "Finished compiling test cpp files to $BIN_DIR."
