/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * events.h
 *
 *  Created on: 2013-08-18
 *      Author: Peter Goodman
 */

#ifndef RCUDBG_EVENTS_H_
#define RCUDBG_EVENTS_H_

#ifndef GRANARY_DONT_INCLUDE_CSTDLIB
#   include "granary/client.h"
#endif

namespace client {

#ifndef GRANARY_DONT_INCLUDE_CSTDLIB
    /// Invoked when an RCU read-side critical section extend beyond the
    /// function in which the read-side critical section was started. This seems
    /// like bad practice as it splits up the read-critical sections, and makes
    /// it more likely for bugs to creep in. This could also be evidence of a
    /// read-side critical section being left locked.
    extern granary::app_pc EVENT_READ_THROUGH_RET;


    /// Invoked when a watched address that is the result of an
    /// `rcu_assign_pointer` is directly accessed (read or write) from within
    /// an RCU read-side critical section.
    extern granary::app_pc EVENT_ACCESS_ASSIGNED_POINTER;


    /// Invoked when a watched address that is the result of an
    /// `rcu_dereference` is written to from within a RCU read-side critical
    /// section.
    extern granary::app_pc EVENT_WRITE_TO_DEREF_POINTER;


    /// Invoked when a watched address that is the result of an
    /// `rcu_dereference` is read from within a RCU read-side critical
    /// section.
    extern granary::app_pc EVENT_READ_FROM_DEREF_POINTER;
#endif

    void *event_rcu_dereference(void *ptr, const char *carat) throw();
    void event_rcu_assign_pointer(void **p, void *v, const char *carat) throw();
    void event_rcu_read_lock(const char *carat) throw();
    void event_rcu_read_unlock(const char *carat) throw();
}


#endif /* RCUDBG_EVENTS_H_ */
