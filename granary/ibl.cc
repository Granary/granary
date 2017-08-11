/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * ibl.cc
 *
 *  Created on: 2013-10-31
 *      Author: Peter Goodman
 */

#include "granary/ibl.h"
#include "granary/instruction.h"
#include "granary/code_cache.h"
#include "granary/emit_utils.h"
#include "granary/spin_lock.h"


extern "C" {
    extern uint16_t granary_bswap16(uint16_t);
}


namespace granary {


    // Make sure this is consistent with `granary_ibl_hash` in
    // `granary/x86/utils.h` and with
    enum {
        IBL_JUMP_TABLE_ALIGN = NUM_IBL_JUMP_TABLE_ENTRIES * sizeof(app_pc)
    };


    __attribute__((aligned (16)))
    static uint8_t IBL_JUMP_TABLE_IMPL[IBL_JUMP_TABLE_ALIGN * 2] = {0};


    /// Coarse grained lock around creating and adding new entries to the IBL.
    /// This lock is exposed through `ibl_lock` and `ibl_unlock` so that the
    /// IBL benefits from the same level of consistency as the global code
    /// cache does.
    static spin_lock IBL_JUMP_TABLE_LOCK;


    /// Aligned beginning of the jump table.
    static app_pc *IBL_JUMP_TABLE = nullptr;
    static dynamorio::instr_t IBL_JUMP_TABLE_INSTR;


    /// Address of the global code cache lookup function.
    static app_pc global_code_cache_find(nullptr);


    static app_pc GLOBAL_CODE_CACHE_ROUTINE = nullptr;


    /// The routine that indirectly jumps to either an IBL exit stub (that
    /// checks if it's the correct exit stub) or jumps to the slow path (full
    /// code cache lookup).
    void ibl_lookup_stub(
        instruction_list &ibl,
        instruction in,
        instrumentation_policy policy
    ) {

        // On the stack:
        //      indirect_target_addr    (saved: arg1, mangled target address)
        //      indirect_clobber_reg    (saved: rax)
        //      indirect_source_addr    (saved: arg2, cache source address)
        //      saved flags             (saved: rax, AFLAGS)

        // Mangle the target address.
        ibl.insert_before(in, bswap_(reg::indirect_target_addr));
        ibl.insert_before(in, IF_USER_ELSE(or_, and_)(
            reg::indirect_target_addr_16,
            int16_((int64_t) (int16_t) granary_bswap16(policy.encode()))));
        ibl.insert_before(in, bswap_(reg::indirect_target_addr));

        // Get the base of the table into `reg_source_addr`.
        ibl.insert_before(in,
            lea_(reg::indirect_source_addr, mem_instr_(&IBL_JUMP_TABLE_INSTR)));

        // Hash the target in `indirect_clobber_reg`.
        ibl.insert_before(in, mov_ld_(
            reg::indirect_clobber_reg, reg::indirect_target_addr));

        ibl.insert_before(in, shl_(reg::indirect_clobber_reg, int8_(48)));
        ibl.insert_before(in, shr_(reg::indirect_clobber_reg, int8_(48 + 5)));

        ibl.insert_before(in, lea_(
            reg::indirect_clobber_reg,
            reg::indirect_source_addr + reg::indirect_clobber_reg * 8));

        // Get a source address into the basic block for trace allocator
        // propagation.
        instruction block_instr(ibl.insert_before(in, label_()));
        ibl.insert_before(in,
            lea_(reg::indirect_source_addr, mem_instr_(block_instr)));

        // On the stack:
        //      indirect_target_addr    (saved: arg1, mangled target address)
        //      indirect_clobber_reg    (saved: rax)
        //      indirect_source_addr    (saved: arg2, cache source address)
        //      saved flags             (saved: rax, AFLAGS)

        // Go off to either `code_cache::find` or a target-specific checker.
        ibl.insert_before(in, mangled(jmp_ind_(*reg::indirect_clobber_reg)));
    }


    void ibl_lock(void) {
        IBL_JUMP_TABLE_LOCK.acquire();
    }


    void ibl_unlock(void) {
        IBL_JUMP_TABLE_LOCK.release();
    }


