/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * user_address.cc
 *
 *  Created on: 2013-11-05
 *      Author: Peter Goodman
 */


#include "granary/instruction.h"
#include "granary/detach.h"

namespace granary {

    /// Special case ranges of kernel code that have been observed to access
    /// user space data. Some/all? of these don't *appear* to use the correct
    /// APIs, and so the pattern matching fails on them.
    ///
    /// TODO: Do further investigation and potentially report these functions
    ///       as having bugs.
    static const app_pc USER_ACCESS_CODE[][2] = {
    #   ifdef DETACH_ADDR_memcpy
        {(app_pc) DETACH_ADDR_memcpy,
         (app_pc) DETACH_ADDR_memcpy + DETACH_LENGTH_memcpy},
    #   endif
    #   ifdef DETACH_ADDR_csum_partial_copy_generic
        {(app_pc) DETACH_ADDR_csum_partial_copy_generic,
         (app_pc) DETACH_ADDR_csum_partial_copy_generic + DETACH_LENGTH_csum_partial_copy_generic},
    #   endif
    #   ifdef DETACH_ADDR_rcu_process_callbacks
        {(app_pc) DETACH_ADDR_rcu_process_callbacks,
         (app_pc) DETACH_ADDR_rcu_process_callbacks + DETACH_LENGTH_rcu_process_callbacks},
    #   endif
    #   ifdef DETACH_ADDR_flush_tlb_mm_range
        {(app_pc) DETACH_ADDR_flush_tlb_mm_range,
         (app_pc) DETACH_ADDR_flush_tlb_mm_range + DETACH_LENGTH_flush_tlb_mm_range},
    #   endif
    #   ifdef DETACH_ADDR___kmalloc
        {(app_pc) DETACH_ADDR___kmalloc,
         (app_pc) DETACH_ADDR___kmalloc + DETACH_LENGTH___kmalloc},
    #   endif
    #   ifdef DETACH_ADDR___kmalloc_node
        {(app_pc) DETACH_ADDR___kmalloc_node,
         (app_pc) DETACH_ADDR___kmalloc_node + DETACH_LENGTH___kmalloc_node},
    #   endif
    #   ifdef DETACH_ADDR_kmem_cache_alloc
        {(app_pc) DETACH_ADDR_kmem_cache_alloc,
         (app_pc) DETACH_ADDR_kmem_cache_alloc + DETACH_LENGTH_kmem_cache_alloc},
    #   endif
    #   ifdef DETACH_ADDR___kmalloc_node_track_caller
        {(app_pc) DETACH_ADDR___kmalloc_node_track_caller,
         (app_pc) DETACH_ADDR___kmalloc_node_track_caller + DETACH_LENGTH___kmalloc_node_track_caller},
    #   endif
    #   ifdef DETACH_ADDR___kmalloc_track_caller
        {(app_pc) DETACH_ADDR___kmalloc_track_caller,
         (app_pc) DETACH_ADDR___kmalloc_track_caller + DETACH_LENGTH___kmalloc_track_caller},
    #   endif
    #   ifdef DETACH_ADDR_kmem_cache_alloc_node
        {(app_pc) DETACH_ADDR_kmem_cache_alloc_node,
         (app_pc) DETACH_ADDR_kmem_cache_alloc_node + DETACH_LENGTH_kmem_cache_alloc_node},
    #   endif
    #   ifdef DETACH_ADDR_release_sock
        {(app_pc) DETACH_ADDR_release_sock,
         (app_pc) DETACH_ADDR_release_sock + DETACH_LENGTH_release_sock},
    #   endif
        {nullptr, nullptr}
    };


    /// Try to detect if the basic block contains a specific binary instruction
    /// pattern that makes it look like it could contain and exception table
    /// entry.
    bool kernel_code_accesses_user_data(
        instruction_list &ls,
        const app_pc start_pc
    ) throw() {

        // Go through the array of exceptions.
        for(unsigned i(0); USER_ACCESS_CODE[i][0]; ++i) {
            if(USER_ACCESS_CODE[i][0] <= start_pc
            && start_pc < USER_ACCESS_CODE[i][1]) {
                return true;
            }
        }

        enum {
            DATA32_XCHG_AX_AX = 0x906666U,
            DATA32_DATA32_XCHG_AX_AX = 0x90666666U
        };

        const unsigned data32_xchg_ax_ax(DATA32_XCHG_AX_AX);
        const unsigned data32_data32_xchg_ax_ax(DATA32_DATA32_XCHG_AX_AX);

        for(instruction in(ls.first()); in.is_valid(); in = in.next()) {
            const app_pc bytes(in.pc_or_raw_bytes());

            // E.g. copy_user_enhanced_fast_string, doesn't contain
            // the binary pattern. Unfortunately, this will also capture
            // memcpy and others.
            if(dynamorio::OP_ins <= in.op_code()
            && dynamorio::OP_repne_scas >= in.op_code()) {
                return true;
            }

            if(nullptr == bytes) {
                continue;
            }

            if(3 == in.encoded_size()) {
                if(0 == memcmp(bytes, &data32_xchg_ax_ax, 3)) {
                    return true;
                }
            } else if(4 == in.encoded_size()) {
                if(0 == memcmp(bytes, &data32_data32_xchg_ax_ax, 4)) {
                    return true;
                }
            }
        }

        return false;
    }


    extern "C" {
        extern void *kernel_search_exception_tables(void *);
    }


    /// Try to get a kernel exception table entry for any instruction within
    /// an instruction list.
    ///
    /// Note: This is very Linux-specific!!
    void *kernel_find_exception_metadata(instruction_list &ls) throw() {

        for(instruction in(ls.first()); in.is_valid(); in = in.next()) {
            app_pc pc(in.pc_or_raw_bytes());
            if(!pc) {
                continue;
            }

            void *ret(kernel_search_exception_tables(pc));
            if(ret) {
                return ret;
            }
        }

        return nullptr;
    }
}

