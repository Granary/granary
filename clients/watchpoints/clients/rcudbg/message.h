/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * error.h
 *
 *  Created on: 2013-08-18
 *      Author: Peter Goodman
 */

#ifndef RCUDBG_MESSAGE
#   define RCUDBG_MESSAGE(ident, kind, message, arg_defs, arg_splat)
#endif

RCUDBG_MESSAGE(
    RCU_DEREFERENCE_UNASSIGNED_ADDRESS,
    WARNING,
    "Warning: Dereferencing from an address that wasn't assigned using rcu_assign_pointer. (%p).\n"
    " > thread: %p.\n"
    " > rcu_dereference: %s.\n"
    " > rcu_read_lock: %s.\n\n",
    (const void *thread, void *address, const char *deref_carat, const char *read_lock_carat),
    (address, thread, deref_carat, read_lock_carat))

RCUDBG_MESSAGE(
    DOUBLE_RCU_DEREFERENCE_WRONG_SECTION,
    ERROR,
    "Error: Dereferencing already dereferenced pointer, but from another context.\n"
    " > thread: %p.\n"
    " > rcu_dereference: %s.\n"
    " > rcu_read_lock (mine): %s.\n"
    " > rcu_read_lock (theirs): %s.\n\n",
    (const void *thread, const char *deref_carat, const char *read_lock_carat, const char *their_read_lock_carat),
    (thread, deref_carat, read_lock_carat, their_read_lock_carat))

RCUDBG_MESSAGE(
    DOUBLE_RCU_DEREFERENCE,
    WARNING,
    "Warning: Dereferencing already dereferenced pointer.\n"
    " > thread: %p.\n"
    " > rcu_dereference: %s.\n"
    " > rcu_read_lock: %s.\n\n",
    (const void *thread, const char *deref_carat, const char *read_lock_carat),
    (thread, deref_carat, read_lock_carat))

RCUDBG_MESSAGE(
    RCU_ASSIGN_TO_RCU_DEREFERENCED_POINTER,
    ERROR,
    "Error: Assigning to dereferenced pointer.\n"
    " > thread: %p.\n"
    " > rcu_dereference: %s.\n"
    " > rcu_assign_pointer: %s.\n\n",
    (const void *thread, const char *deref_carat, const char *assign_carat),
    (thread, deref_carat, assign_carat))

RCUDBG_MESSAGE(
    RCU_ASSIGN_WITH_RCU_DEREFERENCED_POINTER,
    ERROR,
    "Error: Assigning with a dereferenced pointer.\n"
    " > thread: %p.\n"
    " > rcu_dereference: %s.\n"
    " > rcu_assign_pointer: %s.\n\n",
    (const void *thread, const char *deref_carat, const char *assign_carat),
    (thread, deref_carat, assign_carat))

RCUDBG_MESSAGE(
    RCU_READ_SECTION_ACROSS_FUNC_BOUNDARY,
    WARNING,
    "Warning: Read-side critical section expands through function return.\n"
    " > thread: %p.\n"
    " > rcu_read_lock: %s.\n\n",
    (const void *thread, const char *read_lock_carat),
    (thread, read_lock_carat))

RCUDBG_MESSAGE(
    RCU_READ_UNLOCK_OUTSIDE_OF_CRITICAL_SECTION,
    ERROR,
    "Error: rcu_read_unlock invoked outside of a read-side critical section.\n"
    " > thread: %p.\n"
    " > last rcu_read_unlock: %s.\n"
    " > rcu_read_unlock: %s.\n\n",
    (const void *thread, const char *prev_unlock_carat, const char *curr_unlock_carat),
    (thread, prev_unlock_carat, curr_unlock_carat))

RCUDBG_MESSAGE(
    RCU_ASSIGN_IN_CRITICAL_SECTION,
    ERROR,
    "Error: rcu_assign_pointer within read-side critical section.\n"
    " > thread: %p.\n"
    " > rcu_read_lock: %s.\n"
    " > rcu_assign_pointer: %s.\n\n",
    (const void *thread, const char *read_lock_carat, const char *assign_carat),
    (thread, read_lock_carat, assign_carat))


RCUDBG_MESSAGE(
    RCU_DEREFERENCE_OUTSIDE_OF_CRITICAL_SECTION,
    WARNING,
    "Warning: rcu_dereference outside of read-side critical section.\n"
    " > thread: %p.\n"
    " > last rcu_read_unlock: %s.\n"
    " > rcu_dereference: %s.\n\n",
    (const void *thread, const char *read_unlock_carat, const char *deref_carat),
    (thread, read_unlock_carat, deref_carat))

RCUDBG_MESSAGE(
    ACCESS_OF_LEAKED_RCU_DEREFERENCED_POINTER,
    ERROR,
    "Error: Access of an rcu_dereference'd pointer outside of a "
    "read-side critical section.\n"
    " > thread: %p.\n"
    " > last rcu_read_unlock: %s.\n"
    " > last rcu_dereference: %s.\n\n",
    (const void *thread, const char *read_unlock_carat, const char *deref_carat),
    (thread, read_unlock_carat, deref_carat))

RCUDBG_MESSAGE(
    ACCESS_OF_WRONG_RCU_DEREFERENCED_POINTER,
    ERROR,
    "Error: Access of an rcu_dereference'd pointer in the wrong read-side "
    "critical section.\n"
    " > thread: %p.\n"
    " > last rcu_read_unlock: %s.\n"
    " > last rcu_dereference: %s.\n\n",
    (const void *thread, const char *read_unlock_carat, const char *deref_carat),
    (thread, read_unlock_carat, deref_carat))
