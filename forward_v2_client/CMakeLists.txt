cmake_minimum_required(VERSION 3.5)
set (TARGET_NAME "forward_v2_client")
include_directories(${CMAKE_CURRENT_LIST_DIR}/include)
link_directories(${CMAKE_CURRENT_LIST_DIR}/lib)
file(GLOB_RECURSE cpp_files "*.cpp")
add_executable(${TARGET_NAME} ${cpp_files} src/command_process.cpp)


