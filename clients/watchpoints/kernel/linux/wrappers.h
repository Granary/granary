/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * wrappers.h
 *
 *  Created on: 2013-05-15
 *      Author: Peter Goodman
 */

#ifndef WATCHPOINT_WRAPPERS_H_
#define WATCHPOINT_WRAPPERS_H_

#include "clients/watchpoints/instrument.h"

using namespace client::wp;


/// Bounds checking watchpoint policy wrappers.
#ifdef CLIENT_WATCHPOINT_BOUND
#   include "clients/watchpoints/clients/bounds_checker/kernel/linux/wrappers.h"
#endif


/// Leak detector watchpoint policy wrappers.
#ifdef CLIENT_WATCHPOINT_LEAK
#   include "clients/watchpoints/clients/leak_detector/kernel/wrappers.h"
#endif


/// Leak detector watchpoint policy wrappers.
#ifdef CLIENT_WATCHPOINT_PROFILE
#   include "clients/watchpoints/clients/profiler/kernel/wrappers.h"
#endif


/// Null policy that taints all addresses.
#ifdef CLIENT_WATCHPOINT_WATCHED
#   include "clients/watchpoints/clients/everything_watched/kernel/linux/wrappers.h"
#   include "clients/watchpoints/clients/everything_watched/kernel/linux/patch_wrappers.h"
#endif


/// Stats tracking policy that taints addresses.
#ifdef CLIENT_WATCHPOINT_STATS
#   include "clients/watchpoints/clients/everything_watched/kernel/linux/wrappers.h"
#endif


/// Shadow memory watchpoints tool.
#ifdef CLIENT_SHADOW_MEMORY
#   include "clients/watchpoints/clients/shadow_memory/kernel/linux/wrappers.h"
#endif


/// RCU Debugging watchpoints tool.
#ifdef CLIENT_RCUDBG
#   include "clients/watchpoints/clients/rcudbg/kernel/linux/wrappers.h"
#endif


#ifndef APP_WRAPPER_FOR_pointer
#   define APP_WRAPPER_FOR_pointer
    POINTER_WRAPPER({
        PRE_OUT {
            if(!is_valid_address(arg)) {
                return;
            }
            PRE_OUT_WRAP(*unwatched_address_check(arg));
        }
        PRE_IN {
            if(!is_valid_address(arg)) {
                return;
            }
            PRE_IN_WRAP(*unwatched_address_check(arg));
        }
        INHERIT_POST_INOUT
        INHERIT_RETURN_INOUT
    })
#endif


#define WP_BASIC_POINTER_WRAPPER(base_type) \
    TYPE_WRAPPER(base_type *, { \
        NO_PRE \
        NO_POST \
        NO_RETURN \
    })


#ifndef APP_WRAPPER_FOR_void_pointer
#   define APP_WRAPPER_FOR_void_pointer
    WP_BASIC_POINTER_WRAPPER(void)
#endif


#ifndef APP_WRAPPER_FOR_int8_t_pointer
#   define APP_WRAPPER_FOR_int8_t_pointer
    static_assert(sizeof(char) == sizeof(int8_t), "Type size mismatch.");
    WP_BASIC_POINTER_WRAPPER(char)
#endif


#ifndef APP_WRAPPER_FOR_uint8_t_pointer
#   define APP_WRAPPER_FOR_uint8_t_pointer
    static_assert(sizeof(char) == sizeof(int8_t), "Type size mismatch.");
    WP_BASIC_POINTER_WRAPPER(unsigned char)
#endif


#ifndef APP_WRAPPER_FOR_int16_t_pointer
#   define APP_WRAPPER_FOR_int16_t_pointer
    static_assert(sizeof(short) == sizeof(int16_t), "Type size mismatch.");
    WP_BASIC_POINTER_WRAPPER(short)
#endif


#ifndef APP_WRAPPER_FOR_uint16_t_pointer
#   define APP_WRAPPER_FOR_uint16_t_pointer
    static_assert(sizeof(short) == sizeof(int16_t), "Type size mismatch.");
    WP_BASIC_POINTER_WRAPPER(unsigned short)
#endif


#ifndef APP_WRAPPER_FOR_int32_t_pointer
#   define APP_WRAPPER_FOR_int32_t_pointer
    static_assert(sizeof(int) == sizeof(int32_t), "Type size mismatch.");
    WP_BASIC_POINTER_WRAPPER(int)
