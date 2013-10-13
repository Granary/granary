/*
 * patch_wrapper.h
 *
 *  Created on: 2013-10-07
 *      Author: akshayk
 */

#ifndef PATCH_WRAPPER_H_
#define PATCH_WRAPPER_H_

#ifndef  PATCH_WRAPPER_FOR___kmalloc
#   define PATCH_WRAPPER_FOR___kmalloc
    PATCH_WRAPPER(__kmalloc, (void *), (size_t size, gfp_t gfp), {
        void *addr = __builtin_return_address(0);
        void *ret(__kmalloc(size, gfp));
        if(is_valid_address(ret)) {
            add_watchpoint(ret/*, ret, size*/);
            granary::printf("patch wrapper kmalloc\n");
        }
        return ret;
    })
#endif

#ifndef  PATCH_WRAPPER_FOR_kfree
#   define PATCH_WRAPPER_FOR_kfree
    PATCH_WRAPPER_VOID(kfree, (const void *ptr), {
            return kfree(unwatched_address_check(ptr));
    })
#endif


#ifndef  PATCH_WRAPPER_FOR_kmem_cache_alloc
#   define PATCH_WRAPPER_FOR_kmem_cache_alloc
    PATCH_WRAPPER(kmem_cache_alloc, (void *), (struct kmem_cache *cache, gfp_t gfp), {
        cache = unwatched_address_check(cache);

        void *addr = __builtin_return_address(0);

        void *ptr(kmem_cache_alloc(cache, gfp));
        if(!ptr) {
            return ptr;
        }
#if 1
        // Add watchpoint before constructor so that internal pointers
        // maintain their invariants (e.g. list_head structures).
        {
            {
                memset(ptr, 0, cache->object_size);
                add_watchpoint(ptr/*, ptr, cache->size*/);
                if(is_valid_address(cache->ctor)) {
                    cache->ctor(ptr);
                }
            }
        }
#endif
        return ptr;
    })
#endif

#ifndef  PATCH_WRAPPER_FOR_kmem_cache_free
#   define PATCH_WRAPPER_FOR_kmem_cache_free
PATCH_WRAPPER(kmem_cache_free, (void), (struct kmem_cache *cache, void *ptr), {
    kmem_cache_free(cache, unwatched_address_check(ptr));
})
#endif


#endif /* PATCH_WRAPPER_H_ */
