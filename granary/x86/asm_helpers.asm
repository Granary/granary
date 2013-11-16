/* Copyright 2012-2013 Peter Goodman, all rights reserved. */

#ifndef Granary_ASM_HELPERS_ASM_
#define Granary_ASM_HELPERS_ASM_

#define PUSH_LAST_REG(reg) \
    push %reg ;

#define PUSH_REG(reg, rest) \
    PUSH_LAST_REG(reg) \
    rest

#define POP_LAST_REG(reg) \
    pop %reg ;

#define POP_REG(reg, rest) \
    rest \
    POP_LAST_REG(reg)



#define PUSH_LAST_REG_ASM_ARG(reg) \
    push %%reg ;

#define PUSH_REG_ASM_ARG(reg, rest) \
    PUSH_LAST_REG_ASM_ARG(reg) \
    rest

#define POP_LAST_REG_ASM_ARG(reg) \
    pop %%reg ;

#define POP_REG_ASM_ARG(reg, rest) \
    rest \
    POP_LAST_REG_ASM_ARG(reg)


#define PUSH_LAST_XMM_REG(off, reg) \
    movaps %reg, off(%rsp);

#define PUSH_XMM_REG(off, reg, rest) \
    PUSH_LAST_XMM_REG(off, reg) \
    rest

#define POP_LAST_XMM_REG(off, reg) \
    movaps off(%rsp), %reg;

#define POP_XMM_REG(off, reg, rest) \
    rest \
    POP_LAST_XMM_REG(off, reg)


/// used for counting the space needed to store all registers
#define PLUS_8_8 16
#define PLUS_8_16 24
#define PLUS_8_24 32
#define PLUS_8_32 40
#define PLUS_8_40 48
#define PLUS_8_48 56
#define PLUS_8_56 64
#define PLUS_8_64 72
#define PLUS_8_72 80
#define PLUS_8_80 88
#define PLUS_8_88 96
#define PLUS_8_96 104
#define PLUS_8_104 112
#define PLUS_8_112 120
#define PLUS_8_120 128
#define PLUS_8_128 136
#define PLUS_8_136 144
#define PLUS_8_144 152
#define PLUS_8_152 160

#define PLUS_EIGHT(_,rest) CAT(PLUS_8_, rest)
#define EIGHT(_) 8

/// used to save and restore registers
#define PUSHA ALL_REGS(PUSH_REG, PUSH_LAST_REG)
#define POPA ALL_REGS(POP_REG, POP_LAST_REG)
#define PUSHA_SIZE ALL_REGS(PLUS_EIGHT, EIGHT)

#define PUSHA_ASM_ARG ALL_REGS(PUSH_REG_ASM_ARG, PUSH_LAST_REG_ASM_ARG)
#define POPA_ASM_ARG ALL_REGS(POP_REG_ASM_ARG, POP_LAST_REG_ASM_ARG)

/// use to save and restore call-clobbered registers
#define PUSHA_CALL ALL_CALL_REGS(PUSH_REG, PUSH_LAST_REG)
#define POPA_CALL ALL_CALL_REGS(POP_REG, POP_LAST_REG)

/// used to save and restore xmm registers. Note: we assume that %rsp is
/// 16-byte aligned.
#define PUSHA_XMM \
    lea -256(%rsp), %rsp; \
    ALL_XMM_REGS(PUSH_XMM_REG, PUSH_LAST_XMM_REG)

#define POPA_XMM \
    ALL_XMM_REGS(POP_XMM_REG, POP_LAST_XMM_REG) \
    lea 256(%rsp), %rsp; \

#if CONFIG_ENV_KERNEL || !GRANARY_USE_PIC
#   define SHARED_SYMBOL SYMBOL
#else
#   ifdef __APPLE__
#       define SHARED_SYMBOL(x) SYMBOL(x)(%rip)
#   else // Linux
#       define SHARED_SYMBOL(x) SYMBOL(x)@PLT
#   endif
#endif

#if CONFIG_ENV_KERNEL || !GRANARY_USE_PIC
#   define EXTERN_SYMBOL SYMBOL
#else
#   define EXTERN_SYMBOL SHARED_SYMBOL
#endif


