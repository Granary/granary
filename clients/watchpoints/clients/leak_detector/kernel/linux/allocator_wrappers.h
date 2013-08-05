/*
 * allocator_wrappers.h
 *
 *  Created on: 2013-06-25
 *      Author: akshayk
 */

#ifndef _KERNEL_ALLOCATOR_WRAPPERS_H_
#define _KERNEL_ALLOCATOR_WRAPPERS_H_

#include "clients/watchpoints/instrument.h"
#include "clients/watchpoints/clients/leak_detector/descriptor.h"

using namespace client::wp;


#ifdef ENABLE_DEBUG
    PATCH_WRAPPER_VOID(do_page_fault, (struct pt_regs *regs, unsigned long error_code), {
        kernel_handle_fault(regs, error_code);
        do_page_fault(regs, error_code);

    })
#endif



#if defined(CAN_WRAP___kmalloc) && CAN_WRAP___kmalloc
#   define APP_WRAPPER_FOR___kmalloc
    FUNCTION_WRAPPER(APP, __kmalloc, (void *), (size_t size, gfp_t gfp), {
        void *ret(__kmalloc(size, gfp));
        if(is_valid_address(ret)) {
            add_watchpoint(ret, ret, size);
        }
        granary::printf("__kmalloc_wrapper : %llx, %llx\n", ret, kernel_get_current());
        return ret;
    })
#endif



#if defined(CAN_WRAP___kmalloc_track_caller) && CAN_WRAP___kmalloc_track_caller
#   define APP_WRAPPER_FOR___kmalloc_track_caller
    FUNCTION_WRAPPER(APP, __kmalloc_track_caller, (void *), (size_t size, gfp_t gfp, unsigned long caller), {
        void *ret(__kmalloc_track_caller(size, gfp, caller));
        if(is_valid_address(ret)) {
            add_watchpoint(ret, ret, size);
        }
        granary::printf("__kmalloc_track_caller : %llx\n", ret);
        return ret;
    })
#endif


#if defined(CAN_WRAP___kmalloc_node) && CAN_WRAP___kmalloc_node
#   define APP_WRAPPER_FOR___kmalloc_node
    FUNCTION_WRAPPER(APP, __kmalloc_node, (void *), (size_t size, gfp_t gfp, int node), {
        void *ret(__kmalloc_node(size, gfp, node));
        if(is_valid_address(ret)) {
            add_watchpoint(ret, ret, size);
        }
        granary::printf("%s : %llx\n", __FUNCTION__, ret);
        return ret;
    })
#endif


#if defined(CAN_WRAP___kmalloc_node_track_caller) && CAN_WRAP___kmalloc_node_track_caller
#   define APP_WRAPPER_FOR___kmalloc_node_track_caller
    FUNCTION_WRAPPER(APP, __kmalloc_node_track_caller, (void *), (size_t size, gfp_t gfp, int node, unsigned long caller), {
        void *ret(__kmalloc_node_track_caller(size, gfp, node, caller));
        if(is_valid_address(ret)) {
            add_watchpoint(ret, ret, size);
        }
        granary::printf("%s : %llx\n", __FUNCTION__, ret);
        return ret;
    })
#endif


#if defined(CAN_WRAP___krealloc) && CAN_WRAP___krealloc
#   ifndef APP_WRAPPER_FOR___krealloc
#       define APP_WRAPPER_FOR___krealloc
    FUNCTION_WRAPPER(APP, __krealloc, (void *), (const void * ptr, size_t size, gfp_t gfp), {
        if(is_watched_address(ptr)) {
            free_descriptor_of(ptr);
            ptr = unwatched_address(ptr);
        }

        void *ret(__krealloc(ptr, size, gfp));
        add_watchpoint(ret, ret, size);
        granary::printf("%s : %llx\n", __FUNCTION__, ret);
        return ret;
    })
#   endif
#endif


