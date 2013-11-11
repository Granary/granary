/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * bb.cc
 *
 *      Author: Peter Goodman
 */

#include "granary/globals.h"
#include "granary/basic_block.h"
#include "granary/basic_block_info.h"
#include "granary/instruction.h"
#include "granary/state.h"
#include "granary/policy.h"
#include "granary/mangle.h"
#include "granary/emit_utils.h"
#include "granary/code_cache.h"

#if GRANARY_IN_KERNEL
#   include "granary/kernel/linux/user_address.h"
#endif

#include <new>

namespace granary {


    enum {

        /// number of byte states (bit pairs) per byte, i.e. we have a 4-to-1
        /// compression ratio of the instruction bytes to the state set bytes
        BB_BYTE_STATES_PER_BYTE = 4,

        /// misc.
        BITS_PER_BYTE = 8,
        BITS_PER_STATE = BITS_PER_BYTE / BB_BYTE_STATES_PER_BYTE,
        BITS_PER_QWORD = BITS_PER_BYTE * 8,

        /// Optimise by eliding JMPs in basic blocks with fewer than this
        /// many instructions.
#if CONFIG_FOLLOW_CONDITIONAL_BRANCHES
        BB_ELIDE_JMP_MIN_INSTRUCTIONS = 10
#else
        BB_ELIDE_JMP_MIN_INSTRUCTIONS = 10
#endif
    };


