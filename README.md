AllocatorBuilder
================

A highly composable C++ heap based on ideas from Andrei Alexandrescu, presented at the C++ and Beyond 2013 seminar.

The background behind the idea is to compensate the main problem of malloc and the other standard allocators, separation of memory pointer and the allocated size. This makes it very difficult for all kind of allocators to handle in a fast way memory allocations and deallocations. All users of manually allocated memory have to store the size anyway to ensure that no access beyond the length of the allocated buffer takes place.

C++11 features are used as far as Visual Studio 2012 supports them.

Motivation
----------
Inside a method temporary raw memory is needed. Most of the time the amount memory would fit on the stack and so :alloca() is ones friend. But in seldom cases more is needed and so :malloc() must be used.
So the code could look like this
  
  const int STACK_THRESHOLD = 1024;
  int neededBytes = 42
  bool wouldFitOnTheStack = neededBytes < STACK_THRESHOLD;
  char* p = wouldFitOnStack? : (char*):alloca(neededBytes) : nullptr;
  
  std::unique_ptr<char[]> heapAllocated(!wouldFitOnStack? new char[neededBytes] : nullptr);
  
  if (!p)
  {
    p = heapAllocated.get();
  }
  
  // ... work with p ... 

Everybody would agree that this is not nice! So what if one could encapsulate this into something like:

  const int STACK_THRESHOLD = 1024;
  int neededBytes = 42

  typedef FallbackAllocator<StackAllocator<STACK_THRESHOLD>, Mallocator<>> LocalAllocator; 
  LocalAllocator localAllocator;
  
  auto block = localAllocator.allocate(neededBytes);
  SCOPED_EXIT { localAllocator.deallocate(block); };
  
  // ... work with block.ptr ... 
 
So, isn't this nicer? 
  
  
Content
-------
  * Bucketizer
  * FallbackAllocator
  * Mallocator
  * Segregator
  * SharedFreeList
  * SharedCascadingAllocator
  * SharedHeap
  * StackAllocator
  

Author 
------
  Felix Petriconi
  

Contributions
-------------

If you're wanting to make changes, please clone the git repository at [sources]

git://github.com/FelixPetriconi/AllocatorBuilder.git

  
License
-------
  Boost 1.0 License


Version
-------
  0.0.1

Requirements
------------
  * C++ 11 (partly)
  * boost 1.54


Platform
--------
  No platform specific features used, tested so far only with MS Visual Studio 2012

ToDo
----
  * Add more UnitTests
  * Add AlignedMallocator
  * Add FreeList
  * Add CascadingAllocators
  * Add expand at the missing places


