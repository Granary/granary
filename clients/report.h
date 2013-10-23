/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * report.h
 *
 *  Created on: 2013-06-29
 *      Author: Peter Goodman
 */

#ifndef CLIENT_REPORT_H_
#define CLIENT_REPORT_H_

/// How to use an init function:
///     1)  Define the `CLIENT_report` macro in here on a per-client basis.
///     2)  Define the `void report(void) throw()` function within the `client`
///         namespace within your client code.

#if defined(CLIENT_CFG) \
 || defined(CLIENT_INSTR_DIST) \
 || defined(CLIENT_WATCHPOINT_STATS) \
 || defined(CLIENT_WATCHPOINT_PROFILE) \
 || defined(CLIENT_WATCHPOINT_AUGMENT) \
 || defined(CLIENT_WATCHPOINT_USER) \
 || defined(CLIENT_RCUDBG)
#   define CLIENT_report
#endif

#ifdef CLIENT_report
namespace client {
    void report(void) throw();
}
#endif

#endif /* CLIENT_REPORT_H_ */
