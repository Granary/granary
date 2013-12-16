/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * pgo.cc
 *
 *  Created on: 2013-12-16
 *      Author: Peter Goodman
 */

#include "granary/pgo.h"


/// Turn off profile-guided optimisation when using the `cfg` tool, or when
/// not doing kernel instrumentation.
#if defined(CLIENT_CFG) || !CONFIG_ENV_KERNEL
#   undef CONFIG_OPTIMISE_PGO
#   define CONFIG_OPTIMISE_PGO 0
#endif


namespace granary {
    struct cti_info {
        const uint32_t key;
        const uint32_t value;
    };
}


/// Defined in the Makefile if `GR_PGO_PROFILE` specifies a profile file to
/// Granary.
#if CONFIG_OPTIMISE_PGO
#   include "granary/gen/kernel_profile.cc"
#endif

namespace granary {


#if CONFIG_OPTIMISE_PGO
    /// Binary search one of the arrays of CTI infos for a value.
    static uintptr_t search(
        const cti_info *array,
        const long max,
        uint32_t search_key
    ) throw() {
        long first(0);
        long last(max - 1);
        long middle((first + last) / 2);

        for(; first <= middle && middle <= last; ) {
            const cti_info info(array[middle]);

            if(info.key == search_key) {
                return info.value;
            } else if(info.key < search_key) {
                first = middle + 1;
            } else {
                last = middle - 1;
            }

            middle = (first + last) / 2;
        }

        return 0;
    }


    enum : uintptr_t {
        LINUX_KERNEL_BASE = 0xffffffff80000000ULL
    };


    const int REVERSE_OPCODES[] = {
        dynamorio::OP_jno,
        dynamorio::OP_jo,
        dynamorio::OP_jnb,
        dynamorio::OP_jb,
        dynamorio::OP_jnz,
        dynamorio::OP_jz,
        dynamorio::OP_jnbe,
        dynamorio::OP_jbe,
        dynamorio::OP_jns,
        dynamorio::OP_js,
        dynamorio::OP_jnp,
        dynamorio::OP_jp,
        dynamorio::OP_jnl,
        dynamorio::OP_jl,
        dynamorio::OP_jnle,
        dynamorio::OP_jle
    };


    const int REVERSE_OPCODES_SHORT[] = {
        dynamorio::OP_jno_short,
        dynamorio::OP_jo_short,
        dynamorio::OP_jnb_short,
        dynamorio::OP_jb_short,
        dynamorio::OP_jnz_short,
        dynamorio::OP_jz_short,
        dynamorio::OP_jnbe_short,
        dynamorio::OP_jbe_short,
        dynamorio::OP_jns_short,
        dynamorio::OP_js_short,
        dynamorio::OP_jnp_short,
        dynamorio::OP_jp_short,
        dynamorio::OP_jnl_short,
        dynamorio::OP_jl_short,
        dynamorio::OP_jnle_short,
        dynamorio::OP_jle_short
    };
#endif


    /// Optimise an indirect CTI into a direct CTI. This will potentially add
    /// instructions before `in` inside of `ls` that test the target of the CTI
    /// against a known target, and if they match, then jump to that target.
    void profile_optimise_indirect_cti(
        instruction_list &ls,
        instruction in
    ) throw() {
        UNUSED(ls);
        UNUSED(in);
    }


    /// Optimise a conditional branch using profile-guided optimisation. This
    /// function either returns `next_pc` if the fall-through of the Jcc is the
    /// most likely target, or switches the Jcc instruction in place, and
    /// returns its old target as the fall-through. The purpose of this is to
    /// tie in nicely with Granary's ahead-of-time tracing infrastructure.
    app_pc profile_optimise_jcc(
        instruction in,
        app_pc block_start_pc,
        app_pc next_pc
    ) throw() {
#if CONFIG_OPTIMISE_PGO
        const uintptr_t block_start_offset(
            reinterpret_cast<uintptr_t>(block_start_pc) - LINUX_KERNEL_BASE);

        const uintptr_t jcc_target_addr(LINUX_KERNEL_BASE + search(
            CONDITIONAL_CTIS,
            NUM_CONDITIONAL_CTIS,
            static_cast<uint32_t>(block_start_offset)));

        // Don't have a profile entry for this Jcc.
        if(LINUX_KERNEL_BASE == jcc_target_addr) {
            return next_pc;
        }

        app_pc jcc_target(reinterpret_cast<app_pc>(jcc_target_addr));

        // The fall-through is the most often taken target.
        if(jcc_target == next_pc) {
            return next_pc;

        // The Jcc is most often taken. Need to negate the Jcc and sanity check
        // the target.
        } else {
            operand target(in.cti_target());

            ASSERT(target.value.pc == jcc_target);

            // Change the target to be the old fall-through.
            target.value.pc = next_pc;
            in.set_cti_target(target);

            // Reverse the opcode.
            if(in.instr->opcode >= dynamorio::OP_jo
            && in.instr->opcode <= dynamorio::OP_jnle) {
                in.instr->opcode = REVERSE_OPCODES[
                    in.instr->opcode - dynamorio::OP_jo];
            } else {
                in.instr->opcode = REVERSE_OPCODES_SHORT[
                    in.instr->opcode - dynamorio::OP_jo_short];
            }

            // Return the new fall-through as the old target.
            return jcc_target;
        }

#else
        UNUSED(block_start_pc);
        UNUSED(in);
        return next_pc;
#endif
    }

}

