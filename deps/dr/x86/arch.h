/* **********************************************************
 * Copyright (c) 2010-2012 Google, Inc.  All rights reserved.
 * Copyright (c) 2000-2010 VMware, Inc.  All rights reserved.
 * **********************************************************/

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
/* Copyright (c) 2001-2003 Massachusetts Institute of Technology */
/* Copyright (c) 2000-2001 Hewlett-Packard Company */

/* file "arch.h" -- internal x86-specific definitions
 *
 * References: 
 *   "Intel Architecture Software Developer's Manual", 1999.
 */

#ifndef X86_ARCH_H
#define X86_ARCH_H

#include <stddef.h> /* for offsetof */
#include "instr.h" /* for reg_id_t */
#include "decode.h" /* for X64_CACHE_MODE_DC */
#include "arch_exports.h" /* for FRAG_IS_32 and FRAG_IS_X86_TO_X64 */

/* FIXME: check on all platforms: these are for Fedora 8 and XP SP2
 * Keep in synch w/ defines in pre_inject_asm.asm
 */
#define CS32_SELECTOR 0x23
#define CS64_SELECTOR 0x33

#ifdef X64
static inline bool
mixed_mode_enabled(void)
{
    /* XXX i#49: currently only supporting WOW64 and thus only
     * creating x86 versions of gencode for WOW64.  Eventually we'll
     * have to either always create for every x64 process, or lazily
     * create on first appearance of 32-bit code.
     */
# ifdef WINDOWS
    return is_wow64_process(NT_CURRENT_PROCESS);
# else
    return false;
# endif
}
#endif

/* dcontext_t field offsets 
 * N.B.: DO NOT USE offsetof(dcontext_t) anywhere else if passing to the
 * dcontext operand construction routines!
 * Otherwise we will have issues w/ the upcontext offset game below
 */

/* offs is not raw offset, but includes upcontext size, so we
 * can tell unprotected from normal!
 * unprotected are raw 0..sizeof(unprotected_context_t)
 * protected are raw + sizeof(unprotected_context_t)
 * (see the instr.c routines for dcontext instr building)
 * FIXME: we could get rid of this hack if unprotected_context_t == priv_mcontext_t
 */
#define PROT_OFFS         (sizeof(unprotected_context_t))
#define MC_OFFS           (offsetof(unprotected_context_t, mcontext))

#define XAX_OFFSET        ((MC_OFFS) + (offsetof(priv_mcontext_t, xax)))
#define XBX_OFFSET        ((MC_OFFS) + (offsetof(priv_mcontext_t, xbx)))
#define XCX_OFFSET        ((MC_OFFS) + (offsetof(priv_mcontext_t, xcx)))
#define XDX_OFFSET        ((MC_OFFS) + (offsetof(priv_mcontext_t, xdx)))
#define XSI_OFFSET        ((MC_OFFS) + (offsetof(priv_mcontext_t, xsi)))
#define XDI_OFFSET        ((MC_OFFS) + (offsetof(priv_mcontext_t, xdi)))
#define XBP_OFFSET        ((MC_OFFS) + (offsetof(priv_mcontext_t, xbp)))
#define XSP_OFFSET        ((MC_OFFS) + (offsetof(priv_mcontext_t, xsp)))
#define XFLAGS_OFFSET     ((MC_OFFS) + (offsetof(priv_mcontext_t, xflags)))
#define PC_OFFSET         ((MC_OFFS) + (offsetof(priv_mcontext_t, pc)))
#ifdef X64
# define R8_OFFSET        ((MC_OFFS) + (offsetof(priv_mcontext_t, r8)))
# define R9_OFFSET        ((MC_OFFS) + (offsetof(priv_mcontext_t, r9)))
# define R10_OFFSET       ((MC_OFFS) + (offsetof(priv_mcontext_t, r10)))
# define R11_OFFSET       ((MC_OFFS) + (offsetof(priv_mcontext_t, r11)))
# define R12_OFFSET       ((MC_OFFS) + (offsetof(priv_mcontext_t, r12)))
# define R13_OFFSET       ((MC_OFFS) + (offsetof(priv_mcontext_t, r13)))
# define R14_OFFSET       ((MC_OFFS) + (offsetof(priv_mcontext_t, r14)))
# define R15_OFFSET       ((MC_OFFS) + (offsetof(priv_mcontext_t, r15)))
#endif
#define XMM_OFFSET        ((MC_OFFS) + (offsetof(priv_mcontext_t, ymm)))

#define ERRNO_OFFSET      (offsetof(unprotected_context_t, errno))
#define AT_SYSCALL_OFFSET (offsetof(unprotected_context_t, at_syscall))

#define NEXT_TAG_OFFSET        ((PROT_OFFS)+offsetof(dcontext_t, next_tag))
#define LAST_EXIT_OFFSET       ((PROT_OFFS)+offsetof(dcontext_t, last_exit))
#define DSTACK_OFFSET          ((PROT_OFFS)+offsetof(dcontext_t, dstack))
#define NATIVE_EXEC_RETVAL_OFFSET ((PROT_OFFS)+offsetof(dcontext_t, native_exec_retval))
#define NATIVE_EXEC_RETLOC_OFFSET ((PROT_OFFS)+offsetof(dcontext_t, native_exec_retloc))
#ifdef RETURN_STACK
# define RSTACK_OFFSET         ((PROT_OFFS)+offsetof(dcontext_t, rstack))
# define TOP_OF_RSTACK_OFFSET  ((PROT_OFFS)+offsetof(dcontext_t, top_of_rstack))
#endif

#define FRAGMENT_FIELD_OFFSET  ((PROT_OFFS)+offsetof(dcontext_t, fragment_field))
#define PRIVATE_CODE_OFFSET    ((PROT_OFFS)+offsetof(dcontext_t, private_code))

