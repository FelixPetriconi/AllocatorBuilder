cmake_minimum_required(VERSION 3.1)

project(AllocatorBuilder)

if(WIN32)
  add_definitions(-D_WIN32_WINNT=0x0501)
ENDIF(WIN32)

# Visual Studio 2012 only supports up to 8 template parameters in
# std::tr1::tuple by default, but gtest requires 10
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC" AND MSVC_VERSION EQUAL 1700)
  add_definitions(-D_VARIADIC_MAX=10)
endif ()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GCC")  
  #set(CMAKE_CXX_FLAGS "-O3 -fsanitize=thread -g -Wall -std=c++11")
  set(CMAKE_CXX_FLAGS "-O3 -g -Wall -std=c++11")
  set(CMAKE_LINK_FLAGS "-pthreads")
endif()

#set(BOOST_ROOT D:/boost_1_59_0)
set(Boost_USE_STATIC_LIBS        OFF)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME     OFF)
#set(BOOST_ALL_DYN_LINK           ON) 
find_package( Boost 1.59.0 REQUIRED COMPONENTS system thread chrono date_time)


add_subdirectory(util/gtest-1.7.0)
add_subdirectory(source)
add_subdirectory(test)

