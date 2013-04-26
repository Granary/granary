/*
 * watchpoint.cc
 *
 *  Created on: 2013-04-08
 *      Author: akshayk
 */

#include "clients/watchpoint-old/watchpoint.h"

using namespace dynamorio;
using namespace granary;

namespace client {

    void watchpoint_policy::instrumentation_base_disp(
            granary::instruction_list &ilist,
            granary::instruction &instr,
            dynamorio::app_pc pc,
            client::instruction_util &ops,
            bool is_write){

        dynamorio::reg_id_t free_registers[2];
        granary::instruction cursor;

        ops.instruction_collect_regs(instr);
        //(void)is_write;
        granary::instruction emulated_instr;
        granary::instruction label_begin(granary::label_());
        granary::instruction watchpoint(granary::label_());
        granary::instruction not_watchpoint(granary::label_());
        granary::instruction done_instrumenting(granary::label_());
        ilist.insert_before(instr, label_begin);

        free_registers[0] = ops.instruction_get_next_free_reg();
        ilist.insert_before(instr, granary::push_(dynamorio::opnd_create_reg(free_registers[0])));

        free_registers[1] = ops.instruction_get_next_free_reg();
        ilist.insert_before(instr, granary::push_(dynamorio::opnd_create_reg(free_registers[1])));
        ilist.insert_before(instr, granary::pushf_());

        ilist.insert_before(instr, granary::lea_(dynamorio::opnd_create_reg(free_registers[0]),
                dynamorio::opnd_create_base_disp(dynamorio::opnd_get_base(ops.found_operand),
                        dynamorio::opnd_get_index(ops.found_operand), dynamorio::opnd_get_scale(ops.found_operand),
                        dynamorio::opnd_get_disp(ops.found_operand), dynamorio::OPSZ_lea)));

        ilist.insert_before(instr, granary::mov_imm_(dynamorio::opnd_create_reg(free_registers[1]),
                dynamorio::opnd_create_immed_int(WATCHPOINT_INDEX_MASK,dynamorio::OPSZ_8)));

        ilist.insert_before(instr, granary::or_(dynamorio::opnd_create_reg(free_registers[0]),
                dynamorio::opnd_create_reg(free_registers[1])));
        ilist.insert_before(instr, granary::cmp_(dynamorio::opnd_create_reg(free_registers[0]), dynamorio::opnd_create_reg(free_registers[1])));

        ilist.insert_before(instr, granary::jcc_(dynamorio::OP_je,
                dynamorio::opnd_create_instr(not_watchpoint)));

        ilist.insert_before(instr, watchpoint);

        emulated_instr = instruction(dynamorio::instr_clone(granary::instruction::DCONTEXT, (dynamorio::instr_t*)instr));

        ops.replacement_operand = dynamorio::opnd_create_base_disp(
                free_registers[0], dynamorio::DR_REG_NULL,1, 0 , ops.found_operand.size);

        if(is_write){
            ops.replace_operand(emulated_instr);
        } else {
            ops.replace_operand(emulated_instr);
        }

        ilist.insert_before(instr, granary::popf_());

        ilist.insert_before(instr, emulated_instr);

        ilist.insert_before(instr, granary::pop_(dynamorio::opnd_create_reg(free_registers[1])));
        ilist.insert_before(instr, granary::pop_(dynamorio::opnd_create_reg(free_registers[0])));
        ilist.insert_before(instr, granary::jmp_short_(dynamorio::opnd_create_instr(done_instrumenting)));

        ilist.insert_before(instr, not_watchpoint);
        cursor = instr;
        cursor = ilist.insert_after(cursor, granary::popf_());
        cursor = ilist.insert_after(cursor, granary::pop_(dynamorio::opnd_create_reg(free_registers[1])));
        cursor = ilist.insert_after(cursor, granary::pop_(dynamorio::opnd_create_reg(free_registers[0])));
        cursor = ilist.insert_after(cursor, done_instrumenting);

        (void)pc;
        (void)cursor;
    }

    void watchpoint_policy::instrumentation_far_base_disp(
            granary::instruction_list &ilist,
            granary::instruction &instr,
            dynamorio::app_pc pc,
            client::instruction_util &ops,
            bool is_write){

        dynamorio::reg_id_t reg_mask, reg_instr;
        granary::instruction cursor;
        granary::instruction emulated_instr, begin_instrumenting;
        dynamorio::opnd_t opnd_reg_mask, opnd_reg_instr, opnd_instr;

        ops.instruction_collect_regs(instr);
        //ops.instruction_collect_reg(dynamorio::DR_REG_RCX);

        reg_mask = ops.instruction_get_next_free_reg();
        opnd_reg_mask = dynamorio::opnd_create_reg(reg_mask);

        if(is_write == true){
            opnd_instr = dynamorio::instr_get_dst(instr, 0);
        } else {
            opnd_instr = dynamorio::instr_get_src(instr, 0);
        }

        reg_instr = dynamorio::opnd_get_base(opnd_instr);
        opnd_reg_instr = dynamorio::opnd_create_reg(reg_instr);

        begin_instrumenting = granary::label_();

        ilist.insert_before(instr, begin_instrumenting);
        ilist.insert_before(instr, granary::push_(opnd_reg_instr));
        ilist.insert_before(instr, granary::push_(opnd_reg_mask));
        ilist.insert_before(instr, granary::pushf_());
        ilist.insert_before(instr, granary::mov_imm_(opnd_reg_mask,
                dynamorio::opnd_create_immed_int(WATCHPOINT_INDEX_MASK, dynamorio::OPSZ_8)));

        ilist.insert_before(instr, granary::or_(opnd_reg_instr, opnd_reg_mask));

        //emulated_instr = instruction(instr_clone(granary::instruction::DCONTEXT, instr));
        ilist.insert_before(instr, granary::popf_());

        cursor = ilist.insert_after(instr, granary::pop_(opnd_reg_mask));
        cursor = ilist.insert_after(cursor, granary::pop_(opnd_reg_instr));

        (void)pc;
        (void)emulated_instr;

    }

