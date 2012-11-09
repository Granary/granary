/* **********************************************************
 * Copyright (c) 2001-2010 VMware, Inc.  All rights reserved.
 * ********************************************************** */

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
/* Copyright (c) 2001 Hewlett-Packard Company */

/*
 * x86.asm - x86 specific assembly and trampoline code
 *
 * This file is used for both linux and windows.
 * We used to use the gnu assembler on both platforms, but
 * gas does not support 64-bit windows.
 * Thus we now use masm on windows and gas with the new intel-syntax-specifying
 * options so that our code here only needs a minimum of macros to
 * work on both.
 *
 * Note that for gas on cygwin we used to need to prepend _ to global
 * symbols: we don't need that for linux gas or masm so we don't do it anymore.
 */

/* We handle different registers and calling conventions with a CPP pass.
 * It can be difficult to choose registers that work across all ABI's we're
 * trying to support: we need to move each ARG into a register in case
 * it is passed in memory, but we have to pick registers that don't already
 * hold other arguments.  Typically, use this order:
 *   REG_XAX, REG_XBX, REG_XDI, REG_XSI, REG_XDX, REG_XCX
 * Note that REG_XBX is by convention used on linux for PIC base: if we want 
 * to try and avoid relocations (case 7852) we should avoid using it 
 * to avoid confusion (though we can always pick a different register, 
 * even varying by function).
 * FIXME: should we use virtual registers instead?
 * FIXME: should we have ARG1_IN_REG macro that is either nop or load from stack?
 * For now not bothering, but if we add more routines we'll want more support.
 * Naturally the ARG* macros are only valid at function entry.
 */

#include "asm_defines.asm"
.intel_syntax noprefix;
START_FILE

#ifdef LINUX
# include "syscall.h"
#endif
        
#define RESTORE_FROM_DCONTEXT_VIA_REG(reg,offs,dest) mov dest, PTRSZ [offs + reg]
#define SAVE_TO_DCONTEXT_VIA_REG(reg,offs,src) mov PTRSZ [offs + reg], src

/* For the few remaining dcontext_t offsets we need here: */
#ifdef WINDOWS
# ifdef X64
#  define UPCXT_EXTRA 8 /* at_syscall + 4 bytes of padding */
# else
#  define UPCXT_EXTRA 4 /* at_syscall */
# endif
#else
# define UPCXT_EXTRA 8 /* errno + at_syscall */
#endif

/* We should give asm_defines.asm all unique names and then include globals.h
 * and avoid all this duplication!
 */
#ifdef X64
# ifdef WINDOWS
#  define NUM_XMM_SLOTS 6 /* xmm0-5 */
# else
#  define NUM_XMM_SLOTS 16 /* xmm0-15 */
# endif
#else
# define NUM_XMM_SLOTS 8 /* xmm0-7 */
#endif
#define XMM_SAVED_SIZE ((NUM_XMM_SLOTS)*16) /* xmm0-5/15 for PR 264138/PR 302107 */

/* Should we generate all of our asm code instead of having it static?
 * As it is we're duplicating insert_push_all_registers(), dr_insert_call(), etc.,
 * but it's not that much code here in these macros, and this is simpler
 * than emit_utils.c-style code.
 */
#ifdef X64
/* push all registers in dr_mcontext_t order.  does NOT make xsp have a
 * pre-push value as no callers need that (they all use PUSH_DR_MCONTEXT).
 * Leaves space for, but does NOT fill in, the xmm0-5 slots (PR 264138),
 * since it's hard to dynamically figure out during bootstrapping whether
 * movdqu or movups are legal instructions.  The caller is expected
 * to fill in the xmm values prior to any calls that may clobber them.
 */
# define PUSHALL \
        lea      rsp, [rsp - XMM_SAVED_SIZE] @N@\
        push     r15 @N@\
        push     r14 @N@\
        push     r13 @N@\
        push     r12 @N@\
        push     r11 @N@\
        push     r10 @N@\
        push     r9  @N@\
        push     r8  @N@\
        push     rax @N@\
        push     rcx @N@\
        push     rdx @N@\
        push     rbx @N@\
        /* not the pusha pre-push rsp value but see above */ @N@\
        push     rsp @N@\
        push     rbp @N@\
        push     rsi @N@\
        push     rdi
