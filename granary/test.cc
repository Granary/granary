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

}


namespace granary {

    static_test_list STATIC_TEST_LIST_HEAD;

    void run_tests(void) throw() {
        static_test_list *test(STATIC_TEST_LIST_HEAD.next);
        for(; test; test = test->next) {
            test->func();
        }
    }

}
