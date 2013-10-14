/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * watched_wrappers.h
 *
 *  Created on: 2013-05-11
 *      Author: Peter Goodman
 */

#ifndef EVERYTHING_WATCHED_WRAPPERS_H_
#define EVERYTHING_WATCHED_WRAPPERS_H_


#include "clients/watchpoints/clients/everything_watched/instrument.h"


using namespace client::wp;


/// Specify the descriptor type to the generic watchpoint framework.
namespace client { namespace wp {
    template <typename>
    struct descriptor_type {
        typedef void type;
        enum {
            ALLOC_AND_INIT = false,
            REINIT_WATCHED_POINTERS = false
        };
    };
}}


#if defined(CAN_WRAP___kmalloc) && CAN_WRAP___kmalloc
#   define APP_WRAPPER_FOR___kmalloc
    FUNCTION_WRAPPER(APP, __kmalloc, (void *), (size_t size, gfp_t gfp), {
        void *ret(__kmalloc(size, gfp));
        if(is_valid_address(ret)) {
            add_watchpoint(ret);
        }
        return ret;
    })
#endif


#if defined(CAN_WRAP___kmalloc_track_caller) && CAN_WRAP___kmalloc_track_caller
#   define APP_WRAPPER_FOR___kmalloc_track_caller
    FUNCTION_WRAPPER(APP, __kmalloc_track_caller, (void *), (size_t size, gfp_t gfp, unsigned long caller), {
        void *ret(__kmalloc_track_caller(size, gfp, caller));
        if(is_valid_address(ret)) {
            add_watchpoint(ret);
        }
        return ret;
    })
#endif


#if defined(CAN_WRAP___kmalloc_node) && CAN_WRAP___kmalloc_node
#   define APP_WRAPPER_FOR___kmalloc_node
    FUNCTION_WRAPPER(APP, __kmalloc_node, (void *), (size_t size, gfp_t gfp, int node), {
        void *ret(__kmalloc_node(size, gfp, node));
        if(is_valid_address(ret)) {
            add_watchpoint(ret);
        }
        return ret;
    })
#endif


#if defined(CAN_WRAP___kmalloc_node_track_caller) && CAN_WRAP___kmalloc_node_track_caller
#   define APP_WRAPPER_FOR___kmalloc_node_track_caller
    FUNCTION_WRAPPER(APP, __kmalloc_node_track_caller, (void *), (size_t size, gfp_t gfp, int node, unsigned long caller), {
        void *ret(__kmalloc_node_track_caller(size, gfp, node, caller));
        if(is_valid_address(ret)) {
            add_watchpoint(ret);
        }
        return ret;
    })
#endif


#if defined(CAN_WRAP___krealloc) && CAN_WRAP___krealloc
#   ifndef APP_WRAPPER_FOR___krealloc
#       define APP_WRAPPER_FOR___krealloc
    FUNCTION_WRAPPER(APP, __krealloc, (void *), (const void * ptr, size_t size, gfp_t gfp), {
        void *ret(__krealloc(unwatched_address_check(ptr), size, gfp));
        if(is_valid_address(ret)) {
            add_watchpoint(ret);
        }
        return ret;
    })
#   endif
#endif


#if defined(CAN_WRAP_krealloc) && CAN_WRAP_krealloc
#   ifndef APP_WRAPPER_FOR_krealloc
#       define APP_WRAPPER_FOR_krealloc
    FUNCTION_WRAPPER(APP, krealloc, (void *), (const void * ptr, size_t size, gfp_t gfp), {
        void *ret(krealloc(unwatched_address_check(ptr), size, gfp));
        add_watchpoint(ret);
        return ret;
    })
#   endif
#endif


#if defined(CAN_WRAP_kfree) && CAN_WRAP_kfree
#   define APP_WRAPPER_FOR_kfree
    FUNCTION_WRAPPER_VOID(APP, kfree, (const void *ptr), {
        kfree(unwatched_address_check(ptr));
    })
#endif


#if defined(CAN_WRAP_kzfree) && CAN_WRAP_kzfree
#   define APP_WRAPPER_FOR_kzfree
    FUNCTION_WRAPPER_VOID(APP, kzfree, (const void *ptr), {
        kzfree(unwatched_address_check(ptr));
    })
#endif


#if defined(CAN_WRAP_kmem_cache_alloc) && CAN_WRAP_kmem_cache_alloc
#   define APP_WRAPPER_FOR_kmem_cache_alloc
    FUNCTION_WRAPPER(APP, kmem_cache_alloc, (void *), (struct kmem_cache *cache, gfp_t gfp), {
        cache = unwatched_address_check(cache);
        PRE_OUT_WRAP(cache);

        void *ptr(kmem_cache_alloc(cache, gfp));
        if(!ptr) {
            return ptr;
        }

        // Add watchpoint before constructor so that internal pointers
        // maintain their invariants (e.g. list_head structures).
        memset(ptr, 0, cache->object_size);
        add_watchpoint(ptr);
        if(is_valid_address(cache->ctor)) {
            cache->ctor(ptr);
        }

        return ptr;
    })
#endif


