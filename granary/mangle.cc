/*
 * mangle.cc
 *
 *  Created on: Nov 21, 2012
 *      Author: pag
 */


#include "granary/mangle.h"
#include "granary/basic_block.h"
#include "granary/code_cache.h"
#include "granary/state.h"
#include "granary/detach.h"
#include "granary/register.h"
#include "granary/hash_table.h"
#include "granary/emit_utils.h"


/// Used to unroll registers in the opposite order in which they are saved
/// by PUSHA in x86/asm_helpers.asm. This is so that we can operate on the
/// pushed state on the stack as a form of machine context.
#define DPM_DECLARE_REG(reg) \
    uint64_t reg;


#define DPM_DECLARE_REG_CONT(reg, rest) \
    rest \
    DPM_DECLARE_REG(reg)


/// Used to forward-declare the assembly funcion patches. These patch functions
/// eventually call the templates.
#define DEFINE_DIRECT_JUMP_RESOLVER(opcode, size) \
    direct_branch_ ## opcode = \
        make_direct_cti_patch_func<opcode ## _ >( \
            granary_asm_direct_branch_template);

#define DEFINE_XMM_SAFE_DIRECT_JUMP_RESOLVER(opcode, size) \
    direct_branch_ ## opcode ## _xmm = \
        make_direct_cti_patch_func<opcode ## _ >( \
            granary_asm_xmm_safe_direct_branch_template);


/// Used to forward-declare the assembly function patches. These patch functions
/// eventually call the templates.
#define CASE_DIRECT_JUMP_MANGLER(opcode, size) \
    case dynamorio::OP_ ## opcode: return direct_branch_ ## opcode;
#define CASE_XMM_SAFE_DIRECT_JUMP_MANGLER(opcode, size) \
    case dynamorio::OP_ ## opcode: return direct_branch_ ## opcode ## _xmm;

/// Used to forward-declare the assembly funcion patches. These patch functions
/// eventually call the templates.
#define DECLARE_DIRECT_JUMP_MANGLER(opcode, size) \
    static app_pc direct_branch_ ## opcode;

#define DECLARE_XMM_SAFE_DIRECT_JUMP_MANGLER(opcode, size) \
    static app_pc direct_branch_ ## opcode ## _xmm;

extern "C" {
    extern void granary_asm_xmm_safe_direct_branch_template(void);
    extern void granary_asm_direct_branch_template(void);
}


namespace granary {

    namespace {

        /// Specific instruction manglers used for branch lookup.
#if CONFIG_TRACK_XMM_REGS
        DECLARE_DIRECT_JUMP_MANGLER(call, 5)
        FOR_EACH_DIRECT_JUMP(DECLARE_DIRECT_JUMP_MANGLER)
#endif

        DECLARE_XMM_SAFE_DIRECT_JUMP_MANGLER(call, 5)
        FOR_EACH_DIRECT_JUMP(DECLARE_XMM_SAFE_DIRECT_JUMP_MANGLER)
    }


    /// A machine context representation for direct jump/call patches. The
    /// structure contains as much state as is saved by the direct assembly
    /// direct jump patch functions.
    ///
    /// The high-level operation here is that the patch function will know
    /// what to patch because of the return address on the stack (we guarantee
    /// that hot-patchable instructions will be aligned on 8-byte boundaries)
    /// and that we know where we need to patch the target to by looking at the
    /// relative target offset from the beginning of application code.
    struct direct_cti_patch_mcontext {

        /// Saved registers.
        ALL_REGS(DPM_DECLARE_REG_CONT, DPM_DECLARE_REG)

        /// Saved flags.
        uint64_t flags;

        /// Return address into the patch code located in the tail of the
        /// basic block being patched. We use this to figure out the instruction
        /// to patch because the tail code ends with a jmp to that instruction.
        app_pc return_address_into_patch_tail;

        /// The target address of a jump, including the policy to use when
        /// translating the target basic block.
        mangled_address target_address;

    } __attribute__((packed));


    /// Represents an indirect operand.
    union indirect_operand {

        // Note: order of these fields is significant. The policy_id needs to
        //       align with the high-order bits in case the target of an
        //       indirect CTI is loaded through an absolute memory location. If
        //       that's the case, then the policy_id will only clobber the high
        //       bits, which is fine.
        struct {
            int32_t displacement; // low
            uint8_t base_reg;
            uint8_t index_reg;
            uint8_t scale;
            uint8_t policy_bits; // high
        } as_operand __attribute__((packed));

        uint64_t as_uint;

        indirect_operand(void) throw()
            : as_uint(0UL)
        { }


        /// Initialise an indirect operand (for use in operand-specific IBL
        /// lookup) from an operand and from a policy. Care is taken to make
        /// sure different kinds of indirect operands are distinguishable.
        indirect_operand(operand op, instrumentation_policy policy) throw()
            : as_uint(0UL)
        {
            if(dynamorio::opnd_is_abs_addr(op)
            || dynamorio::REL_ADDR_kind == op.kind) {
                as_uint = reinterpret_cast<uint64_t>(op.value.addr);

            } else if(dynamorio::BASE_DISP_kind == op.kind) {
                as_operand.base_reg = op.value.base_disp.base_reg;
                as_operand.index_reg = op.value.base_disp.index_reg;
                as_operand.scale = op.value.base_disp.scale;
                as_operand.displacement = op.value.base_disp.disp;

            } else if(dynamorio::REG_kind == op.kind) {
                as_operand.base_reg = op.value.reg;
                as_operand.scale = ~0; // bigger than any possible valid scale.
                as_operand.index_reg = ~0;

                // should make it unlikely to conflict with abs address kinds
                as_operand.displacement = ~0;
            } else {
                FAULT;
            }

            as_operand.policy_bits = policy.extension_bits();
        }
    };


