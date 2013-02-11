/*
 * code_cache.cc
 *
 *  Created on: 2012-11-28
 *      Author: pag
 *     Version: $Id$
 */

#include "granary/code_cache.h"
#include "granary/detach.h"
#include "granary/hash_table.h"
#include "granary/basic_block.h"
#include "granary/utils.h"
#include "granary/register.h"
#include "granary/emit_utils.h"

namespace granary {

    /// Static initialisation of the global IBL lookup routine.
#if CONFIG_TRACK_XMM_REGS
    app_pc code_cache::IBL_COMMON_ENTRY_ROUTINE(nullptr);
#endif
    app_pc code_cache::XMM_SAFE_IBL_COMMON_ENTRY_ROUTINE(nullptr);


    namespace {


        /// The globally shared code cache. This maps policy-mangled code
        /// code addresses to translated addresses.
        static shared_hash_table<app_pc, app_pc> CODE_CACHE;
    }


    /// Find fast. This looks in the cpu-private cache first, and failing
    /// that, defaults to the global code cache.
    app_pc code_cache::find_on_cpu(mangled_address addr) throw() {
        cpu_state_handle cpu;
        return cpu->code_cache.find(addr.as_address);
    }


    /// Perform both lookup and insertion (basic block translation) into
    /// the code cache.
    app_pc code_cache::find(
        cpu_state_handle &cpu,
        thread_state_handle &thread,
        mangled_address addr
    ) throw() {

        // find the actual targeted address, independent of the policy.
        app_pc app_target_addr(addr.unmangled_address());

        // Determine if this is actually a detach point. This is only relevant
        // for indirect calls/jumps because direct calls and jumps will have
        // inlined this check at basic block translation time.
        app_pc target_addr(find_detach_target(app_target_addr));
        if(nullptr != target_addr) {
            cpu->code_cache.store(addr.as_address, target_addr);
            return target_addr;
        }

        // Try to load the target address from the global code cache.
        if(CODE_CACHE.load(addr.as_address, target_addr)) {
            cpu->code_cache.store(addr.as_address, target_addr);
            return target_addr;
        }

        // figure out the non-policy-mangled target address, and get our policy.
        instrumentation_policy policy(addr);
        instrumentation_policy base_policy(policy.base_policy());
        mangled_address base_addr(app_target_addr, base_policy);

        // translate the basic block according to the policy.
        basic_block bb(basic_block::translate(
            base_policy, cpu, thread, &app_target_addr));

        target_addr = bb.cache_pc_start;

        // store the translated block in the code cache; if the store fails,
        // then that means the block already exists in the code cache (e.g.
        // because a concurrent thread "won" when translating the same
        // block). If the latter is the case, implicitly free the block
        // by freeing the last thing used in the fragment allocator.
        if(!CODE_CACHE.store(base_addr.as_address, target_addr, HASH_KEEP_PREV_ENTRY)) {
            cpu->fragment_allocator.free_last();
            cpu->block_allocator.free_last();

            // TODO: minor memory leak with basic block state and vtables.
            //       consider switching to a "transactional" allocator.
            CODE_CACHE.load(base_addr.as_address, target_addr);
        }

        // TODO: try to pre-load the cache with internal jump targets of the
        //       just-stored basic block (if config option permits).

        // propagate down to the CPU-private code cache.
        cpu->code_cache.store(base_addr.as_address, target_addr);

        // was this an indirect entry? If so, we need to direct control flow
        // to the corresponding IBL exit routine.
        if(policy.is_indirect_cti_policy()) {
            target_addr = ibl_exit_for(target_addr);
            if(!CODE_CACHE.store(addr.as_address, target_addr, HASH_KEEP_PREV_ENTRY)) {
                cpu->fragment_allocator.free_last();
                CODE_CACHE.load(addr.as_address, target_addr);
            }

            cpu->code_cache.store(addr.as_address, target_addr);
        }

        return target_addr;
    }

    /*
    /// Add an entry to the code cache for later prediction.
    void code_cache::predict(cpu_state_handle &cpu,
                             thread_state_handle &thread,
                             app_pc app_code,
                             app_pc cache_code) throw() {

        CODE_CACHE.store(app_code, cache_code);
        //CODE_CACHE.store(cache_code, cache_code);

        (void) cpu;
        (void) thread;
    }
    */


    /// Add a call to a known function after a particular instruction in the
    /// instruction stream. If the location of the function to be called is
    /// too far away then a specified register is clobbered with the target pc,
    /// and an indirect jump is performed.
    instruction_list_handle add_direct_call_after(
        instruction_list &ls,
        instruction_list_handle in,
        app_pc target,
        operand clobber_reg
    ) throw() {
        // start with a staged allocation that should be close enough
        app_pc staged_loc(
            global_state::FRAGMENT_ALLOCATOR.allocate_staged<uint8_t>());

        // add in the indirect call to the find on CPU function
        if(is_far_away(staged_loc, target)) {

            in = ls.insert_after(in,
                mov_imm_(clobber_reg,
                    int64_(unsafe_cast<int64_t>(target))));
            in = ls.insert_after(in, call_ind_(clobber_reg));

        // add in a direct, pc relative call.
        } else {
            in = ls.insert_after(in, call_(pc_(target)));
        }

        return in;
    }


