/*
 * state.cc
 *
 *  Created on: 2012-11-19
 *      Author: pag
 *     Version: $Id$
 */

#include <atomic>
#include "granary/state.h"

namespace granary {

    /// No good concept of per-cpu state in user space so
    /// we'll just have a global state whose accesses are
    /// serialized by a simple spinlock.

    /// the global "CPU" state.
    static cpu_state GLOBAL_STATE;


    /// True iff the "cpu" state is currently locked.
    static std::atomic_bool IS_LOCKED;


    /// User space thread-local storage.
    __thread thread_state THREAD_STATE;


    thread_state_handle::thread_state_handle(void) throw()
        : state(&THREAD_STATE)
    { }


    cpu_state_handle::cpu_state_handle(void) throw()
        : has_lock(false)
    {
        while(!IS_LOCKED.exchange(true)) { /* spin */ }
        has_lock = true;
    }


    cpu_state_handle::cpu_state_handle(cpu_state_handle &&that) throw() {
        has_lock = that.has_lock;
        that.has_lock = false;
    }


    cpu_state_handle::~cpu_state_handle(void) throw() {
        if(has_lock) {
            has_lock = false;
            IS_LOCKED = false;
        }
    }


    cpu_state_handle &cpu_state_handle::operator=(cpu_state_handle &&that) throw() {
        if(this != &that) {
            has_lock = that.has_lock;
            that.has_lock = false;
        }
        return *this;
    }


    cpu_state *cpu_state_handle::operator->(void) throw() {
        return &GLOBAL_STATE;
    }

}



