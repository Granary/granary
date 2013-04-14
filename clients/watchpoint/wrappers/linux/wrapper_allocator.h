/*
 * wrapper_allocator.h
 *
 *  Created on: 2013-04-11
 *      Author: akshayk
 */

#ifndef WRAPPER_ALLOCATOR_H_
#define WRAPPER_ALLOCATOR_H_

#include "clients/watchpoint/descriptors/wrapper_interface.h"


#if defined(CAN_WRAP___kmalloc) && CAN_WRAP___kmalloc
#define WRAPPER_FOR___kmalloc 1
FUNCTION_WRAPPER(__kmalloc, (void*), (size_t size, gfp_t flags), {
    printf("kmalloc wrapper");
    void *watch_ptr = __kmalloc(size, flags);
    client::add_watchpoint(watch_ptr, size);
    printf(" %lx\n", watch_ptr);
    return watch_ptr;
})
#endif

#if defined(CAN_WRAP_kfree) && CAN_WRAP_kfree
#define WRAPPER_FOR_kfree 1
FUNCTION_WRAPPER(kfree, (void), (void *addr), {
    printf("kfree wrapper");
    client::remove_watchpoint(addr);
    kfree(addr);
})
#endif

#if defined(CAN_WRAP_kmem_cache_alloc) && CAN_WRAP_kmem_cache_alloc
#define WRAPPER_FOR_kmem_cache_alloc 1
FUNCTION_WRAPPER(kmem_cache_alloc, (void*), (struct kmem_cache *s, gfp_t gfpflags), {
    printf("kmem_cache_alloc wrapper");
    void *watch_ptr = kmem_cache_alloc(s, gfpflags);
    client::add_watchpoint(watch_ptr, s->size);
    printf(" %lx\n", watch_ptr);
    return watch_ptr;
})
#endif

#if defined(CAN_WRAP_kmem_cache_alloc_trace) && CAN_WRAP_kmem_cache_alloc_trace
#define WRAPPER_FOR_kmem_cache_alloc_trace 1
FUNCTION_WRAPPER(kmem_cache_alloc_trace, (void*), (struct kmem_cache *s, gfp_t gfpflags, size_t size), {
    printf("kmem_cache_alloc_trace wrapper");
    void *watch_ptr = kmem_cache_alloc_trace(s, gfpflags, size);
    client::add_watchpoint(watch_ptr, size);
    printf(" %lx\n", watch_ptr);
    return watch_ptr;
})
#endif


#if defined(CAN_WRAP_kmem_cache_free) && CAN_WRAP_kmem_cache_free
#define WRAPPER_FOR_kmem_cache_free 1
FUNCTION_WRAPPER(kmem_cache_free, (void), (struct kmem_cache *s, void *ptr), {
    printf("kmem_cache_free wrapper : %lx\n", ptr);
    client::remove_watchpoint(ptr);
    return kmem_cache_free(s, ptr);
})
#endif

#endif /* WRAPPER_ALLOCATOR_H_ */
