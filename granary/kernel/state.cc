/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * state.cc
 *
 *  Created on: 2012-11-19
 *      Author: pag
 *     Version: $Id$
 */

#include "granary/globals.h"
#include "granary/state.h"

extern "C" {


    /// Get access to the CPU-private state.
    extern granary::cpu_state **kernel_get_cpu_state(granary::cpu_state *[]);


    /// Used to run a function on each CPU.
    extern void kernel_run_on_each_cpu(void (*func)(void));


    /// Call a function where all CPUs are synchronised.
    void kernel_run_synchronised(void (*func)(void));


    /// Mark a page as being only readable.
    void kernel_make_page_read_only(void *addr);


    /// Get the kernel's actual IDT base.
    descriptor_t *kernel_get_idt_table(void);
}


namespace granary {


    enum {
        MAX_NUM_CPUS = 256
    };


    /// Manually manage our own per-cpu state.
    cpu_state *CPU_STATES[MAX_NUM_CPUS] = {nullptr};


#if CONFIG_DEBUG_CHECK_CPU_ACCESS_SAFE
    /// Check that it's safe to access CPU-private state.
    __attribute__((noinline))
    void check_cpu_access_safety(void) {
        const eflags flags = granary_load_flags();
        if(flags.interrupt) {
            granary_break_on_curiosity();
        }
    }
#endif


    extern "C" uint64_t *granary_get_private_stack_top(void)
    {
#if CONFIG_DEBUG_CHECK_CPU_ACCESS_SAFE && 0
        check_cpu_access_safety();
#endif
        return &((*kernel_get_cpu_state(CPU_STATES))->stack.top[0]);
    }

    namespace detail {
        struct dummy_thread_state : public client::thread_state {
            uint64_t dummy_val;
        };
    }


    /// Figure out the size of the CPU state.
    enum {
        DUMMY_THREAD_STATE_SIZE = sizeof(detail::dummy_thread_state),
        ALIGNED_THREAD_STATE_SIZE = sizeof(granary::thread_state)
                                  + ALIGN_TO(sizeof(granary::thread_state), 8),
        THREAD_STATE_SIZE = (sizeof(uint64_t) == DUMMY_THREAD_STATE_SIZE)
                          ? 0
                          : ALIGNED_THREAD_STATE_SIZE
    };


    /// Gets a handle to the current CPU state.
    cpu_state_handle::cpu_state_handle(void) throw()
        : state(*kernel_get_cpu_state(CPU_STATES))
    { }


    /// Represents CPU state that is actually allocated and has two poisoned
    /// pages around it.
    struct poison {
        char poison[PAGE_SIZE];
    } __attribute__((packed, aligned(CONFIG_ARCH_PAGE_SIZE)));


    static std::atomic<unsigned> NEXT_CPU_ID = ATOMIC_VAR_INIT(0U);


    /// Allocate and initialise state for each CPU.
    static void alloc_cpu_state(void) {
        cpu_state **state_ptr(kernel_get_cpu_state(CPU_STATES));

        ASSERT(nullptr == *state_ptr);

        *state_ptr = allocate_memory<cpu_state>();
        cpu_state *state(*state_ptr);
        state->id = NEXT_CPU_ID.fetch_add(1);

#if CONFIG_FEATURE_HANDLE_INTERRUPTS || CONFIG_FEATURE_INSTRUMENT_HOST
        // Get a copy of the native IDTR.
        get_idtr(&(state->native_idtr));

        // A bit of defensive programming here; we're concerned that the kernel
        // has loaded the IDTR with a fixed, read-only address so as to not
        // leak the kernel's base address.
        if(!is_host_address(state->native_idtr.base)) {
            state->native_idtr.base = kernel_get_idt_table();
        }
#endif

#if CONFIG_FEATURE_INSTRUMENT_HOST
        /// Get a copy of the native MSR_LSTAR model-specific register.
        state->native_msr_lstar = get_msr(MSR_LSTAR);
#endif
    }


    /// Initialise the CPU state.
    void cpu_state::init_early(void) throw() {
        kernel_run_on_each_cpu(alloc_cpu_state);
    }


    /// Initialise the IDTR and MSR_LSTAR for each CPU.
    static void set_percpu(void) {
        cpu_state_handle cpu;

#if CONFIG_FEATURE_HANDLE_INTERRUPTS && !CONFIG_DEBUG_INITIALISE
        set_idtr(&(cpu->idtr));
#endif

#if CONFIG_FEATURE_INSTRUMENT_HOST && !CONFIG_DEBUG_INITIALISE
        set_msr(MSR_LSTAR, cpu->msr_lstar);
#endif

        UNUSED(cpu);
    }


    void cpu_state::init_late(void) throw() {
        eflags flags;
        for(unsigned i(0); i < MAX_NUM_CPUS; ++i) {

            if(!CPU_STATES[i]) {
                continue;
            }

#if CONFIG_FEATURE_HANDLE_INTERRUPTS
#   if CONFIG_FEATURE_INTERRUPT_DELAY
            // Allocate space for interrupt delay handlers.
            CPU_STATES[i]->interrupt_delay_handler = reinterpret_cast<app_pc>(
                global_state::FRAGMENT_ALLOCATOR-> \
                    allocate_untyped(16, INTERRUPT_DELAY_CODE_SIZE));
#   endif
            // Create this CPUs IDT and vector entry points.
            flags = granary_disable_interrupts();
            {
                cpu_state_handle cpu;
                IF_TEST( cpu->in_granary = false; )
                enter(cpu);
                CPU_STATES[i]->idtr = create_idt(CPU_STATES[i]->native_idtr);
            }
            granary_store_flags(flags);
#endif
#if CONFIG_FEATURE_INSTRUMENT_HOST

            // Create this CPUs SYSCALL entry point.
            flags = granary_disable_interrupts();
            {
                cpu_state_handle cpu;
                IF_TEST( cpu->in_granary = false; )
                enter(cpu);
                CPU_STATES[i]->msr_lstar = create_syscall_entrypoint(
                    CPU_STATES[i]->native_msr_lstar);
            }
            granary_store_flags(flags);
#endif

#if CONFIG_FEATURE_HANDLE_INTERRUPTS
            // Page protect the IDTs.
            kernel_make_page_read_only(CPU_STATES[i]->idtr.base);
#endif
        }

        kernel_run_on_each_cpu(set_percpu);

        // Performed initialisation, where all CPUs are first synchronised.
        if(should_init_sync()) {
            kernel_run_synchronised(&init_sync);
        }

        UNUSED(flags);
    }


    /// Note: thread_state_handle::thread_state_handle and ::init are in the
    ///       OS-specific state.cc file.
}
