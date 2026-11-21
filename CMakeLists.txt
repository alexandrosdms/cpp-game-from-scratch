cmake_minimum_required(VERSION 3.22)
project(
    game
    VERSION "0.0.1"
    DESCRIPTION "a sweet game"
    LANGUAGES C CXX
)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(src)