/// Gives us a single register that isn't the
/// one inputted.
#define NOT_rax rcx
#define NOT_rcx rdx
#define NOT_rdx rbx
#define NOT_rbx rbp
#define NOT_rbp rsi
#define NOT_rsi rdi
#define NOT_rdi r8
#define NOT_r8 r9
#define NOT_r9 r10
#define NOT_r10 r11
#define NOT_r11 r12
#define NOT_r12 r13
#define NOT_r13 r14
#define NOT_r14 r15
#define NOT_r15 rax

/// Gives us a register that isn't RAX, if our current register is one of
/// the specified. This is convenient if we know that we need to modify RAX,
/// but don't know if our input register is RAX or not.
#define NOT_rax_rax rcx
#define NOT_rax_rcx rdx
#define NOT_rax_rdx rbx
#define NOT_rax_rbx rbp
#define NOT_rax_rbp rsi
#define NOT_rax_rsi rdi
#define NOT_rax_rdi r8
#define NOT_rax_r8 r9
#define NOT_rax_r9 r10
#define NOT_rax_r10 r11
#define NOT_rax_r11 r12
#define NOT_rax_r12 r13
#define NOT_rax_r13 r14
#define NOT_rax_r14 r15
#define NOT_rax_r15 rdx

/// Gives us a second register that isn't RAX, if our current register is one
/// of the specified, assuming we've already claimed a register according to
/// `NOT_rax_ ## reg`.
#define NOT_rax2_rax rdx
#define NOT_rax2_rcx rbx
#define NOT_rax2_rdx rbp
#define NOT_rax2_rbx rsi
#define NOT_rax2_rbp rsi
#define NOT_rax2_rsi r8
#define NOT_rax2_rdi r9
#define NOT_rax2_r8 r10
#define NOT_rax2_r9 r11
#define NOT_rax2_r10 r12
#define NOT_rax2_r11 r13
#define NOT_rax2_r12 r14
#define NOT_rax2_r13 r15
#define NOT_rax2_r14 rcx
#define NOT_rax2_r15 rbp

#define SPILL_REG__(reg) CAT(NOT_, reg)
#define SPILL_REG_(reg) SPILL_REG__(reg)
#define SPILL_REG(reg) SPILL_REG_(reg)

#define SPILL_REG_NOT_RAX__(reg) CAT(NOT_rax_, reg)
#define SPILL_REG_NOT_RAX_(reg) SPILL_REG_NOT_RAX__(reg)
#define SPILL_REG_NOT_RAX(reg) SPILL_REG_NOT_RAX_(reg)

#define REG64_TO_REG16_rax ax
#define REG64_TO_REG16_rcx cx
#define REG64_TO_REG16_rdx dx
#define REG64_TO_REG16_rbx bx
#define REG64_TO_REG16_rbp bp
#define REG64_TO_REG16_rsi si
#define REG64_TO_REG16_rdi di
#define REG64_TO_REG16_r8 r8w
#define REG64_TO_REG16_r9 r9w
#define REG64_TO_REG16_r10 r10w
#define REG64_TO_REG16_r11 r11w
#define REG64_TO_REG16_r12 r12w
#define REG64_TO_REG16_r13 r13w
#define REG64_TO_REG16_r14 r14w
#define REG64_TO_REG16_r15 r15w

#define REG64_TO_REG32_rax eax
#define REG64_TO_REG32_rcx ecx
#define REG64_TO_REG32_rdx edx
#define REG64_TO_REG32_rbx ebx
#define REG64_TO_REG32_rbp ebp
#define REG64_TO_REG32_rsi esi
#define REG64_TO_REG32_rdi edi
#define REG64_TO_REG32_r8 r8d
#define REG64_TO_REG32_r9 r9d
#define REG64_TO_REG32_r10 r10d
#define REG64_TO_REG32_r11 r11d
#define REG64_TO_REG32_r12 r12d
#define REG64_TO_REG32_r13 r13d
#define REG64_TO_REG32_r14 r14d
#define REG64_TO_REG32_r15 r15d

#define REG64_TO_REG16__(reg) CAT(REG64_TO_REG16_, reg)
#define REG64_TO_REG16_(reg) REG64_TO_REG16__(reg)
#define REG64_TO_REG16(reg) REG64_TO_REG16_(reg)

#define REG64_TO_REG32__(reg) CAT(REG64_TO_REG32_, reg)
#define REG64_TO_REG32_(reg) REG64_TO_REG32__(reg)
#define REG64_TO_REG32(reg) REG64_TO_REG32_(reg)

