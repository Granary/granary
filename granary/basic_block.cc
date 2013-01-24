/*
 * bb.cc
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#include "granary/basic_block.h"
#include "granary/instruction.h"
#include "granary/state.h"
#include "granary/policy.h"
#include "granary/mangle.h"

#include <cstring>
#include <new>


namespace granary {


    enum {

        BB_PADDING      = 0xCC,
        BB_MAGIC        = 0xCCAACC,
        BB_MAGIC_MASK   = 0xFFFFFF00,

        /// number of byte states (bit pairs) per byte, i.e. we have a 4-to-1
        /// compression ratio of the instruction bytes to the state set bytes
        BB_BYTE_STATES_PER_BYTE = 4,

        /// byte and 32-bit alignment of basic block info structs in memory
        BB_INFO_BYTE_ALIGNMENT = 8,
        BB_INFO_INT32_ALIGNMENT = 2,

        /// misc.
        BITS_PER_BYTE = 8,
        BITS_PER_STATE = BITS_PER_BYTE / BB_BYTE_STATES_PER_BYTE,
        BITS_PER_QWORD = BITS_PER_BYTE * 8,

        /// aligned state size
        STATE_SIZE_ = sizeof(basic_block_state),
        STATE_SIZE = STATE_SIZE_ + ALIGN_TO(STATE_SIZE_, 16),

        /// an estimation of the number of bytes in a direct branch slot; each
        /// slot only requires 10 bytes, but we add 4 extra for any needed
        /// padding.
        BRANCH_SLOT_SIZE_ESTIMATE = 14,

        /// Size in bytes of a vtable entry. This is 8 bytes--the size to hold
        /// an address.
        VTABLE_ENTRY_SIZE = 8,

        /// Size of a far jmp.
        CONNECTING_JMP_SIZE = 5
    };


    /// Get the state of a byte in the basic block.
    static code_cache_byte_state get_state(uint8_t *pc_byte_states,
            unsigned i) throw() {
        const unsigned j(i / BB_BYTE_STATES_PER_BYTE); // byte offset
        const unsigned k(2 * (i % BB_BYTE_STATES_PER_BYTE)); // bit offset
        return static_cast<code_cache_byte_state>(
                1 << ((pc_byte_states[j] >> k) & 0x03));
    }


    /// Set one of the states into a trit.
    static void set_state(uint8_t *pc_byte_states,
            unsigned i,
            code_cache_byte_state state) throw() {

        static uint8_t state_to_mask[] = {
                0xFF,
                0,  /* BB_BYTE_NATIVE:          1 << 0 */
                1,  /* BB_BYTE_MANGLED:         1 << 1 */
                0xFF,
                2,  /* BB_BYTE_INSTRUMENTED:    1 << 2 */
                0xFF,
                0xFF,
                0xFF,
                3   /* BB_BYTE_PADDING:         1 << 3 */
        };

        const unsigned j(i / BB_BYTE_STATES_PER_BYTE); // byte offset
        const unsigned k(2 * (i % BB_BYTE_STATES_PER_BYTE)); // bit offset
        pc_byte_states[j] &= ~(0x3 << k);
        pc_byte_states[j] |= state_to_mask[state] << k;
    }


    /// Get the state of all bytes of an instruction.
    static code_cache_byte_state
    get_instruction_state(const instruction in) throw() {

        if(nullptr != in.pc()) {
            return code_cache_byte_state::BB_BYTE_NATIVE;

        // here we take over the INSTR_HAS_CUSTOM_STUB to represent a mangled
        // instruction. Mangling in Granary's case has to do with a jump to a
        // specific basic block that can resolve what to do.
        } else if(in.is_mangled()) {
            return code_cache_byte_state::BB_BYTE_MANGLED;

        } else {
            return code_cache_byte_state::BB_BYTE_INSTRUMENTED;
        }
    }


    /// Set the state of a pair of bits in memory.
    static unsigned initialise_state_bytes(basic_block_info *info,
                                              instruction_list &ls,
                                              app_pc pc) throw() {

        unsigned num_instruction_bits(info->num_bytes * BITS_PER_STATE);
        num_instruction_bits += ALIGN_TO(num_instruction_bits, BITS_PER_QWORD);
        unsigned num_state_bytes = num_instruction_bits / BITS_PER_BYTE;

        // initialise all state bytes to have their states as padding bytes
        memset((void *) pc, static_cast<int>(~0U), num_state_bytes);

        instruction_list_handle in(ls.first());
        unsigned byte_offset(0);

        // for each instruction
        for(unsigned i = 0, max = ls.length(); i < max; ++i) {
            code_cache_byte_state state(get_instruction_state(*in));
            unsigned num_bytes(in->encoded_size());

            // for each byte of each instruction
            for(unsigned j = i; j < (i + num_bytes); ++j) {
                set_state(pc, j, state);
            }

            byte_offset += num_bytes;
            in = in.next();
        }

        return num_state_bytes;
    }


    /// Return the meta information for the current basic block, given some
    /// pointer into the instructions of the basic block.
    basic_block::basic_block(app_pc current_pc_) throw()
        : cache_pc_current(current_pc_)
    {

        // round up to the next 8-byte aligned quantity
        uint64_t addr(reinterpret_cast<uint64_t>(current_pc_));
        unsigned num_bytes = ALIGN_TO(addr, BB_INFO_BYTE_ALIGNMENT);
        addr += num_bytes;

        // go search for the basic block info by finding potential
        // addresses of the meta info, then checking their magic values.
        for(uint64_t *ints(reinterpret_cast<uint64_t *>(addr)); ; ++ints) {
            const basic_block_info * const potential_bb(
                    unsafe_cast<basic_block_info *>(ints));

            if(BB_MAGIC == potential_bb->magic) {
                break;
            }

            num_bytes += BB_INFO_BYTE_ALIGNMENT;
        }

        // we've found the basic block info
        info = unsafe_cast<basic_block_info *>(current_pc_ + num_bytes);
        cache_pc_end = cache_pc_current + num_bytes;
        cache_pc_start = cache_pc_end - info->num_bytes;

        IF_KERNEL(pc_byte_states = reinterpret_cast<uint8_t *>(
                cache_pc_end + sizeof(basic_block_info));)
    }


