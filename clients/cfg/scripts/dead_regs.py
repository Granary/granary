"""Compute conservative sets of registers that are dead at the end of
each basic block.

Copyright (C) 2013 Peter Goodman. All rights reserved."""


import parser
import data_flow


# All 16 general-purpose registers. Note: DR_REG_NULL is at (1 << 0).
ALL_BITS_ONE = ((1 << 16) - 1) << 1
RAX = (1 << 1)
RCX = (1 << 2)
RDX = (1 << 3)
RBX = (1 << 4)
RSP = (1 << 5)
RBP = (1 << 6)
RSI = (1 << 7)
RDI = (1 << 8)
R8 = (1 << 9)
R9 = (1 << 10)
R10 = (1 << 11)
R11 = (1 << 12)
R12 = (1 << 13)
R13 = (1 << 14)
R14 = (1 << 15)
R15 = (1 << 16)

# Includes return registers and callee-saved registers. RSP is always live
# as well.
LIVE_ON_RETURN = RAX | RDX | RBX | RSP | RBP | R12 | R13 | R14 | R15


# Returns the initial register liveness state for a node. This initial
# value has all registers marked as live (=1).
def initial(bb):
  if bb.is_function_exit:
    return LIVE_ON_RETURN
  return ALL_BITS_ONE


# Takes the incoming set of registers that are live at the end of the basic
# block, and combines this with the set of registers that are live at the
# beginning of the basic block given the assumption that all regs are live at
# the end of the basic block, and returns the conservative set of registers
# that are dead at the beginning of the basic block.
def process(bb, live_at_end):
  return ~((bb.used_regs & bb.entry_regs) | (~bb.used_regs & live_at_end))


# Combines two sets of dead registers. A combined register is dead iff
# it is dead (=1) in both sets.
def meet_two_dead(a_dead, b_dead):
  return a_dead & b_dead


# Meet the incoming set of dead registers. A register is dead at the
# end of a basic block iff it is dead in every successor. Therefore, if
# we cannot prove that we've visited every successor then we need to
# assume all regs are live at the end of the basic block. We 
def meet(bb, old_live, incoming_dead):
  if bb.has_outgoing_indirect_jmp \
  or len(incoming_dead) is not bb.num_outgoing_jumps:
    return ALL_BITS_ONE

  return ALL_BITS_ONE & ~reduce(meet_two_dead, incoming_dead)


# Returns True iff the set of new live registers is the same or smaller
# than the previous set of live registers. Here, the TOP is all registers
# live, and the BOTTOM is all registers dead.
def check(old_live, new_live):
  return new_live <= old_live


# Parse a file given as input.
if __name__ == "__main__":
  import sys

  file_name = None
  for arg in sys.argv[1:]:
    arg = arg.strip()
    if arg.startswith("--apps="):
      assert False  # TODO
    else:
      file_name = arg

  if not file_name:
    print "Must specify a file to parse."
    exit(-1)

  with open(file_name) as lines:
    cfg = parser.ControlFlowGraph()
    cfg.parse(lines)
    
    problem = data_flow.DataFlowProblem(
        cfg,
        flow=data_flow.DataFlowProblem.INTRA,
        direction=data_flow.DataFlowProblem.BACKWARD)

    results = problem.run(
        initial=initial,
        meet=meet,
        process=process,
        check=check)

    num_bbs = 0.0
    num_optimised_bbs = 0.0
    for bb, dead_regs in results.items():
      num_bbs += 1
      if ALL_BITS_ONE == dead_regs:
        continue
      num_optimised_bbs += 1
      print bb, bin(dead_regs)
    
    print
    print "Got better regs for %2f%% of basic blocks executed." % (100 * (num_optimised_bbs / num_bbs))
