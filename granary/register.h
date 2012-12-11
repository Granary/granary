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

namespace granary {

    /// Forward declarations.
    struct instruction;
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

        uint32_t live;
        uint32_t undead;

    public:

        register_manager(void) throw();


        /// Visit the registers in the instruction; kill the destination
        /// registers and revive the source registers.
        void visit(dynamorio::instr_t *) throw();


        /// Forcibly kill all registers used within an instruction.
        ///
        /// Note: if the instruction is a call, then all scratch registers are
        ///       killed as well.
        void kill(dynamorio::instr_t *) throw();


        /// Forcibly revive all registers used within an instruction.
        ///
        /// Note: if the instruction is a call, then all scratch registers are
        ///       killed as well.
        void revive(dynamorio::instr_t *) throw();


        /// Revive all registers.
        void revive_all(void) throw();


        /// Kill all registers.
        void kill_all(void) throw();


        /// Forcibly kill/revive all registers used in a particular operand.
        /// Note: zombies can be re-killed/revived.
        void kill(dynamorio::opnd_t) throw();
        void revive(dynamorio::opnd_t) throw();


        /// Forcibly kill/revive a particular register. Note: zombies can be
        /// re-killed/revived.
        void kill(dynamorio::reg_id_t) throw();
        void revive(dynamorio::reg_id_t) throw();


        /// Returns true iff there are any dead registers available.
        bool has_dead(void) const throw();


        /// Returns the next 64-bit "free" dead register.
        dynamorio::reg_id_t get_zombie(void) throw();


        /// Returns the next "free" dead register that is at the same scale as
        /// another register/operand.
        dynamorio::reg_id_t get_zombie(dynamorio::reg_id_t scale) throw();


    private:


        typedef int (opnd_counter)(dynamorio::instr_t *);
        typedef dynamorio::opnd_t (opnd_getter)(dynamorio::instr_t *, dynamorio::uint);

        /// Kill registers used in some class op operands within an instruction.
        void revive(dynamorio::instr_t *, opnd_counter *, opnd_getter *) throw();
        void kill(dynamorio::instr_t *, opnd_counter *, opnd_getter *) throw();

        /// Do opcode-specific killing/reviving.
        void visit(dynamorio::instr_t *in, unsigned num_dests) throw();
    };

}


#endif /* Granary_REGISTER_H_ */