#endif


#ifndef APP_WRAPPER_FOR_uint32_t_pointer
#   define APP_WRAPPER_FOR_uint32_t_pointer
    static_assert(sizeof(int) == sizeof(int32_t), "Type size mismatch.");
    WP_BASIC_POINTER_WRAPPER(unsigned)
#endif


#ifndef APP_WRAPPER_FOR_int64_t_pointer
#   define APP_WRAPPER_FOR_int64_t_pointer
    static_assert(sizeof(long) == sizeof(int64_t), "Type size mismatch.");
    WP_BASIC_POINTER_WRAPPER(long)
#endif


#ifndef APP_WRAPPER_FOR_uint64_t_pointer
#   define APP_WRAPPER_FOR_uint64_t_pointer
    static_assert(sizeof(long) == sizeof(int64_t), "Type size mismatch.");
    WP_BASIC_POINTER_WRAPPER(unsigned long)
#endif


#ifndef APP_WRAPPER_FOR_float_pointer
#   define APP_WRAPPER_FOR_float_pointer
    WP_BASIC_POINTER_WRAPPER(float)
#endif


#ifndef APP_WRAPPER_FOR_double_pointer
#   define APP_WRAPPER_FOR_double_pointer
    WP_BASIC_POINTER_WRAPPER(double)
#endif


#if defined(CAN_WRAP_bit_waitqueue) && CAN_WRAP_bit_waitqueue
#   ifndef APP_WRAPPER_FOR_bit_waitqueue
#       define APP_WRAPPER_FOR_bit_waitqueue
        FUNCTION_WRAPPER(APP, bit_waitqueue, (wait_queue_head_t *), (void *word, int bit), {
            return bit_waitqueue(unwatched_address_check(word), bit);
        })
#   endif
#   ifndef HOST_WRAPPER_FOR_bit_waitqueue
#       define HOST_WRAPPER_FOR_bit_waitqueue
        FUNCTION_WRAPPER(HOST, bit_waitqueue, (wait_queue_head_t *), (void *word, int bit), {
            return bit_waitqueue(unwatched_address_check(word), bit);
        })
#   endif
#endif


#if defined(CAN_WRAP_wake_up_bit) && CAN_WRAP_wake_up_bit
#   ifndef APP_WRAPPER_FOR_wake_up_bit
#       define APP_WRAPPER_FOR_wake_up_bit
        FUNCTION_WRAPPER_VOID(APP, wake_up_bit, (void *word, int bit), {
            wake_up_bit(unwatched_address_check(word), bit);
        })
#   endif
#   ifndef HOST_WRAPPER_FOR_wake_up_bit
#       define HOST_WRAPPER_FOR_wake_up_bit
        FUNCTION_WRAPPER_VOID(HOST, wake_up_bit, (void *word, int bit), {
            wake_up_bit(unwatched_address_check(word), bit);
        })
#   endif
#endif


#if defined(CAN_WRAP___wake_up_bit) && CAN_WRAP___wake_up_bit
#   ifndef APP_WRAPPER_FOR___wake_up_bit
#       define APP_WRAPPER_FOR___wake_up_bit
        FUNCTION_WRAPPER_VOID(APP, __wake_up_bit, (wait_queue_head_t *q, void *word, int bit), {
            __wake_up_bit(
                unwatched_address_check(q),
                unwatched_address_check(word),
                bit);
        })
#   endif
#   ifndef HOST_WRAPPER_FOR___wake_up_bit
#       define HOST_WRAPPER_FOR___wake_up_bit
        FUNCTION_WRAPPER_VOID(HOST, __wake_up_bit, (wait_queue_head_t *q, void *word, int bit), {
            __wake_up_bit(
                unwatched_address_check(q),
                unwatched_address_check(word),
                bit);
        })
#   endif
#endif


#if defined(CAN_WRAP___phys_addr) && CAN_WRAP___phys_addr
#   ifndef APP_WRAPPER_FOR___phys_addr
#       define APP_WRAPPER_FOR___phys_addr
        FUNCTION_WRAPPER(APP, __phys_addr, (uintptr_t), (uintptr_t addr), {
            return __phys_addr(unwatched_address_check(addr));
        })
