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
    
    while True:
      old_line = line
      line = PASTE.sub(r"\1\2", line)
      if old_line == line:
        break
    print line
