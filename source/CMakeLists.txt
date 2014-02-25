project(ALB)

include_directories("${PROJECT_SOURCE_DIR}/../include")

if(WIN32)
  add_definitions(-D_WIN32_WINNT=0x0501)
endif(WIN32)

set(SOURCE
  ALBAllocatorBase.cpp
)

set(HEADERS
  ../include/ALBAffixAllocator.h
  ../include/ALBAlignedMallocator.h
  ../include/ALBAllocatorBase.h
  ../include/ALBAllocatorWithStats.h
  ../include/ALBBucketizer.h
  ../include/ALBFallbackAllocator.h
  ../include/ALBHelperStack.h
  ../include/ALBMallocator.h
  ../include/ALBMemmoryCorruptionDetector.h
  ../include/ALBSegregator.h
  ../include/ALBSharedCascadingAllocators.h
  ../include/ALBFreeList.h
  ../include/ALBSharedHeap.h
  ../include/ALBStackAllocator.h
  ../include/ALBTraits.h
)

add_library(ALB ${SOURCE} ${HEADERS})