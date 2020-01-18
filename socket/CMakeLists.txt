cmake_minimum_required (VERSION 3.15)

project(c_lib_socket)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build/lib)


add_library(libsocket STATIC socket.c)

add_executable(client
    socket.c
    client.c)

add_executable(server
    socket.c
    server.c)