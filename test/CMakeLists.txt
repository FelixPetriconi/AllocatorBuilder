project(ALBUnitTest)

if(WIN32)
  add_definitions(-D_WIN32_WINNT=0x0501)
ENDIF(WIN32)

# Visual Studio 2012 only supports up to 8 template parameters in
# std::tr1::tuple by default, but gtest requires 10
if(MSVC AND MSVC_VERSION EQUAL 1700)
  add_definitions(-D_VARIADIC_MAX=10)
endif ()


include_directories("${PROJECT_SOURCE_DIR}/../.")
include_directories("${PROJECT_SOURCE_DIR}/../util/gtest-1.7.0/include")

set(HEADERS
  TestHelpers/Base.h
  TestHelpers/AffixGuard.h
  TestHelpers/Algorithm.h
  TestHelpers/AllocatorBaseTest.h
  TestHelpers/Data.h
  TestHelpers/Thread.h
  TestHelpers/UsedMemGenerator.h  
)

set(SOURCE
  AffixAllocatorTest.cpp
  AllocatorBaseTest.cpp
  AllocatorWithStatsTest.cpp
  BucketizerTest.cpp
  CascadingAllocatorsTest.cpp
  FallbackAllocatorTest.cpp 
  HeapTest
  MallocatorTest.cpp
  SegregatorTest.cpp    
  FreeListTest.cpp
  StackAllocatorTest.cpp
  main.cpp
  TestHelpers/Base.cpp
)

add_executable(ALBUnitTest ${SOURCE} ${HEADERS})

include_directories(${Boost_INCLUDE_DIRS})
LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
add_definitions(-DBOOST_ALL_NO_LIB)

add_dependencies(ALBUnitTest gtest)
add_dependencies(ALBUnitTest ALB)

set_property(TARGET ALBUnitTest PROPERTY CXX_STANDARD 14)
set_property(TARGET ALBUnitTest PROPERTY CXX_STANDARD_REQUIRED ON)
target_link_libraries(ALBUnitTest ${Boost_LIBRARIES} gtest ALB)


