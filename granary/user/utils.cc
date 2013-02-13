/*
 * utils.cc
 *
 *  Created on: Nov 21, 2012
 *      Author: pag
 */


#include "granary/globals.h"
#include "granary/x86/asm_defines.asm"

namespace granary {

    static uint64_t get_application_start(void) throw() {
        FAULT; // TODO
        return 0;
    }


    /// Returns an offset of some application code from the beginning of
    /// application code.
    int32_t to_application_offset(uint64_t addr) throw() {
        FAULT; // TODO
        return addr - get_application_start();
    }


    uint64_t from_application_offset(int32_t addr) throw() {
        FAULT; // TODO
        return addr + get_application_start();
    }
}

