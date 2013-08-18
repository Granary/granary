/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * wrappers.h
 *
 *  Created on: 2013-08-18
 *      Author: Peter Goodman
 */

#ifndef RCUDBG_WRAPPERS_H_
#define RCUDBG_WRAPPERS_H_

/// Guard that makes it easier to work with wrappers when developing with a
/// syntax highlighter.
#ifndef GRANARY_DONT_INCLUDE_CSTDLIB
#   include "granary/wrapper.h"
#endif

#include "clients/watchpoints/clients/rcudbg/descriptor.h"


/// Wrapper for `rcu_dereference`. The purpose of this wrapper to to add a
/// de-reference watchpoint.
#if defined(CAN_WRAP___granary_rcu_dereference) && CAN_WRAP___granary_rcu_dereference
#   define APP_WRAPPER_FOR___granary_rcu_dereference
#   define HOST_WRAPPER_FOR___granary_rcu_dereference

    FUNCTION_WRAPPER(APP, __granary_rcu_dereference, (void *), ( void * ptr, const char * carat), {
        return ptr;
    })
    FUNCTION_WRAPPER(HOST, __granary_rcu_dereference, (void *), ( void * ptr, const char * carat), {
        return ptr;
    })
#endif


/// Wrapper for `rcu_assign_pointer`. The purpose of this is to modify shadow
/// memory for `p`, and
#if defined(CAN_WRAP___granary_rcu_assign_pointer) && CAN_WRAP___granary_rcu_assign_pointer
#   define APP_WRAPPER_FOR___granary_rcu_assign_pointer
#   define HOST_WRAPPER_FOR___granary_rcu_assign_pointer

    FUNCTION_WRAPPER(APP, __granary_rcu_assign_pointer, (void *), ( void * const p , void * v , const char * ), {
        return v;
    })
    FUNCTION_WRAPPER(HOST, __granary_rcu_assign_pointer, (void *), ( void * const p , void * v , const char * ), {
        return v;
    })
#endif

#endif /* RCUDBG_WRAPPERS_H_ */
