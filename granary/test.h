/*
 * test.h
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#ifndef granary_TEST_H_
#define granary_TEST_H_

namespace granary {

    /// Used for static initialization of test cases.
    struct static_test_list {
        void (*func)(void);
        const char *desc;
        static_test_list *next;

        static_test_list(void) throw();

        static void append(static_test_list &) throw();
    };


    void run_tests(void) throw();

}


#endif /* granary_TEST_H_ */
