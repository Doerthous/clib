cmake_minimum_required (VERSION 3.15)

project(c_lib_socket)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build/lib)

add_library(libsocket STATIC socket.c)
#add_library(libsocket SHARED socket.c)
set_target_properties(libsocket PROPERTIES POSITION_INDEPENDENT_CODE ON)


add_executable(client client.c)
add_dependencies(client libsocket)
target_link_libraries(client libsocket)

add_executable(server server.c)
add_dependencies(server libsocket)
target_link_libraries(server libsocket)