    /// Represents a direct branch lookup operand that is specific to a
    /// mangled target address and to an opcode.
    union direct_operand {
        app_pc as_address;
        struct {
            uint64_t mangled_address:56; // low
            uint64_t op_code:8; // high
        } __attribute__((packed)) as_op;
    };


    /// The operand-specific IBL entry routines.
    static shared_hash_table<uint64_t, app_pc> IBL_ENTRY_ROUTINE;


    /// Hash table for managing direct branch lookup stubs.
    static shared_hash_table<app_pc, app_pc> DBL_ENTRY_ROUTINE;


    /// Policy+property-specific RBL entry routines.
    static shared_hash_table<uint8_t, app_pc> RBL_ENTRY_ROUTINE;


    /// Generate the tail of an IBL/RBL entry point.
    void instruction_list_mangler::ibl_entry_tail(
        instruction_list &ibl,
        instrumentation_policy target_policy
    ) throw() {

        // save flags / disable interrupts
#if GRANARY_IN_KERNEL
        ibl.append(pushf_());
        ibl.append(cli_());
#elif CONFIG_IBL_SAVE_ALL_FLAGS
        ibl.append(pushf_());
#else
        ibl.append(lahf_());
        ibl.append(push_(reg::rax));
#endif

        // add in the policy to the address to be looked up.
        ibl.append(shl_(reg::arg1, int8_(mangled_address::NUM_MANGLED_BITS)));
        ibl.append(or_(reg::arg1, int8_(target_policy.extension_bits())));

        // go to the common ibl lookup routine
#if CONFIG_TRACK_XMM_REGS
        app_pc ibl_routine(code_cache::IBL_COMMON_ENTRY_ROUTINE);
        if(target_policy.is_in_xmm_context()) {
            ibl_routine = code_cache::XMM_SAFE_IBL_COMMON_ENTRY_ROUTINE;
        }
        ibl.append(jmp_(pc_(ibl_routine)));
#else
        ibl.append(jmp_(pc_(code_cache::XMM_SAFE_IBL_COMMON_ENTRY_ROUTINE)));
#endif
    }


    /// Find or make an IBL entry routine.
    app_pc instruction_list_mangler::ibl_entry_for(
        operand target,
        instrumentation_policy policy
    ) throw() {
        app_pc routine(nullptr);
        const indirect_operand op(target, policy);

        // got it; we've previously created it.
        if(IBL_ENTRY_ROUTINE.load(op.as_uint, routine)) {
            return routine;
        }

        // don't have it; go encode it.
        instruction_list ibl;

        IF_USER( ibl.append(lea_(reg::rsp, reg::rsp[-REDZONE_SIZE])); )

        ibl.append(push_(reg::arg1));

        // omit a redundant move if possible; this also handles the case where
        // ret == target.
        if(dynamorio::REG_kind != target.kind
        || (reg::arg1.value.reg != target.value.reg)) {

            if(dynamorio::REL_ADDR_kind == target.kind
            || dynamorio::opnd_is_abs_addr(target)) {
                ibl.append(mov_imm_(
                    reg::arg1,
                    int64_(reinterpret_cast<uint64_t>(target.value.addr))));
                ibl.append(mov_ld_(reg::arg1, reg::arg1[0]));
            } else {
                ibl.append(mov_ld_(reg::arg1, target));
            }
        }

        ibl.append(push_(reg::ret));

        ibl_entry_tail(ibl, policy);

        routine = cpu->fragment_allocator.allocate_array<uint8_t>(
            ibl.encoded_size());

        ibl.encode(routine);

        // try to store the entry routine.
        if(!IBL_ENTRY_ROUTINE.store(op.as_uint, routine, HASH_KEEP_PREV_ENTRY)) {
            cpu->fragment_allocator.free_last();
            IBL_ENTRY_ROUTINE.load(op.as_uint, routine);
        }

        return routine;
    }


