"""Generate the macros mapping kernel functions to their addresses
memory.

Author:       Peter Goodman (peter.goodman@gmail.com)
Copyright:    Copyright 2012-2013 Peter Goodman, all rights reserved.
"""

import sys

SYMBOLS = set()
FOUND_SYMBOLS = {}
ADDRESSES = []
SYMBOL_TO_ADDRESS_INDEX = {}

if "__main__" == __name__:
  prefix = "#ifndef CAN_WRAP_"
  prefix_len = len(prefix)
  lines = []

  with open(sys.argv[1], "r") as lines_:
    for line in lines_:
      lines.append(line)
      line = line.strip(" \r\n\t")
      if line.startswith(prefix):
        SYMBOLS.add(line[prefix_len:])
  
  # special function that we need!
  EXTRA_SYMBOLS = set([
    "module_alloc_update_bounds", "process_one_work", "idt_table",
    "rcu_process_callbacks", "flush_tlb_mm_range", "printk",
    "__schedule_bug", "show_fault_oops", "__stack_chk_fail",
  ])
  MISSING_EXTRA_SYMBOLS = set()
  for sym in EXTRA_SYMBOLS:
    if sym not in SYMBOLS:
      MISSING_EXTRA_SYMBOLS.add(sym)
  SYMBOLS.update(EXTRA_SYMBOLS)

  with open("kernel.syms", "r") as lines_:
    for line in lines_:
      line = line.strip(" \r\n\t")
      line = line.replace("\t", " ")
      parts = line.split(" ")
      sym = parts[2]
      
      ADDRESSES.append(int(parts[0], base=16))

      if sym in SYMBOLS:
        FOUND_SYMBOLS[sym] = parts[0]
        SYMBOL_TO_ADDRESS_INDEX[sym] = len(ADDRESSES) - 1

  with open(sys.argv[1], "w") as f:
    new_lines = []
    for sym, addr in FOUND_SYMBOLS.items():
      func_index = SYMBOL_TO_ADDRESS_INDEX[sym]

      # Some symbols have different names but the same address, e.g.
      # `memset` and `__memset`, so if we only checked immediately
      # adjacent addresses then we might report the length as 0.
      func_len = 0
      while not func_len:
        func_len = ADDRESSES[func_index + 1] - ADDRESSES[func_index]
        func_index += 1

      if sym in MISSING_EXTRA_SYMBOLS:
        new_lines.append("#ifndef CAN_WRAP_%s\n" % sym)
        new_lines.append("#   define CAN_WRAP_%s\n" % sym)
        new_lines.append("    WRAP_FOR_DETACH(%s)\n" % sym)
        new_lines.append("#endif\n")

      new_lines.append("#ifndef DETACH_ADDR_%s\n" % sym)
      new_lines.append("#   define DETACH_ADDR_%s 0x%s\n" % (sym, addr))
      new_lines.append("#   define DETACH_LENGTH_%s %d\n" % (sym, func_len))
      new_lines.append("#endif\n")
    
    missing = SYMBOLS - set(FOUND_SYMBOLS.keys())
    
    if "module_alloc_update_bounds" in missing:
      missing.remove("module_alloc_update_bounds")

    for sym in missing:
      new_lines.append("#ifndef CAN_WRAP_%s\n" % sym)
      new_lines.append("#    define CAN_WRAP_%s 0\n" % sym)
      new_lines.append("#endif\n")
    
    new_lines.extend(lines)
    f.write("".join(new_lines))

