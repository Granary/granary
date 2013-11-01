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
    app_pc *IBL_JUMP_TABLE = nullptr;


    /// Address of the global code cache lookup function.
    static app_pc global_code_cache_find(nullptr);


    /// Nice names for registers used by the IBL and RBL.
    static operand reg_target_addr;
    static operand reg_target_addr_16;


    static app_pc GLOBAL_CODE_CACHE_ROUTINE = nullptr;
    static app_pc IBL_ENTRY_ROUTINE = nullptr;


    /// The routine that indirectly jumps to either an IBL exit stub (that
    /// checks if it's the correct exit stub) or jumps to the slow path (full
    /// code cache lookup).
    app_pc ibl_lookup_routine(void) throw() {
        if(IBL_ENTRY_ROUTINE) {
            return IBL_ENTRY_ROUTINE;
        }

        // On the stack:
        //      redzone                 (cond. saved user space)
        //      reg_target_addr         (saved: arg1, mangled target address)
        //      rax                     (saved: unmangled target address)
        //      flags                   (saved: arithmetic flags)

        instruction_list ibl(INSTRUCTION_LIST_GENCODE);

        const uintptr_t jump_table_base(
            reinterpret_cast<uintptr_t>(IBL_JUMP_TABLE));

        // TODO: need to find a way to get the hash index and add it to the
        //       base, without disturbin the base bits.

        ibl.append(mov_imm_(reg::rax, int64_(jump_table_base)));

        ASSERT(!(jump_table_base & 0xFFULL));

        // Hash function for IBL table. Make sure to keep this consistent
        // with `granary_ibl_hash` located in granary/x86/utils.asm.
        ibl.append(mov_ld_(reg::ax, reg_target_addr_16));

        ibl.append(shr_(reg::ax, int8_(4)));
        ibl.append(shl_(reg::ax, int8_(4)));
        ibl.append(xchg_(reg::ah, reg::al));
        ibl.append(rol_(reg::ah, int8_(3)));
        ibl.append(rol_(reg::ax, int8_(4)));

        ibl.append(or_(
            reg::ah,
            int8_((int8_t) (uint8_t) ((jump_table_base >> 8) & 0xFF))));

        // Go off to the
        ibl.append(jmp_ind_(*reg::rax));

        // Encode.
        const unsigned size(ibl.encoded_size());
        IBL_ENTRY_ROUTINE = global_state::FRAGMENT_ALLOCATOR->\
            allocate_array<uint8_t>(size);
        ibl.encode(IBL_ENTRY_ROUTINE, size);

        IF_PERF( perf::visit_ibl(ibl); )

        return IBL_ENTRY_ROUTINE;
    }


    void ibl_lock(void) throw() {
        IBL_JUMP_TABLE_LOCK.acquire();
    }


    void ibl_unlock(void) throw() {
        IBL_JUMP_TABLE_LOCK.release();
    }


    /// Return or generate the IBL exit routine for a particular jump target.
    /// The target can either be code cache or native code.
    app_pc ibl_exit_routine(
        app_pc native_target_pc,
        app_pc mangled_target_pc,
        app_pc instrumented_target_pc
    ) throw() {

        instruction_list ibl;
        instruction ibl_hit_from_code_cache_find(label_());
        instruction ibl_miss(label_());

        // CASE1 : We're coming in through the IBL jump table, or through
        //         a chained exit routine that was part of the IBL jump table.
        //
        // On the stack:
        //      redzone                 (cond. saved: user space)
        //      reg_target_addr         (saved: arg1)
        //      rax                     (saved: clobbered)
        //      aithmetic flags         (saved)
        ibl.append(mov_imm_(
            reg::rax, int64_(reinterpret_cast<uint64_t>(mangled_target_pc))));
        ibl.append(cmp_(reg::rax, reg_target_addr));
        ibl.append(jnz_(instr_(ibl_miss)));

        // Hit! We found the right target.
        insert_restore_arithmetic_flags_after(ibl, ibl.last(), REG_AH_IS_DEAD);
        ibl.append(pop_(reg::rax));

        // CASE 2: The compare from case (1) succeeded, or we're coming in from
        //         a return from the global code cache find function, and so
        //
        // On the stack:
        //      redzone                 (cond. user space)
        //      reg_target_addr         (saved: arg1)
        ibl.append(ibl_hit_from_code_cache_find);
        ibl.append(pop_(reg_target_addr));
        IF_USER( ibl.append(lea_(reg::rsp, reg::rsp[REDZONE_SIZE])); )

        ASSERT(nullptr != instrumented_target_pc);
        insert_cti_after(
            ibl, ibl.last(), instrumented_target_pc,
            CTI_DONT_STEAL_REGISTER, operand(),
            CTI_JMP);

        ibl.append(ibl_miss);

        const unsigned index(granary_ibl_hash(native_target_pc));
        ASSERT(index < NUM_IBL_JUMP_TABLE_ENTRIES);

        app_pc prev_target(IBL_JUMP_TABLE[index]);
        ASSERT(nullptr != prev_target);
        if(prev_target != GLOBAL_CODE_CACHE_ROUTINE) {
            IF_PERF( perf::visit_ibl_conflict(native_target_pc); )
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

        IF_PERF( perf::visit_ibl_add_entry(native_target_pc); )

        // The value stored in code cache find isn't the full value!!
        return ibl_hit_from_code_cache_find.pc_or_raw_bytes();
    }


    /// Return the IBL entry routine. The IBL entry routine is responsible
    /// for looking to see if an address (stored in reg::arg1) is located
    /// in the CPU-private code cache or in the global code cache. If the
    /// address is in the CPU-private code cache.
    static app_pc global_code_cache_lookup_routine(void) throw() {

        instruction_list ibl;

        // On the stack:
        //      redzone                 (cond. saved if user space)
        //      reg_target_addr         (saved: arg1, mangled address)
        //      rax                     (saved: clobbered)
        //      arithmetic flags        (saved)

        IF_KERNEL( insert_restore_arithmetic_flags_after(
            ibl, ibl.last(), REG_AH_IS_DEAD); )
        IF_KERNEL( insert_save_flags_after(ibl, ibl.last(), REG_AH_IS_DEAD); )

        // save all registers for the IBL.
        register_manager all_regs;
        all_regs.kill_all();
        all_regs.revive(reg_target_addr);
        all_regs.revive(reg::rax);

        // Restore callee-saved registers, because the global code cache routine
        // respects the ABI.
        all_regs.revive(reg::rbx);
        all_regs.revive(reg::rbp);
        all_regs.revive(reg::r12);
        all_regs.revive(reg::r13);
        all_regs.revive(reg::r14);
        all_regs.revive(reg::r15);

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
        safe = ibl.insert_after(safe, mov_ld_(reg_target_addr, reg::ret));

        // Exit from the private stack again.
        IF_KERNEL( safe = insert_cti_after(
            ibl, safe, unsafe_cast<app_pc>(granary_exit_private_stack),
            CTI_STEAL_REGISTER, reg::ret,
            CTI_CALL); )

        // Restore the flags.
        IF_USER( insert_restore_arithmetic_flags_after(
            ibl, ibl.last(), REG_AH_IS_DEAD); )
        IF_KERNEL( insert_restore_flags_after(
            ibl, ibl.last(), REG_AH_IS_DEAD); )

        ibl.append(pop_(reg::rax));

        // Jump to the target. The target in this case is an IBL exit routine,
        // which is responsible for cleaning up the stack (reg_target_addr
        // and a conditionally guarded redzone).
        ibl.append(jmp_ind_(reg_target_addr));

        const unsigned size(ibl.encoded_size());
        app_pc temp(global_state::FRAGMENT_ALLOCATOR-> \
            allocate_array<uint8_t>(size));
        ibl.encode(temp, size);

        IF_PERF( perf::visit_ibl(ibl); )

        return temp;
    }


    STATIC_INITIALISE_ID(ibl, {

        // Set the initial values for global static variables used in this
        // file.
        reg_target_addr = reg::arg1;
        reg_target_addr_16 = reg::arg1_16;
        global_code_cache_find = unsafe_cast<app_pc>(
            (app_pc (*)(mangled_address)) code_cache::find);

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
        IBL_JUMP_TABLE = reinterpret_cast<app_pc *>(jump_table_base);

        // Create the IBL entry routine now that we have the jump table address.
        ibl_lookup_routine();

        // Initialise the jump table.
        GLOBAL_CODE_CACHE_ROUTINE = global_code_cache_lookup_routine();
        for(unsigned i(0); i < NUM_IBL_JUMP_TABLE_ENTRIES; ++i) {
            IBL_JUMP_TABLE[i] = GLOBAL_CODE_CACHE_ROUTINE;
        }
    });

}

