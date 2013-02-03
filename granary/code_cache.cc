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

namespace granary {

    enum {
        _4GB = 4294967296ULL
    };


    /// Static initialisation of the global IBL lookup routine.
    app_pc code_cache::IBL_COMMON_ENTRY_ROUTINE(nullptr);


    namespace {


        /// The globally shared code cache. This maps policy-mangled code
        /// code addresses to translated addresses.
        static shared_hash_table<app_pc, app_pc> CODE_CACHE;


        /// This is a bit of a hack. The idea here is that we want an example
        /// of an address within the current address space, so that we can use
        /// its high-order bits to "fix up" an address that encodes a policy in
        /// its high-order bits. This should be independent of user/kernel
        /// space, at least for typical 48-bit address space implementations
        /// use the high-order bits should be all 0 or all 1.
        static unsigned char SOME_ADDRESS;


        /// This is an "unmangled" mangled version of the address of
        /// SOME_ADDRESS. It exists only for convenient extraction of the high-
        /// order bits through the `mangled_address` fields.
        static mangled_address UNMANGLED_ADDRESS;
    }


    /// Find fast. This looks in the cpu-private cache first, and failing
    /// that, defaults to the global code cache.
    app_pc code_cache::find_on_cpu(mangled_address addr) throw() {
        cpu_state_handle cpu;
        return cpu->code_cache.find(addr.as_address);
    }


    /// Initialise the above hack.
    STATIC_INITIALISE({
        UNMANGLED_ADDRESS.as_address = &SOME_ADDRESS;
    });


    /// Perform both lookup and insertion (basic block translation) into
    /// the code cache.
    app_pc code_cache::find(cpu_state_handle &cpu,
                            thread_state_handle &thread,
                            mangled_address addr) throw() {

        // find the actual targeted address, independent of the policy.
        mangled_address app_target_addr(addr);
        app_target_addr.as_int >>= mangled_address::POLICY_NUM_BITS;
        app_target_addr.as_recovery_address.lost_bits = \
            UNMANGLED_ADDRESS.as_recovery_address.lost_bits;

        // Determine if this is actually a detach point. This is only relevant
        // for indirect calls/jumps because direct calls and jumps will have
        // inlined this check at basic block translation time.
        app_pc target_addr(find_detach_target(app_target_addr.as_address));
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
        app_pc decode_addr(app_target_addr.as_address);
        instrumentation_policy policy(addr.as_policy_address.policy_id);

        // figure out the base policy for this address.
        instrumentation_policy base_policy(policy.base_policy());
        mangled_address base_addr(addr);
        base_addr.as_policy_address.policy_id = base_policy.policy_id;

        // translate the basic block according to the policy.
        basic_block bb(basic_block::translate(
            base_policy, cpu, thread, &decode_addr));

        target_addr = bb.cache_pc_start;

        // store the translated block in the code cache; if the store fails,
        // then that means the block already exists in the code cache (e.g.
        // because a concurrent thread "won" when translating the same
        // block). If the latter is the case, implicitly free the block
        // by freeing the last thing used in the fragment allocator.
        if(!CODE_CACHE.store(base_addr.as_address, target_addr, false)) {
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
            if(!CODE_CACHE.store(addr.as_address, target_addr)) {
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

    /// Traverse through the instruction control-flow graph and look for used
    /// registers.
    static register_manager find_used_regs_in_func(app_pc func) throw() {
        register_manager used_regs;
        list<app_pc> process_bbs;
        list<app_pc>::handle_type next;
        hash_set<app_pc> seen;

        used_regs.revive_all();
        process_bbs.append(func);

        // traverse instructions and find all used registers.
        for(unsigned i(0); i < process_bbs.length(); ++i) {

            if(!next.is_valid()) {
                next = process_bbs.first();
            } else {
                next = next.next();
            }

            app_pc bb(*next);
            if(seen.contains(bb)) {
                break;
            }

            seen.add(bb);

            for(; bb; ) {
                instruction in;

                in = instruction::decode(&bb);
                used_regs.kill(in);

                // done processing this basic block
                if(dynamorio::OP_ret == in.op_code()) {
                    break;
                }

                // this is a cti; add the destination instruction as
                if(in.is_cti()) {

                    operand target(in.cti_target());
                    if(!dynamorio::opnd_is_pc(target)) {
                        used_regs.kill_all();
                        return used_regs;
                    }

                    process_bbs.append(dynamorio::opnd_get_pc(target));
                }
            }
        }

        return used_regs;
    }


    /// Computes the absolute value of a pointer difference.
    inline static uint64_t pc_diff(ptrdiff_t diff) {
        if(diff < 0) {
            return static_cast<uint64_t>(-diff);
        }
        return static_cast<uint64_t>(diff);
    }


    /// Save all dead registers within a particular register manager. This is
    /// useful for saving/restoring only those registers used by a function.
    instruction_list_handle save_and_restore_registers(
        register_manager &regs,
        instruction_list &ls,
        instruction_list_handle in
    ) throw() {
        for(;;) {
            dynamorio::reg_id_t reg_id(regs.get_zombie());
            if(!reg_id) {
                break;
            }

            operand reg(reg_id);

            in = ls.insert_after(in, push_(reg));
            ls.insert_after(in, pop_(reg));
        }

        return in;
    }


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
        if(_4GB <= pc_diff(staged_loc - target)) {

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
#else
        ibl.append(popf_());
        //ibl.append(pop_(reg::rax));
        //ibl.append(lahf_());
#endif

        ibl.append(pop_(reg::rax));
        ibl.append(pop_(reg::arg1));
        ibl.append(jmp_(pc_(pc)));

        app_pc encoded(global_state::FRAGMENT_ALLOCATOR.allocate_array<uint8_t>(
            ibl.encoded_size()));

        ibl.encode(encoded);

        return encoded;
    }


    /// Initialise the indirect branch lookup routine. This method is
    /// complicated because it needs to find all potentially clobbered registers
    /// by a hash table lookup routine.
    void code_cache::init_ibl(void) throw() {
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

        IBL_COMMON_ENTRY_ROUTINE = global_state::FRAGMENT_ALLOCATOR.\
            allocate_array<uint8_t>(ibl.encoded_size());

        // encode it
        ibl.encode(IBL_COMMON_ENTRY_ROUTINE);
    }
}
