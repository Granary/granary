/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * bound_wrappers.h
 *
 *  Created on: 2013-05-07
 *      Author: Peter Goodman
 */

#ifndef WATCHPOINT_BOUND_WRAPPERS_H_
#define WATCHPOINT_BOUND_WRAPPERS_H_


#include "clients/watchpoints/policies/bound_policy.h"

using namespace client::wp;

#if defined(CAN_WRAP_malloc) && CAN_WRAP_malloc
#   define WRAPPER_FOR_malloc
    FUNCTION_WRAPPER(malloc, (void *), (size_t size), {
        void *ptr(malloc(size));
        add_watchpoint(ptr, ptr, size);
        return ptr;
    })
#endif


#if defined(CAN_WRAP___libc_malloc) && CAN_WRAP___libc_malloc
#   define WRAPPER_FOR___libc_malloc
    FUNCTION_WRAPPER(__libc_malloc, (void *), (size_t size), {
        void *ptr(__libc_malloc(size));
        add_watchpoint(ptr, ptr, size);
        return ptr;
    })
#endif


#if defined(CAN_WRAP_free) && CAN_WRAP_free
#   define WRAPPER_FOR_free
    FUNCTION_WRAPPER_VOID(free, (void *ptr), {
        if(is_watched_address(ptr)) {
            free_descriptor_of(ptr);
            ptr = unwatched_address(ptr);
        }
        return free(ptr);
    })
#endif


#if defined(CAN_WRAP_cfree) && CAN_WRAP_cfree
#   define WRAPPER_FOR_cfree
    FUNCTION_WRAPPER_VOID(cfree, (void *ptr), {
        if(is_watched_address(ptr)) {
            free_descriptor_of(ptr);
            ptr = unwatched_address(ptr);
        }
        return cfree(ptr);
    })
#endif


#if defined(CAN_WRAP___libc_free) && CAN_WRAP___libc_free
#   define WRAPPER_FOR___libc_free
    FUNCTION_WRAPPER_VOID(__libc_free, (void *ptr), {
        if(is_watched_address(ptr)) {
            free_descriptor_of(ptr);
            ptr = unwatched_address(ptr);
        }
        return __libc_free(ptr);
    })
#endif


#endif /* WATCHPOINT_BOUND_WRAPPERS_H_ */