    /// Get the state of a byte in the basic block.
    inline static code_cache_byte_state
    get_state(const uint8_t *states, unsigned i) throw() {
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


    enum {
        DELAY_INSTRUCTION = instruction::DELAY_BEGIN | instruction::DELAY_END
    };


#if CONFIG_ENABLE_INTERRUPT_DELAY
    /// Get the state of all bytes of an instruction.
    static code_cache_byte_state
    get_instruction_state(
        const instruction in,
        code_cache_byte_state prev_state
    ) throw() {

        uint8_t flags(in.instr->granary_flags);;

        // Single delayed instruction; makes no sense. It is considered native.
        if(DELAY_INSTRUCTION == (flags & DELAY_INSTRUCTION)) {
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


    /// Scan the instructions to see if the interrupt delay feature is being used.
    static bool requires_state_bytes(instruction in, instruction last) throw() {
        for(; in.is_valid() && in != last; in = in.next()) {
            if(DELAY_INSTRUCTION & in.instr->granary_flags) {
                return true;
            }
        }

        return false;
    }

    /// Set the state of a pair of bits in memory.
    static unsigned initialise_state_bytes(
        instruction in,
        instruction last,
        uint8_t *bytes,
        unsigned num_bytes
    ) throw() {

        unsigned num_instruction_bits(num_bytes * BITS_PER_STATE);
        num_instruction_bits += ALIGN_TO(num_instruction_bits, BITS_PER_QWORD);
        unsigned num_state_bytes = num_instruction_bits / BITS_PER_BYTE;

        unsigned byte_offset(0);
        unsigned prev_byte_offset(0);

        code_cache_byte_state prev_state(BB_BYTE_NATIVE);
        unsigned num_delayed_instructions(0);
        unsigned prev_num_delayed_instructions(0);

        // Initialise all state bytes to have their states as native.
        memset(bytes, 0, num_state_bytes);

        for(; in.is_valid() && in != last; in = in.next()) {

            code_cache_byte_state state(get_instruction_state(in, prev_state));
            code_cache_byte_state stored_state(state);

            const unsigned in_num_bytes(in.encoded_size());

            if(BB_BYTE_NATIVE != stored_state) {
                if(in_num_bytes) {
                    ++num_delayed_instructions;
                }
                stored_state = BB_BYTE_DELAY_CONT;
            } else {
                prev_num_delayed_instructions = num_delayed_instructions;
                num_delayed_instructions = 0;
            }

            // The last delayed block was only one instruction long; clear its
            // state bits so that it is not in fact delayed (as that makes
            // no sense).
            if(1 == prev_num_delayed_instructions) {
                for(unsigned j(prev_byte_offset); j < byte_offset; ++j) {
                    set_state(bytes, j, BB_BYTE_NATIVE);
                }
            }

            // For each byte of each instruction.
            for(unsigned j(byte_offset); j < (byte_offset + in_num_bytes); ++j) {
                set_state(bytes, j, stored_state);
            }

            // Update so that we have byte accuracy on begin/end.
            if(BB_BYTE_DELAY_BEGIN == state) {
                set_state(bytes, byte_offset, BB_BYTE_DELAY_BEGIN);
            } else if(BB_BYTE_DELAY_END == state) {
                set_state(
                    bytes,
                    byte_offset + in_num_bytes - 1,
                    BB_BYTE_DELAY_END);
            }

            prev_byte_offset = byte_offset;
            byte_offset += in_num_bytes;
            prev_state = state;
        }

        return num_state_bytes;
    }
#endif


    /// Return the meta information for the current basic block, given some
    /// pointer into the instructions of the basic block.
    basic_block::basic_block(app_pc current_pc_) throw()
        : info(find_basic_block_info(current_pc_))
        , policy(instrumentation_policy(info->generating_pc))
        , cache_pc_start(info->start_pc)
        , cache_pc_current(current_pc_)
    { }


#if GRANARY_IN_KERNEL && CONFIG_ENABLE_INTERRUPT_DELAY
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
        const uint8_t *delay_states(info->delay_states);

        // Quick check: if we don't have any delay ranges then we never
        // allocated the state bits.
        if(!delay_states || cache_pc_current == cache_pc_start) {
            return false;
        }

        ASSERT(cache_pc_current > cache_pc_start);

        const unsigned current_offset(cache_pc_current - cache_pc_start);
        const unsigned end_offset(info->num_bytes);

        ASSERT(current_offset < info->num_bytes);

        code_cache_byte_state byte_state(get_state(
            info->delay_states, current_offset));

        enum {
            SAFE_INTERRUPT_STATE = BB_BYTE_NATIVE | BB_BYTE_DELAY_BEGIN
        };

        if(0 != (SAFE_INTERRUPT_STATE & byte_state)) {
            begin = nullptr;
            end = nullptr;
            return false;
        }

        for(unsigned i(current_offset); i < end_offset; ++i) {
            if(BB_BYTE_DELAY_END == get_state(delay_states, i)) {
                end = cache_pc_start + i + 1;
                break;
            }
        }

        for(unsigned i(0); i < current_offset; ++i) {
            if(SAFE_INTERRUPT_STATE & get_state(delay_states, i)) {
                begin = cache_pc_start + i;
            }
        }

        return end && begin;
    }
#endif



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
    static void translate_loops(instruction_list &ls) throw() {

        instruction in(ls.first());
        instruction next_in;
        instruction try_skip;

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

                operand target(in.cti_target());
                instruction try_loop(ls.insert_before(in, label_()));
                ls.insert_before(try_loop, mangled(jmp_(instr_(try_loop))));
                instruction do_loop(ls.insert_before(try_loop, label_()));
                ls.insert_before(try_loop, jmp_(target));

                in.set_cti_target(instr_(do_loop));
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
            //              j[n]z <after>
            //              jmp <do_instr>
            // after:
            case dynamorio::OP_rep_cmps:
            case dynamorio::OP_repne_cmps:
            case dynamorio::OP_rep_scas:
            case dynamorio::OP_repne_scas: {
                instruction after(label_());
                instruction do_instr(label_());
                operand counter(rep_counter(in));
                instruction skip(mangled(jmp_(instr_(after))));

                // conditional termination condition
                instruction (*jcc_)(dynamorio::opnd_t) = jnz_;
                if(dynamorio::OP_repne_cmps == in.op_code()
                || dynamorio::OP_repne_scas == in.op_code()) {
                    jcc_ = jz_;
                }

                try_skip = ls.insert_before(in, rep_jrcxz_(skip, counter));
                ls.insert_before(in, mangled(jmp_(instr_(do_instr))));
                ls.insert_before(in, skip);
                ls.insert_before(in, do_instr);

                ls.insert_after(in, after);
                ls.insert_before(after, lea_(counter, counter[-1]));
                ls.insert_before(after, rep_jrcxz_(after, counter));
                ls.insert_before(after, mangled(jcc_(instr_(after))));
                ls.insert_before(after, mangled(jmp_(instr_(do_instr))));

                goto common_REP_tail;
            }

            common_REP_tail: {
                app_pc without_rep(in.pc() + 1);
                in.instr->flags = 0;
                in.replace_with(instruction::decode(&without_rep));
                in.instr->translation = nullptr;
                try_skip.instr->translation = without_rep;

                break;
            }
#endif

            default: break;
            }
        }
    }