    /// Get the return address branch lookup entry for a particular policy
    /// and its properties.
    ///
    /// Note: we assume that flags are dead on return from a function.
    app_pc instruction_list_mangler::rbl_entry_for(
        instrumentation_policy target_policy,
        int num_bytes_to_pop
    ) throw() {

        app_pc routine(nullptr);
        const uint8_t policy_bits(target_policy.extension_bits());


        // got it; we've previously created it.
        if(RBL_ENTRY_ROUTINE.load(policy_bits, routine)) {
            return routine;
        }

        // TODO: retf (far returns)

        instruction_list rbl;
        if(num_bytes_to_pop) {
            rbl.append(lea_(reg::rsp, reg::rsp[num_bytes_to_pop]));
        }

#if !CONFIG_TRANSPARENT_RETURN_ADDRESSES || CONFIG_ENABLE_WRAPPERS

        rbl.append(push_(reg::rax));
        rbl.append(mov_ld_(reg::rax, reg::rsp[8]));

        // the call instruction will be 8 byte aligned, and will occupy ~5bytes.
        // the subsequent jmp needed to link the basic block (which ends with
        // the call) will also by 8 byte aligned, and will be padded to 8 bytes
        // (so that the call's basic block and the subsequent block can be
        // linked with the direct branch lookup/patch mechanism. Thus, we can
        // move the return address forward, then align it back to 8 bytes and
        // expect to find the magic value which begins the basic block meta
        // info.
        rbl.append(lea_(reg::rax, reg::ret[16]));
        rbl.append(and_(reg::rax, int32_(-8)));

        // now compare for the magic value
        rbl.append(mov_ld_(reg::eax, *reg::rax));
        rbl.append(sub_(reg::rax, int32_(basic_block_info::HEADER / 2)));
        instruction_list_handle fast(rbl.append(
            sub_(reg::rax, int32_(basic_block_info::HEADER / 2))));
        instruction_list_handle slow(rbl.append(label_()));

        // fast path: we're returning to the code cache
        fast = rbl.insert_after(fast, jnz_(instr_(*slow)));
        fast = rbl.insert_after(fast, pop_(reg::rax));
        fast = rbl.insert_after(fast, ret_());

        // slow path: emulate an indirect branch lookup.
#   if !GRANARY_IN_KERNEL
        slow = rbl.insert_after(slow, pop_(reg::rax));
        slow = rbl.insert_after(slow, lea_(reg::rsp, reg::rsp[-REDZONE_SIZE]));
        slow = rbl.insert_after(slow, push_(reg::rax));
#   endif
#else
#   if !GRANARY_IN_KERNEL
        instruction_list_handle slow(rbl.append(lea_(reg::rsp, reg::rsp[-REDZONE_SIZE])));
        slow = rbl.insert_after(slow, push_(reg::rax));
#   else
        instruction_list_handle slow(rbl.append(push_(reg::rax)));
#   endif
#endif

        slow = rbl.insert_after(slow,
            mov_ld_(reg::rax, reg::rsp[IF_USER_ELSE(REDZONE_SIZE + 8, 8)]));
        slow = rbl.insert_after(slow, mov_st_(reg::rsp[8], reg::rdi));
        slow = rbl.insert_after(slow, mov_st_(reg::rdi, reg::rax));

        ibl_entry_tail(rbl, target_policy);

        routine = cpu->fragment_allocator.allocate_array<uint8_t>(
            rbl.encoded_size());
        rbl.encode(routine);

        // try to store the entry routine.
        if(!RBL_ENTRY_ROUTINE.store(policy_bits, routine, HASH_KEEP_PREV_ENTRY)) {
            cpu->fragment_allocator.free_last();
            RBL_ENTRY_ROUTINE.load(policy_bits, routine);
        }

        return routine;
    }


    /// Injects N bytes of NOPs into an instuction list after a specific
    /// instruction.
    ///
    /// Note: this will propagate delay regions on NOPs to ensure that an
    ///       hot-patchable regions are never interrupted.
    void instruction_list_mangler::inject_mangled_nops(
        instruction_list &ls,
        instruction_list_handle in,
        unsigned num_nops
    ) throw() {
        instruction_list_handle original(in);
        instruction_list_handle invalid;

        for(; num_nops >= 3; num_nops -= 3) {
            in = ls.insert_after(in, nop3byte_());
            propagate_delay_region(original, invalid, in);
        }

        if(2 == num_nops) {
            in = ls.insert_after(in, nop2byte_());
            propagate_delay_region(original, invalid, in);

        } else if(num_nops) {
            ls.insert_after(in, nop1byte_());
            propagate_delay_region(original, invalid, in);
        }
    }


    /// Stage an 8-byte hot patch. This will encode the instruction `in` into
    /// the `stage` location (as if it were going to be placed at the `dest`
    /// location, and then encodes however many NOPs are needed to fill in 8
    /// bytes.
    void instruction_list_mangler::stage_8byte_hot_patch(
        instruction in,
        app_pc stage,
        app_pc dest
    ) throw() {
        instruction_list ls;
        ls.append(in);

        const unsigned size(in.encoded_size());
        if(size < 8) {
            inject_mangled_nops(ls, ls.first(), 8U - size);
        }

        ls.stage_encode(stage, dest);
    }


