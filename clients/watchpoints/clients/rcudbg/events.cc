/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * events.cc
 *
 *  Created on: 2013-08-15
 *      Author: Peter Goodman
 */

#include "granary/client.h"

#include "clients/watchpoints/clients/rcudbg/carat.h"
#include "clients/watchpoints/clients/rcudbg/instrument.h"
#include "clients/watchpoints/clients/rcudbg/log.h"

using namespace granary;

namespace client {

    /// Invoked when an RCU read-side critical section extend beyond the
    /// function in which the read-side critical section was started. This seems
    /// like bad practice as it splits up the read-critical sections, and makes
    /// it more likely for bugs to creep in. This could also be evidence of a
    /// read-side critical section being left locked.
    app_pc EVENT_READ_THROUGH_RET = nullptr;


    /// Invoked when a watched adddress that is the result of an
    /// `rcu_assign_pointer` is directly accessed (read or write) from within
    /// an RCU read-side critical section.
    app_pc EVENT_ACCESS_ASSIGNED_POINTER = nullptr;


    /// Invoked when a watched address that is the result of an
    /// `rcu_dereference` is written to from within a RCU read-side critical
    /// section.
    app_pc EVENT_WRITE_TO_DEREF_POINTER = nullptr;


    /// Invoked when a watched address that is the result of an
    /// `rcu_dereference` is read from within a RCU read-side critical
    /// section.
    app_pc EVENT_READ_FROM_DEREF_POINTER = nullptr;


    /// Log a warning that a read-side critical section extends through a
    /// function return.
    static void event_read_through_ret(void) throw() {

        // Get the read-side critical section ID for this thread, as well as an
        // identifying pointer for this thread.
        IF_KERNEL( eflags flags = granary_disable_interrupts(); )
        thread_state_handle thread = safe_cpu_access_zone();
        const void *thread_id(thread.identifying_address());
        const char *read_lock_carat(thread->section_carat);
        IF_KERNEL( granary_store_flags(flags); )

        log(RCU_READ_SECTION_ACROSS_FUNC_BOUNDARY, thread_id, read_lock_carat);
    }


    /// Happens when `rcu_dereference` is invoked.
    void *event_rcu_dereference(
        void **derefed_ptr,
        void *ptr,
        const char *carat
    ) throw() {
        rcudbg_watched_address ptr_;
        ptr_.as_pointer = ptr;

        rcudbg_watched_address derefed_ptr_;
        derefed_ptr_.as_pointer = reinterpret_cast<void *>(derefed_ptr);

        wp::add_watchpoint(
            ptr, ptr_, derefed_ptr_, carat, rcu_dereference_tag());
        return ptr;
    }


    /// Happens when `rcu_assign_pointer` is invoked.
    void event_rcu_assign_pointer(void **p, void *v, const char *carat) throw() {
        rcudbg_watched_address p_;
        p_.as_pointer = reinterpret_cast<void *>(p);

        rcudbg_watched_address v_;
        v_.as_pointer = v;

        wp::add_watchpoint(v, p_, v_, carat, rcu_assign_pointer_tag());

        // Do the actual pointer assignment.
        std::atomic_thread_fence(std::memory_order_seq_cst);
        *p = v;
        std::atomic_thread_fence(std::memory_order_seq_cst);
    }


    /// Invoked when `rcu_read_lock` is invoked.
    void event_rcu_read_lock(const char *carat) throw() {
        IF_KERNEL( eflags flags = granary_disable_interrupts(); )
        thread_state_handle thread = safe_cpu_access_zone();
        thread->section_carat_backtrace[thread->section_depth] = carat;
        thread->section_carat = carat;
        if(!thread->section_depth) {
            thread->section_id = client::allocate_section_id(
                carat, thread.identifying_address(), thread->section_id);
        }
        thread->section_depth++;

        const unsigned section_id(thread->section_id);
        IF_KERNEL( granary_store_flags(flags); )

        set_section_carat(section_id, SECTION_LOCK_CARAT, carat);
    }


    /// Invoked when `rcu_read_unlock` is invoked.
    void event_rcu_read_unlock(const char *carat) throw() {
        IF_KERNEL( eflags flags = granary_disable_interrupts(); )
        thread_state_handle thread = safe_cpu_access_zone();
        const void *thread_id(nullptr);
        const char *last_read_unlock_carat(nullptr);

        unsigned section_id(0);
        bool has_section_id(false);

        if(thread->section_depth) {
            thread->section_carat = carat;
            thread->section_carat_backtrace[thread->section_depth] = carat;
            thread->section_depth--;

            has_section_id = true;
            section_id = thread->section_id;
        } else {
            thread_id = thread.identifying_address();
            last_read_unlock_carat = thread->section_carat;
            thread->section_carat = carat;
        }
        IF_KERNEL( granary_store_flags(flags); )

        if(has_section_id) {
            set_section_carat(section_id, SECTION_UNLOCK_CARAT, carat);
        }

        // Report the unbalanced `rcu_read_unlock`.
        if(thread_id) {
            log(RCU_READ_UNLOCK_OUTSIDE_OF_CRITICAL_SECTION,
                thread_id,
                last_read_unlock_carat,
                carat);
        }
    }



    /// Visit a read or write of a watched address that is the result of an
    /// `rcu_assign_pointer` within a read-side critical section.
    void access_assigned_pointer(register rcudbg_watched_address addr) {
        (void) addr;
    }


    /// Visit a write to a watched address that is the result of an
    /// `rcu_dereference` within a read-side critical section.
    void write_to_deref_pointer(register rcudbg_watched_address addr) {
        (void) addr;
    }


    /// Visit a read from a watched address that is the result of an
    /// `rcu_dereference` within a read-side critical section.
    void read_from_deref_pointer(register rcudbg_watched_address addr) {
        thread_state_handle thread = safe_cpu_access_zone();

        // Dereferenced outside of a read-side critical section.
        if(!thread->section_depth) {
            log(ACCESS_OF_LEAKED_RCU_DEREFERENCED_POINTER,
                thread.identifying_address(),
                get_section_carat(addr.read_section_id, SECTION_UNLOCK_CARAT),
                get_section_carat(addr.read_section_id, SECTION_DEREF_CARAT));

        // Dereferenced in wrong read-side critical section.
        } else if(thread->section_id != addr.read_section_id) {
            log(ACCESS_OF_WRONG_RCU_DEREFERENCED_POINTER,
                thread.identifying_address(),
                get_section_carat(addr.read_section_id, SECTION_UNLOCK_CARAT),
                get_section_carat(addr.read_section_id, SECTION_DEREF_CARAT));
        }
    }


    /// Initialise the clean-callable target for the events.
    void init(void) throw() {
        cpu_state_handle cpu;

        cpu.free_transient_allocators();
        EVENT_READ_THROUGH_RET = generate_clean_callable_address(
            event_read_through_ret);

        cpu.free_transient_allocators();
        EVENT_ACCESS_ASSIGNED_POINTER = generate_clean_callable_address(
            access_assigned_pointer);

        cpu.free_transient_allocators();
        EVENT_WRITE_TO_DEREF_POINTER = generate_clean_callable_address(
            write_to_deref_pointer);

        cpu.free_transient_allocators();
        EVENT_READ_FROM_DEREF_POINTER = generate_clean_callable_address(
            read_from_deref_pointer);

        cpu.free_transient_allocators();
    }
}
