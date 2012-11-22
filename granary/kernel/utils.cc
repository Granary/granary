/*
 * utils.cc
 *
 *  Created on: Nov 21, 2012
 *      Author: pag
 */


#include "granary/globals.h"

namespace granary {


    /// Returns an offset of some application code from the beginning of
    /// application code.
    int32_t to_application_offset(uint64_t addr) throw() {
        return addr - KERNEL_MODULE_START;
    }


    uint64_t from_application_offset(int32_t addr) throw() {
        uint64_t ret_addr(addr);
        return ret_addr + KERNEL_MODULE_START;
    }
}
