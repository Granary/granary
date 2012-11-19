/*
 * state.h
 *
 *   Copyright: Copyright 2012 Peter Goodman, all rights reserved.
 *      Author: Peter Goodman
 */

#ifndef granary_STATE_H_
#define granary_STATE_H_

#include <atomic>
#include <bitset>

#if GRANARY_IN_KERNEL
#	include "granary/kernel/linux/per_cpu.h"
#endif

#include "clients/state.h"

namespace granary {


    struct thread_state;
    struct cpu_state;
    struct basic_block_state;


    /// Information maintained by granary about each thread.
    struct thread_state : public client::thread_state {
    public:

    	/// Direct branch lookup slots
    	enum {
    		NUM_DIRECT_BRANCH_SLOTS = 16ULL
    	};

    	std::bitset<NUM_DIRECT_BRANCH_SLOTS> used_slots;
    	struct {

    		/// The native address of where a direct branch targets
    		app_pc target_pc;

    		/// The address of the instruction in the code cache.
    		/// This instruction is patched once the target_pc is
    		/// translated.
    		app_pc patch_pc;

    	} direct_branch_slots[NUM_DIRECT_BRANCH_SLOTS];
    };


    /// Information maintained by granary about each CPU.
    ///
    /// Note: when in kernel space, we assume that this state
    /// 	  is only accessed with interrupts disabled.
    struct cpu_state : public client::cpu_state {
    public:
#if GRANARY_IN_KERNEL



    	static cpu_state *get(void) throw() {
    		DEFINE_PER_CPU(cpu_state, state);
    	}

#else:

    private:

    	/// No good concept of per-cpu state in user space so
    	/// we'll just have a global state whose accesses are
    	/// serialized by a simple spinlock.
    	static cpu_state GLOBAL_STATE;
    	static std::atomic_bool IS_LOCKED;

    	struct cpu_state_handle {
    	private:
    		bool has_lock;

    	public:

    		cpu_state_handle(void) throw()
    			: has_lock(false)
    		{
    			while(!IS_LOCKED.exchange(true)) { /* spin */ }
    			has_lock = true;
    		}

    		cpu_state_handle(cpu_state_handle &&that) throw() {
    			has_lock = that.has_lock;
    			that.has_lock = false;
    		}

    		~cpu_state_handle(void) throw() {
    			if(has_lock) {
    				has_lock = false;
    				IS_LOCKED = false;
    			}
    		}

    		cpu_state_handle &operator=(cpu_state_handle &&that) throw() {
    			if(this != &that) {
    				has_lock = that.has_lock;
    				that.has_lock = false;
    			}
    			return *this;
    		}

    		cpu_state *operator->(void) throw() {
    			return &GLOBAL_STATE;
    		}
    	};
    public:

    	/// Get the CPU-private state
    	static cpu_state_handle get(void) throw() {
    		return cpu_state_handle();
    	}
#endif

    	struct transient_allocator {

    	};
    };


    /// Information maintained within each emitted basic block of
    /// translated application/module code.
    struct basic_block_state : public client::basic_block_state {
    public:

    };
}

#endif /* granary_STATE_H_ */
