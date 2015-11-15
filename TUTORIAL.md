# Tutorial

# Temporary memory inside a function #
### Problem
Let's say that we need an unknown amount of memory inside a function. There are different ways to handle this:
* Create a buffer on the stack that is large enough to handle all cases.
** If it is absolute sure that we never need more memory and that there is never the risk of a stack overflow then this is easiest solution and we should go for it.
~~~C++
  char buffer[1024];
~~~

* We can dynamically allocate it on the stack with ::alloca(). 
** If we are not sure about the needed amount of memory this is more dynamic solution than the fixed buffer. Of course this is as fast as a fixed buffer on the stack, but it introduces the risk of a stack overflow.
~~~C++
  char* buffer = reinterpret_cast<char*>(::alloca(1024));
~~~

* If would like to avoid by any chance a stack overflow we could go for the heap by using a unique_ptr<>
** The probability of going out of memory is normally much lower than a stack overflow. But it has the disadvantage that in most of the cases it is much slower, because a call to ::malloc() normally implies a lock and other threads cannot allocate or deallocate at the same time.
~~~C++
  unique_ptr<char[]> buffer(new char[1024]);
~~~

### Solution
The best would be, if we would have a solution that takes the memory if possible from the stack and if not it goes to the heap. One way would be to use a fallback_allocator in the following way:
~~~C++
  const int StackThreshold = 4096;
  typedef fallback_allocator<stack_allocator<StackThreshold>, mallocator> LocalAllocator; 
  LocalAllocator myAllocator;

  auto mem = myAllocator.allocate(1024);
  auto buffer = reinterpret_cast<char*>(mem.ptr);

  ...

  myAllocator.deallocate(mem); // this should be better put into a scope finalizer like SCOPE_EXIT
~~~
Now we get the memory from the stack until 4096 bytes. Just beyond this, a call to ::malloc() would be done. From my point of view this is a cleaner solution. As well it offers the possibility, easily to expand or reallocate the returned block; by design it automatically covers moving data allocated on the stack to the heap, if not enough memory is left on the stack.

## Memory pool for strings
### Problem
Many string class designs use today for short strings a so called "small string optimization". That means that the characters of the string a stored in a const buffer member or the class if the string is shorter than 16 bytes e.g. Memory is allocated from the heap if the representing text is longer. Something similar like: (I do not claim that this is best possible implementation :-) )
~~~C++
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

   ~string() {
   }
};
~~~
Let's further assume that SIMD CPU instructions are used in further methods to implement comparison, copy etc. These instructions work on modern CPUs (at least on x86) always on 16 bytes chunks. So we would allocate on the heap the memory in 16 bytes aligned chunks, e.g. 32, 48, 64 etc. So it would be good, if the allocator, that we want to use, automatically returns such aligned memory. But that is not all. Further we assume that our application does lot's of string operations. So there would be lot's of allocation and deallocation operations. We already discussed that these operations are quite expensive. So why not keep the not any more used memory blocks in a pool (or free-list) and reuse them soon. So we start with a freelist:
~~~C++
freelist<mallocator, 0, 32, 1024> myFreelist;
~~~
This instance allocates all memory from the heap (via the mallocator) and it stores elements up to 32 bytes in the pool. The max pool size is 1024 elements. This allocator would help for strings that would never be bigger than 32 bytes. In normal cases they get bigger, so let's use a bucketizer that can manage several free-lists:
~~~C++
typedef shared_freelist<mallocator, internal::DynasticDynamicSet, internal::DynasticDynamicSet, 1024> FList;
bucketizer<FList, 17, 512, 16> myBucket;
~~~
So the typedef specifies a free-list that's min- and max size can be changed during runtime. (Normally this values are set during compile time). The bucketizer creates free-lists in increasing 16 bytes steps capacity, [17-32], [33-48], [49-64], ... Each free-list has a maximum capacity of 1024 elements. So now we can handle strings up to the length of 512 bytes. If this strings can get longer, we have to add a segregator:
~~~C++
typedef shared_freelist<mallocator, internal::DynasticDynamicSet, internal::DynasticDynamicSet, 1024> FList;
segregator<512, bucketizer<FList, 17, 512, 16>, mallocator> myAllocator;
~~~
Now all allocations up to 512 bytes are handled by the bucketizer and all above is taken directly from the heap.
The code from the string class would be rewritten to this:
~~~C++
namespace {
  using namespace alb;
  typedef shared_freelist<mallocator, internal::DynasticDynamicSet, internal::DynasticDynamicSet, 1024> FList;
  segregator<512, bucketizer<FList, 17, 512, 16>, mallocator> myAllocator;
}

