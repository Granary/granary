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
#include "granary/list.h"
#include "granary/code_cache.h"
#include "granary/hash_table.h"

#if CONFIG_FEATURE_INSTRUMENT_PATCH_WRAPPERS
#   include "granary/detach.h"
#endif


// Make sure we've got the detach addresses.
#if !CONFIG_FEATURE_WRAPPERS
#   define WRAP_FOR_DETACH(func)
#   define WRAP_ALIAS(func, alias)
#   define DETACH(func)
#   define TYPED_DETACH(func)
#   include "granary/gen/kernel_detach.inc"
#   undef WRAP_ALIAS
#   undef WRAP_FOR_DETACH
#   undef DETACH
#   undef TYPED_DETACH
#endif


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


    /// Create a duplicate version of a function.
    static app_pc duplicate_function(const app_pc addr, int len) throw() {
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


    /// Create a fully instrumented version of the function in question.
    static app_pc instrument_function(const app_pc addr, int len) throw() {
        list<app_pc> process_bbs;
        list<app_pc> lookup_bbs;
        list<app_pc>::handle_type next;
        hash_set<app_pc> seen;
        instruction in;

        process_bbs.append(addr);

        // Traverse instructions and find all used registers.
        while(process_bbs.length()) {

            next = process_bbs.pop();
            app_pc bb(*next);

            if(seen.contains(bb)) {
                continue;
            }

            lookup_bbs.append(bb);
            seen.add(bb);

            for(; bb; ) {

                in.decode_update(&bb);

                // Done processing this basic block.
                if(dynamorio::OP_ret == in.op_code()
                || dynamorio::OP_ret_far == in.op_code()
                || dynamorio::OP_iret == in.op_code()
                || dynamorio::OP_sysret == in.op_code()
                || dynamorio::OP_swapgs == in.op_code()) {
                    break;
                }

                // Try to follow CTIs.
                if(in.is_cti()) {

                    // Assume that a CALL leaves the function.
                    if(in.is_call()) {
                        continue;
                    }

                    operand target(in.cti_target());

                    // Indirect JMP, RET, etc.
                    if(!dynamorio::opnd_is_pc(target)) {
                        continue;
                    }

                    // Add the target only if it stays within the current
                    // function and if we haven't processed it yet.
                    app_pc target_pc(dynamorio::opnd_get_pc(target));
                    if(addr <= target_pc
                    && target_pc < (addr + len)
                    && !seen.contains(target_pc)) {
                        process_bbs.append(target_pc);
                    }

                    // Terminates the basic block.
                    if(in.is_unconditional_cti()) {
                        break;

                    // Add the fall-through.
                    } else {
                        if(!seen.contains(bb)) {
                            process_bbs.append(bb);
                        }
                        break;
                    }
                }
            }
        }

        cpu_state_handle cpu;

        // Processes the basic blocks in the reverse order in which they were
        // discovered from within the original procedure. The hope is that
        // reverse order will be close to a post-order traversal, and that
        // the majority of jumps will be resolvable at translation time.
        while(lookup_bbs.length()) {
            next = lookup_bbs.pop();
            app_pc bb(*next);
            mangled_address am(bb, START_POLICY);
            cpu.free_transient_allocators();
            code_cache::find(cpu, am);
        }

        cpu.free_transient_allocators();
        mangled_address am(addr, START_POLICY);
        return code_cache::find(cpu, am);
    }


    /// Given a function at address `addr` that occupies no more than `len`
    /// contiguous bytes, copy the instructions directly from `addr` into a
    /// new buffer of the same size, re-relativize those instructions, and
    /// return a pointer to the new instructions.
    app_pc copy_and_rerelativize_function(const app_pc addr, int len) throw() {
        ASSERT(0 < len);

#if CONFIG_FEATURE_INSTRUMENT_PATCH_WRAPPERS
        if(reinterpret_cast<app_pc>(DETACH_ADDR_search_exception_tables) != addr) {
            return instrument_function(addr, len);
        }
#endif
        return duplicate_function(addr, len);
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

        buff_ptr = jmp_(pc_(new_address)).stage_encode(buff_ptr, old_address);
        int3_().stage_encode(buff_ptr, old_address);

        volatile uint64_t *hotpatch_target(unsafe_cast<uint64_t *>(old_address));
        *hotpatch_target = buff;
    }
}
