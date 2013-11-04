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
    app_pc *IBL_JUMP_TABLE = nullptr;


    /// Address of the global code cache lookup function.
    static app_pc global_code_cache_find(nullptr);


    /// Nice names for registers used by the IBL.
    static operand reg_target_addr;
    static operand reg_target_addr_16;
    IF_PROFILE_IBL( static operand reg_source_addr; )


    static app_pc GLOBAL_CODE_CACHE_ROUTINE = nullptr;
    static app_pc IBL_JUMP_ROUTINE = nullptr;

    static void make_ibl_jump_routine(void) throw() {
        instruction_list ibl;

        // On the stack:
        //      redzone                 (cond. saved user space)
        //      reg_target_addr         (saved: arg1, mangled target address)
        //      source addr             (cond. saved: IBL profiling)

        // Spill RAX (reg_table_entry_addr) and then save the flags onto the
        // stack now that rax can be killed.
        ibl.append(push_(reg::rax));

        // Save the flags.
        insert_save_arithmetic_flags_after(ibl, ibl.last(), REG_AH_IS_DEAD);

        const uintptr_t jump_table_base(
            reinterpret_cast<uintptr_t>(IBL_JUMP_TABLE));
        ibl.append(mov_imm_(reg::rax, int64_(jump_table_base)));

        // Store the unmangled address into %rax.
        ibl.append(mov_ld_(reg::ax, reg_target_addr_16));
        ibl.append(shr_(reg::ax, int8_(5)));
        ibl.append(shl_(reg::ax, int8_(3)));

        const uint8_t jump_table_mask(((jump_table_base >> 8) & 0xFFU));
        if(jump_table_mask) {
            ibl.append(or_(
                reg::ah,
                int8_((int64_t) (int8_t) jump_table_mask)));
        }

        IF_PERF( perf::visit_ibl(ibl); )

        // Go off to the
        ibl.append(jmp_ind_(*reg::rax));

        // Encode.
        const unsigned size(ibl.encoded_size());
        IBL_JUMP_ROUTINE = global_state::FRAGMENT_ALLOCATOR-> \
            allocate_array<uint8_t>(size);
        ibl.encode(IBL_JUMP_ROUTINE, size);
    }


    /// The routine that indirectly jumps to either an IBL exit stub (that
    /// checks if it's the correct exit stub) or jumps to the slow path (full
    /// code cache lookup).
    void ibl_lookup_stub(
        instruction_list &ibl,
        instrumentation_policy policy
        _IF_PROFILE_IBL( app_pc cti_addr )
    ) throw() {

        // On the stack:
        //      redzone                 (cond. saved: user space)
        //      reg_target_addr         (saved: arg1)

        // Mangle the target address.
        ibl.append(bswap_(reg_target_addr));
        ibl.append(mov_imm_(
            reg_target_addr_16,
            int16_((int64_t) (int16_t) granary_bswap16(policy.encode()))));
        ibl.append(bswap_(reg_target_addr));

#if CONFIG_PROFILE_IBL
        // Spill the source address for use by the IBL profiling.
        ibl.append(push_(reg_source_addr));
        ibl.append(mov_imm_(
            reg_source_addr,
            int64_(reinterpret_cast<uint64_t>(cti_addr))));
#endif

        // Jump to the lookup routine.
        ibl.append(jmp_(pc_(IBL_JUMP_ROUTINE)));
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
        app_pc mangled_target_pc,
        app_pc instrumented_target_pc
        _IF_PROFILE_IBL( app_pc source_addr )
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
        //      reg_source_addr         (cond. saved: IBL profiling)
        //      rax                     (saved, can be clobbered)
        //      aithmetic flags         (saved)
        ibl.append(mov_imm_(
            reg::rax, int64_(reinterpret_cast<uint64_t>(mangled_target_pc))));
        ibl.append(cmp_(reg::rax, reg_target_addr));
        ibl.append(jnz_(instr_(ibl_miss)));

#if CONFIG_PROFILE_IBL
        // Check that it's going to the right target and coming from the right
        // source.

        ibl.append(mov_imm_(
            reg::rax,
            int64_(reinterpret_cast<uint64_t>(source_addr))));
        ibl.append(cmp_(reg::rax, reg_source_addr));
        instruction ibl_count_hit(label_());
        ibl.append(jz_(instr_(ibl_count_hit)));

        // Create hit and miss counters for this (source, dest) pair.
        uint64_t *hit_miss_count(allocate_memory<uint64_t>(2));
        hit_miss_count[0] = 1; // hit count.
        hit_miss_count[1] = 0; // miss count.

        // Fall-through miss, increment the miss count
        ibl.append(mov_imm_(
            reg::rax,
            int64_(reinterpret_cast<uint64_t>(&(hit_miss_count[1])))));
        ibl.append(atomic(inc_(*reg::rax)));
        ibl.append(jmp_short_(instr_(ibl_miss)));

        // Hit, increment the hit count.
        ibl.append(ibl_count_hit);
        ibl.append(mov_imm_(
            reg::rax,
            int64_(reinterpret_cast<uint64_t>(hit_miss_count))));
        ibl.append(atomic(inc_(*reg::rax)));

#endif

        // Hit! We found the right target.
        insert_restore_arithmetic_flags_after(ibl, ibl.last(), REG_AH_IS_DEAD);
        ibl.append(pop_(reg::rax));

        // CASE 2: The compare from case (1) succeeded, or we're coming in from
        //         a return from the global code cache find function, and so
        //
        // On the stack:
        //      redzone                 (cond. user space)
        //      reg_target_addr         (saved: arg1)
        //      reg_source_addr         (cond. saved: IBL profiling)
        ibl.append(ibl_hit_from_code_cache_find);
        IF_PROFILE_IBL( ibl.append(pop_(reg_source_addr)); )
        ibl.append(pop_(reg_target_addr));
        IF_USER( ibl.append(lea_(reg::rsp, reg::rsp[REDZONE_SIZE])); )

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

        IF_PERF( perf::visit_ibl_add_entry(
            mangled_target_pc _IF_PROFILE_IBL(source_addr, hit_miss_count)); )

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
        //      reg_source_addr         (cond. saved: IBL profiling)
        //      rax                     (saved: can be clobbered)
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
        IF_PROFILE_IBL( reg_source_addr = reg::arg2; )

        global_code_cache_find = unsafe_cast<app_pc>(
            (app_pc (*)(mangled_address _IF_PROFILE_IBL(app_pc))) code_cache::find);

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

        // Initialise the jump table.
        GLOBAL_CODE_CACHE_ROUTINE = global_code_cache_lookup_routine();
        for(unsigned i(0); i < NUM_IBL_JUMP_TABLE_ENTRIES; ++i) {
            IBL_JUMP_TABLE[i] = GLOBAL_CODE_CACHE_ROUTINE;
        }

        // Make the jump table lookup + hash routine.
        make_ibl_jump_routine();
    });

}

