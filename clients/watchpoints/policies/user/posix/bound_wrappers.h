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


#define WRAPPER_FOR_pointer
POINTER_WRAPPER({
    PRE_OUT {
        if(!is_valid_address(arg)) {
            return;
        }
        if(is_watched_address(arg)) {
            arg = unwatched_address(arg);
            if(!is_valid_address(arg)) {
                return;
            }
        }
        PRE_OUT_WRAP(*arg);
    }
    INHERIT_PRE_IN
    INHERIT_POST_INOUT
    INHERIT_RETURN_INOUT
})


#define WRAPPER_FOR_void_pointer
TYPE_WRAPPER(void *, {
    NO_PRE_IN
    PRE_OUT {
        if(is_watched_address(arg)) {
            printf("unwrapping void pointer %p!\n", arg);
            arg = unwatched_address(arg);
        }
    }
    NO_POST
    NO_RETURN
})


#if 0
#if defined(CAN_WRAP_realloc) && CAN_WRAP_realloc
#   define WRAPPER_FOR_realloc
    FUNCTION_WRAPPER(realloc, (void *), (void *ptr, size_t size), {
        bound_descriptor *desc(nullptr);

        if(is_watched_address(ptr)) {
            desc = descriptor_of(ptr);

        }
        void *ptr(malloc(size));
        if(!ptr) {
            return ptr;
        }
        add_watchpoint(ptr, ptr, size);
        return ptr;
    })
#endif
#endif


#if defined(CAN_WRAP_malloc) && CAN_WRAP_malloc
#   define WRAPPER_FOR_malloc
    FUNCTION_WRAPPER(malloc, (void *), (size_t size), {
        void *ptr(malloc(size));
        if(!ptr) {
            return ptr;
        }
        add_watchpoint(ptr, ptr, size);
        return ptr;
    })
#endif


#if defined(CAN_WRAP___libc_malloc) && CAN_WRAP___libc_malloc
#   define WRAPPER_FOR___libc_malloc
    FUNCTION_WRAPPER(__libc_malloc, (void *), (size_t size), {
        void *ptr(__libc_malloc(size));
        if(!ptr) {
            return ptr;
        }
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
