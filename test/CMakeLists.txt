project(ALBUnitTest)

if(WIN32)
  add_definitions(-D_WIN32_WINNT=0x0501)
ENDIF(WIN32)

include_directories("${PROJECT_SOURCE_DIR}/../include")
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
  HeapTest.cpp
  MallocatorTest.cpp
  MemoryTest.cpp
  NullAllocatorTest.cpp
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

set(CMAKE_CXX_STANDARD 17)

target_link_libraries(ALBUnitTest ${Boost_LIBRARIES} gtest ALB)


