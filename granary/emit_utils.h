/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * emit_utils.h
 *
 *  Created on: 2013-02-06
 *      Author: pag
 */

#ifndef EMIT_UTILS_H_
#define EMIT_UTILS_H_

#include "granary/instruction.h"
#include "granary/register.h"

namespace granary {


    /// Returns true iff the two values are very far (>= 4GB) away.
    template <typename P1, typename P2>
    inline bool is_far_away(P1 p1, P2 p2) throw() {

        enum {
            _4GB = 4294967296LL
        };

        int64_t diff(
            reinterpret_cast<uint64_t>(p1) - reinterpret_cast<uint64_t>(p2));
        if(diff < 0) {
            diff = -diff;
        }
        return diff >= _4GB;
    }


    /// Returns true iff some address can sign-extend from 32 bits to 64 bits.
    template <typename P>
    inline bool addr_is_32bit(P addr_) throw() {
        const uint64_t addr(reinterpret_cast<uint64_t>(addr_));
        const uint64_t addr_sign(addr >> 31);
        const uint32_t addr_high(addr_sign >> 1);
        if(1 & addr_sign) {
            return ~0U == addr_high;
        } else {
            return 0U == addr_high;
        }
    }


    enum instruction_traversal_constraint {
        USED_REGS_VISIT_ALL_INSTRUCTIONS,
        USED_REGS_IGNORE_CALLS
    };


    /// Traverse through the instruction control-flow graph and look for used
    /// registers.
    register_manager find_used_regs_in_func(
        app_pc func,
        instruction_traversal_constraint constraint=USED_REGS_VISIT_ALL_INSTRUCTIONS
    ) throw();


    /// Push all registers that are dead in a register manager.
    instruction save_registers(
        register_manager regs,
        instruction_list &ls,
        instruction in
    ) throw();


    /// Pop all registers that are dead in a register manager.
    instruction restore_registers(
        register_manager regs,
        instruction_list &ls,
        instruction in
    ) throw();


    /// Save all dead 64-bit registers within a particular register manager.
    /// This is useful for saving/restoring only those registers used by a
    /// function.
    instruction save_and_restore_registers(
        register_manager regs,
        instruction_list &ls,
        instruction in
    ) throw();


    enum xmm_save_constraint {
        XMM_SAVE_ALIGNED,
        XMM_SAVE_UNALIGNED
    };


    /// Save all dead xmm registers within a particular register manager.
    /// This is useful for saving/restoring only those registers used by a
    /// function.
    instruction save_and_restore_xmm_registers(
        register_manager regs,
        instruction_list &ls,
        instruction in,
        xmm_save_constraint
    ) throw();


    enum cti_kind {
        CTI_CALL,
        CTI_JMP
    };


    enum cti_register_steal_constraint {
        CTI_STEAL_REGISTER = true,
        CTI_DONT_STEAL_REGISTER = false
    };


    /// Add a call to a known function after a particular instruction in the
    /// instruction stream. If the location of the function to be called is
    /// too far away then a specified register is clobbered with the target pc,
    /// and an indirect jump is performed. If no clobber register is available
    /// then an indirect jump slot is allocated and used.
    instruction insert_cti_after(
        instruction_list &ls,
        instruction in,
        app_pc target,
        cti_register_steal_constraint steal_constraint,
        operand clobber_reg,
        cti_kind kind
    ) throw();


    enum flag_save_constraint {
        REG_AH_IS_DEAD,
        REG_AH_IS_LIVE
    };


    /// Add the instructions to save the flags onto the top of the stack.
    instruction insert_save_flags_after(
        instruction_list &ls,
        instruction in,
        flag_save_constraint constraint=REG_AH_IS_LIVE
    ) throw();


    /// Add the instructions to restore the flags from the top of the stack.
    instruction insert_restore_flags_after(
        instruction_list &ls,
        instruction in,
        flag_save_constraint constraint=REG_AH_IS_LIVE
    ) throw();


    /// Add instructions to align the stack (to the top of the stack) to a 16
    /// byte boundary.
    instruction insert_align_stack_after(
        instruction_list &ls,
        instruction in
    ) throw();


    /// Add instructions to restore the stack's previous alignment.
    instruction insert_restore_old_stack_alignment_after(
        instruction_list &ls,
        instruction in
    ) throw();