#   endif
#   ifndef HOST_WRAPPER_FOR___phys_addr
#       define HOST_WRAPPER_FOR___phys_addr
        FUNCTION_WRAPPER(HOST, __phys_addr, (uintptr_t), (uintptr_t addr), {
            return __phys_addr(unwatched_address_check(addr));
        })
#   endif
#endif


#if defined(CAN_WRAP___virt_addr_valid) && CAN_WRAP___virt_addr_valid
#   ifndef APP_WRAPPER_FOR___virt_addr_valid
#       define APP_WRAPPER_FOR___virt_addr_valid
        FUNCTION_WRAPPER(APP, __virt_addr_valid, (bool), (uintptr_t addr), {
            return __virt_addr_valid(unwatched_address_check(addr));
        })
#   endif
#   ifndef HOST_WRAPPER_FOR___virt_addr_valid
#       define HOST_WRAPPER_FOR___virt_addr_valid
        FUNCTION_WRAPPER(HOST, __virt_addr_valid, (bool), (uintptr_t addr), {
            return __virt_addr_valid(unwatched_address_check(addr));
        })
#   endif
#endif


#if 0
#if defined(CAN_WRAP_mutex_lock) && CAN_WRAP_mutex_lock
#   ifndef HOST_WRAPPER_FOR_mutex_lock
#       define HOST_WRAPPER_FOR_mutex_lock
        FUNCTION_WRAPPER_VOID(HOST, mutex_lock, (struct mutex * lock), {
            mutex_lock(unwatched_address_check(lock));
        })
#   endif
#   ifndef APP_WRAPPER_FOR_mutex_lock
#       define APP_WRAPPER_FOR_mutex_lock
        FUNCTION_WRAPPER_VOID(APP, mutex_lock, (struct mutex * lock), {
            mutex_lock(unwatched_address_check(lock));
        })
#   endif
#endif


#if defined(CAN_WRAP_mutex_lock_interruptible) && CAN_WRAP_mutex_lock_interruptible
#   ifndef HOST_WRAPPER_FOR_mutex_lock_interruptible
#       define HOST_WRAPPER_FOR_mutex_lock_interruptible
        FUNCTION_WRAPPER(HOST, mutex_lock_interruptible, (int), (struct mutex * lock), {
            return mutex_lock_interruptible(unwatched_address_check(lock));
        })
#   endif
#   ifndef APP_WRAPPER_FOR_mutex_lock_interruptible
#       define APP_WRAPPER_FOR_mutex_lock_interruptible
        FUNCTION_WRAPPER(APP, mutex_lock_interruptible, (int), (struct mutex * lock), {
            return mutex_lock_interruptible(unwatched_address_check(lock));
        })
#   endif
#endif


#if defined(CAN_WRAP_mutex_lock_killable) && CAN_WRAP_mutex_lock_killable
#   ifndef HOST_WRAPPER_FOR_mutex_lock_killable
#       define HOST_WRAPPER_FOR_mutex_lock_killable
        FUNCTION_WRAPPER(HOST, mutex_lock_killable, (int), (struct mutex * lock), {
            return mutex_lock_killable(unwatched_address_check(lock));
        })
#   endif
#   ifndef APP_WRAPPER_FOR_mutex_lock_killable
#       define APP_WRAPPER_FOR_mutex_lock_killable
        FUNCTION_WRAPPER(APP, mutex_lock_killable, (int), (struct mutex * lock), {
            return mutex_lock_killable(unwatched_address_check(lock));
        })
#   endif
#endif


#if defined(CAN_WRAP_mutex_trylock) && CAN_WRAP_mutex_trylock
#   ifndef HOST_WRAPPER_FOR_mutex_trylock
#       define HOST_WRAPPER_FOR_mutex_trylock
        FUNCTION_WRAPPER(HOST, mutex_trylock, (int), (struct mutex * lock), {
            return mutex_trylock(unwatched_address_check(lock));
        })
#   endif
#   ifndef APP_WRAPPER_FOR_mutex_trylock
#       define APP_WRAPPER_FOR_mutex_trylock
        FUNCTION_WRAPPER(APP, mutex_trylock, (int), (struct mutex * lock), {
            return mutex_trylock(unwatched_address_check(lock));
        })
#   endif
#endif


