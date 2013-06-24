/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * thread.cc
 *
 *  Created on: 2013-06-24
 *      Author: Peter Goodman
 */


#include "clients/watchpoints/policies/leak_detector/instrument.h"

namespace client { namespace wp {


    /// Notify the leak detector that this thread's execution has entered the
    /// code cache.
    void leak_notify_thread_enter_module(void) throw() {
        granary::printf("Entering the code cache.\n");
    }


    /// Notify the leak detector that this thread's execution is leaving the
    /// code cache.
    ///
    /// Note: Entry/exits can be nested in the case of the kernel calling the
    ///       module calling the kernel calling the module.
    void leak_notify_thread_exit_module(void) throw() {
        granary::printf("Exiting the code cache.\n");
    }

}}

