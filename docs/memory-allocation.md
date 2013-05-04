WARNING!!! This document is outdated.


Memory Allocation
=================

There are three categories of Granary allocators:
* heap_alloc and heap_free
* allocate_executable(thread), free_executable(thread)
* allocate_executable(cpu), free_executable(cpu)

User Space
----------


Kernel Space
------------

- key ideas:
  - all heap_alloc allocate from a CPU-private bump-pointer arena
  - all heap_free are NOPs
  - leaving granary's stack resets the bump pointer (i.e. all of that heap
    allocated stuff is transient) and only needs to stay as long as we are
    doing instruction encoding/decoding