#if defined(CAN_WRAP_mutex_unlock) && CAN_WRAP_mutex_unlock
#   ifndef HOST_WRAPPER_FOR_mutex_unlock
#       define HOST_WRAPPER_FOR_mutex_unlock
        FUNCTION_WRAPPER_VOID(HOST, mutex_unlock, (struct mutex * lock), {
            mutex_unlock(unwatched_address_check(lock));
        })
#   endif
#   ifndef APP_WRAPPER_FOR_mutex_unlock
#       define APP_WRAPPER_FOR_mutex_unlock
        FUNCTION_WRAPPER_VOID(APP, mutex_unlock, (struct mutex * lock), {
            mutex_unlock(unwatched_address_check(lock));
        })
#   endif
#endif


#define COPY_TO_FROM_USER(context, func) \
    FUNCTION_WRAPPER(context, func, (unsigned long), (void *a, void *b, unsigned len), {\
        return func( \
            unwatched_address_check(a), \
            unwatched_address_check(b), \
            len); \
    })


#if defined(CAN_WRAP_copy_user_enhanced_fast_string) && CAN_WRAP_copy_user_enhanced_fast_string
#   ifndef HOST_WRAPPER_FOR_copy_user_enhanced_fast_string
#       define HOST_WRAPPER_FOR_copy_user_enhanced_fast_string
        COPY_TO_FROM_USER(HOST, copy_user_enhanced_fast_string)
#   endif
#   ifndef APP_WRAPPER_FOR_copy_user_enhanced_fast_string
#       define APP_WRAPPER_FOR_copy_user_enhanced_fast_string
        COPY_TO_FROM_USER(APP, copy_user_enhanced_fast_string)
#   endif
#endif


#if defined(CAN_WRAP_copy_user_generic_string) && CAN_WRAP_copy_user_generic_string
#   ifndef HOST_WRAPPER_FOR_copy_user_generic_string
#       define HOST_WRAPPER_FOR_copy_user_generic_string
        COPY_TO_FROM_USER(HOST, copy_user_generic_string)
#   endif
#   ifndef APP_WRAPPER_FOR_copy_user_generic_string
#       define APP_WRAPPER_FOR_copy_user_generic_string
        COPY_TO_FROM_USER(APP, copy_user_generic_string)
#   endif
#endif


#if defined(CAN_WRAP_copy_user_generic_unrolled) && CAN_WRAP_copy_user_generic_unrolled
#   ifndef HOST_WRAPPER_FOR_copy_user_generic_unrolled
#       define HOST_WRAPPER_FOR_copy_user_generic_unrolled
        COPY_TO_FROM_USER(HOST, copy_user_generic_unrolled)
#   endif
#   ifndef APP_WRAPPER_FOR_copy_user_generic_unrolled
#       define APP_WRAPPER_FOR_copy_user_generic_unrolled
        COPY_TO_FROM_USER(APP, copy_user_generic_unrolled)
#   endif
#endif


#if defined(CAN_WRAP__copy_to_user) && CAN_WRAP__copy_to_user
#   ifndef HOST_WRAPPER_FOR__copy_to_user
#       define HOST_WRAPPER_FOR__copy_to_user
        COPY_TO_FROM_USER(HOST, _copy_to_user)
#   endif
#   ifndef APP_WRAPPER_FOR__copy_to_user
#       define APP_WRAPPER_FOR__copy_to_user
        COPY_TO_FROM_USER(APP, _copy_to_user)
#   endif
#endif


#if defined(CAN_WRAP__copy_from_user) && CAN_WRAP__copy_from_user
#   ifndef HOST_WRAPPER_FOR__copy_from_user
#       define HOST_WRAPPER_FOR__copy_from_user
        COPY_TO_FROM_USER(HOST, _copy_from_user)
#   endif
#   ifndef APP_WRAPPER_FOR__copy_from_user
#       define APP_WRAPPER_FOR__copy_from_user
        COPY_TO_FROM_USER(APP, _copy_from_user)
#   endif
#endif


#if defined(CAN_WRAP_copy_in_user) && CAN_WRAP_copy_in_user
#   ifndef HOST_WRAPPER_FOR_copy_in_user
#       define HOST_WRAPPER_FOR_copy_in_user
        COPY_TO_FROM_USER(HOST, copy_in_user)
