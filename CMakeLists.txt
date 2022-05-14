cmake_minimum_required(VERSION 3.20.0)
project(libstorage VERSION 0.1.0)

set(LIBSTORAGE_HEADERS_DIR ${PROJECT_SOURCE_DIR}/include)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_EXPORT_COMPILE_COMMANDS true)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wfatal-errors")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fconcepts-diagnostics-depth=3")

#set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

set(CMAKE_LIBRARY_PATH "/usr/local/lib/" "/lib/")

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

include_directories(${LIBSTORAGE_HEADERS_DIR})

add_subdirectory(test)
add_subdirectory(src)
