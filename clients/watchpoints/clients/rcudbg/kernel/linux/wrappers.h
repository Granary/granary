/*
 * wrappers.h
 *
 *  Created on: 2013-08-09
 *      Author: akshayk
 */

#ifndef _RCU_WRAPPERS_H_
#define _RCU_WRAPPERS_H_

#include "clients/watchpoints/clients/rcu_debugger/kernel/linux/rcu.h"
#include "clients/watchpoints/clients/rcu_debugger/kernel/linux/allocator_wrappers.h"

#ifndef APP_WRAPPER_FOR_pointer
#   define APP_WRAPPER_FOR_pointer
    POINTER_WRAPPER({
        PRE_OUT {
            if(!is_valid_address(arg)) {
                return;
            }
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



#endif /* _RCU_WRAPPERS_H_ */