    /// Estimate the maximum size of a basic block by being pessimistic about
    /// alignment constraints for hot-patchable instructions.
    static unsigned estimate_max_size(instruction_list &ls) throw() {
        unsigned size(0);
        for(instruction in(ls.first()); in.is_valid(); in = in.next()) {
            size += in.encoded_size();
            if(in.is_patchable()) {
                size += 8;
            }
        }
        return size;
    }


    /// Decode some instructions for use as a basic block. Returns the number
    /// of decoded instructions.
    unsigned basic_block::decode(
        instruction_list &ls,
        instrumentation_policy policy,
        const app_pc start_pc,
        app_pc &end_pc
        _IF_KERNEL( void *&user_exception_metadata )
    ) throw() {
        app_pc local_pc(start_pc);
        app_pc *pc(&local_pc);
        bool fall_through_pc(false);
        bool fall_through_cond_cti(false);
        bool uses_xmm(policy.is_in_xmm_context());
        bool fall_through_detach(false);
        bool detach_tail_call(false);
        const app_pc detach_app_pc(unsafe_cast<app_pc>(&detach));
        unsigned byte_len(0);
        unsigned num_decoded(0);

        // Ensure that the start PC of the basic block is always somewhere in
        // the instruction stream.
        instruction in(ls.append(label_()));
        in.set_pc(start_pc);

        for(;;) {
            in = instruction::decode(pc);

            // TODO: curiosity.
            if(dynamorio::OP_INVALID == in.op_code()
            || dynamorio::OP_UNDECODED == in.op_code()) {

#if CONFIG_ENABLE_ASSERTIONS
                printf(
                    "Failed to decode instruction at %p in "
                    "block starting at %p\n",
                    in.pc(), start_pc);
                USED(in); // To help with debugging.
#endif /* CONFIG_ENABLE_ASSERTIONS */
                ls.append(ud2a_());
                break;
            }

            // Useful to relate back to the kernel's BUG_ON macro. We need to
            // make sure to restore *pc to the address of the UD2 instruction
            // so that a debugger can see the related source code nicely.
            if(dynamorio::OP_ud2a == in.op_code()
            || dynamorio::OP_ud2b == in.op_code()) {
                *pc = in.pc();
                fall_through_detach = true;
                break;
            }

            byte_len += in.instr->length;
            num_decoded += 1;
            ls.append(in);

            // Keep track of the end of this basic block.
            //
            // Note: This can sometimes get out of sync if we're eliding JMPs.
            end_pc = *pc;

#if GRANARY_IN_KERNEL
            // Stop at IRET, SYSRET, SWAPGS, or SYSEXIT. If we also see a
            // write to RSP at some point before then chop it off there so
            // that instrumentation always stays on a safe stack.
            if(dynamorio::OP_sysret == in.op_code()
            || dynamorio::OP_swapgs == in.op_code()
            || dynamorio::OP_sysexit == in.op_code()
            || dynamorio::OP_iret == in.op_code()) {

                fall_through_detach = true;
                *pc = in.pc();
                ls.remove(in);

                // Try to find a write to %rsp somewhere earlier in the
                // instruction list. If so, chop the list off there.
                for(in = ls.last(); in.is_valid(); in = in.prev()) {
                    const bool changes_stack(
                        dynamorio::instr_writes_to_exact_reg(
                            in.instr, dynamorio::DR_REG_RSP));

                    if(changes_stack
                    && dynamorio::OP_mov_ld <= in.op_code()
                    && dynamorio::OP_mov_priv >= in.op_code()) {
                        *pc = in.pc();
                        ls.remove_tail_at(in);
                        break;
                    }
                }

                in = ls.last();
                break;
            }
#endif /* GRANARY_IN_KERNEL */

            if(in.is_cti()) {
                operand target(in.cti_target());
                bool target_is_pc(false);

                // Direct branch (e.g. un/conditional branch, jmp, call).
                if(dynamorio::opnd_is_pc(target)) {
                    target_is_pc = true;
                    app_pc target_pc(dynamorio::opnd_get_pc(target));

                    if(detach_app_pc == target_pc) {
                        fall_through_detach = true;
                        if(in.is_jump()) {
                            detach_tail_call = true;
                        }
                        break;
                    }

                    // We have some native code that has a jump to code cache or
                    // wrapper code. Normally this sounds weird but actually it
                    // comes up when we're doing probe-based instrumentation
                    // with patch wrappers and such.
                    if(is_code_cache_address(target_pc)
                    || is_wrapper_address(target_pc)) {
                        in.set_mangled();
                        break;
                    }
                }

                // Unconditional JMP; ends the block, without possibility
                // of falling through.
                if(in.is_jump()) {

                    if(!target_is_pc
                    || BB_ELIDE_JMP_MIN_INSTRUCTIONS < ls.length()) {
                        fall_through_pc = false;
                        break;
                    }

                    ls.remove(in);
                    *pc = dynamorio::opnd_get_pc(target);

                    if(find_detach_target(*pc, policy.context())) {
                        fall_through_detach = true;
                        break;
                    }

                    // We continue decoding at the target.

                // CALL, if direct returns are supported then the basic
                // block that can continue; however, if direct returns
                // aren't supported then we need to support a fall-through
                // JMP to the next basic block.
                } else if(in.is_call()) {
                    // Nothing to do.

                // RET, far RET, and IRET instruction.
                } else if(in.is_return()) {
                    break;

                // Conditional CTI, end the block with the ability to fall-
                // through.
                } else {
                    fall_through_pc = true;
                    fall_through_cond_cti = true;
                    break;
                }

            // Some other instruction.
            } else {

                // update the policy to be in an xmm context.
                if(!uses_xmm && dynamorio::instr_is_sse_or_sse2(in)) {
                    policy.in_xmm_context();
                    uses_xmm = true;
                }
            }
        }

#if GRANARY_IN_KERNEL
        // Look for potential code that accesses user-space data. This type of
        // code follows some rough binary patterns, so we first search for the
        // patterns, then verify by invoking the kernel itself.
        if(policy.accesses_user_data()
        || kernel_code_accesses_user_data(ls, start_pc)) {
            user_exception_metadata = kernel_find_exception_metadata(ls);

            // We don't un-set the accesses user data policy property, as there
            // is some value in leaving it set.
            //
            // We also don't require that a particular exception table exist.
            // That is, if we match the binary pattern, then that is sufficient
            // to trigger future exception table searches which are more
            // precise.
            if(!policy.accesses_user_data()) {
                policy.access_user_data(true);
            }
        }
#endif

        // Translate loops and resolve local branches into jmps to instructions.
        // done before mangling, as mangling removes the opportunity to do this
        // type of transformation.
        translate_loops(ls);

        // Add in a trailing jump if the last instruction in the basic
        // block if we need to force a connection between this basic block
        // and the next.
        if(fall_through_pc) {
            instruction fall_through(ls.append(jmp_(pc_(*pc))));
            if(fall_through_cond_cti) {
                fall_through.add_flag(instruction::COND_CTI_FALL_THROUGH);
            }

        /// Add in a trailing (emulated) jmp or ret if we are detaching.
        } else if(fall_through_detach) {
            if(detach_tail_call) {
                ls.append(mangled(ret_()));
            } else {
                instruction fall_through_cti(insert_cti_after(
                    ls, ls.last(), *pc,
                    CTI_DONT_STEAL_REGISTER, operand(),
                    CTI_JMP
                ));

                fall_through_cti.set_mangled();
            }
        }

        return num_decoded;
    }