    /// Patch the code by regenerating the original instruction.
    ///
    /// Note: in kernel mode, this function executes with interrupts
    ///       disabled.
    ///
    /// Note: this function alters a return address in `context` so that
    ///       when the corresponding assembly patch function returns, it
    ///       will return to the instruction just patched.
    GRANARY_ENTRYPOINT
    template <instruction (*make_opcode)(dynamorio::opnd_t)>
    static void
    find_and_patch_direct_cti(direct_cti_patch_mcontext *context) throw() {

        // notify Granary that we're entering!
        cpu_state_handle cpu;
        thread_state_handle thread;
        granary::enter(cpu, thread);

        // get an address into the target basic block using two stage lookup.
        app_pc target_pc(
            cpu->code_cache.find(context->target_address.as_address));

        if(!target_pc) {
            target_pc = code_cache::find(cpu, thread, context->target_address);
        }

        // determine the address to patch; this decodes the *tail* of the patch
        // code in the basic block and looks for a CTI (assumed jmp) and takes
        // its target to be the instruction that must be patched.
        app_pc patch_address(nullptr);
        for(app_pc ret_pc(context->return_address_into_patch_tail); ;) {
            instruction maybe_jmp(instruction::decode(&ret_pc));
            if(maybe_jmp.is_cti()) {
                ASSERT(dynamorio::OP_jmp == maybe_jmp.op_code());
                patch_address = maybe_jmp.cti_target().value.pc;
                ASSERT(0 == (reinterpret_cast<uint64_t>(patch_address) % 8));
                break;
            }
        }

        // create the patch code
        uint64_t staged_code_(0ULL);
        app_pc staged_code(reinterpret_cast<app_pc>(&staged_code_));

        // make the cti instruction, and try to widen it if necessary.
        instruction cti(make_opcode(pc_(target_pc)));
        cti.widen_if_cti();

        instruction_list_mangler::stage_8byte_hot_patch(
            cti,
            staged_code,
            patch_address);

        // apply the patch
        granary_atomic_write8(
            staged_code_,
            reinterpret_cast<uint64_t *>(patch_address));
    }


    /// Make a direct patch function that is specific to a particular opcode.
    template <instruction (*make_opcode)(dynamorio::opnd_t)>
    static app_pc make_direct_cti_patch_func(
        void (*template_func)(void)
    ) throw() {

        instruction_list ls;
        app_pc start_pc(reinterpret_cast<app_pc>(template_func));

        for(;;) {
            instruction in(instruction::decode(&start_pc));
            if(in.is_call()) {
                ls.append(mov_imm_(reg::rax, int64_(reinterpret_cast<int64_t>(
                    find_and_patch_direct_cti<make_opcode>))));
            }

            ls.append(in);

            if(dynamorio::OP_ret == in.op_code()) {
                break;
            }
        }

        app_pc dest_pc(global_state::FRAGMENT_ALLOCATOR.allocate_array<uint8_t>(
            ls.encoded_size()));

        ls.encode(dest_pc);

        // free all transiently allocated blocks so that we don't blow up
        // the transient memory requirements when generating code cache
        // templates
        cpu_state_handle cpu;
        cpu->transient_allocator.free_all();

        return dest_pc;
    }


    STATIC_INITIALISE({

#if CONFIG_TRACK_XMM_REGS
        FOR_EACH_DIRECT_JUMP(DEFINE_DIRECT_JUMP_RESOLVER);
        DEFINE_DIRECT_JUMP_RESOLVER(call, 5)
#endif

        FOR_EACH_DIRECT_JUMP(DEFINE_XMM_SAFE_DIRECT_JUMP_RESOLVER);
        DEFINE_XMM_SAFE_DIRECT_JUMP_RESOLVER(call, 5)
    });


    /// Look up and return the assembly patch (see asm/direct_branch.asm)
    /// function needed to patch an instruction that originally had opcode as
    /// `opcode`.
#if CONFIG_TRACK_XMM_REGS
    static app_pc get_direct_cti_patch_func(int opcode) throw() {
        switch(opcode) {
        CASE_DIRECT_JUMP_MANGLER(call, 5)
        FOR_EACH_DIRECT_JUMP(CASE_DIRECT_JUMP_MANGLER);
        default: return nullptr;
        }
    }
#endif
    static app_pc get_xmm_safe_direct_cti_patch_func(int opcode) throw() {
        switch(opcode) {
        CASE_XMM_SAFE_DIRECT_JUMP_MANGLER(call, 5)
        FOR_EACH_DIRECT_JUMP(CASE_XMM_SAFE_DIRECT_JUMP_MANGLER);
        default: return nullptr;
        }
    }


    /// Get or build the direct branch lookup (DBL) routine for some jump/call
    /// target.
    app_pc instruction_list_mangler::dbl_entry_for(
        instrumentation_policy target_policy,
        instruction_list_handle in,
        mangled_address am
    ) throw() {

        direct_operand op;
        const unsigned op_code(in->op_code());
        op.as_address = am.as_address;
        op.as_op.op_code = op_code;

        app_pc routine(nullptr);
        if(DBL_ENTRY_ROUTINE.load(op.as_address, routine)) {
            return routine;
        }

        // add in the patch code, change the initial behaviour of the
        // instruction, and mark it has hot patchable so it is nicely aligned.
#if CONFIG_TRACK_XMM_REGS
        app_pc patcher_for_opcode(nullptr);
        if(target_policy.is_in_xmm_context()) {
            patcher_for_opcode = get_xmm_safe_direct_cti_patch_func(op_code);
        } else {
            patcher_for_opcode = get_direct_cti_patch_func(op_code);
        }
#else
        (void) target_policy;
        app_pc patcher_for_opcode(get_xmm_safe_direct_cti_patch_func(
            in->op_code()));
#endif

        instruction_list ls;

        // TODO: these patch stubs can be reference counted so that they
        //       can be reclaimed (especially since every patch stub will
        //       have the same size!).

        // store the policy-mangled target on the stack;
        //
        // Note: enough space to hold the mangled address will already have
        //       been made available below the return address.
        ls.append(push_(reg::rax));
        ls.append(mov_imm_(reg::rax, int64_(am.as_int)));
        ls.append(mov_st_(reg::rsp[16], reg::rax));
        ls.append(pop_(reg::rax)); // restore

        // tail-call out to the patcher/mangler.
        ls.append(mangled(jmp_(pc_(patcher_for_opcode))));

        routine = cpu->fragment_allocator.allocate_array<uint8_t>(
            ls.encoded_size());

        ls.encode(routine);

        if(!DBL_ENTRY_ROUTINE.store(op.as_address, routine, HASH_KEEP_PREV_ENTRY)) {
            cpu->fragment_allocator.free_last();
            DBL_ENTRY_ROUTINE.load(op.as_address, routine);
        }

        return routine;
    }


