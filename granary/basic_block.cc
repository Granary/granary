/*
 * bb.cc
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#include "granary/globals.h"
#include "granary/basic_block.h"
#include "granary/instruction.h"
#include "granary/state.h"
#include "granary/policy.h"
#include "granary/mangle.h"
#include "granary/emit_utils.h"
#include "granary/code_cache.h"

#include <cstring>
#include <new>


namespace granary {


    enum {

        BB_PADDING      = 0xEA,
        BB_PADDING_LONG = 0xEAEAEAEAEAEAEAEA,
        BB_ALIGN        = 16,

        /// Maximum size in bytes of a decoded basic block. This relates to
        /// *decoding* only and not the resulting size of a basic block after
        /// translation.
        BB_MAX_SIZE_BYTES = (PAGE_SIZE / 4),

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
    };


    /// Get the state of a byte in the basic block.
    inline static code_cache_byte_state
    get_state(uint8_t *states, unsigned i) throw() {
        enum {
            MASK = (BB_BYTE_STATES_PER_BYTE - 1)
        };
        const unsigned j(i / BB_BYTE_STATES_PER_BYTE); // byte offset
        const unsigned k(2 * (i % BB_BYTE_STATES_PER_BYTE)); // bit offset
        return static_cast<code_cache_byte_state>(
                1 << ((states[j] >> k) & MASK));
    }


    /// Set one of the states into a counting set.
    static void set_state(
        uint8_t *pc_byte_states,
        unsigned i,
        code_cache_byte_state state
    ) throw() {

        enum {
            MASK = (BB_BYTE_STATES_PER_BYTE - 1)
        };

        static uint8_t state_to_mask[] = {
            0xFF,
            0,  /* BB_BYTE_NATIVE:      1 << 0 */
            1,  /* BB_BYTE_DELAY_BEGIN: 1 << 1 */
            0xFF,
            2,  /* BB_BYTE_DELAY_CONT:  1 << 2 */
            0xFF,
            0xFF,
            0xFF,
            3   /* BB_BYTE_DELAY_END:   1 << 3 */
        };

        const unsigned j(i / BB_BYTE_STATES_PER_BYTE); // byte offset
        const unsigned k(2 * (i % BB_BYTE_STATES_PER_BYTE)); // bit offset
        pc_byte_states[j] &= ~(MASK << k);
        pc_byte_states[j] |= state_to_mask[state] << k;
    }


    /// Get the state of all bytes of an instruction.
    static code_cache_byte_state
    get_instruction_state(
        const instruction in,
        code_cache_byte_state prev_state
    ) throw() {

        uint8_t flags(in.instr->granary_flags);

        enum {
            SINGLETON = instruction::DELAY_BEGIN | instruction::DELAY_END
        };

        // single delayed instruction; makes no sense. It is considered native.
        if(SINGLETON == (flags & SINGLETON)) {
            return code_cache_byte_state::BB_BYTE_NATIVE;

        } else if(flags & instruction::DELAY_BEGIN) {
            ASSERT(!(code_cache_byte_state::BB_BYTE_DELAY_CONT & prev_state));
            ASSERT(!(code_cache_byte_state::BB_BYTE_DELAY_BEGIN & prev_state));

            return code_cache_byte_state::BB_BYTE_DELAY_BEGIN;

        } else if(flags & instruction::DELAY_END) {
            ASSERT(!(code_cache_byte_state::BB_BYTE_DELAY_END & prev_state));
            ASSERT(!(code_cache_byte_state::BB_BYTE_NATIVE & prev_state));

            return code_cache_byte_state::BB_BYTE_DELAY_END;

        } else if(code_cache_byte_state::BB_BYTE_DELAY_BEGIN == prev_state) {
            return code_cache_byte_state::BB_BYTE_DELAY_CONT;

        } else if(code_cache_byte_state::BB_BYTE_DELAY_CONT == prev_state) {
            return code_cache_byte_state::BB_BYTE_DELAY_CONT;

        } else {
            return code_cache_byte_state::BB_BYTE_NATIVE;
        }
    }


    /// Set the state of a pair of bits in memory.
    static unsigned
    initialise_state_bytes(
        basic_block_info *info,
        instruction in,
        app_pc pc
    ) throw() {

        unsigned num_instruction_bits(
            (info->num_bytes - info->num_patch_bytes) * BITS_PER_STATE);
        num_instruction_bits += ALIGN_TO(num_instruction_bits, BITS_PER_QWORD);
        unsigned num_state_bytes = num_instruction_bits / BITS_PER_BYTE;

        // initialise all state bytes to have their states as native
        memset(pc, 0, num_state_bytes);

        unsigned byte_offset(0);
        unsigned prev_byte_offset(0);

        // for each instruction
        code_cache_byte_state prev_state(BB_BYTE_NATIVE);
        unsigned num_delayed_instructions(0);
        unsigned prev_num_delayed_instructions(0);

        for(; in; in = in.next()) {

            code_cache_byte_state state(get_instruction_state(in, prev_state));
            code_cache_byte_state stored_state(state);

            unsigned num_bytes(in.encoded_size());

            if(BB_BYTE_NATIVE != stored_state) {
                if(num_bytes) {
                    ++num_delayed_instructions;
                }
                stored_state = BB_BYTE_DELAY_CONT;
            } else {
                prev_num_delayed_instructions = num_delayed_instructions;
                num_delayed_instructions = 0;
            }

            // the last delayed block was only one instruction long; clear its
            // state bits so that it is not in fact delayed (as that makes
            // no sense).
            if(1 == prev_num_delayed_instructions) {
                for(unsigned j(prev_byte_offset); j < byte_offset; ++j) {
                    set_state(pc, j, BB_BYTE_NATIVE);
                }
            }

            // for each byte of each instruction
            for(unsigned j(byte_offset); j < (byte_offset + num_bytes); ++j) {
                set_state(pc, j, stored_state);
            }

            // update so that we have byte accuracy on begin/end.
            if(BB_BYTE_DELAY_BEGIN == state) {
                set_state(pc, byte_offset, BB_BYTE_DELAY_BEGIN);
            } else if(BB_BYTE_DELAY_END == state) {
                set_state(
                    pc,
                    byte_offset + num_bytes - 1,
                    BB_BYTE_DELAY_END);
            }

            prev_byte_offset = byte_offset;
            byte_offset += num_bytes;
            prev_state = state;
        }

        return num_state_bytes;
    }


    /// Return the meta information for the current basic block, given some
    /// pointer into the instructions of the basic block.
    basic_block::basic_block(app_pc current_pc_) throw()
        : info(nullptr)
        , policy()
        , cache_pc_start(nullptr)
        , cache_pc_current(current_pc_)
        , cache_pc_end(nullptr)
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

            if(basic_block_info::HEADER == potential_bb->magic) {
                break;
            }

            num_bytes += BB_INFO_BYTE_ALIGNMENT;
        }

        // we've found the basic block info
        info = unsafe_cast<basic_block_info *>(current_pc_ + num_bytes);
        cache_pc_end = cache_pc_current + num_bytes;
        cache_pc_start = cache_pc_end - info->num_bytes + info->num_patch_bytes;
        policy = instrumentation_policy::from_extension_bits(info->policy_bits);

        IF_KERNEL(pc_byte_states = reinterpret_cast<uint8_t *>(
            cache_pc_end + sizeof(basic_block_info));)
    }