#if defined(CAN_WRAP_krealloc) && CAN_WRAP_krealloc
#   ifndef APP_WRAPPER_FOR_krealloc
#       define APP_WRAPPER_FOR_krealloc
    FUNCTION_WRAPPER(APP, krealloc, (void *), (const void * ptr, size_t size, gfp_t gfp), {
        if(is_watched_address(ptr)) {
            free_descriptor_of(ptr);
            ptr = unwatched_address(ptr);
        }

        void *ret(krealloc(ptr, size, gfp));
        add_watchpoint(ret, ret, size);
        granary::printf("%s : %llx\n", __FUNCTION__, ret);
        return ret;
    })
#   endif
#endif


#if defined(CAN_WRAP_kfree) && CAN_WRAP_kfree
#   define APP_WRAPPER_FOR_kfree
    FUNCTION_WRAPPER_VOID(APP, kfree, (const void *ptr), {
        if(is_watched_address(ptr)) {
            free_descriptor_of(ptr);
            ptr = unwatched_address(ptr);
        }
        granary::printf("kfree : %llx\n", ptr);
        return kfree(ptr);
    })
#endif


#if defined(CAN_WRAP_kzfree) && CAN_WRAP_kzfree
#   define APP_WRAPPER_FOR_kzfree
    FUNCTION_WRAPPER_VOID(APP, kzfree, (const void *ptr), {
        if(is_watched_address(ptr)) {
            free_descriptor_of(ptr);
            ptr = unwatched_address(ptr);
        }
        granary::printf("%s : %llx\n", __FUNCTION__, ptr);
        return kzfree(ptr);
    })
#endif


#if defined(CAN_WRAP_kmem_cache_alloc) && CAN_WRAP_kmem_cache_alloc
#   define APP_WRAPPER_FOR_kmem_cache_alloc
    FUNCTION_WRAPPER(APP, kmem_cache_alloc, (void *), (struct kmem_cache *cache, gfp_t gfp), {
        PRE_OUT_WRAP(cache);
        void *ptr(kmem_cache_alloc(cache, gfp));
        if(!ptr) {
            return ptr;
        }

        // Add watchpoint before constructor so that internal pointers
        // maintain their invariants (e.g. list_head structures).
        memset(ptr, 0, cache->object_size);
        add_watchpoint(ptr, ptr, cache->object_size);
        if(is_valid_address(cache->ctor)) {
            cache->ctor(ptr);
        }
        granary::printf("kmem_cache_alloc : %llx, %llx\n", ptr, kernel_get_current());
        return ptr;
    })
#endif


#if defined(CAN_WRAP_kmem_cache_alloc_trace) && CAN_WRAP_kmem_cache_alloc_trace
#   define APP_WRAPPER_FOR_kmem_cache_alloc_trace
    FUNCTION_WRAPPER(APP, kmem_cache_alloc_trace, (void *), (struct kmem_cache *cache, gfp_t gfp, size_t size), {
        if(cache) {
            if(is_watched_address(cache)) {
                cache = unwatched_address(cache);
            }
            WRAP_FUNCTION(cache->ctor);
        }

        void *ptr(kmem_cache_alloc_trace(cache, gfp, size));
        if(!ptr) {
            return ptr;
        }

        // Add watchpoint before constructor so that internal pointers
        // maintain their invariants (e.g. list_head structures).
        memset(ptr, 0, size);
        add_watchpoint(ptr, ptr, size);
        if(is_valid_address(cache->ctor)) {
            cache->ctor(ptr);
        }
        granary::printf("%s : %llx\n", __FUNCTION__, ptr);
        return ptr;
    })
#endif


#if defined(CAN_WRAP_kmem_cache_alloc_node) && CAN_WRAP_kmem_cache_alloc_node
#   define APP_WRAPPER_FOR_kmem_cache_alloc_node
    FUNCTION_WRAPPER(APP, kmem_cache_alloc_node, (void *), (struct kmem_cache *cache, gfp_t gfp, int node), {
        PRE_OUT_WRAP(cache);

        void *ptr(kmem_cache_alloc_node(cache, gfp, node));
        if(!ptr) {
            return ptr;
        }

        // Add watchpoint before constructor so that internal pointers
        // maintain their invariants (e.g. list_head structures).
        memset(ptr, 0, cache->object_size);
        add_watchpoint(ptr, ptr, cache->object_size);
        if(is_valid_address(cache->ctor)) {
            cache->ctor(ptr);
        }
        granary::printf("%s : %llx\n", __FUNCTION__, ptr);
        return ptr;
    })
