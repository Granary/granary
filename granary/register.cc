/*
 * register.cc
 *
 *  Created on: 2012-11-30
 *      Author: pag
 *     Version: $Id$
 */


#include "granary/register.h"
#include "granary/instruction.h"

namespace granary {


    /// Registers that are forced to always be alive.
    static uint16_t FORCE_LIVE(0);


    STATIC_INITIALISE({
        FORCE_LIVE |= (1 << (dynamorio::DR_REG_RSP - 1));
    })


    /// Initialise the register manager so that every register is live.
    register_manager::register_manager(void) throw()
        : live(~0)
        , undead(0)
        , live_xmm(~0)
        , undead_xmm(0)
    { }


    /// Kill all registers.
    void register_manager::kill_all(void) throw() {
        live = FORCE_LIVE;
        undead = 0;

        live_xmm = 0;
        undead_xmm = 0;
    }


    /// Kill all live registers.
    void register_manager::kill_all_live(void) throw() {
        live = FORCE_LIVE;
        live_xmm = 0;
    }


    /// Revive all registers.
    void register_manager::revive_all(void) throw() {
        live = ~0;
        undead = 0;

        live_xmm = ~0;
        undead_xmm = 0;
    }


    /// Revive all registers used in another register manager.
    void register_manager::revive_all(register_manager that) throw() {
        live |= that.live;
        undead |= that.undead;

        live_xmm |= that.live_xmm;
        undead_xmm |= that.undead_xmm;
    }


    /// Kill registers used in some class op operands within an instruction.
    void register_manager::kill(dynamorio::instr_t *in,
                                opnd_counter *num_ops,
                                opnd_getter *which_op) throw() {
        for(int i(0); i < num_ops(in); i++) {
            kill(which_op(in, i));
        }
    }


    /// Revive registers used in some class op operands within an instruction.
    void register_manager::revive(dynamorio::instr_t *in,
                                  opnd_counter *num_ops,
                                  opnd_getter *which_op) throw() {
        for(int i(0); i < num_ops(in); i++) {
            revive(which_op(in, i));
        }
    }


    /// Visit the registers in the instruction; kill the destination
    /// registers and revive the source registers.
    void register_manager::visit(dynamorio::instr_t *in) throw() {

        if(dynamorio::instr_is_cti(in)
        && !dynamorio::instr_is_call(in)
        && !dynamorio::instr_is_return(in)) {
            revive_all();
            return;
        }

        // only kill registers written to; base/disp types are actually
        // sources.
        const unsigned num_dests(dynamorio::instr_num_dsts(in));
        for(unsigned i(0); i < num_dests; i++) {
            dynamorio::opnd_t opnd(dynamorio::instr_get_dst(in, i));
            if(dynamorio::REG_kind == opnd.kind) {
                kill(opnd);
            } else {
                revive(opnd);
            }
        }

        revive(in, dynamorio::instr_num_srcs, dynamorio::instr_get_src);
    }


    /// Visit the registers in the instruction; kill the destination
    /// registers and revive the source registers.
    void register_manager::kill(dynamorio::instr_t *in) throw() {
        kill(in, dynamorio::instr_num_dsts, dynamorio::instr_get_dst);
        kill(in, dynamorio::instr_num_srcs, dynamorio::instr_get_src);
    }


    /// Visit the registers in the instruction; kill the destination
    /// registers and revive the source registers.
    void register_manager::revive(dynamorio::instr_t *in) throw() {
        revive(in, dynamorio::instr_num_dsts, dynamorio::instr_get_dst);
        revive(in, dynamorio::instr_num_srcs, dynamorio::instr_get_src);
    }


    /// Forcibly kill all registers used in a particular operand.
    void register_manager::kill(dynamorio::opnd_t op) throw() {
        if(dynamorio::BASE_DISP_kind == op.kind) {
            if(dynamorio::DR_REG_NULL == op.seg.segment) {
                kill(op.value.base_disp.base_reg);
                kill(op.value.base_disp.index_reg);
            }
        } else if(dynamorio::REG_kind == op.kind) {
            kill(op.value.reg);
        }
    }


    /// Forcibly revive all registers used in a particular operand.
    void register_manager::revive(dynamorio::opnd_t op) throw() {
        if(dynamorio::BASE_DISP_kind == op.kind) {
            revive(op.value.base_disp.base_reg);
            revive(op.value.base_disp.index_reg);
        } else if(dynamorio::REG_kind == op.kind) {
            revive(op.value.reg);
        }
    }


    /// Scale a register to become a 64-bit register. This returns a valid
    /// reg_id_t type, i.e. where 0 = null, 1 = rax, etc.
    static uint8_t reg_to_reg64(dynamorio::reg_id_t reg) throw() {
        if(reg < dynamorio::DR_REG_SPL) {
            while(reg >= dynamorio::DR_REG_EAX) {
                reg -= (dynamorio::DR_REG_EAX - 1);
            }
            return reg;
        }
        return dynamorio::DR_REG_NULL;
    }


