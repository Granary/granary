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


    /// The kind of found target address.
    enum trace_log_target_kind {
        TARGET_TRANSLATED,
        TARGET_IS_DETACH_POINT,
        TARGET_RETURNS_TO_CACHE,
        TARGET_ALREADY_IN_CACHE,
        TARGET_RUNNING
    };


    /// Log a lookup in the code cache.
    void log_code_cache_find(
        app_pc app_addr,
        app_pc target_addr,
        trace_log_target_kind kind
    ) throw();


    /// Log the run of some code. This will add a lot of instructions to the
    /// beginning of an instruction list.
    void log_code_cache_run(instruction_list &) throw();
}

#endif /* TRACE_LOG_H_ */
