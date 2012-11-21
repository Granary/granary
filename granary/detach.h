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


	/// Returns the address of a detach point. For example, in the
	/// kernel, if pc == &printk is a detach point then this will
	/// return the address of the printk wrapper (which might itself
	/// be printk).
	///
	/// Returns:
	///		A translated target address, or nullptr if this isn't a
	/// 	detach target.
	app_pc find_detach_target(app_pc pc) throw();

}

#endif /* GRANARY_DETACH_H_ */
