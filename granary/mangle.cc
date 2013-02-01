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


/// Used to forward-declare the assembly function patches. These patch functions
/// eventually call the templates.
#define CASE_DIRECT_JUMP_MANGLER(opcode, size) \
    case dynamorio::OP_ ## opcode: return direct_branch_ ## opcode;


/// Used to forward-declare the assembly funcion patches. These patch functions
/// eventually call the templates.
#define DECLARE_DIRECT_JUMP_MANGLER(opcode, size) \
    static app_pc direct_branch_ ## opcode;


extern "C" {
    extern void granary_asm_direct_branch_template(void);
}


namespace granary {

    namespace {

        /// Specific instruction manglers used for branch lookup.
        DECLARE_DIRECT_JUMP_MANGLER(call, 5)
        FOR_EACH_DIRECT_JUMP(DECLARE_DIRECT_JUMP_MANGLER)
    }

    enum {
        _4GB = 4294967296ULL
    };


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

        /// The target address of a jump, including the policy to use when
        /// translating the target basic block.
        mangled_address target_address;

        /// The return address *immediately* following the mangled instruction.
        uint64_t return_address_after_mangled_call;
    };


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
            uint8_t policy_id;
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
            if(dynamorio::opnd_is_abs_addr(op)) {
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

            as_operand.policy_id = policy.policy_id;
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


        ibl.append(push_(reg::arg1));

        // omit a redundant move if possible; this also handles the case where
        // ret == target.
        if(dynamorio::REG_kind != target.kind
        || (reg::arg1.value.reg != target.value.reg)) {
            ibl.append(mov_ld_(reg::arg1, target));
        }

        ibl.append(push_(reg::ret));

        // save flags / disable interrupts
#if GRANARY_IN_KERNEL
            ibl.append(pushf_()));
            ibl.append(cli_());
#else
        ibl.append(lahf_());
        ibl.append(push_(reg::rax)); // because rax == ret