#   endif
#   ifndef APP_WRAPPER_FOR_copy_in_user
#       define APP_WRAPPER_FOR_copy_in_user
        COPY_TO_FROM_USER(APP, copy_in_user)
#   endif
#endif


#if defined(CAN_WRAP___copy_user_nocache) && CAN_WRAP___copy_user_nocache
#   ifndef HOST_WRAPPER_FOR___copy_user_nocache
#       define HOST_WRAPPER_FOR___copy_user_nocache
        FUNCTION_WRAPPER(HOST, __copy_user_nocache, (long), (void * dst , const void *src, unsigned size, int zerorest), {
            return __copy_user_nocache(
                unwatched_address_check(dst),
                unwatched_address_check(src),
                size,
                zerorest);
        })
#   endif
#   ifndef APP_WRAPPER_FOR___copy_user_nocache
#       define APP_WRAPPER_FOR___copy_user_nocache
        FUNCTION_WRAPPER(APP, __copy_user_nocache, (long), (void * dst , const void *src, unsigned size, int zerorest), {
            return __copy_user_nocache(
                unwatched_address_check(dst),
                unwatched_address_check(src),
                size,
                zerorest);
        })
#   endif
#endif


#if defined(CAN_WRAP_copy_user_handle_tail) && CAN_WRAP_copy_user_handle_tail
#   ifndef HOST_WRAPPER_FOR_copy_user_handle_tail
#       define HOST_WRAPPER_FOR_copy_user_handle_tail
        FUNCTION_WRAPPER(HOST, copy_user_handle_tail, (unsigned long), ( char * to , char * from , unsigned len , unsigned zerorest ), {
            return copy_user_handle_tail(
                unwatched_address_check(to),
                unwatched_address_check(from),
                len,
                zerorest);
        })
#   endif
#   ifndef APP_WRAPPER_FOR_copy_user_handle_tail
#       define APP_WRAPPER_FOR_copy_user_handle_tail
        FUNCTION_WRAPPER(APP, copy_user_handle_tail, (unsigned long), ( char * to , char * from , unsigned len , unsigned zerorest ), {
            return copy_user_handle_tail(
                unwatched_address_check(to),
                unwatched_address_check(from),
                len,
                zerorest);
        })
#   endif
#endif


#if defined(CAN_WRAP___switch_to)
    FUNCTION_WRAPPER_DETACH(APP, __switch_to)
    FUNCTION_WRAPPER_DETACH(HOST, __switch_to)
#endif


#if defined(CAN_WRAP___schedule)
    FUNCTION_WRAPPER_DETACH(APP, __schedule)
    FUNCTION_WRAPPER_DETACH(HOST, __schedule)
#endif


#if defined(CAN_WRAP__cond_resched)
    FUNCTION_WRAPPER_DETACH(APP, _cond_resched)
    FUNCTION_WRAPPER_DETACH(HOST, _cond_resched)
#endif
#endif

#if 0
#if defined(CAN_WRAP___kmalloc) && CAN_WRAP___kmalloc
#   ifndef HOST_WRAPPER_FOR___kmalloc
#       define HOST_WRAPPER_FOR___kmalloc
        FUNCTION_WRAPPER_DETACH(HOST, __kmalloc)
#   endif
#endif


#if defined(CAN_WRAP_kfree) && CAN_WRAP_kfree
#   ifndef HOST_WRAPPER_FOR_kfree
#       define HOST_WRAPPER_FOR_kfree
        FUNCTION_WRAPPER_DETACH(HOST, kfree)
#   endif
#endif


#if defined(CAN_WRAP_kmem_cache_alloc) && CAN_WRAP_kmem_cache_alloc
#   ifndef HOST_WRAPPER_FOR_kmem_cache_alloc
#       define HOST_WRAPPER_FOR_kmem_cache_alloc
        FUNCTION_WRAPPER_DETACH(HOST, kmem_cache_alloc)
#   endif
#endif


#if defined(CAN_WRAP_kmem_cache_alloc_trace) && CAN_WRAP_kmem_cache_alloc_trace
#   ifndef HOST_WRAPPER_FOR_kmem_cache_alloc_trace
#       define HOST_WRAPPER_FOR_kmem_cache_alloc_trace
        FUNCTION_WRAPPER_DETACH(HOST, kmem_cache_alloc_trace)
