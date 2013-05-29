"""Parse AT&T-style x86 assembly.

The purpose of this parser is to get assembly instructions from large
binaries (such as kernel modules, vmlinux, etc.) and convert them into
a structural format that can be later manipulated.

Author:       Peter Goodman (peter.goodman@gmail.com)
Copyright:    Copyright 2012-2013 Peter Goodman, all rights reserved.
"""


import re
import collections


class Register(object):
  """Represents a system register.

  Note: Registers are considered a smaller unit than operands."""

  __slots__ = (
    'mnemonic',
    'size',
    'parent',
    'children',
    'kind',
  )

  # Different classes of registers.
  KIND_GENERAL_PURPOSE    = 1
  KIND_X87_FPU_STACK_REG  = 2
  KIND_MMX                = 3
  KIND_SIMD               = 4
  KIND_SEGMENT            = 5
  KIND_DEBUG              = 6
  KIND_CONTROL            = 7
  KIND_TEST               = 8
  KIND_INSTRUCTION        = 9
  KIND_STACK              = 10

  # Cache of register objects.
  CACHE = {}

  def __init__(self, mnemonic, size, kind, parent=None):
    global REGISTER_CACHE
    self.mnemonic = mnemonic
    self.size = size
    self.kind = kind
    self.parent = parent and self.CACHE[parent] or None
    self.children = set([self])
    
    parent = self.parent
    while parent:
      parent.children.add(self)
      parent = parent.parent

    self.CACHE[mnemonic] = self

  @classmethod
  def parse(cls, mnemonic):
    if mnemonic.startswith("%"):
      mnemonic = mnemonic[1:]
    return cls.CACHE[mnemonic.upper()]

  def __str__(self):
    return "%%%s" % self.mnemonic


Register("RIP",   8,  Register.KIND_INSTRUCTION)

Register("RAX",   8,  Register.KIND_GENERAL_PURPOSE)
Register("RCX",   8,  Register.KIND_GENERAL_PURPOSE)
Register("RDX",   8,  Register.KIND_GENERAL_PURPOSE)
Register("RBX",   8,  Register.KIND_GENERAL_PURPOSE)
Register("RSP",   8,  Register.KIND_STACK)
Register("RBP",   8,  Register.KIND_GENERAL_PURPOSE)
Register("RSI",   8,  Register.KIND_GENERAL_PURPOSE)
Register("RDI",   8,  Register.KIND_GENERAL_PURPOSE)
Register("R8",    8,  Register.KIND_GENERAL_PURPOSE)
Register("R9",    8,  Register.KIND_GENERAL_PURPOSE)
Register("R10",   8,  Register.KIND_GENERAL_PURPOSE)
Register("R11",   8,  Register.KIND_GENERAL_PURPOSE)
Register("R12",   8,  Register.KIND_GENERAL_PURPOSE)
Register("R13",   8,  Register.KIND_GENERAL_PURPOSE)
Register("R14",   8,  Register.KIND_GENERAL_PURPOSE)
Register("R15",   8,  Register.KIND_GENERAL_PURPOSE)

Register("EAX",   4,  Register.KIND_GENERAL_PURPOSE,   parent="RAX")
Register("ECX",   4,  Register.KIND_GENERAL_PURPOSE,   parent="RCX")
Register("EDX",   4,  Register.KIND_GENERAL_PURPOSE,   parent="RDX")
Register("EBX",   4,  Register.KIND_GENERAL_PURPOSE,   parent="RBX")
Register("ESP",   4,  Register.KIND_STACK,             parent="RSP")
Register("EBP",   4,  Register.KIND_GENERAL_PURPOSE,   parent="RBP")
Register("ESI",   4,  Register.KIND_GENERAL_PURPOSE,   parent="RSI")
Register("EDI",   4,  Register.KIND_GENERAL_PURPOSE,   parent="RDI")
Register("R8D",   4,  Register.KIND_GENERAL_PURPOSE,   parent="R8")
Register("R9D",   4,  Register.KIND_GENERAL_PURPOSE,   parent="R9")
Register("R10D",  4,  Register.KIND_GENERAL_PURPOSE,   parent="R10")
Register("R11D",  4,  Register.KIND_GENERAL_PURPOSE,   parent="R11")
Register("R12D",  4,  Register.KIND_GENERAL_PURPOSE,   parent="R12")
Register("R13D",  4,  Register.KIND_GENERAL_PURPOSE,   parent="R13")
Register("R14D",  4,  Register.KIND_GENERAL_PURPOSE,   parent="R14")
Register("R15D",  4,  Register.KIND_GENERAL_PURPOSE,   parent="R15")