#ifdef WINDOWS
# ifdef CLIENT_INTERFACE
#  define APP_ERRNO_OFFSET      ((PROT_OFFS)+offsetof(dcontext_t, app_errno))
#  define APP_FLS_OFFSET        ((PROT_OFFS)+offsetof(dcontext_t, app_fls_data))
#  define PRIV_FLS_OFFSET       ((PROT_OFFS)+offsetof(dcontext_t, priv_fls_data))
#  define APP_RPC_OFFSET        ((PROT_OFFS)+offsetof(dcontext_t, app_nt_rpc))
#  define PRIV_RPC_OFFSET       ((PROT_OFFS)+offsetof(dcontext_t, priv_nt_rpc))
# endif
# define NONSWAPPED_SCRATCH_OFFSET  ((PROT_OFFS)+offsetof(dcontext_t, nonswapped_scratch))
#endif

#ifdef TRACE_HEAD_CACHE_INCR 
# define TRACE_HEAD_PC_OFFSET  ((PROT_OFFS)+offsetof(dcontext_t, trace_head_pc))
#endif

#ifdef NATIVE_RETURN_CALLDEPTH
# define CALL_DEPTH_OFFSET     ((PROT_OFFS)+offsetof(dcontext_t, call_depth))
#endif

#ifdef WINDOWS
# define SYSENTER_STORAGE_OFFSET ((PROT_OFFS)+offsetof(dcontext_t, sysenter_storage))
# define IGNORE_ENTEREXIT_OFFSET ((PROT_OFFS)+offsetof(dcontext_t, ignore_enterexit))
#endif

#ifdef CLIENT_INTERFACE
# define CLIENT_DATA_OFFSET    ((PROT_OFFS)+offsetof(dcontext_t, client_data))
#endif

#define COARSE_IB_SRC_OFFSET   ((PROT_OFFS)+offsetof(dcontext_t, coarse_exit.src_tag))
#define COARSE_DIR_EXIT_OFFSET ((PROT_OFFS)+offsetof(dcontext_t, coarse_exit.dir_exit))

int
reg_spill_tls_offs(reg_id_t reg);

#define OPSZ_SAVED_XMM (YMM_ENABLED() ? OPSZ_32 : OPSZ_16)
#define REG_SAVED_XMM0 (YMM_ENABLED() ? REG_YMM0 : REG_XMM0)

/* Xref the partially overlapping CONTEXT_PRESERVE_XMM */
/* This routine also determines whether ymm registers should be saved */
static inline bool
preserve_xmm_caller_saved(void)
{
    /* PR 264138: we must preserve xmm0-5 if on a 64-bit Windows kernel.
     * PR 302107: we must preserve xmm0-15 for 64-bit Linux apps.
     * i#139: we save xmm0-7 in 32-bit Linux and Windows b/c DR and client
     * code on modern compilers ends up using xmm regs w/o any flags to easily
     * disable w/o giving up perf.  (Xref PR 306394 where we originally did
     * not preserve xmm0-7 on a 32-bit kernel b/c DR didn't contain any xmm
     * reg usage).
     */
    return proc_has_feature(FEATURE_SSE) /* do xmm registers exist? */;
}

typedef enum {
    IBL_UNLINKED,
    IBL_DELETE,
    /* Pre-ibl routines for far ctis */
    IBL_FAR,
    IBL_FAR_UNLINKED,
#ifdef X64
    /* PR 257963: trace inline cmp has separate entries b/c it saves flags */
    IBL_TRACE_CMP,
    IBL_TRACE_CMP_UNLINKED,
#endif
    IBL_LINKED,
    
    IBL_TEMPLATE, /* a template is presumed to be always linked */
    IBL_LINK_STATE_END
} ibl_entry_point_type_t;

/* we should allow for all {{bb,trace} x {ret,ind call, ind jmp} x {shared, private}} */
/* combinations of routines which are in turn  x {unlinked, linked} */
typedef enum {
    /* FIXME: have a separate flag for private vs shared */
    IBL_BB_SHARED, 
    IBL_SOURCE_TYPE_START = IBL_BB_SHARED,
    IBL_TRACE_SHARED,
    IBL_BB_PRIVATE,
    IBL_TRACE_PRIVATE,
    IBL_COARSE_SHARED, /* no coarse private, for now */
    IBL_SOURCE_TYPE_END
} ibl_source_fragment_type_t;

#define DEFAULT_IBL_BB() \
    (DYNAMO_OPTION(shared_bbs) ? IBL_BB_SHARED : IBL_BB_PRIVATE)
#define DEFAULT_IBL_TRACE() \
    (DYNAMO_OPTION(shared_traces) ? IBL_TRACE_SHARED : IBL_TRACE_PRIVATE)
#define IS_IBL_BB(ibltype) \
    ((ibltype) == IBL_BB_PRIVATE || (ibltype) == IBL_BB_SHARED)
#define IS_IBL_TRACE(ibltype) \
    ((ibltype) == IBL_TRACE_PRIVATE || (ibltype) == IBL_TRACE_SHARED)
#define IS_IBL_LINKED(ibltype) \
    ((ibltype) == IBL_LINKED || (ibltype) == IBL_FAR \
     IF_X64(|| (ibltype) == IBL_TRACE_CMP))
#define IS_IBL_UNLINKED(ibltype) \
    ((ibltype) == IBL_UNLINKED || (ibltype) == IBL_FAR_UNLINKED \
     IF_X64(|| (ibltype) == IBL_TRACE_CMP_UNLINKED))

#define IBL_FRAG_FLAGS(ibl_code) \
    (IS_IBL_TRACE((ibl_code)->source_fragment_type) ? FRAG_IS_TRACE : 0)

static inline ibl_entry_point_type_t
get_ibl_entry_type(uint link_or_instr_flags)
{
#ifdef X64
    if (TEST(LINK_TRACE_CMP, link_or_instr_flags))
        return IBL_TRACE_CMP;
#endif
    if (TEST(LINK_FAR, link_or_instr_flags))
        return IBL_FAR;
    else
        return IBL_LINKED;
}

