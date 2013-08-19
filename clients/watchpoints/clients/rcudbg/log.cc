/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * log.cc
 *
 *  Created on: 2013-08-18
 *      Author: Peter Goodman
 */

#include <atomic>

#include "clients/watchpoints/clients/rcudbg/log.h"

#define SPLAT(...) __VA_ARGS__

namespace client {


    message_container MESSAGES[MAX_NUM_MESSAGES] = {{
        ATOMIC_VAR_INIT(MESSAGE_NOT_READY), {0, 0, 0, 0}
    }};


    /// A generic printer function that operates on an untypes message
    /// container and a buffer.
    typedef int (printer_func)(
        char *buff,
        const message_container *cont
    );


    /// An "untyped" printer implementation function.
    typedef int (untyped_printer_func)(
        char *buff,
        const char *format,
        uint64_t,
        uint64_t,
        uint64_t,
        uint64_t
    );


    /// Next offset into the log to which we can write.
    std::atomic<uint64_t> NEXT_LOG_OFFSET = ATOMIC_VAR_INIT(0);


    /// Puts all the printer format strings into an array.
    const char *PRINTER_FORMATS[] = {
#define RCUDBG_MESSAGE(ident, kind, message, arg_defs, arg_splat) message ,
#include "clients/watchpoints/clients/rcudbg/message.h"
#undef RCUDBG_MESSAGE
        ""
    };


    /// Define logger printers.
#define RCUDBG_MESSAGE(ident, kind, message, arg_defs, arg_splat) \
    static int PRINT_impl_ ## ident ( \
        char *buff, \
        const char *format, \
        SPLAT arg_defs \
    ) throw() { \
        return sprintf(buff, format, SPLAT arg_splat ); \
    } \
    \
    static int PRINT_ ## ident ( \
        char *buff, \
        const message_container *cont \
    ) throw() { \
        untyped_printer_func *print_func = \
            (untyped_printer_func *) PRINT_impl_ ## ident ; \
        \
        return print_func( \
            buff, \
            PRINTER_FORMATS[ LOG_ ## ident ], \
            cont->payload[0], \
            cont->payload[1], \
            cont->payload[2], \
            cont->payload[3] \
        ); \
    }
#include "clients/watchpoints/clients/rcudbg/message.h"
#undef RCUDBG_MESSAGE


    /// Define an array of printers for the various log messages.
    static printer_func *PRINTERS[] = {
#define RCUDBG_MESSAGE(ident, kind, message, arg_defs, arg_splat) \
    & PRINT_ ## ident ,
#include "clients/watchpoints/clients/rcudbg/message.h"
#undef RCUDBG_MESSAGE
        nullptr
    };


    STATIC_INITIALISE({
        (void) PRINTERS;
    });
}