    struct block_translator {
        bool translated;

        block_translator *next;
        basic_block_state *state;

        IF_KERNEL( void *user_exception_metadata; )

        app_pc start_pc;
        app_pc end_pc;

        instrumentation_policy incoming_policy;
        instrumentation_policy outgoing_policy;

        instruction start_label;
        instruction end_label;

        instruction_list ls;

        unsigned num_decoded_instructions;

        block_translator(void) throw();

        void run(cpu_state_handle cpu) throw();

        bool visit_branches(block_translator **head) throw();

        void fixup_branches(
            block_translator *trace,
            block_translator *next_block,
            app_pc trace_start_pc,
            app_pc trace_end_pc
        ) throw();
    };


    block_translator::block_translator(void) throw()
        : translated(false)
        , next(nullptr)
        , state(nullptr)
        _IF_KERNEL( user_exception_metadata(nullptr) )
        , start_pc(nullptr)
        , end_pc(nullptr)
        , start_label(label_())
        , end_label(label_())
        , num_decoded_instructions(0)
    { }


    /// Translate an individual basic block.
    void block_translator::run(cpu_state_handle cpu) throw() {
        if(basic_block_state::size()) {
            state = cpu->block_allocator.allocate<basic_block_state>();
        }

        num_decoded_instructions = basic_block::decode(
            ls, incoming_policy,
            start_pc, end_pc
            _IF_KERNEL(user_exception_metadata));

        // Invoke client code instrumentation on the basic block; the client
        // might return a different instrumentation policy to use. The effect
        // of this is that if we are in policy P1, and the client returns policy
        // P2, then we will emit a block to P1's code cache that jumps us into
        // P2's code cache.
        outgoing_policy = incoming_policy.instrument(
            cpu,
            *state, // potentially NULL.
            ls);

        outgoing_policy.inherit_properties(incoming_policy);

#if CONFIG_TRACE_EXECUTION
        // Add in logging at the beginning of the basic block so that we can
        // debug the flow of execution in the code cache.
        trace_log::log_execution(ls);
#endif

        // Add labels to bound the basic block, so that we can connect blocks
        // together in the trace, while still knowing where the individual
        // begin and end.
        ls.prepend(start_label);
        ls.append(end_label);
    }