    /// Make a direct CTI patch stub. This is used both for mangling direct CTIs
    /// and for emulating policy inheritance/scope when transparent return
    /// addresses are used.
    void instruction_list_mangler::direct_cti_patch_stub(
        instruction_list &patch_ls,
        instruction_list_handle patch,
        instruction_list_handle patched_in,
        app_pc dbl_routine
    ) throw() {
        const bool emit_redzone_buffer(
            IF_USER_ELSE(!patched_in->is_call(), false));

        // shift the stack pointer appropriately; in user space, this deals with
        // the red zone.
        if(emit_redzone_buffer) {
            patch = patch_ls.insert_after(patch,
                lea_(reg::rsp, reg::rsp[-(REDZONE_SIZE + 8)]));
        } else {
            patch = patch_ls.insert_after(patch, lea_(reg::rsp, reg::rsp[-8]));
        }

        patch = patch_ls.insert_after(patch, mangled(call_(pc_(dbl_routine))));

        // deal with the red zone and the policy-mangled  address left on
        // the stack.
        if(emit_redzone_buffer) {
            patch = patch_ls.insert_after(patch,
                lea_(reg::rsp, reg::rsp[REDZONE_SIZE + 8]));
        } else {
            patch = patch_ls.insert_after(patch, lea_(reg::rsp, reg::rsp[8]));
        }

        // the address to be mangled is implicitly encoded in the target of this
        // jmp instruction, which will later be decoded by the direct cti
        // patch function. There are two reasons for this approach of jumping
        // around:
        //      i)  Doesn't screw around with the return address predictor.
        //      ii) Works with user space red zones.
        patch_ls.insert_after(patch, mangled(jmp_(instr_(*patched_in))));
    }

#if CONFIG_TRANSPARENT_RETURN_ADDRESSES
    /// Emulate the push of a function call's return address onto the stack.
    /// This will also pre-populate the code cache so that correct policy
    /// resolution is emulated for transparent return addresses.
    void instruction_list_mangler::emulate_call_ret_addr(
        instruction_list_handle in,
        instrumentation_policy target_policy
    ) throw() {
        app_pc return_address(in->pc() + in->encoded_size());
        ls->insert_before(in, lea_(reg::rsp, reg::rsp[-8]));
        ls->insert_before(in, push_(reg::rax));
        ls->insert_before(in,
            mov_imm_(reg::rax,
                int64_(reinterpret_cast<uint64_t>(return_address))));
        ls->insert_before(in, mov_st_(reg::rsp[8], reg::rax));
        ls->insert_before(in, pop_(reg::rax));

        // pre-fill in the code cache (in a slightly backwards way) to handle
        // getting back into the proper policy on return from the function.
        mangled_address am(return_address, policy);
        instruction_list trampoline;
        instruction_list_handle conn(trampoline.append(
            jmp_(pc_(return_address))));
        app_pc dbl_routine(dbl_entry_for(policy, conn, am));
        instruction_list_handle patch(trampoline.append(label_()));

        direct_cti_patch_stub(trampoline, patch, conn, dbl_routine);

        // because `conn` is the first instruction in the routine, it will
        // inherit the 16-byte alignment from the allocator. Thus, we need to
        // emulate the 8 bytes of padding needed to make `conn` look like a
        // hot-patchable instruction.
        inject_mangled_nops(trampoline, conn, 8 - conn->encoded_size());

        conn->set_cti_target(instr_(*patch));

        // encode it
        app_pc return_routine(global_state::FRAGMENT_ALLOCATOR. \
            allocate_array<uint8_t>(trampoline.encoded_size()));
        trampoline.encode(return_routine);

        // add the ibl exit routine for this return address trampoline to the
        // code cache for each possible property combination for the targeted
        // policy. This means that when we are in the target policy, we can
        // force control to return to the source policy, regardless of the
        // policy properties discovered when translating code targeted by a
        // call.
        target_policy = target_policy.indirect_cti_policy();
        target_policy.clear_properties();

        app_pc ibl_routine(code_cache::ibl_exit_for(return_routine));
        for(; !!target_policy; target_policy = target_policy.next()) {
            mangled_address ram(return_address, target_policy);
            code_cache::add(ram.as_address, ibl_routine);
        }
    }
#endif


