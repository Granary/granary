/*
 * detach.cc
 *
 *  Created on: Nov 18, 2012
 *      Author: pag
 */


#include "granary/detach.h"
#include "granary/hash_table.h"

namespace granary {

#if CONFIG_ENABLE_WRAPPERS

    static hash_table<app_pc, const function_wrapper *> DETACH_HASH_TABLE;

    static function_wrapper DETACH_WRAPPER = {
        reinterpret_cast<app_pc>(&detach),
        reinterpret_cast<app_pc>(&detach)
    };

    STATIC_INITIALISE({
        DETACH_HASH_TABLE.store(
            DETACH_WRAPPER.original_address, &DETACH_WRAPPER);

        for(unsigned i(0); ; ++i) {
            const function_wrapper &wrapper(FUNCTION_WRAPPERS[i]);
            if(!wrapper.original_address) {
                break;
            }
            DETACH_HASH_TABLE.store(wrapper.original_address, &wrapper);
        }
    });

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
	    if(!DETACH_HASH_TABLE.load(target, wrapper)) {
	        return nullptr;
	    }

	    return wrapper->wrapper_address;
	}

#endif /* CONFIG_ENABLE_WRAPPERS */

	/// Detach Granary.
	void detach(void) throw() {
	    ASM("");
	}
}