Register("AX",    2,  Register.KIND_GENERAL_PURPOSE,   parent="EAX")
Register("CX",    2,  Register.KIND_GENERAL_PURPOSE,   parent="ECX")
Register("DX",    2,  Register.KIND_GENERAL_PURPOSE,   parent="EDX")
Register("BX",    2,  Register.KIND_GENERAL_PURPOSE,   parent="EBX")
Register("SP",    2,  Register.KIND_STACK,             parent="ESP")
Register("BP",    2,  Register.KIND_GENERAL_PURPOSE,   parent="EBP")
Register("SI",    2,  Register.KIND_GENERAL_PURPOSE,   parent="ESI")
Register("DI",    2,  Register.KIND_GENERAL_PURPOSE,   parent="EDI")
Register("R8W",   2,  Register.KIND_GENERAL_PURPOSE,   parent="R8D")
Register("R9W",   2,  Register.KIND_GENERAL_PURPOSE,   parent="R9D")
Register("R10W",  2,  Register.KIND_GENERAL_PURPOSE,   parent="R10D")
Register("R11W",  2,  Register.KIND_GENERAL_PURPOSE,   parent="R11D")
Register("R12W",  2,  Register.KIND_GENERAL_PURPOSE,   parent="R12D")
Register("R13W",  2,  Register.KIND_GENERAL_PURPOSE,   parent="R13D")
Register("R14W",  2,  Register.KIND_GENERAL_PURPOSE,   parent="R14D")
Register("R15W",  2,  Register.KIND_GENERAL_PURPOSE,   parent="R15D")

Register("AL",    1,  Register.KIND_GENERAL_PURPOSE,   parent="AX")
Register("CL",    1,  Register.KIND_GENERAL_PURPOSE,   parent="CX")
Register("DL",    1,  Register.KIND_GENERAL_PURPOSE,   parent="DX")
Register("BL",    1,  Register.KIND_GENERAL_PURPOSE,   parent="BX")
Register("AH",    1,  Register.KIND_GENERAL_PURPOSE,   parent="AX")
Register("CH",    1,  Register.KIND_GENERAL_PURPOSE,   parent="CX")
Register("DH",    1,  Register.KIND_GENERAL_PURPOSE,   parent="DX")
Register("BH",    1,  Register.KIND_GENERAL_PURPOSE,   parent="BX")
Register("R8B",   1,  Register.KIND_GENERAL_PURPOSE,   parent="R8W")
Register("R9B",   1,  Register.KIND_GENERAL_PURPOSE,   parent="R9W")
Register("R10B",  1,  Register.KIND_GENERAL_PURPOSE,   parent="R10W")
Register("R11B",  1,  Register.KIND_GENERAL_PURPOSE,   parent="R11W")
Register("R12B",  1,  Register.KIND_GENERAL_PURPOSE,   parent="R12W")
Register("R13B",  1,  Register.KIND_GENERAL_PURPOSE,   parent="R13W")
Register("R14B",  1,  Register.KIND_GENERAL_PURPOSE,   parent="R14W")
Register("R15B",  1,  Register.KIND_GENERAL_PURPOSE,   parent="R15W")
Register("SPL",   1,  Register.KIND_STACK,             parent="SP")
Register("BPL",   1,  Register.KIND_GENERAL_PURPOSE,   parent="BP")
Register("SIL",   1,  Register.KIND_GENERAL_PURPOSE,   parent="SI")
Register("DIL",   1,  Register.KIND_GENERAL_PURPOSE,   parent="DI")

Register("MM0",   8,  Register.KIND_MMX)
Register("MM1",   8,  Register.KIND_MMX)
Register("MM2",   8,  Register.KIND_MMX)
Register("MM3",   8,  Register.KIND_MMX)
Register("MM4",   8,  Register.KIND_MMX)
Register("MM5",   8,  Register.KIND_MMX)
Register("MM6",   8,  Register.KIND_MMX)
Register("MM7",   8,  Register.KIND_MMX)

