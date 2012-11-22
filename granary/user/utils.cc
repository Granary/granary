/*
 * utils.cc
 *
 *  Created on: Nov 21, 2012
 *      Author: pag
 */


#include "granary/globals.h"

namespace granary {

    static uint64_t get_application_start(void) throw() {
        register uint64_t ret_addr asm("rax");
        __asm__ __volatile__ (
            "movq $_start, %0;"
            : "=r"(ret_addr)
        );
        return ret_addr;
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

