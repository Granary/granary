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

        /// Maximum size in bytes of a decoded basic block. This relates to
        /// *decoding* only and not the resulting size of a basic block after
        /// translation.
        BB_MAX_SIZE_BYTES = detail::fragment_allocator_config::SLAB_SIZE / 4,

        /// number of byte states (bit pairs) per byte, i.e. we have a 4-to-1
        /// compression ratio of the instruction bytes to the state set bytes
        BB_BYTE_STATES_PER_BYTE = 4,

        /// misc.
        BITS_PER_BYTE = 8,
        BITS_PER_STATE = BITS_PER_BYTE / BB_BYTE_STATES_PER_BYTE,
        BITS_PER_QWORD = BITS_PER_BYTE * 8,

        /// Optimise by eliding JMPs in basic blocks with fewer than this
        /// many instructions.
        BB_ELIDE_JMP_MIN_INSTRUCTIONS = 10
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


#if CONFIG_ENABLE_INTERRUPT_DELAY
    /// Scan the instructions to see if the interrupt delay feature is being used.
    static bool requires_state_bytes(instruction_list &ls) throw() {
        for(instruction in(ls.first()); in.is_valid(); in = in.next()) {
            if(DELAY_INSTRUCTION & in.instr->granary_flags) {
                return true;
            }
        }

        return false;
    }

    /// Set the state of a pair of bits in memory.
    static unsigned initialise_state_bytes(
        instruction_list &ls,
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

        for(instruction in(ls.first()); in.is_valid(); in = in.next()) {

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
        const app_pc start_pc
        _IF_KERNEL( void *&user_exception_metadata )
    ) throw() {
        app_pc local_pc(start_pc);
        app_pc *pc(&local_pc);
        bool fall_through_pc(false);
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

            // Very big basic block; cut it short and connect it with a tail.
            // we do this test *before* decoding the next instruction because
            // `instruction::decode` modifies `pc`, and `pc` is used to
            // determine the fall-through target.
            if(byte_len > BB_MAX_SIZE_BYTES) {
                fall_through_pc = true;
                break;
            }

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
#if GRANARY_IN_KERNEL
                    // Go look to see if there's a SWAPGS leading to an IRET
                    // and chop the block off at the SWAPGS.
                    if(dynamorio::OP_iret == in.op_code()) {
                        for(in = ls.last(); in.is_valid(); in = in.prev()) {
                            if(dynamorio::OP_swapgs != in.op_code()) {
                                continue;
                            }

                            *pc = in.pc();
                            ls.remove_tail_at(in);
                            fall_through_detach = true;
                            break;
                        }
                    }
#endif /* GRANARY_IN_KERNEL */
                    break;

                // Conditional CTI, end the block with the ability to fall-
                // through.
                } else {
                    if(true || BB_ELIDE_JMP_MIN_INSTRUCTIONS < ls.length()) {
                        fall_through_pc = true;
                        break;
                    }
                }

#if GRANARY_IN_KERNEL
#   if CONFIG_INSTRUMENT_HOST

            // Expect to see a write to %rsp at some point before sysret.
            } else if(dynamorio::OP_sysret == in.op_code()) {

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

            } else if(dynamorio::OP_sysexit == in.op_code()) {
                break;
#   else
            } else if(dynamorio::OP_sysret == in.op_code()
                   || dynamorio::OP_sysexit == in.op_code()
                   || dynamorio::OP_swapgs == in.op_code()) {
                granary_break_on_curiosity();
                break;
#   endif
#endif

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
            ls.append(jmp_(pc_(*pc)));

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


    /// Decode and translate a single basic block of application/module code.
    basic_block basic_block::translate(
        const instrumentation_policy policy,
        cpu_state_handle cpu,
        const app_pc start_pc
    ) throw() {
        instruction_list ls;

        IF_KERNEL( void *user_exception_metadata(nullptr); )
        const unsigned num_decoded_instructions(decode(
            ls, policy, start_pc _IF_KERNEL(user_exception_metadata) ));

#if CONFIG_ENABLE_ASSERTIONS
        // Sanity checking before we begin instrumenting; we don't want to
        // apply the wrong instrumentation function to the code!
        if(policy.is_in_host_context()) {
            ASSERT(!is_app_address(start_pc));
        }
#endif

        basic_block_state *state(nullptr);
        if(basic_block_state::size()) {
            state = cpu->block_allocator.allocate<basic_block_state>();
        }

        // Invoke client code instrumentation on the basic block; the client
        // might return a different instrumentation policy to use. The effect
        // of this is that if we are in policy P1, and the client returns policy
        // P2, then we will emit a block to P1's code cache that jumps us into
        // P2's code cache.
        instrumentation_policy client_policy(policy.instrument(
            cpu,
            *state, // potentially null.
            ls));

        client_policy.inherit_properties(policy);

#if CONFIG_TRACE_EXECUTION
        // Add in logging at the beginning of the basic block so that we can
        // debug the flow of execution in the code cache.
        trace_log::log_execution(ls);
#endif

        instruction bb_begin(ls.prepend(label_()));

        // Prepare the instructions for final execution; this does instruction-
        // specific translations needed to make the code sane/safe to run.
        // mangling uses `client_policy` as opposed to `policy` so that CTIs
        // are mangled to transfer control to the (potentially different) client
        // policy.
        instruction_list patch_stubs(INSTRUCTION_LIST_GENCODE);
        instruction_list_mangler mangler(
            cpu, *state, ls, patch_stubs, client_policy);

        mangler.mangle();

        cpu->fragment_allocator.lock_coarse();

        // Guarantee enough space to allocate the instructions of this basic
        // block so that we can get an exact estimator pc for the beginning of
        // the basic block. This estimator pc will tell us the current cache
        // line alignment of the beginning of the basic block, which will allow
        // us to properly align hot-patchable instructions.
        cpu->fragment_allocator.allocate_array<uint8_t>(estimate_max_size(ls));
        cpu->fragment_allocator.free_last();
        const uintptr_t estimator_addr(reinterpret_cast<uintptr_t>(
            cpu->fragment_allocator.allocate_staged<uint8_t>()));

        // Align all hot-patchable instructions, and get the size of the
        // instruction list.
        const unsigned size(mangler.align(
            estimator_addr % CONFIG_MIN_CACHE_LINE_SIZE));

        uint8_t *generated_pc(
            cpu->fragment_allocator.allocate_array<uint8_t>(size));

        basic_block_info *info(nullptr);
        IF_KERNEL( uint8_t *delay_states(nullptr); )
        IF_KERNEL( unsigned num_state_bytes(0); )

#if GRANARY_IN_KERNEL && CONFIG_ENABLE_INTERRUPT_DELAY
        num_state_bytes = requires_state_bytes(ls)
                        ? (size + 7) / BB_BYTE_STATES_PER_BYTE
                        : 0;
#endif

        allocate_basic_block_info(
            generated_pc,
            info
            _IF_KERNEL(num_state_bytes)
            _IF_KERNEL(delay_states));

        info->start_pc = generated_pc;
        info->num_bytes = size;
        info->generating_pc = mangled_address(start_pc, policy);
        info->generating_num_instructions = num_decoded_instructions;
        info->state = state;

#if GRANARY_IN_KERNEL
        info->user_exception_metadata = user_exception_metadata;
        info->delay_states = delay_states;
        info->num_delay_state_bytes = num_state_bytes;
        if(delay_states) {
            initialise_state_bytes(ls, delay_states, num_state_bytes);
        }
#endif

        cpu->fragment_allocator.unlock_coarse();

        // Calculate the size of the stubs and then encode the stubs.
        const unsigned stub_size(patch_stubs.encoded_size());
        app_pc stub_pc = cpu->stub_allocator.allocate_array<uint8_t>(stub_size);
        patch_stubs.encode(stub_pc, stub_size);

        // Emit the instructions to the code cache.
        IF_TEST( app_pc end_pc = ) ls.encode(generated_pc, size);

        // Re-encode the instruction list to resolve the circular dependencies.
        memset(stub_pc, 0xCC, stub_size);
        patch_stubs.encode(stub_pc, stub_size);

        // The generated pc is not necessarily the actual basic block beginning
        // because direct jump patchers will be prepended to the basic block.
        basic_block ret(generated_pc);

        // Quick double check to make sure that we can properly resolve the
        // basic block info later. If this isn't the case, then we likely need
        // to choose different magic values, or make them longer.
        ASSERT(ret.info == info);
        ASSERT((generated_pc + size) == end_pc);
        ASSERT(bb_begin.pc() == generated_pc);
        ASSERT(bb_begin.pc() == ret.cache_pc_start);

        USED(generated_pc);
        USED(size);
        USED(end_pc);

        IF_PERF( perf::visit_encoded(ret); )

        return ret;
    }
}
