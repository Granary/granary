/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
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
    static const uint16_t FORCE_LIVE =
        (1U << (dynamorio::DR_REG_RSP - 1));


    /// We use a lookup table, primary because of the 8-bit registers,
    /// which don't map directly (using subtraction) to their 64-bit
    /// counterparts.
    static const dynamorio::reg_id_t REG_TO_REG64[] = {
        dynamorio::DR_REG_NULL,

        dynamorio::DR_REG_RAX,
        dynamorio::DR_REG_RCX,
        dynamorio::DR_REG_RDX,
        dynamorio::DR_REG_RBX,
        dynamorio::DR_REG_RSP,
        dynamorio::DR_REG_RBP,
        dynamorio::DR_REG_RSI,
        dynamorio::DR_REG_RDI,
        dynamorio::DR_REG_R8,
        dynamorio::DR_REG_R9,
        dynamorio::DR_REG_R10,
        dynamorio::DR_REG_R11,
        dynamorio::DR_REG_R12,
        dynamorio::DR_REG_R13,
        dynamorio::DR_REG_R14,
        dynamorio::DR_REG_R15,

        dynamorio::DR_REG_RAX,
        dynamorio::DR_REG_RCX,
        dynamorio::DR_REG_RDX,
        dynamorio::DR_REG_RBX,
        dynamorio::DR_REG_RSP,
        dynamorio::DR_REG_RBP,
        dynamorio::DR_REG_RSI,
        dynamorio::DR_REG_RDI,
        dynamorio::DR_REG_R8,
        dynamorio::DR_REG_R9,
        dynamorio::DR_REG_R10,
        dynamorio::DR_REG_R11,
        dynamorio::DR_REG_R12,
        dynamorio::DR_REG_R13,
        dynamorio::DR_REG_R14,
        dynamorio::DR_REG_R15,

        dynamorio::DR_REG_RAX,
        dynamorio::DR_REG_RCX,
        dynamorio::DR_REG_RDX,
        dynamorio::DR_REG_RBX,
        dynamorio::DR_REG_RSP,
        dynamorio::DR_REG_RBP,
        dynamorio::DR_REG_RSI,
        dynamorio::DR_REG_RDI,
        dynamorio::DR_REG_R8,
        dynamorio::DR_REG_R9,
        dynamorio::DR_REG_R10,
        dynamorio::DR_REG_R11,
        dynamorio::DR_REG_R12,
        dynamorio::DR_REG_R13,
        dynamorio::DR_REG_R14,
        dynamorio::DR_REG_R15,

        dynamorio::DR_REG_RAX,
        dynamorio::DR_REG_RCX,
        dynamorio::DR_REG_RDX,
        dynamorio::DR_REG_RBX,
        dynamorio::DR_REG_RAX, // DR_REG_AH
        dynamorio::DR_REG_RCX, // DR_REG_CH
        dynamorio::DR_REG_RDX, // DR_REG_DH
        dynamorio::DR_REG_RBX, // DR_REG_BH
        dynamorio::DR_REG_R8,
        dynamorio::DR_REG_R9,
        dynamorio::DR_REG_R10,
        dynamorio::DR_REG_R11,
        dynamorio::DR_REG_R12,
        dynamorio::DR_REG_R13,
        dynamorio::DR_REG_R14,
        dynamorio::DR_REG_R15,

        dynamorio::DR_REG_RSP,
        dynamorio::DR_REG_RBP,
        dynamorio::DR_REG_RSI,
        dynamorio::DR_REG_RDI
    };


    static const dynamorio::reg_id_t REG_TO_REG32[] = {
        dynamorio::DR_REG_NULL,

        dynamorio::DR_REG_EAX,
        dynamorio::DR_REG_ECX,
        dynamorio::DR_REG_EDX,
        dynamorio::DR_REG_EBX,
        dynamorio::DR_REG_ESP,
        dynamorio::DR_REG_EBP,
        dynamorio::DR_REG_ESI,
        dynamorio::DR_REG_EDI,
        dynamorio::DR_REG_R8D,
        dynamorio::DR_REG_R9D,
        dynamorio::DR_REG_R10D,
        dynamorio::DR_REG_R11D,
        dynamorio::DR_REG_R12D,
        dynamorio::DR_REG_R13D,
        dynamorio::DR_REG_R14D,
        dynamorio::DR_REG_R15D,

        dynamorio::DR_REG_EAX,
        dynamorio::DR_REG_ECX,
        dynamorio::DR_REG_EDX,
        dynamorio::DR_REG_EBX,
        dynamorio::DR_REG_ESP,
        dynamorio::DR_REG_EBP,
        dynamorio::DR_REG_ESI,
        dynamorio::DR_REG_EDI,
        dynamorio::DR_REG_R8D,
        dynamorio::DR_REG_R9D,
        dynamorio::DR_REG_R10D,
        dynamorio::DR_REG_R11D,
        dynamorio::DR_REG_R12D,
        dynamorio::DR_REG_R13D,
        dynamorio::DR_REG_R14D,
        dynamorio::DR_REG_R15D,

        dynamorio::DR_REG_EAX,
        dynamorio::DR_REG_ECX,
        dynamorio::DR_REG_EDX,
        dynamorio::DR_REG_EBX,
        dynamorio::DR_REG_ESP,
        dynamorio::DR_REG_EBP,
        dynamorio::DR_REG_ESI,
        dynamorio::DR_REG_EDI,
        dynamorio::DR_REG_R8D,
        dynamorio::DR_REG_R9D,
        dynamorio::DR_REG_R10D,
        dynamorio::DR_REG_R11D,
        dynamorio::DR_REG_R12D,
        dynamorio::DR_REG_R13D,
        dynamorio::DR_REG_R14D,
        dynamorio::DR_REG_R15D,

        dynamorio::DR_REG_EAX,
        dynamorio::DR_REG_ECX,
        dynamorio::DR_REG_EDX,
        dynamorio::DR_REG_EBX,
        dynamorio::DR_REG_EAX, // DR_REG_AH
        dynamorio::DR_REG_ECX, // DR_REG_CH
        dynamorio::DR_REG_EDX, // DR_REG_DH
        dynamorio::DR_REG_EBX, // DR_REG_BH
        dynamorio::DR_REG_R8D,
        dynamorio::DR_REG_R9D,
        dynamorio::DR_REG_R10D,
        dynamorio::DR_REG_R11D,
        dynamorio::DR_REG_R12D,
        dynamorio::DR_REG_R13D,
        dynamorio::DR_REG_R14D,
        dynamorio::DR_REG_R15D,

        dynamorio::DR_REG_ESP,
        dynamorio::DR_REG_EBP,
        dynamorio::DR_REG_ESI,
        dynamorio::DR_REG_EDI
    };


    static const dynamorio::reg_id_t REG_TO_REG16[] = {
        dynamorio::DR_REG_NULL,
        dynamorio::DR_REG_AX,
        dynamorio::DR_REG_CX,
        dynamorio::DR_REG_DX,
        dynamorio::DR_REG_BX,
        dynamorio::DR_REG_SP,
        dynamorio::DR_REG_BP,
        dynamorio::DR_REG_SI,
        dynamorio::DR_REG_DI,
        dynamorio::DR_REG_R8W,
        dynamorio::DR_REG_R9W,
        dynamorio::DR_REG_R10W,
        dynamorio::DR_REG_R11W,
        dynamorio::DR_REG_R12W,
        dynamorio::DR_REG_R13W,
        dynamorio::DR_REG_R14W,
        dynamorio::DR_REG_R15W,

        dynamorio::DR_REG_AX,
        dynamorio::DR_REG_CX,
        dynamorio::DR_REG_DX,
        dynamorio::DR_REG_BX,
        dynamorio::DR_REG_SP,
        dynamorio::DR_REG_BP,
        dynamorio::DR_REG_SI,
        dynamorio::DR_REG_DI,
        dynamorio::DR_REG_R8W,
        dynamorio::DR_REG_R9W,
        dynamorio::DR_REG_R10W,
        dynamorio::DR_REG_R11W,
        dynamorio::DR_REG_R12W,
        dynamorio::DR_REG_R13W,
        dynamorio::DR_REG_R14W,
        dynamorio::DR_REG_R15W,

        dynamorio::DR_REG_AX,
        dynamorio::DR_REG_CX,
        dynamorio::DR_REG_DX,
        dynamorio::DR_REG_BX,
        dynamorio::DR_REG_SP,
        dynamorio::DR_REG_BP,
        dynamorio::DR_REG_SI,
        dynamorio::DR_REG_DI,
        dynamorio::DR_REG_R8W,
        dynamorio::DR_REG_R9W,
        dynamorio::DR_REG_R10W,
        dynamorio::DR_REG_R11W,
        dynamorio::DR_REG_R12W,
        dynamorio::DR_REG_R13W,
        dynamorio::DR_REG_R14W,
        dynamorio::DR_REG_R15W,

        dynamorio::DR_REG_AX,
        dynamorio::DR_REG_CX,
        dynamorio::DR_REG_DX,
        dynamorio::DR_REG_BX,
        dynamorio::DR_REG_AX, // DR_REG_AH
        dynamorio::DR_REG_CX, // DR_REG_CH
        dynamorio::DR_REG_DX, // DR_REG_DH
        dynamorio::DR_REG_BX, // DR_REG_BH
        dynamorio::DR_REG_R8W,
        dynamorio::DR_REG_R9W,
        dynamorio::DR_REG_R10W,
        dynamorio::DR_REG_R11W,
        dynamorio::DR_REG_R12W,
        dynamorio::DR_REG_R13W,
        dynamorio::DR_REG_R14W,
        dynamorio::DR_REG_R15W,

        dynamorio::DR_REG_SP,
        dynamorio::DR_REG_BP,
        dynamorio::DR_REG_SI,
        dynamorio::DR_REG_DI
    };


    static const dynamorio::reg_id_t REG_TO_REG8[] = {
        dynamorio::DR_REG_NULL,
        dynamorio::DR_REG_AL,
        dynamorio::DR_REG_CL,
        dynamorio::DR_REG_DL,
        dynamorio::DR_REG_BL,
        dynamorio::DR_REG_SPL,
        dynamorio::DR_REG_BPL,
        dynamorio::DR_REG_SIL,
        dynamorio::DR_REG_DIL,
        dynamorio::DR_REG_R8L,
        dynamorio::DR_REG_R9L,
        dynamorio::DR_REG_R10L,
        dynamorio::DR_REG_R11L,
        dynamorio::DR_REG_R12L,
        dynamorio::DR_REG_R13L,
        dynamorio::DR_REG_R14L,
        dynamorio::DR_REG_R15L,

        dynamorio::DR_REG_AL,
        dynamorio::DR_REG_CL,
        dynamorio::DR_REG_DL,
        dynamorio::DR_REG_BL,
        dynamorio::DR_REG_SPL,
        dynamorio::DR_REG_BPL,
        dynamorio::DR_REG_SIL,
        dynamorio::DR_REG_DIL,
        dynamorio::DR_REG_R8L,
        dynamorio::DR_REG_R9L,
        dynamorio::DR_REG_R10L,
        dynamorio::DR_REG_R11L,
        dynamorio::DR_REG_R12L,
        dynamorio::DR_REG_R13L,
        dynamorio::DR_REG_R14L,
        dynamorio::DR_REG_R15L,

        dynamorio::DR_REG_AL,
        dynamorio::DR_REG_CL,
        dynamorio::DR_REG_DL,
        dynamorio::DR_REG_BL,
        dynamorio::DR_REG_SPL,
        dynamorio::DR_REG_BPL,
        dynamorio::DR_REG_SIL,
        dynamorio::DR_REG_DIL,
        dynamorio::DR_REG_R8L,
        dynamorio::DR_REG_R9L,
        dynamorio::DR_REG_R10L,
        dynamorio::DR_REG_R11L,
        dynamorio::DR_REG_R12L,
        dynamorio::DR_REG_R13L,
        dynamorio::DR_REG_R14L,
        dynamorio::DR_REG_R15L,

        dynamorio::DR_REG_AL,
        dynamorio::DR_REG_CL,
        dynamorio::DR_REG_DL,
        dynamorio::DR_REG_BL,

        // annoying inconsistency; likely due to original design being made
        // for x86 and not x86-64.
        dynamorio::DR_REG_AH, // DR_REG_AH
        dynamorio::DR_REG_CH, // DR_REG_CH
        dynamorio::DR_REG_DH, // DR_REG_DH
        dynamorio::DR_REG_BH, // DR_REG_BH
        dynamorio::DR_REG_R8L,
        dynamorio::DR_REG_R9L,
        dynamorio::DR_REG_R10L,
        dynamorio::DR_REG_R11L,
        dynamorio::DR_REG_R12L,
        dynamorio::DR_REG_R13L,
        dynamorio::DR_REG_R14L,
        dynamorio::DR_REG_R15L,

        dynamorio::DR_REG_SPL,
        dynamorio::DR_REG_BPL,
        dynamorio::DR_REG_SIL,
        dynamorio::DR_REG_DIL
    };


    /// Initialise the register manager so that every register is live.
    register_manager::register_manager(void) throw()
        : live(~0)
        , undead(0)
        , live_xmm(~0)
        , undead_xmm(0)
    { }


    /// Scale a register.
    dynamorio::reg_id_t register_manager::scale(
        dynamorio::reg_id_t reg,
        register_scale scale
    ) throw() {
        if(dynamorio::DR_REG_MM0 <= reg) {
            return dynamorio::DR_REG_NULL;
        }

        switch(scale) {
        case REG_8:     return REG_TO_REG8[reg];
        case REG_16:    return REG_TO_REG16[reg];
        case REG_32:    return REG_TO_REG32[reg];
        case REG_64:    return REG_TO_REG64[reg];
        default:        return dynamorio::DR_REG_NULL;
        }
    }


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


    /// Revive all xmm registers.
    void register_manager::revive_all_xmm(void) throw() {
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
    void register_manager::kill(
        dynamorio::instr_t *in,
        opnd_counter *num_ops,
        opnd_getter *which_op
    ) throw() {
        for(int i(0); i < num_ops(in); i++) {
            kill(which_op(in, i));
        }
    }


    /// Revive registers used in some class op operands within an instruction.
    void register_manager::revive(
        dynamorio::instr_t *in,
        opnd_counter *num_ops,
        opnd_getter *which_op
    ) throw() {
        for(int i(0); i < num_ops(in); i++) {
            revive(which_op(in, i));
        }
    }


    /// Visit the destination operands of an instruction. This will kill
    /// register destinations and revive registers that are used in base/
    /// disp operands.
    void register_manager::visit_dests(dynamorio::instr_t *in) throw() {
        unsigned num_dests(dynamorio::instr_num_dsts(in));
        for(unsigned i(0); i < num_dests; i++) {
            dynamorio::opnd_t opnd(dynamorio::instr_get_dst(in, i));
            if(dynamorio::REG_kind == opnd.kind) {
                kill(opnd);
            } else {
                revive(opnd);
            }
        }
    }


    /// Visit the source operands of an instruction. This will revive
    /// register sources and revive registers that are used in base/
    /// disp operands.
    void register_manager::visit_sources(dynamorio::instr_t *in) throw() {
        revive(in, dynamorio::instr_num_srcs, dynamorio::instr_get_src);
    }


    /// Visit the registers in the instruction; kill the destination
    /// registers and revive the source registers.
    void register_manager::visit(dynamorio::instr_t *in) throw() {

        // Conservative.
        if(dynamorio::instr_is_cti(in)) {
            revive_all();
            return;
        }

        visit_dests(in);
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
            kill_64(op.value.base_disp.base_reg);
            kill_64(op.value.base_disp.index_reg);
        } else if(dynamorio::REG_kind == op.kind) {
            kill(op.value.reg);
        }
    }


    /// Forcibly revive all registers used in a particular operand.
    void register_manager::revive(dynamorio::opnd_t op) throw() {
        if(dynamorio::BASE_DISP_kind == op.kind) {
            revive_64(op.value.base_disp.base_reg);
            revive_64(op.value.base_disp.index_reg);
        } else if(dynamorio::REG_kind == op.kind) {
            revive(op.value.reg);
        }
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
        if(dynamorio::DR_REG_MM0 <= reg) {
            kill_xmm(reg);
        } else {
            kill_64(reg);
        }
    }


    /// Forcible revive a particular register.
    void register_manager::revive(dynamorio::reg_id_t reg) throw() {
        if(dynamorio::DR_REG_MM0 <= reg) {
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
    /// are actually using the full 64-bit register. This is because something
    /// like `MOV AL, CL`, while looking like it kills `RCX`, is only killing
    /// the low 8 bytes of `RCX`, and the other bytes might still be live.
    void register_manager::kill_64(dynamorio::reg_id_t reg) throw() {
        const uint8_t reg64(REG_TO_REG64[reg]);
        if(reg64 && (reg64 == reg || REG_TO_REG32[reg] == reg)) {
            const uint16_t mask(1U << (reg64 - 1));
            undead &= ~mask;
            live &= ~mask;
            live |= FORCE_LIVE;
        }
    }


    /// Forcibly revive a particular 64-bit register.
    void register_manager::revive_64(dynamorio::reg_id_t reg) throw() {
        const uint8_t reg64(REG_TO_REG64[reg]);
        if(reg64) {
            const uint16_t mask(1U << (reg64 - 1));
            undead &= ~mask;
            live |= mask;
        }
    }


    /// Returns the next 64-bit "free" dead register. Note: < reg15 instead of
    /// <= reg15 because reg_null=0 and it is not represented in the bitset.
    dynamorio::reg_id_t register_manager::get_zombie(void) throw() {
        const uint64_t zombies(live | undead);
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
        const uint64_t zombies(live_xmm | undead_xmm);
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


    /// Returns true iff a particular register is alive.
    bool register_manager::is_live(dynamorio::reg_id_t reg_) const throw() {
        uint8_t reg;
        if(dynamorio::DR_REG_XMM0 <= reg_
        && reg_ <= dynamorio::DR_REG_XMM15) {
            reg = reg_to_xmm(reg_);
            if(!reg) {
                return false;
            }

            const uint16_t mask(1U << (reg - 1));
            return 0 != (mask & live_xmm);
        } else if(reg_ < dynamorio::DR_REG_ST0) {
            reg = REG_TO_REG64[reg_];
            if(!reg) {
                return false;
            }

            const uint16_t mask(1U << (reg - 1));
            return 0 != (mask & live);
        }

        FAULT;
        return false;
    }


    /// Returns true iff a particular register is dead.
    bool register_manager::is_dead(dynamorio::reg_id_t reg_) const throw() {
        uint8_t reg;
        if(dynamorio::DR_REG_XMM0 <= reg_
        && reg_ <= dynamorio::DR_REG_XMM15) {
            reg = reg_to_xmm(reg_);
            if(!reg) {
                return false;
            }

            const uint16_t mask(1U << (reg - 1));
            return 0 != (mask & ~live_xmm);
        } else if(reg_ < dynamorio::DR_REG_ST0) {
            reg = REG_TO_REG64[reg_];
            if(!reg) {
                return false;
            }

            const uint16_t mask(1U << (reg - 1));
            return 0 != (mask & ~live);
        }

        FAULT;
        return false;
    }


    /// Returns true iff a particular register is a walker, i.e.
    /// living or a zombie!
    bool register_manager::is_undead(dynamorio::reg_id_t reg_) const throw() {
        uint8_t reg;
        if(dynamorio::DR_REG_XMM0 <= reg_
        && reg_ <= dynamorio::DR_REG_XMM15) {
            reg = reg_to_xmm(reg_);
            if(!reg) {
                return false;
            }

            const uint16_t mask(1U << (reg - 1));
            return 0 != (mask & (live_xmm | undead));
        } else if(reg_ < dynamorio::DR_REG_ST0) {
            reg = REG_TO_REG64[reg_];
            if(!reg) {
                return false;
            }

            const uint16_t mask(1U << (reg - 1));
            return 0 != (mask & (live | undead));
        }

        FAULT;
        return false;
    }
}