    /// Add a block translator to a list.
    static block_translator *find_block_translator(
        block_translator *curr,
        app_pc start_pc,
        instrumentation_policy policy
    ) throw() {
        for(; nullptr != curr; curr = curr->next) {

            // Arrange the basic blocks into sorted order.
            if(start_pc == curr->start_pc
            && policy == curr->incoming_policy) {
                return curr;
            }
        }

        return nullptr;
    }


    /// Add a block translator to a list.
    static block_translator *add_block_translator(
        block_translator **head,
        block_translator *item
    ) throw() {
        block_translator *curr(*head);
        for(; nullptr != curr; ) {

            // Arrange the basic blocks into sorted order.
            if(item->start_pc < curr->start_pc) {
                break;

            // Already translated this basic block.
            } else if(item->start_pc == curr->start_pc
                   && item->incoming_policy == curr->incoming_policy) {
                return curr;
            }

            head = &(curr->next);
            curr = curr->next;
        }

        // Insert into our translator list.
        item->next = *head;
        *head = item;

        return item;
    }


    /// Try to aggressively follow and queue up translations for all
    /// fall-through targets of conditional branches.
    bool block_translator::visit_branches(
        block_translator **head
    ) throw() {
        bool found_successor(false);
        for(instruction in(ls.first()); in.is_valid(); in = in.next()) {

            // Filter out indirect CTIs, conditional CTIs, and mangled CTIs.
            if(in.is_mangled() || !in.is_cti() || !in.is_unconditional_cti()) {
                continue;
            }

            // Only allow ourselves to follow the fall-through unconditional
            // jumps, as they are actually conditional ;-) This filters out
            // all CALLs and non-fall-through JMPs.
            if(!in.has_flag(instruction::COND_CTI_FALL_THROUGH)) {
                continue;
            }

            instrumentation_policy target_policy(in.policy());
            if(!target_policy) {
                target_policy = outgoing_policy;
            }

            app_pc target_pc(in.cti_target().value.pc);

            // Same policy inheritance protocol as in the instruction list
            // mangler for `mangle_cti` and `mangle_direct_cti`.
            target_policy.begins_functional_unit(false); // Sane default.
            target_policy.inherit_properties(outgoing_policy, INHERIT_JMP);
            target_policy.return_target(false);
            target_policy.indirect_cti_target(false);

            // Simplifying assumptions:
            //      1) Conditional CTIs never target a detach address.
            //      2) Conditional CTIs never change the context (host vs. app).

            block_translator *block(allocate_memory<block_translator>());
            block->start_pc = target_pc;
            block->incoming_policy = target_policy;

            block_translator *added(add_block_translator(head, block));
            if(added != block) {
                free_memory(block);
            } else {
                found_successor = true;
            }

            // Connect the basic blocks.
            in.set_mangled();
            in.set_cti_target(instr_(added->start_label));
        }

        return found_successor;
    }


