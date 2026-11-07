cmake_minimum_required(VERSION 3.16)

# Use the project's own components, so this tests the real GpioSyncTick.
list(APPEND EXTRA_COMPONENT_DIRS ../../components)
# Only build main and what it needs, not every component in that folder.
set(COMPONENTS main)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(sync_loopback)
