#!/bin/bash
find . -path ./.git -prune -o -path ./build -prune -o -type f | egrep '\.(cpp|hpp|c|h|cc|hh)$' | xargs clang-format -i -style=file