#endif

        // add in the policy to the address to be looked up.
        ibl.append(shl_(reg::arg1, int8_(mangled_address::POLICY_NUM_BITS)));
        ibl.append(or_(reg::arg1, int8_(op.as_operand.policy_id)));

        // go to the common ibl lookup routine
        ibl.append(jmp_(pc_(code_cache::IBL_COMMON_ENTRY_ROUTINE)));

        app_pc ibl_code(cpu->fragment_allocator.allocate_array<uint8_t>(
            ibl.encoded_size()));

        ibl.encode(ibl_code);

        // try to store the entry routine.
        if(!IBL_ENTRY_ROUTINE.store(op.as_uint, ibl_code)) {
            cpu->fragment_allocator.free_last();
            IBL_ENTRY_ROUTINE.load(op.as_uint, ibl_code);
        }

        return ibl_code;
    }


    /// Injects N bytes of NOPs into an instuction list after
    /// a specific instruction.
    static void inject_mangled_nops(
        instruction_list &ls,
        instruction_list_handle in,
        unsigned num_nops
    ) throw() {
        //printf("num nops = %u\n", num_nops);
        //instruction nop;

        for(; num_nops >= 3; num_nops -= 3) {
            in = ls.insert_after(in, nop3byte_());
        }

        if(2 == num_nops) {
            in = ls.insert_after(in, nop2byte_());
            num_nops -= 2;
        }

        if(num_nops) {
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
    static direct_cti_patch_mcontext *
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

        // determine the address to patch; this changes the return address in
        // the machine context to point back to the patch address so that we
        // are guaranteed that we resume in the code cache, we execute the
        // intended instruction.
        uint64_t patch_address(context->return_address_after_mangled_call);
        unsigned offset(patch_address % 8);
        if(0 == offset) {
            context->return_address_after_mangled_call -= 8U;
            patch_address -= 8U;
        } else {
            context->return_address_after_mangled_call -= offset;
            patch_address -= offset;
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
            reinterpret_cast<app_pc>(patch_address));

        // apply the patch
        granary_atomic_write8(
            staged_code_,
            reinterpret_cast<uint64_t *>(patch_address));

        return context;
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
        // templates/
        cpu_state_handle cpu;
        cpu->transient_allocator.free_all();

        return dest_pc;
    }


    STATIC_INITIALISE({

        // initialise the direct jump resolvers in the policy
        FOR_EACH_DIRECT_JUMP(DEFINE_DIRECT_JUMP_RESOLVER);

        // initialise the direct call resolver in the policy
        direct_branch_call = make_direct_cti_patch_func<call_>(
            granary_asm_direct_branch_template);
    });


    /// Look up and return the assembly patch (see asm/direct_branch.asm)
    /// function needed to patch an instruction that originally had opcode as
    /// `opcode`.
    static app_pc get_direct_cti_patch_func(int opcode) throw() {
        switch(opcode) {
        CASE_DIRECT_JUMP_MANGLER(call, 5)
        FOR_EACH_DIRECT_JUMP(CASE_DIRECT_JUMP_MANGLER);
        default: return nullptr;
        }
    }


    /// Computes the absolute value of a pointer difference.
    static uint64_t pc_diff(ptrdiff_t diff) {
        if(diff < 0) {
            return static_cast<uint64_t>(-diff);
        }
        return static_cast<uint64_t>(diff);
    }


    /// Add a direct branch slot; this is a sort of "formula" for direct
    /// branches that pushes two addresses and then jmps to an actual
    /// direct branch handler.
    void instruction_list_mangler::add_direct_branch_stub(
        instruction_list_handle in,
        operand target
    ) throw() {

        // if this is a detach point then replace the target address with the
        // detach address.
        app_pc detach_target_pc(find_detach_target(target));
        if(nullptr != detach_target_pc) {
            in->set_cti_target(pc_(detach_target_pc));
            in->set_mangled();
            return;
        }

        const unsigned old_size(in->encoded_size());

        instrumentation_policy target_policy(in->policy());
        if(!target_policy) {
            target_policy = policy;
        }

        // set the policy-fied target
        mangled_address am(target, target_policy.base_policy());

        // add in the patch code
        instruction_list_handle patch(
            ls->append(lea_(reg::rsp, reg::rsp[-8])));
            ls->append(push_(reg::rax));
            ls->append(mov_imm_(reg::rax, int64_(am.as_int)));
            ls->append(mov_st_(reg::rsp[8], reg::rax));
            ls->append(pop_(reg::rax));
            ls->append(jmp_(pc_(get_direct_cti_patch_func(in->op_code()))));

        *in = call_(instr_(*patch));

        // change the state of the patched instruction.
        in->set_mangled();
        in->set_patchable();

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

        if(in->is_call()) {
            *in = call_(pc_(ibl_entry_for(target, target_policy_ibl)));
        } else {
            *in = jmp_(pc_(ibl_entry_for(target, target_policy_ibl)));
        }
    }


    void instruction_list_mangler::mangle_call(
        instruction_list_handle in
    ) throw() {
        operand target(in->cti_target());

        if(dynamorio::opnd_is_pc(target)) {
            add_direct_branch_stub(in, target);
        } else if(!dynamorio::opnd_is_instr(target)) {
            mangle_indirect_cti(in, target);
        }
    }


    void instruction_list_mangler::mangle_return(
        instruction_list_handle in
    ) throw() {
        (void) in;
    }


    void instruction_list_mangler::mangle_jump(
        instruction_list_handle in
    ) throw() {

        operand target(in->cti_target());
        if(dynamorio::opnd_is_pc(target)) {
            add_direct_branch_stub(in, target);
        } else if(!dynamorio::opnd_is_instr(target)) {
            mangle_indirect_cti(in, target);
        }
    }


    void instruction_list_mangler::mangle_cli(instruction_list_handle in) throw() {
        (void) in;
    }


    void instruction_list_mangler::mangle_sti(instruction_list_handle in) throw() {

        (void) in;
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

        // if the operand is too far away then we will need to spill some
        // registers and use the vtable slot allocated for this address.
        const app_pc addr(reinterpret_cast<app_pc>(op->value.addr));
        if(_4GB >= pc_diff(estimator_pc - addr)) {
            return;
        }

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

        // we need to change the instruction; we assume that no instruction
        // has two memory operands. This is sort of correct, in that
        // we can have two memory operands, but they have to match (e.g. inc)
        //
        // Note: because this is a user space thing only, we don't need to be
        //       as concerned about the kind of a byte of an instruction, as
        //       we don't need to deliver precise (or any for that matter)
        //       interrupts. As such, this code is not interrupt safe.


        const uint64_t addr(reinterpret_cast<uint64_t>(far_op.value.pc));




        register_manager rm;
        rm.kill_all();

        // mini peephole optimisation; ideally will allow us to avoid
        // spilling a register.
        instruction_list_handle next_in(in.next());
        if(next_in.is_valid()) {
            rm.visit(*next_in);
        }

        rm.revive(*in);

        operand spill_reg(rm.get_zombie());

        ls->insert_before(in, push_(spill_reg));
        ls->insert_before(in, mov_imm_(spill_reg, int64_(addr)));
        ls->insert_after(in, pop_(spill_reg));

        operand_base_disp new_op_(*spill_reg);
        new_op_.size = far_op.size;

        operand new_op(new_op_);

        in->for_each_operand(update_far_operand, new_op);
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
            const bool can_skip(nullptr == in->pc() || in->is_mangled());
            next_in = in.next();

            // native instruction, we might need to mangle it.
            if(in->is_cti()) {

                if(dynamorio::instr_is_call(*in)) {
                    mangle_call(in);

                } else if(dynamorio::instr_is_return(*in)) {
                    mangle_return(in);

                // JMP, Jcc
                } else {
                    mangle_jump(in);
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

            // if in user space, look for uses of relative addresses in operands
            // that are no longer reachable with %rip-relative encoding, and
            // convert to a use of an absolute address (requires spilling a
            // register)
            } else {
                IF_USER(mangle_far_memory_refs(in, estimator_pc);)
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

