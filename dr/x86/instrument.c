/* ******************************************************************************
 * Copyright (c) 2010-2012 Google, Inc.  All rights reserved.
 * Copyright (c) 2010-2011 Massachusetts Institute of Technology  All rights reserved.
 * Copyright (c) 2002-2010 VMware, Inc.  All rights reserved.
 * ******************************************************************************/

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * 
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * 
 * * Neither the name of VMware, Inc. nor the names of its contributors may be
 *   used to endorse or promote products derived from this software without
 *   specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL VMWARE, INC. OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

/* Copyright (c) 2003-2007 Determina Corp. */
/* Copyright (c) 2002-2003 Massachusetts Institute of Technology */
/* Copyright (c) 2002 Hewlett-Packard Company */

/*
 * instrument.c - interface for instrumentation
 */

#include "dr/globals.h"   /* just to disable warning C4206 about an empty file */


#include "dr/x86/instrument.h"
#include "dr/instrlist.h"
#include "dr/x86/arch.h"
#include "dr/x86/instr.h"
#include "dr/x86/instr_create.h"
#include "dr/x86/decode.h"
#include "dr/x86/disassemble.h"
#include "dr/fragment.h"
#include "dr/emit.h"
#include "dr/link.h"
#include "dr/monitor.h" /* for mark_trace_head */
#include <string.h> /* for strstr */
#include <stdarg.h> /* for varargs */
#include "dr/nudge.h" /* for nudge_internal() */
#include "dr/synch.h"
#ifdef LINUX
# include <sys/time.h> /* ITIMER_* */
# include "dr/linux/module.h" /* redirect_* functions */
#endif


DR_API
/* Inserts inst as a non-application instruction into ilist prior to "where" */
void
instrlist_meta_preinsert(instrlist_t *ilist, instr_t *where, instr_t *inst)
{
    instr_set_ok_to_mangle(inst, false);
    instrlist_preinsert(ilist, where, inst);
}

DR_API
/* Inserts inst as a non-application instruction into ilist after "where" */
void
instrlist_meta_postinsert(instrlist_t *ilist, instr_t *where, instr_t *inst)
{
    instr_set_ok_to_mangle(inst, false);
    instrlist_postinsert(ilist, where, inst);
}

DR_API
/* Inserts inst as a non-application instruction onto the end of ilist */
void
instrlist_meta_append(instrlist_t *ilist, instr_t *inst)
{
    instr_set_ok_to_mangle(inst, false);
    instrlist_append(ilist, inst);
}

DR_API
void 
instrlist_meta_fault_preinsert(instrlist_t *ilist, instr_t *where, instr_t *inst)
{
    instr_set_meta_may_fault(inst, true);
    instrlist_preinsert(ilist, where, inst);
}

DR_API
void
instrlist_meta_fault_postinsert(instrlist_t *ilist, instr_t *where, instr_t *inst)
{
    instr_set_meta_may_fault(inst, true);
    instrlist_postinsert(ilist, where, inst);
}

DR_API
void
instrlist_meta_fault_append(instrlist_t *ilist, instr_t *inst)
{
    instr_set_meta_may_fault(inst, true);
    instrlist_append(ilist, inst);
}

#if 0

static void
convert_va_list_to_opnd(opnd_t *args, uint num_args, va_list ap)
{
    uint i;
    /* There's no way to check num_args vs actual args passed in */
    for (i = 0; i < num_args; i++) {
        args[i] = va_arg(ap, opnd_t);
        CLIENT_ASSERT(opnd_is_valid(args[i]),
                      "Call argument: bad operand. Did you create a valid opnd_t?");
    }
}