#if GRANARY_IN_KERNEL
    /// Returns true iff this interrupt must be delayed. If the interrupt
    /// must be delayed then the arguments are updated in place with the
    /// range of code that must be copied and re-relativised in order to
    /// safely execute the interruptible code. The range of addresses is
    /// [begin, end), where the `end` address is the next code cache address
    /// to execute after the interrupt has been handled.
    bool basic_block::get_interrupt_delay_range(
        app_pc &begin,
        app_pc &end
    ) const throw() {

        const unsigned current_offset(cache_pc_current - cache_pc_start);
        code_cache_byte_state byte_state(get_state(
            pc_byte_states, current_offset));

        enum {
            SAFE_INTERRUPT_STATE = BB_BYTE_NATIVE | BB_BYTE_DELAY_BEGIN
        };

        if(SAFE_INTERRUPT_STATE & byte_state) {
            begin = nullptr;
            end = nullptr;
            return false;
        }

        for(unsigned i(current_offset); ; ++i) {
            if(BB_BYTE_DELAY_END == get_state(pc_byte_states, i)) {
                end = cache_pc_start + i + 1;
                break;
            }
        }

        for(unsigned i(0); i < current_offset; ++i) {
            if(SAFE_INTERRUPT_STATE & get_state(pc_byte_states, i)) {
                begin = cache_pc_start + i;
            }
        }

        return true;
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
        unsigned size_(info->num_bytes + sizeof *info);

#if GRANARY_IN_KERNEL
        unsigned num_instruction_bits(info->num_bytes * BITS_PER_STATE);
        num_instruction_bits += ALIGN_TO(num_instruction_bits, BITS_PER_QWORD);
        size_ += num_instruction_bits / BITS_PER_BYTE;
#endif
        return size_;
    }


