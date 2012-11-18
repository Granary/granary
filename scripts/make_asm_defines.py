
PREFIX_WITH_UNDERSCORE = False
USE_GLOBL = False
USE_TYPE = False
USE_AT_FUNCTION = False
START_FILE = ".text"
FUNC_ALIGNMENT = ".align 16"

with open("scripts/static/asm.S") as lines:
  for line in lines:
    if "foo" in line:
      if "_foo" in line:
        PREFIX_WITH_UNDERSCORE = True
    if "globl" in line:
      USE_GLOBL = True
    if "type" in line:
      USE_TYPE = True
      if "@function" in line:
        USE_AT_FUNCTION = True
    if ".text" in line:
      START_FILE = line.strip("\r\n \t")
    if ".align" in line:
      FUNC_ALIGNMENT = line.strip("\r\n \t")

with open("dr/x86/asm_defines.asm", "w") as f:
  def W(*args):
    f.write("".join(map(str, args)) + "\n")

  # function type
  type_def = ""
  if USE_TYPE:
    type_def = ".type SYMBOL(x), "
    if USE_AT_FUNCTION:
      type_def += "@function"
    else:
      type_def += "%function"
    type_def += "@N@\\\n"

  # alignment
  align_def = FUNC_ALIGNMENT + ' @N@\\\n'

  # export
  global_def = ""
  if USE_GLOBL:
    global_def = ".globl "
  else:
    global_def = ".global "
  global_def += "SYMBOL(x) @N@\\\n"

  W('#define START_FILE ', START_FILE)
  W('#define END_FILE')
  W('#define SYMBOL(x) ', (PREFIX_WITH_UNDERSCORE and "_ ## " or ""), 'x')
  W('#define DECLARE_FUNC(x) \\\n', align_def, global_def, type_def, " ")
  W('#define GLOBAL_LABEL(x) SYMBOL(x)')
  W('#define END_FUNC(x)')
  W('#define HEX(v) 0x ## v')
  W('#define REG_XAX rax')
  W('#define REG_XBX rbx')
  W('#define REG_XCX rcx')
  W('#define REG_XDX rdx')
  W('#define REG_XSI rsi')
  W('#define REG_XDI rdi')
  W('#define REG_XBP rbp')
  W('#define REG_XSP rsp')
  W('#define ARG1 rdi')
  W('#define ARG2 rsi')
  W('#define ARG3 rdx')
  W('#define ARG4 rcx')
  W('#define ARG5 r8')
  W('#define ARG5_NORETADDR ARG5')
  W('#define ARG6 r9')
  W('#define ARG6_NORETADDR ARG6')
  W('#define ARG7 QWORD [esp]')