    /// Argument registers.
    extern operand ARGUMENT_REGISTERS[];


    /// Contraints on exit points from the code cache.
    enum register_exit_constaint {

        /// The exit point follows the Itanium C++ ABI.
        EXIT_REGS_ABI_COMPATIBLE,

        /// The exit point has no constraints, and so all registers might be
        /// suspected of being clobbered.
        EXIT_REGS_UNCONSTRAINED
    };


    namespace detail {
        app_pc generate_clean_callable_address(
            app_pc func_pc,
            unsigned num_args,
            register_exit_constaint constraint
        ) throw();
    }


    /// Generate a clean way of exiting the code cache through a CALL that will
    /// correctly save/restore registers and align the stack.
    ///
    /// Note: This will assume that whoever invokes this exit point is
    ///       responsible for the argument registers.
    template <typename R, typename... Args>
    app_pc generate_clean_callable_address(
        R (*func)(Args...),
        register_exit_constaint constraint=EXIT_REGS_ABI_COMPATIBLE
    ) throw() {
        app_pc func_pc(unsafe_cast<app_pc>(func));
        return detail::generate_clean_callable_address(
            func_pc, sizeof...(Args), constraint);
    }


    namespace detail {


        /// Represents an argument to a clean call function.
        struct clean_call_argument {

            typedef instruction instrument_argument_t(
                instruction_list &ls,
                instruction in,
                operand reg
            );

            union {
                instrument_argument_t *as_func;
                uint64_t as_value;
            };

            bool is_runtime_arg;

            /// A clean call argument that will be generated by a sequence of
            /// instructions that compute the argument. The instructions must be
            /// emitted by the function.
            inline clean_call_argument(instrument_argument_t *func_) throw()
                : as_func(func_)
                , is_runtime_arg(true)
            { }


            /// A translation-time known clean call argument. These must be 64 bit
            /// values.
            template <typename T>
            inline clean_call_argument(T val_) throw()
                : as_value(unsafe_cast<uint64_t>(val_))
                , is_runtime_arg(false)
            { }
        };


        /// Base case for adding instructions that pass arguments to an event
        /// handler.
        inline instruction insert_clean_call_arguments_after(
            instruction_list &, instruction in, unsigned
        ) throw() {
            return in;
        }


        /// Inductive step for adding arguments that pass arguments to an event
        /// handler.
        template <typename Arg, typename... Args>
        static instruction insert_clean_call_arguments_after(
            instruction_list &ls,
            instruction in,
            unsigned i,
            Arg arg_,
            Args... args
        ) throw() {
            clean_call_argument arg(arg_);

            // Append instructions to compute the argument at runtime.
            if(arg.is_runtime_arg) {
                in = arg.as_func(ls, in, ARGUMENT_REGISTERS[i]);

            // The argument is known at translation time.
            } else {
                in = ls.insert_after(in, mov_imm_(
                    ARGUMENT_REGISTERS[i], int64_(arg.as_value)));
            }

            return insert_clean_call_arguments_after(ls, in, i + 1, args...);
        }

    }


    /// Add a call to an clean-callable function.
    template <typename... Args>
    instruction insert_clean_call_after(
        instruction_list &ls,
        instruction in,
        app_pc clean_call_func,
        Args... args
    ) throw() {
        IF_USER( in = ls.insert_after(in,
            lea_(reg::rsp, reg::rsp[-REDZONE_SIZE])); )

        // Spill argument registers.
        for(unsigned i(0); i < sizeof...(Args); ++i) {
            in = ls.insert_after(in, push_(ARGUMENT_REGISTERS[i]));
        }

        // Assign input variables (assumed to be integrals) to the argument
        // registers.
        in = detail::insert_clean_call_arguments_after(ls, in, 0, args...);

        // Add a call out to an event handler.
        in = insert_cti_after(
            ls, in, clean_call_func,
            CTI_DONT_STEAL_REGISTER, operand(),
            CTI_CALL);
        in.set_mangled();

        // Pop any pushed argument registers.
        for(unsigned i(sizeof...(Args)); i--; ) {
            in = ls.insert_after(in, pop_(ARGUMENT_REGISTERS[i]));
        }

        IF_USER( in = ls.insert_after(in,
            lea_(reg::rsp, reg::rsp[REDZONE_SIZE])); )

        return in;
    }
}


#endif /* EMIT_UTILS_H_ */
