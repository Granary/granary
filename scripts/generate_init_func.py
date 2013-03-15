"""Parse an assembly file and look for (mangled) C++ names that
include 'KERNEL_INIT_VAR_', as these are the names of special functions
used to initialise global variables. Calls to these functions will be
emitted to an assembly routine that can be called from C to initialise
all of Granary's static data structures."""

import re
import sys

# E.g. _ZN7granary12_GLOBAL__N_112_GLOBAL__N_120KERNEL_INIT_VAR_1_37Ev
# where '1' is can be used to order this initialisation function within the
# translation unit, and '37' is the line number on which this function is
# defined.

FUNC_NAME = re.compile(
  r"(([a-zA-Z0-9_]*)KERNEL_INIT_VAR_([0-9]+)_([a-zA-Z0-9_]*))")

funcs = {}

with open(sys.argv[1]) as lines:
  for line in lines:
    if "KERNEL_INIT_VAR_" in line:
      m = FUNC_NAME.search(line.strip(" \r\n\t"))
      if m:
        func_name = m.group(1)
        if "_GLOBAL__" not in func_name:
          funcs[m.group(3)] = func_name

  calls = []
  for i in sorted(funcs.keys()):
    calls.append("call %s;\n" % funcs[i])

  print "".join(calls)
