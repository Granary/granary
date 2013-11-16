/*
 * patch_wrapper.h
 *
 *  Created on: 2013-10-07
 *      Author: akshayk
 */

#ifndef EVERYTHING_WATCHED_PATCH_WRAPPER_H_
#define EVERYTHING_WATCHED_PATCH_WRAPPER_H_


#if CONFIG_FEATURE_INSTRUMENT_HOST && !CONFIG_FEATURE_WRAPPERS


#ifndef PATCH_WRAPPER_FOR___kmalloc
#   define PATCH_WRAPPER_FOR___kmalloc
    PATCH_WRAPPER(__kmalloc, (void *), (size_t size, gfp_t gfp), {
        void *ret(__kmalloc(size, gfp));
        if(is_valid_address(ret)) {
            add_watchpoint(ret);
        }
        return ret;
    })
#endif


#ifndef PATCH_WRAPPER_FOR_kfree
#   define PATCH_WRAPPER_FOR_kfree
    PATCH_WRAPPER_VOID(kfree, (const void *ptr), {
        kfree(unwatched_address_check(ptr));
    })
#endif


#ifndef PATCH_WRAPPER_FOR_kmem_cache_alloc
#   define PATCH_WRAPPER_FOR_kmem_cache_alloc
    PATCH_WRAPPER(kmem_cache_alloc, (void *), (struct kmem_cache *cache, gfp_t gfp), {
        cache = unwatched_address_check(cache);
        void *ptr(kmem_cache_alloc(cache, gfp));
        if(!is_valid_address(ptr)) {
            return ptr;
        }

        // Add watchpoint before invoking the object's constructor so that
        // internal pointers will maintain their invariants (e.g. list_head
        // structures will point to the watched version of a data structure
        // instead of to the unwatched version).
        memset(ptr, 0, cache->object_size);
        add_watchpoint(ptr);
        if(is_valid_address(cache->ctor)) {
            WRAP_FUNCTION(cache->ctor);
            cache->ctor(ptr);
        }

        return ptr;
    })
#endif


#ifndef PATCH_WRAPPER_FOR_kmem_cache_free
#   define PATCH_WRAPPER_FOR_kmem_cache_free
    PATCH_WRAPPER_VOID(kmem_cache_free, (struct kmem_cache *cache, void *ptr), {
        kmem_cache_free(
            unwatched_address_check(cache),
            unwatched_address_check(ptr));
    })
#endif


#if 0
#ifndef PATCH_WRAPPER_FOR_autoremove_wake_function
#   define PATCH_WRAPPER_FOR_autoremove_wake_function
    PATCH_WRAPPER(autoremove_wake_function,
        (int),
        ( wait_queue_t * wait , unsigned mode , int sync , void * key ),
    {
        if(is_watched_address(wait)
        || is_watched_address(key)) {
            granary_break_on_curiosity();
        }

        return autoremove_wake_function(wait, mode, sync, key);
    })
#endif
#endif

#endif /* CONFIG_FEATURE_INSTRUMENT_HOST && !CONFIG_FEATURE_WRAPPERS */
#endif /* EVERYTHING_WATCHED_PATCH_WRAPPER_H_ */
