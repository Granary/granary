/*
 * test.cc
 *
 *  Created on: 2012-11-28
 *      Author: pag
 *     Version: $Id$
 */

#include "globals.h"

extern "C" {

    void granary_break_on_fault(void) { }

    int granary_test_return_true(void) {
        return 1;
    }

    int granary_test_return_false(void) {
        return 0;
    }

    /// Hacks so that we don't need to link in libc++.
    int __cxa_guard_acquire(void) {
        return 1;
    }
    int __cxa_guard_release(void) {
        return 1;
    }
}


namespace granary {

    static_test_list STATIC_TEST_LIST_HEAD;

    static_test_list::static_test_list(void) throw()
        : func(nullptr)
        , desc(nullptr)
        , next(nullptr)
    { }

    void run_tests(void) throw() {
        static_test_list *test(STATIC_TEST_LIST_HEAD.next);
        for(; test; test = test->next) {
            if(test->func) {
                test->func();
            }
        }
    }

}
