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


#endif /* WRAPPER_H_ */