    /// Try to fix up any remaining branch targets that point back into our
    /// trace.
    void block_translator::fixup_branches(
        block_translator *trace,
        block_translator *next_block,
        app_pc trace_start_pc,
        app_pc trace_end_pc
    ) throw() {
        for(instruction in(ls.first()), next_in; in.is_valid(); in = next_in) {
            next_in = in.next();

            if(!in.is_cti()) {
                continue;
            }

            // Peephole optimization for fall-through branches.
            const operand target(in.cti_target());
            if(nullptr != next_block
            && dynamorio::INSTR_kind == target.kind
            && dynamorio::OP_jmp == in.op_code()
            && next_in.is_valid()
            && next_in == end_label
            && target.value.instr == next_block->start_label) {
                ls.remove(in);
            } else {
                continue;
            }

            if(in.is_mangled()) {
                continue;
            }


            if(!dynamorio::opnd_is_pc(target)) {
                continue;
            }

            if(target.value.pc < trace_start_pc
            || target.value.pc >= trace_end_pc) {
                continue;
            }

            // Inductive argument based on assumptions about conditional branch
            // targets:
            //      1) Conditional branches don't target detach addresses,
            //         therefore unconditional branches back into the trace
            //         don't either.
            //      2) Conditional branches don't change the context, therefore
            //         unconditional branches back into the trace won't either.

            instrumentation_policy target_policy(in.policy());
            if(!target_policy) {
                target_policy = outgoing_policy;
            }

            target_policy.begins_functional_unit(false); // Sane default.

            if(in.is_call()) {
                target_policy.inherit_properties(outgoing_policy, INHERIT_CALL);
                target_policy.return_address_in_code_cache(true);
                target_policy.begins_functional_unit(true);
            } else {
                target_policy.inherit_properties(outgoing_policy, INHERIT_JMP);
            }

            target_policy.return_target(false);
            target_policy.indirect_cti_target(false);

            block_translator *target_block(find_block_translator(
                trace, target.value.pc, target_policy));

            // Redirect the CTI back into the trace.
            if(target_block) {
                in.set_cti_target(instr_(target_block->start_label));
                in.set_mangled();

                // Re-visit this instruction so that we can try to peephole-
                // optimize fall-through JMPs out of existence.
                if(next_block) {
                    next_in = in;
                }
            }
        }
    }


