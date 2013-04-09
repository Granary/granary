/*
 * watchpoint_util.h
 *
 *  Created on: 2013-04-08
 *      Author: akshayk
 */

#ifndef WATCHPOINT_UTIL_H_
#define WATCHPOINT_UTIL_H_

namespace client {

	typedef struct memory_operand_modifier {
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

		inline void memory_dsts_operand_finder(dynamorio::opnd_t *opnd) {
		    if(dynamorio::BASE_DISP_kind == opnd->kind){
		        if(opnd->value.base_disp.base_reg && !reg_to_reg64(opnd->value.base_disp.base_reg)) {
		            return;
		        }
		        if(opnd->value.base_disp.index_reg && !reg_to_reg64(opnd->value.base_disp.index_reg)) {
		            return;
		        }
		        has_dest_memory_operand = true;
		        has_memory_operand = true;
		        found_operand = *opnd;
		    }
		    if(!opnd->seg.segment) {
		        has_dsts_seg = 1;
		    }

		    dsts_size = opnd->size;
		}

		inline void memory_src_operand_finder(dynamorio::opnd_t *opnd) {
		    if(dynamorio::BASE_DISP_kind == opnd->kind){
		        if(opnd->value.base_disp.base_reg && !reg_to_reg64(opnd->value.base_disp.base_reg)) {
		            return;
		        }
		        if(opnd->value.base_disp.index_reg && !reg_to_reg64(opnd->value.base_disp.index_reg)) {
		            return;
		        }
		        has_source_memory_operand = true;
		        has_memory_operand = true;
		        found_operand = *opnd;
		    }
		    if(!opnd->seg.segment) {
		        has_src_seg = 1;
		    }

		    src_size = opnd->size;
		}

	public:
		memory_operand_modifier():
			has_memory_operand(false),
			has_source_memory_operand(false),
			has_dest_memory_operand(false),
			has_src_seg(false),
			has_dsts_seg(false),
			src_size(0),
			dsts_size(0){

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

		inline void find_dsts_operand(granary::instruction &instr) {
		    unsigned int i = 0, max = instr.num_destinations();
		    for(; i < max; ++i) {
		    	//memory_dsts_operand_finder(instr.instr.dsts[i]);
		    }
		}
		inline void find_src_operand(granary::instruction &instr) {

		}

	}memory_operand_modifier;
}


#endif /* WATCHPOINT_UTIL_H_ */
