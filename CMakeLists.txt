project(GFW)
cmake_minimum_required(VERSION 3.5)

set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)
#set( CMAKE_C_COMPILER          /home/czx/application/gcc-linaro-arm-linux-gnueabihf-4.9_0614/bin/arm-linux-gnueabihf-gcc )
#set( CMAKE_CXX_COMPILER        /home/czx/application/gcc-linaro-arm-linux-gnueabihf-4.9_0614/bin/arm-linux-gnueabihf-g++ )

#SET( CMAKE_CXX_FLAGS           "${ARM_C_CXX_FLAGS} "   CACHE STRING "c++ flags" )
#SET( CMAKE_C_FLAGS             "${ARM_C_CXX_FLAGS} "   CACHE STRING "c flags" )
#set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS}  ")

set(CMAKE_CXX_FLAGS_DEBUG   "$ENV{CXXFLAGS} -O0 -g -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -std=c++11 -fpermissive -fno-strict-aliasing -fopenmp")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall  -fopenmp")

include_directories(${CMAKE_SOURCE_DIR}/server)
link_directories(${LIBRARY_OUTPUT_PATH})
link_directories(${CMAKE_SOURCE_DIR}/server)

add_subdirectory(server)
add_subdirectory(client)

add_subdirectory(server_rand)
add_subdirectory(client_rand)
add_subdirectory(forward_server)
add_subdirectory(forward_dst)
add_subdirectory(forward_v2_client)
add_subdirectory(forward_v2_server)
add_subdirectory(server_test)
add_subdirectory(client_test)
