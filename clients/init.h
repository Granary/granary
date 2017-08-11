/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * init.h
 *
 *  Created on: 2013-06-24
 *      Author: Peter Goodman
 */

#ifndef CLIENT_INIT_H_
#define CLIENT_INIT_H_


/// How to use an init function:
///     1)  Define the `CLIENT_init` macro in here on a per-client basis.
///     2)  Define the `void init(void) ` function within the `client`
///         namespace within your client code.

/// When to use an init function:
///     Your client needs *late* initialisation, after all entrypoints have
///     taken over (e.g. interrupts), etc.

/// When NOT to use an init function, and instead use a STATIC_INITIALISE[_ID]:
///     Your client needs to initialise data structures used by your
///     instrumentation code. This is not suitable for an init function because
///     the init will only be called after certain entrypoints have been
///     translated been translated (e.g. syscalls, interrupts).

#ifdef CLIENT_WATCHPOINT_LEAK
#   define CLIENT_init
#endif


#ifdef CLIENT_ENTRY
#   define CLIENT_init
#endif


#ifdef CLIENT_RCUDBG
#   define CLIENT_init
#endif

#ifdef CLIENT_LIFETIME
#   define CLIENT_init
#endif

#ifdef CLIENT_TRACER
#   define CLIENT_init
#endif

#ifdef CLIENT_init
namespace client {
    void init(void) ;
}
#endif

#endif /* CLIENT_INIT_H_ */
