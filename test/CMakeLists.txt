project(ALBUnitTest)

if(WIN32)
  add_definitions(-D_WIN32_WINNT=0x0501)
ENDIF(WIN32)

# Visual Studio 2012 only supports up to 8 template parameters in
# std::tr1::tuple by default, but gtest requires 10
if(MSVC AND MSVC_VERSION EQUAL 1700)
  add_definitions(-D_VARIADIC_MAX=10)
endif ()


include_directories("${PROJECT_SOURCE_DIR}/../include")
include_directories("${PROJECT_SOURCE_DIR}/../util/gtest-1.7.0/include")

add_executable(ALBUnitTest 
               ALBAllocatorBaseTest.cpp 
               ALBSharedCascadingAllocatorsTest.cpp
               ALBSharedFreeListTest.cpp
               ALBSharedHeapTest
               ALBStackAllocatorTest.cpp
               ALBTestHelpers.cpp
               main.cpp)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    target_link_libraries(ALBUnitTest ${Boost_LIBRARIES})
endif()

add_dependencies(ALBUnitTest gtest)
add_dependencies(ALBUnitTest ALB)


