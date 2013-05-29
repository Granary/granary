"""Unify parsed assembly instructions.

This will try to unify parsed instructions according to a number of
different granularities of structural unification.

Author:       Peter Goodman (peter.goodman@gmail.com)
Copyright:    Copyright 2012-2013 Peter Goodman, all rights reserved.
"""

import collections
import itertools

from asmparser import *


class UnifiedOperand(object):
  """Represents a generator for unified operands."""
  pass


class UnifiedImmediateOperand(UnifiedOperand):
  """Represents a generator for unified immediate operands."""

  def __init__(self, _):
    pass

  def unify(self, _):
    pass

  def generate(self):
    yield ImmediateOperand(0)
    raise StopIteration()

  def summarize(self):
    yield "$imm"
    raise StopIteration()


REG_KIND_SUMMARY = {
  Register.KIND_GENERAL_PURPOSE:    "reg",
  Register.KIND_X87_FPU_STACK_REG:  "float",
  Register.KIND_MMX:                "float",
  Register.KIND_SIMD:               "simd",
  Register.KIND_SEGMENT:            "seg",
  Register.KIND_DEBUG:              "debug",
  Register.KIND_CONTROL:            "control",
  Register.KIND_TEST:               "test",
  Register.KIND_INSTRUCTION:        "ip",
  Register.KIND_STACK:              "sp",
}


REG_SIZE_SUMMARY = {
  0:    "",
  1:    "8",
  2:    "16",
  4:    "32",
  8:    "64",
  16:   "128",
  32:   "256",
}


# Summarize a register using a string.
def summarize_register(reg):
  if not reg:
    return ""
  global REG_KIND_SUMMARY, REG_SIZE_SUMMARY
  return "%" + REG_KIND_SUMMARY[reg.kind] + REG_SIZE_SUMMARY[reg.size]


class UnifiedRegisterOperand(UnifiedOperand):
  """Represents a generator for unified register operands."""

  __slots__ = ('pairs',)

  def __init__(self, op):
    self.pairs = set([(op.segment_register, op.base_register)])

  def unify(self, op):
    self.pairs.add((op.segment_register, op.base_register))

  def generate(self):
    for seg, base in self.pairs:
      yield RegisterOperand(seg, base)
    raise StopIteration()

  def summarize(self):
    regs = set()
    for seg, base in self.pairs:
      summary = ""
      if seg:
        summary = summarize_register(seg) + ":"
      if base:
        summary += summarize_register(base)
      regs.add(summary)
    return iter(regs)


class UnifiedMemoryOperand(UnifiedOperand):
  """Represents a generator for unified memory operands."""

  def __init__(self, _):
    pass

  def unify(self, _):
    pass

  def generate(self):
    raise StopIteration()

  def summarize(self):
    yield "mem"
    raise StopIteration()


class UnifiedAddressOperand(UnifiedOperand):
  """Represents a generator for unified memory operands."""

  __slots__ = ('forms',)

  class UnifiedBaseDisp(object):

    __slots__ = (
      'base_register',
      'index_register',
    )

    def __init__(self):
      self.base_register = set()
      self.index_register = set()

    def unify(self, op):
      self.base_register.add(op.base_register)
      self.index_register.add(op.index_register)

  def __init__(self, op):
    self.forms = collections.defaultdict(self.UnifiedBaseDisp)
    self.unify(op)

  # Unify an address operand into the unified operand.
  #
  # Note: This ignores displacements.
  def unify(self, op):
    form = self.forms[
        op.segment_register,
        None is not op.displacement,
        not not op.base_register,
        not not op.index_register,
        None is not op.scale]
    form.unify(op)

  def generate(self):
    for (seg, has_d, _, _, has_s), op in self.forms.items():
      for base in op.base_register:
        for index in op.index_register:
          if not base and not index:
            continue
          gen_op = AddressOperand()
          gen_op.displacement = has_d and 0 or None
          gen_op.segment_register = seg
          gen_op.base_register = base
          gen_op.index_register = index
          gen_op.scale = has_s and 1 or None
          yield gen_op
    raise StopIteration()

  def summarize(self):
    summaries = set()
    for (seg, _, _,_, _), op in self.forms.items():
      for base in op.base_register:
        for index in op.index_register:
          if not base and not index:
            continue

          summary = ""
          if seg:
            summary += summarize_register(seg) + ":"
          summary += "("
          if base:
            summary += summarize_register(base)
          if index:
            summary += ","
            summary += summarize_register(index)
            summary += ",1"
          summary += ")"
          summaries.add(summary)
    return iter(summaries)


class UnifiedInstruction(object):
  """Represents a generator for unified instructions."""

  __slots__ = (
    'mnemonic',
    'operands',
  )

  UNIFIER = {
    ImmediateOperand: UnifiedImmediateOperand,
    RegisterOperand:  UnifiedRegisterOperand,
    MemoryOperand:    UnifiedMemoryOperand,
    AddressOperand:   UnifiedAddressOperand,
  }

  # Generate a 'key' for a group of instructions.
  def _key(self, ins):
    key = [ins.prefixes, ins.suffixes]
    key.extend(op.KIND for op in ins.operands)
    return tuple(key)

  # Initialise this instruction from an asmparser.Instruction instance.
  def __init__(self):
    self.operands = collections.defaultdict(list)

  # Unify one instruction into a 
  def unify(self, ins):
    self.mnemonic = ins.mnemonic
    ops = self.operands[self._key(ins)]

    # Initialise with the right constructors.
    if not ops:
      ops.extend(
          self.UNIFIER[op.__class__](op) for op in ins.operands)
    else:
      for uop, op in zip(ops, ins.operands):
        uop.unify(op)

  # Generate different instantiations of this unified instruction.
  def generate(self):
    for key, uops in self.operands.items():
      op_groups = itertools.product(
          *map(lambda uop: list(uop.generate()), uops))
      for op_group in op_groups:
        ins = Instruction()
        ins.mnemonic = self.mnemonic
        ins.prefixes = key[0]
        ins.suffixes = key[1]
        ins.operands = op_group
        yield ins
    raise StopIteration()

  # Generate summary strings of the parsed instructions grouped
  # under this unified instruction.
  def summarize(self):
    for key, uops in self.operands.items():
      summary = "%s %s%s" % (
          " ".join(key[0]), # prefixes
          self.mnemonic.lower(), # mnemonic
          "".join(s.lower() for s in key[1])) # suffixes
      uop_groups = itertools.product(
          *map(lambda uop: list(uop.summarize()), uops))

      for uop_group in uop_groups:
        yield ("%s %s" % (summary, ", ".join(uop_group))).strip()
    raise StopIteration()
       

# Test program to summarize instruction forms.
if __name__ == "__main__":

  # Set of all unified instructions, grouped by opcode.
  INSTRUCTIONS = collections.defaultdict(UnifiedInstruction)

  # Unify a parsed instruction into a UnifiedInstruction.
  def unify_instruction(ins):
    global INSTRUCTIONS
    INSTRUCTIONS[ins.mnemonic_disambiguated].unify(ins)

  import sys
  parser = ASMParser(unify_instruction)

  with open(sys.argv[1], "r") as lines:
    parser.parse(lines)

  for mnemonic, uins in INSTRUCTIONS.items():
    print "###", mnemonic
    print "```asm"
    for summarized_in in uins.summarize():
      print summarized_in
    print "```"
    print
