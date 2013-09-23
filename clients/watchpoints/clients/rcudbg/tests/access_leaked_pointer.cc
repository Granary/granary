/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * test_atomic.cc
 *
 *  Created on: 2013-04-28
 *      Author: Peter Goodman
 */

#include "granary/test.h"

#if CONFIG_RUN_TEST_CASES

#include "clients/watchpoints/clients/rcudbg/instrument.h"
#include "clients/watchpoints/clients/rcudbg/log.h"

#include "clients/watchpoints/clients/rcudbg/tests/rcu.h"

namespace test {


    static int access_leaked_pointer(void) {
        int *q;
        int v = 10;

        rcu_assign_pointer(q, &v);
        rcu_read_lock();
        int *p = rcu_dereference(q);
        rcu_read_unlock();

        // Error: `p` is accessed outside of a read-side critical section.
        return *p;
    }


    static int access_leaked_pointer_wrong_section(void) {
        int *q;
        int v = 10;

        rcu_assign_pointer(q, &v);
        rcu_read_lock();
        int *p = rcu_dereference(q);
        rcu_read_unlock();

        rcu_read_lock();
        // Error: `p` is dereferenced in the wrong read-side critical section.
        int ret = *p;
        rcu_read_unlock();

        return ret;
    }


    /// Test that we detect accesses of rcu_dereferenced objects outside of
    /// read-side critical sections.
    static void detect_access_leaked_pointer(void) {

        granary::app_pc test_pc(
            (granary::app_pc) access_leaked_pointer);
        granary::instrumentation_policy policy =
            granary::policy_for<client::rcu_null>();
        policy.force_attach(true);
        granary::basic_block test_bb(granary::code_cache::find(
            test_pc, policy));

        client::clear_log();
        ASSERT(10 == test_bb.call<int>());
        ASSERT(1 == client::log_size());
        ASSERT(client::log_entry_is(
            0, client::ACCESS_OF_LEAKED_RCU_DEREFERENCED_POINTER));
        client::clear_log();
    }


    /// Test that we detect accesses of rcu_dereferenced objects outside of
    /// read-side critical sections.
    static void detect_access_leaked_pointer_wrong_section(void) {

        granary::app_pc test_pc(
            (granary::app_pc) access_leaked_pointer_wrong_section);
        granary::instrumentation_policy policy =
            granary::policy_for<client::rcu_null>();
        policy.force_attach(true);
        granary::basic_block test_bb(granary::code_cache::find(
            test_pc, policy));

        client::clear_log();
        ASSERT(10 == test_bb.call<int>());
        ASSERT(1 == client::log_size());
        ASSERT(client::log_entry_is(
            0, client::ACCESS_OF_WRONG_RCU_DEREFERENCED_POINTER));
        client::clear_log();
    }


    ADD_TEST(detect_access_leaked_pointer,
        "Test that we detect accesses of rcu_dereferece'd pointers outside "
        "of any read-side critical sections.")


    ADD_TEST(detect_access_leaked_pointer_wrong_section,
        "Test that we detect accesses of rcu_dereferece'd pointers in the "
        "wrong read-side critical section.")
}

#endif /* CONFIG_RUN_TEST_CASES*/
