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
        MAX_BBS_PER_SLAB = (SLAB_SIZE / BB_ALIGN) + 1
    };


    /// GDB helper variable. GDB doesn't always have access to the value of
    /// the `SLAB_SIZE` symbol (either the above defined one, or the one inside
    /// the `fragment_allocator_config`), so we put this variable here to
    /// give it something more "solid", so that we can duplicate the block info
    /// lookup procedure in `.gdbinit`.
    __attribute__((used))
    const unsigned FRAGMENT_SLAB_SIZE = SLAB_SIZE;


    struct generic_info_ptr {

        union {
            basic_block_info *block;
            trace_info *trace;

            struct {
                bool is_trace:1; // low
                uintptr_t:63; // high
            } __attribute__((packed));
        } __attribute__((packed));


        inline app_pc start_pc(void) const throw() {
            if(is_trace) {
                generic_info_ptr untraced_ptr(*this);
                untraced_ptr.is_trace = false;
                return untraced_ptr.trace->start_pc;
            } else {
                return block->start_pc;
            }
        }


        inline app_pc end_pc(void) const throw() {
            if(is_trace) {
                generic_info_ptr untraced_ptr(*this);
                untraced_ptr.is_trace = false;
                return untraced_ptr.trace->start_pc \
                     + untraced_ptr.trace->num_bytes;
            } else {
                return block->start_pc + block->num_bytes;
            }
        }


        /// Get the basic block info from this trace data.
        const basic_block_info *get_block(app_pc cache_pc) const throw() {
            if(is_trace) {
                generic_info_ptr untraced_ptr(*this);
                untraced_ptr.is_trace = false;
                const trace_info *raw_trace(untraced_ptr.trace);
                const basic_block_info *blocks(raw_trace->info);
                for(unsigned i(0); i < raw_trace->num_blocks; ++i) {
                    if(blocks[i].start_pc <= cache_pc
                    && cache_pc < (blocks[i].start_pc + blocks[i].num_bytes)) {
                        return &(blocks[i]);
                    }
                }
            } else {
                return block;
            }

            ASSERT(false);
            return nullptr;
        }
    };


    static const basic_block_info *search_basic_block_info(
        generic_info_ptr *array,
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
            const generic_info_ptr info_ptr(array[middle]);
            const app_pc trace_start_pc(info_ptr.start_pc());
            const app_pc trace_end_pc(info_ptr.end_pc());

            if(trace_start_pc <= cache_pc) {
                if(cache_pc < trace_end_pc) {
                    return info_ptr.get_block(cache_pc);
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
        generic_info_ptr fragments[MAX_BBS_PER_SLAB];
    };


    extern "C" {
        extern fragment_locator **granary_find_fragment_slab(app_pc);
    }


    /// Commit to storing information about a trace.
    void store_trace_meta_info(const trace_info &trace) throw() {

        // Make sure the fragment locator for this fragment slab exists.
        fragment_locator **slab_(granary_find_fragment_slab(trace.start_pc));
        if(unlikely(!*slab_)) {
            *slab_ = allocate_memory<fragment_locator>();
        }

        // Handle the case of having a very large trace that spans multiple
        // slabs. This can easily happen for heavyweight instrumentation like
        // watchpoints and with kernel code like `copy_process`.
        fragment_locator **end_slab_(granary_find_fragment_slab(
            trace.start_pc + trace.num_bytes - 1));
        if(unlikely(end_slab_ != slab_)) {
            for(fragment_locator **next_slab_(slab_ + 1);
                next_slab_ <= end_slab_;
                ++next_slab_) {

                ASSERT(nullptr == *next_slab_);

                // Initialize using the same slab.
                *next_slab_ = *slab_;
            }
        }

        fragment_locator *slab(*slab_);

        ASSERT(0 < trace.num_blocks);
        ASSERT(slab->next_index < MAX_BBS_PER_SLAB);

        generic_info_ptr &info_ptr(slab->fragments[slab->next_index++]);

        if(1 == trace.num_blocks) {
            info_ptr.block = trace.info;
        } else {
            trace_info *perm_trace(allocate_memory<trace_info>());
            memcpy(perm_trace, &trace, sizeof trace);
            info_ptr.trace = perm_trace;
            info_ptr.is_trace = true;
        }
    }


    /// Find the basic block info given an address into our code cache of
    /// basic blocks.
    __attribute__((hot))
    const basic_block_info *find_basic_block_info(app_pc cache_pc) throw() {
        fragment_locator **slab_(granary_find_fragment_slab(cache_pc));
        fragment_locator *slab(*slab_);

        ASSERT(nullptr != slab);

        const basic_block_info *info(search_basic_block_info(
            &(slab->fragments[0]), slab->next_index, cache_pc));

        ASSERT(nullptr != info);
        ASSERT(info->start_pc <= cache_pc);
        ASSERT(cache_pc < (info->start_pc + info->num_bytes));

        return info;
    }


    /// Remove the basic block info for some `cache_pc`. This is only valid
    /// if the associated basic block was the last block added to this
    /// fragment allocator, and if it wasn't added as part of a trace.
    ///
    /// Note: We assume that there is a coarse grained lock that is guarding
    ///       these operations!
    void remove_basic_block_info(app_pc cache_pc) throw() {
        fragment_locator **slab_(granary_find_fragment_slab(cache_pc));
        fragment_locator *slab(*slab_);

        ASSERT(nullptr != slab);
        ASSERT(slab->next_index);

        generic_info_ptr &frag(slab->fragments[slab->next_index - 1]);

        ASSERT(!frag.is_trace);
        ASSERT(nullptr != frag.block);
        ASSERT(frag.block->start_pc <= cache_pc);

#if CONFIG_ENV_KERNEL && CONFIG_FEATURE_INTERRUPT_DELAY
        if(frag.block->delay_states) {
            free_memory(
                frag.block->delay_states, frag.block->num_delay_state_bytes);
        }
#endif

        free_memory(frag.block);

        frag.block = nullptr;
        slab->next_index -= 1;
    }
}

