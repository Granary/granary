/*
 * watchpoint.cc
 *
 *  Created on: 2013-04-08
 *      Author: akshayk
 */

#include "clients/watchpoint/watchpoint.h"


namespace client {

	void watchpoint_policy::watchpoint_memory_operations(
			granary::cpu_state_handle &cpu,
    		granary::thread_state_handle &thread,
    		granary::basic_block_state &bb,
    		granary::instruction_list &ls,
    		granary::instruction &in,
    		dynamorio::app_pc pc,
    		bool is_write){
		(void)cpu;
		(void)thread;
		(void)bb;
		(void)ls;
		(void)pc;

		dynamorio::opnd_t instr_opnd;
		client::memory_operand_modifier ops = client::memory_operand_modifier();

		if(is_write){
			ops.find_dsts_operand(in);
		} else {
			ops.find_src_operand(in);
		}

		switch(ops.found_operand.value.base_disp.base_reg) {
		case dynamorio::DR_REG_RSP: case dynamorio::DR_REG_ESP: case dynamorio::DR_REG_SP:
		case dynamorio::DR_REG_RBP: case dynamorio::DR_REG_EBP: case dynamorio::DR_REG_BP:
			return;
		default:
			break;
		}
		(void)instr_opnd;
/*
			 if(instr->opcode == OP_rep_stos || instr->opcode == OP_rep_movs){
			     instrument_write_rep_stos(drcontext, ilist, instr, pc, &ops, is_write);
			     return;
			 }

			 if(opnd_is_far_base_disp(ops.found_operand)){
			     if (instr->opcode == OP_dec ||
			             instr->opcode == OP_inc||
			             instr->opcode == OP_cmpxchg ||
			             instr->opcode == OP_btr ||
			             instr->opcode == OP_or ||
			             instr->opcode == OP_sub ||
			             instr->opcode == OP_add){
			         if(instr_writes_memory(instr)){
			             instrument_mem_instr(drcontext, ilist, instr, pc, true);
			         } else if(instr_reads_memory(instr)){
			             instrument_mem_instr(drcontext, ilist, instr, pc, false);
			         }
			         return;
			     }
			 }

			 if(instr->opcode == OP_xadd){
			     if(instr_writes_memory(instr)){
			         instrument_mem_instr_xadd(drcontext, ilist, instr, pc, true);
			     } else if(instr_reads_memory(instr)){
			         instrument_mem_instr_xadd(drcontext, ilist, instr, pc, false);
			     }
			 }

			 if (opnd_is_rel_addr(ops.found_operand) || opnd_is_abs_addr(ops.found_operand)) {

			 }else if (opnd_is_base_disp(ops.found_operand)) {
				 if(opnd_is_near_base_disp(ops.found_operand)){
					 if(is_write){
						 instrument_memory_write(drcontext, ilist, instr, pc, &ops);
					 } else {
						 instrument_memory_read(drcontext, ilist, instr, pc, &ops);
					 }
				 }else if(opnd_is_far_base_disp(ops.found_operand)) {

					 if(instr->opcode == OP_rep_stos || instr->opcode == OP_rep_movs){
						 instrument_write_rep_stos(drcontext, ilist, instr, pc, &ops, is_write);
					 } else if (instr->opcode == OP_dec ||
							 instr->opcode == OP_inc||
							 instr->opcode == OP_xadd ||
							 instr->opcode == OP_cmpxchg ||
							 instr->opcode == OP_btr){
						 if(instr_writes_memory(instr)){
							 instrument_mem_instr(drcontext, ilist, instr, pc, true);
						 } else if(instr_reads_memory(instr)){
							 instrument_mem_instr(drcontext, ilist, instr, pc, false);
						 }
					 }
				 }
			 }

			(void)tag;*/
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


