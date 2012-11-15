
import re

code = open('granary/gen/instruction.cc', 'w')
header = open('granary/gen/instruction.h', 'w')

def C(*args):
  global code
  code.write("%s\n" % "".join(map(str, args)))

def H(*args):
  global header
  header.write("%s\n" % "".join(map(str, args)))

CREATE_MACRO = re.compile(r"#\s*define\s+(INSTR|OPND)_CREATE_([a-zA-Z0-9_]+)\((.*?)\)")
AFTER_CREATE_MACRO = re.compile(r"(.*?)\)(.*)")
MACRO_USE = re.compile(r"INSTR_CREATE_([a-zA-Z0-9_]+)")
MACRO_DEF = re.compile(r"^#\s*define")

def format_line(line):
  return format_line_ns(line.strip("\r\n\t \\"))

def format_line_ns(line):
  line = line.replace("dcontext_t", "dynamorio::dcontext_t")
  line = line.replace("reg_id_t", "dynamorio::reg_id_t")
  line = line.replace("instr_t", "dynamorio::instr_t")
  line = line.replace("OP_", "dynamorio::OP_")
  line = line.replace("opnd_", "dynamorio::opnd_")
  line = line.replace("DR_REG_X", "DR_REG_R")
  line = line.replace("DR_REG_RMM", "DR_REG_XMM")
  line = line.replace("DR_", "dynamorio::DR_")
  line = line.replace("OPSZ", "dynamorio::OPSZ")
  line = line.replace("ptr_int_t", "dynamorio::ptr_int_t")
  line = line.replace("instr_create_", "dynamorio::instr_create_")
  return line

def collect_macro_lines(lines, i):
  # collect the lines needed for the function
  sub_lines = [format_line(AFTER_CREATE_MACRO.match(lines[i]).group(2)), ]
  while i < len(lines):
    line = lines[i].rstrip("\r\n\t ")
    if line and line[-1] == "\\":
      sub_lines.append(format_line(lines[i + 1].strip("\n\r\t \\")))
      i = i + 1
      continue
    break
  sub_lines = filter(None, sub_lines)
  return sub_lines, i + 1

def build_typed_arg_list(typing_func, args):
  arg_list = "void"
  if len(args):
    arg_list = ", ".join(map(typing_func, enumerate(args)))
  return arg_list

def emit_instr_function(lines, i, instr, args):
  emit_to_code = True #"nop" in instr or "mov_st" in instr or "lea" in instr
  sub_lines, i = collect_macro_lines(lines, i)

  if emit_to_code:
    C("#define INSTR_CREATE_", instr, "(", ",".join(args), ") \\")
    C("    ", "\\\n    ".join(sub_lines))

  sub_lines.append(";")

  def typed_arg(p):
    i, a = p
    if "op" == a:
      return "int " + a
    elif "n" == a:
      return "unsigned " + a
    else:
      return "dynamorio::opnd_t " + a

  arg_list = build_typed_arg_list(typed_arg, args[1:])
  
  # emit the new function
  H("    instruction ", instr, "_(", arg_list, ");")
  C("instruction ", instr, "_(", arg_list, ") {")
  C("    instruction in__;")
  if 1 <= len(args):
    C("    dynamorio::dcontext_t dc__ = {")
    C("        false, /* x86_mode */")
    C("        0, /* private code */")
    C("        &(in__.instr) /* allocated_instr */")
    C("    };")
    C("    dynamorio::dcontext_t *", args[0], " = &dc__;")
  C("   ", "\n    ".join(sub_lines))
  C("    return in__;")
  for arg in args[1:]:
    C("    (void) ", arg, ";")
  C("}")

  return i

def emit_opnd_function(lines, i, opnd, args):
  emit_to_code = True
  sub_lines, i = collect_macro_lines(lines, i)
  if emit_to_code:
    C("#define OPND_CREATE_", opnd, "(", ",".join(args), ") \\")
    C("    ", "\\\n    ".join(sub_lines))

  sub_lines.append(";")

  def typed_arg(p):
    i, a = p
    if "reg" in a:
      return "dynamorio::reg_id_t " + a
    elif "addr" in a:
      return "void *" + a
    else:
      return "uint64_t " + a

  arg_list = build_typed_arg_list(typed_arg, args)
  H("    operand ", opnd.lower(), "_(", arg_list, ");")
  C("    operand ", opnd.lower(), "_(", arg_list, ") {")
  C("    return ", "\n    ".join(sub_lines))
  C("    }")

  return i

with open("dr/x86/instr_create.h") as lines_:

  C('#include <math.h>')
  C('#include <limits.h>')
  C('#include <stdint.h>')
  C('#include "granary/types/dynamorio.h"')
  C('#include "granary/instruction.h"')
  C('#ifndef GRANARY')
  C('#  define GRANARY')
  C('#endif')
  C('#ifndef X64')
  C("#   define X64")
  C('#endif')  
  C('#define IF_X64_ELSE(t,f) (t)')
  C('extern "C" {')
  
  H('#ifndef GRANARY')
  H('#  define GRANARY')
  H('#endif')
  H('#ifndef GRANARY_GEN_INSTRUCTION_H_')
  H('#define GRANARY_GEN_INSTRUCTION_H_')
  H("namespace granary {")
  H('    inline operand pc_(app_pc pc) { return dynamorio::opnd_create_pc(pc); }')
  H('    inline operand far_pc_(uint16_t sel, app_pc pc) { return dynamorio::opnd_create_far_pc(sel, pc); }')
  H('    inline operand instr_(instruction *instr) { return dynamorio::opnd_create_instr(&(instr->instr)); }')
  H('    inline operand far_instr_(uint16_t sel, instruction *instr) { return dynamorio::opnd_create_far_instr(sel, &(instr->instr)); }')

  lines = list(lines_)
  i = 0
  while i < len(lines):

    line = lines[i]
    m = CREATE_MACRO.match(line)

    # didn't match a specific macro def; emit the same line out
    if not m:
      C(format_line_ns(line.rstrip("\r\n\t ")))
      i = i + 1
      continue

    emitter = None
    func_name = m.group(2)
    args = map(str.strip, m.group(3).split(","))

    # instruction
    if "INSTR" == m.group(1):
      emitter = emit_instr_function
    else:
      emitter = emit_opnd_function

    C('}')
    C('namespace granary {')
    i = emitter(lines, i, func_name, args)
    C('}')
    C('extern "C" {')

  C('}')
  C()
  H('}')
  H('#endif /* GRANARY_GEN_INSTRUCTION_H_ */')
  H()