#endif


#if defined(CAN_WRAP_kmem_cache_free) && CAN_WRAP_kmem_cache_free
#   define APP_WRAPPER_FOR_kmem_cache_free
    FUNCTION_WRAPPER(APP, kmem_cache_free, (void), (struct kmem_cache *cache, void *ptr), {
        granary::printf("kmem_cache_free : %llx\n", ptr);

        if(ptr && is_watched_address(ptr)) {
            free_descriptor_of(ptr);
            ptr = unwatched_address(ptr);
        }

        PRE_OUT_WRAP(cache);
        kmem_cache_free(cache, ptr);
    })
#endif


#if defined(CAN_WRAP___get_free_pages) && CAN_WRAP___get_free_pages
#   ifndef APP_WRAPPER_FOR___get_free_pages
#       define APP_WRAPPER_FOR___get_free_pages
    FUNCTION_WRAPPER(APP, __get_free_pages, (unsigned long), ( gfp_t gfp_mask , unsigned int order ), {
        unsigned long ret(__get_free_pages(gfp_mask, order));
        if(is_valid_address(ret)) {
            add_watchpoint(ret, reinterpret_cast<void *>(ret), (1 << order) * PAGE_SIZE);
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
            add_watchpoint(ret, reinterpret_cast<void *>(ret), PAGE_SIZE);
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
            add_watchpoint(ret, ret, size);
        }
        return ret;
    })
#   endif
#endif


#if defined(CAN_WRAP_free_pages_exact) && CAN_WRAP_free_pages_exact
#   ifndef APP_WRAPPER_FOR_free_pages_exact
#       define APP_WRAPPER_FOR_free_pages_exact
    FUNCTION_WRAPPER_VOID(APP, free_pages_exact, ( void *virt, size_t size ), {
        if(is_watched_address(virt)) {
            free_descriptor_of(virt);
            virt = unwatched_address(virt);
        }
        free_pages_exact(virt, size);
    })
#   endif
#endif


#if defined(CAN_WRAP_alloc_pages_exact_nid) && CAN_WRAP_alloc_pages_exact_nid
#   ifndef APP_WRAPPER_FOR_alloc_pages_exact_nid
#       define APP_WRAPPER_FOR_alloc_pages_exact_nid
    FUNCTION_WRAPPER(APP, alloc_pages_exact_nid, (void *), ( int node, size_t size, gfp_t gfp_mask ), {
        void *ret(alloc_pages_exact_nid(node, size, gfp_mask));
        if(is_valid_address(ret)) {
            add_watchpoint(ret, ret, size);
        }
        return ret;
    })
#   endif
#endif


#if defined(CAN_WRAP_free_pages) && CAN_WRAP_free_pages
#   ifndef APP_WRAPPER_FOR_free_pages
#       define APP_WRAPPER_FOR_free_pages
    FUNCTION_WRAPPER_VOID(APP, free_pages, ( unsigned long addr, unsigned int order ), {
        if(is_watched_address(addr)) {
            free_descriptor_of(addr);
            addr = unwatched_address(addr);
        }
        free_pages(addr, order);
    })
#   endif
#endif


#if defined(CAN_WRAP_free_memcg_kmem_pages) && CAN_WRAP_free_memcg_kmem_pages
#   ifndef APP_WRAPPER_FOR_free_memcg_kmem_pages
#       define APP_WRAPPER_FOR_free_memcg_kmem_pages
    FUNCTION_WRAPPER_VOID(APP, free_memcg_kmem_pages, ( unsigned long addr, unsigned int order ), {
        if(is_watched_address(addr)) {
            free_descriptor_of(addr);
            addr = unwatched_address(addr);
        }
        free_memcg_kmem_pages(addr, order);
    })
#   endif
#endif



#endif /* ALLOCATOR_WRAPPERS_H_ */