typedef struct
{
    /* these could be bit fields, if needed */
    ibl_entry_point_type_t link_state;
    ibl_source_fragment_type_t source_fragment_type;
    ibl_branch_type_t branch_type;
} ibl_type_t;

#ifdef X64
/* PR 282576: With shared_code_x86, GLOBAL_DCONTEXT no longer specifies
 * a unique generated_code_t.  Rather than add GLOBAL_DCONTEXT_X86 everywhere,
 * we add mode parameters to a handful of routines that take in GLOBAL_DCONTEXT.
 */
typedef enum {
    GENCODE_X64 = 0,
    GENCODE_X86,
    GENCODE_X86_TO_X64,
    GENCODE_FROM_DCONTEXT,
} gencode_mode_t;
# define FRAGMENT_GENCODE_MODE(fragment_flags) \
    (FRAG_IS_32(fragment_flags) ? GENCODE_X86 : \
     (FRAG_IS_X86_TO_X64(fragment_flags) ? GENCODE_X86_TO_X64 : GENCODE_X64))
# define SHARED_GENCODE(gencode_mode) get_shared_gencode(GLOBAL_DCONTEXT, gencode_mode)
# define SHARED_GENCODE_MATCH_THREAD(dc) get_shared_gencode(dc, GENCODE_FROM_DCONTEXT)
# define THREAD_GENCODE(dc) get_emitted_routines_code(dc, GENCODE_FROM_DCONTEXT)
# define GENCODE_IS_X64(gencode_mode) ((gencode_mode) == GENCODE_X64)
# define GENCODE_IS_X86(gencode_mode) ((gencode_mode) == GENCODE_X86)
# define GENCODE_IS_X86_TO_X64(gencode_mode) ((gencode_mode) == GENCODE_X86_TO_X64)
#else
# define SHARED_GENCODE(b) get_shared_gencode(GLOBAL_DCONTEXT)
# define THREAD_GENCODE(dc) get_emitted_routines_code(dc)
# define SHARED_GENCODE_MATCH_THREAD(dc) get_shared_gencode(dc)
#endif

#define NUM_XMM_REGS  NUM_XMM_SAVED
#define NUM_GP_REGS   (1 + (IF_X64_ELSE(DR_REG_R15, DR_REG_XDI) - DR_REG_XAX))

/* information about each individual clean call invocation site */
typedef struct _clean_call_info_t {
    void *callee;
    uint num_args;
    bool save_fpstate;
    bool opt_inline;
    bool should_align;
    bool save_all_regs;
    bool skip_save_aflags;
    bool skip_clear_eflags;
    uint num_xmms_skip;
    bool xmm_skip[NUM_XMM_REGS];
    uint num_regs_skip;
    bool reg_skip[NUM_GP_REGS];
    bool preserve_mcontext; /* even if skip reg save, preserve mcontext shape */
    void *callee_info;  /* callee information */
    instrlist_t *ilist; /* instruction list for inline optimization */
} clean_call_info_t;

cache_pc get_ibl_routine_ex(dcontext_t *dcontext, ibl_entry_point_type_t entry_type,
                            ibl_source_fragment_type_t source_fragment_type,
                            ibl_branch_type_t branch_type _IF_X64(gencode_mode_t mode));
cache_pc get_ibl_routine(dcontext_t *dcontext, ibl_entry_point_type_t entry_type,
                         ibl_source_fragment_type_t source_fragment_type, 
                         ibl_branch_type_t branch_type);
cache_pc get_ibl_routine_template(dcontext_t *dcontext, 
                                  ibl_source_fragment_type_t source_fragment_type, 
                                  ibl_branch_type_t branch_type
                                  _IF_X64(gencode_mode_t mode));
bool get_ibl_routine_type(dcontext_t *dcontext, cache_pc target, ibl_type_t *type);
bool get_ibl_routine_type_ex(dcontext_t *dcontext, cache_pc target, ibl_type_t *type
                             _IF_X64(gencode_mode_t *mode_out));
const char *get_ibl_routine_name(dcontext_t *dcontext, cache_pc target, 
                                 const char **ibl_brtype_name);
cache_pc get_trace_ibl_routine(dcontext_t *dcontext, cache_pc current_entry);
cache_pc get_private_ibl_routine(dcontext_t *dcontext, cache_pc current_entry);
cache_pc get_shared_ibl_routine(dcontext_t *dcontext, cache_pc current_entry);
cache_pc get_alternate_ibl_routine(dcontext_t *dcontext, cache_pc current_entry,
                                   uint flags);

ibl_source_fragment_type_t
get_source_fragment_type(dcontext_t *dcontext, uint fragment_flags);

const char *get_target_delete_entry_name(dcontext_t *dcontext,
                                         cache_pc target, 
                                         const char **ibl_brtype_name);

#define GET_IBL_TARGET_TABLE(branch_type, target_trace_table)               \
    ((target_trace_table) ? offsetof(per_thread_t, trace_ibt[(branch_type)]) : \
     offsetof(per_thread_t, bb_ibt[(branch_type)]))


#ifdef RETURN_STACK
cache_pc return_lookup_routine(dcontext_t *dcontext);
cache_pc unlinked_return_routine(dcontext_t *dcontext);
#endif
#ifdef WINDOWS
/* PR 282576: These separate routines are ugly, but less ugly than adding param to
 * after_shared_syscall_code(), which is called in many places and usually passed a
 * non-global dcontext; also less ugly than adding GLOBAL_DCONTEXT_X86.
 */
