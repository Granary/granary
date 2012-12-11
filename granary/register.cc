/*
 * register.cc
 *
 *  Created on: 2012-11-30
 *      Author: pag
 *     Version: $Id$
 */


#include "register.h"
#include "granary/instruction.h"

namespace granary {


    /// Registers that are forced to always be alive.
    static uint32_t FORCE_LIVE(0U);


    STATIC_INITIALIZE({
        FORCE_LIVE |= (1U << dynamorio::DR_REG_NULL);
        FORCE_LIVE |= (1U << dynamorio::DR_REG_RSP);
        FORCE_LIVE |= (1U << dynamorio::DR_REG_RBP);
    })


    register_manager::register_manager(void) throw()
        : live(~0U)
        , undead(0U)
    { }


    /// Kill all registers.
    void register_manager::kill_all(void) throw() {
        live = FORCE_LIVE;
        undead = 0U;
    }


    /// Revive all registers.
    void register_manager::revive_all(void) throw() {
        live = ~0U;
        undead = 0U;
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


    /// Do opcode-specific killing/reviving
    void register_manager::visit(dynamorio::instr_t *in,
                                    unsigned num_dests) throw() {
        switch(in->opcode) {
        case dynamorio::OP_call:
        case dynamorio::OP_call_ind:
        case dynamorio::OP_call_far:
        case dynamorio::OP_call_far_ind:
            kill(dynamorio::DR_REG_R10);
            kill(dynamorio::DR_REG_R11); // TODO(pag): is this correct?
            break;

        case dynamorio::OP_cmpxchg:
        case dynamorio::OP_cmpxchg8b:
            revive(dynamorio::DR_REG_RAX);
            revive(dynamorio::DR_REG_RDX);
            revive(dynamorio::DR_REG_RCX);
            revive(dynamorio::DR_REG_RBX);
            break;

        case dynamorio::OP_cpuid:
            revive(dynamorio::DR_REG_RAX);
            kill(dynamorio::DR_REG_RBX);
            kill(dynamorio::DR_REG_RCX);
            break;

        case dynamorio::OP_cwde:
        case dynamorio::OP_cdq:
            revive(dynamorio::DR_REG_RAX);
            kill(dynamorio::DR_REG_RDX);
            break;

        case dynamorio::OP_div:
        case dynamorio::OP_idiv:
            revive(dynamorio::DR_REG_RAX);
            revive(dynamorio::DR_REG_RDX);
            break;

        case dynamorio::OP_ins:
            if(0U == num_dests) {
                revive(dynamorio::DR_REG_RDX);
                kill(dynamorio::DR_REG_RDI);
            }
            break;

        case dynamorio::OP_imul:
        case dynamorio::OP_mul:
            if(0U == num_dests) {
                revive(dynamorio::DR_REG_RAX);
                kill(dynamorio::DR_REG_RDX);
            }
            break;

        case dynamorio::OP_monitor:
            revive(dynamorio::DR_REG_RAX);
            // fall-through
        case dynamorio::OP_mwait:
            revive(dynamorio::DR_REG_RCX);
            break;

        case dynamorio::OP_out:
            revive(dynamorio::DR_REG_RAX);
            break;

        case dynamorio::OP_outs:
            if(0U == num_dests) {
                revive(dynamorio::DR_REG_RSI);
                kill(dynamorio::DR_REG_RDX);
            }
            break;

        case dynamorio::OP_pcmpestri:
            revive(dynamorio::DR_REG_RCX);
            // fall-through

        case dynamorio::OP_pcmpestrm:
            revive(dynamorio::DR_REG_RAX);
            revive(dynamorio::DR_REG_RDX);
            break;

        case dynamorio::OP_rdmsr:
        case dynamorio::OP_rdpmc:
        case dynamorio::OP_xgetbv:
            revive(dynamorio::DR_REG_RCX);
            // fall-through
        case dynamorio::OP_rdtsc:
            kill(dynamorio::DR_REG_RAX);
            kill(dynamorio::DR_REG_RDX);
            break;

        case dynamorio::OP_rdtscp:
            kill(dynamorio::DR_REG_RAX);
            kill(dynamorio::DR_REG_RCX);
            kill(dynamorio::DR_REG_RDX);
            break;

        case dynamorio::OP_rep_ins:
            revive(dynamorio::DR_REG_RCX);
            revive(dynamorio::DR_REG_RDX);
            kill(dynamorio::DR_REG_RDI);
            break;

        case dynamorio::OP_rep_movs:
            revive(dynamorio::DR_REG_RCX);
            revive(dynamorio::DR_REG_RSI);
            kill(dynamorio::DR_REG_RDI);
            break;

        case dynamorio::OP_rep_outs:
            revive(dynamorio::DR_REG_RCX);
            revive(dynamorio::DR_REG_RSI);
            kill(dynamorio::DR_REG_RDX);
            break;

        case dynamorio::OP_rep_lods:
            revive(dynamorio::DR_REG_RCX);
            revive(dynamorio::DR_REG_RSI);
            kill(dynamorio::DR_REG_RAX);
            break;

        case dynamorio::OP_rep_stos:
            revive(dynamorio::DR_REG_RCX);
            revive(dynamorio::DR_REG_RSI);
            revive(dynamorio::DR_REG_RAX);
            break;

        case dynamorio::OP_rep_cmps:
        case dynamorio::OP_repne_cmps:
            revive(dynamorio::DR_REG_RDI);
            revive(dynamorio::DR_REG_RSI);
            break;

        case dynamorio::OP_rep_scas:
        case dynamorio::OP_repne_scas:
            revive(dynamorio::DR_REG_RAX);
            revive(dynamorio::DR_REG_RDI);
            break;

        case dynamorio::OP_ret:
            revive(dynamorio::DR_REG_RAX); // typically used for return value
            break;

        case dynamorio::OP_wrmsr:
        case dynamorio::OP_xsetbv:
            revive(dynamorio::DR_REG_RCX);
            // fall-through
        case dynamorio::OP_xrstor:
        case dynamorio::OP_xsave:
        case dynamorio::OP_xsaveopt:
            revive(dynamorio::DR_REG_RAX);
            revive(dynamorio::DR_REG_RDX);
            break;

        default: break;
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

        // revive/kill implicitly used registers
        visit(in, num_dests);

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
            kill(op.value.base_disp.base_reg);
            kill(op.value.base_disp.index_reg);
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


    /// Forcibly kill a particular register.
    void register_manager::kill(dynamorio::reg_id_t reg) throw() {
        if(reg < dynamorio::DR_REG_SPL) {
            while(reg >= dynamorio::DR_REG_EAX) {
                reg -= (dynamorio::DR_REG_EAX - 1);
            }

            const uint32_t mask(1UL << reg);
            if(undead & mask) {
                undead &= ~mask;
            }

            live &= ~mask;
            live |= FORCE_LIVE;
        }
    }


    /// Forcibly revive a particular register.
    void register_manager::revive(dynamorio::reg_id_t reg) throw() {
        if(reg < dynamorio::DR_REG_SPL) {
            while(reg >= dynamorio::DR_REG_EAX) {
                reg -= (dynamorio::DR_REG_EAX - 1);
            }
            const uint32_t mask(1UL << reg);
            if(undead & mask) {
                undead &= ~mask;
            }
            live |= mask;
        }
    }


    /// Returns the next 64-bit "free" dead register.
    dynamorio::reg_id_t register_manager::get_zombie(void) throw() {
        uint64_t zombies((~live & ~undead));
        unsigned pos(0);
        for(; pos < 32; ++pos) {
            uint32_t mask = (1 << pos);
            if(!(mask & zombies)) {
                undead |= mask;
                return static_cast<dynamorio::reg_id_t>(pos);
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