    /// Add a direct branch slot; this is a sort of "formula" for direct
    /// branches that pushes two addresses and then jmps to an actual
    /// direct branch handler.
    void instruction_list_mangler::mangle_direct_cti(
        instruction_list_handle in,
        operand target
    ) throw() {
#if CONFIG_ENABLE_WRAPPERS
        app_pc detach_target_pc(target.value.pc);

        // if this is a detach point then replace the target address with the
        // detach address.
        detach_target_pc = find_detach_target(detach_target_pc);
        if(nullptr != detach_target_pc) {

            if(is_far_away(estimator_pc, detach_target_pc)) {
                app_pc *slot = cpu->fragment_allocator.allocate<app_pc>();
                *slot = detach_target_pc;

                // regardless of return address transparency, a direct call to
                // a detach target *always* needs to be a call so that control
                // returns to the code cache.
                if(in->is_call()) {
                    *in = mangled(call_ind_(absmem_(slot, dynamorio::OPSZ_8)));
                } else {
                    *in = mangled(jmp_ind_(absmem_(slot, dynamorio::OPSZ_8)));
                }
            } else {
                in->set_cti_target(pc_(detach_target_pc));
                in->set_mangled();
            }

            return;
        }
#endif

        // policy inheritance for the target basic block.
        instrumentation_policy target_policy(in->policy());
        if(!target_policy) {
            target_policy = policy;
        }
        target_policy.inherit_properties(policy);

#if CONFIG_TRANSPARENT_RETURN_ADDRESSES
        // when emulating functions (for transparent return addresses), we
        // convert the call to a jmp here so that the dbl picks up OP_jmp as
        // the instruction instead of OP_call.
        //
        // Note: this isn't a detach point, or else we would have detected it
        //       above, so regardless of whether or not wrappers are enabled,
        //       we will push on the return address.
        if(in->is_call()) {
            emulate_call_ret_addr(in, target_policy);
            *in = jmp_(target);
        }
#endif

        const unsigned old_size(in->encoded_size());

        // set the policy-fied target
        mangled_address am(target, target_policy.base_policy());
        instruction_list_handle patch(ls->prepend(label_()));
        app_pc dbl_routine(dbl_entry_for(target_policy, in, am));

        direct_cti_patch_stub(*ls, patch, in, dbl_routine);
        *in = patchable(mangled(jmp_(instr_(*patch))));

        const unsigned new_size(in->encoded_size());

        IF_DEBUG(old_size > 8, FAULT)
        IF_DEBUG(new_size > 8, FAULT)

        (void) new_size;
    }


    /// Mangle an indirect control transfer instruction.
    void instruction_list_mangler::mangle_indirect_cti(
        instruction_list_handle in,
        operand target
    ) throw() {

        instrumentation_policy target_policy(in->policy());
        if(!target_policy) {
            target_policy = policy;
        }

        // convert the policy into an IBL policy.
        instrumentation_policy target_policy_ibl(policy.indirect_cti_policy());
        target_policy_ibl.inherit_properties(policy);

        // mark indirect calls as hot-patchable (even though they won't be
        // patched) so that they are automatically aligned to an 8-byte boundary
        // with associated alignment nops after them. This is so that the RBL
        // can treat direct and indirect calls uniformly.
        //
        // TODO: in future, it might be worth hot-patching the call if we
        //       can make good predictions.
        if(in->is_call()) {
#if CONFIG_TRANSPARENT_RETURN_ADDRESSES && !CONFIG_ENABLE_WRAPPERS
            emulate_call_ret_addr(in, target_policy);
            *in = mangled(
                jmp_(pc_(ibl_entry_for(target, target_policy_ibl))));
#else
            *in = patchable(mangled(
                call_(pc_(ibl_entry_for(target, target_policy_ibl)))));
#endif

        } else if(in->is_return()) {
            // TODO: handle RETn/RETf with a byte count.
            *in = mangled(jmp_(pc_(rbl_entry_for(target_policy_ibl, 0))));

        } else {
            *in = mangled(jmp_(pc_(ibl_entry_for(target, target_policy_ibl))));
        }
    }


    void instruction_list_mangler::mangle_cti(
        instruction_list_handle in
    ) throw() {
        if(in->is_return()) {
            mangle_indirect_cti(in, operand());
        } else {
            operand target(in->cti_target());
            if(dynamorio::opnd_is_pc(target)) {
                mangle_direct_cti(in, target);
            } else if(!dynamorio::opnd_is_instr(target)) {
                mangle_indirect_cti(in, target);
            }
        }
    }


    void instruction_list_mangler::mangle_cli(instruction_list_handle in) throw() {
        (void) in;
    }


    void instruction_list_mangler::mangle_sti(instruction_list_handle in) throw() {

        (void) in;
    }


    void instruction_list_mangler::mangle_lea(
        instruction_list_handle in
    ) throw() {
        if(dynamorio::REL_ADDR_kind != in->instr.u.o.src0.kind) {
            return;
        }

        // it's an LEA to a far address; convert to a 64-bit move.
        app_pc target_pc(in->instr.u.o.src0.value.pc);
        if(is_far_away(estimator_pc, target_pc)) {
            *in = mov_imm_(
                in->instr.u.o.dsts[0],
                int64_(reinterpret_cast<uint64_t>(target_pc)));
        }
    }


    /// Propagate a delay region across mangling. If we have mangled a single
    /// instruction that begins/ends a delay region into a sequence of
    /// instructions then we need to change which instruction logically begins
    /// the interrupt delay region's begin/end bounds.
    void instruction_list_mangler::propagate_delay_region(
        instruction_list_handle in,
        instruction_list_handle first,
        instruction_list_handle last
    ) throw() {
        if(in->begins_delay_region() && first.is_valid()) {
            // TODO
        }

        if(in->ends_delay_region() && last.is_valid()) {
            // TODO
        }
    }


#if CONFIG_TRANSLATE_FAR_ADDRESSES