#   endif
#endif


#if defined(CAN_WRAP_kmem_cache_alloc_node) && CAN_WRAP_kmem_cache_alloc_node
#   ifndef HOST_WRAPPER_FOR_kmem_cache_alloc_node
#       define HOST_WRAPPER_FOR_kmem_cache_alloc_node
        FUNCTION_WRAPPER_DETACH(HOST, kmem_cache_alloc_node)
#   endif
#endif


#if defined(CAN_WRAP_kmem_cache_free) && CAN_WRAP_kmem_cache_free
#   ifndef HOST_WRAPPER_FOR_kmem_cache_free
#       define HOST_WRAPPER_FOR_kmem_cache_free
        FUNCTION_WRAPPER_DETACH(HOST, kmem_cache_free)
#   endif
#endif


#if defined(CAN_WRAP___alloc_reserved_percpu) && CAN_WRAP___alloc_reserved_percpu
#   ifndef HOST_WRAPPER_FOR___alloc_reserved_percpu
#       define HOST_WRAPPER_FOR___alloc_reserved_percpu
        FUNCTION_WRAPPER_DETACH(HOST, __alloc_reserved_percpu)
#   endif
#endif


#if defined(CAN_WRAP___alloc_percpu) && CAN_WRAP___alloc_percpu
#   ifndef HOST_WRAPPER_FOR___alloc_percpu
#       define HOST_WRAPPER_FOR___alloc_percpu
        FUNCTION_WRAPPER_DETACH(HOST, __alloc_percpu)
#   endif
#endif


#if defined(CAN_WRAP_free_percpu) && CAN_WRAP_free_percpu
#   ifndef HOST_WRAPPER_FOR_free_percpu
#       define HOST_WRAPPER_FOR_free_percpu
        FUNCTION_WRAPPER_DETACH(HOST, free_percpu)
#   endif
#endif


#if defined(CAN_WRAP___alloc_pages_nodemask) && CAN_WRAP___alloc_pages_nodemask
#   ifndef HOST_WRAPPER_FOR___alloc_pages_nodemask
#       define HOST_WRAPPER_FOR___alloc_pages_nodemask
        FUNCTION_WRAPPER_DETACH(HOST, __alloc_pages_nodemask)
#   endif
#endif


#if defined(CAN_WRAP_alloc_pages_current) && CAN_WRAP_alloc_pages_current
#   ifndef HOST_WRAPPER_FOR_alloc_pages_current
#       define HOST_WRAPPER_FOR_alloc_pages_current
        FUNCTION_WRAPPER_DETACH(HOST, alloc_pages_current)
#   endif
#endif


#if defined(CAN_WRAP_alloc_pages_vma) && CAN_WRAP_alloc_pages_vma
#   ifndef HOST_WRAPPER_FOR_alloc_pages_vma
#       define HOST_WRAPPER_FOR_alloc_pages_vma
        FUNCTION_WRAPPER_DETACH(HOST, alloc_pages_vma)
#   endif
#endif


#if defined(CAN_WRAP___get_free_pages) && CAN_WRAP___get_free_pages
#   ifndef HOST_WRAPPER_FOR___get_free_pages
#       define HOST_WRAPPER_FOR___get_free_pages
        FUNCTION_WRAPPER_DETACH(HOST, __get_free_pages)
#   endif
#endif


#if defined(CAN_WRAP_get_zeroed_page) && CAN_WRAP_get_zeroed_page
#   ifndef HOST_WRAPPER_FOR_get_zeroed_page
#       define HOST_WRAPPER_FOR_get_zeroed_page
        FUNCTION_WRAPPER_DETACH(HOST, get_zeroed_page)
#   endif
#endif


#if defined(CAN_WRAP_alloc_pages_exact) && CAN_WRAP_alloc_pages_exact
#   ifndef HOST_WRAPPER_FOR_alloc_pages_exact
#       define HOST_WRAPPER_FOR_alloc_pages_exact
        FUNCTION_WRAPPER_DETACH(HOST, alloc_pages_exact)
#   endif
#endif


#if defined(CAN_WRAP_free_pages_exact) && CAN_WRAP_free_pages_exact
#   ifndef HOST_WRAPPER_FOR_free_pages_exact
#       define HOST_WRAPPER_FOR_free_pages_exact
        FUNCTION_WRAPPER_DETACH(HOST, free_pages_exact)
