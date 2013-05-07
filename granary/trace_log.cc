/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * trace_log.cc
 *
 *  Created on: 2013-05-06
 *      Author: Peter Goodman
 */

#include "granary/globals.h"
#include "granary/trace_log.h"

namespace granary {

#if CONFIG_TRACE_CODE_CACHE_FIND
#   define I(...) __VA_ARGS__

    struct trace_log_item {

        /// Previous log item in this thread.
        trace_log_item *prev;

        /// Native address.
        void *app_address;

        /// Code cache address representing the translated version of
        /// the native address.
        void *cache_address;

        /// Kind of log item.
        trace_log_target_kind kind;
    };


    /// Head of the log.
    static std::atomic<trace_log_item *> TRACE = ATOMIC_VAR_INIT(nullptr);


    /// Configuration for log items.
    struct trace_log_allocator_config {
        enum {
            SLAB_SIZE = 1024 * sizeof(trace_log_item),
            EXECUTABLE = false,
            TRANSIENT = false,
            SHARED = true,
            EXEC_WHERE = EXEC_NONE,
            MIN_ALIGN = 1
        };
    };


    /// Allocator for the log.
    static static_data<
        bump_pointer_allocator<trace_log_allocator_config>
    > TRACE_LOG_ALLOCATOR;


    STATIC_INITIALISE_ID(trace_logger, {
        TRACE_LOG_ALLOCATOR.construct();
    })

#else
#   define I(...)
#endif

    void log_code_cache_find(
        app_pc I(app_addr),
        app_pc I(target_addr),
        trace_log_target_kind I(kind)
    ) throw() {
#if CONFIG_TRACE_CODE_CACHE_FIND
        trace_log_item *prev(nullptr);
        trace_log_item *item(TRACE_LOG_ALLOCATOR->allocate<trace_log_item>());
        item->app_address = app_addr;
        item->cache_address = target_addr;
        item->kind = kind;

        do {
            prev = TRACE.load();
            item->prev = prev;
        } while(!TRACE.compare_exchange_weak(prev, item));
#endif
    }

}


