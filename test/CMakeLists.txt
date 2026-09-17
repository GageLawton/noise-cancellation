cmake_minimum_required(VERSION 3.16)
project(anc_host_tests CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Match the device build's constraints so host tests exercise the same
# code shape (design doc 10.1). Scoped to C++ only -- Unity is C, and
# -fno-rtti is not a valid C flag.
set(ANC_CXX_FLAGS -Wall -Wextra -fno-exceptions -fno-rtti)

include(FetchContent)
FetchContent_Declare(
    unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG        v2.6.0
)
FetchContent_MakeAvailable(unity)

set(ANC_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)

# Only components with no ESP-IDF dependency build on host. That is what
# makes off-target testing free -- see design doc 9.1.
add_library(anc_portable
    ${ANC_ROOT}/components/config/node_config.cpp
)
target_compile_options(anc_portable PRIVATE ${ANC_CXX_FLAGS})
target_include_directories(anc_portable PUBLIC
    ${ANC_ROOT}/components/config/include
    ${ANC_ROOT}/components/hal/include
    ${ANC_ROOT}/components/dsp_core/include
    ${ANC_ROOT}/components/alignment/include
    ${ANC_ROOT}/components/calibration/include
)

enable_testing()

find_package(Threads REQUIRED)

# Each test_*.cpp becomes its own binary and its own ctest entry.
file(GLOB TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp)
foreach(test_src ${TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    target_compile_options(${test_name} PRIVATE ${ANC_CXX_FLAGS})
    target_link_libraries(${test_name} PRIVATE
        anc_portable unity Threads::Threads)
    target_include_directories(${test_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
