/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * descriptor.cc
 *
 *  Created on: 2013-08-18
 *      Author: Peter Goodman
 */

#include "granary/client.h"

#include "clients/watchpoints/clients/rcudbg/instrument.h"
#include "clients/watchpoints/clients/rcudbg/descriptor.h"
#include "clients/watchpoints/clients/rcudbg/log.h"
#include "clients/watchpoints/clients/rcudbg/carat.h"

using namespace granary;

namespace client {


    /// Initialisation for an `rcu_dereference`d pointer.
    bool rcudbg_descriptor::allocate_and_init(
        rcudbg_descriptor *, // descriptor, unused
        uintptr_t &counter_index,
        uintptr_t, // inherited index, unused
        rcudbg_watched_address deref_address,
        const char *carat
    ) throw() {

        // Get the read-side critical section ID for this task.
        IF_KERNEL( eflags flags = granary_disable_interrupts(); )
        thread_state_handle thread = safe_cpu_access_zone();
        const uint16_t section_id(thread->read_section_id);
        const void *task(thread.state);
        const char *read_lock_carat(thread->section_carat);
        IF_KERNEL( granary_store_flags(flags); )

        // Doing a de-reference on an unwatched address. Issue a warning that
        // there was no matching `rcu_assign_pointer` for this
        // `rcu_dereference`. This is only treated as a warning and not as an
        // error because a writer might, for example, write a chain of list
        // elements into an RCU-protected linked list, for which only the head
        // element of the list must be assigned.
        if(!wp::is_watched_address(deref_address.as_uint)) {
            log(DEREF_UNWATCHED_ADDRESS,
                task,
                deref_address.as_pointer,
                carat,
                read_lock_carat);

        // A double-dereference has occurred.
        } else if(deref_address.is_deref) {

            // Issue a warning that this is potentially redundant, or
            // potentially indicative of an error.
            if(deref_address.read_section_id == section_id) {
                log(DOUBLE_DEREF,
                    task,
                    carat,
                    read_lock_carat);

            // Issue an error that this pointer was de-referenced in one
            // read-side critical section, and then de-referenced again in
            // another.
            } else {
                log(DOUBLE_DEREF_WRONG_SECTION,
                    task,
                    carat,
                    read_lock_carat,
                    get_location_carat(deref_address.assign_location_id));
            }

        // We are dereferencing a pointer that was created using
        // `rcu_assign_pointer`.
        } else {
            // TODO
        }

        rcudbg_counter_index index;
        index.is_deref = true;
        index.read_section_id = section_id;

        counter_index = index.as_uint;
        return true;
    }


    /// Initialisation for an `rcu_assign_pointer`d pointer.
    bool rcudbg_descriptor::allocate_and_init(
        rcudbg_descriptor *, // descriptor, unused
        uintptr_t &counter_index,
        uintptr_t, // inherited index, unused
        rcudbg_watched_address assign_pointer,
        rcudbg_watched_address assigned_pointer,
        const char *carat
    ) throw() {

        // We are assigning to a watched address.
        if(wp::is_watched_address(assign_pointer.as_uint)) {

            // This is not good, we're doing the equivalent of:
            // rcu_assign_pointer(rcu_dereference(p), q). This could be
            // indicative of a pointer leaking from a critical section. We'll
            // report the most recent carat for that section ID.
            //
            // Note: The reported carat might be out-of-date.
            if(assign_pointer.is_deref) {

                // TODO

            // We're assigning to a previously assigned pointer. This is good
            // because it gives us information about where inside of a given
            // structure the pointers to other RCU-protected objects are.
            // Because we don't track types or maintain descriptors, we'll fall
            // back on a global, sort-of probabilistic approach to detecting
            // reads that don't do rcu_dereference on this pointer by using
            // shadow memory.
            } else {
                // TODO
            }

        } else {
            // TODO: Is it worth it to record these addresses into a heavier
            //       weight shadow memory mechanism?
        }

        return true;
    }
}
