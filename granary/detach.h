/*
 * detach.h
 *
 *  Created on: Nov 18, 2012
 *      Author: pag
 */

#ifndef GRANARY_DETACH_H_
#define GRANARY_DETACH_H_

#include "granary/globals.h"

namespace granary {


    /// Represents an entry in the detach hash table. Entries need to map
    /// original function addresses to wrapped function addresses.
    struct function_wrapper {
        const app_pc original_address;
        const app_pc wrapper_address;
    };


    /// Represents the entries of the detach hash table. The indexes of each
    /// function in this array are found in `granary/gen/detach.h`. The actual
    /// entries of this array are statically populated in
    /// `granary/gen/detach.cc`.
    extern const function_wrapper FUNCTION_WRAPPERS[];


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
	__attribute__((noinline))
	void detach(void) throw();

}

#endif /* GRANARY_DETACH_H_ */
