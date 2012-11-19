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

Indirect Control Flow
---------------------
Indirect control flow lookup is based on operand-specific branch lookup/replace
routines.