cache_pc
shared_syscall_routine_ex(dcontext_t *dcontext _IF_X64(gencode_mode_t mode));
cache_pc
unlinked_shared_syscall_routine_ex(dcontext_t *dcontext _IF_X64(gencode_mode_t mode));
cache_pc shared_syscall_routine(dcontext_t *dcontext);
cache_pc unlinked_shared_syscall_routine(dcontext_t *dcontext);
#endif
#ifdef TRACE_HEAD_CACHE_INCR
cache_pc trace_head_incr_routine(dcontext_t *dcontext);
cache_pc trace_head_incr_shared_routine(IF_X64(gencode_mode_t mode));
#endif

/* in mangle.c but not exported to non-x86 files */
/* clean call optimization */
void
mangle_init(void);
void
mangle_exit(void);
bool
analyze_clean_call(dcontext_t *dcontext, clean_call_info_t *cci, instr_t *where,
                   void *callee, bool save_fpstate, uint num_args, opnd_t *args);
void
insert_inline_clean_call(dcontext_t *dcontext, clean_call_info_t *cci,
                         instrlist_t *ilist, instr_t *where, opnd_t *args);

void mangle_insert_clone_code(dcontext_t *dcontext, instrlist_t *ilist,
                              instr_t *instr, bool skip
                              _IF_X64(gencode_mode_t mode));
void
set_selfmod_sandbox_offsets(dcontext_t *dcontext);
uint
insert_push_all_registers(dcontext_t *dcontext, clean_call_info_t *cci,
                          instrlist_t *ilist, instr_t *instr,
                          uint alignment, instr_t *push_pc);
void
insert_pop_all_registers(dcontext_t *dcontext, clean_call_info_t *cci,
                         instrlist_t *ilist, instr_t *instr,
                         uint alignment);
bool
parameters_stack_padded(void);
/* Inserts a complete call to callee with the passed-in arguments */
void
insert_meta_call_vargs(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr,
                       bool clean_call, void *callee, uint num_args, opnd_t *args);
void
insert_get_mcontext_base(dcontext_t *dcontext, instrlist_t *ilist, 
                         instr_t *where, reg_id_t reg);
uint
prepare_for_clean_call(dcontext_t *dcontext, clean_call_info_t *cci,
                       instrlist_t *ilist, instr_t *instr);
void
cleanup_after_clean_call(dcontext_t *dcontext, clean_call_info_t *cci,
                         instrlist_t *ilist, instr_t *instr);
void convert_to_near_rel(dcontext_t *dcontext, instr_t *instr);
instr_t *convert_to_near_rel_meta(dcontext_t *dcontext, instrlist_t *ilist,
                                  instr_t *instr);
int find_syscall_num(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr);
bool insert_selfmod_sandbox(dcontext_t *dcontext, instrlist_t *ilist, uint flags,
                            app_pc start_pc, app_pc end_pc, /* end is open */
                            bool record_translation, bool for_cache);

/* offsets within local_state_t used for specific scratch purposes */
enum {
    /* ok for this guy to overlap w/ others since he is pre-cache */
    FCACHE_ENTER_TARGET_SLOT    = TLS_XAX_SLOT,
    /* FIXME: put register name in each enum name to avoid conflicts
     * when mixed with raw slot names?
     */
    /* ok for the next_tag and direct_stub to overlap as next_tag is
     * used for sysenter shared syscall mangling, which uses an
     * indirect stub.
     */
    MANGLE_NEXT_TAG_SLOT        = TLS_XAX_SLOT,
    DIRECT_STUB_SPILL_SLOT      = TLS_XAX_SLOT,
    MANGLE_RIPREL_SPILL_SLOT    = TLS_XAX_SLOT,
    /* ok for far cti mangling/far ibl and stub/ibl xbx slot usage to overlap */
    INDIRECT_STUB_SPILL_SLOT    = TLS_XBX_SLOT,
    MANGLE_FAR_SPILL_SLOT       = TLS_XBX_SLOT,
    MANGLE_XCX_SPILL_SLOT       = TLS_XCX_SLOT,
    /* FIXME: edi is used as the base, yet I labeled this slot for edx
     * since it's next in the progression -- change one or the other?
     * (this is case 5239)
     */
    DCONTEXT_BASE_SPILL_SLOT    = TLS_XDX_SLOT,
    PREFIX_XAX_SPILL_SLOT       = TLS_XAX_SLOT,
#ifdef HASHTABLE_STATISTICS
    HTABLE_STATS_SPILL_SLOT     = TLS_HTABLE_STATS_SLOT,
#endif
};

void mangle(dcontext_t *dcontext, instrlist_t *ilist, uint flags,
            bool mangle_calls, bool record_translation);

/* in interp.c but not exported to non-x86 files */
bool must_not_be_inlined(app_pc pc);

/* A simple linker to give us indirection for patching after relocating structures */
typedef struct patch_entry_t {
    union {
        instr_t *instr;       /* used before instructions are encoded */
        size_t   offset;      /* offset in instruction stream */
    } where;
    ptr_uint_t value_location_offset; /* location containing value to be updated */
                 /* offset from dcontext->fragment_field (usually pt->trace.field),
                  * or an absolute address */
    ushort patch_flags; /* whether to use the address of location or its value */
    short  instr_offset; /* desired offset within instruction, 
                            negative offsets are from end of instruction */
} patch_entry_t;

enum {
    MAX_PATCH_ENTRIES = 
#ifdef HASHTABLE_STATISTICS
6 +     /* will need more only for statistics */
#endif  
    7, /* we use 5 normally, 7 w/ -atomic_inlined_linking and inlining */
    /* Patch entry flags */
    /* Patch offset entries for dynamic updates from input variables */
    PATCH_TAKE_ADDRESS      = 0x01, /* use computed address if set, value at address otherwise */
    PATCH_PER_THREAD        = 0x02, /* address is relative to the per_thread_t thread local field */
    PATCH_UNPROT_STAT       = 0x04, /* address is (unprot_ht_statistics_t offs << 16) | (stats offs) */
    /* Patch offset markers update an output variable in encode_with_patch_list */
    PATCH_MARKER            = 0x08, /* if set use only as a static marker */
    PATCH_ASSEMBLE_ABSOLUTE = 0x10, /* if set retrieve an absolute pc into given target address, 
                                      otherwise relative to start pc */
    PATCH_OFFSET_VALID      = 0x20, /* if set use patch_entry_t.where.offset;
                                     * else patch_entry_t.where.instr */
    PATCH_UINT_SIZED        = 0x40, /* if set value is uint-sized; else pointer-sized */
};

