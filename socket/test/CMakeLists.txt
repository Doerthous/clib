cmake_minimum_required (VERSION 3.15)

#project(c_lib_socket)

if(WIN32)
    message(STATUS System: window)
    set(libsocket libsocket)
elseif(UNIX)
    message(STATUS System: unix-like)
    set(libsocket socket)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build/bin)

include_directories(
    ../include
)

link_directories(
    ../lib
)

add_executable(client client.c)
add_dependencies(client ${libsocket})
target_link_libraries(client ${libsocket})

add_executable(server server.c)
add_dependencies(server ${libsocket})
target_link_libraries(server ${libsocket})