    /// Return or generate the IBL exit routine for a particular jump target.
    /// The target can either be code cache or native code.
    app_pc ibl_exit_routine(
        app_pc mangled_target_pc,
        app_pc instrumented_target_pc
    ) {

        instruction_list ibl;
        instruction ibl_hit_from_code_cache_find(label_());
        instruction ibl_miss(label_());

        // CASE1 : We're coming in through the IBL jump table, or through
        //         a chained exit routine that was part of the IBL jump table.
        //
        // On the stack:
        //      indirect_target_addr    (saved: arg1, mangled target address)
        //      indirect_clobber_reg    (saved: rax)
        //      indirect_source_addr    (saved: arg2, cache source address)
        //      saved flags             (saved: rax, AFLAGS)
        ibl.append(mov_imm_(
            reg::rax, int64_(reinterpret_cast<uint64_t>(mangled_target_pc))));
        ibl.append(cmp_(reg::rax, reg::indirect_target_addr));
        ibl.append(jnz_(instr_(ibl_miss)));

        // Hit! We found the right target.
        insert_restore_arithmetic_flags_after(ibl, ibl.last(), REG_AH_IS_DEAD);
        ibl.append(pop_(reg::indirect_source_addr));
        ibl.append(pop_(reg::indirect_clobber_reg));

        // CASE 2: The compare from case (1) succeeded, or we're coming in from
        //         a return from the global code cache find function, and so
        //
        // On the stack:
        //      reg_target_addr         (saved: arg1)
        ibl.append(ibl_hit_from_code_cache_find);
        ibl.append(pop_(reg::indirect_target_addr));

        ASSERT(nullptr != instrumented_target_pc);
        insert_cti_after(
            ibl, ibl.last(), instrumented_target_pc,
            CTI_DONT_STEAL_REGISTER, operand(),
            CTI_JMP);

        ibl.append(ibl_miss);

        const unsigned index(granary_ibl_hash(mangled_target_pc));
        ASSERT(index < NUM_IBL_JUMP_TABLE_ENTRIES);

        app_pc prev_target(IBL_JUMP_TABLE[index]);
        ASSERT(nullptr != prev_target);
        if(prev_target != GLOBAL_CODE_CACHE_ROUTINE) {
            IF_PERF( perf::visit_ibl_conflict(mangled_target_pc); )
        }

        // Only allow one thread/core to update the IBL jump table at a time.
        insert_cti_after(
            ibl, ibl.last(), prev_target,
            CTI_DONT_STEAL_REGISTER, operand(),
            CTI_JMP);
        IF_PERF( perf::visit_ibl_exit(ibl); )

        // Encode the IBL exit routine.
        const unsigned size(ibl.encoded_size());
        app_pc routine(global_state::FRAGMENT_ALLOCATOR-> \
            allocate_array<uint8_t>(size));
        ibl.encode(routine, size);
        IBL_JUMP_TABLE[index] = routine;

        IF_PERF( perf::visit_ibl_add_entry(mangled_target_pc); )

        // The value stored in code cache find isn't the full value!!
        return ibl_hit_from_code_cache_find.pc_or_raw_bytes();
    }


