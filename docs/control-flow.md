Control Flow
============
Granary manages control-flow instructions in several different ways. There are
four classes of control flow transfer instructions:

1.  Direct control-flow transfer instructions.
2.  Indirect control-flow transfer instructions.
3.  Unexpected, asynchronous control-flow transfer instructions (interrupts).
4.  Unexpected, synchronous control-flow transfer instructions (exceptions).

This document discusses classes 1 and 2 and leaves classes 3 and 4 for
[interrupts and exceptions](interrupts-exceptions.md).

Control-flow handling in the presence of interrupts is an important
consideration in the design of how Granary handles control flow. For example,
we want to minimize the "target size" of any control-flow lookup primitives
within basic blocks. We also want to localize all control-flow lookup primitives
to some well-defined region so that if an interrupt occurs at a non-opportune
moment, we can know very quickly "what" we are doing and how we want to react
(e.g. delay interrupts until we know the branch target).

The following sections detail how direct and indirect control flow is handled in
Granary.

Direct Control Flow
-------------------
Handling direct control flow (e.g. call to known addresses) is challenging, in
part because the code cache is shared. The major challenges involved are:

1.  Granary cannot eagerly translate the target code of a direct branch lest it
    enter a scenario where it blows the stack. As such, any policy used must be
    lazy, i.e. direct branches should be resolved only when they are executed.
2.  Granary self-imposes the requirement that if an instruction is mangled into
    and equivalent form then the mangled form will be exactly one instruction.
    This requirement simplifies interrupt handling because it externalizes the
    concerns of how to handle interrupts in the code that supports mangled code
    to something that lives "outside" of the module code cache blocks.
3.  The code cache is shared, which means any mechanism in place must work if
    another processor reaches a lazy direct control flow lookup mechanism before
    the processor that generated the code for that direct control flow
    instructions.

The approach taken is to generate short sequences of generated code that are
then used to for lookup and hotpatching of the basic block.

The process for direct branch translation and lookup follows:
1.  A direct branch is (in principle) translated into the following three
    instructions:
    ```gas
        push    <64-bit address of direct branch instruction>
        push    <64-bit target address of direct banch instruction>
        jmp     <direct branch handler>
    ```
    The above instructions contain all of the information needed to resolve and
    patch the direct branch instruction. However, the above instructions take up
    a lot of space (and cannot even be encoded as such). As such, direct
    branches are translated into an equivalent form:
    ```gas
        call <stub>
        ...
    stub:
        push <32-bit address of target relative to application start>
        jmp <direct branch handler>
    ```
    The implication is that all direct branches (including jumps) are translated
    into 5-byte calls. The purpose of the call is to put the address immediately
    following the call onto the stack. From this, we can calculate the address
    of the instruction that we will patch. Further, we can translate the 32-bit
    target back into it's 64-bit value by adding that offset to the address of
    the start of application code. Obviously, this implies that we assume that
    application code occupies no more than 4GB of space. For Linux kernel modules
    this is true.

    This is implemented by emitting the direct branch lookup stubs immediately
    following the last instruction of the basic block.

2.  When executed, the direct branch will jump the the associated instruction 
    stub, which contains enough information for the direct branch handler
    to perform a lookup and patch.

3.  The direct branch handler performs a lookup on the targeted address. If the
    targeted address is not associated with a basic block then that basic block
    is translated. If the target is associated with a basic block, or if we just
    constructed the basic block, then the translated branch instruction is hot
    patched to point to the basic block.

Note: the instructions of the direct branch lookup stub listed above are all
considered safe locations for interrupts to arrive.

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

Indirect branch lookup routines do not disable interrupts. Instead, they depend
on a higher level policy that allows the interrupt to be effectively delayed
across the lookup (and potential basic block translation) code. One the
destination has been resolved, the interrupt is emulated.
