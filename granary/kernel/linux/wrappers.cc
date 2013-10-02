/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * wrappers.cc
 *
 *  Created on: 2013-09-26
 *      Author: Peter Goodman
 */

#include "granary/wrapper.h"
#include "granary/state.h"


/// Hot-patch the `process_one_work` function to wrap `work_struct`s before
/// their functions are executed. This is here because there are ways to get
/// work structs registered through macros/inline functions, thus bypassing
/// wrapping.
#if defined(DETACH_ADDR_process_one_work)
    PATCH_WRAPPER_VOID(process_one_work, (struct worker *worker, struct work_struct *work), {
        PRE_OUT_WRAP(work);
        process_one_work(worker, work);
    })
#endif


/// Appears to be used for swapping the user-space GS register value instead of
/// using WRMSR. The benefit of the approach appears to be that there is very
/// little possibility for exceptions using a SWAPGS in this function, whereas
/// WRMSR has a few more exception possibilities.
#if defined(DETACH_ADDR_native_load_gs_index)
    GRANARY_DETACH_ADDR_POINT(DETACH_ADDR_native_load_gs_index);
#endif


/// Wraps the exception table search mechanism (queried during page faults).
/// We implement an unsafe "pacification" mechanism whereby we look for code
/// patterns that look like they might dereference user space addresses and
/// then mark those basic blocks as being special, so that if a page fault
/// happens in them, then the kernel won't over-react and do something unusual.
#if defined(DETACH_ADDR_search_exception_tables)
    PATCH_WRAPPER(search_exception_tables, (const struct exception_table_entry *), (void *pc), {
        cpu_state_handle cpu;
        if(pc == cpu->last_exception_instruction_pointer) {
            return unsafe_cast<const struct exception_table_entry *>(
                cpu->last_exception_table_entry);
        }
        return search_exception_tables(pc);
    });
#endif


/// Wraps memset to try to make sure we never memset over existing code cache
/// code.
#if CONFIG_ENABLE_ASSERTIONS && defined(DETACH_ADDR_memset)
    extern "C" {

        // Defined in module.c, actually as unsigned long long, but for
        // comparison's sake, it's easier to declare them differently here.
        extern uint8_t *EXEC_START;
        extern uint8_t *CODE_CACHE_END;
    }

    PATCH_WRAPPER(memset, (void *), (uint8_t *addr, uint8_t val, size_t size), {

        // Slow path: check that all bytes are zero first.
        if(addr >= EXEC_START && (addr + size) <= CODE_CACHE_END) {
            bool reported_issue(false);
            for(size_t i(0); i < size; ++i) {
                if(!reported_issue && 0 != addr[i]) {
                    granary_break_on_curiosity();
                    reported_issue = true;
                }
            }
        }

        return memset(addr, val, size);
    })
#endif

