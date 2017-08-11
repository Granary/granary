"""Compute conservative sets of registers that are dead at the end of
each basic block.

Copyright (C) 2013 Peter Goodman. All rights reserved."""


import collections
import data_flow
import parser


# All 16 general-purpose registers
ALL_LIVE = ((1 << 16) - 1)
RAX   = (1 << 0)
RCX   = (1 << 1)
RDX   = (1 << 2)
RBX   = (1 << 3)
RSP   = (1 << 4)
RBP   = (1 << 5)
RSI   = (1 << 6)
RDI   = (1 << 7)
R8    = (1 << 8)
R9    = (1 << 9)
R10   = (1 << 10)
R11   = (1 << 11)
R12   = (1 << 12)
R13   = (1 << 13)
R14   = (1 << 14)
R15   = (1 << 15)


# Registers always marked as live.
FORCE_LIVE = RSP


# Includes return registers and callee-saved registers. RSP is always live
# as well.
LIVE_ON_RETURN = RAX | RDX | RBX | RSP | RBP | R12 | R13 | R14 | R15


# Returns the initial register liveness state for a node. This initial
# value has all registers marked as live (=1).
def initial(bb):
  if bb.is_function_exit:
    return LIVE_ON_RETURN
  return ALL_LIVE


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
    return ALL_LIVE

  return FORCE_LIVE | (ALL_LIVE & ~reduce(meet_two_dead, incoming_dead))


# Returns True iff the set of new live registers is the same or smaller
# than the previous set of live registers. Here, the TOP is all registers
# live, and the BOTTOM is all registers dead.
def check(old_live, new_live):
  return new_live <= old_live


# Output some stuff to stdout.
def O(*args):
  print "".join(str(s) for s in args)


# Generate nested C++ if statements for doing a binary search on some values.
def gen_bin_search(bbs, indent):
  if not bbs:
    return

  if 1 == len(bbs):
    O(indent, "if(", hex(bbs[0][0]), " == app_offset) return ", hex(bbs[0][1]), ";")
    return

  # If we've got a small number of block offsets to check, then just check
  # them sequentially.
  if 4 >= len(bbs):
    for bb in bbs:
      gen_bin_search([bb], indent)
    return

  split = len(bbs) / 2
  first_half = bbs[:split]
  second_half = bbs[split:]

  O(indent, "if(", hex(first_half[-1][0]), " >= app_offset) {")
  gen_bin_search(first_half, indent + "    ")
  O(indent, "} else {")
  gen_bin_search(second_half, indent + "    ")
  O(indent, "}")


# Generate a C++ function for getting some computed dead register information
# about a module.
def gen_reg_getter(app, bbs):
  bbs.sort(key=lambda t: t[0])
  O("    static uint32_t get_live_registers_in_", app, "(uint32_t app_offset) {")
  gen_bin_search(bbs, "        ")
  O("        return 0;")
  O("    }")


# Parse a file given as input.
if __name__ == "__main__":
  import sys

  include = data_flow.DataFlowProblem.DONT_FILTER
  file_name = None
  
  # Parse the arguments.
  for arg in sys.argv[1:]:
    arg = arg.strip()
    if arg.startswith("--apps="):
      apps = arg[len("--apps="):].split(",")
      include = lambda bb: bb.app_name in apps
    else:
      file_name = arg

  if not file_name:
    print "Must specify a file to parse."
    exit(-1)

  # Run the data flow problem.
  with open(file_name) as lines:
    cfg = parser.ControlFlowGraph()
    cfg.parse(lines)
    
    problem = data_flow.DataFlowProblem(
        cfg,
        flow=data_flow.DataFlowProblem.INTRA,
        direction=data_flow.DataFlowProblem.BACKWARD,
        include=include)

    results = problem.run(
        initial=initial,
        meet=meet,
        process=process,
        check=check)

    bbs = collections.defaultdict(list)
    nums_bbs, num_exit_bbs = 0, 0
    for bb, dead_regs in results.items():
      if ALL_LIVE == dead_regs:
        continue
      nums_bbs += 1
      if bb.is_function_exit:
        num_exit_bbs += 1
      bbs[bb.app_name].append((bb.app_offset_begin, dead_regs))

    O("/* AUTO-GENERATED FILE by clients/cfg/scripts/dead_regs.py */")
    O("/* Results for ", nums_bbs, " basic blocks, of which ", num_exit_bbs, " are function exit blocks. */")
    O("")
    O("#define WEAK_SYMBOL")
    O("#include \"granary/globals.h\"")
    O("#include \"granary/register.h\"")
    O("")
    O("#if CONFIG_ENV_KERNEL")
    O("#   include \"granary/kernel/linux/module.h\"")
    O("extern \"C\" {")
    O("    /// Returns the kernel module information for a given app address.")
    O("    extern const kernel_module *kernel_get_module(granary::app_pc addr);")
    O("}")
    O("#endif")
    O("")
    O("namespace granary {")
    O("")
    O("")

    for app in bbs:
      gen_reg_getter(app, bbs[app])
      O("")
      O("")

    O("    /// Returns the set of registers that are live at the end of a basic block.")
    O("    /// If we already know about the basic block by having computed the")
    O("    /// (conservative) sets of live registers at the ends of basic blocks")
    O("    /// in advance (e.g. with the CFG tool) then we use that information.")
    O("    register_manager get_live_registers(const app_pc bb_start_addr) {")
    O("        register_manager live_regs; // default = all live")
    O("#if CONFIG_ENV_KERNEL")
    O("        const kernel_module *module(kernel_get_module(bb_start_addr));")
    O("        if(!module) {")
    O("            return live_regs;")
    O("        }")

    for app in bbs:
      O("        if(0 == strcmp(\"", app, "\", module->name)) {")
      O("            const uint32_t offset(static_cast<uint32_t>(")
      O("                bb_start_addr - reinterpret_cast<app_pc>(module->text_begin)));")
      O("            const uint32_t live_regs_int(get_live_registers_in_", app, "(offset));")
      O("            if(live_regs_int) {")
      O("                live_regs.decode(live_regs_int);")
      O("            }")
      O("        }")

    O("#endif")
    O("        return live_regs;")
    O("    }")
    O("")
    O("} /* namespace granary */")
    O("")
