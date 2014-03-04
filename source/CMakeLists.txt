project(ALB)

include_directories("${PROJECT_SOURCE_DIR}/../include")

if(WIN32)
  add_definitions(-D_WIN32_WINNT=0x0501)
endif(WIN32)

set(SOURCE
  allocator_base.cpp
)

set(HEADERS
  ../include/affix_allocator.hpp
  ../include/aligned_mallocator.hpp
  ../include/allocator_base.hpp
  ../include/allocator_with_stats.hpp
  ../include/bucketizer.hpp
  ../include/cascading_allocator.hpp
  ../include/fallback_allocator.hpp
  ../include/internal/stack.hpp
  ../include/mallocator.hpp
  ../include/memory_corruption_detector.hpp
  ../include/internal/noatomic.hpp
  ../include/segregator.hpp
  ../include/freelist.hpp
  ../include/shared_heap.hpp
  ../include/stack_allocator.hpp
  ../include/internal/traits.hpp
)

include_directories(${Boost_INCLUDE_DIRS})

add_library(ALB ${SOURCE} ${HEADERS})