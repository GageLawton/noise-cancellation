#!/usr/bin/env bash
# Format the repository's C++ sources, or check them without changing files.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CHECK=0

case "${1:-}" in
    "") ;;
    --check) CHECK=1 ;;
    *)
        echo "usage: $0 [--check]" >&2
        exit 2
        ;;
esac

files=()
while IFS= read -r -d '' file; do
    files+=("$file")
done < <(
    find "$ROOT/components" "$ROOT/main" "$ROOT/test" \
        -path "$ROOT/test/build" -prune -o \
        \( -name '*.cpp' -o -name '*.hpp' \) -print0
)

if ((${#files[@]} == 0)); then
    echo "no C++ sources found" >&2
    exit 1
fi

if ((CHECK)); then
    clang-format --dry-run --Werror "${files[@]}"
else
    clang-format -i "${files[@]}"
fi
