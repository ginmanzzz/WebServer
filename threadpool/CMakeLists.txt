# Logger subProject
project(pools)

# Pack these into a static library
add_library(pools STATIC
	threadpool.h
	connectionpool.h
	connectionpool.cc
)

# 手动设置 MySQL 库和包含路径
set(MySQL_INCLUDE_DIR /usr/include/mysql)
set(MySQL_LIBRARY_DIR /usr/lib/x86_64-linux-gnu)
set(MySQL_LIBRARIES libmysqlclient.so)

# 设置构建类型为 Debug
if(NOT CMAKE_BUILD_TYPE)
	    set(CMAKE_BUILD_TYPE Debug)
endif()

# 设置 Debug 构建的编译器选项
set(CMAKE_CXX_FLAGS_DEBUG "-g")

target_include_directories(pools PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${MySQL_INCLUDE_DIR})
target_link_libraries(pools PUBLIC ${MySQL_LIBRARY_DIR}/${MySQL_LIBRARIES})

# Test
add_executable(test_threadPool test_threadPool.cc connectionpool.cc)
add_executable(test_conn test_conn.cc connectionpool.cc)
target_link_libraries(test_conn PUBLIC ${MySQL_LIBRARY_DIR}/${MySQL_LIBRARIES} logger)
target_link_libraries(test_threadPool PUBLIC ${MySQL_LIBRARY_DIR}/${MySQL_LIBRARIES} logger)


# Include directories
include_directories(logger)
include_directories(threadpool)
