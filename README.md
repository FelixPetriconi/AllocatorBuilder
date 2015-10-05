Allocator Builder {#mainpage}
=================

A highly composable, policy based C++ allocator.

The layout idea of the library is was presented by [Andrei Alexandrescu](http://erdani.com/) at the [C++ and Beyond 2013](http://cppandbeyond.com/) seminar and at the [CppCon 2015](http://cppcon2015.sched.org/event/95b6c3282248b7e2595c5c3182d7652b).

The background behind the idea is to compensate the main problem of malloc and the other current standard allocators, separation of memory pointer and the allocated size. This makes it very difficult for all kind of memory management to handle in a fast way memory allocations and especially deallocations, because the memory manager must find the corresponding length of the pointer that shall be free.
Additionally all users of manually raw allocated memory have to store the size anyway to ensure that no access beyond the length of the allocated buffer takes place.

A second idea behind this allocator library is, that one can compose for every use case a special designed one. 
Example use cases:
  * Collect statistic information about the memory usage profile.
  * Apply guards to memory allocated blocks to detect buffer under- or overflows, even in release mode of the compiled application.
  * Wait free allocations in a single threaded environment

So the approach is, every allocator returns such a block:
~~~
struct block {
  void  *ptr;
  size_t length;
};
~~~

And a request goes this way:
~~~
auto myMemBlock = allocator.allocate(42);
~~~

Motivation
----------
Raw memory is temporary needed inside a method. Most of the time the amount memory would fit on the stack and so :alloca() is ones friend. But in seldom cases more is needed and so :malloc() must be used. (Allocation on the stack is much faster because it is a wait-free operation and in many cases the allocated memory is much more cache friendly.)

So the code could look like this
~~~ 
const int STACK_THRESHOLD = 1024;
int neededBytes = 42
bool wouldFitOnTheStack = neededBytes < STACK_THRESHOLD;
char* p = wouldFitOnStack? : (char*):alloca(neededBytes) : nullptr;

std::unique_ptr<char[]> heapAllocated(!wouldFitOnStack? new char[neededBytes] : nullptr);

if (!p) {
  p = heapAllocated.get();
}

// ... work with p[0..neededBytes-1]
~~~

Everybody would agree that this is not nice! So what if one could encapsulate this into something like:
~~~
const int STACK_THRESHOLD = 1024;
int neededBytes = 42

typedef fallback_allocator<stack_allocator<STACK_THRESHOLD>, mallocator> LocalAllocator; 
LocalAllocator localAllocator;

auto block = localAllocator.allocate(neededBytes);
auto p = static_cast<char*>(block.ptr);

// ... work with p[0..neededBytes-1]

localAllocator.deallocate(block); // better to be deleted by a scope finalizer
~~~  
So, isn't this much cleaner? 


A more advanced allocator with different sized buckets as one are used in [jemalloc](http://www.canonware.com/jemalloc/) would look like:
~~~
// This defines a FreeList that is later configured by the bucketizer to its size
typedef freelist<mallocator, DynamicSetSize, DynamicSetSize> FList;

// All allocation requests up to 3584 bytes are handled by the cascade of FLists.
// Sizes from 3584 till 4MB are handled by the Heap and all beyond that are forwarded
// directly to the normal OS
typedef segregator<
  8, freelist<mallocator, 0, 8>, segregator<
    128, bucketizer<FList, 1, 128, 16>, segregator<
      256, bucketizer<FList, 129, 256, 32>, Segegator<
        512, bucketizer<FList, 257, 512, 64>, Segegator<
          1024, bucketizer<FList, 513, 1024, 128>, Segegator<
            2048, bucketizer<FList, 1025, 2048, 256>, Segegator<
              3584, bucketizer<FList, 2049, 3584, 512>, Segegator<
                4072 * 1024, cascading_allocator<Heap<mallocator, 1018, 4096>>, mallocator
              >
            >
          >
        >
      >
    >
  >>AdvancedAllocator;
~~~
 
  
Allocator Overview
------------------

|Allocator                 |Description                                                                 |
---------------------------|----------------------------------------------------------------------------
| affix_allocator          | Allows to automatically pre- and sufix allocated regions. |
| allocator_with_stats     | An allocator that collects a configured number of statistic information, like number of allocated bytes, number of successful expansions and high tide |
| bucketizer               | Manages a bunch of Allocators with increasing bucket size |
| fallback_allocator       | Either the default Allocator can handle a request, otherwise it is passed to a fall-back Allocator |
| (aligned_)mallocator     | Provides and interface to systems ::malloc(), the aligned variant allocates according to a given alignment  |
| segregator               | Separates allocation requests depending on a threshold to Allocator A or B |
| (shared_)freelist        | Manages a list of freed memory blocks in a list for faster re-usage. (The Shared variant is thread safe) |
| (shared_)cascading_allocator | Manages in a thread safe way Allocators and automatically creates a new one when the previous are out of memory. (The Shared variant is thread safe, but it needs further improvements, because it does not frees unused allocators) |
| (shared_)heap            | A heap block based heap. (The Shared variant is thread safe manner with minimal overhead and as far as possible in a lock-free way.) |
| stack_allocator          | Provides a memory access, taken from the stack |

Documentation
-------------
  Online Documentation is available on [GitHub.io] (http://felixpetriconi.github.io/AllocatorBuilder/index.html) as well.
  
  There is the begin of a tutorial in the [tutorial section](http://felixpetriconi.github.io/AllocatorBuilder/md__t_u_t_o_r_i_a_l.html)

Author 
------
  Felix Petriconi (felix at petriconi.net)
  

Contributions
-------------

  Comments, feedback or contributions are welcome!

  
License
-------
  Boost 1.0 License


Version
-------
  0.9.6

Prerequisites
-------------
  * C++ 11 (partly, as far as Visual Studio 2015 supports it)
  * boost 1.59.0 (lockfree, thread, assert, option)
  * CMake 2.8 or later
  * GoogleTest 1.7 (Is part of the repository, because it's CMakeFiles.txt needs some patches to compile with Visual Studio)


Platform
--------
| Compiler | Status |
-----------|---------
| Visual Studio 2013 x64 | All tests pass |
| Visual Studio 2015 x64 | All tests pass |
| Debian x64, Clang 3.4  | All tests pass |
| Intel XE Inspector x64 | No detections  |
| Clang thread sanitizer | No detections  |

Installation Win
----------------
  * Have boost installed and the standard libs be build, install into D:\boost_1_59_0
  * Clone into e.g. D:\misc\AllocatorBuilder
  * Create a build folder, eg D:\misc\alb_build
  * Open a command prompt in that alb_build folder
  * Have CMake in the path
  * Execute cmake -G "Visual Studio 14 2015 Win64" ..\alb_build
  * Open created solution in .\alb_build\AllocatorBuilder.sln
  * Compile and run all test (if necessary add D:\boost_1_59_0\stage\lib to search path)
  
ToDo
----
  See issue list of [open enhancements] (https://github.com/FelixPetriconi/AllocatorBuilder/issues?labels=enhancement&page=1&state=open)


