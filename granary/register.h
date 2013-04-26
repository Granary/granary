/*
 * register.h
 *
 *  Created on: 2012-11-30
 *      Author: pag
 *     Version: $Id$
 */

#ifndef Granary_REGISTER_H_
#define Granary_REGISTER_H_

#include "granary/globals.h"
#include "granary/instruction.h"

namespace granary {

    /// Forward declarations.
    struct instruction_list;
    struct operand;


    /// A class for managing spill registers, dead registers, etc.
    ///
    /// This class can be used to track dead registers in an instruction list
    /// by visiting instructions in reverse order.
    ///
    /// This class can also be used to get zombie (i.e. spill) registers for
    /// used.
    struct register_manager {
    private:

        /// Tracks 64-bit registers.
        uint16_t live;
        uint16_t undead;

        /// Tracks 128-bit XMM registers.
        uint16_t live_xmm;
        uint16_t undead_xmm;

    public:


        /// Initialise the register manager so that every register is live.
        register_manager(void) throw();


        /// Visit the registers in the instruction; kill the destination
        /// registers and revive the source registers.
        void visit(dynamorio::instr_t *) throw();
        inline void visit(instruction in) throw() {
            return visit(in.instr);
        }


        /// Forcibly kill all registers used within an instruction.
        ///
        /// Note: if the instruction is a call, then all scratch registers are
        ///       killed as well.
        void kill(dynamorio::instr_t *) throw();
        inline void kill(instruction in) throw() {
            return kill(in.instr);
        }


        /// Forcibly revive all registers used within an instruction.
        ///
        /// Note: if the instruction is a call, then all scratch registers are
        ///       killed as well.
        void revive(dynamorio::instr_t *) throw();
        inline void revive(instruction in) throw() {
            return revive(in.instr);
        }


        /// Revive all registers.
        void revive_all(void) throw();
        void revive_all_xmm(void) throw();


        /// Revive all registers used in another register manager (including
        /// zombies). This is like a set union.
        void revive_all(register_manager) throw();


        /// Kill all registers.
        void kill_all(void) throw();
        void kill_all_live(void) throw();


        /// Forcibly kill/revive all registers used in a particular operand.
        /// Note: zombies can be re-killed/revived.
        void kill(dynamorio::opnd_t) throw();
        void revive(dynamorio::opnd_t) throw();


        /// Forcibly kill/revive a particular register. Note: zombies can be
        /// re-killed/revived.
        void kill(dynamorio::reg_id_t) throw();
        void revive(dynamorio::reg_id_t) throw();

    private:

        void kill_64(dynamorio::reg_id_t) throw();
        void revive_64(dynamorio::reg_id_t) throw();

        void kill_xmm(dynamorio::reg_id_t) throw();
        void revive_xmm(dynamorio::reg_id_t) throw();

    public:


        /// Returns true iff there are any dead registers available.
        bool has_dead(void) throw();


        /// Returns true iff a particular register is alive.
        bool is_live(dynamorio::reg_id_t) throw();


        /// Returns true iff a particular register is dead.
        bool is_dead(dynamorio::reg_id_t) throw();


        /// Returns true iff a particular register is a walker, i.e.
        /// living or a zombie!
        bool is_undead(dynamorio::reg_id_t) throw();


        /// Returns the next 64-bit "free" dead register.
        dynamorio::reg_id_t get_zombie(void) throw();


        /// Returns the next xmm "free" dead register.
        dynamorio::reg_id_t get_xmm_zombie(void) throw();


        /// Returns the next "free" dead register that is at the same scale as
        /// another register/operand.
        dynamorio::reg_id_t get_zombie(dynamorio::reg_id_t scale) throw();


    private:


        typedef int (opnd_counter)(dynamorio::instr_t *);
        typedef dynamorio::opnd_t (opnd_getter)(dynamorio::instr_t *, dynamorio::uint);


        /// Kill registers used in some class op operands within an instruction.
        void revive(dynamorio::instr_t *, opnd_counter *, opnd_getter *) throw();
        void kill(dynamorio::instr_t *, opnd_counter *, opnd_getter *) throw();

        inline void revive(
            instruction in,
            opnd_counter *counter,
            opnd_getter *getter
        ) throw() {
            return revive(in.instr, counter, getter);
        }

        inline void kill(
            instruction in,
            opnd_counter *counter,
            opnd_getter *getter
        ) throw() {
            return kill(in.instr, counter, getter);
        }


        /// Do opcode-specific killing/reviving.
        void visit(dynamorio::instr_t *in, unsigned num_dests) throw();

        inline void visit(instruction in, unsigned num_dests) throw() {
            return visit(in.instr, num_dests);
        }
    };

}


#endif /* Granary_REGISTER_H_ */
