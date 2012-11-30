/*
 * types.h
 *
 *  Created on: 2012-11-08
 *      Author: pag
 *     Version: $Id$
 */

#ifndef Granary_TYPES_H_
#define Granary_TYPES_H_

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

typedef unsigned long long uint64;
typedef long long int64;
typedef unsigned char byte;
typedef unsigned char uchar;
typedef unsigned long int ulong;
typedef unsigned short int ushort;
typedef unsigned int uint;
typedef unsigned long long ptr_uint_t;
typedef long long ptr_int_t;
typedef unsigned char *app_pc;
typedef app_pc cache_pc;
typedef uint64 reg_t;

#ifndef bool
typedef enum {
    true = 1,
    TRUE = 1,
    false = 0,
    FALSE = 0
} bool;
#endif


struct _opnd_t;
typedef struct _opnd_t opnd_t;
struct _instr_t;
typedef struct _instr_t instr_t;
struct _instr_list_t;
struct _fragment_t;
typedef struct _fragment_t fragment_t;
struct _future_fragment_t;
typedef struct _future_fragment_t future_fragment_t;
struct _trace_t;
typedef struct _trace_t trace_t;
struct _linkstub_t;
typedef struct _linkstub_t linkstub_t;
struct vm_area_vector_t;
typedef struct vm_area_vector_t vm_area_vector_t;
struct _coarse_info_t;
typedef struct _coarse_info_t coarse_info_t;
struct _coarse_freeze_info_t;
typedef struct _coarse_freeze_info_t coarse_freeze_info_t;
struct _module_data_t;
/* DR_API EXPORT TOFILE dr_defines.h */
/* DR_API EXPORT BEGIN */
typedef struct _instr_list_t instrlist_t;
typedef struct _module_data_t module_data_t;
/* DR_API EXPORT END */


typedef struct {
    bool x86_mode;      // true iff x64
    void *private_code;
    instr_t *allocated_instr;
} dcontext_t;


/** 256-bit YMM register. */
typedef union _dr_ymm_t {
#ifdef AVOID_API_EXPORT
    /* We avoid having 8-byte-aligned fields here for 32-bit: they cause
     * cl to add padding in app_state_at_intercept_t and unprotected_context_t,
     * which messes up our interception stack layout and our x86.asm offsets.
     * We don't access these very often, so we just omit this field.
     *
     * With the new dr_mcontext_t's size field pushing its ymm field to 0x44
     * having an 8-byte-aligned field here adds 4 bytes padding.
     * We could shrink PRE_XMM_PADDING for client header files but simpler
     * to just have u64 only be there for 64-bit for clients.
     * We do the same thing for dr_xmm_t just to look consistent.
     */
#endif
#ifdef API_EXPORT_ONLY
#ifdef X64
    uint64 u64[4]; /**< Representation as 4 64-bit integers. */
#endif
#endif
    uint   u32[8]; /**< Representation as 8 32-bit integers. */
    byte   u8[32]; /**< Representation as 32 8-bit integers. */
    reg_t  reg[IF_X64_ELSE(4,8)]; /**< Representation as 4 or 8 registers. */
} dr_ymm_t;

typedef struct {
    reg_t xax, xbx, xcx, xdx, xsi, xdi, xbp, xsp, xflags, pc;
    reg_t r8, r9, r10, r11, r12, r13, r14, r15;
    dr_ymm_t ymm;
} dr_mcontext_t;
typedef dr_mcontext_t priv_mcontext_t;
typedef struct {
    dr_mcontext_t mcontext;
} unprotected_context_t;

typedef enum {
    IBL_NONE = -1,
    /* N.B.: order determines which table is on 2nd cache line in local_state_t */
    IBL_RETURN = 0, /* returns lookup routine has stricter requirements */
    IBL_BRANCH_TYPE_START = IBL_RETURN,
    IBL_INDCALL,
    IBL_INDJMP,
    IBL_GENERIC           = IBL_INDJMP, /* currently least restrictive */
    /* can double if a generic lookup is needed
       FIXME: remove this and add names for specific needs */
    IBL_SHARED_SYSCALL    = IBL_GENERIC,
    IBL_BRANCH_TYPE_END
} ibl_branch_type_t;


enum {
    MAX_INSTR_LENGTH = 17,
    /* size of 32-bit-offset jcc instr, assuming it has no
     * jcc branch hint!
     */
    CBR_LONG_LENGTH  = 6,
    JMP_LONG_LENGTH  = 5,
    JMP_SHORT_LENGTH = 2,
    CBR_SHORT_REWRITE_LENGTH = 9, /* FIXME: use this in mangle.c */
    RET_0_LENGTH     = 1,
    PUSH_IMM32_LENGTH = 5,

    /* size of 32-bit call and jmp instructions w/o prefixes. */
    CTI_IND1_LENGTH    = 2, /* FF D6             call esi                      */
    CTI_IND2_LENGTH    = 3, /* FF 14 9E          call dword ptr [esi+ebx*4]    */
    CTI_IND3_LENGTH    = 4, /* FF 54 B3 08       call dword ptr [ebx+esi*4+8]  */
    CTI_DIRECT_LENGTH  = 5, /* E8 9A 0E 00 00    call 7C8024CB                 */
    CTI_IAT_LENGTH     = 6, /* FF 15 38 10 80 7C call dword ptr ds:[7C801038h] */
    CTI_FAR_ABS_LENGTH = 7, /* 9A 1B 07 00 34 39 call 0739:3400071B            */
                            /* 07                                              */

    INT_LENGTH = 2,
    SYSCALL_LENGTH = 2,
    SYSENTER_LENGTH = 2,
};

typedef void (*fcache_enter_func_t)(void);
typedef void (*generic_func_t)();

extern dcontext_t DCONTEXT;
extern dcontext_t *get_thread_private_dcontext(void);
extern bool set_x86_mode(dcontext_t *dcontext, bool x86);
extern uint atomic_swap(uint *addr, uint value);
extern bool cpuid_supported(void);
extern void our_cpuid(int res[4], int eax);

#endif /* Granary_TYPES_H_ */