Register("XMM0",  16, Register.KIND_SIMD)
Register("XMM1",  16, Register.KIND_SIMD)
Register("XMM2",  16, Register.KIND_SIMD)
Register("XMM3",  16, Register.KIND_SIMD)
Register("XMM4",  16, Register.KIND_SIMD)
Register("XMM5",  16, Register.KIND_SIMD)
Register("XMM6",  16, Register.KIND_SIMD)
Register("XMM7",  16, Register.KIND_SIMD)
Register("XMM8",  16, Register.KIND_SIMD)
Register("XMM9",  16, Register.KIND_SIMD)
Register("XMM10", 16, Register.KIND_SIMD)
Register("XMM11", 16, Register.KIND_SIMD)
Register("XMM12", 16, Register.KIND_SIMD)
Register("XMM13", 16, Register.KIND_SIMD)
Register("XMM14", 16, Register.KIND_SIMD)
Register("XMM15", 16, Register.KIND_SIMD)

Register("ST",    8,  Register.KIND_X87_FPU_STACK_REG)
Register("ST1",   8,  Register.KIND_X87_FPU_STACK_REG)
Register("ST2",   8,  Register.KIND_X87_FPU_STACK_REG)
Register("ST3",   8,  Register.KIND_X87_FPU_STACK_REG)
Register("ST4",   8,  Register.KIND_X87_FPU_STACK_REG)
Register("ST5",   8,  Register.KIND_X87_FPU_STACK_REG)
Register("ST6",   8,  Register.KIND_X87_FPU_STACK_REG)
Register("ST7",   8,  Register.KIND_X87_FPU_STACK_REG)

Register("ES",    8,  Register.KIND_SEGMENT)
Register("CS",    8,  Register.KIND_SEGMENT)
Register("SS",    8,  Register.KIND_SEGMENT)
Register("DS",    8,  Register.KIND_SEGMENT)
Register("FS",    8,  Register.KIND_SEGMENT)
Register("GS",    8,  Register.KIND_SEGMENT)

Register("DR0",   8,  Register.KIND_DEBUG)
Register("DR1",   8,  Register.KIND_DEBUG)
Register("DR2",   8,  Register.KIND_DEBUG)
Register("DR3",   8,  Register.KIND_DEBUG)
Register("DR4",   8,  Register.KIND_DEBUG)
Register("DR5",   8,  Register.KIND_DEBUG)
Register("DR6",   8,  Register.KIND_DEBUG)
Register("DR7",   8,  Register.KIND_DEBUG)
Register("DR8",   8,  Register.KIND_DEBUG)
Register("DR9",   8,  Register.KIND_DEBUG)
Register("DR10",  8,  Register.KIND_DEBUG)
Register("DR11",  8,  Register.KIND_DEBUG)
Register("DR12",  8,  Register.KIND_DEBUG)
Register("DR13",  8,  Register.KIND_DEBUG)
Register("DR14",  8,  Register.KIND_DEBUG)
Register("DR15",  8,  Register.KIND_DEBUG)

Register("CR0",   8,  Register.KIND_CONTROL)
Register("CR1",   8,  Register.KIND_CONTROL)
Register("CR2",   8,  Register.KIND_CONTROL)
Register("CR3",   8,  Register.KIND_CONTROL)
Register("CR4",   8,  Register.KIND_CONTROL)
Register("CR5",   8,  Register.KIND_CONTROL)
Register("CR6",   8,  Register.KIND_CONTROL)
Register("CR7",   8,  Register.KIND_CONTROL)
Register("CR8",   8,  Register.KIND_CONTROL)
Register("CR9",   8,  Register.KIND_CONTROL)
Register("CR10",  8,  Register.KIND_CONTROL)
Register("CR11",  8,  Register.KIND_CONTROL)
Register("CR12",  8,  Register.KIND_CONTROL)
Register("CR13",  8,  Register.KIND_CONTROL)
Register("CR14",  8,  Register.KIND_CONTROL)
Register("CR15",  8,  Register.KIND_CONTROL)

Register("YMM0",  32, Register.KIND_SIMD)
Register("YMM1",  32, Register.KIND_SIMD)
Register("YMM2",  32, Register.KIND_SIMD)
Register("YMM3",  32, Register.KIND_SIMD)
Register("YMM4",  32, Register.KIND_SIMD)
Register("YMM5",  32, Register.KIND_SIMD)
Register("YMM6",  32, Register.KIND_SIMD)
Register("YMM7",  32, Register.KIND_SIMD)
Register("YMM8",  32, Register.KIND_SIMD)
Register("YMM9",  32, Register.KIND_SIMD)
Register("YMM10", 32, Register.KIND_SIMD)
Register("YMM11", 32, Register.KIND_SIMD)
Register("YMM12", 32, Register.KIND_SIMD)
Register("YMM13", 32, Register.KIND_SIMD)
Register("YMM14", 32, Register.KIND_SIMD)
Register("YMM15", 32, Register.KIND_SIMD)



