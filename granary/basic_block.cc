/*
 * bb.cc
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#include "granary/basic_block.h"
#include "granary/instruction.h"

#include <cstdio>

namespace granary {

    enum {
        /// the magic value is a 3 int3 instructions, followed by the number of
        /// instructions in the basic block.
        BB_PADDING      = 0xCC,
        BB_MAGIC        = 0xCCCCCC00,
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
        BITS_PER_QWORD = BITS_PER_BYTE * 8
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
            0,  /* 1 << 0 */
            1,  /* 1 << 1 */
            0xFF,
            2,  /* 1 << 2 */
            0xFF,
            0xFF,
            0xFF,
            3   /* 1 << 3 */
        };

        const unsigned j(i / BB_BYTE_STATES_PER_BYTE); // byte offset
        const unsigned k(2 * (i % BB_BYTE_STATES_PER_BYTE)); // bit offset
        pc_byte_states[j] &= ~(0x3 << k);
        pc_byte_states[j] |= state_to_mask[state] << k;
    }


    /// Get the state of all bytes of an instruction.
    static code_cache_byte_state get_instruction_state(const instruction in) throw() {
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
    static unsigned initialize_state_bytes(basic_block_info *info,
                                           instruction_list &ls,
                                           app_pc pc) throw() {


        unsigned num_instruction_bits(info->num_bytes * BITS_PER_STATE);
        num_instruction_bits += ALIGN_TO(num_instruction_bits, BITS_PER_QWORD);
        unsigned num_state_bytes = num_instruction_bits / BITS_PER_BYTE;

        // initialize all state bytes to have their states as padding bytes
        memset(pc, ~0, num_state_bytes);

        auto in(ls.first());
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

        // round up to the next 64-bit aligned
        uint64_t addr(reinterpret_cast<uint64_t>(current_pc_));
        unsigned num_bytes = addr % sizeof(uint64_t);
        addr += num_bytes;

        // go for 8-byte aligned addresses, look for a 32 bit sequence
        uint32_t *ints(reinterpret_cast<uint32_t *>(addr));
        for(; (BB_MAGIC_MASK & *ints) != BB_MAGIC;
            ints += BB_INFO_INT32_ALIGNMENT) {
            num_bytes += BB_INFO_BYTE_ALIGNMENT;
        }

        // we've found the basic block info
        info = unsafe_cast<basic_block_info *>(ints);
        cache_pc_end = cache_pc_current + num_bytes;
        cache_pc_start = cache_pc_current - (info->num_bytes - num_bytes);
        pc_byte_states = reinterpret_cast<uint8_t *>(
            cache_pc_end + sizeof(basic_block_info));
    }


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


    /// Compute the size of a basic block given an instruction list. This
    /// computes the size of each instruction, the amount of padding, meta
    /// information, etc.
    unsigned basic_block::size(instruction_list &ls) throw() {
        unsigned size(ls.encoded_size());

        // alignment and meta info
        size += ALIGN_TO(size, BB_INFO_BYTE_ALIGNMENT);

        // counting set for the bits that have state info about the instruction
        // bytes
        unsigned num_instruction_bits(size * BITS_PER_STATE);
        num_instruction_bits += ALIGN_TO(num_instruction_bits, BITS_PER_QWORD);

        size += sizeof(basic_block_info); // meta info
        size += num_instruction_bits / BITS_PER_BYTE;
        return size;
    }


    /// Compute the size of an existing basic block.
    unsigned basic_block::size(void) const throw() {
        unsigned size(info->num_bytes + sizeof *info);
        unsigned num_instruction_bits(info->num_bytes * BITS_PER_STATE);
        num_instruction_bits += ALIGN_TO(num_instruction_bits, BITS_PER_QWORD);
        size += num_instruction_bits / BITS_PER_BYTE;

        return size;
    }


    /// Emit an instruction list as code into a byte array. This will also
    /// emit the basic block meta information.
    ///
    /// Note: it is assumed that pc is well-aligned, e.g. to an 8 or 16 byte
    ///       boundary.
    basic_block basic_block::emit(basic_block_kind kind,
                                  instruction_list &ls,
                                  app_pc generating_pc,
                                  app_pc *generated_pc) throw() {

        app_pc pc = *generated_pc;
        const app_pc start_pc(pc);

        // add in the instructions
        pc = ls.encode(pc);

        uint64_t pc_uint(reinterpret_cast<uint64_t>(pc));
        uint64_t pc_uint_aligned(pc_uint + ALIGN_TO(
            pc_uint, BB_INFO_BYTE_ALIGNMENT));

        // add in the padding
        for(unsigned i = 0, max = (pc_uint_aligned - pc_uint); i < max; ++i) {
            *pc++ = BB_PADDING;
        }

        // add in the meta information header
        *(unsafe_cast<uint32_t *>(pc)) = uint32_t(
            (BB_PADDING << 24) |
            (BB_PADDING << 16) |
            (BB_PADDING <<  8) |
            kind);

        basic_block_info *info(unsafe_cast<basic_block_info *>(pc));

        // fill in the info
        info->num_bytes = static_cast<unsigned>(pc - start_pc);
        info->hotness = 0;
        info->generating_pc = generating_pc;

        // fill in the byte state set
        pc += sizeof(basic_block_info);
        pc += initialize_state_bytes(info, ls, pc);

        *generated_pc = pc;
        return basic_block(start_pc);
    }
}


