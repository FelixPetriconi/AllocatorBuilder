project(ALB)

include_directories("${PROJECT_SOURCE_DIR}/../alb")

if(WIN32)
  add_definitions(-D_WIN32_WINNT=0x0501)
endif(WIN32)

set(SOURCE
  allocator_base.cpp
)

set(HEADERS
  ../alb/affix_allocator.hpp
  ../alb/aligned_mallocator.hpp
  ../alb/allocator_base.hpp
  ../alb/allocator_with_stats.hpp
  ../alb/bucketizer.hpp
  ../alb/cascading_allocator.hpp
  ../alb/fallback_allocator.hpp
  ../alb/global_allocator.hpp
  ../alb/heap.hpp
  ../alb/mallocator.hpp
  ../alb/memory_corruption_detector.hpp
  ../alb/null_allocator.hpp
  ../alb/segregator.hpp
  ../alb/freelist.hpp
  ../alb/shared_heap.hpp
  ../alb/stack_allocator.hpp
  ../alb/stl_allocator.hpp
  ../alb/internal/dynastic.hpp
  ../alb/internal/heap_helpers.hpp
  ../alb/internal/noatomic.hpp
  ../alb/internal/reallocator.hpp
  ../alb/internal/shared_helpers.hpp
  ../alb/internal/stack.hpp
  ../alb/internal/traits.hpp
)

include_directories(${Boost_INCLUDE_DIRS})

add_library(ALB ${SOURCE} ${HEADERS})
set_property(TARGET ALB PROPERTY CXX_STANDARD 14)
set_property(TARGET ALB PROPERTY CXX_STANDARD_REQUIRED ON)

