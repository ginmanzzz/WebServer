# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/ginman/web/myWebServer

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/ginman/web/myWebServer/build

# Include any dependencies generated for this target.
include threadpool/CMakeFiles/test_conn.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include threadpool/CMakeFiles/test_conn.dir/compiler_depend.make

# Include the progress variables for this target.
include threadpool/CMakeFiles/test_conn.dir/progress.make

# Include the compile flags for this target's objects.
include threadpool/CMakeFiles/test_conn.dir/flags.make

threadpool/CMakeFiles/test_conn.dir/test_conn.cc.o: threadpool/CMakeFiles/test_conn.dir/flags.make
threadpool/CMakeFiles/test_conn.dir/test_conn.cc.o: ../threadpool/test_conn.cc
threadpool/CMakeFiles/test_conn.dir/test_conn.cc.o: threadpool/CMakeFiles/test_conn.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/ginman/web/myWebServer/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object threadpool/CMakeFiles/test_conn.dir/test_conn.cc.o"
	cd /home/ginman/web/myWebServer/build/threadpool && /usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT threadpool/CMakeFiles/test_conn.dir/test_conn.cc.o -MF CMakeFiles/test_conn.dir/test_conn.cc.o.d -o CMakeFiles/test_conn.dir/test_conn.cc.o -c /home/ginman/web/myWebServer/threadpool/test_conn.cc

threadpool/CMakeFiles/test_conn.dir/test_conn.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/test_conn.dir/test_conn.cc.i"
	cd /home/ginman/web/myWebServer/build/threadpool && /usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/ginman/web/myWebServer/threadpool/test_conn.cc > CMakeFiles/test_conn.dir/test_conn.cc.i

threadpool/CMakeFiles/test_conn.dir/test_conn.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/test_conn.dir/test_conn.cc.s"
	cd /home/ginman/web/myWebServer/build/threadpool && /usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/ginman/web/myWebServer/threadpool/test_conn.cc -o CMakeFiles/test_conn.dir/test_conn.cc.s

threadpool/CMakeFiles/test_conn.dir/connectionpool.cc.o: threadpool/CMakeFiles/test_conn.dir/flags.make
threadpool/CMakeFiles/test_conn.dir/connectionpool.cc.o: ../threadpool/connectionpool.cc
threadpool/CMakeFiles/test_conn.dir/connectionpool.cc.o: threadpool/CMakeFiles/test_conn.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/ginman/web/myWebServer/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object threadpool/CMakeFiles/test_conn.dir/connectionpool.cc.o"
	cd /home/ginman/web/myWebServer/build/threadpool && /usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT threadpool/CMakeFiles/test_conn.dir/connectionpool.cc.o -MF CMakeFiles/test_conn.dir/connectionpool.cc.o.d -o CMakeFiles/test_conn.dir/connectionpool.cc.o -c /home/ginman/web/myWebServer/threadpool/connectionpool.cc

threadpool/CMakeFiles/test_conn.dir/connectionpool.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/test_conn.dir/connectionpool.cc.i"
	cd /home/ginman/web/myWebServer/build/threadpool && /usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/ginman/web/myWebServer/threadpool/connectionpool.cc > CMakeFiles/test_conn.dir/connectionpool.cc.i

threadpool/CMakeFiles/test_conn.dir/connectionpool.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/test_conn.dir/connectionpool.cc.s"
	cd /home/ginman/web/myWebServer/build/threadpool && /usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/ginman/web/myWebServer/threadpool/connectionpool.cc -o CMakeFiles/test_conn.dir/connectionpool.cc.s

# Object files for target test_conn
test_conn_OBJECTS = \
"CMakeFiles/test_conn.dir/test_conn.cc.o" \
"CMakeFiles/test_conn.dir/connectionpool.cc.o"

# External object files for target test_conn
test_conn_EXTERNAL_OBJECTS =

threadpool/test_conn: threadpool/CMakeFiles/test_conn.dir/test_conn.cc.o
threadpool/test_conn: threadpool/CMakeFiles/test_conn.dir/connectionpool.cc.o
threadpool/test_conn: threadpool/CMakeFiles/test_conn.dir/build.make
threadpool/test_conn: /usr/lib/x86_64-linux-gnu/libmysqlclient.so
threadpool/test_conn: logger/liblogger.a
threadpool/test_conn: /usr/lib/x86_64-linux-gnu/libfmt.so.8.1.1
threadpool/test_conn: threadpool/CMakeFiles/test_conn.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/ginman/web/myWebServer/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX executable test_conn"
	cd /home/ginman/web/myWebServer/build/threadpool && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/test_conn.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
threadpool/CMakeFiles/test_conn.dir/build: threadpool/test_conn
.PHONY : threadpool/CMakeFiles/test_conn.dir/build

threadpool/CMakeFiles/test_conn.dir/clean:
	cd /home/ginman/web/myWebServer/build/threadpool && $(CMAKE_COMMAND) -P CMakeFiles/test_conn.dir/cmake_clean.cmake
.PHONY : threadpool/CMakeFiles/test_conn.dir/clean

threadpool/CMakeFiles/test_conn.dir/depend:
	cd /home/ginman/web/myWebServer/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/ginman/web/myWebServer /home/ginman/web/myWebServer/threadpool /home/ginman/web/myWebServer/build /home/ginman/web/myWebServer/build/threadpool /home/ginman/web/myWebServer/build/threadpool/CMakeFiles/test_conn.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : threadpool/CMakeFiles/test_conn.dir/depend
