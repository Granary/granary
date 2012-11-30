/*
 * mangle.h
 *
 *  Created on: 2012-11-27
 *      Author: pag
 *     Version: $Id$
 */

#ifndef Granary_MANGLE_H_
#define Granary_MANGLE_H_

#include "instruction.h"


namespace granary {


    /// Stage an 8-byte hot patch. This will encode the instruction `in` into
    /// the `stage` location (as if it were going to be placed at the `dest`
    /// location, and then encodes however many NOPs are needed to fill in 8
    /// bytes.
    void stage_8byte_hot_patch(instruction in, app_pc stage, app_pc dest);

}

#undef DPM_DECLARE_REG
#undef DPM_DECLARE_REG_CONT
#endif /* Granary_MANGLE_H_ */
