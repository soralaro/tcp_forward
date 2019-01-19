cmake_minimum_required(VERSION 3.5)

set (TARGET_NAME "server_test")


file(GLOB_RECURSE cpp_files "*.cpp")
add_executable(${TARGET_NAME} ${cpp_files} )