class Operand(object):
  """Represents a parsed operand and common utilities that operate
  on operands in general."""

  # Operand sizes.
  SIZE_1              = 1
  SIZE_2              = 2
  SIZE_4              = 4
  SIZE_8              = 8
  SIZE_16             = 16
  SIZE_32             = 32
  
  @classmethod
  def parse(cls, instruction_mnemonic, op_str):
    op_str = op_str.strip(" \t\n") \
                   .replace(" ", "")

    # Operand is the target of a (non-RET) control-flow instruction.
    if instruction_mnemonic.startswith("J") \
    or "CALL" in instruction_mnemonic \
    or "LOOP" in instruction_mnemonic \
    or "JMP" in instruction_mnemonic:
      if op_str.startswith("*"):
        if "(" in op_str:
          return AddressOperand.parse(op_str[1:])
        elif "%" in op_str:
          return RegisterOperand.parse(op_str)
        else:
          return MemoryOperand.parse(op_str[1:])
      else:
        return AddressOperand.parse("0x" + op_str + "(%rip)")
    
    # Immediate integer operand.
    elif op_str.startswith("$"):
      return ImmediateOperand.parse(op_str)

    # Likely address operand of a base/displacement form.
    elif "(" in op_str:
      return AddressOperand.parse(op_str)

    # Register operand, or access of a displacement of a segment
    # register.
    elif "%" in op_str:
      if ":" in op_str and op_str[-1] in "0123456789abcdefABCDEF":
        return AddressOperand.parse(op_str)
      else:
        return RegisterOperand.parse(op_str)

    # Absolute memory address operand.
    elif op_str:
      return MemoryOperand.parse(op_str)

    # This is unusual.
    else:
      assert False

  # Returns True iff the current operand is an effective
  # address.
  def is_effective_address(self):
    if not isinstance(self, AddressOperand):
      return False

    if self.base_register:
      return Register.KIND_GENERAL_PURPOSE == self.base_register.kind
    elif self.index_register:
      return True
    return False


class ImmediateOperand(Operand):
  """Represents an immediate integer operand."""

  __slots__ = ('value',)

  KIND = 1

  def __init__(self, value):
    self.value = value

  @classmethod
  def parse(cls, op_str):
    if op_str.startswith("$"):
      op_str = op_str[1:]
    return ImmediateOperand(int(op_str, base=0))

  # Re-serialise this operand as a string in a way that can be
  # parsed again.
  def __str__(self):
    return "$%s0x%x" % (
      self.value < 0 and "-" or "", abs(self.value))


class RegisterOperand(Operand):
  """Represents a register operand."""

  __slots__ = (
    'segment_register',
    'base_register',
  )

  CACHE = {}
  KIND = 2

  def __init__(self, seg, base):
    self.segment_register = seg
    self.base_register = base

  @classmethod
  def parse(cls, op_str):
    op_str = op_str.replace("*", "")

    if op_str in cls.CACHE:
      return cls.CACHE[op_str]

    parts = op_str.split(":")
    base_register = Register.parse(parts[-1])
    segment_register = None
    parts = parts[:-1]
    if parts:
      segment_register = Register.parse(parts[0])

    op = RegisterOperand(segment_register, base_register)
    cls.CACHE[op_str] = op
    return op

  # Re-serialise this operand as a string in a way that can be
  # parsed again.
  def __str__(self):
    ret = ""
    if self.segment_register:
      ret = str(self.segment_register)
      if self.base_register:
        ret += ":"
    if self.base_register:
      ret += str(self.base_register)
    return ret


class MemoryOperand(Operand):
  """Represents an address operand."""

  CACHE = {}
  KIND = 3

  __slots__ = ('address',)

  def __init__(self, addr):
    self.address = addr

  # Parse a memory operand.
  #
  # Returns:
  #   A MemoryOperand instance or an AddressOperand instance.
  @classmethod
  def parse(cls, op_str):
    addr = int(op_str, base=0)
    if addr in cls.CACHE:
      return cls.CACHE[addr]

    op = MemoryOperand(addr)
    cls.CACHE[addr] = op
    return op

  # Re-serialise this operand as a string in a way that can be
  # parsed again.
  def __str__(self):
    return "%s0x%x" % (
      self.address < 0 and "-" or "", abs(self.address))