#if GRANARY_IN_KERNEL
    /// Returns the next safe interrupt location. This is used in the event that
    /// a basic block is interrupted and we need to determine if we have to
    /// delay the interrupt (e.g. across potentially non-reentrant
    /// instrumentation code) or if we can take the interrupt immediately.
    app_pc basic_block::next_safe_interrupt_location(void) const throw() {
        app_pc next(cache_pc_current);

        for(;;) {
            const code_cache_byte_state byte_state(get_state(
                    pc_byte_states, next - cache_pc_start));

            // we're in front of a native byte, which is fine.
            if(code_cache_byte_state::BB_BYTE_NATIVE & byte_state) {
                return next;

            // In the case of a mangled instruction, we will follow the discipline
            // that there can only be one mangled instruction in a translated
            // fragment, which is a safe interrupt point.
            //
            // otherwise, we should be in a different kind of basic block
            // altogether, and the interrupt policy should be handled where we
            // have that context.
            } else if(code_cache_byte_state::BB_BYTE_MANGLED & byte_state) {
                if(basic_block_kind::BB_TRANSLATED_FRAGMENT == info->kind) {
                    return next;
                }

                return nullptr;

            // we might need to delay, but only if the previous byte is instrumented
            // if the previous byte is instrumented, then it likely means we're in
            // a sequence of instrumented instruction, which might not be re-entrant
            // and so we need to advance the pc to find a safe interrupt location.
            } else if(code_cache_byte_state::BB_BYTE_INSTRUMENTED & byte_state) {

                if(next == cache_pc_start) {
                    return next;
                }

                const code_cache_byte_state prev_byte_state(get_state(
                        pc_byte_states, next - cache_pc_start - 1));

                if(code_cache_byte_state::BB_BYTE_NATIVE == prev_byte_state) {
                    return next;
                }

                ++next;

            // this is bad; we should never be executing padding instructions
            // (which are interrupts)
            } else {
                return nullptr;
            }
        }

        return nullptr; // shouldn't be reached
    }
#endif


    /// Compute the size of a basic block given an instruction list. This
    /// computes the size of each instruction, the amount of padding, meta
    /// information, etc.
    unsigned basic_block::size(instruction_list &ls) throw() {
        unsigned size(ls.encoded_size());

        // alignment and meta info
        size += ALIGN_TO(size, BB_INFO_BYTE_ALIGNMENT);
        size += sizeof(basic_block_info); // meta info

#if GRANARY_IN_KERNEL
        // counting set for the bits that have state info about the instruction
        // bytes
        unsigned num_instruction_bits(size * BITS_PER_STATE);
        num_instruction_bits += ALIGN_TO(num_instruction_bits, BITS_PER_QWORD);
        size += num_instruction_bits / BITS_PER_BYTE;
#endif

        return size;
    }


    /// Compute the size of an existing basic block.
    ///
    /// Note: This excludes block-local storage because it is actually
    ///       allocated separately from the basic block.
    unsigned basic_block::size(void) const throw() {
        unsigned size(info->num_bytes + sizeof *info);

#if GRANARY_IN_KERNEL
        unsigned num_instruction_bits(info->num_bytes * BITS_PER_STATE);
        num_instruction_bits += ALIGN_TO(num_instruction_bits, BITS_PER_QWORD);
        size += num_instruction_bits / BITS_PER_BYTE;
#endif
        return size;
    }


