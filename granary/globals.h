/*
 * globals.h
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#ifndef granary_GLOBALS_H_
#define granary_GLOBALS_H_

#include <cstring>
#include <stdint.h>

#include "granary/pp.h"
#include "granary/utils.h"
#include "granary/types/dynamorio.h"

namespace granary {

    /// Program counter type.
    typedef dynamorio::app_pc app_pc;

}


#endif /* granary_GLOBALS_H_ */
