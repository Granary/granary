/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * ibl.h
 *
 *  Created on: 2013-10-31
 *      Author: Peter Goodman
 */

#ifndef GRANARY_IBL_H_
#define GRANARY_IBL_H_

#include "granary/globals.h"

namespace granary {


    enum {
        NUM_IBL_JUMP_TABLE_ENTRIES = 2048
    };


    extern "C" {
        unsigned granary_ibl_hash(app_pc) throw();
    }


    /// Return the IBL entry routine. The IBL entry routine is responsible
    /// for looking to see if an address (stored in reg::arg1) is located
    /// in the CPU-private code cache or in the global code cache. If the
    /// address is in the CPU-private code cache.
    void ibl_lookup_stub(
        instruction_list &ibl,
        instruction in,
        instrumentation_policy policy
        _IF_PROFILE_IBL( app_pc cti_addr )
    ) throw();


    /// Coarse-grained locks used to guard the creation and addition of
    /// IBL exit routines into the IBL jump table. The lock is exposed so that
    /// "duplicate" exit routines are not added. We avoid duplicates by
    /// bootstrapping on the global code cache's inability to have duplication.
    ///
    /// Note: Must be called around a use of `ibl_exit_routine`.
    void ibl_lock(void) throw();
    void ibl_unlock(void) throw();


    /// Return or generate the IBL exit routine for a particular jump target.
    /// The target can either be code cache or native code.
    ///
    /// Note: invocations must be guarded by `ibl_lock` and `ibl_unlock`.
    app_pc ibl_exit_routine(
        app_pc mangled_target_pc,
        app_pc instrumented_target_pc
        _IF_PROFILE_IBL( app_pc source_addr )
    ) throw();
}

#endif /* GRANARY_IBL_H_ */
