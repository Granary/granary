/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * error.h
 *
 *  Created on: 2013-08-18
 *      Author: Peter Goodman
 */

#ifndef RCUDBG_MESSAGE_H_
#define RCUDBG_MESSAGE_H_

#ifndef RCUDBG_MESSAGE
#   define RCUDBG_MESSAGE(ident, kind, message, arg_defs, arg_splat)
#endif

RCUDBG_MESSAGE(
    DEREF_UNWATCHED_ADDRESS,
    WARNING,
    "Warning: Dereferencing unwatched address (%p).\n"
    " > task: %p.\n"
    " > rcu_dereference: %s.\n"
    " > rcu_read_lock: %s.",
    (const void *task, void *address, const char *deref_carat, const char *read_lock_carat),
    (address, task, deref_carat, read_lock_carat))

RCUDBG_MESSAGE(
    DOUBLE_DEREF_WRONG_SECTION,
    ERROR,
    "Error: Dereferencing already dereferenced pointer, but from another context.\n"
    " > task: %p.\n"
    " > rcu_dereference: %s.\n"
    " > rcu_read_lock (mine): %s.\n"
    " > rcu_read_lock (theirs): %s.",
    (const void *task, void *deref_carat, const char *read_lock_carat, const char *their_read_lock_carat),
    (task, deref_carat, read_lock_carat, their_read_lock_carat))

RCUDBG_MESSAGE(
    DOUBLE_DEREF,
    WARNING,
    "Warning: Dereferencing already dereferenced pointer.\n"
    " > task: %p\n"
    " > rcu_dereference: %s.\n"
    " > rcu_read_lock: %s.",
    (const void *task, void *deref_carat, const char *read_lock_carat),
    (task, deref_carat, read_lock_carat))

#undef RCUDBG_MESSAGE

#endif /* RCUDBG_MESSAGE_H_ */