#   endif
#endif


#if defined(CAN_WRAP_alloc_pages_exact_nid) && CAN_WRAP_alloc_pages_exact_nid
#   ifndef HOST_WRAPPER_FOR_alloc_pages_exact_nid
#       define HOST_WRAPPER_FOR_alloc_pages_exact_nid
        FUNCTION_WRAPPER_DETACH(HOST, alloc_pages_exact_nid)
#   endif
#endif


#if defined(CAN_WRAP___free_pages) && CAN_WRAP___free_pages
#   ifndef HOST_WRAPPER_FOR___free_pages
#       define HOST_WRAPPER_FOR___free_pages
        FUNCTION_WRAPPER_DETACH(HOST, __free_pages)
#   endif
#endif


#if defined(CAN_WRAP_free_pages) && CAN_WRAP_free_pages
#   ifndef HOST_WRAPPER_FOR_free_pages
#       define HOST_WRAPPER_FOR_free_pages
        FUNCTION_WRAPPER_DETACH(HOST, free_pages)
#   endif
#endif


#if defined(CAN_WRAP_free_hot_cold_page) && CAN_WRAP_free_hot_cold_page
#   ifndef HOST_WRAPPER_FOR_free_hot_cold_page
#       define HOST_WRAPPER_FOR_free_hot_cold_page
        FUNCTION_WRAPPER_DETACH(HOST, free_hot_cold_page)
#   endif
#endif


#if defined(CAN_WRAP_free_hot_cold_page_list) && CAN_WRAP_free_hot_cold_page_list
#   ifndef HOST_WRAPPER_FOR_free_hot_cold_page_list
#       define HOST_WRAPPER_FOR_free_hot_cold_page_list
        FUNCTION_WRAPPER_DETACH(HOST, free_hot_cold_page_list)
#   endif
#endif


#if defined(CAN_WRAP___free_memcg_kmem_pages) && CAN_WRAP___free_memcg_kmem_pages
#   ifndef HOST_WRAPPER_FOR___free_memcg_kmem_pages
#       define HOST_WRAPPER_FOR___free_memcg_kmem_pages
        FUNCTION_WRAPPER_DETACH(HOST, __free_memcg_kmem_pages)
#   endif
#endif


#if defined(CAN_WRAP_free_memcg_kmem_pages) && CAN_WRAP_free_memcg_kmem_pages
#   ifndef HOST_WRAPPER_FOR_free_memcg_kmem_pages
#       define HOST_WRAPPER_FOR_free_memcg_kmem_pages
        FUNCTION_WRAPPER_DETACH(HOST, free_memcg_kmem_pages)
#   endif
#endif


#if defined(CAN_WRAP___krealloc) && CAN_WRAP___krealloc
#   ifndef HOST_WRAPPER_FOR___krealloc
#       define HOST_WRAPPER_FOR___krealloc
        FUNCTION_WRAPPER_DETACH(HOST, __krealloc)
#   endif
#endif


#if defined(CAN_WRAP_krealloc) && CAN_WRAP_krealloc
#   ifndef HOST_WRAPPER_FOR_krealloc
#       define HOST_WRAPPER_FOR_krealloc
        FUNCTION_WRAPPER_DETACH(HOST, krealloc)
#   endif
#endif


#if defined(CAN_WRAP_kzfree) && CAN_WRAP_kzfree
#   ifndef HOST_WRAPPER_FOR_kzfree
#       define HOST_WRAPPER_FOR_kzfree
        FUNCTION_WRAPPER_DETACH(HOST, kzfree)
#   endif
#endif


#if defined(CAN_WRAP___kmalloc_node) && CAN_WRAP___kmalloc_node
#   ifndef HOST_WRAPPER_FOR___kmalloc_node
#       define HOST_WRAPPER_FOR___kmalloc_node
        FUNCTION_WRAPPER_DETACH(HOST, __kmalloc_node)
#   endif
#endif


#if defined(CAN_WRAP___kmalloc_track_caller) && CAN_WRAP___kmalloc_track_caller
#   ifndef HOST_WRAPPER_FOR___kmalloc_track_caller
#       define HOST_WRAPPER_FOR___kmalloc_track_caller
        FUNCTION_WRAPPER_DETACH(HOST, __kmalloc_track_caller)
