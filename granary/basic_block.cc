/*
 * bb.cc
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#include "granary/basic_block.h"
#include "granary/instruction.h"

namespace granary {

    enum {
        BB_INFO_BYTE_ALIGNMENT = 8,
        BB_INFO_INT32_ALIGNMENT = 2,
        BITS_PER_BYTE = 8,
        BITS_PER_STATE = BITS_PER_BYTE / BB_BYTE_STATES_PER_BYTE,
        BITS_PER_QWORD = BITS_PER_BYTE * 8
    };

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


    /// Returns the next safe interrupt location.
    app_pc basic_block::next_safe_interrupt_location(void) const throw() {
        app_pc next(cache_pc_current);

        for(;;) {
            const code_cache_byte_state byte_state(state_of(next));

            // we're in front of a native byte, which is fine.
            if(BB_BYTE_NATIVE & byte_state) {
                return next;

            // In the case of a mangled instruction, we will follow the discipline
            // that there can only be one mangled instruction in a translated
            // fragment, which is a safe interrupt point.
            //
            // otherwise, we should be in a different kind of basic block
            // altogether.
            } else if(BB_BYTE_MANGLED & byte_state) {
                if(BB_TRANSLATED_FRAGMENT == info->kind) {
                    return next;
                }

                return nullptr;

            // we might need to delay, but only if the previous byte is instrumented
            // if the previous byte is instrumented, then it likely means we're in
            // a sequence of instrumented instruction, which might not be re-entrant
            // and so we need to advance the pc to find a safe interrupt location.
            } else if(BB_BYTE_INSTRUMENTED & byte_state) {

                if(next == cache_pc_start) {
                    return next;
                }

                if(BB_BYTE_NATIVE == state_of(next - 1)) {
                    return next;
                }

                ++next;

            // this is bad; we should never be executing padding instructions
            // (which are interrupts)
            } else {
                return nullptr;
            }
        }
    }

    /// Compute the size of a basic block given an instruction list. This
    /// computes the size of each instruction, the amount of padding, meta
    /// information, etc.
    unsigned basic_block::size(instruction_list &ls) throw() {
        unsigned size(0);
        auto in = ls.first();
        for(unsigned i = 0, max = ls.length(); i < max; ++i) {
            size += in->size();
        }

        size += size % BB_INFO_BYTE_ALIGNMENT; // padding bytes before meta info
        size += sizeof(basic_block_info); // meta info

        // counting set for the bits that have state info about the instruction
        // bytes
        unsigned num_instruction_bits(size * BITS_PER_STATE);
        num_instruction_bits += num_instruction_bits % BITS_PER_QWORD;

        size += num_instruction_bits / BITS_PER_BYTE;

        return size;
    }

    /// Compute the size of an existing basic block.
    unsigned basic_block::size(void) const throw() {
        unsigned size((pc_byte_states - cache_pc_start));
        unsigned num_instruction_bits(info->num_bytes * BITS_PER_STATE);
        num_instruction_bits += num_instruction_bits % BITS_PER_QWORD;
        size += num_instruction_bits / BITS_PER_BYTE;

        return size;
    }
}


