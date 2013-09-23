/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * rcu.h
 *
 *  Created on: 2013-09-01
 *      Author: Peter Goodman
 */

#ifndef RCUDBG_TESTS_RCU_H_
#define RCUDBG_TESTS_RCU_H_

#define GRANARY_TO_STR__(x) #x
#define GRANARY_TO_STR_(x) GRANARY_TO_STR__(x)
#define GRANARY_TO_STR(x) GRANARY_TO_STR_(x)
#define GRANARY_SPLAT(x) x
#define GRANARY_CARAT GRANARY_SPLAT( __FILE__ ":" GRANARY_TO_STR(__LINE__) )

#include "granary/types.h"


#define rcu_dereference(p) \
    ((decltype(p)) granary::types::__granary_rcu_dereference( \
        (void **) &(p), \
        (void *) (p), \
        GRANARY_CARAT))


#define rcu_read_lock() \
    do { \
        granary::types::__granary_rcu_read_lock( \
            granary::types::GRANARY_RCU_READ_LOCK_CLASSIC, \
            (void *) __builtin_return_address(0), \
            GRANARY_CARAT); \
    } while(0)


#define rcu_read_unlock() \
    do { \
        granary::types::__granary_rcu_read_unlock( \
            granary::types::GRANARY_RCU_READ_LOCK_CLASSIC, \
            (void *) __builtin_return_address(0), \
            GRANARY_CARAT); \
    } while(0)


#define rcu_assign_pointer(p, v) \
    do { \
        granary::types::__granary_rcu_assign_pointer(\
            (void **) &(p), \
            (void *) v, \
            GRANARY_CARAT); \
    } while(0)

#endif /* RCUDBG_TESTS_RCU_H_ */
