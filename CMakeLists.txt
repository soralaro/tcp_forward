project(GFW)
cmake_minimum_required(VERSION 3.6)

set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS}  ")

set(CMAKE_CXX_FLAGS_DEBUG   "$ENV{CXXFLAGS} -O0 -g -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Werror -std=c++11 -fpermissive -fno-strict-aliasing -fopenmp")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Werror -fopenmp")

include_directories(${CMAKE_SOURCE_DIR}/server)
link_directories(${LIBRARY_OUTPUT_PATH})
link_directories(${CMAKE_SOURCE_DIR}/server)

add_subdirectory(server)
add_subdirectory(client)

add_subdirectory(server_rand)
add_subdirectory(client_rand)
add_subdirectory(forward_server)
add_subdirectory(forward_dst)
