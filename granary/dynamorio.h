/* **********************************************************
 * Copyright (c) 2011-2012 Google, Inc.  All rights reserved.
 * Copyright (c) 2002-2010 VMware, Inc.  All rights reserved.
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
/* Copyright (c) 2002-2003 Massachusetts Institute of Technology */

#ifndef Granary_DYNAMORIO_H_
#define Granary_DYNAMORIO_H_

#ifndef GRANARY_DONT_INCLUDE_CSTDLIB
#   include <string.h>
#endif

#ifndef X64
#   define X64 1
#endif

#define PREFIX_LOCK           0x1 /**< Makes the instruction's memory accesses atomic. */
#define PREFIX_JCC_NOT_TAKEN  0x2 /**< Branch hint: conditional branch is taken. */
#define PREFIX_JCC_TAKEN      0x4 /**< Branch hint: conditional branch is not taken. */
/* These are used only in the decoding tables.  We decode the
 * information into the operands.
 * For encoding these properties are specified in the operands,
 * with our encoder auto-adding the appropriate prefixes.
 */
#define PREFIX_DATA           0x0008
#define PREFIX_ADDR           0x0010
#define PREFIX_REX_W          0x0020
#define PREFIX_REX_R          0x0040
#define PREFIX_REX_X          0x0080
#define PREFIX_REX_B          0x0100
#define PREFIX_REX_GENERAL    0x0200 /* 0x40: only matters for SPL...SDL vs AH..BH */
#define PREFIX_REX_ALL        (PREFIX_REX_W|PREFIX_REX_R|PREFIX_REX_X|PREFIX_REX_B|\
                               PREFIX_REX_GENERAL)
#define PREFIX_SIZE_SPECIFIERS (PREFIX_DATA|PREFIX_ADDR|PREFIX_REX_ALL)

/* Unused except in decode tables (we encode the prefix into the opcodes) */
#define PREFIX_REP            0x0400
#define PREFIX_REPNE          0x0800

/* PREFIX_SEG_* is set by decode or decode_cti and is only a hint
 * to the caller.  Is ignored by encode in favor of the segment
 * reg specified in the applicable opnds.  We rely on it being set during
 * bb building.
 */
#define PREFIX_SEG_FS         0x1000
#define PREFIX_SEG_GS         0x2000

/* First two are only used during initial decode so if running out of
 * space could replace w/ byte value compare.
 */
#define PREFIX_VEX_2B    0x000004000
#define PREFIX_VEX_3B    0x000008000
#define PREFIX_VEX_L     0x000010000

/****************************************************************************
 * EFLAGS
 */
/* we only care about these 11 flags, and mostly only about the first 6
 * we consider an undefined effect on a flag to be a write
 */
#define EFLAGS_READ_CF   0x00000001 /**< Reads CF (Carry Flag). */
#define EFLAGS_READ_PF   0x00000002 /**< Reads PF (Parity Flag). */
#define EFLAGS_READ_AF   0x00000004 /**< Reads AF (Auxiliary Carry Flag). */
#define EFLAGS_READ_ZF   0x00000008 /**< Reads ZF (Zero Flag). */
#define EFLAGS_READ_SF   0x00000010 /**< Reads SF (Sign Flag). */
#define EFLAGS_READ_TF   0x00000020 /**< Reads TF (Trap Flag). */
#define EFLAGS_READ_IF   0x00000040 /**< Reads IF (Interrupt Enable Flag). */
#define EFLAGS_READ_DF   0x00000080 /**< Reads DF (Direction Flag). */
#define EFLAGS_READ_OF   0x00000100 /**< Reads OF (Overflow Flag). */
#define EFLAGS_READ_NT   0x00000200 /**< Reads NT (Nested Task). */
#define EFLAGS_READ_RF   0x00000400 /**< Reads RF (Resume Flag). */
#define EFLAGS_WRITE_CF  0x00000800 /**< Writes CF (Carry Flag). */
#define EFLAGS_WRITE_PF  0x00001000 /**< Writes PF (Parity Flag). */
#define EFLAGS_WRITE_AF  0x00002000 /**< Writes AF (Auxiliary Carry Flag). */
#define EFLAGS_WRITE_ZF  0x00004000 /**< Writes ZF (Zero Flag). */
#define EFLAGS_WRITE_SF  0x00008000 /**< Writes SF (Sign Flag). */
#define EFLAGS_WRITE_TF  0x00010000 /**< Writes TF (Trap Flag). */
#define EFLAGS_WRITE_IF  0x00020000 /**< Writes IF (Interrupt Enable Flag). */
#define EFLAGS_WRITE_DF  0x00040000 /**< Writes DF (Direction Flag). */
#define EFLAGS_WRITE_OF  0x00080000 /**< Writes OF (Overflow Flag). */
#define EFLAGS_WRITE_NT  0x00100000 /**< Writes NT (Nested Task). */
#define EFLAGS_WRITE_RF  0x00200000 /**< Writes RF (Resume Flag). */

#define EFLAGS_READ_ALL  0x000007ff /**< Reads all flags. */
#define EFLAGS_WRITE_ALL 0x003ff800 /**< Writes all flags. */

