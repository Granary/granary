/*
 * utils.cc
 *
 *  Created on: Nov 21, 2012
 *      Author: pag
 */


#include "granary/globals.h"
#include "granary/x86/asm_defines.asm"

extern int main(int, char **);

namespace granary {

    static uint64_t get_application_start(void) throw() {
        return unsafe_cast<uint64_t>(main);
    }


    /// Returns an offset of some application code from the beginning of
    /// application code.
    int32_t to_application_offset(uint64_t addr) throw() {
        return addr - get_application_start();
    }


    uint64_t from_application_offset(int32_t addr) throw() {
        return addr + get_application_start();
    }
}

