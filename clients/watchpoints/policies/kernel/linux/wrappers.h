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


#if defined(CAN_WRAP___phys_addr) && CAN_WRAP___phys_addr
#   ifndef APP_WRAPPER_FOR___phys_addr
#       define APP_WRAPPER_FOR___phys_addr
        FUNCTION_WRAPPER(APP, __phys_addr, (uintptr_t), (uintptr_t addr), {
            return __phys_addr(unwatched_address(addr));
        })
#   endif
#   ifndef HOST_WRAPPER_FOR___phys_addr
#       define HOST_WRAPPER_FOR___phys_addr
        FUNCTION_WRAPPER(HOST, __phys_addr, (uintptr_t), (uintptr_t addr), {
            return __phys_addr(unwatched_address(addr));
        })
#   endif
#endif


#if defined(CAN_WRAP___virt_addr_valid) && CAN_WRAP___virt_addr_valid
#   ifndef APP_WRAPPER_FOR___virt_addr_valid
#       define APP_WRAPPER_FOR___virt_addr_valid
        FUNCTION_WRAPPER(APP, __virt_addr_valid, (bool), (uintptr_t addr), {
            return __virt_addr_valid(unwatched_address(addr));
        })
#   endif
#   ifndef HOST_WRAPPER_FOR___virt_addr_valid
#       define HOST_WRAPPER_FOR___virt_addr_valid
        FUNCTION_WRAPPER(HOST, __virt_addr_valid, (bool), (uintptr_t addr), {
            return __virt_addr_valid(unwatched_address(addr));
        })
#   endif
#endif

#endif /* WRAPPERS_H_ */
