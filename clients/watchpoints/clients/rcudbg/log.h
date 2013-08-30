/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * log.h
 *
 *  Created on: 2013-08-18
 *      Author: Peter Goodman
 */

#ifndef RCUDBG_LOG_H_
#define RCUDBG_LOG_H_

#include "granary/client.h"

namespace client {
    enum log_message_id : unsigned {
        MESSAGE_NOT_READY = 0,
#define RCUDBG_MESSAGE(ident, kind, message, arg_defs, arg_splat) ident,
#include "clients/watchpoints/clients/rcudbg/message.h"
#undef RCUDBG_MESSAGE
        MAX_MESSAGE_ID
    };


    enum log_level : unsigned {
        INFO = 0,
        WARNING,
        ERROR,

        MIN_LOG_LEVEL = ERROR
    };


    struct message_info {
        const log_level level;
        const char * const format;
    };


    /// A generic message container for log entries from the rcudbg tool.
    struct message_container {
        std::atomic<log_message_id> message_id;
        unsigned num_args;
        uint64_t payload[4];
    };


    /// Records static message information.
    extern const message_info MESSAGE_INFO[];


    /// Next offset into the log to which we can write.
    extern std::atomic<uint64_t> NEXT_LOG_OFFSET;


    /// Ring buffer for messages.
    extern message_container MESSAGES[];


    enum {
        MAX_NUM_MESSAGES = 4096
    };


    template <typename A0>
    void log(log_message_id id, A0 a0) throw() {

        if(MIN_LOG_LEVEL > MESSAGE_INFO[id].level) {
            return;
        }

        const uint64_t index(NEXT_LOG_OFFSET.fetch_add(1));
        message_container &cont(MESSAGES[index % MAX_NUM_MESSAGES]);

        if(index >= MAX_NUM_MESSAGES) {
            return;
        }

        cont.message_id.store(MESSAGE_NOT_READY);
        cont.num_args = 1;
        cont.payload[0] = granary::unsafe_cast<uint64_t>(a0);
        cont.message_id.store(id);
    }


    template <typename A0, typename A1>
    void log(log_message_id id, A0 a0, A1 a1) throw() {

        if(MIN_LOG_LEVEL > MESSAGE_INFO[id].level) {
            return;
        }

        const uint64_t index(NEXT_LOG_OFFSET.fetch_add(1));
        message_container &cont(MESSAGES[index % MAX_NUM_MESSAGES]);

        if(index >= MAX_NUM_MESSAGES) {
            return;
        }

        cont.message_id.store(MESSAGE_NOT_READY);
        cont.num_args = 2;
        cont.payload[0] = granary::unsafe_cast<uint64_t>(a0);
        cont.payload[1] = granary::unsafe_cast<uint64_t>(a1);
        cont.message_id.store(id);
    }


    template <typename A0, typename A1, typename A2>
    void log(log_message_id id, A0 a0, A1 a1, A2 a2) throw() {

        if(MIN_LOG_LEVEL > MESSAGE_INFO[id].level) {
            return;
        }

        const uint64_t index(NEXT_LOG_OFFSET.fetch_add(1));
        message_container &cont(MESSAGES[index % MAX_NUM_MESSAGES]);

        if(index >= MAX_NUM_MESSAGES) {
            return;
        }

        cont.message_id.store(MESSAGE_NOT_READY);
        cont.num_args = 3;
        cont.payload[0] = granary::unsafe_cast<uint64_t>(a0);
        cont.payload[1] = granary::unsafe_cast<uint64_t>(a1);
        cont.payload[2] = granary::unsafe_cast<uint64_t>(a2);
        cont.message_id.store(id);
    }


    template <typename A0, typename A1, typename A2, typename A3>
    void log(log_message_id id, A0 a0, A1 a1, A2 a2, A3 a3) throw() {

        if(MIN_LOG_LEVEL > MESSAGE_INFO[id].level) {
            return;
        }

        const uint64_t index(NEXT_LOG_OFFSET.fetch_add(1));
        message_container &cont(MESSAGES[index % MAX_NUM_MESSAGES]);

        if(index >= MAX_NUM_MESSAGES) {
            return;
        }

        cont.message_id.store(MESSAGE_NOT_READY);
        cont.num_args = 4;
        cont.payload[0] = granary::unsafe_cast<uint64_t>(a0);
        cont.payload[1] = granary::unsafe_cast<uint64_t>(a1);
        cont.payload[2] = granary::unsafe_cast<uint64_t>(a2);
        cont.payload[3] = granary::unsafe_cast<uint64_t>(a3);
        cont.message_id.store(id);
    }
}

#endif /* RCUDBG_LOG_H_ */
