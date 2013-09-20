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


    enum {
        MAX_NUM_SHADOW_BITS = 1ULL << 16
    };


    /// `p` for `p` in `rcu_dereference(p, q)` where `p` is unwatched.
    static bitset<MAX_NUM_SHADOW_BITS> UNWATCHED_AND_ASSIGNED_POINTERS;


    /// `p` for `p` in `rcu_dereference(p, q)` where `p` is watched.
    static bitset<MAX_NUM_SHADOW_BITS> WATCHED_AND_ASSIGNED_POINTERS;


    /// Initialisation for an `rcu_dereference`d pointer.
    bool rcudbg_descriptor::allocate_and_init(
        rcudbg_descriptor *, // descriptor, unused
        uintptr_t &counter_index,
        uintptr_t, // inherited index, unused
        rcudbg_watched_address deref_address,
        const char *dereference_carat
    ) throw() {
        bool watch_the_address(true);

        // Get the read-side critical section ID for this thread, as well as an
        // identifying pointer for this thread.
        IF_KERNEL( eflags flags = granary_disable_interrupts(); )
        thread_state_handle thread = safe_cpu_access_zone();
        const uint16_t section_id(thread->section_id);
        const void *thread_id(thread.identifying_address());
        const char *read_lock_carat(thread->section_carat);
        const unsigned section_depth(thread->section_depth);
        IF_KERNEL( granary_store_flags(flags); )

        // Doing a dereference outside of a read-side critical section.
        if(0 == section_depth) {
            log(RCU_DEREFERENCE_OUTSIDE_OF_CRITICAL_SECTION,
                thread_id,
                read_lock_carat,
                dereference_carat);

            watch_the_address = false;
        }

        // Doing a dereference on an unwatched address. Issue a warning that
        // there was no matching `rcu_assign_pointer` for this
        // `rcu_dereference`. This is only treated as a warning and not as an
        // error because a writer might, for example, write a chain of list
        // elements into an RCU-protected linked list, for which only the head
        // element of the list must be assigned.
        if(!wp::is_watched_address(deref_address.as_uint)) {

            if(!UNWATCHED_AND_ASSIGNED_POINTERS.get(deref_address.shadow_offset)) {
                log(RCU_DEREFERENCE_UNASSIGNED_ADDRESS,
                    thread_id,
                    deref_address.as_pointer,
                    dereference_carat,
                    read_lock_carat);
            }

        // A double-dereference has occurred.
        } else if(deref_address.is_deref) {

            // Issue a warning that this is potentially redundant, or
            // potentially indicative of an error.
            if(deref_address.read_section_id == section_id) {
                log(DOUBLE_RCU_DEREFERENCE,
                    thread_id,
                    dereference_carat,
                    read_lock_carat);

            // Issue an error that this pointer was de-referenced in one
            // read-side critical section, and then de-referenced again in
            // another.
            } else {
                log(DOUBLE_RCU_DEREFERENCE_WRONG_SECTION,
                    thread_id,
                    dereference_carat,
                    read_lock_carat,
                    get_location_carat(deref_address.assign_location_id));
            }

        // We are dereferencing a pointer that was created using
        // `rcu_assign_pointer`.
        } else {
            if(!WATCHED_AND_ASSIGNED_POINTERS.get(deref_address.shadow_offset)) {
                log(RCU_DEREFERENCE_UNASSIGNED_ADDRESS,
                    thread_id,
                    deref_address.as_pointer,
                    dereference_carat,
                    read_lock_carat);
            }
        }

        // Inherit the section ID from the current thread.
        rcudbg_counter_index index;
        index.is_deref = true;
        index.read_section_id = section_id;

        set_section_carat(section_id, SECTION_DEREF_CARAT, dereference_carat);

        counter_index = index.as_uint;
        return watch_the_address;
    }


    /// Initialisation for an `rcu_assign_pointer`d pointer.
    bool rcudbg_descriptor::allocate_and_init(
        rcudbg_descriptor *, // descriptor, unused
        uintptr_t &counter_index,
        uintptr_t, // inherited index, unused
        rcudbg_watched_address assign_pointer,
        rcudbg_watched_address assigned_pointer,
        const char *assign_carat
    ) throw() {

        // Get an identifying pointer for the current thread.
        IF_KERNEL( eflags flags = granary_disable_interrupts(); )
        thread_state_handle thread = safe_cpu_access_zone();
        const unsigned section_depth(thread->section_depth);
        const char *section_carat(thread->section_carat);
        const void *thread_id(thread.identifying_address());
        IF_KERNEL( granary_store_flags(flags); )

        // We are assigning withing a read-side critical section.
        if(0 != section_depth) {
            log(RCU_ASSIGN_IN_CRITICAL_SECTION,
                thread_id,
                section_carat,
                assign_carat);
        }

        // We are assigning to a watched address.
        if(wp::is_watched_address(assign_pointer.as_uint)) {

            // This is not good, we're doing the equivalent of:
            //      rcu_assign_pointer(rcu_dereference(p), q)
            // This could be indicative of a pointer leaking from a critical
            // section. We'll report the most recent carat for that section ID.
            //
            // Note: The reported carat might be out-of-date.
            if(assign_pointer.is_deref) {

                log(RCU_ASSIGN_TO_RCU_DEREFERENCED_POINTER,
                    thread_id,
                    get_section_carat(
                        assign_pointer.read_section_id,
                        SECTION_DEREF_CARAT),
                    assign_carat);

            // We're assigning to a previously assigned pointer. This is good
            // because it gives us information about where inside of a given
            // structure the pointers to other RCU-protected objects are.
            // Because we don't track types or maintain descriptors, we'll fall
            // back on a global, sort-of probabilistic approach to detecting
            // reads that don't do rcu_dereference on this pointer by using
            // shadow memory.
            } else {
                WATCHED_AND_ASSIGNED_POINTERS.set(
                    assign_pointer.shadow_offset, true);
            }

        // Record the shadow memory bits as being `rcu_dereference`able.
        } else {
            UNWATCHED_AND_ASSIGNED_POINTERS.set(
                assign_pointer.shadow_offset, true);
        }

        if(wp::is_watched_address(assign_pointer.as_uint)) {

            // Not good, we're doing the equivalent of:
            //      rcu_assign_pointer(p, rcu_dereference(q))
            // This might be indicative of a pointer leak from a read-side
            // critical section into a writer.
            if(assigned_pointer.is_deref) {
                log(RCU_ASSIGN_WITH_RCU_DEREFERENCED_POINTER,
                    thread_id,
                    get_section_carat(
                        assign_pointer.read_section_id,
                        SECTION_DEREF_CARAT),
                    assign_carat);
            }
        }

        // Inherit the location ID from the carat.
        rcudbg_counter_index index;
        index.is_deref = false;
        index.assign_location_id = get_location_id(assign_carat);

        counter_index = index.as_uint;
        return true;
    }
}