/* dr_insert_* are used by general DR */
/* Inserts a complete call to callee with the passed-in arguments */
void
dr_insert_call(void *drcontext, instrlist_t *ilist, instr_t *where,
               void *callee, uint num_args, ...)
{
    dcontext_t *dcontext = (dcontext_t *) drcontext;
    opnd_t *args = NULL;
    va_list ap;
    CLIENT_ASSERT(drcontext != NULL, "dr_insert_call: drcontext cannot be NULL");
    /* we don't check for GLOBAL_DCONTEXT since DR internally calls this */
    if (num_args != 0) {
        args = HEAP_ARRAY_ALLOC(drcontext, opnd_t, num_args, 
                                ACCT_CLEANCALL, UNPROTECTED);
        va_start(ap, num_args);
        convert_va_list_to_opnd(args, num_args, ap);
        va_end(ap);
    }
    insert_meta_call_vargs(dcontext, ilist, where, false/*not clean*/,
                           callee, num_args, args);
    if (num_args != 0) {
        HEAP_ARRAY_FREE(drcontext, args, opnd_t, num_args,
                        ACCT_CLEANCALL, UNPROTECTED);
    }
}

/* Internal utility routine for inserting context save for a clean call.
 * Returns the size of the data stored on the DR stack 
 * (in case the caller needs to align the stack pointer). 
 * XSP and XAX are modified by this call.
 */
static uint
prepare_for_call_ex(dcontext_t  *dcontext, clean_call_info_t *cci,
                    instrlist_t *ilist, instr_t *where)
{
    instr_t *in;
    uint dstack_offs;
    in = (where == NULL) ? instrlist_last(ilist) : instr_get_prev(where);
    dstack_offs = prepare_for_clean_call(dcontext, cci, ilist, where);
    /* now go through and mark inserted instrs as meta */
    if (in == NULL)
        in = instrlist_first(ilist);
    else
        in = instr_get_next(in);
    while (in != where) {
        instr_set_ok_to_mangle(in, false);
        in = instr_get_next(in);
    }
    return dstack_offs;
}

/* Internal utility routine for inserting context restore for a clean call. */
static void
cleanup_after_call_ex(dcontext_t *dcontext, clean_call_info_t *cci,
                      instrlist_t *ilist, instr_t *where, uint sizeof_param_area)
{
    instr_t *in;
    in = (where == NULL) ? instrlist_last(ilist) : instr_get_prev(where);
    if (sizeof_param_area > 0) {
        /* clean up the parameter area */
        CLIENT_ASSERT(sizeof_param_area <= 127,
                      "cleanup_after_call_ex: sizeof_param_area must be <= 127");
        /* mark it meta down below */
        instrlist_preinsert(ilist, where,
            INSTR_CREATE_add(dcontext, opnd_create_reg(REG_XSP),
                             OPND_CREATE_INT8(sizeof_param_area)));
    }
    cleanup_after_clean_call(dcontext, cci, ilist, where);
    /* now go through and mark inserted instrs as meta */
    if (in == NULL)
        in = instrlist_first(ilist);
    else
        in = instr_get_next(in);
    while (in != where) {
        instr_set_ok_to_mangle(in, false);
        in = instr_get_next(in);
    }
}

/* Inserts a complete call to callee with the passed-in arguments, wrapped
 * by an app save and restore.
 * If "save_fpstate" is true, saves the fp/mmx/sse state.
 *
 * NOTE : this routine clobbers TLS_XAX_SLOT and the XSP mcontext slot via
 * dr_prepare_for_call(). We guarantee to clients that all other slots
 * (except the XAX mcontext slot) will remain untouched.
 */
