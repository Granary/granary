/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * bound_wrappers.h
 *
 *  Created on: 2013-05-07
 *      Author: Peter Goodman
 */

#ifndef WATCHPOINT_WATCHED_WRAPPERS_H_
#define WATCHPOINT_WATCHED_WRAPPERS_H_


#include "clients/watchpoints/clients/everything_watched/instrument.h"


using namespace client::wp;


#if defined(CAN_WRAP_realloc) && CAN_WRAP_realloc
#   define APP_WRAPPER_FOR_realloc
    FUNCTION_WRAPPER(APP, realloc, (void *), (void *ptr, size_t size), {
        void *new_ptr(realloc(unwatched_address_check(ptr), size));
        if(new_ptr) {
            add_watchpoint(new_ptr);
        }
        return new_ptr;
    })
#endif


#define MALLOCATOR(func, size) \
    { \
        void *ptr(func(size)); \
        if(!ptr) { \
            add_watchpoint(ptr); \
        } \
        return ptr; \
    }


#if defined(CAN_WRAP_malloc) && CAN_WRAP_malloc
#   define APP_WRAPPER_FOR_malloc
    FUNCTION_WRAPPER(APP, malloc, (void *), (size_t size), MALLOCATOR(malloc, size))
#endif


#if defined(CAN_WRAP_valloc) && CAN_WRAP_valloc
#   define APP_WRAPPER_FOR_valloc
    FUNCTION_WRAPPER(APP, valloc, (void *), (size_t size), MALLOCATOR(valloc, size))
#endif


#if defined(CAN_WRAP_malloc) && CAN_WRAP_calloc
#   define APP_WRAPPER_FOR_calloc
    FUNCTION_WRAPPER(APP, calloc, (void *), (size_t count, size_t size), {
        void *ptr(calloc(count, size));
        if(!ptr) {
            add_watchpoint(ptr);
        }
        return ptr;
    })
#endif


#if defined(CAN_WRAP_free) && CAN_WRAP_free
#   define APP_WRAPPER_FOR_free
	FUNCTION_WRAPPER_VOID(APP, free, (void *ptr), {
		free(unwatched_address_check(ptr));
	})
#endif


#endif /* WATCHPOINT_WATCHED_WRAPPERS_H_ */
