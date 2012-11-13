
import re

code = open('granary/gen/instruction.cc', 'w')
header = open('granary/gen/instruction.h', 'w')

def C(*args):
  global code
  code.write("%s\n" % "".join(map(str, args)))

def H(*args):
  global header
  header.write("%s\n" % "".join(map(str, args)))

CREATE_MACRO = re.compile(r"#define\s+INSTR_CREATE_([a-zA-Z0-9_]+)\((.*?)\)")
AFTER_CREATE_MACRO = re.compile(r"(.*?)\)(.*)")

def format_line(line):
  line = line.strip("\r\n\t ")
  line = line.replace("instr_", "dynamorio::instr_")
  line = line.replace("OP_", "dynamorio::OP_")
  line = line.replace("opnd_", "dynamorio::opnd_")
  line = line.replace("DR_REG_X", "DR_REG_R")
  line = line.replace("DR_", "dynamorio::DR_")
  return line

def emit_function(lines, i, instr, args):
  
  # collect the lines needed for the function
  sub_lines = [format_line(AFTER_CREATE_MACRO.match(lines[i]).group(2)), ]
  while i < len(lines):
    line = lines[i].rstrip("\r\n\t ")
    if line and line[-1] == "\\":
      sub_lines.append(format_line(lines[i + 1]))
      i = i + 1
      continue
    break

  def typed_arg(p):
    i, a = p
    if "op" == a:
      return "int " + a
    else:
      return "dynamorio::opnd_t " + a

  sub_lines.append(";")

  arg_list = "void"
  if 1 < len(args):
    arg_list = ", ".join(map(typed_arg, enumerate(args[1:])))

  # emit the new function
  H("    instruction ", instr, "_(", arg_list, ");")
  C("instruction ", instr, "_(", arg_list, ") {")
  C("    instruction in__;")
  if 1 <= len(args):
    C("    dynamorio::dcontext_t dc__ = {")
    C("        true, /* x86_mode */")
    C("        0, /* private code */")
    C("        &in__ /* allocated_instr */")
    C("    };")
    C("    dynamorio::dcontext_t *", args[0], " = &dc__;")
  C("   ", "\n    ".join(sub_lines))
  C("    return in__;")
  C("}")

  return i + 1

with open("dr/x86/instr_create.h") as lines_:

  C('#include <math.h>')
  C('#include "granary/types/dynamorio.h"')
  C('#include "granary/gen/instruction.h"')
  C('extern "C" {')

  H('#include "granary/instruction.h"')
  H("namespace granary {")

  lines = list(lines_)
  i = 0
  while i < len(lines):

    line = lines[i]
    m = CREATE_MACRO.match(line)

    # didn't match a macro; emit the same line out
    if not m:
      C(line.rstrip("\r\n\t "))
      i = i + 1
      continue

    instr = m.group(1)
    args = map(str.strip, m.group(2).split(","))

    C('}')
    C('namespace granary {')
    i = emit_function(lines, i, instr, args)
    C('}')
    C('extern "C" {')

  C('}')
  C()
  H('}')
  H()