class string {
  char local_[16];
  alb::block chunk_;
  char *data_;
public:
  string(const char* t) : data_(nullptr) {
    int len = t? strlen(t) : 0;
    if (len == 0) 
      return;

    if (len < 16) {
      strcpy(local_, t, len);
    }
    else {
	  chunk_ = myAllocator.allocate(len + 1);
	  if (chunk_) {
      data_ = reinterpret_cast<char*>(chunk_.ptr);
      strcpy(data_ , t, len);
		  data_ [len] = '\0';
	  }
  }

  ~string() {
    myAllocator.deallocate(chunk_);
  }
};

~~~
## Creation of unique_ptr and shared_ptr objects
### Problem
The std::make_unique and std::make_shared creation function cannot take their memory from custom allocators. They always allocate the memory via the global ::new() functions.

### Solution
Create an ALB allocator of any that type that has as the outmost allocator an alb::affix_allocator.
~~~C++
using MyAllocator = alb::affix_allocator<alb::stack_allocator<4096>, size_t>
~~~

Now use the ALB variants of these factory functions.

~~~C++
MyAllocator myAllocator;

// Let's create a unique Foo that needs as parameter Foo1stParam and Foo2ndParam
auto myFoo = alb::make_unique<Foo>(myAllocator, Foo1stPara, Foo2ndParam);

// Let's create an array of 42 Foo
auto moreFoo = alb::make_unique<Foo[]>(myAllocator, 42);

// Let's create an shared object of Foo
auto mySharedFoo = alb::make_shared<Foo>(myAllocator, Foo1stPara, Foo2ndParam);
~~~

## Construction and Destruction of objects in random order on the stack
### Problem

### Solution

~~~C++
~~~

## Replacement of global ::new() and ::delete()
### Problem
Let's assume that we see potential in replacing the standard heap with our own, custom optimized version. In general this is pretty easy. We only have to replace the global new(), new[]() and the corresponding delete operators. A regular implementation might look like:
~~~C++
void* operator new(std::size_t sz) {
  auto result = ::malloc(sz);
  if (result) 
    return result;

  throw std::bad_alloc();
}

void* operator new[](std::size_t sz) {
  auto result = ::malloc(sz);
  if (result) 
    return result;

  throw std::bad_alloc();
}

void operator delete(void* ptr)
{
  if (ptr)
    ::free(ptr);
}

void operator delete[](void* ptr)
{
  if (ptr)
    ::free(ptr);
}
~~~
### Solution
Let's make the assumption that MyAllocator is a combined allocator from the Allocator-Builder (ALB) library that should serve as the basis of the memory allocation.

We cannot use this MyAllocator directly, because the ALB relies on the fact that always an alb::block must be passed to deallocate().
~~~C++
struct block {
  void* ptr;
  size_t length;
};
~~~
Beside the pointer to the memory it contains the length. The operator ::delete(void*) does not get passed the size of the block. So we have to keep track by ourselves. Probably the easiest way is to encapsulate our MyAllocator within an affix_allocator with a prefix that contains the length of the block. I choose for the moment the prefix of type uint32_t, because that can cover memory allocations up to 4GB which is fine for now. (When we come to the point, that it might happen, our application needs more than 4GB in a single chunk, then we can change the prefix to uint64_t).
~~~C++
typedef affix_allocator<MyAllocator,uint32_t> MyAllocatorPrefixed;
~~~
Let's make as a next step two convenience function for allocation and releasing the memory:
~~~
namespace {
  MyAllocatorPrefixed myGlobalAllocator;
}

void* operatorNewInternal(std::size_t n) {
  if (n == 0) {
    n = 1;
  }
  auto result = myGlobalAllocator.allocate(sz);
  if (result) 
  {
    auto p = myGlobalAllocator.outerToPrefix(result);
    *p = static_cast<uint32_t>(result.length);
    return result.ptr;
  }
  throw std::bad_alloc();
}

void operatorDeleteInternal(void* ptr) {
  if (ptr) {
    alb::block pseudoBlock(ptr, 1);
    auto p = myGlobalAllocator.outerToPrefix(pseudoBlock);
    alb::block realBlock(ptr, *p);
    myGlobalAllocator.deallocate(realBlock);
  }  
}
~~~C++
The global operators would become:
~~~
void* operator new(std::size_t sz) {
  return operatorNewInternal(sz);
}

void* operator new[](std::size_t sz) {
  return operatorNewInternal(sz);
}

void operator delete(void* ptr)
{
  operatorDeleteInternal(ptr);
}

void operator delete[](void* ptr)
{
  operatorDeleteInternal(ptr);
}
~~~
We are almost done. Finally we have to cope the problem of unknown initialization order before main(). So we encapsulate the allocator into a singleton and then we are done. 
~~~C++
namespace {
  MyAllocatorPrefixed& myGlobalAllocator() {
    static MyAllocatorPrefixed instance;
    return instance;
  }
}
~~~
Or even better, we use a ready template from the ALB library and change the code of the helper functions to:
~~~C++
void* operatorNewInternal(std::size_t n) {
  if (n == 0) {
    return nullptr;
  }
  auto result = alb::global_allocator<MyAllocator>::instance().allocate(n);
  if (result) 
  {
    auto p = alb::global_allocator<MyAllocator>::instance().outerToPrefix(result);
    *p = static_cast<uint32_t>(result.length);
    return result.ptr;
  }
  throw std::bad_alloc();
}

void operatorDeleteInternal(void* ptr) {
  if (ptr) {
    alb::block pseudoBlock(ptr, 1);
    auto p = alb::global_allocator<MyAllocator>::instance().outerToPrefix(pseudoBlock);
    alb::block realBlock(ptr, *p);
    alb::global_allocator<MyAllocator>::instance().deallocate(realBlock);
  }  
}
~~~
## Custom STL compatible allocator
### Problem
Let's assume for the moment that we have a STL container under heavy load and with lot's of size changes or lot's of re-balancing in case of a tree base one. (I know, that changing the design in a way, that regularly rebalancing of the tree is avoided totally is even better.) But stick to the idea for the moment.
So we have to implement the complete interface as it is described e.g. in Nicolai Josuttis excellent book "The Standard Template Library". --> Refer to adendum pdf!
###Solution
Not very much must be done. Just take the generic alb::stl_allocator and we are done:
~~~C++
  alb::global_allocator<MyAllocator> globalInstance;
  struct MyValues;
  
  typedef alb::stl_allocator<MyValues, alb::global_allocator<MyAllocator>>  MySTLAllocator;
~~~
Now we can use in in our code as:
~~~C++
  std::vector<MyValues, MySTLAllocator> myValueVector;
~~~
Or when we have to deal with a map:
~~~C++
  alb::global_allocator<MyAllocator> globalInstance;
  struct MyValues;
  typedef std::pair<const int, MyValue> MyMapPair;
  typedef alb::stl_allocator<MyMapPair, alb::global_allocator<MyAllocator>>  MyMapAllocator;

  std::map<int, MyValue, std::less<int>, MyMapAllocator> myValueMap;
~~~
  
 
