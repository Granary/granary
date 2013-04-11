/*
 * watchpoint.cc
 *
 *  Created on: 2013-04-08
 *      Author: akshayk
 */

#include "clients/watchpoint/watchpoint.h"

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

		ilist.insert_before(instr, granary::mov_imm_(dynamorio::opnd_create_reg(free_registers[0]),
								dynamorio::opnd_create_immed_int(WATCHPOINT_INDEX_MASK,sizeof(reg_t))));

	    ilist.insert_before(instr, granary::or_(dynamorio::opnd_create_reg(free_registers[0]),
	    								dynamorio::opnd_create_reg(free_registers[1])));
	    ilist.insert_before(instr, granary::cmp_(dynamorio::opnd_create_reg(free_registers[0]), dynamorio::opnd_create_reg(free_registers[1])));

	    ilist.insert_before(instr, granary::jcc_(dynamorio::OP_jecxz,
	    								dynamorio::opnd_create_instr(not_watchpoint)));

	    ilist.insert_before(instr, watchpoint);

	    emulated_instr = instruction(dynamorio::instr_clone(granary::instruction::DCONTEXT, (dynamorio::instr_t*)instr));

	    ops.replacement_operand = dynamorio::opnd_create_base_disp(
	            free_registers[0], dynamorio::DR_REG_NULL,1, 0 , ops.found_operand.size);

	    if(is_write){
	    	ops.replace_dsts_operand(emulated_instr);
	    } else {
	    	ops.replace_src_operand(emulated_instr);
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
	}

    void watchpoint_policy::instrumentation_far_base_disp(granary::instruction_list &ls,
    		granary::instruction &in,
    		dynamorio::app_pc pc,
    		client::instruction_util &ops,
    		bool is_write){
    	(void)in;
    	(void)pc;
    	(void)ops;
    	(void)is_write;
    	(void)ls;

    }

    void watchpoint_policy::instrumentation_operand_rep(granary::instruction_list &ls,
    		granary::instruction &in,
    		dynamorio::app_pc pc,
    		client::instruction_util &ops,
    		bool is_write){
    	(void)in;
    	(void)pc;
    	(void)ops;
    	(void)is_write;
    	(void)ls;
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

    //	client::memory_operand_modifier ops = memory_operand_modifier();// = {0, 0, 0, 0, 0, 0, 0, 0, 0};

    	for(unsigned i(0), max(ls.length()); i < max; i++, in = in.next()) {
    		if(in.is_valid()) {
    			continue;
    		}

    		if(dynamorio::instr_writes_memory(in.instr)){
    			watchpoint_memory_operations(cpu, thread, bb, ls, in, in.pc(), true);

    		} else if(dynamorio::instr_writes_memory(in.instr)){
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