#ifdef __cplusplus
namespace dynamorio {
extern "C" {
#endif

typedef unsigned short ushort;

typedef long long ssize_t;

typedef unsigned int uint;

typedef unsigned long long uint64;

typedef long long int64;

typedef unsigned char byte;

typedef unsigned char uchar;

typedef unsigned long long ptr_uint_t;

typedef long long ptr_int_t;

typedef unsigned char * app_pc;

typedef app_pc cache_pc;

typedef uint64 reg_t;

//typedef enum { true = 1 , TRUE = 1 , false = 0 , FALSE = 0 } bool ;

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

typedef struct _instr_list_t instrlist_t;

typedef struct _module_data_t module_data_t;

typedef struct {
    bool x86_mode;
    void * private_code;
    instr_t *allocated_instr;
} dcontext_t;

typedef union _dr_ymm_t {
    uint64 u64[4];
    uint u32[8];
    byte u8[32];
    reg_t reg[4];
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
    IBL_RETURN = 0,
    IBL_BRANCH_TYPE_START = IBL_RETURN,
    IBL_INDCALL,
    IBL_INDJMP,
    IBL_GENERIC = IBL_INDJMP,
    IBL_SHARED_SYSCALL = IBL_GENERIC,
    IBL_BRANCH_TYPE_END
} ibl_branch_type_t;

/*enum type*/

enum {
    MAX_INSTR_LENGTH = 17,
    CBR_LONG_LENGTH = 6,
    JMP_LONG_LENGTH = 5,
    JMP_SHORT_LENGTH = 2,
    CBR_SHORT_REWRITE_LENGTH = 9,
    RET_0_LENGTH = 1,
    PUSH_IMM32_LENGTH = 5,
    CTI_IND1_LENGTH = 2,
    CTI_IND2_LENGTH = 3,
    CTI_IND3_LENGTH = 4,
    CTI_DIRECT_LENGTH = 5,
    CTI_IAT_LENGTH = 6,
    CTI_FAR_ABS_LENGTH = 7,
    INT_LENGTH = 2,
    SYSCALL_LENGTH = 2,
    SYSENTER_LENGTH = 2
};

typedef void (*fcache_enter_func_t)(void);

typedef void (*generic_func_t)();

enum {
    VENDOR_INTEL, VENDOR_AMD, VENDOR_UNKNOWN
};

typedef struct {
    uint flags_edx;
    uint flags_ecx;
    uint ext_flags_edx;
    uint ext_flags_ecx;
} features_t;

typedef enum {
    FEATURE_FPU = 0,
    FEATURE_VME = 1,
    FEATURE_DE = 2,
    FEATURE_PSE = 3,
    FEATURE_TSC = 4,
    FEATURE_MSR = 5,
    FEATURE_PAE = 6,
    FEATURE_MCE = 7,
    FEATURE_CX8 = 8,
    FEATURE_APIC = 9,
    FEATURE_SEP = 11,
    FEATURE_MTRR = 12,
    FEATURE_PGE = 13,
    FEATURE_MCA = 14,
    FEATURE_CMOV = 15,
    FEATURE_PAT = 16,
    FEATURE_PSE_36 = 17,
    FEATURE_PSN = 18,
    FEATURE_CLFSH = 19,
    FEATURE_DS = 21,
    FEATURE_ACPI = 22,
    FEATURE_MMX = 23,
    FEATURE_FXSR = 24,
    FEATURE_SSE = 25,
    FEATURE_SSE2 = 26,
    FEATURE_SS = 27,
    FEATURE_HTT = 28,
    FEATURE_TM = 29,
    FEATURE_IA64 = 30,
    FEATURE_PBE = 31,
    FEATURE_SSE3 = 0 + 32,
    FEATURE_PCLMULQDQ = 1 + 32,
    FEATURE_MONITOR = 3 + 32,
    FEATURE_DS_CPL = 4 + 32,
    FEATURE_VMX = 5 + 32,
    FEATURE_EST = 7 + 32,
    FEATURE_TM2 = 8 + 32,
    FEATURE_SSSE3 = 9 + 32,
    FEATURE_CID = 10 + 32,
    FEATURE_FMA = 12 + 32,
    FEATURE_CX16 = 13 + 32,
    FEATURE_xPTR = 14 + 32,
    FEATURE_SSE41 = 19 + 32,
    FEATURE_SSE42 = 20 + 32,
    FEATURE_MOVBE = 22 + 32,
    FEATURE_POPCNT = 23 + 32,
    FEATURE_AES = 25 + 32,
    FEATURE_XSAVE = 26 + 32,
    FEATURE_OSXSAVE = 27 + 32,
    FEATURE_AVX = 28 + 32,
    FEATURE_SYSCALL = 11 + 64,
    FEATURE_XD_Bit = 20 + 64,
    FEATURE_EM64T = 29 + 64,
    FEATURE_LAHF = 0 + 96
} feature_bit_t;

typedef enum {
    CACHE_SIZE_8_KB,
    CACHE_SIZE_16_KB,
    CACHE_SIZE_32_KB,
    CACHE_SIZE_64_KB,
    CACHE_SIZE_128_KB,
    CACHE_SIZE_256_KB,
    CACHE_SIZE_512_KB,
    CACHE_SIZE_1_MB,
    CACHE_SIZE_2_MB,
    CACHE_SIZE_UNKNOWN
} cache_size_t;

enum {
    LINK_DIRECT = 0x0001,
    LINK_INDIRECT = 0x0002,
    LINK_RETURN = 0x0004,
    LINK_CALL = 0x0008,
    LINK_JMP = 0x0010,
    LINK_FAR = 0x0020,
    LINK_SELFMOD_EXIT = 0x0040,
    LINK_TRACE_CMP = 0x0100,
    LINK_NI_SYSCALL_INT = 0x0200,
    LINK_NI_SYSCALL = 0x0400,
    LINK_FINAL_INSTR_SHARED_FLAG = LINK_NI_SYSCALL,
    LINK_FRAG_OFFS_AT_END = 0x0800,
    LINK_END_OF_LIST = 0x1000,
    LINK_FAKE = 0x2000,
    LINK_LINKED = 0x4000,
    LINK_SEPARATE_STUB = 0x8000
};

struct _linkstub_t {
    ushort flags;
    ushort cti_offset;
};

typedef struct _common_direct_linkstub_t {
    linkstub_t l;
    linkstub_t * next_incoming;
} common_direct_linkstub_t;

typedef struct _direct_linkstub_t {
    common_direct_linkstub_t cdl;
    app_pc target_tag;
    cache_pc stub_pc;
} direct_linkstub_t;

typedef struct _cbr_fallthrough_linkstub_t {
    common_direct_linkstub_t cdl;
} cbr_fallthrough_linkstub_t;

typedef struct _indirect_linkstub_t {
    linkstub_t l;
} indirect_linkstub_t;

typedef struct _post_linkstub_t {
    ushort fragment_offset;
    ushort padding;
} post_linkstub_t;

typedef struct _coarse_incoming_t {
    union {
        cache_pc stub_pc;
        linkstub_t * fine_l;
    } in;
    bool coarse;
    struct _coarse_incoming_t * next;
} coarse_incoming_t;

struct _instr_list_t {
    instr_t * first;
    instr_t * last;
    int flags;
    app_pc translation_target;
    app_pc fall_through_bb;
};

struct instr_info_t;

enum register_type {
    DR_REG_NULL,
    DR_REG_RAX,
    DR_REG_RCX,
    DR_REG_RDX,
    DR_REG_RBX,
    DR_REG_RSP,
    DR_REG_RBP,
    DR_REG_RSI,
    DR_REG_RDI,
    DR_REG_R8,
    DR_REG_R9,
    DR_REG_R10,
    DR_REG_R11,
    DR_REG_R12,
    DR_REG_R13,
    DR_REG_R14,
    DR_REG_R15,

    DR_REG_EAX,
    DR_REG_ECX,
    DR_REG_EDX,
    DR_REG_EBX,
    DR_REG_ESP,
    DR_REG_EBP,
    DR_REG_ESI,
    DR_REG_EDI,
    DR_REG_R8D,
    DR_REG_R9D,
    DR_REG_R10D,
    DR_REG_R11D,
    DR_REG_R12D,
    DR_REG_R13D,
    DR_REG_R14D,
    DR_REG_R15D,

    DR_REG_AX,
    DR_REG_CX,
    DR_REG_DX,
    DR_REG_BX,
    DR_REG_SP,
    DR_REG_BP,
    DR_REG_SI,
    DR_REG_DI,
    DR_REG_R8W,
    DR_REG_R9W,
    DR_REG_R10W,
    DR_REG_R11W,
    DR_REG_R12W,
    DR_REG_R13W,
    DR_REG_R14W,
    DR_REG_R15W,

    DR_REG_AL,
    DR_REG_CL,
    DR_REG_DL,
    DR_REG_BL,
    DR_REG_AH,
    DR_REG_CH,
    DR_REG_DH,
    DR_REG_BH,
    DR_REG_R8L,
    DR_REG_R9L,
    DR_REG_R10L,
    DR_REG_R11L,
    DR_REG_R12L,
    DR_REG_R13L,
    DR_REG_R14L,
    DR_REG_R15L,

    DR_REG_SPL,
    DR_REG_BPL,
    DR_REG_SIL,
    DR_REG_DIL,
    DR_REG_MM0,
    DR_REG_MM1,
    DR_REG_MM2,
    DR_REG_MM3,
    DR_REG_MM4,
    DR_REG_MM5,
    DR_REG_MM6,
    DR_REG_MM7,
    DR_REG_XMM0,
    DR_REG_XMM1,
    DR_REG_XMM2,
    DR_REG_XMM3,
    DR_REG_XMM4,
    DR_REG_XMM5,
    DR_REG_XMM6,
    DR_REG_XMM7,
    DR_REG_XMM8,
    DR_REG_XMM9,
    DR_REG_XMM10,
    DR_REG_XMM11,
    DR_REG_XMM12,
    DR_REG_XMM13,
    DR_REG_XMM14,
    DR_REG_XMM15,
    DR_REG_ST0,
    DR_REG_ST1,
    DR_REG_ST2,
    DR_REG_ST3,
    DR_REG_ST4,
    DR_REG_ST5,
    DR_REG_ST6,
    DR_REG_ST7,
    DR_SEG_ES,
    DR_SEG_CS,
    DR_SEG_SS,
    DR_SEG_DS,
    DR_SEG_FS,
    DR_SEG_GS,
    DR_REG_DR0,
    DR_REG_DR1,
    DR_REG_DR2,
    DR_REG_DR3,
    DR_REG_DR4,
    DR_REG_DR5,
    DR_REG_DR6,
    DR_REG_DR7,
    DR_REG_DR8,
    DR_REG_DR9,
    DR_REG_DR10,
    DR_REG_DR11,
    DR_REG_DR12,
    DR_REG_DR13,
    DR_REG_DR14,
    DR_REG_DR15,
    DR_REG_CR0,
    DR_REG_CR1,
    DR_REG_CR2,
    DR_REG_CR3,
    DR_REG_CR4,
    DR_REG_CR5,
    DR_REG_CR6,
    DR_REG_CR7,
    DR_REG_CR8,
    DR_REG_CR9,
    DR_REG_CR10,
    DR_REG_CR11,
    DR_REG_CR12,
    DR_REG_CR13,
    DR_REG_CR14,
    DR_REG_CR15,
    DR_REG_INVALID,
    DR_REG_YMM0,
    DR_REG_YMM1,
    DR_REG_YMM2,
    DR_REG_YMM3,
    DR_REG_YMM4,
    DR_REG_YMM5,
    DR_REG_YMM6,
    DR_REG_YMM7,
    DR_REG_YMM8,
    DR_REG_YMM9,
    DR_REG_YMM10,
    DR_REG_YMM11,
    DR_REG_YMM12,
    DR_REG_YMM13,
    DR_REG_YMM14,
    DR_REG_YMM15
};

typedef byte reg_id_t;

typedef byte opnd_size_t;

struct _opnd_t {
    byte kind;
    opnd_size_t size;
    union {
        ushort far_pc_seg_selector;
        reg_id_t segment :8;
        ushort disp;
    } seg;
    union {
        ptr_int_t immed_int;
        float immed_float;
        app_pc pc;
        instr_t * instr;
        reg_id_t reg;
        struct {
            int disp;
            reg_id_t base_reg :8;
            reg_id_t index_reg :8;
            byte scale :4;
            byte encode_zero_disp :1;
            byte force_full_disp :1;
            byte disp_short_addr :1;
        } base_disp;
        void * addr;
    } value;
};

enum op_kind_type {
    NULL_kind,
    IMMED_INTEGER_kind,
    IMMED_FLOAT_kind,
    PC_kind,
    INSTR_kind,
    REG_kind,
    BASE_DISP_kind,
    FAR_PC_kind,
    FAR_INSTR_kind,
    REL_ADDR_kind,
    ABS_ADDR_kind,
    MEM_INSTR_kind,
    LAST_kind
};

enum {
    INSTR_DIRECT_EXIT = LINK_DIRECT,
    INSTR_INDIRECT_EXIT = LINK_INDIRECT,
    INSTR_RETURN_EXIT = LINK_RETURN,
    INSTR_CALL_EXIT = LINK_CALL,
    INSTR_JMP_EXIT = LINK_JMP,
    INSTR_IND_JMP_PLT_EXIT = (INSTR_JMP_EXIT | INSTR_CALL_EXIT ),
    INSTR_FAR_EXIT = LINK_FAR,
    INSTR_BRANCH_SELFMOD_EXIT = LINK_SELFMOD_EXIT,
    INSTR_TRACE_CMP_EXIT = LINK_TRACE_CMP,
    INSTR_NI_SYSCALL_INT = LINK_NI_SYSCALL_INT,
    INSTR_NI_SYSCALL = LINK_NI_SYSCALL,
    INSTR_NI_SYSCALL_ALL = (LINK_NI_SYSCALL | LINK_NI_SYSCALL_INT ),
    EXIT_CTI_TYPES = (INSTR_DIRECT_EXIT | INSTR_INDIRECT_EXIT
            | INSTR_RETURN_EXIT | INSTR_CALL_EXIT | INSTR_JMP_EXIT
            | INSTR_FAR_EXIT | INSTR_BRANCH_SELFMOD_EXIT | INSTR_TRACE_CMP_EXIT
            | INSTR_NI_SYSCALL_INT | INSTR_NI_SYSCALL ),
    INSTR_OPERANDS_VALID = 0x00010000,
    INSTR_FIRST_NON_LINK_SHARED_FLAG = INSTR_OPERANDS_VALID,
    INSTR_EFLAGS_VALID = 0x00020000,
    INSTR_EFLAGS_6_VALID = 0x00040000,
    INSTR_RAW_BITS_VALID = 0x00080000,
    INSTR_RAW_BITS_ALLOCATED = 0x00100000,
    INSTR_DO_NOT_MANGLE = 0x00200000,
    INSTR_HAS_CUSTOM_STUB = 0x00400000,
    INSTR_IND_CALL_DIRECT = 0x00800000,
    INSTR_CLOBBER_RETADDR = 0x02000000,
    INSTR_HOT_PATCHABLE = 0x04000000,
    INSTR_DO_NOT_EMIT = 0x10000000,
    INSTR_RIP_REL_VALID = 0x20000000,
    INSTR_X86_MODE = 0x40000000,
    INSTR_OUR_MANGLING = 0x80000000
};

typedef struct _dr_instr_label_data_t {
    ptr_uint_t data[4];
} dr_instr_label_data_t;

struct _instr_t {
    uint flags;
    byte * bytes;
    uint length;
    app_pc translation;
    uint16_t opcode;
    uint16_t granary_policy; // code cache policy for granary
    uint8_t granary_flags; // flags, e.g. delay, don't mangle, etc.
    byte rip_rel_pos;
    byte num_dsts;
    byte num_srcs;
    union {
        struct {
            opnd_t src0;
            opnd_t * srcs;
            opnd_t * dsts;
        } o;
        dr_instr_label_data_t label_data;
    } u;
    uint prefixes;
    uint eflags;
    void * note;
    instr_t * prev;
    instr_t * next;
};

enum {
    EFLAGS_CF = 0x00000001,
    EFLAGS_PF = 0x00000004,
    EFLAGS_AF = 0x00000010,
    EFLAGS_ZF = 0x00000040,
    EFLAGS_SF = 0x00000080,
    EFLAGS_DF = 0x00000400,
    EFLAGS_OF = 0x00000800
};

enum {
    RAW_OPCODE_nop = 0x90,
    RAW_OPCODE_jmp_short = 0xeb,
    RAW_OPCODE_call = 0xe8,
    RAW_OPCODE_ret = 0xc3,
    RAW_OPCODE_jmp = 0xe9,
    RAW_OPCODE_push_imm32 = 0x68,
    RAW_OPCODE_jcc_short_start = 0x70,
    RAW_OPCODE_jcc_short_end = 0x7f,
    RAW_OPCODE_jcc_byte1 = 0x0f,
    RAW_OPCODE_jcc_byte2_start = 0x80,
    RAW_OPCODE_jcc_byte2_end = 0x8f,
    RAW_OPCODE_loop_start = 0xe0,
    RAW_OPCODE_loop_end = 0xe3,
    RAW_OPCODE_lea = 0x8d,
    RAW_PREFIX_jcc_not_taken = 0x2e,
    RAW_PREFIX_jcc_taken = 0x3e,
    RAW_PREFIX_lock = 0xf0
};

enum {
    FS_SEG_OPCODE = 0x64,
    GS_SEG_OPCODE = 0x65,
    TLS_SEG_OPCODE = GS_SEG_OPCODE,
    DATA_PREFIX_OPCODE = 0x66,
    ADDR_PREFIX_OPCODE = 0x67,
    REPNE_PREFIX_OPCODE = 0xf2,
    REP_PREFIX_OPCODE = 0xf3,
    REX_PREFIX_BASE_OPCODE = 0x40,
    REX_PREFIX_W_OPFLAG = 0x8,
    REX_PREFIX_R_OPFLAG = 0x4,
    REX_PREFIX_X_OPFLAG = 0x2,
    REX_PREFIX_B_OPFLAG = 0x1,
    REX_PREFIX_ALL_OPFLAGS = 0xf,
    MOV_REG2MEM_OPCODE = 0x89,
    MOV_MEM2REG_OPCODE = 0x8b,
    MOV_XAX2MEM_OPCODE = 0xa3,
    MOV_MEM2XAX_OPCODE = 0xa1,
    MOV_IMM2XAX_OPCODE = 0xb8,
    MOV_IMM2XBX_OPCODE = 0xbb,
    MOV_IMM2MEM_OPCODE = 0xc7,
    JECXZ_OPCODE = 0xe3,
    JMP_SHORT_OPCODE = 0xeb,
    JMP_OPCODE = 0xe9,
    JNE_OPCODE_1 = 0x0f,
    SAHF_OPCODE = 0x9e,
    LAHF_OPCODE = 0x9f,
    SETO_OPCODE_1 = 0x0f,
    SETO_OPCODE_2 = 0x90,
    ADD_AL_OPCODE = 0x04,
    INC_MEM32_OPCODE_1 = 0xff,
    MODRM16_DISP16 = 0x06,
    SIB_DISP32 = 0x25
};

enum {
    NUM_REGPARM = 6,
    REGPARM_0 = DR_REG_RDI,
    REGPARM_1 = DR_REG_RSI,
    REGPARM_2 = DR_REG_RDX,
    REGPARM_3 = DR_REG_RCX,
    REGPARM_4 = DR_REG_R8,
    REGPARM_5 = DR_REG_R9,
    REGPARM_MINSTACK = 0,
    REDZONE_SIZE = 128,
    REGPARM_END_ALIGN = 16
};

enum op_code_type {
    OP_INVALID,
    OP_UNDECODED,
    OP_CONTD,
    OP_LABEL,
    OP_add,
    OP_or,
    OP_adc,
    OP_sbb,
    OP_and,
    OP_daa,
    OP_sub,
    OP_das,
    OP_xor,
    OP_aaa,
    OP_cmp,
    OP_aas,
    OP_inc,
    OP_dec,
    OP_push,
    OP_push_imm,
    OP_pop,
    OP_pusha,
    OP_popa,
    OP_bound,
    OP_arpl,
    OP_imul,
    OP_jo_short,
    OP_jno_short,
    OP_jb_short,
    OP_jnb_short,
    OP_jz_short,
    OP_jnz_short,
    OP_jbe_short,
    OP_jnbe_short,
    OP_js_short,
    OP_jns_short,
    OP_jp_short,
    OP_jnp_short,
    OP_jl_short,
    OP_jnl_short,
    OP_jle_short,
    OP_jnle_short,
    OP_call,
    OP_call_ind,
    OP_call_far,
    OP_call_far_ind,
    OP_jmp,
    OP_jmp_short,
    OP_jmp_ind,
    OP_jmp_far,
    OP_jmp_far_ind,
    OP_loopne,
    OP_loope,
    OP_loop,
    OP_jecxz,
    OP_mov_ld,
    OP_mov_st,
    OP_mov_imm,
    OP_mov_seg,
    OP_mov_priv,
    OP_test,
    OP_lea,
    OP_xchg,
    OP_cwde,
    OP_cdq,
    OP_fwait,
    OP_pushf,
    OP_popf,
    OP_sahf,
    OP_lahf,
    OP_ret,
    OP_ret_far,
    OP_les,
    OP_lds,
    OP_enter,
    OP_leave,
    OP_int3,
    OP_int,
    OP_into,
    OP_iret,
    OP_aam,
    OP_aad,
    OP_xlat,
    OP_in,
    OP_out,
    OP_hlt,
    OP_cmc,
    OP_clc,
    OP_stc,
    OP_cli,
    OP_sti,
    OP_cld,
    OP_std,
    OP_lar,
    OP_lsl,
    OP_syscall,
    OP_clts,
    OP_sysret,
    OP_invd,
    OP_wbinvd,
    OP_ud2a,
    OP_nop_modrm,
    OP_movntps,
    OP_movntpd,
    OP_wrmsr,
    OP_rdtsc,
    OP_rdmsr,
    OP_rdpmc,
    OP_sysenter,
    OP_sysexit,
    OP_cmovo,
    OP_cmovno,
    OP_cmovb,
    OP_cmovnb,
    OP_cmovz,
    OP_cmovnz,
    OP_cmovbe,
    OP_cmovnbe,
    OP_cmovs,
    OP_cmovns,
    OP_cmovp,
    OP_cmovnp,
    OP_cmovl,
    OP_cmovnl,
    OP_cmovle,
    OP_cmovnle,
    OP_punpcklbw,
    OP_punpcklwd,
    OP_punpckldq,
    OP_packsswb,
    OP_pcmpgtb,
    OP_pcmpgtw,
    OP_pcmpgtd,
    OP_packuswb,
    OP_punpckhbw,
    OP_punpckhwd,
    OP_punpckhdq,
    OP_packssdw,
    OP_punpcklqdq,
    OP_punpckhqdq,
    OP_movd,
    OP_movq,
    OP_movdqu,
    OP_movdqa,
    OP_pshufw,
    OP_pshufd,
    OP_pshufhw,
    OP_pshuflw,
    OP_pcmpeqb,
    OP_pcmpeqw,
    OP_pcmpeqd,
    OP_emms,
    OP_jo,
    OP_jno,
    OP_jb,
    OP_jnb,
    OP_jz,
    OP_jnz,
    OP_jbe,
    OP_jnbe,
    OP_js,
    OP_jns,
    OP_jp,
    OP_jnp,
    OP_jl,
    OP_jnl,
    OP_jle,
    OP_jnle,
    OP_seto,
    OP_setno,
    OP_setb,
    OP_setnb,
    OP_setz,
    OP_setnz,
    OP_setbe,
    OP_setnbe,
    OP_sets,
    OP_setns,
    OP_setp,
    OP_setnp,
    OP_setl,
    OP_setnl,
    OP_setle,
    OP_setnle,
    OP_cpuid,
    OP_bt,
    OP_shld,
    OP_rsm,
    OP_bts,
    OP_shrd,
    OP_cmpxchg,
    OP_lss,
    OP_btr,
    OP_lfs,
    OP_lgs,
    OP_movzx,
    OP_ud2b,
    OP_btc,
    OP_bsf,
    OP_bsr,
    OP_movsx,
    OP_xadd,
    OP_movnti,
    OP_pinsrw,
    OP_pextrw,
    OP_bswap,
    OP_psrlw,
    OP_psrld,
    OP_psrlq,
    OP_paddq,
    OP_pmullw,
    OP_pmovmskb,
    OP_psubusb,
    OP_psubusw,
    OP_pminub,
    OP_pand,
    OP_paddusb,
    OP_paddusw,
    OP_pmaxub,
    OP_pandn,
    OP_pavgb,
    OP_psraw,
    OP_psrad,
    OP_pavgw,
    OP_pmulhuw,
    OP_pmulhw,
    OP_movntq,
    OP_movntdq,
    OP_psubsb,
    OP_psubsw,
    OP_pminsw,
    OP_por,
    OP_paddsb,
    OP_paddsw,
    OP_pmaxsw,
    OP_pxor,
    OP_psllw,
    OP_pslld,
    OP_psllq,
    OP_pmuludq,
    OP_pmaddwd,
    OP_psadbw,
    OP_maskmovq,
    OP_maskmovdqu,
    OP_psubb,
    OP_psubw,
    OP_psubd,
    OP_psubq,
    OP_paddb,
    OP_paddw,
    OP_paddd,
    OP_psrldq,
    OP_pslldq,
    OP_rol,
    OP_ror,
    OP_rcl,
    OP_rcr,
    OP_shl,
    OP_shr,
    OP_sar,
    OP_not,
    OP_neg,
    OP_mul,
    OP_div,
    OP_idiv,
    OP_sldt,
    OP_str,
    OP_lldt,
    OP_ltr,
    OP_verr,
    OP_verw,
    OP_sgdt,
    OP_sidt,
    OP_lgdt,
    OP_lidt,
    OP_smsw,
    OP_lmsw,
    OP_invlpg,
    OP_cmpxchg8b,
    OP_fxsave,
    OP_fxrstor,
    OP_ldmxcsr,
    OP_stmxcsr,
    OP_lfence,
    OP_mfence,
    OP_clflush,
    OP_sfence,
    OP_prefetchnta,
    OP_prefetcht0,
    OP_prefetcht1,
    OP_prefetcht2,
    OP_prefetch,
    OP_prefetchw,
    OP_movups,
    OP_movss,
    OP_movupd,
    OP_movsd,
    OP_movlps,
    OP_movlpd,
    OP_unpcklps,
    OP_unpcklpd,
    OP_unpckhps,
    OP_unpckhpd,
    OP_movhps,
    OP_movhpd,
    OP_movaps,
    OP_movapd,
    OP_cvtpi2ps,
    OP_cvtsi2ss,
    OP_cvtpi2pd,
    OP_cvtsi2sd,
    OP_cvttps2pi,
    OP_cvttss2si,
    OP_cvttpd2pi,
    OP_cvttsd2si,
    OP_cvtps2pi,
    OP_cvtss2si,
    OP_cvtpd2pi,
    OP_cvtsd2si,
    OP_ucomiss,
    OP_ucomisd,
    OP_comiss,
    OP_comisd,
    OP_movmskps,
    OP_movmskpd,
    OP_sqrtps,
    OP_sqrtss,
    OP_sqrtpd,
    OP_sqrtsd,
    OP_rsqrtps,
    OP_rsqrtss,
    OP_rcpps,
    OP_rcpss,
    OP_andps,
    OP_andpd,
    OP_andnps,
    OP_andnpd,
    OP_orps,
    OP_orpd,
    OP_xorps,
    OP_xorpd,
    OP_addps,
    OP_addss,
    OP_addpd,
    OP_addsd,
    OP_mulps,
    OP_mulss,
    OP_mulpd,
    OP_mulsd,
    OP_cvtps2pd,
    OP_cvtss2sd,
    OP_cvtpd2ps,
    OP_cvtsd2ss,
    OP_cvtdq2ps,
    OP_cvttps2dq,
    OP_cvtps2dq,
    OP_subps,
    OP_subss,
    OP_subpd,
    OP_subsd,
    OP_minps,
    OP_minss,
    OP_minpd,
    OP_minsd,
    OP_divps,
    OP_divss,
    OP_divpd,
    OP_divsd,
    OP_maxps,
    OP_maxss,
    OP_maxpd,
    OP_maxsd,
    OP_cmpps,
    OP_cmpss,
    OP_cmppd,
    OP_cmpsd,
    OP_shufps,
    OP_shufpd,
    OP_cvtdq2pd,
    OP_cvttpd2dq,
    OP_cvtpd2dq,
    OP_nop,
    OP_pause,
    OP_ins,
    OP_rep_ins,
    OP_outs,
    OP_rep_outs,
    OP_movs,
    OP_rep_movs,
    OP_stos,
    OP_rep_stos,
    OP_lods,
    OP_rep_lods,
    OP_cmps,
    OP_rep_cmps,
    OP_repne_cmps,
    OP_scas,
    OP_rep_scas,
    OP_repne_scas,
    OP_fadd,
    OP_fmul,
    OP_fcom,
    OP_fcomp,
    OP_fsub,
    OP_fsubr,
    OP_fdiv,
    OP_fdivr,
    OP_fld,
    OP_fst,
    OP_fstp,
    OP_fldenv,
    OP_fldcw,
    OP_fnstenv,
    OP_fnstcw,
    OP_fiadd,
    OP_fimul,
    OP_ficom,
    OP_ficomp,
    OP_fisub,
    OP_fisubr,
    OP_fidiv,
    OP_fidivr,
    OP_fild,
    OP_fist,
    OP_fistp,
    OP_frstor,
    OP_fnsave,
    OP_fnstsw,
    OP_fbld,
    OP_fbstp,
    OP_fxch,
    OP_fnop,
    OP_fchs,
    OP_fabs,
    OP_ftst,
    OP_fxam,
    OP_fld1,
    OP_fldl2t,
    OP_fldl2e,
    OP_fldpi,
    OP_fldlg2,
    OP_fldln2,
    OP_fldz,
    OP_f2xm1,
    OP_fyl2x,
    OP_fptan,
    OP_fpatan,
    OP_fxtract,
    OP_fprem1,
    OP_fdecstp,
    OP_fincstp,
    OP_fprem,
    OP_fyl2xp1,
    OP_fsqrt,
    OP_fsincos,
    OP_frndint,
    OP_fscale,
    OP_fsin,
    OP_fcos,
    OP_fcmovb,
    OP_fcmove,
    OP_fcmovbe,
    OP_fcmovu,
    OP_fucompp,
    OP_fcmovnb,
    OP_fcmovne,
    OP_fcmovnbe,
    OP_fcmovnu,
    OP_fnclex,
    OP_fninit,
    OP_fucomi,
    OP_fcomi,
    OP_ffree,
    OP_fucom,
    OP_fucomp,
    OP_faddp,
    OP_fmulp,
    OP_fcompp,
    OP_fsubrp,
    OP_fsubp,
    OP_fdivrp,
    OP_fdivp,
    OP_fucomip,
    OP_fcomip,
    OP_fisttp,
    OP_haddpd,
    OP_haddps,
    OP_hsubpd,
    OP_hsubps,
    OP_addsubpd,
    OP_addsubps,
    OP_lddqu,
    OP_monitor,
    OP_mwait,
    OP_movsldup,
    OP_movshdup,
    OP_movddup,
    OP_femms,
    OP_unknown_3dnow,
    OP_pavgusb,
    OP_pfadd,
    OP_pfacc,
    OP_pfcmpge,
    OP_pfcmpgt,
    OP_pfcmpeq,
    OP_pfmin,
    OP_pfmax,
    OP_pfmul,
    OP_pfrcp,
    OP_pfrcpit1,
    OP_pfrcpit2,
    OP_pfrsqrt,
    OP_pfrsqit1,
    OP_pmulhrw,
    OP_pfsub,
    OP_pfsubr,
    OP_pi2fd,
    OP_pf2id,
    OP_pi2fw,
    OP_pf2iw,
    OP_pfnacc,
    OP_pfpnacc,
    OP_pswapd,
    OP_pshufb,
    OP_phaddw,
    OP_phaddd,
    OP_phaddsw,
    OP_pmaddubsw,
    OP_phsubw,
    OP_phsubd,
    OP_phsubsw,
    OP_psignb,
    OP_psignw,
    OP_psignd,
    OP_pmulhrsw,
    OP_pabsb,
    OP_pabsw,
    OP_pabsd,
    OP_palignr,
    OP_popcnt,
    OP_movntss,
    OP_movntsd,
    OP_extrq,
    OP_insertq,
    OP_lzcnt,
    OP_pblendvb,
    OP_blendvps,
    OP_blendvpd,
    OP_ptest,
    OP_pmovsxbw,
    OP_pmovsxbd,
    OP_pmovsxbq,
    OP_pmovsxdw,
    OP_pmovsxwq,
    OP_pmovsxdq,
    OP_pmuldq,
    OP_pcmpeqq,
    OP_movntdqa,
    OP_packusdw,
    OP_pmovzxbw,
    OP_pmovzxbd,
    OP_pmovzxbq,
    OP_pmovzxdw,
    OP_pmovzxwq,
    OP_pmovzxdq,
    OP_pcmpgtq,
    OP_pminsb,
    OP_pminsd,
    OP_pminuw,
    OP_pminud,
    OP_pmaxsb,
    OP_pmaxsd,
    OP_pmaxuw,
    OP_pmaxud,
    OP_pmulld,
    OP_phminposuw,
    OP_crc32,
    OP_pextrb,
    OP_pextrd,
    OP_extractps,
    OP_roundps,
    OP_roundpd,
    OP_roundss,
    OP_roundsd,
    OP_blendps,
    OP_blendpd,
    OP_pblendw,
    OP_pinsrb,
    OP_insertps,
    OP_pinsrd,
    OP_dpps,
    OP_dppd,
    OP_mpsadbw,
    OP_pcmpestrm,
    OP_pcmpestri,
    OP_pcmpistrm,
    OP_pcmpistri,
    OP_movsxd,
    OP_swapgs,
    OP_vmcall,
    OP_vmlaunch,
    OP_vmresume,
    OP_vmxoff,
    OP_vmptrst,
    OP_vmptrld,
    OP_vmxon,
    OP_vmclear,
    OP_vmread,
    OP_vmwrite,
    OP_int1,
    OP_salc,
    OP_ffreep,
    OP_vmrun,
    OP_vmmcall,
    OP_vmload,
    OP_vmsave,
    OP_stgi,
    OP_clgi,
    OP_skinit,
    OP_invlpga,
    OP_rdtscp,
    OP_invept,
    OP_invvpid,
    OP_pclmulqdq,
    OP_aesimc,
    OP_aesenc,
    OP_aesenclast,
    OP_aesdec,
    OP_aesdeclast,
    OP_aeskeygenassist,
    OP_movbe,
    OP_xgetbv,
    OP_xsetbv,
    OP_xsave,
    OP_xrstor,
    OP_xsaveopt,
    OP_vmovss,
    OP_vmovsd,
    OP_vmovups,
    OP_vmovupd,
    OP_vmovlps,
    OP_vmovsldup,
    OP_vmovlpd,
    OP_vmovddup,
    OP_vunpcklps,
    OP_vunpcklpd,
    OP_vunpckhps,
    OP_vunpckhpd,
    OP_vmovhps,
    OP_vmovshdup,
    OP_vmovhpd,
    OP_vmovaps,
    OP_vmovapd,
    OP_vcvtsi2ss,
    OP_vcvtsi2sd,
    OP_vmovntps,
    OP_vmovntpd,
    OP_vcvttss2si,
    OP_vcvttsd2si,
    OP_vcvtss2si,
    OP_vcvtsd2si,
    OP_vucomiss,
    OP_vucomisd,
    OP_vcomiss,
    OP_vcomisd,
    OP_vmovmskps,
    OP_vmovmskpd,
    OP_vsqrtps,
    OP_vsqrtss,
    OP_vsqrtpd,
    OP_vsqrtsd,
    OP_vrsqrtps,
    OP_vrsqrtss,
    OP_vrcpps,
    OP_vrcpss,
    OP_vandps,
    OP_vandpd,
    OP_vandnps,
    OP_vandnpd,
    OP_vorps,
    OP_vorpd,
    OP_vxorps,
    OP_vxorpd,
    OP_vaddps,
    OP_vaddss,
    OP_vaddpd,
    OP_vaddsd,
    OP_vmulps,
    OP_vmulss,
    OP_vmulpd,
    OP_vmulsd,
    OP_vcvtps2pd,
    OP_vcvtss2sd,
    OP_vcvtpd2ps,
    OP_vcvtsd2ss,
    OP_vcvtdq2ps,
    OP_vcvttps2dq,
    OP_vcvtps2dq,
    OP_vsubps,
    OP_vsubss,
    OP_vsubpd,
    OP_vsubsd,
    OP_vminps,
    OP_vminss,
    OP_vminpd,
    OP_vminsd,
    OP_vdivps,
    OP_vdivss,
    OP_vdivpd,
    OP_vdivsd,
    OP_vmaxps,
    OP_vmaxss,
    OP_vmaxpd,
    OP_vmaxsd,
    OP_vpunpcklbw,
    OP_vpunpcklwd,
    OP_vpunpckldq,
    OP_vpacksswb,
    OP_vpcmpgtb,
    OP_vpcmpgtw,
    OP_vpcmpgtd,
    OP_vpackuswb,
    OP_vpunpckhbw,
    OP_vpunpckhwd,
    OP_vpunpckhdq,
    OP_vpackssdw,
    OP_vpunpcklqdq,
    OP_vpunpckhqdq,
    OP_vmovd,
    OP_vpshufhw,
    OP_vpshufd,
    OP_vpshuflw,
    OP_vpcmpeqb,
    OP_vpcmpeqw,
    OP_vpcmpeqd,
    OP_vmovq,
    OP_vcmpps,
    OP_vcmpss,
    OP_vcmppd,
    OP_vcmpsd,
    OP_vpinsrw,
    OP_vpextrw,
    OP_vshufps,
    OP_vshufpd,
    OP_vpsrlw,
    OP_vpsrld,
    OP_vpsrlq,
    OP_vpaddq,
    OP_vpmullw,
    OP_vpmovmskb,
    OP_vpsubusb,
    OP_vpsubusw,
    OP_vpminub,
    OP_vpand,
    OP_vpaddusb,
    OP_vpaddusw,
    OP_vpmaxub,
    OP_vpandn,
    OP_vpavgb,
    OP_vpsraw,
    OP_vpsrad,
    OP_vpavgw,
    OP_vpmulhuw,
    OP_vpmulhw,
    OP_vcvtdq2pd,
    OP_vcvttpd2dq,
    OP_vcvtpd2dq,
    OP_vmovntdq,
    OP_vpsubsb,
    OP_vpsubsw,
    OP_vpminsw,
    OP_vpor,
    OP_vpaddsb,
    OP_vpaddsw,
    OP_vpmaxsw,
    OP_vpxor,
    OP_vpsllw,
    OP_vpslld,
    OP_vpsllq,
    OP_vpmuludq,
    OP_vpmaddwd,
    OP_vpsadbw,
    OP_vmaskmovdqu,
    OP_vpsubb,
    OP_vpsubw,
    OP_vpsubd,
    OP_vpsubq,
    OP_vpaddb,
    OP_vpaddw,
    OP_vpaddd,
    OP_vpsrldq,
    OP_vpslldq,
    OP_vmovdqu,
    OP_vmovdqa,
    OP_vhaddpd,
    OP_vhaddps,
    OP_vhsubpd,
    OP_vhsubps,
    OP_vaddsubpd,
    OP_vaddsubps,
    OP_vlddqu,
    OP_vpshufb,
    OP_vphaddw,
    OP_vphaddd,
    OP_vphaddsw,
    OP_vpmaddubsw,
    OP_vphsubw,
    OP_vphsubd,
    OP_vphsubsw,
    OP_vpsignb,
    OP_vpsignw,
    OP_vpsignd,
    OP_vpmulhrsw,
    OP_vpabsb,
    OP_vpabsw,
    OP_vpabsd,
    OP_vpalignr,
    OP_vpblendvb,
    OP_vblendvps,
    OP_vblendvpd,
    OP_vptest,
    OP_vpmovsxbw,
    OP_vpmovsxbd,
    OP_vpmovsxbq,
    OP_vpmovsxdw,
    OP_vpmovsxwq,
    OP_vpmovsxdq,
    OP_vpmuldq,
    OP_vpcmpeqq,
    OP_vmovntdqa,
    OP_vpackusdw,
    OP_vpmovzxbw,
    OP_vpmovzxbd,
    OP_vpmovzxbq,
    OP_vpmovzxdw,
    OP_vpmovzxwq,
    OP_vpmovzxdq,
    OP_vpcmpgtq,
    OP_vpminsb,
    OP_vpminsd,
    OP_vpminuw,
    OP_vpminud,
    OP_vpmaxsb,
    OP_vpmaxsd,
    OP_vpmaxuw,
    OP_vpmaxud,
    OP_vpmulld,
    OP_vphminposuw,
    OP_vaesimc,
    OP_vaesenc,
    OP_vaesenclast,
    OP_vaesdec,
    OP_vaesdeclast,
    OP_vpextrb,
    OP_vpextrd,
    OP_vextractps,
    OP_vroundps,
    OP_vroundpd,
    OP_vroundss,
    OP_vroundsd,
    OP_vblendps,
    OP_vblendpd,
    OP_vpblendw,
    OP_vpinsrb,
    OP_vinsertps,
    OP_vpinsrd,
    OP_vdpps,
    OP_vdppd,
    OP_vmpsadbw,
    OP_vpcmpestrm,
    OP_vpcmpestri,
    OP_vpcmpistrm,
    OP_vpcmpistri,
    OP_vpclmulqdq,
    OP_vaeskeygenassist,
    OP_vtestps,
    OP_vtestpd,
    OP_vzeroupper,
    OP_vzeroall,
    OP_vldmxcsr,
    OP_vstmxcsr,
    OP_vbroadcastss,
    OP_vbroadcastsd,
    OP_vbroadcastf128,
    OP_vmaskmovps,
    OP_vmaskmovpd,
    OP_vpermilps,
    OP_vpermilpd,
    OP_vperm2f128,
    OP_vinsertf128,
    OP_vextractf128,
    OP_vcvtph2ps,
    OP_vcvtps2ph,
    OP_vfmadd132ps,
    OP_vfmadd132pd,
    OP_vfmadd213ps,
    OP_vfmadd213pd,
    OP_vfmadd231ps,
    OP_vfmadd231pd,
    OP_vfmadd132ss,
    OP_vfmadd132sd,
    OP_vfmadd213ss,
    OP_vfmadd213sd,
    OP_vfmadd231ss,
    OP_vfmadd231sd,
    OP_vfmaddsub132ps,
    OP_vfmaddsub132pd,
    OP_vfmaddsub213ps,
    OP_vfmaddsub213pd,
    OP_vfmaddsub231ps,
    OP_vfmaddsub231pd,
    OP_vfmsubadd132ps,
    OP_vfmsubadd132pd,
    OP_vfmsubadd213ps,
    OP_vfmsubadd213pd,
    OP_vfmsubadd231ps,
    OP_vfmsubadd231pd,
    OP_vfmsub132ps,
    OP_vfmsub132pd,
    OP_vfmsub213ps,
    OP_vfmsub213pd,
    OP_vfmsub231ps,
    OP_vfmsub231pd,
    OP_vfmsub132ss,
    OP_vfmsub132sd,
    OP_vfmsub213ss,
    OP_vfmsub213sd,
    OP_vfmsub231ss,
    OP_vfmsub231sd,
    OP_vfnmadd132ps,
    OP_vfnmadd132pd,
    OP_vfnmadd213ps,
    OP_vfnmadd213pd,
    OP_vfnmadd231ps,
    OP_vfnmadd231pd,
    OP_vfnmadd132ss,
    OP_vfnmadd132sd,
    OP_vfnmadd213ss,
    OP_vfnmadd213sd,
    OP_vfnmadd231ss,
    OP_vfnmadd231sd,
    OP_vfnmsub132ps,
    OP_vfnmsub132pd,
    OP_vfnmsub213ps,
    OP_vfnmsub213pd,
    OP_vfnmsub231ps,
    OP_vfnmsub231pd,
    OP_vfnmsub132ss,
    OP_vfnmsub132sd,
    OP_vfnmsub213ss,
    OP_vfnmsub213sd,
    OP_vfnmsub231ss,
    OP_vfnmsub231sd,
    OP_movq2dq,
    OP_movdq2q,
    OP_AFTER_LAST,
    OP_FIRST = OP_add,
    OP_LAST = OP_AFTER_LAST - 1
};

instr_t *convert_to_near_rel_common(dcontext_t *dcontext,
                                    instrlist_t *ilist,
                                    instr_t *instr);


dcontext_t * get_thread_private_dcontext(void);

enum {
    OPCODE_TWOBYTES = 0x00000010,
    OPCODE_REG = 0x00000020,
    OPCODE_MODRM = 0x00000040,
    OPCODE_SUFFIX = 0x00000080,
    OPCODE_THREEBYTES = 0x00000008
};

typedef struct instr_info_t {
    int type;
    uint opcode;
    const char * name;
    byte dst1_type;
    opnd_size_t dst1_size;
    byte dst2_type;
    opnd_size_t dst2_size;
    byte src1_type;
    opnd_size_t src1_size;
    byte src2_type;
    opnd_size_t src2_size;
    byte src3_type;
    opnd_size_t src3_size;
    byte flags;
    uint eflags;
    ptr_int_t code;
} instr_info_t;

enum {
    INVALID = OP_LAST + 1,
    PREFIX,
    ESCAPE,
    FLOAT_EXT,
    EXTENSION,
    PREFIX_EXT,
    REP_EXT,
    REPNE_EXT,
    MOD_EXT,
    RM_EXT,
    SUFFIX_EXT,
    X64_EXT,
    ESCAPE_3BYTE_38,
    ESCAPE_3BYTE_3a,
    REX_EXT,
    VEX_PREFIX_EXT,
    VEX_EXT,
    VEX_L_EXT,
    VEX_W_EXT
};

typedef struct decode_info_t {
    uint prefixes;
    byte seg_override;
    byte modrm;
    byte mod;
    byte reg;
    byte rm;
    bool has_sib;
    byte scale;
    byte index;
    byte base;
    bool has_disp;
    int disp;
    opnd_size_t size_immed;
    opnd_size_t size_immed2;
    ptr_int_t immed;
    ptr_int_t immed2;
    byte * start_pc;
    byte * final_pc;
    uint len;
    byte * disp_abs;
    bool x86_mode;
    byte * orig_pc;
    bool data_prefix;
    bool rep_prefix;
    bool repne_prefix;
    byte vex_vvvv;
    bool vex_encoded;
    ptr_int_t cur_note;
    bool has_instr_opnds;
} decode_info_t;

enum {
    TYPE_NONE,
    TYPE_A,
    TYPE_C,
    TYPE_D,
    TYPE_E,
    TYPE_G,
    TYPE_H,
    TYPE_I,
    TYPE_J,
    TYPE_L,
    TYPE_M,
    TYPE_O,
    TYPE_P,
    TYPE_Q,
    TYPE_R,
    TYPE_S,
    TYPE_V,
    TYPE_W,
    TYPE_X,
    TYPE_Y,
    TYPE_P_MODRM,
    TYPE_V_MODRM,
    TYPE_1,
    TYPE_FLOATCONST,
    TYPE_XLAT,
    TYPE_MASKMOVQ,
    TYPE_FLOATMEM,
    TYPE_REG,
    TYPE_VAR_REG,
    TYPE_VARZ_REG,
    TYPE_VAR_XREG,
    TYPE_VAR_ADDR_XREG,
    TYPE_REG_EX,
    TYPE_VAR_REG_EX,
    TYPE_VAR_XREG_EX,
    TYPE_VAR_REGX_EX,
    TYPE_INDIR_E,
    TYPE_INDIR_REG,
    TYPE_INDIR_VAR_XREG,
    TYPE_INDIR_VAR_REG,
    TYPE_INDIR_VAR_XIREG,
    TYPE_INDIR_VAR_XREG_OFFS_1,
    TYPE_INDIR_VAR_XREG_OFFS_8,
    TYPE_INDIR_VAR_XREG_OFFS_N,
    TYPE_INDIR_VAR_XIREG_OFFS_1,
    TYPE_INDIR_VAR_REG_OFFS_2,
    TYPE_INDIR_VAR_XREG_SIZEx8,
    TYPE_INDIR_VAR_REG_SIZEx2,
    TYPE_INDIR_VAR_REG_SIZEx3x5
};

enum op_size_type {
    OPSZ_NA = DR_REG_INVALID + 1,
    OPSZ_FIRST = OPSZ_NA,
    OPSZ_0,
    OPSZ_1,
    OPSZ_2,
    OPSZ_4,
    OPSZ_6,
    OPSZ_8,
    OPSZ_10,
    OPSZ_16,
    OPSZ_14,
    OPSZ_28,
    OPSZ_94,
    OPSZ_108,
    OPSZ_512,
    OPSZ_2_short1,
    OPSZ_4_short2,
    OPSZ_4_rex8_short2,
    OPSZ_4_rex8,
    OPSZ_6_irex10_short4,
    OPSZ_8_short2,
    OPSZ_8_short4,
    OPSZ_28_short14,
    OPSZ_108_short94,
    OPSZ_4x8,
    OPSZ_6x10,
    OPSZ_4x8_short2,
    OPSZ_4x8_short2xi8,
    OPSZ_4_short2xi4,
    OPSZ_1_reg4,
    OPSZ_2_reg4,
    OPSZ_4_reg16,
    OPSZ_xsave,
    OPSZ_12,
    OPSZ_32,
    OPSZ_40,
    OPSZ_32_short16,
    OPSZ_8_rex16,
    OPSZ_8_rex16_short4,
    OPSZ_12_rex40_short6,
    OPSZ_16_vex32,
    OPSZ_LAST
};

enum {
    OPSZ_4_of_8 = OPSZ_LAST,
    OPSZ_4_of_16,
    OPSZ_8_of_16,
    OPSZ_8_of_16_vex32,
    OPSZ_16_of_32,
    OPSZ_LAST_ENUM
};

typedef enum {
    IBL_UNLINKED,
    IBL_DELETE,
    IBL_FAR,
    IBL_FAR_UNLINKED,
    IBL_TRACE_CMP,
    IBL_TRACE_CMP_UNLINKED,
    IBL_LINKED,
    IBL_TEMPLATE,
    IBL_LINK_STATE_END
} ibl_entry_point_type_t;

typedef enum {
    IBL_BB_SHARED,
    IBL_SOURCE_TYPE_START = IBL_BB_SHARED,
    IBL_TRACE_SHARED,
    IBL_BB_PRIVATE,
    IBL_TRACE_PRIVATE,
    IBL_COARSE_SHARED,
    IBL_SOURCE_TYPE_END
} ibl_source_fragment_type_t;

typedef struct {
    ibl_entry_point_type_t link_state;
    ibl_source_fragment_type_t source_fragment_type;
    ibl_branch_type_t branch_type;
} ibl_type_t;

typedef enum {
    GENCODE_X64 = 0,
    GENCODE_X86,
    GENCODE_X86_TO_X64,
    GENCODE_FROM_DCONTEXT
} gencode_mode_t;

#ifndef GRANARY
typedef struct _clean_call_info_t {
    void * callee;
    uint num_args;
    bool save_fpstate;
    bool opt_inline;
    bool should_align;
    bool save_all_regs;
    bool skip_save_aflags;
    bool skip_clear_eflags;
    uint num_xmms_skip;
    bool xmm_skip[0];
    uint num_regs_skip;
    bool reg_skip[(1 + (DR_REG_R15 - DR_REG_RAX) ) ];
    bool preserve_mcontext;
    void * callee_info;
    instrlist_t * ilist;
} clean_call_info_t;
#endif

enum {
    FCACHE_ENTER_TARGET_SLOT = 0 ,
    MANGLE_NEXT_TAG_SLOT = 0 ,
    DIRECT_STUB_SPILL_SLOT = 0 ,
    MANGLE_RIPREL_SPILL_SLOT = 0 ,
    INDIRECT_STUB_SPILL_SLOT = 0 ,
    MANGLE_FAR_SPILL_SLOT = 0 ,
    MANGLE_XCX_SPILL_SLOT = 0 ,
    DCONTEXT_BASE_SPILL_SLOT = 0 ,
    PREFIX_XAX_SPILL_SLOT = 0
};

typedef struct patch_entry_t {
    union {
        instr_t * instr;
        size_t offset;
    } where;
    ptr_uint_t value_location_offset;
    ushort patch_flags;
    short instr_offset;
} patch_entry_t;

enum {
    MAX_PATCH_ENTRIES = 7,
    PATCH_TAKE_ADDRESS = 0x01,
    PATCH_PER_THREAD = 0x02,
    PATCH_UNPROT_STAT = 0x04,
    PATCH_MARKER = 0x08,
    PATCH_ASSEMBLE_ABSOLUTE = 0x10,
    PATCH_OFFSET_VALID = 0x20,
    PATCH_UINT_SIZED = 0x40
};

typedef enum {
    PATCH_TYPE_ABSOLUTE = 0x0,
    PATCH_TYPE_INDIRECT_XDI = 0x1,
    PATCH_TYPE_INDIRECT_FS = 0x2
} patch_list_type_t;

typedef struct patch_list_t {
    ushort num_relocations;
    ushort type;
    patch_entry_t entry[MAX_PATCH_ENTRIES];
} patch_list_t;

typedef struct _far_ref_t {
    uint pc;
    ushort selector;
} far_ref_t;

typedef struct ibl_code_t {
    bool initialised :1;
    bool thread_shared_routine :1;
    bool ibl_head_is_inlined :1;
    byte * indirect_branch_lookup_routine;
    byte * far_ibl;
    byte * far_ibl_unlinked;
    byte * trace_cmp_entry;
    byte * trace_cmp_unlinked;
    bool x86_mode;
    bool x86_to_x64_mode;
    far_ref_t far_jmp_opnd;
    far_ref_t far_jmp_unlinked_opnd;
    byte * unlinked_ibl_entry;
    byte * target_delete_entry;
    uint ibl_routine_length;
    patch_list_t ibl_patch;
    ibl_branch_type_t branch_type;
    ibl_source_fragment_type_t source_fragment_type;
    byte * inline_ibl_stub_template;
    patch_list_t ibl_stub_patch;
    uint inline_stub_length;
    uint inline_linkstub_first_offs;
    uint inline_linkstub_second_offs;
    uint inline_unlink_offs;
    uint inline_linkedjmp_offs;
    uint inline_unlinkedjmp_offs;
} ibl_code_t;

typedef struct _generated_code_t {
    byte * fcache_enter;
    byte * fcache_return;
    ibl_code_t trace_ibl[IBL_BRANCH_TYPE_END];
    ibl_code_t bb_ibl[IBL_BRANCH_TYPE_END];
    ibl_code_t coarse_ibl[IBL_BRANCH_TYPE_END];
    byte * do_syscall;
    uint do_syscall_offs;
    byte * do_int_syscall;
    uint do_int_syscall_offs;
    byte * do_clone_syscall;
    uint do_clone_syscall_offs;
    byte * new_thread_dynamo_start;
    byte * reset_exit_stub;
    byte * fcache_return_coarse;
    byte * trace_head_return_coarse;
    byte * client_ibl_xfer;
    uint client_ibl_unlink_offs;
    bool thread_shared;
    bool writable;
    gencode_mode_t gencode_mode;
    byte * gen_start_pc;
    byte * gen_end_pc;
    byte * commit_end_pc;
} generated_code_t;

typedef float float_t;

typedef double double_t;

bool opnd_is_null ( opnd_t op );

bool opnd_is_immed_int ( opnd_t op );

bool opnd_is_immed_float ( opnd_t op );

bool opnd_is_near_pc ( opnd_t op );

bool opnd_is_near_instr ( opnd_t op );

bool opnd_is_reg ( opnd_t op );

bool opnd_is_base_disp ( opnd_t op );

bool opnd_is_far_pc ( opnd_t op );

bool opnd_is_far_instr ( opnd_t op );

bool opnd_is_mem_instr ( opnd_t op );

bool opnd_is_valid ( opnd_t op );

bool opnd_is_rel_addr ( opnd_t op );

bool opnd_is_abs_addr ( opnd_t opnd );

bool opnd_is_near_abs_addr ( opnd_t opnd );

bool opnd_is_far_abs_addr ( opnd_t opnd );

bool opnd_is_reg_32bit ( opnd_t opnd );

bool reg_is_32bit ( reg_id_t reg );

bool opnd_is_reg_64bit ( opnd_t opnd );

bool reg_is_64bit ( reg_id_t reg );

bool opnd_is_reg_pointer_sized ( opnd_t opnd );

bool reg_is_pointer_sized ( reg_id_t reg );

reg_id_t opnd_get_reg ( opnd_t opnd );

opnd_size_t opnd_get_size ( opnd_t opnd );

void opnd_set_size ( opnd_t * opnd , opnd_size_t newsize );

opnd_t opnd_create_immed_int ( ptr_int_t i , opnd_size_t size );

opnd_t opnd_create_immed_float ( float i );

opnd_t opnd_create_immed_float_zero ( void );

ptr_int_t opnd_get_immed_int ( opnd_t opnd );

float opnd_get_immed_float ( opnd_t opnd );

opnd_t opnd_create_far_pc ( ushort seg_selector , app_pc pc );

opnd_t opnd_create_instr ( instr_t * instr );

opnd_t opnd_create_far_instr ( ushort seg_selector , instr_t * instr );

opnd_t opnd_create_mem_instr ( instr_t * instr , short disp , opnd_size_t data_size );

app_pc opnd_get_pc ( opnd_t opnd );

ushort opnd_get_segment_selector ( opnd_t opnd );

instr_t * opnd_get_instr ( opnd_t opnd );

short opnd_get_mem_instr_disp ( opnd_t opnd );

opnd_t opnd_create_base_disp_ex ( reg_id_t base_reg , reg_id_t index_reg , int scale , int disp , opnd_size_t size , bool encode_zero_disp , bool force_full_disp , bool disp_short_addr );

opnd_t opnd_create_base_disp ( reg_id_t base_reg , reg_id_t index_reg , int scale , int disp , opnd_size_t size );

opnd_t opnd_create_far_base_disp_ex ( reg_id_t seg , reg_id_t base_reg , reg_id_t index_reg , int scale , int disp , opnd_size_t size , bool encode_zero_disp , bool force_full_disp , bool disp_short_addr );

opnd_t opnd_create_far_base_disp ( reg_id_t seg , reg_id_t base_reg , reg_id_t index_reg , int scale , int disp , opnd_size_t size );

reg_id_t opnd_get_base ( opnd_t opnd );

int opnd_get_disp ( opnd_t opnd );

reg_id_t opnd_get_index ( opnd_t opnd );

int opnd_get_scale ( opnd_t opnd );

reg_id_t opnd_get_segment ( opnd_t opnd );

bool opnd_is_disp_encode_zero ( opnd_t opnd );

bool opnd_is_disp_force_full ( opnd_t opnd );

bool opnd_is_disp_short_addr ( opnd_t opnd );

void opnd_set_disp ( opnd_t * opnd , int disp );

void opnd_set_disp_ex ( opnd_t * opnd , int disp , bool encode_zero_disp , bool force_full_disp , bool disp_short_addr );

opnd_t opnd_create_abs_addr ( void * addr , opnd_size_t data_size );

opnd_t opnd_create_far_abs_addr ( reg_id_t seg , void * addr , opnd_size_t data_size );

opnd_t opnd_create_rel_addr ( void * addr , opnd_size_t data_size );

opnd_t opnd_create_far_rel_addr ( reg_id_t seg , void * addr , opnd_size_t data_size );

void * opnd_get_addr ( opnd_t opnd );

bool opnd_is_memory_reference ( opnd_t opnd );

bool opnd_is_far_memory_reference ( opnd_t opnd );

bool opnd_is_near_memory_reference ( opnd_t opnd );

int opnd_num_regs_used ( opnd_t opnd );

reg_id_t opnd_get_reg_used ( opnd_t opnd , int index );

bool opnd_uses_reg ( opnd_t opnd , reg_id_t reg );

bool opnd_replace_reg ( opnd_t * opnd , reg_id_t old_reg , reg_id_t new_reg );

bool opnd_same_address ( opnd_t op1 , opnd_t op2 );

bool opnd_same ( opnd_t op1 , opnd_t op2 );

bool opnd_share_reg ( opnd_t op1 , opnd_t op2 );

bool opnd_defines_use ( opnd_t def , opnd_t use );

uint opnd_size_in_bytes ( opnd_size_t size );

opnd_t opnd_shrink_to_16_bits ( opnd_t opnd );

opnd_t opnd_shrink_to_32_bits ( opnd_t opnd );

reg_t reg_get_value_priv ( reg_id_t reg , priv_mcontext_t * mc );

reg_t reg_get_value ( reg_id_t reg , dr_mcontext_t * mc );

void reg_set_value_priv ( reg_id_t reg , priv_mcontext_t * mc , reg_t value );

void reg_set_value ( reg_id_t reg , dr_mcontext_t * mc , reg_t value );

app_pc opnd_compute_address_priv ( opnd_t opnd , priv_mcontext_t * mc );

app_pc opnd_compute_address ( opnd_t opnd , dr_mcontext_t * mc );

const char * get_register_name ( reg_id_t reg );

reg_id_t reg_to_pointer_sized ( reg_id_t reg );

reg_id_t reg_32_to_16 ( reg_id_t reg );

reg_id_t reg_32_to_8 ( reg_id_t reg );

reg_id_t reg_32_to_64 ( reg_id_t reg );

reg_id_t reg_64_to_32 ( reg_id_t reg );

bool reg_is_extended ( reg_id_t reg );

reg_id_t reg_32_to_opsz ( reg_id_t reg , opnd_size_t sz );

reg_id_t reg_resize_to_opsz ( reg_id_t reg , opnd_size_t sz );

int reg_parameter_num ( reg_id_t reg );

int opnd_get_reg_dcontext_offs ( reg_id_t reg );

int opnd_get_reg_mcontext_offs ( reg_id_t reg );

bool reg_overlap ( reg_id_t r1 , reg_id_t r2 );

enum {REG_INVALID_BITS = 0x0};

byte reg_get_bits ( reg_id_t reg );

opnd_size_t reg_get_size ( reg_id_t reg );

instr_t * instr_create ( dcontext_t * dcontext );

void instr_destroy ( dcontext_t * dcontext , instr_t * instr );

instr_t * instr_clone ( dcontext_t * dcontext , instr_t * orig );

void instr_init ( dcontext_t * dcontext , instr_t * instr );

void instr_free ( dcontext_t * dcontext , instr_t * instr );

int instr_mem_usage ( instr_t * instr );

void instr_reset ( dcontext_t * dcontext , instr_t * instr );

void instr_reuse ( dcontext_t * dcontext , instr_t * instr );

instr_t * instr_build ( dcontext_t * dcontext , int opcode , int instr_num_dsts , int instr_num_srcs );

instr_t * instr_build_bits ( dcontext_t * dcontext , int opcode , uint num_bytes );

int instr_get_opcode ( instr_t * instr );

void instr_set_opcode ( instr_t * instr , int opcode );

bool instr_valid ( instr_t * instr );

app_pc instr_get_app_pc ( instr_t * instr );

bool instr_opcode_valid ( instr_t * instr );

const instr_info_t * instr_get_instr_info ( instr_t * instr );

const instr_info_t * get_instr_info ( int opcode );

opnd_t instr_get_src ( instr_t * instr , uint pos );

opnd_t instr_get_dst ( instr_t * instr , uint pos );

void instr_set_num_opnds ( dcontext_t * dcontext , instr_t * instr , int instr_num_dsts , int instr_num_srcs );

void instr_set_src ( instr_t * instr , uint pos , opnd_t opnd );

void instr_set_dst ( instr_t * instr , uint pos , opnd_t opnd );

opnd_t instr_get_target ( instr_t * instr );

void instr_set_target ( instr_t * instr , opnd_t target );

instr_t * instr_set_prefix_flag ( instr_t * instr , uint prefix );

bool instr_get_prefix_flag ( instr_t * instr , uint prefix );

void instr_set_prefixes ( instr_t * instr , uint prefixes );

uint instr_get_prefixes ( instr_t * instr );

void instr_set_x86_mode ( instr_t * instr , bool x86 );

bool instr_get_x86_mode ( instr_t * instr );

bool instr_branch_selfmod_exit ( instr_t * instr );

void instr_branch_set_selfmod_exit ( instr_t * instr , bool val );

int instr_exit_branch_type ( instr_t * instr );

void instr_exit_branch_set_type ( instr_t * instr , uint type );

void instr_set_ok_to_mangle ( instr_t * instr , bool val );

bool instr_is_meta_may_fault ( instr_t * instr );

void instr_set_meta_may_fault ( instr_t * instr , bool val );

void instr_set_meta_no_translation ( instr_t * instr );

void instr_set_ok_to_emit ( instr_t * instr , bool val );

uint instr_get_eflags ( instr_t * instr );

uint instr_get_opcode_eflags ( int opcode );

uint instr_get_arith_flags ( instr_t * instr );

bool instr_eflags_valid ( instr_t * instr );

void instr_set_eflags_valid ( instr_t * instr , bool valid );

bool instr_arith_flags_valid ( instr_t * instr );

void instr_set_arith_flags_valid ( instr_t * instr , bool valid );

void instr_set_operands_valid ( instr_t * instr , bool valid );

void instr_set_raw_bits ( instr_t * instr , byte * addr , uint length );

void instr_shift_raw_bits ( instr_t * instr , ssize_t offs );

void instr_set_raw_bits_valid ( instr_t * instr , bool valid );

bool instr_operands_valid ( instr_t * instr );

bool instr_raw_bits_valid ( instr_t * instr );

bool instr_has_allocated_bits ( instr_t * instr );

bool instr_needs_encoding ( instr_t * instr );

void instr_free_raw_bits ( dcontext_t * dcontext , instr_t * instr );

void instr_allocate_raw_bits ( dcontext_t * dcontext , instr_t * instr , uint num_bytes );

instr_t * instr_set_translation ( instr_t * instr , app_pc addr );

app_pc instr_get_translation ( instr_t * instr );

void instr_make_persistent ( dcontext_t * dcontext , instr_t * instr );

byte * instr_get_raw_bits ( instr_t * instr );

byte instr_get_raw_byte ( instr_t * instr , uint pos );

uint instr_get_raw_word ( instr_t * instr , uint pos );

void instr_set_raw_byte ( instr_t * instr , uint pos , byte val );

void instr_set_raw_bytes ( instr_t * instr , byte * start , uint num_bytes );

void instr_set_raw_word ( instr_t * instr , uint pos , uint word );

int instr_length ( dcontext_t * dcontext , instr_t * instr );

instr_t * instr_expand ( dcontext_t * dcontext , instrlist_t * ilist , instr_t * instr );

bool instr_is_level_0 ( instr_t * instr );

instr_t * instr_get_next_expanded ( dcontext_t * dcontext , instrlist_t * ilist , instr_t * instr );

instr_t * instr_get_prev_expanded ( dcontext_t * dcontext , instrlist_t * ilist , instr_t * instr );

instr_t * instrlist_first_expanded ( dcontext_t * dcontext , instrlist_t * ilist );

instr_t * instrlist_last_expanded ( dcontext_t * dcontext , instrlist_t * ilist );

void instr_decode_cti ( dcontext_t * dcontext , instr_t * instr );

void instr_decode_opcode ( dcontext_t * dcontext , instr_t * instr );

void instr_decode ( dcontext_t * dcontext , instr_t * instr );

instr_t * instr_decode_with_current_dcontext ( instr_t * instr );

void instrlist_decode_cti ( dcontext_t * dcontext , instrlist_t * ilist );

void loginst ( dcontext_t * dcontext , uint level , instr_t * instr , char * string );

void logopnd ( dcontext_t * dcontext , uint level , opnd_t opnd , char * string );

void logtrace ( dcontext_t * dcontext , uint level , instrlist_t * trace , char * string );

void instr_shrink_to_16_bits ( instr_t * instr );

void instr_shrink_to_32_bits ( instr_t * instr );

bool instr_uses_reg ( instr_t * instr , reg_id_t reg );

bool instr_reg_in_dst ( instr_t * instr , reg_id_t reg );

bool instr_reg_in_src ( instr_t * instr , reg_id_t reg );

bool instr_reads_from_reg ( instr_t * instr , reg_id_t reg );

bool instr_writes_to_reg ( instr_t * instr , reg_id_t reg );

bool instr_writes_to_exact_reg ( instr_t * instr , reg_id_t reg );

bool instr_replace_src_opnd ( instr_t * instr , opnd_t old_opnd , opnd_t new_opnd );

bool instr_same ( instr_t * inst1 , instr_t * inst2 );

bool instr_reads_memory ( instr_t * instr );

bool instr_writes_memory ( instr_t * instr );

bool instr_rip_rel_valid ( instr_t * instr );

void instr_set_rip_rel_valid ( instr_t * instr , bool valid );

uint instr_get_rip_rel_pos ( instr_t * instr );

void instr_set_rip_rel_pos ( instr_t * instr , uint pos );

bool instr_get_rel_addr_target ( instr_t * instr , app_pc * target );

bool instr_has_rel_addr_reference ( instr_t * instr );

int instr_get_rel_addr_dst_idx ( instr_t * instr );

int instr_get_rel_addr_src_idx ( instr_t * instr );

bool instr_is_our_mangling ( instr_t * instr );

void instr_set_our_mangling ( instr_t * instr , bool ours );

bool instr_compute_address_ex_priv ( instr_t * instr , priv_mcontext_t * mc , uint index , app_pc * addr , bool * is_write , uint * pos );

bool instr_compute_address_ex ( instr_t * instr , dr_mcontext_t * mc , uint index , app_pc * addr , bool * is_write );

bool instr_compute_address_ex_pos ( instr_t * instr , dr_mcontext_t * mc , uint index , app_pc * addr , bool * is_write , uint * pos );

app_pc instr_compute_address_priv ( instr_t * instr , priv_mcontext_t * mc );

app_pc instr_compute_address ( instr_t * instr , dr_mcontext_t * mc );

uint instr_memory_reference_size ( instr_t * instr );

app_pc decode_memory_reference_size ( dcontext_t * dcontext , app_pc pc , uint * size_in_bytes );

dr_instr_label_data_t * instr_get_label_data_area ( instr_t * instr );

uint instr_branch_type ( instr_t * cti_instr );

app_pc instr_get_branch_target_pc ( instr_t * cti_instr );

void instr_set_branch_target_pc ( instr_t * cti_instr , app_pc pc );

bool instr_is_exit_cti ( instr_t * instr );

bool instr_is_mov ( instr_t * instr );

bool instr_is_call ( instr_t * instr );

bool instr_is_jmp(instr_t *instr);

bool instr_is_call_direct ( instr_t * instr );

bool instr_is_near_call_direct ( instr_t * instr );

bool instr_is_call_indirect ( instr_t * instr );

bool instr_is_return ( instr_t * instr );

bool instr_is_cbr ( instr_t * instr );

bool instr_is_mbr ( instr_t * instr );

bool instr_is_far_cti ( instr_t * instr );

bool instr_is_far_abs_cti ( instr_t * instr );

bool instr_is_ubr ( instr_t * instr );

bool instr_is_near_ubr ( instr_t * instr );

bool instr_is_cti ( instr_t * instr );

bool instr_is_cti_short ( instr_t * instr );

bool instr_is_cti_loop ( instr_t * instr );

bool instr_is_cti_short_rewrite ( instr_t * instr , byte * pc );

bool instr_is_interrupt ( instr_t * instr );

int instr_get_interrupt_number ( instr_t * instr );

bool instr_is_syscall ( instr_t * instr );

bool instr_is_mov_constant ( instr_t * instr , ptr_int_t * value );

bool instr_is_prefetch ( instr_t * instr );

bool instr_is_floating ( instr_t * instr );

bool opcode_is_mmx ( int op );

bool opcode_is_sse_or_sse2 ( int op );

bool type_is_sse ( int type );

bool instr_is_mmx ( instr_t * instr );

bool instr_is_sse_or_sse2 ( instr_t * instr );

bool instr_is_mov_imm_to_tos ( instr_t * instr );

bool instr_is_label ( instr_t * instr );

bool instr_is_undefined ( instr_t * instr );

void instr_invert_cbr ( instr_t * instr );

instr_t * instr_convert_short_meta_jmp_to_long ( dcontext_t * dcontext , instrlist_t * ilist , instr_t * instr );

bool instr_cbr_taken ( instr_t * instr , priv_mcontext_t * mcontext , bool pre );

bool instr_jcc_taken ( instr_t * instr , reg_t eflags );

int instr_cmovcc_to_jcc ( int cmovcc_opcode );

bool instr_cmovcc_triggered ( instr_t * instr , reg_t eflags );

bool instr_uses_fp_reg ( instr_t * instr );

bool reg_is_gpr ( reg_id_t reg );

bool reg_is_segment ( reg_id_t reg );

bool reg_is_ymm ( reg_id_t reg );

bool reg_is_xmm ( reg_id_t reg );

bool reg_is_mmx ( reg_id_t reg );

bool reg_is_fp ( reg_id_t reg );

instr_t * instr_create_0dst_0src ( dcontext_t * dcontext , int opcode );

instr_t * instr_create_0dst_1src ( dcontext_t * dcontext , int opcode , opnd_t src );

instr_t * instr_create_0dst_2src ( dcontext_t * dcontext , int opcode , opnd_t src1 , opnd_t src2 );

instr_t * instr_create_0dst_3src ( dcontext_t * dcontext , int opcode , opnd_t src1 , opnd_t src2 , opnd_t src3 );

instr_t * instr_create_1dst_0src ( dcontext_t * dcontext , int opcode , opnd_t dst );

instr_t * instr_create_1dst_1src ( dcontext_t * dcontext , int opcode , opnd_t dst , opnd_t src );

instr_t * instr_create_1dst_2src ( dcontext_t * dcontext , int opcode , opnd_t dst , opnd_t src1 , opnd_t src2 );

instr_t * instr_create_1dst_3src ( dcontext_t * dcontext , int opcode , opnd_t dst , opnd_t src1 , opnd_t src2 , opnd_t src3 );

instr_t * instr_create_1dst_5src ( dcontext_t * dcontext , int opcode , opnd_t dst , opnd_t src1 , opnd_t src2 , opnd_t src3 , opnd_t src4 , opnd_t src5 );

instr_t * instr_create_2dst_0src ( dcontext_t * dcontext , int opcode , opnd_t dst1 , opnd_t dst2 );

instr_t * instr_create_2dst_1src ( dcontext_t * dcontext , int opcode , opnd_t dst1 , opnd_t dst2 , opnd_t src );

instr_t * instr_create_2dst_2src ( dcontext_t * dcontext , int opcode , opnd_t dst1 , opnd_t dst2 , opnd_t src1 , opnd_t src2 );

instr_t * instr_create_2dst_3src ( dcontext_t * dcontext , int opcode , opnd_t dst1 , opnd_t dst2 , opnd_t src1 , opnd_t src2 , opnd_t src3 );

instr_t * instr_create_2dst_4src ( dcontext_t * dcontext , int opcode , opnd_t dst1 , opnd_t dst2 , opnd_t src1 , opnd_t src2 , opnd_t src3 , opnd_t src4 );

instr_t * instr_create_3dst_0src ( dcontext_t * dcontext , int opcode , opnd_t dst1 , opnd_t dst2 , opnd_t dst3 );

instr_t * instr_create_3dst_3src ( dcontext_t * dcontext , int opcode , opnd_t dst1 , opnd_t dst2 , opnd_t dst3 , opnd_t src1 , opnd_t src2 , opnd_t src3 );

instr_t * instr_create_3dst_4src ( dcontext_t * dcontext , int opcode , opnd_t dst1 , opnd_t dst2 , opnd_t dst3 , opnd_t src1 , opnd_t src2 , opnd_t src3 , opnd_t src4 );

instr_t * instr_create_3dst_5src ( dcontext_t * dcontext , int opcode , opnd_t dst1 , opnd_t dst2 , opnd_t dst3 , opnd_t src1 , opnd_t src2 , opnd_t src3 , opnd_t src4 , opnd_t src5 );

instr_t * instr_create_4dst_1src ( dcontext_t * dcontext , int opcode , opnd_t dst1 , opnd_t dst2 , opnd_t dst3 , opnd_t dst4 , opnd_t src );

instr_t * instr_create_4dst_4src ( dcontext_t * dcontext , int opcode , opnd_t dst1 , opnd_t dst2 , opnd_t dst3 , opnd_t dst4 , opnd_t src1 , opnd_t src2 , opnd_t src3 , opnd_t src4 );

instr_t * instr_create_popa ( dcontext_t * dcontext );

instr_t * instr_create_pusha ( dcontext_t * dcontext );

instr_t * instr_create_raw_1byte ( dcontext_t * dcontext , byte byte1 );

instr_t * instr_create_raw_2bytes ( dcontext_t * dcontext , byte byte1 , byte byte2 );

instr_t * instr_create_raw_3bytes ( dcontext_t * dcontext , byte byte1 , byte byte2 , byte byte3 );

instr_t * instr_create_raw_4bytes ( dcontext_t * dcontext , byte byte1 , byte byte2 , byte byte3 , byte byte4 );

instr_t * instr_create_raw_5bytes ( dcontext_t * dcontext , byte byte1 , byte byte2 , byte byte3 , byte byte4 , byte byte5 );

instr_t * instr_create_raw_6bytes ( dcontext_t * dcontext , byte byte1 , byte byte2 , byte byte3 , byte byte4 , byte byte5 , byte byte6 );

instr_t * instr_create_raw_7bytes ( dcontext_t * dcontext , byte byte1 , byte byte2 , byte byte3 , byte byte4 , byte byte5 , byte byte6 , byte byte7 );

instr_t * instr_create_raw_8bytes ( dcontext_t * dcontext , byte byte1 , byte byte2 , byte byte3 , byte byte4 , byte byte5 , byte byte6 , byte byte7 , byte byte8 );

instr_t * instr_create_nbyte_nop ( dcontext_t * dcontext , uint num_bytes , bool raw );

bool instr_is_nop ( instr_t * inst );

bool set_x86_mode ( dcontext_t * dcontext , bool x86 );

bool get_x86_mode ( dcontext_t * dcontext );

opnd_size_t resolve_var_reg_size ( opnd_size_t sz , bool is_reg );

opnd_size_t resolve_variable_size ( decode_info_t * di , opnd_size_t sz , bool is_reg );

opnd_size_t resolve_variable_size_dc ( dcontext_t * dcontext , uint prefixes , opnd_size_t sz , bool is_reg );

opnd_size_t resolve_addr_size ( decode_info_t * di );

bool optype_is_indir_reg ( int optype );

opnd_size_t indir_var_reg_size ( decode_info_t * di , int optype );

int indir_var_reg_offs_factor ( int optype );

typedef enum {DECODE_REG_REG , DECODE_REG_BASE , DECODE_REG_INDEX , DECODE_REG_RM}decode_reg_t;

reg_id_t resolve_var_reg ( decode_info_t * di , reg_id_t reg32 , bool addr , bool can_shrink , bool default_64 , bool can_grow , bool extendable );

byte * decode_eflags_usage ( dcontext_t * dcontext , byte * pc , uint * usage );

byte * decode_opcode ( dcontext_t * dcontext , byte * pc , instr_t * instr );

byte * decode ( dcontext_t * dcontext , byte * pc , instr_t * instr );

byte * decode_from_copy ( dcontext_t * dcontext , byte * copy_pc , byte * orig_pc , instr_t * instr );

const instr_info_t * get_next_instr_info ( const instr_info_t * info );

byte decode_first_opcode_byte ( int opcode );

const char * decode_opcode_name ( int opcode );

enum {VARLEN_NONE , VARLEN_MODRM , VARLEN_FP_OP , VARLEN_ESCAPE , VARLEN_3BYTE_38_ESCAPE , VARLEN_3BYTE_3A_ESCAPE};

int decode_sizeof ( dcontext_t * dcontext , byte * start_pc , int * num_prefixes , uint * rip_rel_pos );

byte * decode_cti ( dcontext_t * dcontext , byte * pc , instr_t * instr );

byte * decode_next_pc ( dcontext_t * dcontext , byte * pc );

byte * decode_raw ( dcontext_t * dcontext , byte * pc , instr_t * instr );

const instr_info_t * instr_info_extra_opnds ( const instr_info_t * info );

bool instr_is_encoding_possible ( instr_t * instr );

const instr_info_t * get_encoding_info ( instr_t * instr );

byte instr_info_opnd_type ( const instr_info_t * info , bool src , int num );

byte * copy_and_re_relativize_raw_instr ( dcontext_t * dcontext , instr_t * instr , byte * dst_pc , byte * final_pc );

byte * instr_encode_ignore_reachability ( dcontext_t * dcontext , instr_t * instr , byte * pc );

byte * instr_encode_check_reachability ( dcontext_t * dcontext , instr_t * instr , byte * pc , bool * has_instr_opnds );

byte * instr_encode_to_copy ( dcontext_t * dcontext , instr_t * instr , byte * copy_pc , byte * final_pc );

byte * instr_encode ( dcontext_t * dcontext , instr_t * instr , byte * pc );

byte * instrlist_encode_to_copy ( dcontext_t * dcontext , instrlist_t * ilist , byte * copy_pc , byte * final_pc , byte * max_pc , bool has_instr_jmp_targets );

byte * instrlist_encode ( dcontext_t * dcontext , instrlist_t * ilist , byte * pc , bool has_instr_jmp_targets );

instr_t * convert_to_near_rel_meta ( dcontext_t * dcontext , instrlist_t * ilist , instr_t * instr );

void convert_to_near_rel ( dcontext_t * dcontext , instr_t * instr );

byte * remangle_short_rewrite ( dcontext_t * dcontext , instr_t * instr , byte * pc , app_pc target );

void proc_init ( void );

uint proc_get_vendor ( void );

int proc_set_vendor ( uint new_vendor );

uint proc_get_family ( void );

uint proc_get_type ( void );

uint proc_get_model ( void );

uint proc_get_stepping ( void );

bool proc_has_feature ( feature_bit_t f );

features_t * proc_get_all_feature_bits ( void );

char * proc_get_brand_string ( void );

cache_size_t proc_get_L1_icache_size ( void );

cache_size_t proc_get_L1_dcache_size ( void );

cache_size_t proc_get_L2_cache_size ( void );

const char * proc_get_cache_size_str ( cache_size_t size );

size_t proc_get_cache_line_size ( void );

bool proc_is_cache_aligned ( void * addr );

ptr_uint_t proc_bump_to_end_of_cache_line ( ptr_uint_t sz );

void * proc_get_containing_page ( void * addr );

void machine_cache_sync ( void * pc_start , void * pc_end , bool flush_icache );

size_t proc_fpstate_save_size ( void );

size_t proc_save_fpstate ( byte * buf );

void proc_restore_fpstate ( byte * buf );

void dr_insert_save_fpstate ( void * drcontext , instrlist_t * ilist , instr_t * where , opnd_t buf );

void dr_insert_restore_fpstate ( void * drcontext , instrlist_t * ilist , instr_t * where , opnd_t buf );

void instrlist_meta_preinsert ( instrlist_t * ilist , instr_t * where , instr_t * inst );

void instrlist_meta_postinsert ( instrlist_t * ilist , instr_t * where , instr_t * inst );

void instrlist_meta_append ( instrlist_t * ilist , instr_t * inst );

void instrlist_meta_fault_preinsert ( instrlist_t * ilist , instr_t * where , instr_t * inst );

void instrlist_meta_fault_postinsert ( instrlist_t * ilist , instr_t * where , instr_t * inst );

void instrlist_meta_fault_append ( instrlist_t * ilist , instr_t * inst );

#ifndef GRANARY
struct ftrace_branch_data {const char * func; const char * file; unsigned line; union {struct {unsigned long correct; unsigned long incorrect;}; struct {unsigned long miss; unsigned long hit;}; unsigned long miss_hit [ 2 ];};};
#endif

void instr_set_note(instr_t *instr, void *value);

void *
instr_get_note(instr_t *instr);

int
instr_num_dsts(instr_t *instr);

int
instr_num_srcs(instr_t *instr);

bool
instr_ok_to_mangle(instr_t *instr);

opnd_t
opnd_create_pc(app_pc pc);

opnd_t
opnd_create_reg(reg_id_t r);

opnd_t
opnd_create_null(void);

bool
opnd_is_far_rel_addr(opnd_t opnd);

bool
opnd_is_near_rel_addr(opnd_t opnd);

bool
opnd_is_far_base_disp(opnd_t op);

bool
opnd_is_near_base_disp(opnd_t op);

bool
opnd_is_instr(opnd_t op);

bool
opnd_is_pc(opnd_t op);

bool
opnd_is_immed(opnd_t op);

/* opnd_t predicates */

/* Simple predicates */
#define OPND_IS_NULL(op)        ((op).kind == NULL_kind)
#define OPND_IS_IMMED_INT(op)   ((op).kind == IMMED_INTEGER_kind)
#define OPND_IS_IMMED_FLOAT(op) ((op).kind == IMMED_FLOAT_kind)
#define OPND_IS_NEAR_PC(op)     ((op).kind == PC_kind)
#define OPND_IS_NEAR_INSTR(op)  ((op).kind == INSTR_kind)
#define OPND_IS_REG(op)         ((op).kind == REG_kind)
#define OPND_IS_BASE_DISP(op)   ((op).kind == BASE_DISP_kind)
#define OPND_IS_FAR_PC(op)      ((op).kind == FAR_PC_kind)
#define OPND_IS_FAR_INSTR(op)   ((op).kind == FAR_INSTR_kind)
#define OPND_IS_MEM_INSTR(op)   ((op).kind == MEM_INSTR_kind)
#define OPND_IS_VALID(op)       ((op).kind < LAST_kind)

#define opnd_is_null            OPND_IS_NULL
#define opnd_is_immed_int       OPND_IS_IMMED_INT
#define opnd_is_immed_float     OPND_IS_IMMED_FLOAT
#define opnd_is_near_pc         OPND_IS_NEAR_PC
#define opnd_is_near_instr      OPND_IS_NEAR_INSTR
#define opnd_is_reg             OPND_IS_REG
#define opnd_is_base_disp       OPND_IS_BASE_DISP
#define opnd_is_far_pc          OPND_IS_FAR_PC
#define opnd_is_far_instr       OPND_IS_FAR_INSTR
#define opnd_is_mem_instr       OPND_IS_MEM_INSTR
#define opnd_is_valid           OPND_IS_VALID

#ifdef X64
# define OPSZ_PTR OPSZ_8       /**< Operand size for pointer values. */
# define OPSZ_STACK OPSZ_8     /**< Operand size for stack push/pop operand sizes. */
#else
# define OPSZ_PTR OPSZ_4       /**< Operand size for pointer values. */
# define OPSZ_STACK OPSZ_4     /**< Operand size for stack push/pop operand sizes. */
#endif
#define OPSZ_VARSTACK OPSZ_4x8_short2 /**< Operand size for prefix-varying stack
 * push/pop operand sizes. */
#define OPSZ_REXVARSTACK OPSZ_4_rex8_short2 /* Operand size for prefix/rex-varying
 * stack push/pop like operand sizes. */

#define OPSZ_ret OPSZ_4x8_short2xi8 /**< Operand size for ret instruction. */
#define OPSZ_call OPSZ_ret         /**< Operand size for push portion of call. */

/* Convenience defines for specific opcodes */
#define OPSZ_lea OPSZ_0              /**< Operand size for lea memory reference. */
#define OPSZ_invlpg OPSZ_0           /**< Operand size for invlpg memory reference. */
#define OPSZ_xlat OPSZ_1             /**< Operand size for xlat memory reference. */
#define OPSZ_clflush OPSZ_1          /**< Operand size for clflush memory reference. */
#define OPSZ_prefetch OPSZ_1         /**< Operand size for prefetch memory references. */
#define OPSZ_lgdt OPSZ_6x10          /**< Operand size for lgdt memory reference. */
#define OPSZ_sgdt OPSZ_6x10          /**< Operand size for sgdt memory reference. */
#define OPSZ_lidt OPSZ_6x10          /**< Operand size for lidt memory reference. */
#define OPSZ_sidt OPSZ_6x10          /**< Operand size for sidt memory reference. */
#define OPSZ_bound OPSZ_8_short4     /**< Operand size for bound memory reference. */
#define OPSZ_maskmovq OPSZ_8         /**< Operand size for maskmovq memory reference. */
#define OPSZ_maskmovdqu OPSZ_16      /**< Operand size for maskmovdqu memory reference. */
#define OPSZ_fldenv OPSZ_28_short14  /**< Operand size for fldenv memory reference. */
#define OPSZ_fnstenv OPSZ_28_short14 /**< Operand size for fnstenv memory reference. */
#define OPSZ_fnsave OPSZ_108_short94 /**< Operand size for fnsave memory reference. */
#define OPSZ_frstor OPSZ_108_short94 /**< Operand size for frstor memory reference. */
#define OPSZ_fxsave OPSZ_512         /**< Operand size for fxsave memory reference. */
#define OPSZ_fxrstor OPSZ_512        /**< Operand size for fxrstor memory reference. */

#ifndef INT8_MIN
# define INT8_MIN   SCHAR_MIN
# define INT8_MAX   SCHAR_MAX
# define INT16_MIN  SHRT_MIN
# define INT16_MAX  SHRT_MAX
# define INT32_MIN  INT_MIN
# define INT32_MAX  INT_MAX
#endif
/* DR_API EXPORT END */

/* alternative names */
/* we do not equate the fwait+op opcodes
 *   fstsw, fstcw, fstenv, finit, fclex
 * for us that has to be a sequence of instructions: a separate fwait
 */
/* 16-bit versions that have different names */
#define OP_cbw        OP_cwde /**< Alternative opcode name for 16-bit version. */
#define OP_cwd        OP_cdq /**< Alternative opcode name for 16-bit version. */
#define OP_jcxz       OP_jecxz /**< Alternative opcode name for 16-bit version. */
/* 64-bit versions that have different names */
#define OP_jrcxz      OP_jecxz     /**< Alternative opcode name for 64-bit version. */
#define OP_cmpxchg16b OP_cmpxchg8b /**< Alternative opcode name for 64-bit version. */
#define OP_pextrq     OP_pextrd    /**< Alternative opcode name for 64-bit version. */
#define OP_pinsrq     OP_pinsrd    /**< Alternative opcode name for 64-bit version. */
/* reg-reg version has different name */
#define OP_movhlps    OP_movlps /**< Alternative opcode name for reg-reg version. */
#define OP_movlhps    OP_movhps /**< Alternative opcode name for reg-reg version. */
/* condition codes */
#define OP_jae_short  OP_jnb_short  /**< Alternative opcode name. */
#define OP_jnae_short OP_jb_short   /**< Alternative opcode name. */
#define OP_ja_short   OP_jnbe_short /**< Alternative opcode name. */
#define OP_jna_short  OP_jbe_short  /**< Alternative opcode name. */
#define OP_je_short   OP_jz_short   /**< Alternative opcode name. */
#define OP_jne_short  OP_jnz_short  /**< Alternative opcode name. */
#define OP_jge_short  OP_jnl_short  /**< Alternative opcode name. */
#define OP_jg_short   OP_jnle_short /**< Alternative opcode name. */
#define OP_jae  OP_jnb        /**< Alternative opcode name. */
#define OP_jnae OP_jb         /**< Alternative opcode name. */
#define OP_ja   OP_jnbe       /**< Alternative opcode name. */
#define OP_jna  OP_jbe        /**< Alternative opcode name. */
#define OP_je   OP_jz         /**< Alternative opcode name. */
#define OP_jne  OP_jnz        /**< Alternative opcode name. */
#define OP_jge  OP_jnl        /**< Alternative opcode name. */
#define OP_jg   OP_jnle       /**< Alternative opcode name. */
#define OP_setae  OP_setnb    /**< Alternative opcode name. */
#define OP_setnae OP_setb     /**< Alternative opcode name. */
#define OP_seta   OP_setnbe   /**< Alternative opcode name. */
#define OP_setna  OP_setbe    /**< Alternative opcode name. */
#define OP_sete   OP_setz     /**< Alternative opcode name. */
#define OP_setne  OP_setnz    /**< Alternative opcode name. */
#define OP_setge  OP_setnl    /**< Alternative opcode name. */
#define OP_setg   OP_setnle   /**< Alternative opcode name. */
#define OP_cmovae  OP_cmovnb  /**< Alternative opcode name. */
#define OP_cmovnae OP_cmovb   /**< Alternative opcode name. */
#define OP_cmova   OP_cmovnbe /**< Alternative opcode name. */
#define OP_cmovna  OP_cmovbe  /**< Alternative opcode name. */
#define OP_cmove   OP_cmovz   /**< Alternative opcode name. */
#define OP_cmovne  OP_cmovnz  /**< Alternative opcode name. */
#define OP_cmovge  OP_cmovnl  /**< Alternative opcode name. */
#define OP_cmovg   OP_cmovnle /**< Alternative opcode name. */
/* undocumented opcodes */
#define OP_icebp OP_int1
#define OP_setalc OP_salc

#ifdef __cplusplus
} /* extern */
} /* kernel namespace */
#endif

#endif /* Granary_DYNAMORIO_H_ */