    /// Convert a (possibly) xmm register to be in the range [1, 16], where 0
    /// is the null register.
    static uint8_t reg_to_xmm(dynamorio::reg_id_t reg) throw() {
        if(dynamorio::DR_REG_XMM0 <= reg && reg <= dynamorio::DR_REG_XMM15) {
            return reg - dynamorio::DR_REG_XMM0 + 1;
        }
        return dynamorio::DR_REG_NULL;
    }


    /// Forcible kill a particular register.
    void register_manager::kill(dynamorio::reg_id_t reg) throw() {
        if(dynamorio::DR_REG_XMM0 <= reg) {
            kill_xmm(reg);
        } else {
            kill_64(reg);
        }
    }


    /// Forcible revive a particular register.
    void register_manager::revive(dynamorio::reg_id_t reg) throw() {
        if(dynamorio::DR_REG_XMM0 <= reg) {
            revive_xmm(reg);
        } else {
            revive_64(reg);
        }
    }


    /// Forcible kill a particular xmm register.
    void register_manager::kill_xmm(dynamorio::reg_id_t reg) throw() {
        const uint8_t reg_xmm(reg_to_xmm(reg));
        if(reg_xmm) {
            const uint16_t mask(1U << (reg_xmm - 1));
            undead_xmm &= ~mask;
            live_xmm &= ~mask;
        }
    }


    /// Forcible revive a particular xmm register.
    void register_manager::revive_xmm(dynamorio::reg_id_t reg) throw() {
        const uint8_t reg_xmm(reg_to_xmm(reg));
        if(reg_xmm) {
            const uint16_t mask(1U << (reg_xmm - 1));
            undead_xmm &= ~mask;
            live_xmm |= mask;
        }
    }


    /// Forcibly kill a particular 64-bit register. Here we do something
    /// special in that we don't consider a register dead unless we
    /// are actually using the full 64-bit register.
    void register_manager::kill_64(dynamorio::reg_id_t reg) throw() {

        const uint8_t reg64(reg_to_reg64(reg));
        if(reg64 && reg64 == reg) {
            const uint16_t mask(1U << (reg64 - 1));
            undead &= ~mask;
            live &= ~mask;
            live |= FORCE_LIVE;
        }
    }


    /// Forcibly revive a particular 64-bit register.
    void register_manager::revive_64(dynamorio::reg_id_t reg) throw() {
        const uint8_t reg64(reg_to_reg64(reg));
        if(reg64) {
            const uint16_t mask(1U << (reg64 - 1));
            undead &= ~mask;
            live |= mask;
        }
    }


    /// Returns the next 64-bit "free" dead register. Note: < reg15 instead of
    /// <= reg15 because reg_null=0 and it is not represented in the bitset.
    dynamorio::reg_id_t register_manager::get_zombie(void) throw() {
        uint64_t zombies((live | undead));
        for(unsigned pos(0); pos < dynamorio::DR_REG_R15; ++pos) {
            const uint16_t mask(1U << pos);
            if(!(mask & zombies)) {
                undead |= mask;
                return static_cast<dynamorio::reg_id_t>(pos + 1);
            }
        }
        return dynamorio::DR_REG_NULL;
    }


    /// Returns the next xmm "free" dead register.
    dynamorio::reg_id_t register_manager::get_xmm_zombie(void) throw() {
        uint64_t zombies((live_xmm | undead_xmm));
        for(unsigned pos(0); pos < 16; ++pos) {
            const uint16_t mask(1U << pos);
            if(!(mask & zombies)) {
                undead_xmm |= mask;
                return static_cast<dynamorio::reg_id_t>(
                    pos + dynamorio::DR_REG_XMM0);
            }
        }
        return dynamorio::DR_REG_NULL;
    }


    /// Returns the next "free" dead register that is at the same scale as
    /// another register/operand.
    dynamorio::reg_id_t register_manager::get_zombie(dynamorio::reg_id_t scale) throw() {
        dynamorio::reg_id_t zombie(get_zombie());

        if(dynamorio::DR_REG_RAX <= scale && scale <= dynamorio::DR_REG_R15) {
            return zombie;
        }

        if(dynamorio::DR_REG_EAX <= scale && scale <= dynamorio::DR_REG_R15D) {
            return zombie + (dynamorio::DR_REG_EAX - 1);
        }

        if(dynamorio::DR_REG_AX <= scale && scale <= dynamorio::DR_REG_R15W) {
            return zombie + (dynamorio::DR_REG_AX - 1);
        }

        if(dynamorio::DR_REG_AL <= scale && scale <= dynamorio::DR_REG_R15L) {
            return zombie + (dynamorio::DR_REG_AL - 1);
        }

        return zombie;
    }
}