#if CONFIG_BB_PATCH_LOCAL_BRANCHES
    /// Get a handle for an instruction in the instruction list.
    static instruction
    find_local_back_edge_target(instruction_list &ls, app_pc pc) throw() {
        instruction in(ls.first());
        for(unsigned i(0), max(ls.length()); i < max; ++i) {
            if(in.pc() == pc) {
                return in;
            }
            in = in.next();
        }
        return instruction(nullptr);
    }
#endif


    /// Represents a potential local branch target, and the instruction
    /// that would need to be resolved if we can find the target within
    /// the current basic block.
    struct local_branch_target {
        app_pc target;
        instruction source;

        bool operator<(const local_branch_target &that) const throw() {
            return target < that.target;
        }
    };


    /// Build an array of in-block branches and attempt to resolve their
    /// targets to local instructions. The net effect is to turn CTIs with pc
    /// targets into instructions with instr targets (so long as one of the
    /// instructions in the block has a pc that is targets by one of the CTIs).
    void resolve_local_branches(
        instrumentation_policy policy,
        cpu_state_handle &cpu,
        instruction_list &ls
    ) throw() {
#if CONFIG_BB_PATCH_LOCAL_BRANCHES

        instruction in(ls.first());
        const unsigned max(ls.length());

        // find the number of pc targets
        unsigned num_direct_branches(0);
        for(unsigned i(0); i < max; ++i) {
            if(in.is_cti()
            && (!in.policy() || in.policy() == policy)
            && dynamorio::opnd_is_pc(in.cti_target())) {
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
            if(in.is_cti() && policy == in.policy()) {
                operand target(in.cti_target());
                if(dynamorio::opnd_is_pc(target)) {
                    branch_targets[j].target = target;
                    branch_targets[j].source = in;
                    ++j;
                }
            }
            in = in.next();
        }

        // sort targets by their destination (target pc)
#       error "Find an alternative to sort, e.g. using simple hash tables, as" \
              "sorting requires a dependency on <algorithm>, which uses " \
              "floating point (and hence sse / mmx)."
        branch_targets.sort();

        // re-target the instructions
        in = ls.first();
        for(unsigned i(0), j(0); i < max && j < num_direct_branches; ++i) {
            while(j < num_direct_branches
               && in.pc() == branch_targets[j].target
               && in.policy() == policy) {

                dynamorio::instr_set_target(
                    (branch_targets[j++].source), instr_(in));
            }
            in = in.next();
        }
#else
        (void) policy;
        (void) cpu;
        (void) ls;
#endif
    }


#if CONFIG_PRE_MANGLE_REP_INSTRUCTIONS
    /// Get the counter operand for a REP instruction.
    operand rep_counter(instruction in) throw() {
        return in.instr->u.o.dsts[in.instr->num_dsts - 1];
    }


    /// Create a jrcxz/jecxz/jcxz instruction for operating on a REP-prefixed
    /// instruction.
    instruction rep_jrcxz_(instruction label, const operand &counter) throw() {
        switch(dynamorio::opnd_get_size(counter)) {
        case dynamorio::OPSZ_8:
            return mangled(jrcxz_(instr_(label)));
        case dynamorio::OPSZ_4:
            return mangled(jecxz_(instr_(label)));
        default:
            return mangled(jcxz_(instr_(label)));
        }
    }
#endif


    /// Translate any loop instructions into a 3- or 4-instruction form, and
    /// return the number of loop instructions found. This is done here instead
    /// of at mangle time so that we can potentially benefit from
    /// `resolve_local_branches`. Also, we do this before mangling so that if
    /// complex memory-operand-related instrumentation is needed, then that
    /// instrumentation does not have to special-case instructions with a REP
    /// prefix.
    static unsigned translate_loops(instruction_list &ls) throw() {

        instruction in(ls.first());
        instruction next_in;
        instruction try_skip;
        unsigned num_loops(0);

        for(; in.is_valid(); in = next_in) {
            next_in = in.next();

            switch(in.op_code()) {

            // This turns an instruction like `jecxz <foo>` or `loop <foo>` into:
            //              jmp <try_loop>
            // do_loop:     jmp <foo>
            // try_loop:    loop <do_loop>
            case dynamorio::OP_loopne:
            case dynamorio::OP_loope:
            case dynamorio::OP_loop:
            case dynamorio::OP_jecxz: {
                ++num_loops;

                operand target(in.cti_target());
                instruction in_first(ls.insert_before(in,
                    mangled(jmp_(instr_(in)))));

                instruction in_second(ls.insert_before(in, jmp_(target)));

                in_first.set_pc(in.pc());
                in.set_cti_target(instr_(in_second));
                in.set_pc(nullptr);
                in.set_mangled();
                break;
            }

#if CONFIG_PRE_MANGLE_REP_INSTRUCTIONS
            // Based on ECX/RCX; these instructions are such that for each rep_
            // version, the non-rep version is OP_rep_* - 1.
            //
            // This converts something like `rep ins <...>` into:
            //
            // try_skip:    j[er]?cxz <done>
            //              jmp <do_instr>
            // done:        jmp <after>
            //              ins ...
            //              lea -1(%[er]?cx), %[er]?cx
            //              jmp <try_loop>
            // after:

            // after:
            case dynamorio::OP_rep_ins:
            case dynamorio::OP_rep_outs:
            case dynamorio::OP_rep_movs:
            case dynamorio::OP_rep_stos:
            case dynamorio::OP_rep_lods: {

                instruction after(label_());
                instruction done(mangled(jmp_(instr_(after))));
                instruction do_instr(label_());
                operand counter(rep_counter(in));

                try_skip = ls.insert_before(in, rep_jrcxz_(done, counter));
                ls.insert_before(in, mangled(jmp_(instr_(do_instr))));
                ls.insert_before(in, done);
                ls.insert_before(in, do_instr);

                ls.insert_after(in, after);
                ls.insert_before(after, lea_(counter, counter[-1]));
                ls.insert_before(after, mangled(jmp_(instr_(try_skip))));

                goto common_REP_tail;
            }

            // Based on condition codes. This converts something like
            // `repe cmps ...` into:
            //
            // try_skip:    j[er]cxz <skip>
            //              jmp <do_instr>
            // skip:        jmp <after>
            // do_instr:    cmps ....
            //              lea -1(%[er]cx), %[er]cx
            // try_loop:    j[er]cxz <after>
            //              jnz <after>
            //              jmp <do_instr>
            // after:
            case dynamorio::OP_rep_cmps:
            case dynamorio::OP_rep_scas: {
                instruction after(label_());
                instruction do_instr(label_());
                operand counter(rep_counter(in));
                instruction skip(jmp_(instr_(after)));

                try_skip = ls.insert_before(in, rep_jrcxz_(skip, counter));
                ls.insert_before(in, jmp_(instr_(do_instr)));
                ls.insert_before(in, skip);
                ls.insert_before(in, do_instr);

                ls.insert_after(in, after);
                ls.insert_before(after, lea_(counter, counter[-1]));
                ls.insert_before(after, rep_jrcxz_(after, counter));
                ls.insert_before(after, mangled(jnz_(instr_(after))));
                ls.insert_before(after, mangled(jmp_(instr_(do_instr))));

                goto common_REP_tail;
            }

            // Based on condition codes. This converts something like
            // `repne cmps ...` into:
            //
            // try_skip:    j[er]cxz <skip>
            //              jmp <do_instr>
            // skip:        jmp <after>
            // do_instr:    cmps ....
            //              lea -1(%[er]cx), %[er]cx
            // try_loop:    j[er]cxz <after>
            //              jz <after>
            //              jmp <do_instr>
            // after:
            case dynamorio::OP_repne_cmps:
            case dynamorio::OP_repne_scas: {
                instruction after(label_());
                instruction do_instr(label_());
                operand counter(rep_counter(in));
                instruction skip(jmp_(instr_(after)));

                try_skip = ls.insert_before(in, rep_jrcxz_(skip, counter));
                ls.insert_before(in, jmp_(instr_(do_instr)));
                ls.insert_before(in, skip);
                ls.insert_before(in, do_instr);

                ls.insert_after(in, after);
                ls.insert_before(after, lea_(counter, counter[-1]));
                ls.insert_before(after, rep_jrcxz_(after, counter));
                ls.insert_before(after, mangled(jz_(instr_(after))));
                ls.insert_before(after, mangled(jmp_(instr_(do_instr))));

                goto common_REP_tail;
            }

            common_REP_tail: {
                app_pc without_rep(in.pc() + 1);
                in.instr->flags = 0;
                in.replace_with(instruction::decode(&without_rep));
                in.instr->translation = nullptr;
                try_skip.instr->translation = without_rep;

                ++num_loops;
                break;
            }
#endif

            default: break;
            }
        }

        return num_loops;
    }


    /// Decode and translate a single basic block of application/module code.
    basic_block basic_block::translate(
        instrumentation_policy policy,
        cpu_state_handle &cpu,
        thread_state_handle &thread,
        app_pc start_pc
    ) throw() {

        app_pc local_pc(start_pc);
        app_pc *pc(&local_pc);
        uint8_t *generated_pc(nullptr);
        instruction_list ls;
        unsigned num_direct_branches(0);
        bool fall_through_pc(false);

        basic_block_state *block_storage(nullptr);
        if(basic_block_state::size()) {
            block_storage = cpu->block_allocator.allocate<basic_block_state>();
        }

#if CONFIG_TRACK_XMM_REGS
        bool uses_xmm(policy.is_in_xmm_context());
#endif

        bool fall_through_detach(false);
        bool detach_tail_call(false);
        app_pc detach_app_pc(unsafe_cast<app_pc>(detach));

        for(unsigned byte_len(0); ;) {

            // very big basic block; cut it short and connect it with a tail.
            // we do this test *before* decoding the next instruction because
            // `instruction::decode` modifies `pc`, and `pc` is used to
            // determine the fall-through target.
            if(byte_len > BB_MAX_SIZE_BYTES) {
                fall_through_pc = true;
                break;
            }

            instruction in(instruction::decode(pc));

            // TODO: curiosity.
            if(dynamorio::OP_INVALID == in.op_code()) {
                granary_fault();
                break;
            }

            byte_len += in.instr->length;

            if(in.is_cti()) {
                operand target(in.cti_target());

                // direct branch (e.g. un/conditional branch, jmp, call)
                if(dynamorio::opnd_is_pc(target)) {
                    app_pc target_pc(dynamorio::opnd_get_pc(target));

                    if(detach_app_pc == target_pc) {
                        fall_through_detach = true;
                        if(in.is_jump()) {
                            detach_tail_call = true;
                        }
                        break;

                    // a very simple optimisation to keep more code in a basic
                    // block is to inline the calls that will detach.
                    } else if(find_detach_target(target_pc)) {

                        if(in.is_call() || !in.is_unconditional_cti()) {
                            ls.append(in);
                            continue;
                        }

#if CONFIG_USE_ONLY_ONE_POLICY
                    // If only one policy is being used, then we can inline
                    // some direct CTIs, knowing that the policy for the target
                    // of a CTI should never be changed by the instrumenter.
                    //
                    // We treat user/kernel space slightly differently here
                    // because in kernel space, we can properly detect (from
                    // `code_cache::find`) that we are re-entering the code
                    // cache, but in user space we don't attempt such detection.
                    } else IF_USER( if(!in.is_call()) ) {
                        mangled_address am(target_pc, policy);
                        if(cpu->code_cache.find(am.as_address)) {
                            ls.append(in);
                            if(in.is_unconditional_cti()
                            IF_KERNEL( && !in.is_call() )) {
                                break;
                            } else {
                                continue;
                            }
                        }
#endif
                    }


#if CONFIG_BB_PATCH_LOCAL_BRANCHES

                    // if it is local back edge then keep control within the
                    // same basic block. Note: the policy of this basic block
                    // must match the policy of the destination basic block.
                    if(start_pc <= target_pc
                    && target_pc < *pc
                    && policy == in.policy()) {
                        instruction prev_in(
                            find_local_back_edge_target(ls, target_pc));

                        if(prev_in.is_valid()) {
                            target = instr_(prev_in);
                            in.set_cti_target(target);
                        }
                    }
#endif
                }

                instruction added_in(ls.append(in));

                // used to estimate how many direct branch slots we'll
                // need to add to the final basic block
                if(dynamorio::opnd_is_pc(target)) {
                    ++num_direct_branches;
                    added_in.set_patchable();
                }

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

                // we end basic blocks with function calls so that we can detect
                // when a return targets the code cache by looking for the magic
                // value that precedes basic block meta info.
                } else {
#if !CONFIG_ENABLE_DIRECT_RETURN
#   if CONFIG_ENABLE_WRAPPERS || !CONFIG_TRANSPARENT_RETURN_ADDRESSES
                    fall_through_pc = true;
#   endif
                    break;
#endif
                }

#if 0
            // between this block and next block we should disable interrupts.
            } else if(dynamorio::OP_cli == in.op_code()) {
                ls.append(in);
                fall_through_pc = true; // TODO
                break;

            // between this block and next block, we should enable interrupts.
            } else if(dynamorio::OP_sti == in.op_code()) {
                ls.append(in);
                fall_through_pc = true; // TODO
                break;
#endif

            // some other instruction
            } else {

#if CONFIG_TRACK_XMM_REGS
                // update the policy to be in an xmm context.
                if(!uses_xmm && dynamorio::instr_is_sse_or_sse2(in)) {
                    policy.in_xmm_context();
                    uses_xmm = true;
                }
#endif

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

        client_policy.inherit_properties(policy);

        // add in a trailing jump if the last instruction in the basic
        // block if we need to force a connection between this basic block
        // and the next.
        if(fall_through_pc) {
            ls.append(jmp_(pc_(*pc)));

        /// Add in a trailing (emulated) jmp or ret if we are detaching.
        } else if(fall_through_detach) {
            if(detach_tail_call) {
                ls.append(mangled(ret_()));
            } else {
                insert_cti_after(
                    ls, ls.last(), *pc, false, operand(), CTI_JMP
                ).set_mangled();
            }
        }

        // translate loops and resolve local branches into jmps to instructions.
        // done before mangling, as mangling removes the opportunity to do this
        // type of transformation.
        num_direct_branches += translate_loops(ls);
        if(num_direct_branches) {
            resolve_local_branches(policy, cpu, ls);
        }

        instruction bb_begin(ls.prepend(label_()));

        // prepare the instructions for final execution; this does instruction-
        // specific translations needed to make the code sane/safe to run.
        // mangling uses `client_policy` as opposed to `policy` so that CTIs
        // are mangled to transfer control to the (potentially different) client
        // policy.
        instruction_list_mangler mangler(
            cpu, thread, block_storage, client_policy);
        mangler.mangle(ls);

        // re-calculate the size and re-allocate; if our earlier
        // guess was too small then we need to re-instrument the
        // instruction list
        generated_pc = cpu->fragment_allocator.\
            allocate_array<uint8_t>(basic_block::size(ls));

        IF_TEST( app_pc emitted_pc = ) emit(
            policy, ls, bb_begin, block_storage, start_pc, generated_pc);

        // If this isn't the case, then there there was likely a buffer
        // overflow. This assumes that the fragment allocator always aligns
        // executable code on a 16 byte boundary.
        ASSERT(generated_pc == emitted_pc);

        // The generated pc is not necessarily the actual basic block beginning
        // because direct jump patchers will be prepended to the basic block.
        basic_block ret(bb_begin.pc());

        // quick double check to make sure that we can properly resolve the
        // basic block info later. If this isn't the case, then we likely need
        // to choose different magic values, or make them longer.
        ASSERT(bb_begin.pc() == ret.cache_pc_start);

        //printf("%p -> %p\n", start_pc, bb_begin->pc());

        IF_PERF( perf::visit_encoded(ret); )

        return ret;
    }


    /// Emit an instruction list as code into a byte array. This will also
    /// emit the basic block meta information.
    ///
    /// Note: it is assumed that no field in *state points back to itself
    ///       or any other temporary storage location.
    app_pc basic_block::emit(
        instrumentation_policy policy,
        instruction_list &ls,
        instruction bb_begin,
        basic_block_state *block_storage,
        app_pc generating_pc,
        app_pc pc
    ) throw() {

        pc += ALIGN_TO(reinterpret_cast<uint64_t>(pc), BB_ALIGN);

        // add in the instructions
        const app_pc start_pc(pc);
        pc = ls.encode(pc);

        uint64_t pc_uint(reinterpret_cast<uint64_t>(pc));
        app_pc pc_aligned(pc + ALIGN_TO(pc_uint, BB_INFO_BYTE_ALIGNMENT));

        // add in the padding
        for(; pc < pc_aligned; ) {
            *pc++ = BB_PADDING;
        }

        basic_block_info *info(unsafe_cast<basic_block_info *>(pc));

        // fill in the info
        info->magic = basic_block_info::HEADER;
        info->num_bytes = static_cast<unsigned>(pc - start_pc);
        info->num_patch_bytes = static_cast<unsigned>(bb_begin.pc() - start_pc);
        info->policy_bits = policy.extension_bits();
        info->generating_pc = generating_pc;
        info->rel_state_addr = reinterpret_cast<int64_t>(info) \
                             - reinterpret_cast<int64_t>(block_storage);

        // fill in the byte state set
        pc += sizeof(basic_block_info);
        IF_KERNEL( pc += initialise_state_bytes(info, bb_begin, pc); )

        return start_pc;
    }
}


