cmake_minimum_required(VERSION 3.10)

project(WebServer)

# CPP Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Compiler warning profile
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Werror
        -pedantic
        -Wconversion
        -Wshadow
        -Wcast-qual
        -Wcast-align
        -Wpointer-arith
    )
endif()

# Find fmt package
find_package(fmt REQUIRED)

# Add sub directory
add_subdirectory(Logger)
add_subdirectory(ThreadPool)

# Include directories
include_directories(Logger)
include_directories(ThreadPool)

# Executable file
add_executable(server webserver.cc)

# Link subDirectories libraries
target_link_libraries(server PRIVATE logger pools)
