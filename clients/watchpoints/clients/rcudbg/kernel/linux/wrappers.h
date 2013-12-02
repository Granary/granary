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


#include "clients/watchpoints/clients/rcudbg/events.h"


/// Wrapper for `rcu_dereference`. The purpose of this wrapper to to add a
/// de-reference watchpoint.
#if defined(CAN_WRAP___granary_rcu_dereference) && CAN_WRAP___granary_rcu_dereference
#   define APP_WRAPPER_FOR___granary_rcu_dereference
#   define HOST_WRAPPER_FOR___granary_rcu_dereference

    FUNCTION_WRAPPER(APP, __granary_rcu_dereference, (void *), (
        void ** derefed_ptr,
        void * ptr,
        const char * carat
    ), {
        return client::event_rcu_dereference(derefed_ptr, ptr, carat);
    })
    FUNCTION_WRAPPER(HOST, __granary_rcu_dereference, (void *), (
       void ** derefed_ptr,
       void * ptr,
       const char * carat
    ), {
        return client::event_rcu_dereference(derefed_ptr, ptr, carat);
    })
#endif


/// Wrapper for `rcu_assign_pointer`. The purpose of this is to modify shadow
/// memory for `p`, and
#if defined(CAN_WRAP___granary_rcu_assign_pointer) && CAN_WRAP___granary_rcu_assign_pointer
#   define APP_WRAPPER_FOR___granary_rcu_assign_pointer
#   define HOST_WRAPPER_FOR___granary_rcu_assign_pointer

    FUNCTION_WRAPPER_VOID(APP, __granary_rcu_assign_pointer, (
        void * * p ,
        void * v ,
        const char * carat
    ), {
        client::event_rcu_assign_pointer(p, v, carat);
    })
    FUNCTION_WRAPPER_VOID(HOST, __granary_rcu_assign_pointer, (
        void * * p ,
        void * v ,
        const char * carat
    ), {
        client::event_rcu_assign_pointer(p, v, carat);
    })
#endif


/// Wrapper for `rcu_read_lock`.
#if defined(CAN_WRAP___granary_rcu_read_lock) && CAN_WRAP___granary_rcu_read_lock
#   define APP_WRAPPER_FOR___granary_rcu_read_lock
#   define HOST_WRAPPER_FOR___granary_rcu_read_lock

    FUNCTION_WRAPPER_VOID(APP, __granary_rcu_read_lock, (
        enum granary_rcu_read_lock_type,
        void *,
        const char * carat
    ), {
        client::event_rcu_read_lock(carat);
    })
    FUNCTION_WRAPPER_VOID(HOST, __granary_rcu_read_lock, (
        enum granary_rcu_read_lock_type,
        void * ret_address,
        const char * carat
    ), {
        client::event_rcu_read_lock(carat);
    })
#endif


/// Wrapper for `rcu_read_unlock`.
#if defined(CAN_WRAP___granary_rcu_read_unlock) && CAN_WRAP___granary_rcu_read_unlock
#   define APP_WRAPPER_FOR___granary_rcu_read_unlock
#   define HOST_WRAPPER_FOR___granary_rcu_read_unlock

    FUNCTION_WRAPPER_VOID(APP, __granary_rcu_read_unlock, (
        enum granary_rcu_read_lock_type ,
        void * ,
        const char * carat
    ), {
        client::event_rcu_read_unlock(carat);
    })
    FUNCTION_WRAPPER_VOID(HOST, __granary_rcu_read_unlock, (
        enum granary_rcu_read_lock_type ,
        void * ,
        const char * carat
    ), {
        client::event_rcu_read_unlock(carat);
    })
#endif

#endif /* RCUDBG_WRAPPERS_H_ */
