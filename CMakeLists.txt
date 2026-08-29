cmake_minimum_required(VERSION 3.25)

project(trading_system
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH
    "${CMAKE_SOURCE_DIR}/cmake"
)

include(BuildOptions)
include(CompilerWarnings)

enable_testing()

add_subdirectory(libs)
add_subdirectory(apps)
add_subdirectory(tests)
add_subdirectory(benchmarks)