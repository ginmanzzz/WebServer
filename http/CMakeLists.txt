# Logger subProject
project(http)

# Pack these into a static library
add_library(http STATIC
	http.cc
	http.h
)

# 设置构建类型为 Debug
if(NOT CMAKE_BUILD_TYPE)
	    set(CMAKE_BUILD_TYPE Debug)
endif()

# 设置 Debug 构建的编译器选项
set(CMAKE_CXX_FLAGS_DEBUG "-g")

include_directories(${CMAKE_SOURCE_DIR}/threadpool)
include_directories(${CMAKE_SOURCE_DIR}/logger)

target_include_directories(http PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

