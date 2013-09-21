/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * bb.cc
 *
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

        BB_PADDING              = 0xEA,
        BB_PADDING_LONG         = 0xEAEAEAEAEAEAEAEA,
        BB_ALIGN                = 16,

        /// Maximum size in bytes of a decoded basic block. This relates to
        /// *decoding* only and not the resulting size of a basic block after
        /// translation.
        BB_MAX_SIZE_BYTES       = (PAGE_SIZE / 4),

        /// number of byte states (bit pairs) per byte, i.e. we have a 4-to-1
        /// compression ratio of the instruction bytes to the state set bytes
        BB_BYTE_STATES_PER_BYTE = 4,

        /// byte and 32-bit alignment of basic block info structs in memory
        BB_INFO_BYTE_ALIGNMENT  = 8,
        BB_INFO_INT32_ALIGNMENT = 2,

        /// misc.
        BITS_PER_BYTE   = 8,
        BITS_PER_STATE  = BITS_PER_BYTE / BB_BYTE_STATES_PER_BYTE,
        BITS_PER_QWORD  = BITS_PER_BYTE * 8,
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


    /// Set the state of a pair of bits in memory.
    static unsigned initialise_state_bytes(
        basic_block_info *info,
        instruction in,
        app_pc bytes
    ) throw() {

        unsigned num_instruction_bits(info->num_bytes * BITS_PER_STATE);
        num_instruction_bits += ALIGN_TO(num_instruction_bits, BITS_PER_QWORD);
        unsigned num_state_bytes = num_instruction_bits / BITS_PER_BYTE;

        // Initialise all state bytes to have their states as native.
        memset(bytes, 0, num_state_bytes);

        unsigned byte_offset(0);
        unsigned prev_byte_offset(0);

        code_cache_byte_state prev_state(BB_BYTE_NATIVE);
        unsigned num_delayed_instructions(0);
        unsigned prev_num_delayed_instructions(0);

        // Scan the instructions to see if the interrupt delay feature is
        // even being used.
        instruction first(in);
        bool has_delay(false);
        for(; in.is_valid(); in = in.next()) {
            if(DELAY_INSTRUCTION & in.instr->granary_flags) {
                has_delay = true;
            }
        }

        // If we don't need state bytes, then don't initialise any!
        info->has_delay_range = has_delay;
        if(!has_delay) {
            return 0;
        }

        for(in = first; in.is_valid(); in = in.next()) {

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

            // The last delayed block was only one instruction long; clear its
            // state bits so that it is not in fact delayed (as that makes
            // no sense).
            if(1 == prev_num_delayed_instructions) {
                for(unsigned j(prev_byte_offset); j < byte_offset; ++j) {
                    set_state(bytes, j, BB_BYTE_NATIVE);
                }
            }

            // For each byte of each instruction.
            for(unsigned j(byte_offset); j < (byte_offset + num_bytes); ++j) {
                set_state(bytes, j, stored_state);
            }

            // Update so that we have byte accuracy on begin/end.
            if(BB_BYTE_DELAY_BEGIN == state) {
                set_state(bytes, byte_offset, BB_BYTE_DELAY_BEGIN);
            } else if(BB_BYTE_DELAY_END == state) {
                set_state(
                    bytes,
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
        // Round our search address down to `BB_INFO_BYTE_ALIGNMENT` before we
        // search for the meta info magic value.
        uintptr_t addr(reinterpret_cast<uint64_t>(current_pc_));
        addr -= (addr % BB_INFO_BYTE_ALIGNMENT);

        // Go search for the basic block info by finding potential
        // addresses of the meta info, then checking their magic values.
        uint32_t *ints(reinterpret_cast<uint32_t *>(addr));
        for(; ; ++ints) {
            if(basic_block_info::HEADER == *ints) {
                break;
            }
        }

        // We've found the basic block info.
        info = unsafe_cast<basic_block_info *>(ints);
        cache_pc_end = reinterpret_cast<app_pc>(ints);
        cache_pc_start = (cache_pc_end - info->num_bytes) \
                       + info->num_patch_bytes;
        policy = instrumentation_policy::decode(info->policy_bits);

#if GRANARY_IN_KERNEL
        // Initialise the byte states only if we have them.
        if(info->has_delay_range) {
            pc_byte_states = reinterpret_cast<uint8_t *>(
                ints + sizeof(basic_block_info));
        } else {
            pc_byte_states = nullptr;
        }
#endif
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

        // Quick check: if we don't have any delay ranges then we never
        // allocated the state bits.
        if(!pc_byte_states) {
            return false;
        }

        // Interrupted in stub code (<) OR at the first non-stub code
        // instruction (=).
        //
        // Note: This implies that code within a delay region should not
        //       use features that would cause a stub to be generated.
        if(cache_pc_current <= cache_pc_start) {
            return false;
        }

        app_pc start_with_stub(cache_pc_start - info->num_patch_bytes);
        const unsigned current_offset(cache_pc_current - start_with_stub);
        const unsigned end_offset(info->num_bytes);

        ASSERT(current_offset < info->num_bytes);

        code_cache_byte_state byte_state(get_state(
            pc_byte_states, current_offset));

        enum {
            SAFE_INTERRUPT_STATE = BB_BYTE_NATIVE | BB_BYTE_DELAY_BEGIN
        };

        if(0 != (SAFE_INTERRUPT_STATE & byte_state)) {
            begin = nullptr;
            end = nullptr;
            return false;
        }

        for(unsigned i(current_offset); i < end_offset; ++i) {
            if(BB_BYTE_DELAY_END == get_state(pc_byte_states, i)) {
                end = start_with_stub + i + 1;
                break;
            }
        }

        for(unsigned i(0); i < current_offset; ++i) {
            if(SAFE_INTERRUPT_STATE & get_state(pc_byte_states, i)) {
                begin = start_with_stub + i;
            }
        }

        return end && begin;
    }
#endif


    /// Compute the size of a basic block given an instruction list. This
    /// computes the size of each instruction, the amount of padding, meta
    /// information, etc.
    unsigned basic_block::size(instruction_list &ls) throw() {

#if GRANARY_IN_KERNEL
        unsigned size(0);
        bool has_delay(false);
        instruction in(ls.first());

        for(; in.is_valid(); in = in.next()) {
            size += in.encoded_size();
            if(DELAY_INSTRUCTION & in.instr->granary_flags) {
                has_delay = true;
            }
        }
#else
        unsigned size(ls.encoded_size());
#endif

        // Alignment and meta info.
        size += ALIGN_TO(size, BB_INFO_BYTE_ALIGNMENT);
        size += sizeof(basic_block_info);

#if GRANARY_IN_KERNEL
        // State set, where pairs of bits represent the state of a byte within
        // the instructions.
        if(has_delay) {
            unsigned num_instruction_bits(size * BITS_PER_STATE);
            num_instruction_bits += ALIGN_TO(
                num_instruction_bits, BITS_PER_QWORD);
            size += num_instruction_bits / BITS_PER_BYTE;
        }
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
        if(pc_byte_states) {
            unsigned num_instruction_bits(info->num_bytes * BITS_PER_STATE);
            num_instruction_bits += ALIGN_TO(
                num_instruction_bits, BITS_PER_QWORD);
            size_ += num_instruction_bits / BITS_PER_BYTE;
        }
#endif
        return size_;
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
                instruction jmp_try_loop(ls.insert_before(try_loop,
                    mangled(jmp_(instr_(try_loop)))));

                instruction do_loop(ls.insert_before(try_loop, label_()));
                ls.insert_before(try_loop, jmp_(target));

                jmp_try_loop.set_pc(in.pc());
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


    /// Try to decode a `RDRAND` instruction.
    static bool decode_rdrand(instruction in, app_pc pc) throw() {
        UNUSED(in);
        UNUSED(pc);
        return false;
    }


    /// Decode and translate a single basic block of application/module code.
    basic_block basic_block::translate(
        instrumentation_policy policy,
        cpu_state_handle cpu,
        app_pc start_pc
    ) throw() {

        app_pc local_pc(start_pc);
        app_pc *pc(&local_pc);
        uint8_t *generated_pc(nullptr);
        instruction_list ls;

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
        const app_pc detach_app_pc(unsafe_cast<app_pc>(&detach));
        unsigned byte_len(0);

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

            app_pc decoded_pc(*pc);
            in = instruction::decode(pc);

            // TODO: curiosity.
            if(dynamorio::OP_INVALID == in.op_code()
            || dynamorio::OP_UNDECODED == in.op_code()) {

                // TODO: DynamoRIO currently does not support `RDRAND`, so see
                //       if we can special case it for the time being.
                if(decode_rdrand(in, decoded_pc)) {

                }

#if CONFIG_ENABLE_ASSERTIONS
                printf(
                    "Failed to decode instruction at %p in "
                    "block starting at %p\n",
                    in.pc(), start_pc);
#endif /* CONFIG_ENABLE_ASSERTIONS */
                granary_fault();
                USED(in); // To help with debugging.
                break;
            }

            byte_len += in.instr->length;
            ls.append(in);

            if(in.is_cti()) {
                operand target(in.cti_target());

                // Direct branch (e.g. un/conditional branch, jmp, call).
                if(dynamorio::opnd_is_pc(target)) {
                    app_pc target_pc(dynamorio::opnd_get_pc(target));
                    if(detach_app_pc == target_pc) {
                        fall_through_detach = true;
                        if(in.is_jump()) {
                            detach_tail_call = true;
                        }
                        break;
                    }
                }

                // Unconditional JMP; ends the block, without possibility
                // of falling through.
                if(in.is_jump()) {
                    fall_through_pc = false;
                    break;

                // CALL, if direct returns are supported then the basic
                // block that can continue; however, if direct returns
                // aren't supported then we need to support a fall-through
                // JMP to the next basic block.
                } else if(in.is_call()) {
#if !CONFIG_ENABLE_DIRECT_RETURN
                    fall_through_pc = true;
                    break;
#endif

                // RET instruction.
                } else if(in.is_return() || dynamorio::OP_iret == in.op_code()) {
                    break;

                // Conditional CTI, end the block with the ability to fall-
                // through.
                } else {
                    fall_through_pc = true;
                    break;
                }


            // some other instruction
            } else {

#if GRANARY_IN_KERNEL
                if(dynamorio::OP_sysexit == in.op_code()
                || dynamorio::OP_sysret == in.op_code()) {
                    break;
                }
#endif

#if CONFIG_TRACK_XMM_REGS
                // update the policy to be in an xmm context.
                if(!uses_xmm && dynamorio::instr_is_sse_or_sse2(in)) {
                    policy.in_xmm_context();
                    uses_xmm = true;
                }
#endif /* CONFIG_TRACK_XMM_REGS */
            }
        }

        // Translate loops and resolve local branches into jmps to instructions.
        // done before mangling, as mangling removes the opportunity to do this
        // type of transformation.
        translate_loops(ls);

        // Add in a trailing jump if the last instruction in the basic
        // block if we need to force a connection between this basic block
        // and the next.
        if(fall_through_pc) {
            instruction fall_through_cti(ls.append(jmp_(pc_(*pc))));
            fall_through_cti.set_pc(*pc);

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
                fall_through_cti.set_pc(*pc);
            }
        }

        // Invoke client code instrumentation on the basic block; the client
        // might return a different instrumentation policy to use. The effect
        // of this is that if we are in policy P1, and the client returns policy
        // P2, then we will emit a block to P1's code cache that jumps us into
        // P2's code cache.
        instrumentation_policy client_policy(policy.instrument(
            cpu,
            *block_storage,
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
        instruction_list_mangler mangler(
            cpu, block_storage, client_policy);
        mangler.mangle(ls);

        // Re-calculate the size and re-allocate; if our earlier
        // guess was too small then we need to re-instrument the
        // instruction list
        generated_pc = cpu->fragment_allocator.\
            allocate_array<uint8_t>(basic_block::size(ls));

        IF_TEST( app_pc emitted_pc = ) emit(
            policy, ls, bb_begin, block_storage,
            start_pc, byte_len,
            generated_pc);

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
        unsigned byte_len,
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
        info->policy_bits = policy.encode();
        info->generating_num_bytes = byte_len;
        info->generating_pc = reinterpret_cast<uintptr_t>(generating_pc);
        info->state_addr = block_storage;

        // fill in the byte state set
        pc += sizeof(basic_block_info);
        IF_KERNEL( pc += initialise_state_bytes(info, ls.first(), pc); )

        return start_pc;
    }
}


