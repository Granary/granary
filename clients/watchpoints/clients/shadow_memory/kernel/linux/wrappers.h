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

using namespace granary;
using namespace client::wp;

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

            if(is_watched_address(arg)) {
                descriptor_of(arg)->update_type(arg);
            }

            client::report();

            PRE_OUT_WRAP(*unwatched_address_check(arg));
        }
        PRE_IN {
            if(!is_valid_address(arg)) {
                return;
            }

            if(is_watched_address(arg)) {
                descriptor_of(arg)->update_type(arg);
            }

            PRE_IN_WRAP(*unwatched_address_check(arg));
        }
        INHERIT_POST_INOUT
        RETURN_OUT {
            if(!is_valid_address(arg)){
                return;
            }

            if(is_watched_address(arg)) {
                descriptor_of(arg)->update_type(arg);
            }

        }
        RETURN_IN {
            if(!is_valid_address(arg)){
                return;
            }

            if(is_watched_address(arg)) {
                descriptor_of(arg)->update_type(arg);
            }

        }
    })
#endif


#endif /* WRAPPER_H_ */