void 
dr_insert_clean_call_ex_varg(void *drcontext, instrlist_t *ilist, instr_t *where,
                             void *callee, dr_cleancall_save_t save_flags,
                             uint num_args, va_list ap)
{
    dcontext_t *dcontext = (dcontext_t *) drcontext;
    uint dstack_offs, pad = 0;
    size_t buf_sz = 0;
    clean_call_info_t cci; /* information for clean call insertion. */
    opnd_t *args = NULL;
    bool save_fpstate = TEST(DR_CLEANCALL_SAVE_FLOAT, save_flags);
    CLIENT_ASSERT(drcontext != NULL, "dr_insert_clean_call: drcontext cannot be NULL");
    STATS_INC(cleancall_inserted);
    LOG(THREAD, LOG_CLEANCALL, 2, "CLEANCALL: insert clean call to "PFX"\n", callee);
    if (num_args != 0) {
        /* we don't check for GLOBAL_DCONTEXT since DR internally calls this */
        /* allocate at least one argument opnd */
        args = HEAP_ARRAY_ALLOC(drcontext, opnd_t, num_args,
                                ACCT_CLEANCALL, UNPROTECTED);
        convert_va_list_to_opnd(args, num_args, ap);
    }
    /* analyze the clean call, return true if clean call can be inlined. */
    if (analyze_clean_call(dcontext, &cci, where, callee, 
                           save_fpstate, num_args, args)) {
#ifdef CLIENT_INTERFACE
        /* we can perform the inline optimization and return. */
        STATS_INC(cleancall_inlined);
        LOG(THREAD, LOG_CLEANCALL, 2, "CLEANCALL: inlined callee "PFX"\n", callee);
        insert_inline_clean_call(dcontext, &cci, ilist, where, args);
        if (num_args != 0) {
            HEAP_ARRAY_FREE(drcontext, args, opnd_t, num_args,
                            ACCT_CLEANCALL, UNPROTECTED);
        }
        return;
#else /* CLIENT_INTERFACE */
        ASSERT_NOT_REACHED();
#endif /* CLIENT_INTERFACE */
    }
    /* honor requests from caller */
    if (TEST(DR_CLEANCALL_NOSAVE_FLAGS, save_flags)) {
        /* even if we remove flag saves we want to keep mcontext shape */
        cci.preserve_mcontext = true;
        cci.skip_save_aflags = true;
        /* we assume this implies DF should be 0 already */
        cci.skip_clear_eflags = true;
        /* XXX: should also provide DR_CLEANCALL_NOSAVE_NONAFLAGS to
         * preserve just arith flags on return from a call
         */
    }
    if (TESTANY(DR_CLEANCALL_NOSAVE_XMM |
                DR_CLEANCALL_NOSAVE_XMM_NONPARAM |
                DR_CLEANCALL_NOSAVE_XMM_NONRET, save_flags)) {
        uint i;
        /* even if we remove xmm saves we want to keep mcontext shape */
        cci.preserve_mcontext = true;
        /* start w/ all */
#if defined(X64) && defined(WINDOWS)
        cci.num_xmms_skip = 6;
#else
        /* all 8 (or 16) are scratch */
        cci.num_xmms_skip = NUM_XMM_REGS;
#endif
        for (i=0; i<cci.num_xmms_skip; i++)
            cci.xmm_skip[i] = true;
        /* now remove those used for param/retval */
#ifdef X64
        if (TEST(DR_CLEANCALL_NOSAVE_XMM_NONPARAM, save_flags)) {
            /* xmm0-3 (-7 for linux) are used for params */
# ifdef LINUX
            for (i=0; i<7; i++)
# else
            for (i=0; i<3; i++)
# endif
                cci.xmm_skip[i] = false;
            cci.num_xmms_skip -= i;
        }
        if (TEST(DR_CLEANCALL_NOSAVE_XMM_NONRET, save_flags)) {
            /* xmm0 (and xmm1 for linux) are used for retvals */
            cci.xmm_skip[0] = false;
            cci.num_xmms_skip--;
# ifdef LINUX
            cci.xmm_skip[1] = false;
            cci.num_xmms_skip--;
# endif
        }
#endif         
    }
    dstack_offs = prepare_for_call_ex(dcontext, &cci, ilist, where);
#ifdef X64
    /* PR 218790: we assume that dr_prepare_for_call() leaves stack 16-byte
     * aligned, which is what insert_meta_call_vargs requires. */
    if (cci.should_align) {
        CLIENT_ASSERT(ALIGNED(dstack_offs, 16),
                      "internal error: bad stack alignment");
    }
#endif
    if (save_fpstate) {
        /* save on the stack: xref PR 202669 on clients using more stack */
        buf_sz = proc_fpstate_save_size();
        /* we need 16-byte-alignment */
        pad = ALIGN_FORWARD_UINT(dstack_offs, 16) - dstack_offs;
        IF_X64(CLIENT_ASSERT(CHECK_TRUNCATE_TYPE_int(buf_sz + pad),
                             "dr_insert_clean_call: internal truncation error"));
        MINSERT(ilist, where, INSTR_CREATE_sub(dcontext, opnd_create_reg(REG_XSP),
                                               OPND_CREATE_INT32((int)(buf_sz + pad))));
        dr_insert_save_fpstate(drcontext, ilist, where,
                               opnd_create_base_disp(REG_XSP, REG_NULL, 0, 0,
                                                     OPSZ_512));
    }

    /* PR 302951: restore state if clean call args reference app memory.
     * We use a hack here: this is the only instance where we mark as our-mangling
     * but do not have a translation target set, which indicates to the restore
     * routines that this is a clean call.  If the client adds instrs in the middle
     * translation will fail; if the client modifies any instr, the our-mangling
     * flag will disappear and translation will fail.
     */
    instrlist_set_our_mangling(ilist, true);
    insert_meta_call_vargs(dcontext, ilist, where, true/*clean*/,
                           callee, num_args, args);
    if (num_args != 0) {
        HEAP_ARRAY_FREE(drcontext, args, opnd_t, num_args, 
                        ACCT_CLEANCALL, UNPROTECTED);
    }
    instrlist_set_our_mangling(ilist, false);

    if (save_fpstate) {
        dr_insert_restore_fpstate(drcontext, ilist, where,
                                  opnd_create_base_disp(REG_XSP, REG_NULL, 0, 0,
                                                        OPSZ_512));
        MINSERT(ilist, where, INSTR_CREATE_add(dcontext, opnd_create_reg(REG_XSP),
                                               OPND_CREATE_INT32(buf_sz + pad)));
    }
    cleanup_after_call_ex(dcontext, &cci, ilist, where, 0);
}

