/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * bound_wrappers.h
 *
 *  Created on: 2013-05-07
 *      Author: Peter Goodman
 */

#ifndef WATCHPOINT_ARK_WRAPPERS_H_
#define WATCHPOINT_ARK_WRAPPERS_H_


#include "clients/watchpoints/clients/ark/instrument.h"


using namespace client::wp;

#if defined(CAN_WRAP_fread) && CAN_WRAP_fread
#   define APP_WRAPPER_FOR_fread
	FUNCTION_WRAPPER(APP, fread, (size_t), (void *ptr, size_t size, size_t count, FILE *stream), {
		granary::printf("ptr:%p", ptr);
		return fread(ptr, size, count, stream);
	})
#endif

#if defined(CAN_WRAP__libc_fread) && CAN_WRAP__libc_fread
#   define APP_WRAPPER_FOR__libc_fread
	FUNCTION_WRAPPER(APP, __libc_fread, (size_t), (void *ptr, size_t size, size_t count, FILE *stream), {
		granary::printf("ptr:%p", ptr);
		return __libc_fread(ptr, size, count, stream);
	})
#endif

#define MALLOCATOR(func, size) \
    { \
        void *ptr(func(size)); \
        if(!ptr) { \
            return ptr; \
        } \
		granary::printf("allocated:%p", ptr); \
        add_watchpoint(ptr); \
        return ptr; \
    }


#if defined(CAN_WRAP_malloc) && CAN_WRAP_malloc
#   define APP_WRAPPER_FOR_malloc
    FUNCTION_WRAPPER(APP, malloc, (void *), (size_t size), MALLOCATOR(malloc, size))
#endif


#if defined(CAN_WRAP___libc_malloc) && CAN_WRAP___libc_malloc
#   define APP_WRAPPER_FOR___libc_malloc
    FUNCTION_WRAPPER(APP, __libc_malloc, (void *), (size_t size), MALLOCATOR(__libc_malloc, size))
#endif

#endif /* WATCHPOINT_ARK_WRAPPERS_H_ */