#if CONFIG_BB_PATCH_LOCAL_BRANCHES
    /// Get a handle for an instruction in the instruction list.
    static instruction_list_handle
    find_local_back_edge_target(instruction_list &ls, app_pc pc) throw() {
        instruction_list_handle in(ls.first());
        for(unsigned i(0), max(ls.length()); i < max; ++i) {
            if(in->pc() == pc) {
                return in;
            }
            in = in.next();
        }
        return instruction_list_handle();
    }
#endif


    /// Represents a potential local branch target, and the instruction
    /// that would need to be resolved if we can find the target within
    /// the current basic block.
    struct local_branch_target {
        app_pc target;
        instruction_list_handle source;

        bool operator<(const local_branch_target &that) const throw() {
            return target < that.target;
        }
    };


    /// Build an array of in-block branches and attempt to resolve their
    /// targets to local instructions. The net effect is to turn CTIs with pc
    /// targets into instructions with instr targets (so long as one of the
    /// instructions in the block has a pc that is targets by one of the CTIs).
    void resolve_local_branches(instrumentation_policy policy,
                                cpu_state_handle &cpu,
                                instruction_list &ls) throw() {
#if CONFIG_BB_PATCH_LOCAL_BRANCHES

        instruction_list_handle in(ls.first());
        const unsigned max(ls.length());

        // find the number of pc targets
        unsigned num_direct_branches(0);
        for(unsigned i(0); i < max; ++i) {
            if(in->is_cti()
            && in->policy() == policy
            && dynamorio::opnd_is_pc(in->cti_target())) {
                ++num_direct_branches;
            }
            in = in.next();
        }

        if(!num_direct_branches) {
            return;
        }

        /// build an array of targets
        array<local_branch_target> branch_targets(
            cpu->transient_allocator.\
                allocate_array<local_branch_target>(num_direct_branches));

        in = ls.first();
        for(unsigned i(0), j(0); i < max; ++i) {
            if(in->is_cti() && policy == in->policy()) {
                operand target(in->cti_target());
                if(dynamorio::opnd_is_pc(target)) {
                    branch_targets[j].target = target;
                    branch_targets[j].source = in;
                    ++j;
                }
            }
            in = in.next();
        }

        // sort targets by their destination (target pc)
        branch_targets.sort();

        // re-target the instructions
        in = ls.first();
        for(unsigned i(0), j(0); i < max && j < num_direct_branches; ++i) {
            while(j < num_direct_branches
               && in->pc() == branch_targets[j].target
               && in->policy() == policy) {

                dynamorio::instr_set_target(
                    *(branch_targets[j++].source), instr_(*in));
            }
            in = in.next();
        }
#else
       (void) cpu;
       (void) ls;
#endif
    }


    /// Translate any loop instructions into a 3-instruction form, and return
    /// the number of loop instructions found. This is done here instead of
    /// at mangle time so that we can potentially benefit from `resolve_local_
    /// branches`.
    ///
    /// This turns an instruction like `jecxz <foo>` or `loop <foo>` into:
    ///         jmp <try_loop>
    /// do_loop:jmp <foo>
    ///         loop <do_loop>
    static unsigned translate_loops(instruction_list &ls) throw() {

        instruction_list_handle in(ls.first());
        const unsigned max(ls.length());
        unsigned num_loops(0);

        for(unsigned i(0); i < max; ++i) {
            if(dynamorio::instr_is_cti_loop(*in)) {
                ++num_loops;

                operand target(in->cti_target());
                instruction_list_handle in_first(ls.insert_before(in,
                    jmp_(instr_(*in))));
                instruction_list_handle in_second(ls.insert_before(in,
                    jmp_(target)));

                in_first->set_pc(in->pc());
                in->set_cti_target(instr_(*in_second));
                in->set_pc(nullptr);
                in->set_mangled();
            }

            in = in.next();
        }

        return num_loops;
    }


    /// Decode and translate a single basic block of application/module code.
    basic_block basic_block::translate(instrumentation_policy policy,
                                       cpu_state_handle &cpu,
                                       thread_state_handle &thread,
                                       app_pc *pc) throw() {

        const app_pc start_pc(*pc);
        uint8_t *generated_pc(nullptr);
        instruction_list ls;
        unsigned num_direct_branches(0);
        bool fall_through_pc(false);

        basic_block_state *block_storage(
            cpu->fragment_allocator.allocate<basic_block_state>());

        for(;;) {
            instruction in(instruction::decode(pc));

            if(in.is_cti()) {
                operand target(in.cti_target());

#if CONFIG_BB_PATCH_LOCAL_BRANCHES
                // direct branch (e.g. un/conditional branch, jmp, call)
                if(dynamorio::opnd_is_pc(target)) {
                    app_pc target_pc(dynamorio::opnd_get_pc(target));

                    // if it is local back edge then keep control within the
                    // same basic block. Note: the policy of this basic block
                    // must match the policy of the destination basic block.
                    if(start_pc <= target_pc
                    && target_pc < *pc
                    && policy == in.policy()) {
                        instruction_list_handle prev_in(
                            find_local_back_edge_target(ls, target_pc));

                        if(prev_in.is_valid()) {
                            target = instr_(*prev_in);
                            in.set_cti_target(target);
                        }
                    }
                }
#endif

                instruction_list_handle added_in(ls.append(in));

                // used to estimate how many direct branch slots we'll
                // need to add to the final basic block
                if(dynamorio::opnd_is_pc(target)) {
                    ++num_direct_branches;
                    added_in->set_patchable();
                }

                // it's an conditional jmp; terminate the basic block
                // it's a conditional jmp; terminate the basic block, and save
                // the next pc as a fall-through point.
                if(!dynamorio::instr_is_call(in)) {
                    if(dynamorio::instr_is_cbr(in)) {
#if CONFIG_BB_EXTEND_BBS_PAST_CBRS
                        continue;
#else
                        fall_through_pc = true;
#endif
                    }
                    break;
                }

            // between this block and next block we should disable interrupts.
            } else if(dynamorio::OP_cli == in->opcode) {
                ls.append(in);
                fall_through_pc = true; // TODO
                break;

            // between this block and next block, we should enable interrupts.
            } else if(dynamorio::OP_sti == in->opcode) {
                ls.append(in);
                fall_through_pc = true; // TODO
                break;

            // some other instruction
            } else {
                ls.append(in);
            }
        }

        // invoke client code instrumentation on the basic block; the client
        // might return a different instrumentation policy to use. The effect
        // of this is that if we are in policy P1, and the client returns policy
        // P2, then we will emit a block to P1's code cache that jumps us into
        // P2's code cache.
        instrumentation_policy client_policy(policy.instrument(
            cpu,
            thread,
            *block_storage,
            ls));

        // add in a trailing jump if the last instruction in the basic
        // block if we need to force a connection between this basic block
        // and the next.
        if(fall_through_pc) {
            instruction connecting_jmp(jmp_(pc_(*pc)));
            connecting_jmp.set_mangled();
            ls.append(connecting_jmp);
        }

        // translate loops and resolve local branches into jmps to instructions.
        // done before mangling, as mangling removes the opportunity to do this
        // type of transformation.
        num_direct_branches += translate_loops(ls);
        if(num_direct_branches) {
            resolve_local_branches(policy, cpu, ls);
        }

        basic_block_vtable vtable;

        // prepare the instructions for final execution; this does instruction-
        // specific translations needed to make the code sane/safe to run.
        instruction_list_mangler mangler(cpu, thread, client_policy, vtable);
        mangler.mangle(ls);

        // re-calculate the size and re-allocate; if our earlier
        // guess was too small then we need to re-instrument the
        // instruction list
        generated_pc = cpu->fragment_allocator.\
            allocate_array<uint8_t>(basic_block::size(ls));

        // allocated pc immediately following block-local storage
        uint8_t *bb_pc(generated_pc + vtable.size());

        return emit(BB_TRANSLATED_FRAGMENT, ls, start_pc, &bb_pc);
    }


    /// Emit an instruction list as code into a byte array. This will also
    /// emit the basic block meta information.
    ///
    /// Note: it is assumed that no field in *state points back to itself
    ///       or any other temporary storage location.
    basic_block basic_block::emit(basic_block_kind kind,
                                  instruction_list &ls,
                                  app_pc generating_pc,
                                  app_pc *generated_pc) throw() {

        app_pc pc = *generated_pc;
        pc += ALIGN_TO(reinterpret_cast<uint64_t>(pc), 16);

        // add in the instructions
        const app_pc start_pc(pc);
        pc = ls.encode(pc);

        uint64_t pc_uint(reinterpret_cast<uint64_t>(pc));
        app_pc pc_aligned(pc + ALIGN_TO(pc_uint, BB_INFO_BYTE_ALIGNMENT));

        // add in the padding
        for(; pc < pc_aligned; ) {
            *pc++ = BB_PADDING;
        }

        //BARRIER;

        basic_block_info *info(unsafe_cast<basic_block_info *>(pc));

        // fill in the info
        info->magic = static_cast<uint32_t>(BB_MAGIC);
        info->kind = kind;
        info->num_bytes = static_cast<unsigned>(pc - start_pc);
        info->hotness = 0;
        info->generating_pc = generating_pc;

        // fill in the byte state set
        pc += sizeof(basic_block_info);
        IF_KERNEL( pc += initialise_state_bytes(info, ls, pc); )

        *generated_pc = pc;
        return basic_block(start_pc);
    }
}


