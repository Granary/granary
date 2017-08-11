/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * utils.cc
 *
 *  Created on: 2013-06-28
 *      Author: Peter Goodman
 */

#include "clients/watchpoints/utils.h"

using namespace granary;

namespace client { namespace wp {


    /// Register-specific (generated) functions to do bounds checking.
    static unsigned REG_TO_INDEX[] = {
        ~0U,    // null
        0,      // rax
        1,      // rcx
        2,      // rdx
        3,      // rbx
        ~0U,    // rsp
        4,      // rbp
        5,      // rsi
        6,      // rdi
        7,      // r8
        8,      // r9
        9,      // r10
        10,     // r11
        11,     // r12
        12,     // r13
        13,     // r14
        14      // r15
    };


    /// Size to index.
    static unsigned SIZE_TO_ORDER[] = {
        ~0U,
        0,      // 1
        1,      // 2
        ~3U,
        2,      // 4
        ~5U,
        ~6U,
        ~7U,
        3,      // 8
        ~8U,
        ~9U,
        ~10U,
        ~11U,
        ~12U,
        ~13U,
        ~14U,
        4       // 16
    };


    /// Convert a register into an index between [0, 14].
    unsigned register_to_index(dynamorio::reg_id_t reg) {
        return REG_TO_INDEX[register_manager::scale(reg, REG_64)];
    }


    /// Convert an operand size into an integer `i` such that `2^i` is the
    /// number of bytes required to represent an operand of that size.
    unsigned operand_size_order(operand_size size) {
        return SIZE_TO_ORDER[size];
    }
}}

