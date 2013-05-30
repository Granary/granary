/* Copyright 2012-2013 Peter Goodman, all rights reserved. */
/*
 * test_auto_data.asm
 *
 *  Created on: 2013-05-29
 *      Author: Peter Goodman
 */

#include "granary/x86/asm_defines.asm"
#include "granary/x86/asm_helpers.asm"

START_FILE

    .globl SYMBOL(granary_wp_auto_instructions_begin)
    .globl SYMBOL(granary_wp_auto_instructions_end)

GLOBAL_LABEL(granary_wp_auto_instructions_begin:)

    /// Sequences of memory instructions generated from the the following
    /// binaries:
    ///     - Linux 3.8.2
    ///     - Clang 3.2
    ///     - Python 2.7
    repz cmpsb ;

    subw $0x0,(%r14) ;
    lock subl %r10d,(%r14,%r13,1) ;
    subl $0x0,(%r14,%r13,1) ;
    subl (%r14),%r10d ;
    subw %r10w,(%r14,%r13,1) ;
    subl $0x0,(%r14) ;
    subw (%r14),%r10w ;
    subq (%r14),%r14 ;
    subq (%r14),%r10 ;
    subb $0x0,(%r14) ;
    subw $0x0,(%r14,%r13,1) ;
    subq $0x0,(%r14,%r13,1) ;
    subq $0x0,(,%r14,1) ;
    subl (%r14,%r13,1),%r10d ;
    subq $0x0,(%r14) ;
    subb (%r14),%r10b ;
    lock subl %r10d,(%r14) ;
    subl $0x0,(,%r14,1) ;
    subb %r10b,(%r14) ;
    lock subl $0x0,(%r14) ;
    subb $0x0,(%r14,%r13,1) ;
    subl %r10d,(%r14) ;
    subl (,%r14,1),%r10d ;
    subq %r10,(%r14) ;
    subw %r10w,(%r14) ;
    subw (%r14,%r13,1),%r10w ;
    lock subq %r10,(%r14) ;
    subq (%r14,%r13,1),%r14 ;
    subq (%r14,%r13,1),%r10 ;
    subl %r10d,(%r14,%r13,1) ;

    cmovnsl (%r14),%r10d ;
    cmovnsq (%r14),%r10 ;

    cmovael (%r14),%r10d ;
    cmovaeq (%r14),%r10 ;
    cmovaew (%r14),%r10w ;

    imull $0x0,(%r14,%r13,1),%r14d ;
    imull (%r14),%r10d ;
    imull $0x0,(%r14,%r13,1),%r10d ;
    imulq $0x0,(%r14),%r10 ;
    imull $0x0,(%r14),%r14d ;
    imull (,%r14,1),%r10d ;
    imull $0x0,(%r14),%r10d ;
    imulq (%r14),%r10 ;
    imulq (%r14,%r13,1),%r10 ;
    imull (%r14,%r13,1),%r10d ;

    movw (%r14),%r14w ;
    movq (%r14,%r14,1),%r13 ;
    movq (%r14,%r14,1),%r10 ;
    movq (%r14,%r14,1),%r14 ;
    movb (%r14,%r13,1),%r10b ;
    movb $0x0,(%r14,%r13,1) ;
    movb (%r14),%r10b ;
    movq (,%r14,1),%r10 ;
    movb (%r14d),%r13b ;
    movb %gs:(%r14),%r14b ;
    movw %r10w,(%r14,%r10,1) ;
    movl (%r14d),%r14d ;
    movq (%r14,%r13,1),%r14 ;
    movl (,%r14,1),%r14d ;
    movl $0x0,(%r14,%r13,1) ;
    movq $0x0,(,%r14,1) ;
    movw (%r14,%r14,1),%r14w ;
    movq (%r14),%r14 ;
    movl %r10d,(%r14) ;
    movq (%r14),%r10 ;
    movw %cs,(%r14,%r13,1) ;
    movl %gs:(,%r14,1),%r13d ;
    movl %r10d,%gs:(%r14) ;
    movl (%r14,%r13,1),%r14d ;
    movb (%r14d),%r14b ;
    movw (%r14,%r13,1),%r10w ;
    movl (,%r14,1),%r13d ;
    movw $0x0,(%r14,%r14,1) ;
    movb %r10b,(%r14) ;
    movw %cs,(%r14,%r10,1) ;
    movl %r10d,(%r14,%r13,1) ;
    movl %gs:(,%r14,1),%r10d ;
    movl (%r14),%r10d ;
    movw %cs,(%r14d) ;
    movw %cs,(%r14,%r14,1) ;
    movq $0x0,%gs:(%r14) ;
    movw %r10w,(%r14d) ;
    movl %r10d,(,%r14,1) ;
    movq %r10,(%r14) ;
    movl %gs:(%r14),%r14d ;
    movb $0x0,(%r14) ;
    movl %gs:(%r14),%r10d ;
    movw (%r14d),%r10w ;
    movw $0x0,(%r14) ;
    movw %r10w,(%r14,%r13,1) ;
    movl %gs:(%r14),%r13d ;
    movb %r10b,(%r14,%r14,1) ;
    movw %r10w,(%r14) ;
    movb (%r14,%r13,1),%r14b ;
    movl (%r14d),%r13d ;
    movq %r10,(,%r14,1) ;
    movq (%r14,%r13,1),%r10 ;
    movq (%r14,%r13,1),%r13 ;
    movw (%r14),%r10w ;
    movl (%r14,%r13,1),%r13d ;
    movl $0x0,(,%r14,1) ;
    movb (%r14),%r14b ;
    movq %gs:(,%r14,1),%r10 ;
    movl (,%r14,1),%r10d ;
    movq %r10,(%r14,%r13,1) ;
    movq (,%r14,1),%r14 ;
    movq %gs:(,%r14,1),%r14 ;
    movq $0x0,(%r14) ;
    movq (,%r14,1),%r13 ;
    movb %gs:(%r14),%r10b ;
    movb $0x0,(,%r14,1) ;
    movl (%r14d),%r10d ;
    movl $0x0,%gs:(%r14) ;
    movb (%r14d),%r10b ;
    movw (%r14,%r14,1),%r10w ;
    movl (%r14,%r13,1),%r10d ;
    movq %gs:(,%r14,1),%r13 ;
    movb (,%r14,1),%r14b ;
    movw %cs,(%r14) ;
    movw (%r14,%r13,1),%r14w ;
    movb %gs:(%r14),%r13b ;
    movb (,%r14,1),%r10b ;
    movq %r10,%gs:(%r14) ;
    movl %r10d,(%r14d) ;
    movl (%r14),%r14d ;
    movw $0x0,(%r14,%r13,1) ;
    movl $0x0,(%r14) ;
    movq $0x0,(%r14,%r13,1) ;
    movl (%r14),%r13d ;
    movq %gs:(%r14),%r10 ;
    movq %gs:(%r14),%r13 ;
    movl %gs:(,%r14,1),%r14d ;
    movq %r10,(%r10) ;
    movb %r10b,%gs:(%r14) ;
    movw %r10w,(%r14,%r14,1) ;
    movb (,%r14,1),%r13b ;
    movw (%r14d),%r14w ;
    movb %r10b,(%r14,%r13,1) ;
    movb %r10b,(,%r14,1) ;
    movb (%r14,%r13,1),%r13b ;
    movb (%r14),%r13b ;
    movb %r10b,(%r14d) ;
    movq %gs:(%r14),%r14 ;
    movl $0x0,%gs:(,%r14,1) ;
    movq (%r14),%r13 ;

    btl %r10d,(%r14,%r13,1) ;
    btl $0x0,(%r14) ;
    btl %r10d,(%r14) ;

    prefetcht0 (%r14) ;
    prefetcht0 (%r14,%r13,1) ;

    mulq (%r14) ;
    mulb (%r14) ;

    divl (%r14) ;
    divq (%r14) ;

    lock decq (%r14) ;
    decb (%r14) ;
    lock decl (%r14,%r13,1) ;
    decq %gs:(%r14) ;
    decq (%r14) ;
    lock decl (%r14) ;
    lock decl (,%r14,1) ;
    decl (%r14) ;
    decw (%r14) ;
    decl %gs:(%r14) ;
    lock decw (%r14) ;

    lock cmpxchg16b (%r14) ;

    movsl ;
    rep movsq ;
    movsw ;
    movsb ;
    rep movsl ;
    rep movsb ;

    cmovbeq (%r14),%r10 ;
    cmovbel (%r14),%r10d ;
    cmovbeq (%r14,%r13,1),%r10 ;
    cmovbeq (,%r14,1),%r10 ;

    cmpq $0x0,(%r14) ;
    cmpb $0x0,(%r14) ;
    cmpl (%r14),%r10d ;
    cmpb %r10b,(%r14,%r13,1) ;
    cmpq (%r14,%r13,1),%r14 ;
    cmpq (%r14,%r13,1),%r10 ;
    cmpl %r10d,(%r14,%r13,1) ;
    cmpl %r10d,(,%r14,1) ;
    cmpq %r10,(,%r14,1) ;
    cmpl $0x0,(%r14,%r13,1) ;
    cmpq %r10,(%r14) ;
    cmpl %r10d,(%r14) ;
    cmpb (%r14),%r10b ;
    cmpq %r10,(%r14,%r13,1) ;
    cmpw (%r14,%r13,1),%r10w ;
    cmpq $0x0,(%r14,%r13,1) ;
    cmpb %r10b,(%r14) ;
    cmpw $0x0,(%r14,%r13,1) ;
    cmpq (,%r14,1),%r14 ;
    cmpw %r10w,(%r14) ;
    cmpq (,%r14,1),%r10 ;
    cmpb $0x0,(,%r14,1) ;
    cmpq %gs:(%r14),%r10 ;
    cmpb (%r14,%r13,1),%r10b ;
    cmpq %gs:(%r14),%r14 ;
    cmpw $0x0,(%r14,%r14,1) ;
    cmpb $0x0,(%r14,%r13,1) ;
    cmpw $0x0,(%r14) ;
    cmpw %r10w,(%r14,%r13,1) ;
    cmpl (,%r14,1),%r10d ;
    cmpl $0x0,(,%r14,1) ;
    cmpb %r10b,(,%r14,1) ;
    cmpq $0x0,(,%r14,1) ;
    cmpq (%r14),%r14 ;
    cmpw (%r14),%r10w ;
    cmpq (%r14),%r10 ;
    cmpq %r10,(%r10) ;
    cmpw (,%r14,1),%r10w ;
    cmpl $0x0,(%r14) ;
    cmpl (%r14,%r13,1),%r10d ;

    sete (%r14) ;

    setg (%r14) ;

    seta (%r14) ;

    clflush (%r14) ;

    bsrl (%r14),%r10d ;
    bsrq (%r14,%r13,1),%r10 ;
    bsrq (%r14),%r10 ;

    lock xaddl %r10d,(%r14,%r13,1) ;
    xaddq %r10,(%r14) ;
    lock xaddq %r10,(%r14) ;
    lock xaddl %r10d,(%r14) ;
    xaddb %r10b,%gs:(%r14) ;

    notq (%r14) ;
    notw (%r14) ;
    notl (%r14) ;

    lock cmpxchgq %r10,(%r14) ;
    cmpxchgq %r10,(%r14) ;
    lock cmpxchgw %r10w,(%r14) ;
    lock cmpxchgq %r10,(,%r14,1) ;
    cmpxchgb %r10b,%gs:(%r14) ;
    lock cmpxchgl %r10d,(%r14) ;
    cmpxchgq %r10,%gs:(%r14) ;

    xchgq %r10,(,%r14,1) ;
    xchgl %r10d,(%r14) ;
    xchgb %r10b,(%r14) ;
    xchgq %r10,(%r14) ;
    xchgq %r10,(%r14,%r13,1) ;

    sarq %cl,(%rbp) ;
    sarl %cl,(%rbp) ;
    sarq $0x0,(%rbp) ;

    bsfl (%r14),%r10d ;

    shrq $0x0,(%rsi) ;
    shrl $0x0,(%r14) ;
    shrq %cl,(%rbp) ;
    shrl $0x0,(%r12) ;
    shrl $0x0,(%rbp) ;
    shrq (%rdi) ;
    shrq (%rbx) ;
    shrl %cl,(%rbp) ;
    shrq $0x0,(%rbp) ;
    shrq (%rbp) ;
    shrq (%r13) ;
    shrw (%rbx) ;
    shrq (%r14) ;
    shrb $0x0,(%rbp) ;
    shrb (%rbx) ;
    shrq %cl,(%rbx) ;
    shrl $0x0,(%rbx) ;

    xorq (%r14,%r13,1),%r10 ;
    xorl (%r14,%r13,1),%r10d ;
    xorq (%r14),%r10 ;
    xorl $0x0,(%r14) ;
    xorb %r10b,(%r14) ;
    xorb $0x0,(%r14) ;
    xorw $0x0,(%r14) ;
    xorw (%r14,%r14,1),%r10w ;
    xorb $0x0,(%r14,%r13,1) ;
    xorl (,%r14,1),%r10d ;
    xorb (%r14),%r10b ;
    xorw (%r14),%r10w ;
    xorl (%r14),%r10d ;
    xorb (%r14,%r13,1),%r10b ;
    xorw %r10w,(%r14) ;
    xorl %r10d,(%r14) ;
    xorq %r10,(%r14) ;

    tzcntq (%r14),%r10 ;

    movntiq %r10,(%r14) ;

    setne (%r14,%r14,1) ;
    setne (%r14) ;

    cmovsl (%r14),%r10d ;
    cmovsq (%r14),%r10 ;

    negq (%r14) ;
    negl (%r14) ;

    cmovgel (%r14),%r10d ;
    cmovgel (%r14,%r13,1),%r10d ;

    crc32w (%r14),%r10d ;
    crc32l (%r14),%r10d ;
    crc32b (%r14),%r10d ;
    crc32q (%r14),%r10 ;

    addl %r10d,%gs:(%r14,%r13,1) ;
    addq (%r14,%r13,1),%r14 ;
    addl %r10d,(%r14,%r13,1) ;
    addq (%r14,%r13,1),%r10 ;
    addl (%r14),%r10d ;
    addl (,%r14,1),%r10d ;
    addq (,%r14,1),%r10 ;
    addq $0x0,(%r14,%r13,1) ;
    addq (,%r14,1),%r14 ;
    addb (%r14),%r14b ;
    addq %r10,(%r14) ;
    addq $0x0,(,%r14,1) ;
    lock addl %r10d,(%r14) ;
    addq (%r14),%r10 ;
    addq (%r14),%r14 ;
    addq $0x0,(%r14) ;
    lock addl %r10d,(%r14,%r13,1) ;
    addb (%r14),%r10b ;
    addb $0x0,(%r14) ;
    addl $0x0,(%r14) ;
    addq $0x0,%gs:(%r14) ;
    addw $0x0,(%r14,%r13,1) ;
    addw %r10w,(%r14,%r13,1) ;
    addw $0x0,(%r14) ;
    addq %r10,%gs:(%r14) ;
    addb %r10b,(%r10) ;
    addw (%r14),%r10w ;
    addq %r10,(%r14,%r13,1) ;
    addb %r10b,(%r14) ;
    addq %r10,%gs:(,%r14,1) ;
    addb %r10b,(%r14,%r14,1) ;
    lock addq %r10,(,%r14,1) ;
    addl (%r14,%r13,1),%r10d ;
    lock addq %r10,(%r14,%r13,1) ;
    lock addq $0x0,(%r14) ;
    lock addl $0x0,(%r14) ;
    addl %r10d,(%r10) ;
    lock addq %r10,(%r14) ;
    addl %r10d,%gs:(%r14) ;
    addq %r10,(,%r14,1) ;
    addw %r10w,(%r14) ;
    addb (%r14,%r13,1),%r10b ;
    addl $0x0,(%r14,%r13,1) ;
    addb (%r14,%r13,1),%r14b ;
    addw (%r14,%r13,1),%r10w ;
    addl $0x0,(,%r14,1) ;
    addl %r10d,(%r14) ;

    cmovlq (%r14),%r10 ;
    cmovll (%r14),%r10d ;

    adcl (%r14),%r10d ;
    adcl (%r14,%r13,1),%r10d ;
    adcq (%r14),%r10 ;
    adcl $0x0,(%r14) ;

    movslq (,%r14,1),%r13 ;
    movslq (%r14),%r10 ;
    movslq (%r14),%r13 ;
    movslq (,%r14,1),%r10 ;
    movslq (,%r14,1),%r14 ;
    movslq (%r14),%r14 ;
    movsbl (%r14),%r10d ;
    movslq (%r14,%r13,1),%r14 ;
    movsbl (%r14),%r14d ;
    movswq (%r14),%r10 ;
    movslq (%r14,%r13,1),%r13 ;
    movslq (%r14,%r13,1),%r10 ;
    movswl (%r14),%r10d ;
    movswl (%r14),%r14d ;
    movsbq (,%r14,1),%r14 ;
    movswl (%r14,%r14,1),%r10d ;
    movswl (%r14,%r14,1),%r14d ;
    movsbq (%r14),%r14 ;

    cmoval (%r14),%r10d ;
    cmovaq (%r14),%r10 ;

    cmovbq (%r14),%r10 ;
    cmovbl (%r14),%r10d ;

    rep stosb ;
    stosb ;
    stosl ;
    rep stosl ;
    rep stosq ;
    stosw ;
    stosq ;

    shlq $0x0,(%rbp) ;
    shlq %cl,(%rbp) ;
    shlq (%r13) ;
    shll (%rbx) ;
    shll (%rbp) ;
    shlw (%rbx) ;
    shlq $0x0,(%rsi) ;
    shlq (%rbp) ;
    shlb (%rbx) ;
    shll $0x0,(%r12) ;
    shll %cl,(%rbp) ;
    shlq %cl,(%rbx) ;
    shlq (%r14) ;
    shlq $0x0,(%r14) ;

    cmoveq (%r14),%r10 ;
    cmovel (%r14,%r13,1),%r10d ;
    cmovew (%r14),%r10w ;
    cmovel (%r14),%r10d ;

    cmovgl (%r14),%r10d ;

    andw (%r14,%r13,1),%r10w ;
    andq %r10,(%r14,%r13,1) ;
    andb (%r14),%r10b ;
    andq $0x0,(%r14) ;
    andq (%r14,%r13,1),%r10 ;
    andb %r10b,(%r14,%r13,1) ;
    andw (%r14),%r10w ;
    andl $0x0,(%r14) ;
    andl %r10d,(%r14,%r13,1) ;
    andl (%r14,%r13,1),%r10d ;
    andw $0x0,(%r14,%r13,1) ;
    andw %r10w,(%r14) ;
    andl (%r14),%r10d ;
    andw $0x0,(%r14) ;
    andb $0x0,(%r14,%r13,1) ;
    andl %r10d,(%r14) ;
    andq $0x0,(%r14,%r13,1) ;
    andq (%r14),%r10 ;
    lock andb $0x0,(%r14) ;
    andb %r10b,(%r14) ;
    andw %r10w,(%r14,%r13,1) ;
    andb $0x0,(%r14) ;
    andl $0x0,(%r14,%r13,1) ;
    andb (%r14,%r13,1),%r10b ;
    andl (,%r14,1),%r10d ;
    andq %r10,(%r14) ;

    setae (%r14) ;

    lock btsl $0x0,(%r14) ;
    lock btsl %r10d,(%r14) ;
    btsl $0x0,(%r14) ;
    btsl %r10d,(%r14) ;

    btrl %r10d,(%r14) ;
    btrl $0x0,(%r14) ;
    lock btrl $0x0,(%r14) ;
    lock btrl %r10d,(%r14) ;

    movzbl (%r14,%r14,1),%r14d ;
    movzwl (%r14),%r14d ;
    movzwl (%r14),%r10d ;
    movzwl (,%r14,1),%r13d ;
    movzwl (%r14,%r14,1),%r14d ;
    movzbl (,%r14,1),%r14d ;
    movzbl (%r14,%r13,1),%r13d ;
    movzwl (%r14,%r14,1),%r10d ;
    movzbl (%r14,%r14,1),%r13d ;
    movzbl (,%r14,1),%r10d ;
    movzbq (%r14,%r13,1),%r10 ;
    movzwl (%r14,%r13,1),%r10d ;
    movzwq (%r14,%r13,1),%r10 ;
    movzwl (,%r14,1),%r10d ;
    movzbq (%r14),%r10 ;
    movzwl (%r14),%r13d ;
    movzwl (%r14,%r13,1),%r14d ;
    movzbl (%r14),%r13d ;
    movzwl (,%r14,1),%r14d ;
    movzbl (%r14),%r14d ;
    movzbl (%r14,%r13,1),%r14d ;
    movzbl (,%r14,1),%r13d ;
    movzbl (%r14,%r13,1),%r10d ;
    movzbl (%r14,%r14,1),%r10d ;
    movzwl (%r14,%r14,1),%r13d ;
    movzwl (%r14,%r13,1),%r13d ;
    movzbl (%r14),%r10d ;

    idivl (%r14) ;
    idivq (%r14) ;

    sbbl $0x0,(%r14) ;
    sbbb %r10b,(%r14) ;

    cmovlel (%r14,%r13,1),%r10d ;
    cmovlew (%r14),%r10w ;
    cmovlel (%r14),%r10d ;
    cmovleq (%r14),%r10 ;

    setle (%r14) ;

    btcl %r10d,(%r14) ;

    testq $0x0,(%r14) ;
    testq %r10,(%r14) ;
    testw %r10w,(%r14) ;
    testw $0x0,(%r14,%r13,1) ;
    testl %r10d,(%r14) ;
    testb $0x0,(%r14) ;
    testb $0x0,(,%r14,1) ;
    testl $0x0,(%r14,%r13,1) ;
    testq $0x0,(%r14,%r13,1) ;
    testb %r10b,(%r14) ;
    testw $0x0,(%r14) ;
    testw %r10w,(,%r14,1) ;
    testl %r10d,(%r14,%r13,1) ;
    testl $0x0,(%r14) ;
    testl %r10d,(,%r14,1) ;
    testb $0x0,(%r14,%r13,1) ;
    testq %r10,(%r14,%r13,1) ;

    cmovnew (%r14),%r10w ;
    cmovneq (%r14),%r10 ;
    cmovnel (%r14),%r10d ;

    orw %r10w,(%r14) ;
    orq (%r14,%r13,1),%r10 ;
    orw (%r14,%r14,1),%r10w ;
    orq %r10,(%r14,%r13,1) ;
    lock orb $0x0,(%r14) ;
    orb %r10b,(%r14,%r10,1) ;
    orb %r10b,(%r14) ;
    orb %r10b,(%r14,%r13,1) ;
    orl (%r14),%r10d ;
    orl %r10d,(%r14,%r14,1) ;
    orw %r10w,(%r14,%r13,1) ;
    orq $0x0,(%r14) ;
    orb $0x0,(%r14,%r13,1) ;
    orb (%r14),%r10b ;
    orq (,%r14,1),%r10 ;
    orw $0x0,(%r14) ;
    orb $0x0,(%r14) ;
    orl $0x0,(%r14) ;
    orl $0x0,(%r14,%r13,1) ;
    orb (%r14,%r14,1),%r10b ;
    orl (,%r14,1),%r10d ;
    orq %r10,(%r14) ;
    orq (%r14),%r10 ;
    orl %r10d,(%r14) ;
    orq $0x0,(%r14,%r13,1) ;
    orb (%r14,%r13,1),%r10b ;
    orl (%r14,%r13,1),%r10d ;
    orl %r10d,(%r14,%r13,1) ;
    orw (%r14),%r10w ;

    scasb ;
    repnz scasb ;

    incl (%r14) ;
    lock incw (%r14) ;
    incl (%r14,%r13,1) ;
    lock incl (%r14) ;
    incl %gs:(,%r14,1) ;
    lock incq (%r14) ;
    incl (,%r14,1) ;
    incq (%r14) ;
    lock incl (,%r14,1) ;
    incb (%r14) ;
    incl %gs:(%r14) ;
    incw (%r14) ;
    lock incq (%r14,%r13,1) ;
    incq %gs:(%r14,%r13,1) ;
    lock incl (%r14,%r13,1) ;
    incq %gs:(%r14) ;

GLOBAL_LABEL(granary_wp_auto_instructions_end:)

END_FILE

