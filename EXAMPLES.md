Examples
========

= Temporary memory inside a method
## Problem
Let's say that we need an unknown amount of memory inside a function. There are different ways to handle this:
* Create a buffer on the stack that is large enough to handle all cases.
** If it is absolute sure that we never need more memory and that there is never the risk of a stack overflow then this is easiest solution and we should go for it.
~~~
  char buffer[1024];
~~~
= Replacement of global ::new() and ::delete
** If we are not sure about the needed amount of memory this is more dynamic solution than the fixed buffer. Of course this is as fast as a fixed buffer on the stack, but it introduces the risk of a stack overflow.
~~~
  char* buffer = reinterpret_cast<char*>(::alloca(1024));
~~~
* If would like to avoid by any chance a stack overflow we could go for the heap by using a unique_ptr<>
** The probability of going out of memory is normally much lower than a stack overflow. But it has the disadvantage that in most of the cases it is much slower, because a call to ::malloc() normally implies a lock and other threads cannot allocate or deallocate at the same time.
~~~
  unique_ptr<char[]> buffer(new char[1024]);
~~~

## Solution
The best would be, if we would have a solution that takes the memory if possible from the stack and if not it goes to the heap. One way would be to use a fallback_allocator in the following way:
~~~
  const int StackThreshold = 4096;
  typedef fallback_allocator<stack_allocator<StackThreshold>, mallocator> LocalAllocator; 
  LocalAllocator myAllocator;

= Creating a custom STL compatible allocator
  auto buffer = reinterpret_cast<char*>(mem.ptr);

  ...

  myAllocator.deallocate(mem); // this should be better put into a scope finalizer like SCOPE_EXIT
~~~
Now we get the memory from the stack until 4096 bytes. Just beyond this, a call to ::malloc() would be done. From my point of view this is a cleaner solution. As well it offers the possibility, easily to expand or reallocate the returned block; by design it automatically covers moving data allocated on the stack to the heap, if not enough memory is left on the stack.

# Memory pool for strings
## Problem
Many string class designs use today for short strings a so called "small string optimization". That means that the characters of the string a stored in a const buffer member or the class if the string is shorter than 16 bytes e.g. Memory is allocated from the heap if the representing text is longer. Something similar like: (I do not claim that this is best possible implementation :-) )
~~~
class string {
  char local_[16];
  unique_ptr<char[]> p_;
  int length_;
 public:
   string(const char* t) : length_(0) {
     int len = t? strlen(t) : 0;
     if (len == 0) 
       return;

     if (len < 16) {
       strcpy(local_, t, len);
     }
     else {
       length_ = len+1;
       p_.reset(new char[length_]);
       strcpy(p_.get(), t, len);
       p_[len] = '\0';
     }
   }  
};
~~~
Let's further assume that SIMD CPU instructions are used in further methods to implement comparison, copy etc. These instructions work on modern CPUs (at least on x86) always on 16 bytes chunks. So we would allocate on the heap the memory in 16 bytes aligned chunks, e.g. 32, 48, 64 etc. So it would be good, if the allocator, that we want to use, automatically returns such aligned memory. But that is not all. Further we assume that our application does lot's of string operations. So there would be lot's of allocation and deallocation operations. We already discussed that these operations are quite expensive. So why not keep the not any more used memory blocks in a pool (or free-list) and reuse them soon. So we start with a freelist:
~~~
freelist<mallocator, 0, 32, 1024> myFreelist;
~~~
This instance allocates all memory from the heap (via the mallocator) and it stores elements up to 32 bytes in the pool. The max pool size is 1024 elements. This allocator would help for strings that would never be bigger than 32 bytes. In normal cases they get bigger, so let's use a bucketizer that can manage several free-lists:
~~~
typedef shared_freelist<mallocator, internal::DynasticDynamicSet, internal::DynasticDynamicSet, 1024> FList;
bucketizer<FList, 17, 512, 16> myBucket;
~~~
So the typedef specifies a free-list that's min- and max size can be changed during runtime. (Normally this values are set during compile time). The bucketizer creates free-lists in increasing 16 bytes steps capacity, [17-32], [33-48], [49-64], ... Each free-list has a maximum capacity of 1024 elements. So now we can handle strings up to the length of 512 bytes. If this strings can get longer, we have to add a segregator:
~~~
typedef shared_freelist<mallocator, internal::DynasticDynamicSet, internal::DynasticDynamicSet, 1024> FList;
segregator<512, bucketizer<FList, 17, 512, 16>, mallocator> myAllocator;
~~~
Now all allocations up to 512 bytes are handled by the bucketizer and all above is taken directly from the heap.


= Replacement of global ::new() and ::delete

= Creating a custom STL compatible allocator