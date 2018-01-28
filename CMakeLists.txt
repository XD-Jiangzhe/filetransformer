cmake_minimum_required(VERSION 3.5)

project(filetransform)

SET(CMAKE_CXX_FLAGS "-g -std=c++11")

find_library(muduo_base muduo_base)
find_library(muduo_net muduo_net)

MESSAGE(STATUS muduo_base)
MESSAGE(STATUS muduo_net)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/../bin)
MESSAGE(STATUS PROJECT_BINARY_DIR)

add_executable(filetransform filetransform.cc)
target_link_libraries(filetransform muduo_base muduo_net pthread)


add_executable(transform_client transform_client.cc)
target_link_libraries(transform_client  muduo_net muduo_base  pthread boost_system)

