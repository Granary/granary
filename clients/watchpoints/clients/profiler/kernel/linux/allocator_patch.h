/*
 * allocator_patch.h
 *
 *  Created on: 2013-08-05
 *      Author: akshayk
 */

#ifndef _KERNEL_ALLOCATOR_PATCH_H
#define _KERNEL_ALLOCATOR_PATCH_H

#include "clients/watchpoints/instrument.h"
#include "clients/watchpoints/clients/leak_detector/descriptor.h"

#if defined(CAN_WRAP___kmalloc) && CAN_WRAP___kmalloc
#   define APP_PATCH_FOR___kmalloc
    PATCH_WRAPPER(__kmalloc, (void *), (size_t size, gfp_t gfp), {
        client::wp::leak_detector_thread_state *thread_state = get_thread_private_info();
        void *ret(__kmalloc(size, gfp));

        void *ret_address = __builtin_return_address(0);

        if(thread_state && (thread_state->local_state == MODULE_RUNNING)
                && (!is_wrapper_address(unsafe_cast<app_pc>(ret_address)))){
            if(is_valid_address(ret)) {
                add_watchpoint(ret, ret, size);
            }

          //  granary::printf("kmalloc : %llx\n", ret);
        }
        return ret;
    })
#endif

#if defined(CAN_WRAP_kfree) && CAN_WRAP_kfree
#   define APP_PATCH_FOR_kfree
    PATCH_WRAPPER_VOID(kfree, (const void *ptr), {
        if(is_watched_address(ptr)) {
           // granary::printf("kfree : %llx\n", ptr);
           // free_descriptor_of(ptr);
            set_descriptor_leak(ptr);
            ptr = unwatched_address(ptr);
        }
        return kfree(ptr);
    })
#endif

#if defined(CAN_WRAP_kmem_cache_alloc) && CAN_WRAP_kmem_cache_alloc
#   define PATCH_WRAPPER_FOR_kmem_cache_alloc
    PATCH_WRAPPER(kmem_cache_alloc, (void *), (struct kmem_cache *cache, gfp_t gfp), {
        client::wp::leak_detector_thread_state *thread_state = get_thread_private_info();
        void *ret_address = __builtin_return_address(0);
        void *ptr(kmem_cache_alloc(cache, gfp));

        if(!ptr) {
            return ptr;
        }

        if(thread_state && (thread_state->local_state == MODULE_RUNNING)
                && (!is_wrapper_address(unsafe_cast<app_pc>(ret_address)))){

            PRE_OUT_WRAP(cache);

            // Add watchpoint before constructor so that internal pointers
            // maintain their invariants (e.g. list_head structures).
            memset(ptr, 0, cache->object_size);
            add_watchpoint(ptr, ptr, cache->object_size);
            if(is_valid_address(cache->ctor)) {
                cache->ctor(ptr);
            }

          //  granary::printf("kmem_cache_alloc : %llx, %llx\n", ptr, kernel_get_current());
        }

        return ptr;
    })
#endif

#if defined(CAN_WRAP_kmem_cache_free) && CAN_WRAP_kmem_cache_free
#   define PATCH_WRAPPER_FOR_kmem_cache_free
    PATCH_WRAPPER_VOID(kmem_cache_free, (struct kmem_cache *cache, void *ptr), {
        if(ptr && is_watched_address(ptr)) {
           // granary::printf("kmem_cache_free : %llx\n", ptr);
            //free_descriptor_of(ptr);
            set_descriptor_leak(ptr);
            ptr = unwatched_address(ptr);
            PRE_OUT_WRAP(cache);
        }

        kmem_cache_free(cache, ptr);
    })
#endif

#endif /* _KERNEL_ALLOCATOR_PATCH_H */
