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
    PATCH_WRAPPER(search_exception_tables, (const struct exception_table_entry *), (unsigned long add), {
        cpu_state_handle cpu;
        if(cpu->unsafe_pacify_exception_table_search) {
            cpu->unsafe_pacify_exception_table_search = false;

            // Return's a noticably bad value, just in case this is ever
            // de-referenced.
            //
            // Note: bits 48 and 47 are both 1, so this should also not
            //       interfere with behavioural watchpoints.
            return unsafe_cast<const struct exception_table_entry *>(
                0xEA7DEADBEEFFEE1FULL);
        }
        return search_exception_tables(add);
    });
#endif

