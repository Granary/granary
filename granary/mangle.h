/*
 * mangle.h
 *
 *  Created on: 2012-11-27
 *      Author: pag
 *     Version: $Id$
 */

#ifndef Granary_MANGLE_H_
#define Granary_MANGLE_H_

#include "globals.h"
#include "state.h"
#include "instruction.h"
#include "code_cache.h"


/// Used to unroll registers in the opposite order in which they are saved
/// by PUSHA in x86/asm_helpers.asm. This is so that we can operate on the
/// pushed state on the stack as a form of machine context.
#define DPM_DECLARE_REG(reg) \
    uint64_t reg;

#define DPM_DECLARE_REG_CONT(reg, rest) \
    rest \
    DPM_DECLARE_REG(reg)


namespace granary {


    /// Stage an 8-byte hot patch. This will encode the instruction `in` into
    /// the `stage` location (as if it were going to be placed at the `dest`
    /// location, and then encodes however many NOPs are needed to fill in 8
    /// bytes.
    void stage_8byte_hot_patch(instruction in, app_pc stage, app_pc dest);


    /// A machine context representation for direct jump patches. The struture
    /// contains as much state as is saved by the direct assembly direct jump
    /// patch functions.
    ///
    /// The high-level operation here is that the patch function will know
    /// what to patch because of the return address on the stack (we guarantee
    /// that hot-patchable instructions will be aligned on 8-byte boundaries)
    /// and that we know where we need to patch the target to by looking at the
    /// relative target offset from the beginning of application code.
    struct direct_patch_mcontext {

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


    /// Patch the code by regenerating the original instruction.
    ///
    /// Note: in kernel mode, this function executes with interrupts disabled.
    ///
    /// Note: this function alters a return address in `context` so that when
    ///       the corresponding assembly patch function returns, it will return
    ///       to the instruction just patched.
    GRANARY_ENTRYPOINT
    template <instruction (*make_opcode)(dynamorio::opnd_t)>
    void patch_mangled_direct_cti(direct_patch_mcontext *context) throw() {

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

        target_pc = code_cache::find(target_pc);

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
}

#undef DPM_DECLARE_REG
#undef DPM_DECLARE_REG_CONT
#endif /* Granary_MANGLE_H_ */