typedef enum {
    PATCH_TYPE_ABSOLUTE     = 0x0, /* link with absolute address, updated dynamically */
    PATCH_TYPE_INDIRECT_XDI = 0x1, /* linked with indirection through EDI, no updates */
    PATCH_TYPE_INDIRECT_FS  = 0x2, /* linked with indirection through FS, no updates */
} patch_list_type_t;

typedef struct patch_list_t {
    ushort num_relocations;
    ushort /* patch_list_type_t */ type;
    patch_entry_t entry[MAX_PATCH_ENTRIES];
} patch_list_t;

void
init_patch_list(patch_list_t *patch, patch_list_type_t type);

void
add_patch_marker(patch_list_t *patch, instr_t *instr, ushort patch_flags,
                 short instr_offset, ptr_uint_t *target_offset /* OUT */);

int
encode_with_patch_list(dcontext_t *dcontext, patch_list_t *patch, 
                       instrlist_t *ilist, cache_pc start_pc);

#ifdef X64
/* Shouldn't need to mark as packed.  We order for 6-byte little-endian selector:pc. */
typedef struct _far_ref_t {
    /* We target WOW64 and cross-plaform so no 8-byte Intel-only pc */
    uint pc;
    ushort selector;
} far_ref_t;
#endif

/* Defines book-keeping structures needed for an indirect branch lookup routine */
typedef struct ibl_code_t {
    bool initialized:1; /* currently only used for ibl routines */
    bool thread_shared_routine:1;
    bool ibl_head_is_inlined:1;
    byte *indirect_branch_lookup_routine;
    /* for far ctis (i#823) */
    byte *far_ibl;
    byte *far_ibl_unlinked;
#ifdef X64
    /* PR 257963: trace inline cmp has already saved eflags */
    byte *trace_cmp_entry;
    byte *trace_cmp_unlinked;
    bool x86_mode; /* Is this code for 32-bit (x86 mode)? */
    bool x86_to_x64_mode; /* Does this code use r8-r10 as scratch (for x86_to_x64)? */
    /* for far ctis (i#823) in mixed-mode (i#49) and x86_to_x64 mode (i#751) */
    far_ref_t far_jmp_opnd;
    far_ref_t far_jmp_unlinked_opnd;
#endif
    byte *unlinked_ibl_entry;
    byte *target_delete_entry;
    uint ibl_routine_length;
    /* offsets into ibl routine */
    patch_list_t ibl_patch;
    ibl_branch_type_t branch_type;
    ibl_source_fragment_type_t source_fragment_type;

    /* bookkeeping for the inlined ibl stub template, if inlining */
    byte *inline_ibl_stub_template;
    patch_list_t ibl_stub_patch;
    uint inline_stub_length;
    /* for atomic_inlined_linking we store the linkstub twice so need to update
     * two offsets */
    uint inline_linkstub_first_offs;
    uint inline_linkstub_second_offs;
    uint inline_unlink_offs;
    uint inline_linkedjmp_offs;
    uint inline_unlinkedjmp_offs;

#ifdef HASHTABLE_STATISTICS
    /* need two offsets to get to stats, since in unprotected memory */
    uint unprot_stats_offset; 
    uint hashtable_stats_offset; 
    /* e.g. offsetof(per_thread_t, trace) + offsetof(ibl_table_t, bb_ibl_stats) */
    /* Note hashtable statistics are associated with the hashtable for easier use when sharing IBL routines */
    uint entry_stats_to_lookup_table_offset; /* offset to (entry_stats - lookup_table)  */
#endif 
} ibl_code_t;
    
/* Each thread needs its own copy of these routines, but not all
 * routines here are created in a thread-private: we could save space
 * by splitting into two separate structs.
 *
 * On x64, we only have thread-shared generated routines,
 * including do_syscall and shared_syscall and detach's post-syscall
 * continuation (PR 244737).
 */
