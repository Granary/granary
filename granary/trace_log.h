/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * trace_log.h
 *
 *  Created on: 2013-05-06
 *      Author: Peter Goodman
 */

#ifndef TRACE_LOG_H_
#define TRACE_LOG_H_


namespace granary {


    /// Forward declarations.
    struct instruction_list;
    union simple_machine_state;
    struct instrumentation_policy;

    struct trace_log {

        /// Log a lookup in the code cache.
        static void add_entry(
            app_pc code_cache_addr,
            simple_machine_state *state
        ) throw();


        /// Log the run of some code. This will add a lot of instructions to the
        /// beginning of an instruction list.
        static void log_execution(instruction_list &) throw();


        /// A generic reverse-execution debugger.
        static void debug(app_pc pc) throw();
    };
}

#endif /* TRACE_LOG_H_ */
