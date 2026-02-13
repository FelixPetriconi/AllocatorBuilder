cmake_minimum_required(VERSION 3.25)

project(AllocatorBuilder)

if(WIN32)
  add_definitions(-D_WIN32_WINNT=0x0501)
ENDIF(WIN32)

set(CMAKE_CXX_STANDARD 20)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")  
  #set(CMAKE_CXX_FLAGS "-O3 -fsanitize=thread -g -Wall -std=c++11")
  set(CMAKE_CXX_FLAGS "-O3 -g -Wall -std=c++20")
  set(CMAKE_LINK_FLAGS "-pthreads")
endif()


set(Boost_USE_STATIC_LIBS        OFF)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME     OFF)
#set(BOOST_ALL_DYN_LINK           ON) 
find_package( Boost REQUIRED COMPONENTS system thread chrono date_time)


add_subdirectory(modules/googletest)
add_subdirectory(src)
add_subdirectory(test)

