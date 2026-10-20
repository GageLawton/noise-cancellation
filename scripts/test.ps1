# Build and run the host tests in one step (Windows PowerShell). Works
# from any folder. Stops at the first command that fails.

$ErrorActionPreference = "Stop"

Set-Location (Join-Path $PSScriptRoot "..\test")

cmake -B build -S . -G "MinGW Makefiles"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

cmake --build build --parallel
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

ctest --test-dir build --output-on-failure
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
