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


    /// Counter index for an `rcudbg` watched address.
    union rcudbg_counter_index {

        struct {
            uint16_t assign_location_id:14;
            uint16_t:2;
        } __attribute__((packed));

        struct {
            uint16_t read_section_id:14;

            /// Was this an assigned pointer (using `rcu_assign_pointer`) or a
            /// dereferenced pointer (using `rcu_dereference`).
            bool is_deref:1;

            uint16_t:1; // high, shifted out.
        } __attribute__((packed));

        uint16_t as_uint;

    } __attribute__((packed));


    static_assert(sizeof(uint16_t) == sizeof(rcudbg_counter_index),
        "Invalid packing of `rcudbg_counter_index` structure.");


    /// Structure of a watched address.
    union rcudbg_watched_address {

        struct {
            uint64_t:49;

            /// ID for the pointer assign location. Overlaps with
            /// `read_section_id`.
            uint64_t assign_location_id:14;

            uint64_t:1;
        } __attribute__((packed));

        struct {
            uint64_t:3;                  // low

            /// Offset into shadow memory for marking certain pointer bits as
            /// non-dereferenceable without an `rcu_dereference`.
            uint64_t shadow_offset:16;

            uint64_t:29;

            bool IF_USER_ELSE(is_watched, is_unwatched):1;

            /// ID for the read-side critical section. Overlaps with
            /// `assign_location_id`.
            uint64_t read_section_id:14;

            /// Was this an assigned pointer (using `rcu_assign_pointer`) or a
            /// dereferenced pointer (using `rcu_dereference`).
            bool is_deref:1;            // high
        } __attribute__((packed));

        uintptr_t as_uint;
        void *as_pointer;

    } __attribute__((packed));


    static_assert(sizeof(uint64_t) == sizeof(rcudbg_watched_address),
        "Invalid packing of `rcudbg_watched_address` structure.");


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


    enum {
        MAX_NUM_SHADOW_BITS = 1ULL << 16
    };


    /// `p` for `p` in `rcu_dereference(p, q)` where `p` is unwatched.
    static bitset<MAX_NUM_SHADOW_BITS> UNWATCHED_AND_ASSIGNED_POINTERS;


    /// `p` for `p` in `rcu_dereference(p, q)` where `p` is watched.
    static bitset<MAX_NUM_SHADOW_BITS> WATCHED_AND_ASSIGNED_POINTERS;


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
        void **derefed_ptr_,
        void *ptr_,
        const char *dereference_carat
    ) throw() {

        // The pointer value resulting from the `rcu_dereference`.
        rcudbg_watched_address deref_address;
        deref_address.as_pointer = ptr_;

        // The address being de-referenced by the `rcu_dereference`.
        rcudbg_watched_address derefed_address;
        derefed_address.as_pointer = reinterpret_cast<void *>(derefed_ptr_);

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

        // Doing a dereference of an unwatched object. This is common, so we
        // double check with the shadow memory associated with unwatched
        // objects. Because it's common, we treat it as a warning. For example:
        //      <&GLOBAL_P is unwatched>
        //      <value of GLOBAL_P is potentially watched, if it was first
        //       assigned with rcu_assign_pointer>
        //      void *p = rcu_dereference(GLOBAL_P)
        //      <value of p is watched>
        //      <&p is unwatched>
        if(!wp::is_watched_address(derefed_address.as_uint)) {
            if(!UNWATCHED_AND_ASSIGNED_POINTERS.get(derefed_address.shadow_offset)) {
                log(RCU_DEREFERENCE_UNASSIGNED_ADDRESS,
                    derefed_address.as_pointer,
                    thread_id,
                    dereference_carat,
                    read_lock_carat);
            }
        } else if(!WATCHED_AND_ASSIGNED_POINTERS.get(derefed_address.shadow_offset)) {
            log(RCU_DEREFERENCE_UNASSIGNED_ADDRESS,
                derefed_address.as_pointer,
                thread_id,
                dereference_carat,
                read_lock_carat);
        }

        // The value/pointer stored is not watched. For example:
        //      <&q is watched or unwatched>
        //      <value of q is unwatched>
        //      void *p = rcu_dereference(q);
        //      <p is watched>
        // We report this as a warning because it's possible that multiple
        // items were assigned in a hurd, for example:
        //      q = new list()
        //      q_next = new list()
        //      q->next = q_next
        //      rcu_assign_pointer(p, q)
        // Here, p's value will be a watched q, but q->next's value will be
        // unwatched.
        if(!wp::is_watched_address(deref_address.as_uint)) {
            log(RCU_DEREFERENCE_UNASSIGNED_VALUE,
                deref_address.as_pointer,
                thread_id,
                dereference_carat,
                read_lock_carat);

        // A double-dereference has occurred. That is:
        //      void *q = rcu_dereference(p);
        //      <value of q is watched, is_deref=true>
        //      <&q is unwatched>
        //      void *r = rcu_dereference(q);
        //      <value of r is watched>
        //      <&r is unwatched>
        // We look at deref_address instead of derefed_address because we expect
        // that derefed_address will have is_deref=true, for example:
        //      list_head *first = rcu_dereference(HEAD);
        //      <value of first is watched, is_deref=true>
        //      list_head *second = rcu_dereference(first->next);
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


        } else {

            // We are dereferencing a pointer that was created using
            // `rcu_assign_pointer`. For example:
            //      rcu_assign_pointer(p->next, q);
            //      <value of p->next is watched>
            //      <value of p may be watched>
            //      <&p is unlikely to be watched>
            //      rcu_dereference(p->next);
            // Don't have enough info to check anything meaningful.
            //
            // TODO: Perhaps associate assign location_ids with read lock
            //       carats.
        }

        // Inherit the section ID from the current thread.
        rcudbg_counter_index index;
        index.is_deref = true;
        index.read_section_id = section_id;

        set_section_carat(section_id, SECTION_DEREF_CARAT, dereference_carat);

        if(watch_the_address) {
            wp::taint(ptr_, index.as_uint);
        }

        return ptr_;
    }


    /// Happens when `rcu_assign_pointer` is invoked.
    void event_rcu_assign_pointer(
        void **p, void *v, const char *assign_carat
    ) throw() {
        rcudbg_watched_address assign_pointer;
        assign_pointer.as_pointer = reinterpret_cast<void *>(p);

        rcudbg_watched_address assigned_pointer;
        assigned_pointer.as_pointer = v;

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

        if(wp::is_watched_address(assigned_pointer.as_uint)) {

            // Not good, we're doing the equivalent of:
            //      rcu_assign_pointer(p, rcu_dereference(q))
            // This might be indicative of a pointer leak from a read-side
            // critical section into a writer.
            if(assigned_pointer.is_deref) {
                log(RCU_ASSIGN_WITH_RCU_DEREFERENCED_POINTER,
                    thread_id,
                    get_section_carat(
                        assigned_pointer.read_section_id,
                        SECTION_DEREF_CARAT),
                    assign_carat);
            }
        }

        // Inherit the location ID from the carat.
        rcudbg_counter_index index;
        index.is_deref = false;
        index.assign_location_id = get_location_id(assign_carat);
        wp::taint(v, index.as_uint);

        // Do the actual pointer assignment.
        std::atomic_thread_fence(std::memory_order_acquire);
        *p = v;
        std::atomic_thread_fence(std::memory_order_release);
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
        IF_KERNEL( eflags flags = granary_disable_interrupts(); )
        thread_state_handle thread = safe_cpu_access_zone();
        const char *section_carat(thread->section_carat);
        const void *thread_id(thread.identifying_address());
        IF_KERNEL( granary_store_flags(flags); )

        log(ACCESS_OF_ASSIGNED_POINTER_IN_CRITICAL_SECTION,
            thread_id,
            section_carat,
            get_location_carat(addr.assign_location_id));
    }


    /// Visit a write to a watched address that is the result of an
    /// `rcu_dereference` within a read-side critical section.
    void write_to_deref_pointer(register rcudbg_watched_address addr) {
        IF_KERNEL( eflags flags = granary_disable_interrupts(); )
        thread_state_handle thread = safe_cpu_access_zone();
        const char *section_carat(thread->section_carat);
        const void *thread_id(thread.identifying_address());
        IF_KERNEL( granary_store_flags(flags); )

        log(WRITE_TO_DEREF_POINTER_IN_CRITICAL_SECTION,
            thread_id,
            section_carat,
            get_section_carat(addr.read_section_id, SECTION_DEREF_CARAT));
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

        IF_TEST( cpu->in_granary = false; )
        cpu.free_transient_allocators();
        EVENT_READ_THROUGH_RET = generate_clean_callable_address(
            event_read_through_ret);

        IF_TEST( cpu->in_granary = false; )
        cpu.free_transient_allocators();
        EVENT_ACCESS_ASSIGNED_POINTER = generate_clean_callable_address(
            access_assigned_pointer);

        IF_TEST( cpu->in_granary = false; )
        cpu.free_transient_allocators();
        EVENT_WRITE_TO_DEREF_POINTER = generate_clean_callable_address(
            write_to_deref_pointer);

        IF_TEST( cpu->in_granary = false; )
        cpu.free_transient_allocators();
        EVENT_READ_FROM_DEREF_POINTER = generate_clean_callable_address(
            read_from_deref_pointer);

        IF_TEST( cpu->in_granary = false; )
        cpu.free_transient_allocators();
    }
}
