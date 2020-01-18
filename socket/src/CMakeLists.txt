cmake_minimum_required (VERSION 3.15)

#project(c_lib_socket)

if(WIN32)
    message(STATUS System: window)
    set(libsocket libsocket)
elseif(UNIX)
    message(STATUS System: unix-like)
    set(libsocket socket)
endif()

include_directories(
    ../include
)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build/lib)

if(WIN32)
    add_library(${libsocket} STATIC socket.c)
elseif(UNIX)
    add_library(${libsocket} SHARED socket.c)
endif()
set_target_properties(${libsocket} PROPERTIES POSITION_INDEPENDENT_CODE ON)
