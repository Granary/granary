/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * basic_block_info.cc
 *
 *  Created on: 2013-11-05
 *      Author: Peter Goodman
 */

#include "granary/globals.h"
#include "granary/basic_block.h"
#include "granary/state.h"


namespace granary {

    enum {
        SLAB_SIZE = detail::fragment_allocator_config::SLAB_SIZE,
        BB_ALIGN_ = detail::fragment_allocator_config::MIN_ALIGN,
        BB_ALIGN = 1 == BB_ALIGN_ ? 16 : BB_ALIGN_,
#if CONFIG_FOLLOW_CONDITIONAL_BRANCHES
        MULTIPLIER = 4,
#else
        MULTIPLIER = 1,
#endif
        MAX_BBS_PER_SLAB = MULTIPLIER * ((SLAB_SIZE / BB_ALIGN) + 1)
    };


    struct basic_block_info_ptr {
        basic_block_info *ptr;

        inline bool operator<(const app_pc that) const throw() {
            return ptr->start_pc < that;
        }

        inline bool operator==(const app_pc that) const throw() {
            return ptr->start_pc <= that
                && that < (ptr->start_pc + ptr->num_bytes);
        }
    };


    static basic_block_info *search_basic_block_info(
        basic_block_info_ptr *array,
        const long max,
        app_pc cache_pc
    ) throw() {
        if(!max) {
            return nullptr;
        }

        long first(0);
        long last(max - 1);
        long middle((first + last) / 2);

        for(; first <= middle && middle <= last; ) {
            basic_block_info_ptr curr(array[middle]);
            if(curr.ptr->start_pc <= cache_pc) {
                if(cache_pc < (curr.ptr->start_pc + curr.ptr->num_bytes)) {
                    return curr.ptr;
                } else {
                    first = middle + 1;
                }
            } else {
                last = middle - 1;
            }
            middle = (first + last) / 2;
        }

        return nullptr;
    }


    struct fragment_locator {
        unsigned next_index;
        basic_block_info_ptr fragments[MAX_BBS_PER_SLAB];
    };


    extern "C" {
        extern fragment_locator **granary_find_fragment_slab(app_pc);
    }


    /// Allocate basic block data. Basic block data includes both basic block
    /// info (meta-data) and client state (client meta-data).
    ///
    /// Pointers are returned by reference through output parameters.
    basic_block_info *allocate_basic_block_info(
        app_pc start_pc
        _IF_KERNEL( unsigned num_state_bytes )
        _IF_KERNEL( uint8_t *&state_bytes )
    ) throw() {
        // Make sure the fragment locator for this fragment slab exists.
        fragment_locator **slab_(granary_find_fragment_slab(start_pc));
        if(unlikely(!*slab_)) {
            *slab_ = allocate_memory<fragment_locator>();
        }

        fragment_locator *slab(*slab_);

#if GRANARY_IN_KERNEL
#   if CONFIG_ENABLE_INTERRUPT_DELAY
        basic_block_info *info(nullptr);
        if(num_state_bytes) {
            uint8_t *memory(allocate_memory<uint8_t>(
                sizeof(basic_block_info) + num_state_bytes));
            info = unsafe_cast<basic_block_info *>(memory);
            state_bytes = &(memory[sizeof(basic_block_info)]);
        } else {
            state_bytes = nullptr;
            info = allocate_memory<basic_block_info>();
        }
#   else
        UNUSED(num_state_bytes);

        basic_block_info *info(allocate_memory<basic_block_info>());
        state_bytes = nullptr;
#   endif
#else
        basic_block_info *info(allocate_memory<basic_block_info>());
#endif

        ASSERT(slab->next_index < MAX_BBS_PER_SLAB);

        slab->fragments[slab->next_index++].ptr = info;
        return info;
    }


    /// Find the basic block info given an address into our code cache of
    /// basic blocks.
    __attribute__((hot))
    basic_block_info *find_basic_block_info(app_pc cache_pc) throw() {
        fragment_locator **slab_(granary_find_fragment_slab(cache_pc));
        fragment_locator *slab(*slab_);

        ASSERT(nullptr != slab);

        basic_block_info *info(search_basic_block_info(
            &(slab->fragments[0]), slab->next_index, cache_pc));

        ASSERT(nullptr != info);

        return info;
    }


    /// Try to delete a fragment locator, but don't necessarily succeed.
    ///
    /// Note: We assume that there is a coarse grained lock that is guarding
    ///       these operations!
    bool try_remove_basic_block_info(app_pc cache_pc) throw() {
        fragment_locator **slab_(granary_find_fragment_slab(cache_pc));
        fragment_locator *slab(*slab_);

        ASSERT(nullptr != slab);

        if(!slab->next_index) {
            return false;
        }

        basic_block_info_ptr &frag(slab->fragments[slab->next_index - 1]);
        ASSERT(nullptr != frag.ptr);

        if(frag.ptr->start_pc > cache_pc) {
            return false;
        }

#if GRANARY_IN_KERNEL
#   if CONFIG_ENABLE_INTERRUPT_DELAY
        if(frag.ptr->delay_states) {
            free_memory<uint8_t>(
                unsafe_cast<uint8_t *>(frag.ptr),
                sizeof(basic_block_info) + frag.ptr->num_delay_state_bytes);
        } else {
            free_memory<basic_block_info>(frag.ptr);
        }
#   else
        free_memory<basic_block_info>(frag.ptr);
#   endif
#else
        free_memory<basic_block_info>(frag.ptr);
#endif

        frag.ptr = nullptr;
        slab->next_index -= 1;
        return true;
    }
}