#   endif
#endif


#if defined(CAN_WRAP___kmalloc_node_track_caller) && CAN_WRAP___kmalloc_node_track_caller
#   ifndef HOST_WRAPPER_FOR___kmalloc_node_track_caller
#       define HOST_WRAPPER_FOR___kmalloc_node_track_caller
        FUNCTION_WRAPPER_DETACH(HOST, __kmalloc_node_track_caller)
#   endif
#endif


#if defined(CAN_WRAP_dma_pool_alloc) && CAN_WRAP_dma_pool_alloc
#   ifndef HOST_WRAPPER_FOR_dma_pool_alloc
#       define HOST_WRAPPER_FOR_dma_pool_alloc
        FUNCTION_WRAPPER_DETACH(HOST, dma_pool_alloc)
#   endif
#endif


#if defined(CAN_WRAP_dma_pool_free) && CAN_WRAP_dma_pool_free
#   ifndef HOST_WRAPPER_FOR_dma_pool_free
#       define HOST_WRAPPER_FOR_dma_pool_free
        FUNCTION_WRAPPER_DETACH(HOST, dma_pool_free)
#   endif
#endif


#if defined(CAN_WRAP_vmemmap_alloc_block) && CAN_WRAP_vmemmap_alloc_block
#   ifndef HOST_WRAPPER_FOR_vmemmap_alloc_block
#       define HOST_WRAPPER_FOR_vmemmap_alloc_block
        FUNCTION_WRAPPER_DETACH(HOST, vmemmap_alloc_block)
#   endif
#endif


#if defined(CAN_WRAP_vmemmap_alloc_block_buf) && CAN_WRAP_vmemmap_alloc_block_buf
#   ifndef HOST_WRAPPER_FOR_vmemmap_alloc_block_buf
#       define HOST_WRAPPER_FOR_vmemmap_alloc_block_buf
        FUNCTION_WRAPPER_DETACH(HOST, vmemmap_alloc_block_buf)
#   endif
#endif


#if defined(CAN_WRAP_mempool_alloc_slab) && CAN_WRAP_mempool_alloc_slab
#   ifndef HOST_WRAPPER_FOR_mempool_alloc_slab
#       define HOST_WRAPPER_FOR_mempool_alloc_slab
        FUNCTION_WRAPPER_DETACH(HOST, mempool_alloc_slab)
#   endif
#endif


#if defined(CAN_WRAP_mempool_free_slab) && CAN_WRAP_mempool_free_slab
#   ifndef HOST_WRAPPER_FOR_mempool_free_slab
#       define HOST_WRAPPER_FOR_mempool_free_slab
        FUNCTION_WRAPPER_DETACH(HOST, mempool_free_slab)
#   endif
#endif


#if defined(CAN_WRAP_mempool_kmalloc) && CAN_WRAP_mempool_kmalloc
#   ifndef HOST_WRAPPER_FOR_mempool_kmalloc
#       define HOST_WRAPPER_FOR_mempool_kmalloc
        FUNCTION_WRAPPER_DETACH(HOST, mempool_kmalloc)
#   endif
#endif


#if defined(CAN_WRAP_mempool_kfree) && CAN_WRAP_mempool_kfree
#   ifndef HOST_WRAPPER_FOR_mempool_kfree
#       define HOST_WRAPPER_FOR_mempool_kfree
        FUNCTION_WRAPPER_DETACH(HOST, mempool_kfree)
#   endif
#endif


#if defined(CAN_WRAP_mempool_alloc_pages) && CAN_WRAP_mempool_alloc_pages
#   ifndef HOST_WRAPPER_FOR_mempool_alloc_pages
#       define HOST_WRAPPER_FOR_mempool_alloc_pages
        FUNCTION_WRAPPER_DETACH(HOST, mempool_alloc_pages)
#   endif
#endif


#if defined(CAN_WRAP_mempool_free_pages) && CAN_WRAP_mempool_free_pages
#   ifndef HOST_WRAPPER_FOR_mempool_free_pages
#       define HOST_WRAPPER_FOR_mempool_free_pages
        FUNCTION_WRAPPER_DETACH(HOST, mempool_free_pages)
#   endif
#endif
#endif

#endif /* WRAPPERS_H_ */