typedef struct _generated_code_t {
    byte *fcache_enter;
    byte *fcache_return;
#ifdef WINDOWS_PC_SAMPLE
    byte *fcache_enter_return_end;
#endif

    ibl_code_t trace_ibl[IBL_BRANCH_TYPE_END];
    ibl_code_t bb_ibl[IBL_BRANCH_TYPE_END];
    ibl_code_t coarse_ibl[IBL_BRANCH_TYPE_END];
#ifdef WINDOWS_PC_SAMPLE
    byte *ibl_routines_end;
#endif

#ifdef RETURN_STACK
    byte *return_lookup;
    byte *unlinked_return;
#endif
#ifdef WINDOWS
    /* for the shared_syscalls option */
    ibl_code_t shared_syscall_code;
    byte *shared_syscall;
    byte *unlinked_shared_syscall;
    byte *end_shared_syscall; /* just marks end */
    /* N.B.: these offsets are from the start of unlinked_shared_syscall,
     * not from shared_syscall (which is later)!!!
     */
    /* offsets into shared_syscall routine */
    uint sys_syscall_offs;
    /* where to patch to unlink end of syscall thread-wide */
    uint sys_unlink_offs;
#endif
    byte *do_syscall;
    uint do_syscall_offs; /* offs of pc after actual syscall instr */
#ifdef WINDOWS
    byte *fcache_enter_indirect;
    byte *do_callback_return;
#else
    /* PR 286922: we both need an int and a sys{call,enter} do-syscall for
     * 32-bit apps on 64-bit kernels.  do_syscall is whatever is in
     * vsyscall, while do_int_syscall is hardcoded to use OP_int.
     */
    byte *do_int_syscall;
    uint do_int_syscall_offs; /* offs of pc after actual syscall instr */
    byte *do_clone_syscall;
    uint do_clone_syscall_offs; /* offs of pc after actual syscall instr */
# ifdef VMX86_SERVER
    byte *do_vmkuw_syscall;
    uint do_vmkuw_syscall_offs; /* offs of pc after actual syscall instr */
# endif
#endif
#ifdef LINUX
    /* PR 212290: can't be static code in x86.asm since it can't be PIC */
    byte *new_thread_dynamo_start;
#endif
#ifdef TRACE_HEAD_CACHE_INCR
    byte *trace_head_incr;
#endif
#ifdef CHECK_RETURNS_SSE2
    byte *pextrw;
    byte *pinsrw;
#endif
#ifdef WINDOWS_PC_SAMPLE
    profile_t *profile;
#endif
    /* For control redirection from a syscall.
     * We could make this shared-only and save some space, if we
     * generated a shared fcache_return in all-private-fragment configs.
     */
    byte *reset_exit_stub;

    /* Coarse-grain fragments don't have linkstubs and need custom routines.
     * Direct exits use entrance stubs that record the target app pc,
     * while coarse indirect stubs record the source cache cti.
     */
    /* FIXME: these two return routines are only needed in the global struct */
    byte *fcache_return_coarse;
    byte *trace_head_return_coarse;
#ifdef CLIENT_INTERFACE
    /* i#849: low-overhead xfer for clients */
    byte *client_ibl_xfer;
    uint client_ibl_unlink_offs;
#endif

    bool thread_shared;
    bool writable;
#ifdef X64
    gencode_mode_t gencode_mode; /* mode of this code (x64, x86, x86_to_x64) */
#endif

    /* We store the start of the generated code for simplicity even
     * though it is always right after this struct; if we really need
     * to shrink 4 bytes we can remove this field and replace w/
     * ((char *)TPC_ptr) + sizeof(generated_code_t)
     */
    byte *gen_start_pc; /* start of generated code */
    byte *gen_end_pc; /* end of generated code */
    byte *commit_end_pc; /* end of committed region */
    /* generated code follows, ends at gen_end_pc < commit_end_pc */
} generated_code_t;

/* thread-private generated code */
fcache_enter_func_t fcache_enter_routine(dcontext_t *dcontext);
cache_pc fcache_return_routine(dcontext_t *dcontext);
cache_pc fcache_return_routine_ex(dcontext_t *dcontext _IF_X64(gencode_mode_t mode));

/* thread-shared generated code */
byte * emit_fcache_enter_shared(dcontext_t *dcontext, generated_code_t *code, byte *pc);
byte * emit_fcache_return_shared(dcontext_t *dcontext, generated_code_t *code, byte *pc);
fcache_enter_func_t fcache_enter_shared_routine(dcontext_t *dcontext);
/* the fcache_return routines are queried by get_direct_exit_target and need more
 * direct control than the dcontext
 */
cache_pc fcache_return_shared_routine(IF_X64(gencode_mode_t mode));

/* coarse-grain generated code */
byte * emit_fcache_return_coarse(dcontext_t *dcontext, generated_code_t *code, byte *pc);
byte * emit_trace_head_return_coarse(dcontext_t *dcontext, generated_code_t *code,
                                     byte *pc);
cache_pc fcache_return_coarse_routine(IF_X64(gencode_mode_t mode));
cache_pc trace_head_return_coarse_routine(IF_X64(gencode_mode_t mode));

void protect_generated_code(generated_code_t *code, bool writable);

extern generated_code_t *shared_code;
#ifdef X64
extern generated_code_t *shared_code_x86;
extern generated_code_t *shared_code_x86_to_x64;
#endif

static inline bool
is_shared_gencode(generated_code_t *code)
{
    if (code == NULL) /* since shared_code_x86 in particular can be NULL */
        return false;
#ifdef X64
    return code == shared_code_x86 || code == shared_code ||
           code == shared_code_x86_to_x64;
#else
    return code == shared_code;
#endif
}

extern bool get_x86_mode(dcontext_t *);

static inline generated_code_t *
get_shared_gencode(dcontext_t *dcontext _IF_X64(gencode_mode_t mode))
{
#ifdef X64
    ASSERT(mode != GENCODE_FROM_DCONTEXT || dcontext != GLOBAL_DCONTEXT
           IF_INTERNAL(IF_CLIENT_INTERFACE(|| dynamo_exited)));
# if defined(INTERNAL) || defined(CLIENT_INTERFACE)
    /* PR 302344: this is here only for tracedump_origins */
    if (dynamo_exited && mode == GENCODE_FROM_DCONTEXT && dcontext == GLOBAL_DCONTEXT) {
        if (get_x86_mode(dcontext))
            return X64_CACHE_MODE_DC(dcontext) ? shared_code_x86_to_x64 : shared_code_x86;
        else
            return shared_code;
    }
# endif
    if (mode == GENCODE_X86)
        return shared_code_x86;
    else if (mode == GENCODE_X86_TO_X64)
        return shared_code_x86_to_x64;
    else if (mode == GENCODE_FROM_DCONTEXT && dcontext->x86_mode)
        return X64_CACHE_MODE_DC(dcontext) ? shared_code_x86_to_x64 : shared_code_x86;
    else
        return shared_code;
#else
    return shared_code;
#endif
}

/* PR 244737: thread-private uses shared gencode on x64 */
#define USE_SHARED_GENCODE_ALWAYS() IF_X64_ELSE(true, false)
/* PR 212570: on linux we need a thread-shared do_syscall for our vsyscall hook,
 * if we have TLS and support sysenter (PR 361894) 
 */