    /// Find a far memory operand and its size. If we've already found one in
    /// this instruction then don't repeat the check.
    static void find_far_operand(
        operand_ref op,
        app_pc &estimator_pc,
        operand &far_op,
        bool &has_far_op
    ) throw() {
        if(has_far_op || dynamorio::REL_ADDR_kind != op->kind) {
            return;
        }

        if(!is_far_away(estimator_pc, op->value.addr)) {
            return;
        }

        // if the operand is too far away then we will need to indirectly load
        // the operand through its absolute address.
        has_far_op = true;
        far_op = op;
    }


    /// Update a far operand in place.
    static void update_far_operand(operand_ref op, operand &new_op) throw() {
        if(dynamorio::REL_ADDR_kind != op->kind
        && dynamorio::PC_kind != op->kind) {
            return;
        }

        op = new_op; // update the op in place
    }


    /// Mangle a `push addr`, where `addr` is unreachable. A nice convenience
    /// in user space is that we don't need to worry about the redzone because
    /// `push` is operating on the stack.
    void instruction_list_mangler::mangle_far_memory_push(
        instruction_list_handle in,
        bool first_reg_is_dead,
        dynamorio::reg_id_t dead_reg_id,
        dynamorio::reg_id_t spill_reg_id,
        uint64_t addr
    ) throw() {
        instruction_list_handle first_in;
        instruction_list_handle last_in;

        if(first_reg_is_dead) {
            const operand reg_addr(dead_reg_id);
            first_in = ls->insert_before(in, mov_imm_(reg_addr, int64_(addr)));
            *in = push_(*reg_addr);

        } else {
            const operand reg_addr(spill_reg_id);
            const operand reg_value(spill_reg_id);
            first_in = ls->insert_before(in, lea_(reg::rsp, reg::rsp[-8]));
            ls->insert_before(in, push_(reg_addr));
            ls->insert_before(in, mov_imm_(reg_addr, int64_(addr)));
            ls->insert_before(in, mov_ld_(reg_value, *reg_addr));

            *in = mov_st_(reg::rsp[8], reg_value);

            last_in = ls->insert_after(in, pop_(reg_addr));
        }

        propagate_delay_region(in, first_in, last_in);
    }


    /// Mangle a `pop addr`, where `addr` is unreachable. A nice convenience
    /// in user space is that we don't need to worry about the redzone because
    /// `pop` is operating on the stack.
    void instruction_list_mangler::mangle_far_memory_pop(
        instruction_list_handle in,
        bool first_reg_is_dead,
        dynamorio::reg_id_t dead_reg_id,
        dynamorio::reg_id_t spill_reg_id,
        uint64_t addr
    ) throw() {
        instruction_list_handle first_in;
        instruction_list_handle last_in;

        if(first_reg_is_dead) {
            const operand reg_value(dead_reg_id);
            const operand reg_addr(spill_reg_id);

            first_in = ls->insert_before(in, pop_(reg_value));
            ls->insert_before(in, push_(reg_addr));
            ls->insert_before(in, mov_imm_(reg_addr, int64_(addr)));

            *in = mov_st_(*reg_addr, reg_value);

            last_in = ls->insert_after(in, pop_(reg_addr));

        } else {
            const operand reg_value(dead_reg_id);
            const operand reg_addr(spill_reg_id);

            first_in = ls->insert_before(in, push_(reg_value));
            ls->insert_before(in, push_(reg_addr));
            ls->insert_before(in, mov_imm_(reg_addr, int64_(addr)));
            ls->insert_before(in, mov_ld_(reg_value, reg::rsp[16]));

            *in = mov_st_(*reg_addr, reg_value);

            ls->insert_after(in, pop_(reg_addr));
            ls->insert_after(in, pop_(reg_value));
            last_in = ls->insert_after(in, lea_(reg::rsp, reg::rsp[8]));
        }

        propagate_delay_region(in, first_in, last_in);
    }