#if defined(CAN_WRAP_kmem_cache_alloc_trace) && CAN_WRAP_kmem_cache_alloc_trace
#   define APP_WRAPPER_FOR_kmem_cache_alloc_trace
    FUNCTION_WRAPPER(APP, kmem_cache_alloc_trace, (void *), (struct kmem_cache *cache, gfp_t gfp, size_t size), {
        cache = unwatched_address_check(cache);
        PRE_OUT_WRAP(cache);

        void *ptr(kmem_cache_alloc_trace(cache, gfp, size));
        if(!ptr) {
            return ptr;
        }

        // Add watchpoint before constructor so that internal pointers
        // maintain their invariants (e.g. list_head structures).
        memset(ptr, 0, size);
        add_watchpoint(ptr);
        if(is_valid_address(cache->ctor)) {
            cache->ctor(ptr);
        }

        return ptr;
    })
#endif


#if defined(CAN_WRAP_kmem_cache_alloc_node) && CAN_WRAP_kmem_cache_alloc_node
#   define APP_WRAPPER_FOR_kmem_cache_alloc_node
    FUNCTION_WRAPPER(APP, kmem_cache_alloc_node, (void *), (struct kmem_cache *cache, gfp_t gfp, int node), {
        cache = unwatched_address_check(cache);
        PRE_OUT_WRAP(cache);

        void *ptr(kmem_cache_alloc_node(cache, gfp, node));
        if(!ptr) {
            return ptr;
        }

        // Add watchpoint before constructor so that internal pointers
        // maintain their invariants (e.g. list_head structures).
        memset(ptr, 0, cache->object_size);
        add_watchpoint(ptr);
        if(is_valid_address(cache->ctor)) {
            cache->ctor(ptr);
        }

        return ptr;
    })
#endif


#if defined(CAN_WRAP_kmem_cache_free) && CAN_WRAP_kmem_cache_free
#   define APP_WRAPPER_FOR_kmem_cache_free
    FUNCTION_WRAPPER(APP, kmem_cache_free, (void), (struct kmem_cache *cache, void *ptr), {
        kmem_cache_free(cache, unwatched_address_check(ptr));
    })
#endif


#if defined(CAN_WRAP___get_free_pages) && CAN_WRAP___get_free_pages
#   ifndef APP_WRAPPER_FOR___get_free_pages
#       define APP_WRAPPER_FOR___get_free_pages
    FUNCTION_WRAPPER(APP, __get_free_pages, (unsigned long), ( gfp_t gfp_mask , unsigned int order ), {
        unsigned long ret(__get_free_pages(gfp_mask, order));
        if(is_valid_address(ret)) {
            add_watchpoint(ret);
        }
        return ret;
    })
#   endif
#endif


#if defined(CAN_WRAP_get_zeroed_page) && CAN_WRAP_get_zeroed_page
#   ifndef APP_WRAPPER_FOR_get_zeroed_page
#       define APP_WRAPPER_FOR_get_zeroed_page
    FUNCTION_WRAPPER(APP, get_zeroed_page, (unsigned long), ( gfp_t gfp_mask ), {
        unsigned long ret(get_zeroed_page(gfp_mask));
        if(is_valid_address(ret)) {
            add_watchpoint(ret);
        }
        return ret;
    })
#   endif
#endif


#if defined(CAN_WRAP_alloc_pages_exact) && CAN_WRAP_alloc_pages_exact
#   ifndef APP_WRAPPER_FOR_alloc_pages_exact
#       define APP_WRAPPER_FOR_alloc_pages_exact
    FUNCTION_WRAPPER(APP, alloc_pages_exact, (void *), ( size_t size, gfp_t gfp_mask ), {
        void *ret(alloc_pages_exact(size, gfp_mask));
        if(is_valid_address(ret)) {
            add_watchpoint(ret);
        }
        return ret;
    })
#   endif
#endif


#if defined(CAN_WRAP_free_pages_exact) && CAN_WRAP_free_pages_exact
#   ifndef APP_WRAPPER_FOR_free_pages_exact
#       define APP_WRAPPER_FOR_free_pages_exact
    FUNCTION_WRAPPER_VOID(APP, free_pages_exact, ( void *virt, size_t size ), {
        free_pages_exact(unwatched_address_check(virt), size);
    })
#   endif
#endif


#if defined(CAN_WRAP_alloc_pages_exact_nid) && CAN_WRAP_alloc_pages_exact_nid
#   ifndef APP_WRAPPER_FOR_alloc_pages_exact_nid
#       define APP_WRAPPER_FOR_alloc_pages_exact_nid
    FUNCTION_WRAPPER(APP, alloc_pages_exact_nid, (void *), ( int node, size_t size, gfp_t gfp_mask ), {
        void *ret(alloc_pages_exact_nid(node, size, gfp_mask));
        if(is_valid_address(ret)) {
            add_watchpoint(ret);
        }
        return ret;
    })
#   endif
#endif


#if defined(CAN_WRAP_free_pages) && CAN_WRAP_free_pages
#   ifndef APP_WRAPPER_FOR_free_pages
#       define APP_WRAPPER_FOR_free_pages
    FUNCTION_WRAPPER_VOID(APP, free_pages, ( unsigned long addr, unsigned int order ), {
        free_pages(unwatched_address_check(addr), order);
    })
#   endif
#endif


#if defined(CAN_WRAP_free_memcg_kmem_pages) && CAN_WRAP_free_memcg_kmem_pages
#   ifndef APP_WRAPPER_FOR_free_memcg_kmem_pages
#       define APP_WRAPPER_FOR_free_memcg_kmem_pages
    FUNCTION_WRAPPER_VOID(APP, free_memcg_kmem_pages, ( unsigned long addr, unsigned int order ), {
        free_memcg_kmem_pages(unwatched_address_check(addr), order);
    })
#   endif
#endif


#endif /* EVERYTHING_WATCHED_WRAPPERS_H_ */
