/*
 * detach.h
 *
 *  Created on: Nov 18, 2012
 *      Author: pag
 */

#ifndef GRANARY_DETACH_H_
#define GRANARY_DETACH_H_

#include "granary/globals.h"

#define GRANARY_DETACH_POINT(func_name) \
    STATIC_INITIALISE({ \
        static granary::function_wrapper wrapper = { \
            reinterpret_cast<granary::app_pc>(&func_name), \
            reinterpret_cast<granary::app_pc>(&func_name), \
            #func_name \
        }; \
        granary::add_detach_target(wrapper); \
    })

namespace granary {

    /// Represents an entry in the detach hash table. Entries need to map
    /// original function addresses to wrapped function addresses.
    struct function_wrapper {
        const app_pc original_address;
        const app_pc wrapper_address;
        const char * const name;
    };


#if CONFIG_ENABLE_WRAPPERS
    /// Represents the entries of the detach hash table. The indexes of each
    /// function in this array are found in `granary/gen/detach.h`. The actual
    /// entries of this array are statically populated in
    /// `granary/gen/detach.cc`.
    extern const function_wrapper FUNCTION_WRAPPERS[];
#endif


    /// Add a detach target to the hash table.
    void add_detach_target(function_wrapper &wrapper) throw();


	/// Returns the address of a detach point. For example, in the
	/// kernel, if pc == &printk is a detach point then this will
	/// return the address of the printk wrapper (which might itself
	/// be printk).
	///
	/// Returns:
	///		A translated target address, or nullptr if this isn't a
	/// 	detach target.
	app_pc find_detach_target(app_pc pc) throw();


	/// Detach Granary.
	__attribute__((noinline, optimize("O0")))
	void detach(void) throw();

}

#endif /* GRANARY_DETACH_H_ */