class AddressOperand(Operand):
  """Represents an effective address operand."""

  __slots__ = (
    'base_register',
    'index_register',
    'segment_register',
    'displacement',
    'scale',
  )

  KIND = 4

  def __init__(self):
    self.base_register = None
    self.index_register = None
    self.segment_register = None
    self.displacement = None
    self.scale = None

  @classmethod
  def parse(cls, op_str):
    orig_op_str = op_str
    op = AddressOperand()
    parts = op_str.split(":")
    if 2 == len(parts):
      op.segment_register = Register.parse(parts[0])
      op_str = parts[1]

    op_str = op_str.strip("()") \
                   .replace("(", ",") \
                   .replace(")", "")
    parts = op_str.split(",")

    # Parse the displacement.
    if parts[0][0] in "-0123456789abcdefABCDEF":
      op.displacement = int(parts[0], base=0)
      parts = parts[1:]

    # Parse or skip the base register.
    if parts:
      if parts[0]:
        op.base_register = Register.parse(parts[0])
      parts = parts[1:]

    # Parse the scale.
    if parts and parts[-1][-1] in "1248":
      op.scale = int(parts[-1], base=0)
      parts = parts[:-1]

    # Parse or skip the index register.
    if parts and parts[0]:
      op.index_register = Register.parse(parts[0])

    return op

  # Re-serialise this operand as a string in a way that can be
  # parsed again.
  def __str__(self):
    ret = ""
    if self.segment_register:
      ret += "%s:" % str(self.segment_register)
    if self.displacement is not None:
      ret += "%s0x%x" % (
        self.displacement < 0 and "-" or "", abs(self.displacement))
    ret += "("
    if self.base_register:
      ret += str(self.base_register)
    if self.index_register:
      ret += ",%s,%d" % (str(self.index_register), self.scale)
    ret += ")"
    return ret


class Instruction(object):
  """Represents an instruction."""

  __slots__ = (
    'mnemonic',
    'mnemonic_disambiguated', # Close to Intel syntax.
    'prefixes',
    'suffixes',
    'operands',
  )

  # Initialise an Instruction instance with default values.
  #
  # Note: Instruction objects should not be manually initialised.
  def __init__(self):
    self.mnemonic = None
    self.mnemonic_disambiguated = None
    self.prefixes = None
    self.suffixes = None
    self.operands = []

  # Re-serialise this instruction as a string in a way that can be
  # parsed again.
  def __str__(self):
    ret = ""
    if self.prefixes:
      ret = " ".join(self.prefixes)
    ret += " %s" % self.mnemonic
    if self.suffixes:
      ret += self.suffixes
    ret += " "
    if self.mnemonic in ("CALL", "JMP", "LCALL", "LJMP") \
    and self.operands:
      if isinstance(self.operands[0], RegisterOperand):
        ret += "*"
      elif isinstance(self.operands[0], AddressOperand) \
      and self.operands[0].is_effective_address():
        ret += "*"
    ret += ",".join(str(op) for op in self.operands)
    return ret.lower().strip()

  # Returns True iff this instruction is a control-flow instruction.
  def is_cti(self):
    if self.mnemonic.startswith("J"):
      return True
    return self.mnemonic in ("CALL", "LCALL", "RET", "LRET", "LJMP")

  # Returns True iff this instruction operates on memory.
  def accesses_memory(self):
    if self.mnemonic in ("LEA", "NOP"):
      return False
    for op in self.operands:
      if op.is_effective_address():
        return True
    return False

  # Set of instructions that implicitly change the stack pointer
  # and/or operate on stack memory.
  STACK_INSTRUCTIONS = set([
    "CALL", "LCALL",
    "RET", "LRET",
    "IRET",
    "PUSH", "POP",
    "PUSHA", "POPA",
    "PUSHF", "POPF",
  ])

  # Returns true iff an instruction operates on the stack. This
  # includes reading/writing the stack pointer, changing the stack
  # pointer, or reading/writing stack memory using the stack pointer.
  def operates_on_stack(self):
    if self.mnemonic in self.STACK_INSTRUCTIONS:
      return True
    for op in self.operands:
      if isinstance(op, RegisterOperand) \
      or isinstance(op, AddressOperand):
        if op.base_register \
        and Register.KIND_STACK == op.base_register.kind:
          return True
      if isinstance(op, AddressOperand):
        if op.index_register \
        and Register.KIND_STACK == op.index_register.kind:
          return True
    return False

  # Set of FPU register kinds.
  FPU_REGISTER_KINDS = (
    Register.KIND_X87_FPU_STACK_REG,
    Register.KIND_MMX,
    Register.KIND_SIMD,
  )

  # Returns True if this instruction uses floating point registers,
  # mmx registers, or xmm registers.
  def uses_fpu(self):
    if self.mnemonic.startswith("F"):
      return True
    for op in self.operands:
      if isinstance(op, RegisterOperand) \
      or isinstance(op, AddressOperand):
        if op.base_register \
        and op.base_register.kind in self.FPU_REGISTER_KINDS:
          return True
      if isinstance(op, AddressOperand):
        if op.index_register \
        and op.index_register.kind in self.FPU_REGISTER_KINDS:
          return True
    return False


