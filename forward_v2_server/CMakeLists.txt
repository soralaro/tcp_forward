cmake_minimum_required(VERSION 3.5)
set (TARGET_NAME "forward_v2_server")
include_directories(${CMAKE_CURRENT_LIST_DIR}/include)
link_directories(${CMAKE_CURRENT_LIST_DIR}/lib)
file(GLOB_RECURSE cpp_files "*.cpp")
file(GLOB_RECURSE c_files "*.c")
add_executable(${TARGET_NAME} ${cpp_files} ${c_files})
TARGET_LINK_LIBRARIES(${TARGET_NAME} -lmysqlclient)

