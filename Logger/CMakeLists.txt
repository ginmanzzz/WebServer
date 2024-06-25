# Logger subProject

add_library(logger logger.cc)
target_include_directories(logger PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Test
add_executable(test_log test_log.cc)
target_link_libraries(test_log logger)