class ASMParser(object):
  """Parses an AT&T-style x86-64 instruction as output by the `objdump`
  command-line program.

  This expects `objdump` to be invoked with the following command-line
  options:
      --disassemble
      --no-show-raw-insn
      --disassembler-options=no-aliases,att-mnemonic,addr64,suffix
      --section=.text
  """

  ADJACENT_SPACES = re.compile(r"([ \t\r\n]+)")


  PREFIXES = set([
    "REP",
    "REPE",
    "REPZ",
    "REPNE",
    "REPNZ",
    "LOCK",
    "DATA16",
    "DATA32",
    "ADDR16",
    "ADDR32",
    "REX",
    "REX.W",
    "REX.R",
    "REX.X",
    "REX.B",
    "CS",
    "SS",
    "DS",
    "ES",
    "FS",
    "GS",
  ])

  # GAS/AT&T instruction size suffixes.
  SIZE_SUFFIX = "BWLQSDT"

  # Instructions that look like that have a size suffix but don't.
  #
  # Note: This is an incomplete list.
  NO_SUFFIX = {
    "SWAPGS", "SYSEXIT", "FWAIT", "MOVABS", 
    "JB", "JNB", "JL", "JNL", "JS", "JNS",
    "CMOVB", "CMOVNB", "CMOVL", "CMOVNL", "CMOVS", "CMOVNS", 

    "SETB", "SETNB", "SETL", "SETNL", "SETS", "SETNS",
    "NOT", "SHL", "SAL", "ADD", "MUL", "IMUL", "BTS", "AND", "TEST", "BT",
    "MOVZ", "MOVSS",
    "INS", "MOVS", "OUTS", "LODS", "STOS", "CMPS", "SCAS", 
    "LIDT", "SIDT", "LGDT", "SGDT", "HLT", "RDRAND",
    "MWAIT",

    "STD", "CLD", "SBB",

    "FCOS", "FMUL", "FXTRACT",
  }

  MULTI_SIZED_INSTRUCTIONS = re.compile(r"^(MOVS|MOVZ|SHR|SHL)")

  # Regular expressions for matching split points in operand strings.
  COMMA_BEFORE_BASE_DISP = re.compile(
      r",(([%][a-zA-Z]+[:])*[0-9a-fA-FxX-]*[(])")
  COMMA_AFTER_BASE_DISP = re.compile(r"[)],")
  COMMA_AFTER_IMMED_OR_ADDR = re.compile(
      r"^(([%][a-zA-Z]+[:])?[$-]?[0-9a-fA-FxX]+),")
  COMMA_BEFORE_REG = re.compile(r",([%][a-zA-Z0-9]+)$")
  COMMA_AFTER_REG = re.compile(r"^([%][a-zA-Z0-9]+),")

  # Translate debug registers to a canonical naming scheme.
  DEBUG_REG = re.compile(r"[%]db")

  # Translate FPU stack registers to a canonical naming scheme.
  ST_REG = re.compile(r"[%]st\(([0-9]+)\)")

  # A "useless" encoding where a scale is used with a known zero index.
  ZERO_INDEX_REG = re.compile(r",[%](riz|RIZ),[01248]\)")

  # Initialise an ASM Parser.
  #
  # Args:
  #   instruction_visitor: A function that is called for each parsed
  #                        instruction.
  def __init__(self, instruction_visitor):
    self.instruction_visitor = instruction_visitor

  # Parse an iterable of lines, where some lines potentially contain
  # instructions in the AT&T syntax.
  #
  # Args:
  #   lines:          An iterable of strings, where each string represents
  #                   a line with no more than one instruction.
  def parse(self, lines):
    i = 1
    for line in lines:
      self._parse_line(i, line)
      i += 1

  # Parse a single line of output from `objdump` and try to extract a
  # single instruction.
  #
  # Args:
  #   line:           A string containing zero or one instructions.
  def _parse_line(self, line_num, line):
    orig_line = line

    if "<internal disassembler error>" in line:
      return

    line = line + " "
    line = line[:line.find("<")]
    line = line + " "
    line = line[:line.find("#")]
    line = line.strip(" \t\r\n;")

    # Try to root out disassembly errors or function names.
    if not line \
    or line.endswith(":") \
    or ":" not in line \
    or line.startswith("/") \
    or line.startswith(".") \
    or "file format" in line \
    or "(bad)" in line \
    or ",pt" in line \
    or "%?" in line:
      return  
    
    # Drop the leading binary offset, and deal with spaces.
    line = ":".join(line.split(":")[1:])
    line = line.lstrip(" \r\n\t")
    line = self.ADJACENT_SPACES.sub(" ", line)

    # Split the instruction into its constituent parts.
    ins = Instruction()
    line = line.replace(", ", ",")
    parts = line.split(" ")

    # Extract the instruction prefixes.
    i = 0
    prefixes = []
    for part in map(str.upper, parts):
      if "REX." in part or part in self.PREFIXES:
        prefixes.append(part)
        i += 1
      else:
        break

    # Prefixes or empty.
    if i >= len(parts):
      return

    mnemonic = parts[i].upper()
    op_strs = "".join(parts[i+1:]).lower()

    # Extract the instruction suffixes.
    suffixes = ""
    if "." in mnemonic:
      mnemonic, suffixes = mnemonic.split(".")
      suffixes = map(lambda s: ".%s" % s, suffixes)

    # Get the mnemonic and the operand size suffix.
    #
    # Note: Some operands have two size suffixes; however, we won't
    #       deal with those just yet.
    while mnemonic not in self.NO_SUFFIX \
    and mnemonic[-1] in self.SIZE_SUFFIX:
      suffixes = "%s%s" % (mnemonic[-1], suffixes)
      mnemonic = mnemonic[:-1]
      if not self.MULTI_SIZED_INSTRUCTIONS.match(mnemonic):
        break

    ins.mnemonic = mnemonic
    ins.prefixes = tuple(prefixes)
    if suffixes:
      ins.suffixes = suffixes

    ins.mnemonic_disambiguated = ins.mnemonic
    if ins.mnemonic == "MOVS" and suffixes and 2 <= len(suffixes):
      ins.mnemonic_disambiguated = "MOVSX"
    elif ins.mnemonic == "MOVZ":
      ins.mnemonic_disambiguated = "MOVZX"

    # Parse the operands. First they need to be reasonably split up.
    op_strs = self.ST_REG.sub(r"%st\1", op_strs)
    op_strs = op_strs.replace("st0", "st")
    op_strs = self.DEBUG_REG.sub("%dr", op_strs)
    op_strs = self.ZERO_INDEX_REG.sub(")", op_strs)
    op_strs = self.COMMA_BEFORE_BASE_DISP.sub(r"|\1", op_strs)
    op_strs = self.COMMA_AFTER_BASE_DISP.sub(r")|", op_strs)
    op_strs = self.COMMA_AFTER_IMMED_OR_ADDR.sub(r"\1|", op_strs)
    op_strs = self.COMMA_BEFORE_REG.sub(r"|\1", op_strs)
    op_strs = self.COMMA_AFTER_REG.sub(r"\1|", op_strs)

    if op_strs:
      ins.operands = tuple(map(
          lambda s: Operand.parse(mnemonic, s),
          op_strs.split("|")))

    # Visit the parsed instruction.
    self.instruction_visitor(ins)

if __name__ == "__main__":
  import sys

  def print_instruction(ins):
    print str(ins)

  parser = ASMParser(print_instruction)
  with open(sys.argv[1], "r") as lines:
    parser.parse(lines)