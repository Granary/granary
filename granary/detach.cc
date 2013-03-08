/*
 * detach.cc
 *
 *  Created on: Nov 18, 2012
 *      Author: pag
 */


#include "granary/detach.h"
#include "granary/utils.h"
#include "granary/hash_table.h"

#if !GRANARY_IN_KERNEL
#   ifndef _GNU_SOURCE
#      define _GNU_SOURCE
#   endif
#   include <dlfcn.h>
#endif

namespace granary {

    static static_data<
        hash_table<app_pc, const function_wrapper *>
    > DETACH_HASH_TABLE;

    STATIC_INITIALISE({

        DETACH_HASH_TABLE.construct();

        // add all wrappers to the detach hash table
#if CONFIG_ENABLE_WRAPPERS
        for(unsigned i(0); i < LAST_DETACH_ID; ++i) {
            const function_wrapper &wrapper(FUNCTION_WRAPPERS[i]);
            DETACH_HASH_TABLE->store(wrapper.original_address, &wrapper);
        }
#endif /* CONFIG_ENABLE_WRAPPERS */

        // add internal dynamic symbols to the detach hash table.
#if !GRANARY_IN_KERNEL
        for(unsigned i(LAST_DETACH_ID + 1); ; ++i) {
            function_wrapper &wrapper(FUNCTION_WRAPPERS[i]);

            if(!wrapper.name) {
                break;
            }

            wrapper.original_address = reinterpret_cast<app_pc>(
                dlsym(RTLD_NEXT, wrapper.name));
            if(wrapper.original_address) {
                wrapper.wrapper_address = wrapper.original_address;
            }

            DETACH_HASH_TABLE->store(wrapper.original_address, &wrapper);
        }
#endif /* !GRANARY_IN_KERNEL */
    })



    /// Add a detach target to the hash table.
    void add_detach_target(function_wrapper &wrapper) throw() {
        DETACH_HASH_TABLE->store(wrapper.original_address, &wrapper);
    }


	/// Returns the address of a detach point. For example, in the
	/// kernel, if pc == &printk is a detach point then this will
	/// return the address of the printk wrapper (which might itself
	/// be printk).
	///
	/// Returns:
	///		A translated target address, or nullptr if this isn't a
	/// 	detach target.
	app_pc find_detach_target(app_pc target) throw() {
	    const function_wrapper *wrapper(nullptr);
	    if(!DETACH_HASH_TABLE->load(target, wrapper)) {
	        return nullptr;
	    }

	    printf("detaching on %s\n", wrapper->name);
	    return wrapper->wrapper_address;
	}


	/// Detach Granary.
	void detach(void) throw() {
	    ASM("");
	}


	GRANARY_DETACH_POINT(detach)
}


/// Special cases for dynamically loaded symbols in user space Linux.
#if 0 && !GRANARY_IN_KERNEL && defined(__linux)
    GRANARY_DYNAMIC_DETACH_POINT(__GI___libc_dlopen_mode)
    GRANARY_DYNAMIC_DETACH_POINT(__GI___libc_dlsym)

    GRANARY_DYNAMIC_DETACH_POINT(__libc_dlopen_mode)
    GRANARY_DYNAMIC_DETACH_POINT(__libc_dlsym)
#endif


