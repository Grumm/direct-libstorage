cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

project(oa VERSION 1.0 LANGUAGES CXX)

FIND_PACKAGE( Boost 1.60 COMPONENTS program_options program_options date_time REQUIRED )
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )

ADD_EXECUTABLE(oa main.cpp TradingAlgo.cpp algo1.cpp)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-unused")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -flto")


TARGET_LINK_LIBRARIES(oa LINK_PUBLIC ${Boost_LIBRARIES})