#define USE_SHARED_GENCODE()                                         \
    (USE_SHARED_GENCODE_ALWAYS() || IF_LINUX(IF_HAVE_TLS_ELSE(true, false) ||) \
     SHARED_FRAGMENTS_ENABLED() || DYNAMO_OPTION(shared_trace_ibl_routine))

/* returns the thread private code or GLOBAL thread shared code */
static inline generated_code_t*
get_emitted_routines_code(dcontext_t *dcontext _IF_X64(gencode_mode_t mode))
{
    generated_code_t *code;
    /* This routine exists only because GLOBAL_DCONTEXT is not a real dcontext
     * structure. Still, useful to wrap all references to private_code. */
    /* PR 244737: thread-private uses only shared gencode on x64 */
    /* PR 253431: to distinguish shared x86 gencode from x64 gencode, a dcontext
     * must be passed in; use get_shared_gencode() for x64 builds */
    IF_X64(ASSERT(mode != GENCODE_FROM_DCONTEXT || dcontext != GLOBAL_DCONTEXT));
    if (USE_SHARED_GENCODE_ALWAYS() ||
        (USE_SHARED_GENCODE() && dcontext == GLOBAL_DCONTEXT)) {
        code = get_shared_gencode(dcontext _IF_X64(mode));
    } else {
        ASSERT(dcontext != GLOBAL_DCONTEXT);
        /* NOTE thread private code entry points may also refer to shared
         * routines */
        code = (generated_code_t *) dcontext->private_code;
    }
    return code;
}

ibl_code_t *get_ibl_routine_code(dcontext_t *dcontext, ibl_branch_type_t branch_type,
                                 uint fragment_flags);
ibl_code_t *get_ibl_routine_code_ex(dcontext_t *dcontext, ibl_branch_type_t branch_type,
                                    uint fragment_flags _IF_X64(gencode_mode_t mode));

/* in emit_utils.c but not exported to non-x86 files */
byte * emit_inline_ibl_stub(dcontext_t *dcontext, byte *pc, 
                            ibl_code_t *ibl_code, bool target_trace_table);
byte * emit_fcache_enter(dcontext_t *dcontext, generated_code_t *code, byte *pc);
byte * emit_fcache_return(dcontext_t *dcontext, generated_code_t *code, byte *pc);
byte * emit_indirect_branch_lookup(dcontext_t *dcontext, generated_code_t *code, byte *pc,
                                   byte *fcache_return_pc,
                                   bool target_trace_table,
                                   bool inline_ibl_head,
                                   ibl_code_t *ibl_code);
void update_indirect_branch_lookup(dcontext_t *dcontext);

byte *emit_far_ibl(dcontext_t *dcontext, byte *pc, ibl_code_t *ibl_code, cache_pc ibl_tgt
                   _IF_X64(far_ref_t *far_jmp_opnd));

#ifdef RETURN_STACK
byte * emit_return_lookup(dcontext_t *dcontext, byte *pc,
                          byte *indirect_branch_lookup_pc,
                          byte *unlinked_ib_lookup_pc,
                          byte **return_lookup_pc);
#endif

#ifndef WINDOWS
void update_syscalls(dcontext_t *dcontext);
#endif

#ifdef WINDOWS
/* FIXME If we widen the interface any further, do we want to use an options
 * struct or OR-ed flags to replace the bool args? */
byte * emit_shared_syscall(dcontext_t *dcontext, generated_code_t *code, byte *pc,
                           ibl_code_t *ibl_code,
                           patch_list_t *patch,
                           byte *ind_br_lookup_pc,
                           byte *unlinked_ib_lookup_pc,
                           bool target_trace_table,
                           bool inline_ibl_head,
                           bool thread_shared,
                           byte **shared_syscall_pc);

byte *
emit_shared_syscall_dispatch(dcontext_t *dcontext, byte *pc);

byte *
emit_unlinked_shared_syscall_dispatch(dcontext_t *dcontext, byte *pc);

# ifdef CLIENT_INTERFACE
/* i#249: isolate app's PEB by keeping our own copy and swapping on cxt switch */
void
preinsert_swap_peb(dcontext_t *dcontext, instrlist_t *ilist, instr_t *next,
                   bool absolute, reg_id_t reg_dr, reg_id_t reg_scratch, bool to_priv);
# endif

void emit_patch_syscall(dcontext_t *dcontext, byte *target _IF_X64(gencode_mode_t mode));
#endif /* WINDOWS */

byte * emit_do_syscall(dcontext_t *dcontext, generated_code_t *code, byte *pc,
                       byte *fcache_return_pc, bool thread_shared, bool force_int,
                       uint *syscall_offs /*OUT*/);

#ifdef WINDOWS
/* PR 282576: These separate routines are ugly, but less ugly than adding param to
 * the main routines, which are called in many places and usually passed a
 * non-global dcontext; also less ugly than adding GLOBAL_DCONTEXT_X86.
 */
cache_pc
after_shared_syscall_code_ex(dcontext_t *dcontext _IF_X64(gencode_mode_t mode));
cache_pc
after_do_syscall_code_ex(dcontext_t *dcontext _IF_X64(gencode_mode_t mode));

byte * emit_fcache_enter_indirect(dcontext_t *dcontext, generated_code_t *code, byte *pc,
                                  byte *fcache_return_pc);
byte * emit_do_callback_return(dcontext_t *dcontext, byte *pc,
                               byte *fcache_return_pc, bool thread_shared);
#else
byte * emit_do_clone_syscall(dcontext_t *dcontext, generated_code_t *code, byte *pc,
                             byte *fcache_return_pc, bool thread_shared,
                             uint *syscall_offs /*OUT*/);
# ifdef VMX86_SERVER
byte * emit_do_vmkuw_syscall(dcontext_t *dcontext, generated_code_t *code, byte *pc,
                             byte *fcache_return_pc, bool thread_shared,
                             uint *syscall_offs /*OUT*/);
