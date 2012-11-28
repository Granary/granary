/*
 * state.cc
 *
 *  Created on: Nov 21, 2012
 *      Author: pag
 */

#include "granary/state.h"

namespace granary {
    /// Hack to ensure static initializers are compiled.
    static_init_list STATIC_LIST_HEAD;
}
