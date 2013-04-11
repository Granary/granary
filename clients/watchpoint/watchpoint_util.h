/*
 * watchpoint_util.h
 *
 *  Created on: 2013-04-08
 *      Author: akshayk
 */

#ifndef WATCHPOINT_UTIL_H_
#define WATCHPOINT_UTIL_H_

#include "granary/types/dynamorio.h"

using namespace dynamorio;

namespace client {

typedef unsigned int uint;

typedef struct instruction_util {
private:
	inline dynamorio::reg_id_t reg_to_reg64(dynamorio::reg_id_t reg) {
		if(reg < dynamorio::DR_REG_SPL) {
			while(reg >= dynamorio::DR_REG_EAX) {
				reg -= (dynamorio::DR_REG_EAX - 1);
			}
			return reg;
		}
		return dynamorio::DR_REG_NULL;
	}

	inline void collect_reg(dynamorio::reg_id_t reg) {
		// map the registers onto the 64-bit registers
		if(reg < dynamorio::DR_REG_SPL) {
			while(reg >= dynamorio::DR_REG_EAX) {
				reg -= (dynamorio::DR_REG_EAX - 1);
			}
			used_registers |= (1 << reg);
		}
	}

	inline void collect_regs(dynamorio::instr_t *instr,
			int (*num_ops)(dynamorio::instr_t *), dynamorio::opnd_t (*get_op)(dynamorio::instr_t *, uint) ) {
		int i;
		dynamorio::opnd_t opnd;

		used_registers |= (1 << dynamorio::DR_REG_NULL);

		for(i=0; i < num_ops(instr); i++) {
			opnd = get_op(instr, i);
			if(opnd.kind == dynamorio::REG_kind) {
				collect_reg(dynamorio::opnd_get_reg(opnd));
			} else if(opnd.kind == dynamorio::BASE_DISP_kind) {
				collect_reg(dynamorio::opnd_get_base(opnd));
				collect_reg(dynamorio::opnd_get_index(opnd));
			}
		}

		collect_reg(dynamorio::DR_REG_RSP);
		collect_reg(dynamorio::DR_REG_RBP);
		collect_reg(dynamorio::DR_REG_RAX);
		collect_reg(dynamorio::DR_REG_RDX);
	}

	inline void memory_dsts_operand_finder(dynamorio::opnd_t opnd) {
		if(dynamorio::BASE_DISP_kind == opnd.kind){
			if(opnd.value.base_disp.base_reg && !reg_to_reg64(opnd.value.base_disp.base_reg)) {
				return;
			}
			if(opnd.value.base_disp.index_reg && !reg_to_reg64(opnd.value.base_disp.index_reg)) {
				return;
			}
			has_dest_memory_operand = true;
			has_memory_operand = true;
			found_operand = opnd;
		}
		if(!opnd.seg.segment) {
			has_dsts_seg = 1;
		}

		dsts_size = opnd.size;
	}

	inline void memory_src_operand_finder(dynamorio::opnd_t opnd) {
		if(dynamorio::BASE_DISP_kind == opnd.kind){
			if(opnd.value.base_disp.base_reg && !reg_to_reg64(opnd.value.base_disp.base_reg)) {
				return;
			}
			if(opnd.value.base_disp.index_reg && !reg_to_reg64(opnd.value.base_disp.index_reg)) {
				return;
			}
			has_source_memory_operand = true;
			has_memory_operand = true;
			found_operand = opnd;
		}
		if(!opnd.seg.segment) {
			has_src_seg = 1;
		}

		src_size = opnd.size;
	}

	inline void
	memory_operand_replacer(dynamorio::opnd_t *opnd) {
	    if(BASE_DISP_kind == opnd->kind) {
	        const int orig_size = dynamorio::opnd_get_size(*opnd);
	        *opnd = replacement_operand;
	        dynamorio::opnd_set_size(opnd, orig_size);
	    }
	}
public:
	instruction_util():
		has_memory_operand(false),
		has_source_memory_operand(false),
		has_dest_memory_operand(false),
		has_src_seg(false),
		has_dsts_seg(false),
		src_size(0),
		dsts_size(0),
		used_registers(0){

	}

	bool has_memory_operand;
	bool has_source_memory_operand;
	bool has_dest_memory_operand;
	bool has_src_seg;
	bool has_dsts_seg;
	uint32_t src_size;
	uint32_t dsts_size;
	dynamorio::opnd_t found_operand;
	dynamorio::opnd_t replacement_operand;

	unsigned long used_registers;



	inline void instruction_collect_regs( granary::instruction &in) {
		collect_regs(in.instr, dynamorio::instr_num_srcs, dynamorio::instr_get_src );
		collect_regs(in.instr, dynamorio::instr_num_dsts, dynamorio::instr_get_dst );
	}

	inline void instruction_collect_reg( dynamorio::reg_id_t reg) {
		collect_reg(reg);
	}

	inline dynamorio::reg_id_t instruction_get_next_free_reg(void) {
		unsigned pos = 0;
		for(; pos < 32; ++pos) {
			unsigned long mask = (1 << pos);
			if(!(mask & used_registers)) {
				used_registers |= mask;
				return (dynamorio::reg_id_t) pos;
			}
		}
		return dynamorio::DR_REG_NULL;
	}

	inline void find_dsts_operand(granary::instruction &in) {
		unsigned int i = 0, max = in.num_destinations();
		dynamorio::instr_t *instr = (dynamorio::instr_t*)in;
		for(; i < max; ++i) {
			memory_dsts_operand_finder(dynamorio::instr_get_dst(instr, i));
		}
	}

	inline void replace_dsts_operand(granary::instruction &instr){
		(void)instr;
	}

	inline void find_src_operand(granary::instruction &in) {
		dynamorio::instr_t *instr;
		instr = (dynamorio::instr_t*)in;
		unsigned int i = 0, max = in.num_sources();
		for(i = 0; i < max; ++i) {
				memory_src_operand_finder(dynamorio::instr_get_src(instr, i));
		}
	}

	inline void replace_src_operand(granary::instruction &instr){
		(void)instr;
	}

}instruction_util;
}


#endif /* WATCHPOINT_UTIL_H_ */