    void watchpoint_policy::instrumentation_operand_rep(
            granary::instruction_list &ilist,
            granary::instruction &instr,
            dynamorio::app_pc pc,
            client::instruction_util &ops,
            bool is_write){

        dynamorio::reg_id_t reg_mask, reg_instr;
        granary::instruction cursor;
        granary::instruction emulated_instr, begin_instrumenting;
        dynamorio::opnd_t opnd_reg_mask, opnd_reg_instr, opnd_instr;

        ops.instruction_collect_regs(instr);
        ops.instruction_collect_reg(dynamorio::DR_REG_RCX);

        reg_mask = ops.instruction_get_next_free_reg();
        opnd_reg_mask = dynamorio::opnd_create_reg(reg_mask);

        if(is_write == true){
            opnd_instr = dynamorio::instr_get_dst(instr, 0);
        } else {
            opnd_instr = dynamorio::instr_get_src(instr, 0);
        }

        reg_instr = dynamorio::opnd_get_base(opnd_instr);
        opnd_reg_instr = dynamorio::opnd_create_reg(reg_instr);

        begin_instrumenting = granary::label_();

        ilist.insert_before(instr, begin_instrumenting);
        ilist.insert_before(instr, granary::push_(opnd_reg_instr));
        ilist.insert_before(instr, granary::push_(opnd_reg_mask));
        ilist.insert_before(instr, granary::pushf_());
        ilist.insert_before(instr, granary::mov_imm_(opnd_reg_mask,
                dynamorio::opnd_create_immed_int(WATCHPOINT_INDEX_MASK, dynamorio::OPSZ_8)));

        ilist.insert_before(instr, granary::or_(opnd_reg_instr, opnd_reg_mask));

        //emulated_instr = instruction(instr_clone(granary::instruction::DCONTEXT, instr));
        ilist.insert_before(instr, granary::popf_());

        cursor = ilist.insert_after(instr, granary::pop_(opnd_reg_mask));
        cursor = ilist.insert_after(cursor, granary::pop_(opnd_reg_instr));

        (void)pc;
        (void)emulated_instr;
    }

    void watchpoint_policy::watchpoint_memory_operations(
            granary::cpu_state_handle &cpu,
            granary::thread_state_handle &thread,
            granary::basic_block_state &bb,
            granary::instruction_list &ilist,
            granary::instruction &instr,
            dynamorio::app_pc pc,
            bool is_write){
        (void)cpu;
        (void)thread;
        (void)bb;
        (void)ilist;
        (void)pc;

        dynamorio::opnd_t instr_opnd;
        client::instruction_util ops = client::instruction_util();

        if(is_write){
            ops.find_dsts_operand(instr);
        } else {
            ops.find_src_operand(instr);
        }

        switch(ops.found_operand.value.base_disp.base_reg) {
        case dynamorio::DR_REG_RSP: case dynamorio::DR_REG_ESP: case dynamorio::DR_REG_SP:
        case dynamorio::DR_REG_RBP: case dynamorio::DR_REG_EBP: case dynamorio::DR_REG_BP:
            return;
        default:
            break;
        }

        if(dynamorio::opnd_is_rel_addr(ops.found_operand)
        || dynamorio::opnd_is_abs_addr(ops.found_operand)) {
            /* TODO : Handle this case*/
            return;
        } else if(dynamorio::opnd_is_near_base_disp(ops.found_operand)) {
            instrumentation_base_disp(ilist, instr, pc, ops, is_write);
        } else if(dynamorio::opnd_is_far_base_disp(ops.found_operand)) {
            if((dynamorio::instr_get_opcode((dynamorio::instr_t*)instr) == dynamorio::OP_rep_stos)
                    || (dynamorio::instr_get_opcode((dynamorio::instr_t*)instr) == dynamorio::OP_rep_movs)){
                instrumentation_operand_rep(ilist, instr, pc, ops, is_write);
                return;
            } else {
                instrumentation_far_base_disp(ilist, instr, pc, ops, is_write);
                return;
            }
        }

        (void)instr_opnd;
    }

    /// Instruction a basic block.
    granary::instrumentation_policy watchpoint_policy::visit_basic_block(
            granary::cpu_state_handle &cpu,
            granary::thread_state_handle &thread,
            granary::basic_block_state &bb,
            granary::instruction_list &ls
    ) throw() {

        granary::instruction in(ls.first());
        dynamorio::instr_t *instr = (dynamorio::instr_t*)in;
        dynamorio::instr_t *next_instr;

        for(unsigned i(0); instr != NULL; i++, instr = next_instr) {
            next_instr = (dynamorio::instr_t*)(instr->next);
            in = instruction(instr);
            if(!in.is_valid()) {
                continue;
            }

            check_pc(in.pc());

            if(dynamorio::instr_writes_memory((dynamorio::instr_t*)in)){
                watchpoint_memory_operations(cpu, thread, bb, ls, in, in.pc(), true);

            } else if(dynamorio::instr_reads_memory((dynamorio::instr_t*)instr)){
                watchpoint_memory_operations(cpu, thread, bb, ls, in, in.pc(), false);
            }


        }

        //granary::printf("pc = %p\n", ls.first()->pc());

        (void) cpu;
        (void) thread;
        (void) bb;
        (void) ls;

        return granary::policy_for<watchpoint_policy>();
    }

}


