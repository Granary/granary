/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * instrument.cc
 *
 *  Created on: 2013-06-28
 *      Author: Peter Goodman
 */

#include "clients/cfg/instrument.h"

using namespace granary;


#if GRANARY_IN_KERNEL
#   include "granary/kernel/linux/module.h"
extern "C" {
    /// Returns the kernel module information for a given app address.
    extern const kernel_module *kernel_get_module(app_pc addr);
}
#endif


namespace client {


    /// Entry point for the return-from-call event handler.
    app_pc EVENT_RETURN_FROM_CALL = nullptr;


    /// Entry point for the on-enter-bb event handler.
    app_pc EVENT_ENTER_CALL = nullptr;


    /// Entry point for the on-enter-bb event handler.
    app_pc EVENT_ENTER_BASIC_BLOCK = nullptr;


    /// Entry point for the on-indirect-call event handler.
    app_pc EVENT_CALL_INDIRECT = nullptr;


    /// Entry point for the on-app-call event handler.
    app_pc EVENT_CALL_APP = nullptr;


    /// Entry point for the on-host-call event handler.
    app_pc EVENT_CALL_HOST = nullptr;


    static operand ARGS[5];


    STATIC_INITIALISE_ID(init_cfg_args, {
        ARGS[0] = reg::arg1;
        ARGS[1] = reg::arg2;
        ARGS[2] = reg::arg3;
        ARGS[3] = reg::arg4;
        ARGS[4] = reg::arg5;
    })


    /// Generate an entry point function that saves/restores the appropriate
    /// registers.
    template <typename R, typename... Args>
    static app_pc generate_entry_point(R (*func)(Args...)) throw() {
        register_manager dead_regs(find_used_regs_in_func(
            unsafe_cast<app_pc>(func)));

        for(unsigned i(sizeof...(Args)); i--; ) {
            dead_regs.revive(ARGS[i]);
        }

        instruction_list ls;
        instruction in(ls.append(label_()))
    }


    /// Base case for adding instructions that pass arguments to an event
    /// handler.
    static instruction add_event_args(
        instruction_list &, instruction in, unsigned
    ) throw() {
        return in;
    }


    /// Inductive step for adding arguments that pass arguments to an event
    /// handler.
    template <typename Arg, typename... Args>
    static instruction add_event_args(
        instruction_list &ls,
        instruction in,
        unsigned i, Arg arg,
        Args... args
    ) throw() {
        in = ls.insert_after(in, mov_imm_(
            ARGS[i],
            int64_(reinterpret_cast<uint64_t>(arg))));
        return add_event_args(ls, in, i + 1, args...);
    }


    /// Add a call to an event handler that passes in the basic block state.
    template <typename... Args>
    void add_event_call(
        instruction_list &ls,
        instruction in,
        app_pc event_handler,
        Args... args
    ) throw() {
        IF_USER( in = ls.insert_after(in,
            lea_(reg::rsp, reg::rsp[-REDZONE_SIZE])); )

        // Spill the argument registers and pass in the values to a call.
        in = add_event_args(ls, in, 0, args...);

        // Add a call out to an event handler.
        in = insert_cti_after(
            ls, in,
            event_handler, false, operand(),
            CTI_CALL);
        in.set_mangled();

        // Pop any pushed argument registers.
        for(unsigned i(sizeof...(Args)); i--; ) {
            in = ls.insert_after(in, pop_(ARGS[i]));
        }

        IF_USER( in = ls.insert_after(in,
            lea_(reg::rsp, reg::rsp[REDZONE_SIZE])); )
    }


    /// Initialise the basic block state and add in calls to event handlers.
    void instrument_basic_block(
        basic_block_state &bb,
        instruction_list &ls
    ) throw() {
        instrument_basic_block(bb, ls);

        instruction in(ls.last());

        bb.num_executions = 0;
        bb.num_interrupts = 0;

        bb.used_regs.kill_all();
        bb.entry_regs.kill_all();

        IF_KERNEL( app_pc start_pc(nullptr); )
        IF_KERNEL( app_pc end_pc(nullptr); )

        for(instruction prev_in; in.is_valid(); in = prev_in) {

            prev_in = in.prev();

            // Track the registers.
            bb.used_regs.revive(in);
            bb.entry_regs.visit(in);

#if GRANARY_IN_KERNEL
            // Track the translation so that we can find the offset.
            if(in.pc()) {
                start_pc = in.pc();
                if(!end_pc) {
                    end_pc = start_pc;
                }
            }
#endif /* GRANARY_IN_KERNEL */

            if(in.is_call()) {
                instruction call_in(in);
                operand target(in.cti_target());

                in = ls.insert_before(call_in, label_());

                // Direct call.
                if(dynamorio::opnd_is_pc(target)) {

                    if(is_app_address(target.value.pc)) {
                        add_event_call(ls, in, EVENT_CALL_APP, &bb);
                    } else {
                        add_event_call(
                            ls, in, EVENT_CALL_HOST, &bb, target.value.pc);
                    }

                // Indirect call.
                } else {
                    add_event_call(ls, in, EVENT_CALL_INDIRECT, &bb);
                }

                in = ls.insert_after(call_in, label_());
                add_event_call(ls, in, EVENT_RETURN_FROM_CALL, &bb);
            }
        }

        // Add an event handler that executes before the basic block executes.
        in = ls.insert_before(ls.first(), label_());
        add_event_call(ls, in, EVENT_ENTER_BASIC_BLOCK, &bb);

#if GRANARY_IN_KERNEL
        // Record the name and relative offset of the code within the
        // module. We can correlate this offset with an `objdump` or do
        // static instrumentation of the ELF/KO based on this offset.
        const kernel_module *module(kernel_get_module(start_pc));
        if(module) {
            const uintptr_t app_begin(
                reinterpret_cast<uintptr_t>(module->text_begin));

            bb.app_offset_begin = reinterpret_cast<uintptr_t>(start_pc) \
                                - app_begin;

            bb.app_offset_end = reinterpret_cast<uintptr_t>(end_pc) \
                              - app_begin;

            bb.app_name = module->name;
        }
#endif /* GRANARY_IN_KERNEL */
    }


    instrumentation_policy visit_app_instructions(
        cpu_state_handle &cpu,
        basic_block_state &bb,
        instruction_list &ls
    ) throw() {

        return policy_for<cfg_exit_policy>();
    }

}

