/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * allocators.h
 *
 *  Created on: 2013-07-08
 *      Author: Peter Goodman
 */

#ifndef CLIENT_CFG_ALLOCATORS_H_
#define CLIENT_CFG_ALLOCATORS_H_


#if defined(CAN_WRAP___kmalloc) && CAN_WRAP___kmalloc
    CFG_MEMORY_ALLOCATOR(__kmalloc)
#endif


#if defined(CAN_WRAP___kmalloc_track_caller) && CAN_WRAP___kmalloc_track_caller
    CFG_MEMORY_ALLOCATOR(__kmalloc_track_caller)
#endif


#if defined(CAN_WRAP___kmalloc_node) && CAN_WRAP___kmalloc_node
    CFG_MEMORY_ALLOCATOR(__kmalloc_node)
#endif


#if defined(CAN_WRAP___kmalloc_node_track_caller) && CAN_WRAP___kmalloc_node_track_caller
    CFG_MEMORY_ALLOCATOR(__kmalloc_node_track_caller)
#endif


#if defined(CAN_WRAP___krealloc) && CAN_WRAP___krealloc
    CFG_MEMORY_REALLOCATOR(__krealloc)
#endif


#if defined(CAN_WRAP_krealloc) && CAN_WRAP_krealloc
    CFG_MEMORY_REALLOCATOR(krealloc)
#endif


#if defined(CAN_WRAP_kfree) && CAN_WRAP_kfree
    CFG_MEMORY_DEALLOCATOR(kfree)
#endif


#if defined(CAN_WRAP_kzfree) && CAN_WRAP_kzfree
    CFG_MEMORY_DEALLOCATOR(kzfree)
#endif


#if defined(CAN_WRAP_kmem_cache_alloc) && CAN_WRAP_kmem_cache_alloc
    CFG_MEMORY_ALLOCATOR(kmem_cache_alloc)
#endif


#if defined(CAN_WRAP_kmem_cache_alloc_trace) && CAN_WRAP_kmem_cache_alloc_trace
    CFG_MEMORY_ALLOCATOR(kmem_cache_alloc_trace)
#endif


#if defined(CAN_WRAP_kmem_cache_alloc_node) && CAN_WRAP_kmem_cache_alloc_node
    CFG_MEMORY_ALLOCATOR(kmem_cache_alloc_node)
#endif


#if defined(CAN_WRAP_kmem_cache_free) && CAN_WRAP_kmem_cache_free
    CFG_MEMORY_DEALLOCATOR(kmem_cache_free)
#endif


#if defined(CAN_WRAP___get_free_pages) && CAN_WRAP___get_free_pages
    CFG_MEMORY_ALLOCATOR(__get_free_pages)
#endif


#if defined(CAN_WRAP_get_zeroed_page) && CAN_WRAP_get_zeroed_page
    CFG_MEMORY_ALLOCATOR(get_zeroed_page)
#endif


#if defined(CAN_WRAP_alloc_pages_exact) && CAN_WRAP_alloc_pages_exact
    CFG_MEMORY_ALLOCATOR(alloc_pages_exact)
#endif


#if defined(CAN_WRAP_free_pages_exact) && CAN_WRAP_free_pages_exact
    CFG_MEMORY_DEALLOCATOR(free_pages_exact)
#endif


#if defined(CAN_WRAP_alloc_pages_exact_nid) && CAN_WRAP_alloc_pages_exact_nid
    CFG_MEMORY_ALLOCATOR(alloc_pages_exact_nid)
#endif


#if defined(CAN_WRAP_free_pages) && CAN_WRAP_free_pages
    CFG_MEMORY_DEALLOCATOR(free_pages)
#endif


#if defined(CAN_WRAP_free_memcg_kmem_pages) && CAN_WRAP_free_memcg_kmem_pages
    CFG_MEMORY_DEALLOCATOR(free_memcg_kmem_pages)
#endif


#if defined(CAN_WRAP___alloc_skb) && CAN_WRAP___alloc_skb
    CFG_MEMORY_DEALLOCATOR(__alloc_skb)
#endif

#endif /* CLIENT_CFG_ALLOCATORS_H_ */