    /// Decode and translate a single basic block of application/module code.
    basic_block basic_block::translate(
        const instrumentation_policy policy,
        cpu_state_handle cpu,
        const app_pc start_pc
    ) throw() {

        instruction_list patch_stubs(INSTRUCTION_LIST_GENCODE);

        block_translator *trace(allocate_memory<block_translator>());
        block_translator * const trace_original_bb(trace);
        app_pc end_pc(nullptr);

        trace->start_pc = start_pc;
        trace->incoming_policy = policy;

#if CONFIG_FOLLOW_CONDITIONAL_BRANCHES
        for(bool changed(true); changed; ) {
            changed = false;

            // Build a trace of all successors that we can follow through
            // conditional CTIs.
            for(block_translator *block(trace);
                nullptr != block;
                block = block->next) {

                if(!block->translated) {
                    block->run(cpu);
                    if(block->visit_branches(&trace)) {
                        changed = true;
                    }
                    block->translated = true;
                }

                if(block->end_pc > end_pc) {
                    end_pc = block->end_pc;
                }
            }
        }
#else
        trace->run(cpu);
        end_pc = trace->end_pc;
#endif

        const_app_pc estimator_pc(
            cpu->current_fragment_allocator->allocate_staged<uint8_t>());

        // Form one large trace instruction list.
        instruction_list ls;
        for(block_translator *block(trace);
            nullptr != block;
            block = block->next) {

            block->fixup_branches(
                trace, block->next, start_pc, end_pc);

            instruction_list_mangler mangler(
                cpu, *block->state, block->ls, patch_stubs,
                block->outgoing_policy, estimator_pc);

            // Prepare the instructions for final execution; this does
            // instruction-specific translations needed to make the code
            // sane/safe to run. mangling uses `client_policy` as opposed to
            // `policy` so that CTIs are mangled to transfer control to the
            // (potentially different) client policy.
            mangler.mangle();

            // Extend our trace instruction list.
            ls.extend(block->ls);
        }

        // Guarantee enough space to allocate the instructions of this basic
        // block so that we can get an exact estimator pc for the beginning of
        // the basic block. This estimator pc will tell us the current cache
        // line alignment of the beginning of the basic block, which will allow
        // us to properly align hot-patchable instructions.
        cpu->current_fragment_allocator->allocate_array<uint8_t>(
            estimate_max_size(ls));
        cpu->current_fragment_allocator->free_last();
        const uintptr_t estimator_addr(reinterpret_cast<uintptr_t>(
            cpu->current_fragment_allocator->allocate_staged<uint8_t>()));

        // Align all hot-patchable instructions, and get the size of the
        // instruction list.
        const unsigned emitted_size(instruction_list_mangler::align(
            ls, estimator_addr % CONFIG_MIN_CACHE_LINE_SIZE));

        uint8_t *emitted_start_pc(
            cpu->current_fragment_allocator->allocate_array<uint8_t>(
                emitted_size));

        // Calculate the size of the stubs and then encode the stubs.
        app_pc stub_pc(nullptr);
        unsigned stub_size(0);
        if(patch_stubs.length()) {
            stub_size = patch_stubs.encoded_size();
            stub_pc = cpu->stub_allocator.allocate_array<uint8_t>(stub_size);
            patch_stubs.encode(stub_pc, stub_size);

        // Make sure we do even a fake allocation so that next time a basic
        // block is killed on the CPU, we don't accidentally kill the last
        // allocated stubs.
        } else {
            cpu->stub_allocator.allocate_staged<uint8_t>();
        }

        // Emit the instructions into the code cache.
        ls.encode(emitted_start_pc, emitted_size);

        // Re-encode the patch instruction list to resolve the circular
        // dependencies.
        if(patch_stubs.length()) {
            memset(stub_pc, 0xCC, stub_size);
            patch_stubs.encode(stub_pc, stub_size);
        }

        IF_PERF( unsigned num_bbs_in_trace(0); )
        const app_pc trace_start_pc(trace_original_bb->start_label.pc());

        // Create the basic block info for each trace basic block.
        for(block_translator *block(trace), *next_block(nullptr);
            nullptr != block;
            block = next_block) {

            next_block = block->next;

            IF_PERF( ++num_bbs_in_trace; )

            IF_KERNEL( uint8_t *delay_states(nullptr); )
            IF_KERNEL( unsigned num_state_bytes(0); )

            const app_pc block_start_pc(block->start_label.pc());
            const app_pc block_end_pc(block->end_label.pc());
            const unsigned block_size(block_end_pc - block_start_pc);

#if GRANARY_IN_KERNEL && CONFIG_ENABLE_INTERRUPT_DELAY
            if(requires_state_bytes(block->start_label, block->end_label)) {
                num_state_bytes = (block_size + 7) / BB_BYTE_STATES_PER_BYTE;
            }
#endif

            basic_block_info *info(allocate_basic_block_info(
                block_start_pc
                _IF_KERNEL(num_state_bytes)
                _IF_KERNEL(delay_states)));

            const mangled_address am(block->start_pc, block->incoming_policy);

            info->start_pc = block_start_pc;
            info->num_bytes = block_size;
            info->generating_pc = am;
            info->generating_num_instructions = block->num_decoded_instructions;
            info->state = block->state;

#if CONFIG_ENABLE_TRACE_ALLOCATOR
            /// !!!!! TODO !!!!!
            ///     Ensure proper inheritance of the allocator w.r.t.
            ///     direct CTI mangling.
            info->allocator = cpu->current_fragment_allocator;
#endif

#if GRANARY_IN_KERNEL
            info->user_exception_metadata = block->user_exception_metadata;
#   if CONFIG_ENABLE_INTERRUPT_DELAY
            info->delay_states = delay_states;
            info->num_delay_state_bytes = num_state_bytes;
            if(delay_states) {
                initialise_state_bytes(
                    block->start_label, block->end_label,
                    delay_states, num_state_bytes);
            }
#   endif
#endif

            // Inject all of the internal trace basic blocks into the code cache.
            if(block != trace_original_bb) {
                code_cache::add(am.as_address, block_start_pc);
            }

            // Don't need it anymore!
            free_memory(block);
        }

        // The generated pc is not necessarily the actual basic block beginning
        // because direct jump patchers will be prepended to the basic block.
        basic_block ret(trace_start_pc);

        IF_PERF( perf::visit_trace(num_bbs_in_trace); )

        return ret;
    }
}