# define POPALL        \
        pop      rdi @N@\
        pop      rsi @N@\
        pop      rbp @N@\
        pop      rbx /* rsp into dead rbx */ @N@\
        pop      rbx @N@\
        pop      rdx @N@\
        pop      rcx @N@\
        pop      rax @N@\
        pop      r8  @N@\
        pop      r9  @N@\
        pop      r10 @N@\
        pop      r11 @N@\
        pop      r12 @N@\
        pop      r13 @N@\
        pop      r14 @N@\
        pop      r15 @N@\
        lea      rsp, [rsp + XMM_SAVED_SIZE]
# define DR_MCONTEXT_SIZE (18*ARG_SZ + XMM_SAVED_SIZE)
# define dstack_OFFSET     (DR_MCONTEXT_SIZE+UPCXT_EXTRA+3*ARG_SZ)
#else
# define PUSHALL \
        lea      esp, [esp - XMM_SAVED_SIZE] @N@\
        pusha
# define POPALL  \
        popa @N@\
        lea      esp, [esp + XMM_SAVED_SIZE]
# define DR_MCONTEXT_SIZE (10*ARG_SZ + XMM_SAVED_SIZE)
# define dstack_OFFSET     (DR_MCONTEXT_SIZE+UPCXT_EXTRA+3*ARG_SZ)
#endif
#define is_exiting_OFFSET (dstack_OFFSET+3*ARG_SZ+4)
#define PUSHALL_XSP_OFFS  (3*ARG_SZ)
#define MCONTEXT_XSP_OFFS (PUSHALL_XSP_OFFS)
#define MCONTEXT_PC_OFFS  (DR_MCONTEXT_SIZE-ARG_SZ)

/* Pushes a dr_mcontext_t on the stack, with an xsp value equal to the
 * xsp before the pushing.  Clobbers xax!
 * Does fill in xmm0-5, if necessary, for PR 264138.
 * Assumes that DR has been initialized (get_xmm_vals() checks proc feature bits).
 * Caller should ensure 16-byte stack alignment prior to the push (PR 306421).
 */
#define PUSH_DR_MCONTEXT(pc)                              \
        push     pc                                    @N@\
        PUSHF                                          @N@\
        PUSHALL                                        @N@\
        lea      REG_XAX, [REG_XSP]                    @N@\
        CALLC1(get_xmm_vals, REG_XAX)                  @N@\
        lea      REG_XAX, [DR_MCONTEXT_SIZE + REG_XSP] @N@\
        mov      [PUSHALL_XSP_OFFS + REG_XSP], REG_XAX



/*#############################################################################
 *#############################################################################
 * Utility routines moved here due to the lack of inline asm support
 * in VC8.
 */

/* uint atomic_swap(uint *addr, uint value)
 * return current contents of addr and replace contents with value.
 * on win32 could use InterlockedExchange intrinsic instead.
 */
        DECLARE_FUNC(atomic_swap)
GLOBAL_LABEL(atomic_swap:)
        mov      REG_XAX, ARG2
        mov      REG_XCX, ARG1 /* nop on win64 (ditto for linux64 if used rdi) */
        xchg     [REG_XCX], eax
        ret
        END_FUNC(atomic_swap)

/* bool cpuid_supported(void)
 * Checks for existence of the cpuid instr by attempting to modify bit 21 of eflags
 */
        DECLARE_FUNC(cpuid_supported)
GLOBAL_LABEL(cpuid_supported:)
        PUSHF
        pop      REG_XAX
        mov      ecx, eax      /* original eflags in ecx */
        xor      eax, HEX(200000) /* try to modify bit 21 of eflags */
        push     REG_XAX
        POPF
        PUSHF
        pop      REG_XAX
        cmp      ecx, eax
        mov      eax, 0        /* zero out top bytes */
        setne    al
        push     REG_XCX         /* now restore original eflags */
        POPF
        ret
        END_FUNC(cpuid_supported)

/* void our_cpuid(int res[4], int eax)
 * Executes cpuid instr, which is hard for x64 inline asm b/c clobbers rbx and can't
 * push in middle of func.
 */
        DECLARE_FUNC(our_cpuid)
GLOBAL_LABEL(our_cpuid:)
        mov      REG_XDX, ARG1
        mov      REG_XAX, ARG2
        push     REG_XBX /* callee-saved */
        push     REG_XDI /* callee-saved */
        /* not making a call so don't bother w/ 16-byte stack alignment */
        mov      REG_XDI, REG_XDX
        cpuid
        mov      [ 0 + REG_XDI], eax
        mov      [ 4 + REG_XDI], ebx
        mov      [ 8 + REG_XDI], ecx
        mov      [12 + REG_XDI], edx
        pop      REG_XDI /* callee-saved */
        pop      REG_XBX /* callee-saved */
        ret
        END_FUNC(our_cpuid)

END_FILE
