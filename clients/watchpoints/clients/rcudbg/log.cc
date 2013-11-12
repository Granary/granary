/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * log.cc
 *
 *  Created on: 2013-08-18
 *      Author: Peter Goodman
 */

#include <atomic>

#include "granary/client.h"

#include "clients/watchpoints/clients/rcudbg/log.h"


#ifndef SPLAT
#   define SPLAT(...) __VA_ARGS__
#endif


extern "C" {
    extern int sprintf(char *buf, const char *fmt, ...);
}

using namespace granary;

namespace client {


    message_container MESSAGES[MAX_NUM_MESSAGES] = {{
        ATOMIC_VAR_INIT(MESSAGE_NOT_READY), 0, {0, 0, 0, 0}
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


    /// Puts all the static info of each message into a common place.
    const message_info MESSAGE_INFO[] = {
        {INFO, ""},
#define RCUDBG_MESSAGE(ident, kind, message, arg_defs, arg_splat) \
    { kind , message },
#include "clients/watchpoints/clients/rcudbg/message.h"
#undef RCUDBG_MESSAGE
        {INFO, ""}
    };

    typedef int print_4_func(
        char *, const char *, uint64_t, uint64_t, uint64_t, uint64_t);
    typedef int print_3_func(
        char *, const char *, uint64_t, uint64_t, uint64_t);
    typedef int print_2_func(
        char *, const char *, uint64_t, uint64_t);
    typedef int print_1_func(
        char *, const char *, uint64_t);
    typedef int (int_func)(void);
    typedef int (generic_printer_func)(
        char *, const message_container *, int_func *);


    static int print_4(
        char *buff,
        const message_container *cont,
        int_func *printer
    ) throw() {
        print_4_func *print_func = (print_4_func *) printer;
        const unsigned id(cont->message_id.load(std::memory_order_relaxed));

        return print_func(
            buff,
            MESSAGE_INFO[id].format,
            cont->payload[0],
            cont->payload[1],
            cont->payload[2],
            cont->payload[3]
        );
    }


    static int print_3(
        char *buff,
        const message_container *cont,
        int_func *printer
    ) throw() {
        print_3_func *print_func = (print_3_func *) printer;
        const unsigned id(cont->message_id.load(std::memory_order_relaxed));

        return print_func(
            buff,
            MESSAGE_INFO[id].format,
            cont->payload[0],
            cont->payload[1],
            cont->payload[2]
        );
    }


    static int print_2(
        char *buff,
        const message_container *cont,
        int_func *printer
    ) throw() {
        print_2_func *print_func = (print_2_func *) printer;
        const unsigned id(cont->message_id.load(std::memory_order_relaxed));

        return print_func(
            buff,
            MESSAGE_INFO[id].format,
            cont->payload[0],
            cont->payload[1]
        );
    }


    static int print_1(
        char *buff,
        const message_container *cont,
        int_func *printer
    ) throw() {
        print_1_func *print_func = (print_1_func *) printer;
        const unsigned id(cont->message_id.load(std::memory_order_relaxed));

        return print_func(
            buff,
            MESSAGE_INFO[id].format,
            cont->payload[0]
        );
    }


    generic_printer_func *VARIADIC_PRINTERS[] = {
        nullptr,
        &print_1,
        &print_2,
        &print_3,
        &print_4
    };


    /// Define logger printers.
#define RCUDBG_MESSAGE(ident, kind, message, arg_defs, arg_splat) \
    static int CAT(PRINT_, ident) ( \
        char *buff, \
        const char *format, \
        SPLAT arg_defs \
    ) throw() { \
        return sprintf(buff, format, SPLAT arg_splat ); \
    }
#include "clients/watchpoints/clients/rcudbg/message.h"
#undef RCUDBG_MESSAGE


    /// Define an array of printers for the various log messages.
    static int_func *MESSAGE_PRINTERS[] = {
        nullptr,
#define RCUDBG_MESSAGE(ident, kind, message, arg_defs, arg_splat) \
    (int_func *) & CAT(PRINT_, ident) ,
#include "clients/watchpoints/clients/rcudbg/message.h"
#undef RCUDBG_MESSAGE
        nullptr
    };


    enum {
        BUFFER_SIZE = PAGE_SIZE * 4,
        BUFFER_FLUSH_SIZE = BUFFER_SIZE - (PAGE_SIZE / 4)
    };


    char BUFFER[BUFFER_SIZE] = {'\0'};


    /// Log the reports.
    void report(void) {
        detach();

        message_container cont;
        int b(0);
        unsigned num_messages(NEXT_LOG_OFFSET.load());
        if(MAX_NUM_MESSAGES > num_messages) {
            num_messages = MAX_NUM_MESSAGES;
        }

        for(unsigned i(0); i < num_messages; ++i) {
            const message_container &cont_(MESSAGES[i]);
            if(MESSAGE_NOT_READY == cont_.message_id.load()) {
                continue;
            }

            // Get a consistent view of the data structure.
            log_message_id id(MESSAGE_NOT_READY);
            log_message_id id_check(MESSAGE_NOT_READY);
            do {
                id = cont_.message_id.load();
                memcpy(&cont, &cont_, sizeof cont);
                id_check = cont.message_id.load(std::memory_order_relaxed);
            } while(id_check != id && MESSAGE_NOT_READY != id_check);

            // Invalid message id.
            if(MESSAGE_NOT_READY >= id && MAX_MESSAGE_ID <= id) {
                continue;
            }

            if(b >= BUFFER_FLUSH_SIZE) {
                granary::log(&(BUFFER[0]), b);
                b = 0;
            }

            b += VARIADIC_PRINTERS[cont.num_args](
                &(BUFFER[b]),
                &cont,
                MESSAGE_PRINTERS[id]);
        }

        if(b) {
            granary::log(&(BUFFER[0]), b);
        }

        NEXT_LOG_OFFSET.store(0);
    }


    bool log_entry_is(unsigned offset, const log_message_id expect_id) throw() {
        if(offset >= MAX_NUM_MESSAGES) {
            return false;
        }

        message_container &cont(MESSAGES[offset]);
        return expect_id == cont.message_id.load();
    }
}
