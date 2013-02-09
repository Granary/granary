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
            int32_t displacement;
            uint8_t base_reg;
            uint8_t index_reg;
            uint8_t scale;
            uint8_t policy_bits;
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


    /// The operand-specific IBL entry routines.
    static shared_hash_table<uint64_t, app_pc> IBL_ENTRY_ROUTINE;


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

        // save flags / disable interrupts
#if GRANARY_IN_KERNEL
        ibl.append(pushf_());
        ibl.append(cli_());
#else
        //ibl.append(pushf_());
        ibl.append(lahf_());
        ibl.append(push_(reg::rax)); // because rax == ret
#endif

        // add in the policy to the address to be looked up.
        ibl.append(shl_(reg::arg1, int8_(mangled_address::NUM_MANGLED_BITS)));
        ibl.append(or_(reg::arg1, int8_(op.as_operand.policy_bits)));

        // go to the common ibl lookup routine
#if CONFIG_TRACK_XMM_REGS
        app_pc ibl_routine(code_cache::IBL_COMMON_ENTRY_ROUTINE);
        if(policy.is_in_xmm_context()) {
            ibl_routine = code_cache::XMM_SAFE_IBL_COMMON_ENTRY_ROUTINE;
        }
        ibl.append(jmp_(pc_(ibl_routine)));
#else
        ibl.append(jmp_(pc_(code_cache::XMM_SAFE_IBL_COMMON_ENTRY_ROUTINE)));