void 
dr_insert_clean_call_ex(void *drcontext, instrlist_t *ilist, instr_t *where,
                        void *callee, dr_cleancall_save_t save_flags,
                        uint num_args, ...)
{
    va_list ap;
    va_start(ap, num_args);
    dr_insert_clean_call_ex_varg(drcontext, ilist, where, callee, save_flags,
                                 num_args, ap);
    va_end(ap);
}

DR_API
void 
dr_insert_clean_call(void *drcontext, instrlist_t *ilist, instr_t *where,
                     void *callee, bool save_fpstate, uint num_args, ...)
{
    va_list ap;
    dr_cleancall_save_t flags = (save_fpstate ? DR_CLEANCALL_SAVE_FLOAT : 0);
    va_start(ap, num_args);
    dr_insert_clean_call_ex_varg(drcontext, ilist, where, callee, flags, num_args, ap);
    va_end(ap);
}

/* Utility routine for inserting a clean call to an instrumentation routine
 * Returns the size of the data stored on the DR stack (in case the caller
 * needs to align the stack pointer).  XSP and XAX are modified by this call.
 * 
 * NOTE : this routine clobbers TLS_XAX_SLOT and the XSP mcontext slot via
 * prepare_for_clean_call(). We guarantee to clients that all other slots
 * (except the XAX mcontext slot) will remain untouched.
 */
DR_API uint
dr_prepare_for_call(void *drcontext, instrlist_t *ilist, instr_t *where)
{
    CLIENT_ASSERT(drcontext != NULL, "dr_prepare_for_call: drcontext cannot be NULL");
    CLIENT_ASSERT(drcontext != GLOBAL_DCONTEXT,
                  "dr_prepare_for_call: drcontext is invalid");
    return prepare_for_call_ex((dcontext_t *)drcontext, NULL, ilist, where);
}

DR_API void
dr_cleanup_after_call(void *drcontext, instrlist_t *ilist, instr_t *where,
                       uint sizeof_param_area)
{
    CLIENT_ASSERT(drcontext != NULL, "dr_cleanup_after_call: drcontext cannot be NULL");
    CLIENT_ASSERT(drcontext != GLOBAL_DCONTEXT,
                  "dr_cleanup_after_call: drcontext is invalid");
    cleanup_after_call_ex((dcontext_t *)drcontext, NULL, ilist, where, 
                          sizeof_param_area);
}

#endif
