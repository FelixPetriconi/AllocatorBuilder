AllocatorBuilder
================

A highly composable C++ heap based on ideas from Andrei Alexandrescu, presented at the C++ and Beyond 2013 seminar.

The background behind the idea is to compensate the main problem of malloc and the other standard allocators, separation of memory pointer and the allocated size. This makes it very difficult for all kind of allocators to handle in a fast way memory allocations and deallocations. All users of manually allocated memory have to store the size anyway to ensure that no access beyond the length of the allocated buffer takes place.


Author: Felix Petriconi

License: Boost 1.0 license

Version: 0.1

Requirements: boost 1.54, C++ 11 (partly)

Platform: MS Visual Studio 2012 (tested)

ToDo: 