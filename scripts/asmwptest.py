"""Automatically generate the instructions that need to be manually
tested for the watchpoint instrumentation.

The purpose of this script is to parse the output of `objdump` and
derive a string of assembly instructions that should be tested
by Granary. Granary will test these instructions by decoding each
and using the extra information about these instructions made
available to the DynamoRIO side of Granary to auto-generate
instruction-specific test cases. These test cases will be run
"natively" and instrumented, with their results compared.

Author:       Peter Goodman (peter.goodman@gmail.com)
Copyright:    Copyright 2012-2013 Peter Goodman, all rights reserved.
"""

import collections

from asmunify import *


INSTRUCTIONS = collections.defaultdict(UnifiedInstruction)


class RegisterRenamer(object):
  """Manages renaming registers in a consistent way.

  Registers participating in a memory operand are renamed
  according to one set of choices, while non-memory operand
  registers are renamed in another way.

  While this approach to renaming does not guarantee that all
  index registers will receive the same name, it generally
  yields a good enough renaming such that we can group many
  like instructions together."""

  RENAME_BASE = map(Register.parse, (
    "%r11", "%r12", "%r13", "%r14", 
  ))

  RENAME_ORDER_REG = map(Register.parse, (
    "%r8", "%r9", "%r10",
  ))

  # Set of instructions that can't have their registers renamed.
  CANT_RENAME = set([
    "XLAT",
    "INS", "MOVS", "OUTS", "LODS", "STOS", "CMPS", "SCAS", 
  ])

  __slots__ = (
    'unused',
    'used',
  )

  def __init__(self):
    self.unused = {
      AddressOperand: list(self.RENAME_ORDER_MEM),
      RegisterOperand: list(self.RENAME_ORDER_REG)
    }
    self.used = {}

  def _get_reg_root(self, reg):
    while reg.parent:
      reg = reg.parent
    return reg

  def _get_reg_map(self, reg):
    regs = self._get_reg_root(reg).children
    reg_map = collections.defaultdict(list)
    for reg in regs:
      reg_map[reg.size].append(reg)
    return reg_map

  def _assign_new_reg(self, renamed_reg, cls):
    new_name_map = self._get_reg_map(self.unused[cls][-1])
    old_name_map = self._get_reg_map(renamed_reg)
    self.unused[cls].pop()

    for size, old_names in old_name_map.items():
      for old_name in old_names:
        self.used[old_name] = new_name_map[size][0]

  def _rename_reg(self, reg, cls):
    if reg not in self.used:
      self._assign_new_reg(reg, cls)
    return self.used[reg]

  # Renames the registers of an instruction. We use the newer
  # 64-bit registers (r8-r15) to avoid tricky clashes with
  # implicit operands that aren't specified in the assembly.
  def rename(self, ins):
    if ins.mnemonic_disambiguated in self.CANT_RENAME:
      return

    for op in ins.operands:
      if isinstance(op, AddressOperand) \
      or isinstance(op, RegisterOperand):
        if op.base_register \
        and Register.KIND_GENERAL_PURPOSE == op.base_register.kind:
          op.base_register = self._rename_reg(
              op.base_register, op.__class__)

      if isinstance(op, AddressOperand) \
      and op.index_register \
      and Register.KIND_GENERAL_PURPOSE == op.index_register.kind:
        op.index_register = self._rename_reg(
            op.index_register, op.__class__)


# Decide whether or not to collect this instruction as a source
# for test generation.
def collect_instruction(ins):
  if ins.is_cti() \
  or not ins.accesses_memory() \
  or ins.operates_on_stack() \
  or ins.uses_fpu():
    return

  renamer = RegisterRenamer()
  renamer.rename(ins)

  global INSTRUCTIONS
  INSTRUCTIONS[ins.mnemonic_disambiguated].unify(ins)


if __name__ == "__main__":
  import sys
  parser = ASMParser(collect_instruction)

  with open(sys.argv[1], "r") as lines:
    parser.parse(lines)

  for _, uins in INSTRUCTIONS.items():
    gen_ins = set()
    for ins in uins.generate():
      gen_ins.add(str(ins))

    for ins in gen_ins:
      print ins
    print