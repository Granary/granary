Control Flow
============
Granary manages control-flow instructions in several different ways. There are
four classes of control flow transfer instructions:

1.  Direct control-flow transfer instructions.
2.  Indirect control-flow transfer instructions.
3.  Unexpected, asynchronous control-flow transfer instructions.
4.  Unexpected, synchronous control-flow transfer instructions.

This document discusses classes 1 and 2 and leaves classes 3 and 4 for
[interrupts and exceptions](interrupts-exceptions.md).

Control-flow handling in the presence of interrupts is an important
consideration in the design of how Granary handles control flow. For example,
we want to minimize the "target size" of any control-flow lookup primitives
within basic blocks. We also want to localize all control-flow lookup primitives
to some well-defined region so that if an interrupt occurs at a non-opportune
moment, we can know very quickly "what" we are doing and how we want to react
(i.e. delay interrupts until we know the branch target).

Direct Control Flow
-------------------
TODO
- key idea: fixed number of thread-local slots, same number of direct branch
            encoders. Each direct branch in a basic block is directed to a
            pointer with a one-to-one correspondence with one of the free slots.

            when called, that pointer lazily performs the direct branch lookup
            and block translation and then patches the original code to point
            to the new basic block.


Indirect Control Flow
---------------------
Indirect control flow lookup is based on operand-specific branch lookup/replace
routines. For example, the translation of a `jmp *%rax` instruction is a
`jmp indirect_jmp_rax`, where `indirect_unsafe_rax` is the address of an `%rax`-specific
indirect branch lookup routine. If there is a `call *%rax` then this translates
to a similar `call indirect_call_rax`.

The distinction between the `call` and `jmp` versions of the indirect branch
lookup routines follows from the assumptions made about register state. That is,
across a call boundary, we assume that the caller is following the ABI and is
saving certain registers to avoid their being clobbered (this assumption might
need to be revised). In the case of a `jmp`, no assumptions about already saved
registers can be made, nor are any registers "free" to clobber (e.g. `%r10`),
and so `jmp` branch lookup routines must be careful.

Indirect branch lookup routines disable interrupts before performing branch
lookup. If the routine is interrupted before interrupts are disabled then
interrupts will--and can safely be--taken.