/*
 * code_cache.h
 *
 *  Created on: 2012-11-28
 *      Author: pag
 *     Version: $Id$
 */

#ifndef Granary_CODE_CACHE_H_
#define Granary_CODE_CACHE_H_

#include "granary/globals.h"
#include "granary/state.h"
#include "granary/hash_table.h"
#include "granary/mangle.h"
#include "granary/policy.h"
#include "granary/detach.h"
#include "granary/instruction.h"
#include "granary/basic_block.h"


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
    instrumenter<Policy>::direct_branch_ ## opcode = \
        make_direct_cti_patch_func<\
            direct_jump_patch_mcontext, \
            opcode ## _ \
        >(granary_asm_direct_branch_template);


extern "C" {
    extern void granary_asm_direct_branch_template(void);
    extern void granary_asm_direct_call_template(void);
}


namespace granary {


    /// Represents a policy-specific code cache. A single client can be wholly
    /// represented by a policy, apply many policies (in the form of an
    /// aggregate policy), or dynamically switch between instrumentation
    /// policies.
    ///
    /// Note: This structure is tightly coupled with `instrumenter` of policy.h.
    template <typename Policy=null_policy>
    struct code_cache {
    private:


        /// Policy-specific shared code cache.
        static hash_table<app_pc, app_pc> CODE_CACHE;


        /// Instrumentation policy to use.
        static instrumenter<Policy> POLICY;

    public:

        /// Perform both lookup and insertion (basic block translation) into
        /// the code cache.
        inline static app_pc find(app_pc addr) throw() {
            cpu_state_handle cpu;
            thread_state_handle thread;
            return find(cpu, thread, addr);
        }

        /// Perform both lookup and insertion (basic block translation) into
        /// the code cache.
        static app_pc find(cpu_state_handle &cpu,
                    thread_state_handle &thread,
                    app_pc addr) throw() {

            app_pc target_addr(find_detach_target(addr));
            if(nullptr != target_addr) {
                return target_addr;
            }

            if(CODE_CACHE.load(addr, &target_addr)) {
                return target_addr;
            }

            app_pc decode_addr(addr);
            basic_block bb(basic_block::translate(
                POLICY, cpu, thread, &decode_addr));

            CODE_CACHE.store(addr, bb.cache_pc_start);

            return bb.cache_pc_start;
        }

    private:

        /// A machine context representation for direct jump patches. The
        /// struture contains as much state as is saved by the direct assembly
        /// direct jump patch functions.
        ///
        /// The high-level operation here is that the patch function will know
        /// what to patch because of the return address on the stack (we guarantee
        /// that hot-patchable instructions will be aligned on 8-byte boundaries)
        /// and that we know where we need to patch the target to by looking at the
        /// relative target offset from the beginning of application code.
        struct direct_jump_patch_mcontext {

            /// Saved regs.
            ALL_REGS(DPM_DECLARE_REG_CONT, DPM_DECLARE_REG)

            /// Saved flags.
            uint64_t flags;

            /// to_application_offset(target), where target is the destination
            /// address of the direct branch.
            int64_t rel_target_offset_from_app;

            /// The return address *immediately* following the mangled instruction.
            uint64_t return_address_after_mangled_call;
        };

        struct direct_call_patch_mcontext {

            /// Saved regs.
            ALL_ARG_REGS(DPM_DECLARE_REG_CONT, DPM_DECLARE_REG)

            /// Saved flags.
            IF_KERNEL(uint64_t flags;)

            /// to_application_offset(target), where target is the destination
            /// address of the direct branch.
            int64_t rel_target_offset_from_app;

            /// The return address *immediately* following the mangled instruction.
            uint64_t return_address_after_mangled_call;
        };


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
            typename MContext,
            instruction (*make_opcode)(dynamorio::opnd_t)
        >
        static void
        find_and_patch_direct_cti(MContext *context) throw() {

            cpu_state_handle cpu;
            thread_state_handle thread;

            granary::enter(cpu, thread);

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

            // get an address into the target basic block
            app_pc target_pc(reinterpret_cast<app_pc>(
                from_application_offset(context->rel_target_offset_from_app)));

            target_pc = find(target_pc);

            // create the patch code
            uint64_t staged_code_(0ULL);
            app_pc staged_code(reinterpret_cast<app_pc>(&staged_code_));

            stage_8byte_hot_patch(
                make_opcode(pc_(target_pc)),
                staged_code,
                reinterpret_cast<app_pc>(patch_address));

            // apply the patch
            granary_atomic_write8(
                staged_code_,
                reinterpret_cast<uint64_t *>(patch_address));
        }


        /// Make a direct patch function that is specific to the instrumentation
        /// policy and a particular opcode.
        template <typename MContext, instruction (*make_opcode)(dynamorio::opnd_t)>
        static app_pc make_direct_cti_patch_func(void (*template_func)(void)) throw() {
            instruction_list ls;
            app_pc start_pc(reinterpret_cast<app_pc>(template_func));

            for(;;) {
                instruction in(instruction::decode(&start_pc));
                if(in.is_call()) {
                    in = call_(pc_(reinterpret_cast<app_pc>(
                        find_and_patch_direct_cti<MContext, make_opcode>)));
                }

                ls.append(in);

                if(dynamorio::OP_ret == in.op_code()) {
                    break;
                }
            }

            app_pc dest_pc(global_state::fragment_allocator.allocate_array<uint8_t>(
                ls.encoded_size()));

            ls.encode(dest_pc);

            return dest_pc;
        }


        /// Static initialize the policy for this code cache.
        ///
        /// Note: this function initializes fields in the instrumenter template
        ///       of `policy.h`.
        static instrumenter<Policy> init_policy(void) throw() {

            // initialize the diruct jump resolvers in the policy
            FOR_EACH_DIRECT_JUMP(DEFINE_DIRECT_JUMP_RESOLVER);

            // initialize the direct call resolver in the policy
            instrumenter<Policy>::direct_branch_call = \
                make_direct_cti_patch_func<
                    direct_call_patch_mcontext,
                    call_
                >(granary_asm_direct_call_template);

            return instrumenter<Policy>();
        }
    };

    /// Static initialization.
    template <typename Policy>
    hash_table<app_pc, app_pc> code_cache<Policy>::CODE_CACHE;

    template <typename Policy>
    instrumenter<Policy> code_cache<Policy>::POLICY = \
        code_cache<Policy>::init_policy();

}

#undef DPM_DECLARE_REG
#undef DPM_DECLARE_REG_CONT
#undef DEFINE_DIRECT_JUMP_MANGLER
#endif /* Granary_CODE_CACHE_H_ */