    /// Mangle %rip-relative memory operands into absolute memory operands
    /// (indirectly through a spill register) in user space. This checks to see
    /// if %rip-relative operands and > 4gb away and converts them to a
    /// different form. This is a "two step" process, in that a DR instruction
    /// might have multiple memory operands (e.g. inc, add), and some of them
    /// must all be equivalent.
    ///
    /// We assume it's always legal to convert %rip-relative into a base/disp
    /// type operand (of the same size).
    void instruction_list_mangler::mangle_far_memory_refs(
        instruction_list_handle in
    ) throw() {

        bool has_far_op(false);
        operand far_op;

        in->for_each_operand(
            find_far_operand, estimator_pc, far_op, has_far_op);

        if(!has_far_op) {
            return;
        }

        const uint64_t addr(reinterpret_cast<uint64_t>(far_op.value.pc));

        register_manager rm;
        rm.revive_all();

        // peephole optimisation; ideally will allow us to avoid spilling a
        // register by finding a dead register.
        instruction_list_handle next_in(in.next());
        if(next_in.is_valid()) {
            rm.visit(*next_in);
        }

        rm.revive(*in);
        dynamorio::reg_id_t dead_reg_id(rm.get_zombie());

        rm.kill_all();
        rm.revive(*in);
        rm.kill(dead_reg_id);
        dynamorio::reg_id_t spill_reg_id(rm.get_zombie());

        // overload the dead register to be a second spill register; needed for
        // `pop addr`.
        bool first_reg_is_dead(!!dead_reg_id);
        if(!first_reg_is_dead) {
            dead_reg_id = rm.get_zombie();
        }

        // push and pop need to be handled specially because they operate on
        // the stack, so the usual save/restore is not legal.
        switch(in->op_code()) {
        case dynamorio::OP_push:
            return mangle_far_memory_push(
                in, first_reg_is_dead, dead_reg_id, spill_reg_id, addr);
        case dynamorio::OP_pop:
            return mangle_far_memory_pop(
                in, first_reg_is_dead, dead_reg_id, spill_reg_id, addr);
        default: break;
        }

        operand used_reg;
        instruction_list_handle first_in;
        instruction_list_handle last_in;

        // use a dead register
        if(dead_reg_id) {
            used_reg = dead_reg_id;
            first_in = ls->insert_before(in, mov_imm_(used_reg, int64_(addr)));

        // spill a register, then use that register to load the value from
        // memory. Note: the ordering of managing `first_in` is intentional and
        // done for delay propagation.
        } else {

            used_reg = spill_reg_id;
            first_in = ls->insert_before(in, push_(used_reg));
            IF_USER( first_in = ls->insert_before(first_in,
                lea_(reg::rsp, reg::rsp[-REDZONE_SIZE])); )
            ls->insert_before(in, mov_imm_(used_reg, int64_(addr)));
            last_in = ls->insert_after(in, pop_(used_reg));
            IF_USER( last_in = ls->insert_after(in,
                lea_(reg::rsp, reg::rsp[REDZONE_SIZE])); )
        }

        operand_base_disp new_op_(*used_reg);
        new_op_.size = far_op.size;

        operand new_op(new_op_);
        in->for_each_operand(update_far_operand, new_op);

        // propagate interrupt delaying.
        propagate_delay_region(in, first_in, last_in);
    }
#endif


    /// Convert non-instrumented instructions that change control-flow into
    /// mangled instructions.
    void instruction_list_mangler::mangle(instruction_list &ls_) throw() {

        ls = &ls_;

        instruction_list_handle in(ls->first());
        instruction_list_handle next_in;

        // go mangle instructions; note: indirect CTI mangling happens here.
        for(unsigned i(0), max(ls->length()); i < max; ++i, in = next_in) {
            const bool is_mangled(in->is_mangled());
            const bool can_skip(nullptr == in->pc() || is_mangled);
            next_in = in.next();

            // native instruction, we might need to mangle it.
            if(in->is_cti()) {
                if(!is_mangled) {
                    mangle_cti(in);
                }

            // clear interrupt
            } else if(dynamorio::OP_cli == (*in)->opcode) {
                if(can_skip) {
                    continue;
                }
                mangle_cli(in);

            // restore interrupt
            } else if(dynamorio::OP_sti == (*in)->opcode) {
                if(can_skip) {
                    continue;
                }
                mangle_sti(in);

#if CONFIG_TRANSLATE_FAR_ADDRESSES
            // look for cases where an lea loads from a memory address that is
            // too far away and fix it.
            } else if(dynamorio::OP_lea == (*in)->opcode) {
               mangle_lea(in);

            // look for uses of relative addresses in operands that are no
            // longer reachable with %rip-relative encoding, and convert to a
            // use of an absolute address.
            } else {
                mangle_far_memory_refs(in);
#endif
            }
        }


        // do a second-pass over all instructions, looking for any hot-patchable
        // instructions, and aligning them nicely.
        //
        // Extra alignment/etc needs to be done here instead of in encoding
        // because of how basic block allocation works.
        unsigned align(0);
        instruction_list_handle prev_in;
        in = ls->first();

        for(unsigned i(0), max(ls->length()); i < max; ++i, in = next_in) {

            next_in = in.next();
            const bool is_hot_patchable(in->is_patchable());

            // x86-64 guaranteed quadword atomic writes so long as the memory
            // location is aligned on an 8-byte boundary; we will assume that
            // we are never patching an instruction longer than 8 bytes
            if(is_hot_patchable) {
                uint64_t forward_align(ALIGN_TO(align, 8));
                inject_mangled_nops(*ls, prev_in, forward_align);
                align += forward_align;
            }

            prev_in = in;
            align += in->encoded_size();

            // make sure that the instruction is the only "useful" one in it's
            // 8-byte block
            if(is_hot_patchable) {
                uint64_t forward_align(ALIGN_TO(align, 8));
                inject_mangled_nops(ls_, prev_in, forward_align);
                align += forward_align;
            }
        }
    }


    /// Constructor
    instruction_list_mangler::instruction_list_mangler(
        cpu_state_handle &cpu_,
        thread_state_handle &thread_,
        instrumentation_policy &policy_
    ) throw()
        : cpu(cpu_)
        , thread(thread_)
        , policy(policy_)
        , ls(nullptr)
        , estimator_pc(cpu->fragment_allocator.allocate_staged<uint8_t>())
    { }

}

