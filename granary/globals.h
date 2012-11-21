/*
 * globals.h
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#ifndef granary_GLOBALS_H_
#define granary_GLOBALS_H_

#include <cstring>
#include <cstddef>
#include <stdint.h>

#include "granary/pp.h"
#include "granary/utils.h"
#include "granary/type_traits.h"
#include "granary/types/dynamorio.h"

namespace granary {

    /// Program counter type.
    typedef typename dynamorio::app_pc app_pc;


    enum {
        PAGE_SIZE = 4096U
    };
}

#include "granary/allocator.h"

#endif /* granary_GLOBALS_H_ */
