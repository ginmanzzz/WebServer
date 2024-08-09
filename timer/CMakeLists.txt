# Pools subProject
project(timer)

# Pack these into a static library
add_library(timer STATIC
	timer.cc
	timer.h
)

# 设置构建类型为 Debug
if(NOT CMAKE_BUILD_TYPE)
	    set(CMAKE_BUILD_TYPE Debug)
endif()

# 设置 Debug 构建的编译器选项
set(CMAKE_CXX_FLAGS_DEBUG "-g")

target_include_directories(timer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(timer PRIVATE http pools)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/http)
include_directories(${CMAKE_SOURCE_DIR}/threadpool)
