"""Post-process an assembly file and do simple symbol concatenation 
(in case the assembly pre-processor does not support it) and replace
the @N@ symbol with a new line.

Author:       Peter Goodman (peter.goodman@gmail.com)
Copyright:    Copyright 2012-2013 Peter Goodman, all rights reserved.
"""

import sys
import re

NL = re.compile(r"@N@")
POPA = re.compile(r"@POP_ALL_REGS@")
PUSHA = re.compile(r"@PUSH_ALL_REGS@")
POPA_XMM = re.compile(r"@POP_ALL_XMM_REGS@")
PUSHA_XMM = re.compile(r"@PUSH_ALL_XMM_REGS@")
PASTE = re.compile(r"(.*?)\s*##\s*(.*?)")

# open up a post-processed assembly file and replace each @N@ with a new line
# and if the preprocessor didn't do token pasting, then automatically perform
# it.
with open(sys.argv[1]) as lines:
  for line in lines:
    line = NL.sub("\n", line.rstrip(" \t\r\n")).rstrip("\n")
    line = POPA.sub(
        "pop %r15 ; pop %r14 ; pop %r13 ; pop %r12 ; pop %r11 ; pop %r10 ; pop %r9 ; pop %r8 ; pop %rdi ; pop %rsi ; pop %rbp ; pop %rdx ; pop %rcx ; pop %rbx ; pop %rax ;",
        line)
    line = PUSHA.sub(
        "push %rax ; push %rbx ; push %rcx ; push %rdx ; push %rbp ; push %rsi ; push %rdi ; push %r8 ; push %r9 ; push %r10 ; push %r11 ; push %r12 ; push %r13 ; push %r14 ; push %r15 ;",
        line)
    line = PUSHA_XMM.sub(
        "lea -256(%rsp), %rsp; movaps %xmm0, 240(%rsp); movaps %xmm1, 224(%rsp); movaps %xmm2, 208(%rsp); movaps %xmm3, 192(%rsp); movaps %xmm4, 176(%rsp); movaps %xmm5, 160(%rsp); movaps %xmm6, 144(%rsp); movaps %xmm7, 128(%rsp); movaps %xmm8, 112(%rsp); movaps %xmm9, 96(%rsp); movaps %xmm10, 80(%rsp); movaps %xmm11, 64(%rsp); movaps %xmm12, 48(%rsp); movaps %xmm13, 32(%rsp); movaps %xmm14, 16(%rsp); movaps %xmm15, 0(%rsp);",
        line)
    line = POPA_XMM.sub(
        "movaps 0(%rsp), %xmm15; movaps 16(%rsp), %xmm14; movaps 32(%rsp), %xmm13; movaps 48(%rsp), %xmm12; movaps 64(%rsp), %xmm11; movaps 80(%rsp), %xmm10; movaps 96(%rsp), %xmm9; movaps 112(%rsp), %xmm8; movaps 128(%rsp), %xmm7; movaps 144(%rsp), %xmm6; movaps 160(%rsp), %xmm5; movaps 176(%rsp), %xmm4; movaps 192(%rsp), %xmm3; movaps 208(%rsp), %xmm2; movaps 224(%rsp), %xmm1; movaps 240(%rsp), %xmm0; lea 256(%rsp), %rsp;",
        line)
    
    while True:
      old_line = line
      line = PASTE.sub(r"\1\2", line)
      if old_line == line:
        break
    print line
