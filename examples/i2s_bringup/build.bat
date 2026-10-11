@echo off
rem Build the I2S bring-up example, and optionally flash it and open the monitor.
rem Usage: build.bat          build only
rem        build.bat COM5     build, flash and monitor on COM5
setlocal
cd /d "%~dp0"

if "%IDF_PATH%"=="" (
    echo IDF_PATH is not set. Run this from the ESP-IDF 5.3 CMD shell.
    exit /b 1
)

rem A build folder with no CMake cache is left over from a failed run, and idf.py refuses to delete it.
if exist build if not exist build\CMakeCache.txt rmdir /s /q build

rem First build in a clean folder: set the chip once.
if not exist build\CMakeCache.txt (
    call idf.py set-target esp32s3 || exit /b 1
)

if "%~1"=="" (
    call idf.py build
) else (
    call idf.py -p %1 build flash monitor
)