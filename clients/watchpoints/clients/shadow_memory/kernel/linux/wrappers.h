/*
 * wrapper.h
 *
 *  Created on: 2013-08-06
 *      Author: akshayk
 */

#ifndef _SHADOW_POLICY_WRAPPERS_H_
#define _SHADOW_POLICY_WRAPPERS_H_

//#include "clients/watchpoints/clients/shadow_memory/kernel/linux/scanner.h"
#include "clients/watchpoints/clients/shadow_memory/kernel/linux/allocator_wrappers.h"

namespace client {
    extern void report(void);
}

//extern void client::report(void);

#ifndef APP_WRAPPER_FOR_pointer
#   define APP_WRAPPER_FOR_pointer
    POINTER_WRAPPER({
        PRE_OUT {
            if(!is_valid_address(arg)) {
                return;
            }
            client::report();
            //SCAN_HEAD_FUNC(decltype(arg))(arg);
#if 0
            client::wp::add_to_scanlist(
                unsafe_cast<app_pc>(arg),
                unsafe_cast<app_pc>(SCAN_HEAD_FUNC(decltype(arg))));
#endif
            PRE_OUT_WRAP(*unwatched_address_check(arg));
        }
        PRE_IN {
            if(!is_valid_address(arg)) {
                return;
            }
#if 0
            client::wp::add_to_scanlist(
                unsafe_cast<app_pc>(arg),
                unsafe_cast<app_pc>(SCAN_HEAD_FUNC(decltype(arg))));
#endif
            PRE_IN_WRAP(*unwatched_address_check(arg));
        }
        INHERIT_POST_INOUT
        RETURN_OUT {
            if(!is_valid_address(arg)){
                return;
            }
#if 0
            client::wp::add_to_scanlist(
                unsafe_cast<app_pc>(arg),
                unsafe_cast<app_pc>(SCAN_HEAD_FUNC(decltype(arg))));
#endif
        }
        RETURN_IN {
            if(!is_valid_address(arg)){
                return;
            }
#if 0
            client::wp::add_to_scanlist(
                unsafe_cast<app_pc>(arg),
                unsafe_cast<app_pc>(SCAN_HEAD_FUNC(decltype(arg))));
#endif
        }
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



#endif /* WRAPPER_H_ */
