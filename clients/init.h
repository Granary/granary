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
///     2)  Define the `void init(void) throw()` function within the `client`
///         namespace within your client code.

#ifdef CLIENT_WATCHPOINT_LEAK
#   define CLIENT_init
#endif


#ifdef CLIENT_init
namespace client {
    void init(void) throw();
}
#endif

#endif /* CLIENT_INIT_H_ */
