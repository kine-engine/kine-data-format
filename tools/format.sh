#!/bin/bash

# project root directory
cd "$(dirname "$0")/.." || exit 1

# find and format all C and H files, skip build directories
find . -type d -name "build" -prune -o -type f \( -name "*.c" -o -name "*.h" \) -exec clang-format -i {} +

echo "formatting complete."