#endif
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


    /// Injects N bytes of NOPs into an instuction list after
    /// a specific instruction.
    static void inject_mangled_nops(
        instruction_list &ls,
        instruction_list_handle in,
        unsigned num_nops
    ) throw() {
        for(; num_nops >= 3; num_nops -= 3) {
            in = ls.insert_after(in, nop3byte_());
        }

        if(2 == num_nops) {
            in = ls.insert_after(in, nop2byte_());
            num_nops -= 2;
        } else if(num_nops) {
            ls.insert_after(in, nop1byte_());
        }
    }


    /// Stage an 8-byte hot patch. This will encode the instruction `in` into
    /// the `stage` location (as if it were going to be placed at the `dest`
    /// location, and then encodes however many NOPs are needed to fill in 8
    /// bytes.
    static void stage_8byte_hot_patch(
        instruction in,
        app_pc stage,
        app_pc dest
    ) {
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
    template <
        instruction (*make_opcode)(dynamorio::opnd_t)
    >
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

        stage_8byte_hot_patch(
            cti,
            staged_code,
            patch_address);

        // apply the patch
        granary_atomic_write8(
            staged_code_,
            reinterpret_cast<uint64_t *>(patch_address));
    }


    /// Make a direct patch function that is specific to a particular opcode.
    template <
        instruction (*make_opcode)(dynamorio::opnd_t)
    >
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


    /// Represents a direct branch lookup operand that is specific to a
    /// mangled target address and to an opcode.
    union direct_operand {
        app_pc as_address;
        struct {
            uint64_t mangled_address:56; // low
            uint64_t op_code:8; // high
        } __attribute__((packed)) as_op;
    };


    /// Hash table for managing direct branch lookup stubs.
    static shared_hash_table<app_pc, app_pc> DIRECT_BRANCH_PATCH_STUB;


    /// Get or build the direct branch lookup (DBL) routine for some jump/call
    /// target.
    app_pc instruction_list_mangler::dbl_entry_for(
        instrumentation_policy target_policy,
        instruction_list_handle in,
        mangled_address am
    ) throw() {

        direct_operand op;
        op.as_address = am.as_address;
        op.as_op.op_code = in->op_code();

        app_pc routine(nullptr);
        if(DIRECT_BRANCH_PATCH_STUB.load(op.as_address, routine)) {
            return routine;
        }

        // add in the patch code, change the initial behaviour of the
        // instruction, and mark it has hot patchable so it is nicely aligned.
#if CONFIG_TRACK_XMM_REGS
        app_pc patcher_for_opcode(nullptr);
        if(target_policy.is_in_xmm_context()) {
            patcher_for_opcode = get_xmm_safe_direct_cti_patch_func(
                in->op_code());
        } else {
            patcher_for_opcode = get_direct_cti_patch_func(in->op_code());
        }
#else
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

        if(!DIRECT_BRANCH_PATCH_STUB.store(op.as_address, routine, HASH_KEEP_PREV_ENTRY)) {
            cpu->fragment_allocator.free_last();
            DIRECT_BRANCH_PATCH_STUB.load(op.as_address, routine);
        }

        return routine;
    }


    /// Add a direct branch slot; this is a sort of "formula" for direct
    /// branches that pushes two addresses and then jmps to an actual
    /// direct branch handler.
    void instruction_list_mangler::mangle_direct_cti(
        instruction_list_handle in,
        operand target
    ) throw() {

        // if this is a detach point then replace the target address with the
        // detach address.
        app_pc detach_target_pc(find_detach_target(target));
        if(nullptr != detach_target_pc) {

            // TODO: won't work in user space
            in->set_cti_target(pc_(detach_target_pc));
            in->set_mangled();
            return;
        }

        const unsigned old_size(in->encoded_size());

        instrumentation_policy target_policy(in->policy());
        if(!target_policy) {
            target_policy = policy;
        }
        target_policy.inherit_properties(policy);

        // set the policy-fied target
        mangled_address am(target, target_policy.base_policy());
        instruction_list_handle patch(ls->append(label_()));
        app_pc dbl_routine(dbl_entry_for(target_policy, in, am));
        const bool emit_redzone_buffer(IF_USER_ELSE(!in->is_call(), false));

        *in = patchable(mangled(jmp_(instr_(*patch))));

        // shift the stack pointer appropriately; in user space, this deals with
        // the red zone.
        if(emit_redzone_buffer) {
            ls->append(lea_(reg::rsp, reg::rsp[-(REDZONE_SIZE + 8)]));
        } else {
            ls->append(lea_(reg::rsp, reg::rsp[-8]));
        }

        ls->append(mangled(call_(pc_(dbl_routine))));

        // deal with the red zone and the policy-mangled  address left on
        // the stack.
        if(emit_redzone_buffer) {
            ls->append(lea_(reg::rsp, reg::rsp[REDZONE_SIZE + 8]));
        } else {
            ls->append(lea_(reg::rsp, reg::rsp[8]));
        }

        // the address to be mangled is implicitly encoded in the target of this
        // jmp instruction, which will later be decoded by the direct cti
        // patch function. There are two reasons for this approach of jumping
        // around:
        //      i)  Doesn't screw around with the return address predictor.
        //      ii) Works with user space red zones.
        ls->append(mangled(jmp_(instr_(*in))));

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

        if(in->is_call()) {
            *in = mangled(call_(pc_(ibl_entry_for(target, target_policy_ibl))));
        } else {
            *in = mangled(jmp_(pc_(ibl_entry_for(target, target_policy_ibl))));
        }
    }


    void instruction_list_mangler::mangle_cti(
        instruction_list_handle in
    ) throw() {
        operand target(in->cti_target());

        if(dynamorio::opnd_is_pc(target)) {
            mangle_direct_cti(in, target);
        } else if(!dynamorio::opnd_is_instr(target)) {
            mangle_indirect_cti(in, target);
        }
    }


    void instruction_list_mangler::mangle_return(
        instruction_list_handle in
    ) throw() {
        (void) in;
    }


    void instruction_list_mangler::mangle_cli(instruction_list_handle in) throw() {
        (void) in;
    }


    void instruction_list_mangler::mangle_sti(instruction_list_handle in) throw() {

        (void) in;
    }


    void instruction_list_mangler::mangle_lea(
        instruction_list_handle in,
        app_pc estimator_pc
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
        instruction_list_handle in,
        app_pc estimator_pc
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

        // used to estimate if an address is too far away from the code cache
        // to use relative addressing.
        uint8_t *estimator_pc(
            cpu->fragment_allocator.allocate_staged<uint8_t>());

        // go mangle instructions; note: indirect CTI mangling happens here.
        for(unsigned i(0), max(ls->length()); i < max; ++i, in = next_in) {
            const bool is_mangled(in->is_mangled());
            const bool can_skip(nullptr == in->pc() || is_mangled);
            next_in = in.next();

            // native instruction, we might need to mangle it.
            if(in->is_cti()) {

                if(!is_mangled) {

                    if(dynamorio::instr_is_return(*in)) {
                        mangle_return(in);

                    // call, jump, Jcc
                    } else {
                        mangle_cti(in);
                    }
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
               mangle_lea(in, estimator_pc);

            // look for uses of relative addresses in operands that are no
            // longer reachable with %rip-relative encoding, and convert to a
            // use of an absolute address.
            } else {
                mangle_far_memory_refs(in, estimator_pc);
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
    { }

}

