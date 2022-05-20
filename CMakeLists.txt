# CMake entry point
cmake_minimum_required (VERSION 3.5)
project (LSH)
 
## Add headers files
include_directories (
	modules/
)

set(HEADERS modules/SafeQueue.h modules/ThreadPool.h modules/cosine_lsh.h modules/cosine_lsh.cpp)

set(SOURCES main.cpp)

SET(CMAKE_CXX_FLAGS -pthread)
 
add_compile_options(
	-std=c++11
#        -D_DEBUG
#        -g
) 
 
# main.cpp
add_executable(main ${HEADERS} ${SOURCES})
target_link_libraries(main)