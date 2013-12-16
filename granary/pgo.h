/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * pgo.h
 *
 *  Created on: 2013-12-16
 *      Author: Peter Goodman
 */

#ifndef PGO_H_
#define PGO_H_

#include "granary/globals.h"
#include "granary/instruction.h"

namespace granary {

    /// Optimise a conditional branch using profile-guided optimisation. This
    /// function either returns `next_pc` if the fall-through of the Jcc is the
    /// most likely target, or switches the Jcc instruction in place, and
    /// returns its old target as the fall-through. The purpose of this is to
    /// tie in nicely with Granary's ahead-of-time tracing infrastructure.
    app_pc profile_optimise_jcc(
        instruction in,
        app_pc block_start_pc,
        app_pc next_pc
    ) throw();


    /// Optimise an indirect CTI into a direct CTI. This will potentially add
    /// instructions before `in` inside of `ls` that test the target of the CTI
    /// against a known target, and if they match, then jump to that target.
    void profile_optimise_indirect_cti(
        instruction_list &ls,
        instruction in
    ) throw();
}

#endif /* PGO_H_ */
