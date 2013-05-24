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


#ifndef APP_WRAPPER_FOR_pointer
#   define APP_WRAPPER_FOR_pointer
    POINTER_WRAPPER({
        PRE_OUT {
            if(!is_valid_address(arg)) {
                return;
            }
            RELAX_WRAP_DEPTH;
            PRE_OUT_WRAP(*unwatched_address_check(arg));
        }
        PRE_IN {
            if(!is_valid_address(arg)) {
                return;
            }
            RELAX_WRAP_DEPTH;
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


#if defined(CAN_WRAP_mutex_lock) && CAN_WRAP_mutex_lock
#   ifndef HOST_WRAPPER_FOR_mutex_lock
#       define HOST_WRAPPER_FOR_mutex_lock
        FUNCTION_WRAPPER_VOID(HOST, mutex_lock, (struct mutex * lock), {
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
#endif


#if defined(CAN_WRAP_mutex_lock_killable) && CAN_WRAP_mutex_lock_killable
#   ifndef HOST_WRAPPER_FOR_mutex_lock_killable
#       define HOST_WRAPPER_FOR_mutex_lock_killable
        FUNCTION_WRAPPER(HOST, mutex_lock_killable, (int), (struct mutex * lock), {
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
#endif


#if defined(CAN_WRAP_mutex_unlock) && CAN_WRAP_mutex_unlock
#   ifndef HOST_WRAPPER_FOR_mutex_unlock
#       define HOST_WRAPPER_FOR_mutex_unlock
        FUNCTION_WRAPPER_VOID(HOST, mutex_unlock, (struct mutex * lock), {
            mutex_unlock(unwatched_address_check(lock));
        })
#   endif
#endif


#define COPY_TO_FROM_USER(func) \
    FUNCTION_WRAPPER(HOST, func, (unsigned long), (void *a, void *b, unsigned len), {\
        return func( \
            unwatched_address_check(a), \
            unwatched_address_check(b), \
            len); \
    })


#if defined(CAN_WRAP_copy_user_enhanced_fast_string) && CAN_WRAP_copy_user_enhanced_fast_string
#   ifndef HOST_WRAPPER_FOR_copy_user_enhanced_fast_string
#       define HOST_WRAPPER_FOR_copy_user_enhanced_fast_string
        COPY_TO_FROM_USER(copy_user_enhanced_fast_string)
#   endif
#endif


#if defined(CAN_WRAP_copy_user_generic_string) && CAN_WRAP_copy_user_generic_string
#   ifndef HOST_WRAPPER_FOR_copy_user_generic_string
#       define HOST_WRAPPER_FOR_copy_user_generic_string
        COPY_TO_FROM_USER(copy_user_generic_string)
#   endif
#endif


#if defined(CAN_WRAP_copy_user_generic_unrolled) && CAN_WRAP_copy_user_generic_unrolled
#   ifndef HOST_WRAPPER_FOR_copy_user_generic_unrolled
#       define HOST_WRAPPER_FOR_copy_user_generic_unrolled
        COPY_TO_FROM_USER(copy_user_generic_unrolled)
#   endif
#endif


#if defined(CAN_WRAP__copy_to_user) && CAN_WRAP__copy_to_user
#   ifndef HOST_WRAPPER_FOR__copy_to_user
#       define HOST_WRAPPER_FOR__copy_to_user
        COPY_TO_FROM_USER(_copy_to_user)
#   endif
#endif


#if defined(CAN_WRAP__copy_from_user) && CAN_WRAP__copy_from_user
#   ifndef HOST_WRAPPER_FOR__copy_from_user
#       define HOST_WRAPPER_FOR__copy_from_user
        COPY_TO_FROM_USER(_copy_from_user)
#   endif
#endif


#if defined(CAN_WRAP_copy_in_user) && CAN_WRAP_copy_in_user
#   ifndef HOST_WRAPPER_FOR_copy_in_user
#       define HOST_WRAPPER_FOR_copy_in_user
        COPY_TO_FROM_USER(copy_in_user)
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
#endif


#if defined(CAN_WRAP___switch_to)
    GRANARY_DETACH_INSTEAD_OF_WRAP(__switch_to, RUNNING_AS_HOST)
#endif


#if defined(CAN_WRAP___schedule)
    GRANARY_DETACH_INSTEAD_OF_WRAP(__schedule, RUNNING_AS_HOST)
#endif


#endif /* WRAPPERS_H_ */
