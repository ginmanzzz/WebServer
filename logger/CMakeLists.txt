# Logger subProject
project(logger)

# Pack these into a static library
add_library(logger STATIC
	blockqueue.h
	logger.h
	logger.cc
)

# 设置构建类型为 Debug
if(NOT CMAKE_BUILD_TYPE)
	    set(CMAKE_BUILD_TYPE Debug)
endif()

# 设置 Debug 构建的编译器选项
set(CMAKE_CXX_FLAGS_DEBUG "-g")

target_include_directories(logger PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Find fmt package
find_package(fmt REQUIRED)

# Link fmt library 
target_link_libraries(logger PUBLIC fmt::fmt)

# Test
add_executable(test_blockqueue test_blockqueue.cc)