    /// Create the IBL exit for for a program counter. See:
    /// `instruction_list_mangler::ibl_entry_for`, as this is sort of the
    /// opposite of it. The purpose of this is that we now know where we're
    /// going, we just need to clean up stuff that we've saved on the stack.
    app_pc code_cache::ibl_exit_for(app_pc pc) throw() {
        instruction_list ibl;

        // restore the flags / interrupts.
#if GRANARY_IN_KERNEL
        ibl.append(popf_());
#elif CONFIG_IBL_SAVE_ALL_FLAGS
        ibl.append(popf_());
#else
        ibl.append(pop_(reg::rax));
        ibl.append(sahf_());
#endif

        ibl.append(pop_(reg::rax));
        ibl.append(pop_(reg::arg1));
        IF_USER( ibl.append(lea_(reg::rsp, reg::rsp[REDZONE_SIZE])); )
        ibl.append(jmp_(pc_(pc)));

        app_pc encoded(global_state::FRAGMENT_ALLOCATOR.allocate_array<uint8_t>(
            ibl.encoded_size()));

        ibl.encode(encoded);

        return encoded;
    }


    /// Initialise the indirect branch lookup routine. This method is
    /// complicated because it needs to find all potentially clobbered registers
    /// by a hash table lookup routine.
    void code_cache::init_ibl(app_pc &routine, bool in_xmm_context) throw() {
        app_pc find_on_cpu_func(unsafe_cast<app_pc>(find_on_cpu));
        app_pc find_in_global(unsafe_cast<app_pc>(
            (app_pc (*)(mangled_address)) find));
        register_manager cpu_regs(find_used_regs_in_func(find_on_cpu_func));

        // the IBL entyr points use reg::arg1 for passing the address thing to
        // be looked up, and save rax for potential flags and later use,
        // so it is guaranteed to already be saved.
        cpu_regs.revive(reg::arg1);
        cpu_regs.revive(reg::ret);

        instruction_list ibl;
        operand clobber_reg(reg::ret);

        instruction_list_handle in(
            save_and_restore_registers(cpu_regs, ibl, ibl.first()));

        if(in_xmm_context) {
            in = save_and_restore_xmm_registers(
                cpu_regs, ibl, in, XMM_SAVE_UNALIGNED);
        }

        // set up the label so that the fast path can target the instructions
        // that restore the registers saved by the cpu-private code cache
        // lookup funciton.
        instruction_list_handle fast_path(ibl.insert_after(in, label_()));

        // make a copy of the address to be lookup up in case we hit the
        // slow path; accessing this later is a bit tricky.
        in = ibl.insert_after(in, push_(reg::arg1));

        // align the stack to a 16 byte boundary before the call.
        in = ibl.insert_after(in, push_(reg::rsp));
        in = ibl.insert_after(in, push_(reg::rsp[0]));
        in = ibl.insert_after(in, and_(reg::rsp, int8_(-16)));

        // call the cpu-private hash table lookup function
        in = add_direct_call_after(ibl, in, find_on_cpu_func, clobber_reg);

        // store the returned address in arg1 in case the lookup succeeded and
        // we go down the fast path.
        in = ibl.insert_after(in, mov_ld_(reg::arg1, reg::ret));

        // restore previous stack alignment
        in = ibl.insert_after(in, mov_ld_(reg::rsp, reg::rsp[8]));

        // bring the previously saved lookup value back into rax as a way of
        // setting up for the global lookup call. Either it will be used (slow
        // path), or it will be clobbered (fast path).
        in = ibl.insert_after(in, pop_(reg::ret));

        // check the return value
        in = ibl.insert_after(in, test_(reg::arg1, reg::arg1));

        // check if we should take the fast or slow path
        in = ibl.insert_after(in, jnz_(instr_(*fast_path)));

        // slow path: do a global lookup.

        // move the old lookup address back into arg1
        in = ibl.insert_after(in, mov_ld_(reg::arg1, reg::ret));

        register_manager global_regs(cpu_regs);
        global_regs.kill_all_live();

        // want to make sure we don't save arg1, so that later we can get at
        // it through a move of ret (rax) to arg1 (rdi).
        global_regs.revive(reg::arg1);
        global_regs.revive(reg::ret);

        instruction_list_handle slow_in(
            save_and_restore_registers(global_regs, ibl, in));

        // re-align the stack to a 16 byte boundary before the call.
        slow_in = ibl.insert_after(slow_in, push_(reg::rsp));
        slow_in = ibl.insert_after(slow_in, push_(reg::rsp[0]));
        slow_in = ibl.insert_after(slow_in, and_(reg::rsp, int8_(-16)));

        // call the cpu-private hash table lookup function
        slow_in = add_direct_call_after(ibl, slow_in, find_in_global, clobber_reg);
        slow_in = ibl.insert_after(slow_in, mov_ld_(reg::arg1, reg::ret));

        // restore previous stack alignment
        slow_in = ibl.insert_after(slow_in, mov_ld_(reg::rsp, reg::rsp[8]));

        // fast path, the cpu-lookup succeeded.
        // OR: we defaulted to the slow path and just did a global lookup.
        ibl.append(jmp_ind_(reg::arg1));

        routine = global_state::FRAGMENT_ALLOCATOR.\
            allocate_array<uint8_t>(ibl.encoded_size());

        // encode it
        ibl.encode(routine);
    }
}
