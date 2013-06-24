/*
 * leak_policy.cc
 *
 *  Created on: 2013-06-21
 *      Author: akshayk
 */

#include "clients/watchpoints/policies/leak_detector/leak_policy.h"

using namespace granary;


namespace client { namespace wp {

    /// Initialise the leak policy enter.
    STATIC_INITIALISE({
        app_leak_policy_enter::init();
    })

    void app_leak_policy_enter::init() throw() {
        granary::printf("leak policy init !!!!!!!!!!!!!!!!\n");
    }



    /// Instrument the first entrypoint into instrumented code.
    void instrument_entry_to_code_cache(
        granary::instruction_list &ls,
        granary::instruction in
    ) throw() {
        using namespace granary;
        register_manager rm;
        rm.kill_all();

        in = ls.insert_before(in, label_());
        in = save_and_restore_registers(rm, ls, in);
        in = insert_cti_after(
            ls, in,
            unsafe_cast<app_pc>(on_enter_code_cache), true, reg::rax,
            CTI_CALL);
        in.set_mangled();
    }


    /// Instrument a exit from code code.
    void instrument_exit_from_code_cache(
        granary::instruction_list &ls,
        granary::instruction in
    ) throw() {
        using namespace granary;
        register_manager rm;
        rm.kill_all();

        in = ls.insert_before(in, label_());
        in = save_and_restore_registers(rm, ls, in);
        in = insert_cti_after(
            ls, in,
            unsafe_cast<app_pc>(on_exit_code_cache), true, reg::rax,
            CTI_CALL);
        in.set_mangled();
    }



#if CONFIG_CLIENT_HANDLE_INTERRUPT
    interrupt_handled_state app_leak_policy_enter::handle_interrupt(
        cpu_state_handle &,
        thread_state_handle &,
        granary::basic_block_state &,
        interrupt_stack_frame &,
        interrupt_vector vec
    ) throw() {
        if(VECTOR_OVERFLOW == vec) {
            return INTERRUPT_IRET;
        }
        return INTERRUPT_DEFER;
    }

    interrupt_handled_state app_leak_policy_exit::handle_interrupt(
        cpu_state_handle &,
        thread_state_handle &,
        granary::basic_block_state &,
        interrupt_stack_frame &,
        interrupt_vector vec
    ) throw() {
        if(VECTOR_OVERFLOW == vec) {
            return INTERRUPT_IRET;
        }
        return INTERRUPT_DEFER;
    }

    interrupt_handled_state app_leak_policy_continue::handle_interrupt(
        cpu_state_handle &,
        thread_state_handle &,
        granary::basic_block_state &,
        interrupt_stack_frame &,
        interrupt_vector vec
    ) throw() {
        if(VECTOR_OVERFLOW == vec) {
            return INTERRUPT_IRET;
        }
        return INTERRUPT_DEFER;
    }
#endif /* CONFIG_CLIENT_HANDLE_INTERRUPT */

}

    /// Visit app instructions for leak_policy_continue
    granary::instrumentation_policy leak_policy_continue::visit_app_instructions(
        granary::cpu_state_handle &cpu,
        granary::thread_state_handle &thread,
        granary::basic_block_state &bb,
        granary::instruction_list &ls
    ) throw() {
        granary::printf("inside policy leak_continue\n");

        granary::instruction in(ls.first());

        for(; in.is_valid(); in = in.next()) {
            if(in.is_return()){
                in.set_policy(granary::policy_for<client::leak_policy_exit>());
            }
        }

        return client::watchpoints<
                wp::app_leak_policy_continue,
                wp::host_leak_policy_continue>
                ::visit_app_instructions(cpu, thread, bb, ls);
    }


    /// Visit app instructions for leak_policy_exit
    granary::instrumentation_policy leak_policy_exit::visit_app_instructions(
        granary::cpu_state_handle &cpu,
        granary::thread_state_handle &thread,
        granary::basic_block_state &bb,
        granary::instruction_list &ls
    ) throw() {
        granary::printf("inside policy leak_exit\n");

        granary::instruction in(ls.first());

        for(; in.is_valid(); in = in.next()) {
            if(in.is_call()) {
                in.set_policy(granary::policy_for<leak_policy_continue>());
            } else if(in.is_return()){
                wp::instrument_exit_from_code_cache(ls, in);
            }
        }

        return client::watchpoints<
                wp::app_leak_policy_exit,
                wp::host_leak_policy_exit>
                ::visit_app_instructions(cpu, thread, bb, ls);
    }

} /* client namespace */