# endif
#endif

#ifdef LINUX
byte * 
emit_new_thread_dynamo_start(dcontext_t *dcontext, byte *pc);

cache_pc get_new_thread_start(dcontext_t *dcontext _IF_X64(gencode_mode_t mode));
#endif

#ifdef TRACE_HEAD_CACHE_INCR
byte *emit_trace_head_incr(dcontext_t *dcontext, byte *pc,
                           byte *fcache_return_pc);
byte * 
emit_trace_head_incr_shared(dcontext_t *dcontext, byte *pc, byte *fcache_return_pc);
#endif

#ifdef CLIENT_INTERFACE
byte *
emit_client_ibl_xfer(dcontext_t *dcontext, byte *pc, generated_code_t *code);
#endif

void
insert_save_eflags(dcontext_t *dcontext, instrlist_t *ilist, instr_t *where,
                   uint flags, bool tls, bool absolute _IF_X64(bool x86_to_x64));
void
insert_restore_eflags(dcontext_t *dcontext, instrlist_t *ilist, instr_t *where,
                      uint flags, bool tls, bool absolute _IF_X64(bool x86_to_x64));

instr_t * create_syscall_instr(dcontext_t *dcontext);

void
append_shared_get_dcontext(dcontext_t *dcontext, instrlist_t *ilist, bool save_xdi);

void
append_shared_restore_dcontext_reg(dcontext_t *dcontext, instrlist_t *ilist);

/* in optimize.c */
instr_t *find_next_self_loop(dcontext_t *dcontext, app_pc tag, instr_t *instr);
void replace_inst(dcontext_t *dcontext, instrlist_t *ilist, instr_t *old, instr_t *new);
void remove_redundant_loads(dcontext_t *dcontext, app_pc tag,
                            instrlist_t *trace);
void remove_dead_code(dcontext_t *dcontext, app_pc tag, 
                      instrlist_t *trace);

#ifdef CHECK_RETURNS_SSE2
/* in retcheck.c */
void check_return_handle_call(dcontext_t *dcontext, instrlist_t *ilist, instr_t *next);
void check_return_handle_return(dcontext_t *dcontext, instrlist_t *ilist, instr_t *next);
void check_return_too_deep(dcontext_t *dcontext,
                           volatile int errno, volatile reg_t eflags,
                           volatile reg_t reg_edi, volatile reg_t reg_esi,
                           volatile reg_t reg_ebp, volatile reg_t reg_esp,
                           volatile reg_t reg_ebx, volatile reg_t reg_edx,
                           volatile reg_t reg_ecx, volatile reg_t reg_eax);
void check_return_too_shallow(dcontext_t *dcontext,
                              volatile int errno, volatile reg_t eflags,
                              volatile reg_t reg_edi, volatile reg_t reg_esi,
                              volatile reg_t reg_ebp, volatile reg_t reg_esp,
                              volatile reg_t reg_ebx, volatile reg_t reg_edx,
                              volatile reg_t reg_ecx, volatile reg_t reg_eax);
void check_return_ra_mangled(dcontext_t *dcontext,
                             volatile int errno, volatile reg_t eflags,
                             volatile reg_t reg_edi, volatile reg_t reg_esi,
                             volatile reg_t reg_ebp, volatile reg_t reg_esp,
                             volatile reg_t reg_ebx, volatile reg_t reg_edx,
                             volatile reg_t reg_ecx, volatile reg_t reg_eax);
#endif

/* experimental native execution feature */
/* in x86_code.c */
void entering_native(void);

#ifdef LINUX
void new_thread_setup(priv_mcontext_t *mc);
#endif

void
get_xmm_vals(priv_mcontext_t *mc);

/* i#350: Fast safe_read without dcontext.  On success or failure, returns the
 * current source pointer.  Requires fault handling to be set up.
 */
void *safe_read_asm(void *dst, const void *src, size_t size);
/* These are labels, not function pointers.  We declare them as functions to
 * prevent loads and stores to these globals from compiling.
 */
void safe_read_asm_pre(void);
void safe_read_asm_mid(void);
void safe_read_asm_post(void);
void safe_read_asm_recover(void);

/* from x86.asm */
/* Note these have specialized calling conventions and shouldn't be called from
 * C code (see comments in x86.asm). */
void global_do_syscall_sysenter(void);
void global_do_syscall_int(void);
void global_do_syscall_sygate_int(void);
void global_do_syscall_sygate_sysenter(void);
# ifdef WINDOWS
void global_do_syscall_wow64(void);
void global_do_syscall_wow64_index0(void);
# endif
#ifdef X64
void global_do_syscall_syscall(void);
#endif
void get_xmm_caller_saved(dr_ymm_t *xmm_caller_saved_buf);
void get_ymm_caller_saved(dr_ymm_t *ymm_caller_saved_buf);

/* in encode.c */
byte *instr_encode_ignore_reachability(dcontext_t *dcontext_t, instr_t *instr, byte *pc);
byte *instr_encode_check_reachability(dcontext_t *dcontext_t, instr_t *instr, byte *pc,
                                      bool *has_instr_opnds/*OUT OPTIONAL*/);
byte *copy_and_re_relativize_raw_instr(dcontext_t *dcontext, instr_t *instr,
                                       byte *dst_pc, byte *final_pc);

/* in instr.c */
uint
move_mm_reg_opcode(bool aligned16, bool aligned32);

/* in mangle.c */
void
insert_push_retaddr(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr,
                    ptr_int_t retaddr, opnd_size_t opsize);

ptr_uint_t
get_call_return_address(dcontext_t *dcontext, instrlist_t *ilist, instr_t *instr);

#ifdef X64
/* in x86_to_x64.c */
void
translate_x86_to_x64(dcontext_t *dcontext, instrlist_t *ilist, INOUT instr_t **instr);
#endif

#endif /* X86_ARCH_H */

