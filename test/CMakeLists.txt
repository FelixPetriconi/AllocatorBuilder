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
  ALBTestHelpers.h
  ALBTestHelpersAffixGuard.h
  ALBTestHelpersAlgorithm.h
  ALBTestHelpersAllocatorBaseTest.h
  ALBTestHelpersData.h
  ALBTestHelpersThread.h
  ALBTestHelpersUsedMemGenerator.h  
)

set(SOURCE
  ALBAffixAllocatorTest.cpp
  ALBAllocatorBaseTest.cpp
  ALBAllocatorWithStatsTest.cpp
  ALBBucketizerTest.cpp
  ALBCascadingAllocatorsTest.cpp
  ALBFallbackAllocatorTest.cpp 
  ALBMallocatorTest.cpp
  ALBSegregatorTest.cpp    
  ALBFreeListTest.cpp
  ALBSharedHeapTest
  ALBStackAllocatorTest.cpp
  ALBTestHelpers.cpp
  Main.cpp
)

add_executable(ALBUnitTest ${SOURCE} ${HEADERS})

include_directories(${Boost_INCLUDE_DIRS})
LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
add_definitions(-DBOOST_ALL_NO_LIB)

add_dependencies(ALBUnitTest gtest)
add_dependencies(ALBUnitTest ALB)
target_link_libraries(ALBUnitTest ${Boost_LIBRARIES} gtest ALB)