    /// Return the IBL entry routine. The IBL entry routine is responsible
    /// for looking to see if an address (stored in reg::arg1) is located
    /// in the CPU-private code cache or in the global code cache. If the
    /// address is in the CPU-private code cache.
    static app_pc global_code_cache_lookup_routine(void) {

        instruction_list ibl;

        // On the stack:
        //      indirect_target_addr    (saved: arg1, mangled target address)
        //      indirect_clobber_reg    (saved: rax)
        //      indirect_source_addr    (saved: arg2, cache source address)
        //      saved flags             (saved: rax, AFLAGS)

#if CONFIG_ENV_KERNEL
        insert_restore_arithmetic_flags_after(
            ibl, ibl.last(), REG_AH_IS_DEAD);

        insert_save_flags_after(ibl, ibl.last(), REG_AH_IS_DEAD);
#endif

        // On the stack:
        //      indirect_target_addr    (saved: arg1, mangled target address)
        //      indirect_clobber_reg    (saved: rax)
        //      indirect_source_addr    (saved: arg2, cache source address)
        //      saved flags             (saved: pushf, RFLAGS)

        // save all registers for the IBL.
        register_manager all_regs;
        all_regs.kill_all();
        all_regs.revive(reg::indirect_target_addr);
        all_regs.revive(reg::indirect_clobber_reg);
        all_regs.revive(reg::indirect_source_addr);

        // Restore callee-saved registers, because the global code cache routine
        // respects the ABI.
        IF_NOT_TEST(
            all_regs.revive(reg::rbx);
            all_regs.revive(reg::rbp);
            all_regs.revive(reg::r12);
            all_regs.revive(reg::r13);
            all_regs.revive(reg::r14);
            all_regs.revive(reg::r15);
        )

        // Create a "safe" region of code around which all registers are saved
        // and restored.
        instruction safe(
            save_and_restore_registers(all_regs, ibl, ibl.last()));

        // Find the local private stack to use. %rax is safe to clobber for
        // the target address because `granary_enter_private_stack` will
        // clobber it too.
        IF_KERNEL( safe = insert_cti_after(
            ibl, safe, unsafe_cast<app_pc>(granary_enter_private_stack),
            CTI_STEAL_REGISTER, reg::ret,
            CTI_CALL); )

        safe = insert_cti_after(
            ibl, safe, global_code_cache_find,
            CTI_STEAL_REGISTER, reg::ret,
            CTI_CALL);

        // Stash the return value before it disappears because
        // `granary_exit_private_stack` clobbers %rax.
        safe = ibl.insert_after(safe,
            mov_ld_(reg::indirect_target_addr, reg::ret));

        // Exit from the private stack again.
        IF_KERNEL( safe = insert_cti_after(
            ibl, safe, unsafe_cast<app_pc>(granary_exit_private_stack),
            CTI_STEAL_REGISTER, reg::ret,
            CTI_CALL); )

        // On the stack:
        //      indirect_target_addr    (saved: arg1, mangled target address)
        //      indirect_clobber_reg    (saved: rax)
        //      indirect_source_addr    (saved: arg2, cache source address)
        //      saved flags             (saved: pushf, RFLAGS)

        // Restore the flags.
        insert_restore_flags_after(ibl, ibl.last(), REG_AH_IS_DEAD);

        // Restore the source address and clobber reg.
        ibl.append(pop_(reg::indirect_source_addr));
        ibl.append(pop_(reg::indirect_clobber_reg));

        // Jump to the target. The target in this case is an IBL exit routine,
        // which is responsible for cleaning up the stack (reg_target_addr
        // and a conditionally guarded redzone).
        ibl.append(jmp_ind_(reg::indirect_target_addr));

        const unsigned size(ibl.encoded_size());
        app_pc temp(global_state::FRAGMENT_ALLOCATOR-> \
            allocate_array<uint8_t>(size));
        ibl.encode(temp, size);

        IF_PERF( perf::visit_ibl(ibl); )

        return temp;
    }


    STATIC_INITIALISE_ID(ibl, {

        global_code_cache_find = unsafe_cast<app_pc>(
            (app_pc (*)(mangled_address, app_pc)) code_cache::find);

        // Double check that our hash function is valid.
        app_pc i_ptr(nullptr);
        unsigned min_index(0x10000U);
        unsigned max_index(0);
        for(unsigned i(0); i <= 0xFFFFU; ++i) {
            const unsigned index(granary_ibl_hash(i_ptr + i));
            ASSERT(NUM_IBL_JUMP_TABLE_ENTRIES > index);
            min_index = index < min_index ? index : min_index;
            max_index = index > max_index ? index : max_index;
        }

        // Sanity checking on the range of the hash table function.
        ASSERT((NUM_IBL_JUMP_TABLE_ENTRIES - 1) == max_index);
        ASSERT(0 == min_index);

        // Create a properly-aligned jump table base.
        uintptr_t jump_table_base(
            reinterpret_cast<uintptr_t>(&(IBL_JUMP_TABLE_IMPL[0])));
        jump_table_base += ALIGN_TO(jump_table_base, IBL_JUMP_TABLE_ALIGN);
        const app_pc jump_table_addr(
            reinterpret_cast<app_pc>(jump_table_base));

        IBL_JUMP_TABLE = reinterpret_cast<app_pc *>(jump_table_base);

        memset(&IBL_JUMP_TABLE_INSTR, 0, sizeof IBL_JUMP_TABLE_INSTR);
        IBL_JUMP_TABLE_INSTR.opcode = dynamorio::OP_LABEL;
        IBL_JUMP_TABLE_INSTR.translation = jump_table_addr;
        IBL_JUMP_TABLE_INSTR.bytes = jump_table_addr;
        IBL_JUMP_TABLE_INSTR.note = jump_table_addr;
        IBL_JUMP_TABLE_INSTR.flags |= dynamorio::INSTR_RAW_BITS_VALID;

        // Initialise the jump table.
        GLOBAL_CODE_CACHE_ROUTINE = global_code_cache_lookup_routine();
        for(unsigned i(0); i < NUM_IBL_JUMP_TABLE_ENTRIES; ++i) {
            IBL_JUMP_TABLE[i] = GLOBAL_CODE_CACHE_ROUTINE;
        }
    });

}

