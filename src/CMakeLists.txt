project(ALB)

include_directories("${PROJECT_SOURCE_DIR}/../include;")

if(WIN32)
  add_definitions(-D_WIN32_WINNT=0x0501)
endif(WIN32)

set(SOURCE
  allocator_base.cpp
        internal/array_creation_evaluator.cpp
)

set(HEADERS
        ../include/alb/affix_allocator.hpp
        ../include/alb/aligned_mallocator.hpp
        ../include/alb/allocator_base.hpp
        ../include/alb/allocator_with_stats.hpp
        ../include/alb/bucketizer.hpp
        ../include/alb/config.hpp
        ../include/alb/cascading_allocator.hpp
        ../include/alb/fallback_allocator.hpp
        ../include/alb/global_allocator.hpp
        ../include/alb/heap.hpp
        ../include/alb/mallocator.hpp
        ../include/alb/memory_corruption_detector.hpp
        ../include/alb/null_allocator.hpp
        ../include/alb/segregator.hpp
        ../include/alb/freelist.hpp
        ../include/alb/shared_heap.hpp
        ../include/alb/stack_allocator.hpp
        ../include/alb/stl_allocator.hpp
        ../include/alb/stl_allocator_adapter.hpp
        ../include/alb/internal/affix_helper.hpp
        ../include/alb/internal/array_creation_evaluator.hpp
        ../include/alb/internal/atomic_shared_ptr.hpp
        ../include/alb/internal/dynastic.hpp
        ../include/alb/internal/heap_helpers.hpp
        ../include/alb/internal/noatomic.hpp
        ../include/alb/internal/reallocator.hpp
        ../include/alb/internal/shared_helpers.hpp
        ../include/alb/internal/stack.hpp
        ../include/alb/internal/traits.hpp
)

include_directories(${Boost_INCLUDE_DIRS})

add_library(ALB ${SOURCE} ${HEADERS})
set_property(TARGET ALB PROPERTY CXX_STANDARD 20)
set_property(TARGET ALB PROPERTY CXX_STANDARD_REQUIRED ON)

