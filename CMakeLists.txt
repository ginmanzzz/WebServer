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

# Add sub directory
add_subdirectory(Logger)

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/Logger)

# Source files
set(SOURCES
	webserver.cc
	Logger/logger.cc
)

# Executable file
add_executable(server ${SOURCES})

# Link subDirectories library
# target_link_libraries(server logger)