#define REG_LIST(...) (__VA_ARGS__)
#define LIST_NOT_REG_rax REG_LIST( r14,r9,rcx,rsi,r10,rbx,r11,r8,rdx,rbp,r15,r12,rdi,r13 )
#define LIST_NOT_REG_rcx REG_LIST( r14,r9,rsi,r10,rbx,r11,r8,rdx,rbp,r15,r12,rdi,rax,r13 )
#define LIST_NOT_REG_rdx REG_LIST( r14,r9,rcx,rsi,r10,rbx,r11,r8,rbp,r15,r12,rdi,rax,r13 )
#define LIST_NOT_REG_rbx REG_LIST( r14,r9,rcx,rsi,r10,r11,r8,rdx,rbp,r15,r12,rdi,rax,r13 )
#define LIST_NOT_REG_rbp REG_LIST( r14,r9,rcx,rsi,r10,rbx,r11,r8,rdx,r15,r12,rdi,rax,r13 )
#define LIST_NOT_REG_rsi REG_LIST( r14,r9,rcx,r10,rbx,r11,r8,rdx,rbp,r15,r12,rdi,rax,r13 )
#define LIST_NOT_REG_rdi REG_LIST( r14,r9,rcx,rsi,r10,rbx,r11,r8,rdx,rbp,r15,r12,rax,r13 )
#define LIST_NOT_REG_r8 REG_LIST( r14,r9,rcx,rsi,r10,rbx,r11,rdx,rbp,r15,r12,rdi,rax,r13 )
#define LIST_NOT_REG_r9 REG_LIST( r14,rcx,rsi,r10,rbx,r11,r8,rdx,rbp,r15,r12,rdi,rax,r13 )
#define LIST_NOT_REG_r10 REG_LIST( r14,r9,rcx,rsi,rbx,r11,r8,rdx,rbp,r15,r12,rdi,rax,r13 )
#define LIST_NOT_REG_r11 REG_LIST( r14,r9,rcx,rsi,r10,rbx,r8,rdx,rbp,r15,r12,rdi,rax,r13 )
#define LIST_NOT_REG_r12 REG_LIST( r14,r9,rcx,rsi,r10,rbx,r11,r8,rdx,rbp,r15,rdi,rax,r13 )
#define LIST_NOT_REG_r13 REG_LIST( r14,r9,rcx,rsi,r10,rbx,r11,r8,rdx,rbp,r15,r12,rdi,rax )
#define LIST_NOT_REG_r14 REG_LIST( r9,rcx,rsi,r10,rbx,r11,r8,rdx,rbp,r15,r12,rdi,rax,r13 )
#define LIST_NOT_REG_r15 REG_LIST( r14,r9,rcx,rsi,r10,rbx,r11,r8,rdx,rbp,r12,rdi,rax,r13 )
#define LIST_ELEMENT_0__( a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13 ) a0
#define LIST_ELEMENT_0_(...) LIST_ELEMENT_0__( __VA_ARGS__)
#define LIST_ELEMENT_0(...) LIST_ELEMENT_0_( __VA_ARGS__)
#define LIST_ELEMENT_1__( a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13 ) a1
#define LIST_ELEMENT_1_(...) LIST_ELEMENT_1__( __VA_ARGS__)
#define LIST_ELEMENT_1(...) LIST_ELEMENT_1_( __VA_ARGS__)
#define LIST_ELEMENT_2__( a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13 ) a2
#define LIST_ELEMENT_2_(...) LIST_ELEMENT_2__( __VA_ARGS__)
#define LIST_ELEMENT_2(...) LIST_ELEMENT_2_( __VA_ARGS__)
#define LIST_ELEMENT_3__( a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13 ) a3
#define LIST_ELEMENT_3_(...) LIST_ELEMENT_3__( __VA_ARGS__)
#define LIST_ELEMENT_3(...) LIST_ELEMENT_3_( __VA_ARGS__)
#define LIST_ELEMENT_4__( a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13 ) a4
#define LIST_ELEMENT_4_(...) LIST_ELEMENT_4__( __VA_ARGS__)
#define LIST_ELEMENT_4(...) LIST_ELEMENT_4_( __VA_ARGS__)
#define LIST_ELEMENT_5__( a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13 ) a5
#define LIST_ELEMENT_5_(...) LIST_ELEMENT_5__( __VA_ARGS__)
#define LIST_ELEMENT_5(...) LIST_ELEMENT_5_( __VA_ARGS__)
#define LIST_ELEMENT_6__( a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13 ) a6
#define LIST_ELEMENT_6_(...) LIST_ELEMENT_6__( __VA_ARGS__)
#define LIST_ELEMENT_6(...) LIST_ELEMENT_6_( __VA_ARGS__)
#define LIST_ELEMENT_7__( a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13 ) a7
#define LIST_ELEMENT_7_(...) LIST_ELEMENT_7__( __VA_ARGS__)
#define LIST_ELEMENT_7(...) LIST_ELEMENT_7_( __VA_ARGS__)
#define LIST_ELEMENT_8__( a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13 ) a8
#define LIST_ELEMENT_8_(...) LIST_ELEMENT_8__( __VA_ARGS__)
#define LIST_ELEMENT_8(...) LIST_ELEMENT_8_( __VA_ARGS__)
#define LIST_ELEMENT_9__( a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13 ) a9
#define LIST_ELEMENT_9_(...) LIST_ELEMENT_9__( __VA_ARGS__)
#define LIST_ELEMENT_9(...) LIST_ELEMENT_9_( __VA_ARGS__)
#define LIST_ELEMENT_10__( a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13 ) a10
#define LIST_ELEMENT_10_(...) LIST_ELEMENT_10__( __VA_ARGS__)
#define LIST_ELEMENT_10(...) LIST_ELEMENT_10_( __VA_ARGS__)
#define LIST_ELEMENT_11__( a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13 ) a11
#define LIST_ELEMENT_11_(...) LIST_ELEMENT_11__( __VA_ARGS__)
#define LIST_ELEMENT_11(...) LIST_ELEMENT_11_( __VA_ARGS__)
#define LIST_ELEMENT_12__( a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13 ) a12
#define LIST_ELEMENT_12_(...) LIST_ELEMENT_12__( __VA_ARGS__)
#define LIST_ELEMENT_12(...) LIST_ELEMENT_12_( __VA_ARGS__)
#define LIST_ELEMENT_13__( a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13 ) a13
#define LIST_ELEMENT_13_(...) LIST_ELEMENT_13__( __VA_ARGS__)
#define LIST_ELEMENT_13(...) LIST_ELEMENT_13_( __VA_ARGS__)
#define NOT_REG_0(not_reg) LIST_ELEMENT_0 CAT(LIST_NOT_REG_, not_reg)
#define NOT_REG_1(not_reg) LIST_ELEMENT_1 CAT(LIST_NOT_REG_, not_reg)
#define NOT_REG_2(not_reg) LIST_ELEMENT_2 CAT(LIST_NOT_REG_, not_reg)
#define NOT_REG_3(not_reg) LIST_ELEMENT_3 CAT(LIST_NOT_REG_, not_reg)
#define NOT_REG_4(not_reg) LIST_ELEMENT_4 CAT(LIST_NOT_REG_, not_reg)
#define NOT_REG_5(not_reg) LIST_ELEMENT_5 CAT(LIST_NOT_REG_, not_reg)
#define NOT_REG_6(not_reg) LIST_ELEMENT_6 CAT(LIST_NOT_REG_, not_reg)
#define NOT_REG_7(not_reg) LIST_ELEMENT_7 CAT(LIST_NOT_REG_, not_reg)
#define NOT_REG_8(not_reg) LIST_ELEMENT_8 CAT(LIST_NOT_REG_, not_reg)
#define NOT_REG_9(not_reg) LIST_ELEMENT_9 CAT(LIST_NOT_REG_, not_reg)
#define NOT_REG_10(not_reg) LIST_ELEMENT_10 CAT(LIST_NOT_REG_, not_reg)
#define NOT_REG_11(not_reg) LIST_ELEMENT_11 CAT(LIST_NOT_REG_, not_reg)
#define NOT_REG_12(not_reg) LIST_ELEMENT_12 CAT(LIST_NOT_REG_, not_reg)
#define NOT_REG_13(not_reg) LIST_ELEMENT_13 CAT(LIST_NOT_REG_, not_reg)

/// Useful for documentation
#define COMMENT_HASH #
#define COMMENT(...) COMMENT_HASH __VA_ARGS__ @N@

#define ENTER_PRIVATE_STACK()       call EXTERN_SYMBOL(granary_enter_private_stack)
#define EXIT_PRIVATE_STACK()        call EXTERN_SYMBOL(granary_exit_private_stack)

#endif /* Granary_ASM_HELPERS_ASM_ */
