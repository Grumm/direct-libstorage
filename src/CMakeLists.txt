cmake_minimum_required(VERSION 3.20.0)
project(libstorage)

include_directories(${LIBSTORAGE_HEADERS_DIR})

set(SOURCE_FILES libstorage.cpp)

add_library(libstorage ${SOURCE_FILES})
