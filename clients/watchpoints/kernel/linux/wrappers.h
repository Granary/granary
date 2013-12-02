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
#   define WRAP_CONTEXT APP
#   include "clients/watchpoints/clients/bounds_checker/kernel/linux/wrappers.h"
#   if CONFIG_FEATURE_INSTRUMENT_HOST
#       undef WRAP_CONTEXT
#       define WRAP_CONTEXT HOST
#       include "clients/watchpoints/clients/bounds_checker/kernel/linux/wrappers.h"
#   endif
#endif


/// Null policy that taints all addresses.
#ifdef CLIENT_WATCHPOINT_WATCHED
#   include "clients/watchpoints/clients/everything_watched/kernel/linux/wrappers.h"
#   include "clients/watchpoints/clients/everything_watched/kernel/linux/patch_wrappers.h"
#endif


/// Null policy that augments to a watchpoints null policy. Uses everything_watched's
/// wrappers.
#ifdef CLIENT_WATCHPOINT_AUGMENT
#   include "clients/watchpoints/clients/everything_watched/kernel/linux/wrappers.h"
#   include "clients/watchpoints/clients/everything_watched/kernel/linux/patch_wrappers.h"
#endif


/// Stats tracking policy that taints addresses.
#ifdef CLIENT_WATCHPOINT_STATS
#   include "clients/watchpoints/clients/everything_watched/kernel/linux/wrappers.h"
#   include "clients/watchpoints/clients/everything_watched/kernel/linux/patch_wrappers.h"
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


#endif /* WRAPPERS_H_ */
