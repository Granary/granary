/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * hotpatch.cc
 *
 *  Created on: 2013-06-27
 *      Author: Peter Goodman
 */

#include "granary/globals.h"
#include "granary/state.h"
#include "granary/instruction.h"


extern "C" {
    void kernel_make_memory_writeable(void *addr);
}


namespace granary {


    /// Find a far memory operand and its size. If we've already found one in
    /// this instruction then don't repeat the check.
    static void find_relative_memory_operand(
        const operand_ref op,
        bool &has_far_op
    ) throw() {
        if(has_far_op || dynamorio::REL_ADDR_kind != op->kind) {
            return;
        }

        has_far_op = true;
    }


    /// Given a function at address `addr` that occupies no more than `len`
    /// contiguous bytes, copy the instructions directly from `addr` into a
    /// new buffer of the same size, re-relativize those instructions, and
    /// return a pointer to the new instructions.
    app_pc copy_and_rerelativize_function(const app_pc addr, int len) throw() {
        app_pc pc(addr);
        const app_pc end_addr(addr + len);
        const int new_len(len + ALIGN_TO(len, 16));

        app_pc new_addr(global_state::FRAGMENT_ALLOCATOR->
            allocate_array<uint8_t>(new_len));

        memcpy(new_addr, addr, len);

        instruction in;
        for(; nullptr != pc && 0 < len; ) {

            in.decode_update(&pc, DECODE_DONT_WIDEN_CTI);
            len -= in.encoded_size();

            if(dynamorio::OP_INVALID == in.op_code()
            || dynamorio::OP_UNDECODED == in.op_code()) {
                continue;
            }

            app_pc new_pc(new_addr + (in.instr->translation - addr));

            // Fix up %rip-relative CTI targets.
            if(in.is_cti()) {
                operand target(in.cti_target());
                if(!dynamorio::opnd_is_pc(target)) {
                    continue;
                }

                // Re-direct the relative address.
                if(addr <= target.value.pc && target.value.pc < end_addr) {
                    in.instr->u.o.src0.value.pc = new_addr + (target.value.pc - addr);
                }

                // Encode the instruction in place at its new location.
                in.invalidate_raw_bits();
                in.encode(new_pc);

            // Fix up %rip-relative memory addresses.
            } else {
                bool has_far_op(false);
                in.for_each_operand(find_relative_memory_operand, has_far_op);

                if(has_far_op) {
                    in.invalidate_raw_bits();
                    in.encode(new_pc);
                }
            }
        }

        return new_addr;
    }


    /// Prepare to redirect a function.
    void prepare_redirect_function(app_pc old_address) throw() {
        kernel_make_memory_writeable(old_address);
    }


    /// Hot-patch a kernel function.
    void redirect_function(app_pc old_address, app_pc new_address) throw() {

        ASSERT(0 == (reinterpret_cast<uintptr_t>(old_address) % 8));


        uint64_t buff(0);
        uint8_t *buff_ptr(unsafe_cast<uint8_t *>(&buff));
        instruction in(jmp_(pc_(new_address)));
        in.stage_encode(buff_ptr, old_address);

        volatile uint64_t *hotpatch_target(unsafe_cast<uint64_t *>(old_address));
        *hotpatch_target = buff;
    }
}
