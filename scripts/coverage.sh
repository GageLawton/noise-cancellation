#!/usr/bin/env bash
# Build, test, and report coverage locally -- the same steps CI runs.
#
# Usage: ./scripts/coverage.sh [--open]
#
# Requires: cmake, a C++17 compiler, gcovr (pip install gcovr)

set -euo pipefail

usage() {
    echo "Usage: ./scripts/coverage.sh [--open]"
    echo "  --open   open the HTML report when done"
}

OPEN=0
case "${1:-}" in
    "") ;;
    --open) OPEN=1 ;;
    -h|--help) usage; exit 0 ;;
    *) echo "Unknown argument: $1" >&2; usage >&2; exit 2 ;;
esac

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="$ROOT/test/build-coverage"

if ! command -v gcovr >/dev/null 2>&1; then
    echo "gcovr not found. Install it with:  pip install gcovr" >&2
    exit 1
fi

echo "==> configuring"
cmake -B "$BUILD" -S "$ROOT/test" -DANC_COVERAGE=ON >/dev/null

echo "==> building"
cmake --build "$BUILD" --parallel >/dev/null

echo "==> running tests"
(cd "$BUILD" && ctest --output-on-failure)

echo "==> coverage"
cd "$BUILD"
gcovr --root "$ROOT" \
      --filter "$ROOT/components/" \
      --exclude '.*/_deps/.*' \
      --html-details coverage.html \
      --print-summary

echo
echo "HTML report: $BUILD/coverage.html"

if [[ "$OPEN" == 1 ]]; then
    if command -v xdg-open >/dev/null 2>&1; then
        xdg-open "$BUILD/coverage.html"
    elif command -v open >/dev/null 2>&1; then
        open "$BUILD/coverage.html"
    fi
fi
