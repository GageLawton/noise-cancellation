#!/usr/bin/env bash
# Build and run the host tests in one step. Works from any folder.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT/test"

cmake -B build -S .
cmake --build build --parallel
ctest --test-dir build --output-on-failure
