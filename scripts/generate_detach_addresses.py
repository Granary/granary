"""Generate the macros mapping kernel functions to their addresses
memory."""

SYMBOLS = set()
FOUND_SYMBOLS = {}

if "__main__" == __name__:
  prefix = "#ifndef CAN_WRAP_"
  prefix_len = len(prefix)
  lines = []

  with open("granary/gen/detach.inc", "r") as lines_:
    for line in lines_:
      lines.append(line)
      line = line.strip(" \r\n\t")
      if line.startswith(prefix):
        SYMBOLS.add(line[prefix_len:])
  
  # special function that we need!
  SYMBOLS.add("module_alloc_update_bounds")

  with open("/proc/kallsyms", "r") as lines_:
    for line in lines_:
      line = line.strip(" \r\n\t")
      line = line.replace("\t", " ")
      parts = line.split(" ")
      sym = parts[2]
      if sym in SYMBOLS:
        FOUND_SYMBOLS[sym] = parts[0]

  with open("granary/gen/detach.inc", "w") as f:
    new_lines = []
    for sym, addr in FOUND_SYMBOLS.items():
      new_lines.append("#ifndef DETACH_ADDR_%s\n" % sym)
      new_lines.append("#    define DETACH_ADDR_%s 0x%s\n" % (sym, addr))
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

