/*
 * detach.cc
 *
 *  Created on: Nov 18, 2012
 *      Author: pag
 */


#include "granary/detach.h"
#include "granary/utils.h"
#include "granary/hash_table.h"

namespace granary {

    static static_data<
        hash_table<app_pc, const function_wrapper *>
    > DETACH_HASH_TABLE;

    STATIC_INITIALISE({

        DETACH_HASH_TABLE.construct();

#if CONFIG_ENABLE_WRAPPERS
        for(unsigned i(0); ; ++i) {
            const function_wrapper &wrapper(FUNCTION_WRAPPERS[i]);
            if(!wrapper.original_address) {
                break;
            }
            DETACH_HASH_TABLE->store(wrapper.original_address, &wrapper);
        }
#endif /* CONFIG_ENABLE_WRAPPERS */
    });


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

	    return wrapper->wrapper_address;
	}


	/// Detach Granary.
	void detach(void) throw() {
	    ASM("");
	}


	GRANARY_DETACH_POINT(detach)
}
