"""Get the (exported) symbols from a dll (.so or .dylib) and add them to
the end of the detach table. This improves re-entrancy of Granary in user
space because it is typical for internal libc functions to be invoked, and
those functions are sometimes non-reentrant (and are indirectly invoked by
Granary). Many symbols resolved through this process will be looked up
using dlsym.

Author:       Peter Goodman (peter.goodman@gmail.com)
Copyright:    Copyright 2012-2013 Peter Goodman, all rights reserved.
"""

import sys
import re
import fileinput
import os
from cparser import *
from ignore import IGNORE, WHITELIST

FULL_PATH = re.compile("(/([^/]+/)+([^ \n\r\t=>()]*))")
SYMBOL_NAME = re.compile("^[a-zA-Z_][a-zA-Z_0-9]*$")
FUNCTION_NAMES = set()

# Add a symbol to our set of symbols. This is mostly just
# a way to weed out any unusual symbols that may have slipped
# past any previous sanity checks.
def add_symbol(sym, symbols):
  global SYMBOL_NAME
  if "@" in sym:
    sym = sym.split("@")[0]
  if SYMBOL_NAME.match(sym):
    symbols.add(sym)
    if "libc" in sym:
      symbols.add("_GI_" + sym)


# Get exported symbols from a .dylib file.
def get_symbols_darwin(dll, symbols):
  try:
    gen = os.popen("nm -g " + dll).readlines()
  except:
    gen = []

  for line in gen:
    if " T _" in line or " U _" in line:
      parts = line.strip(" \r\n\t").split(" ")
      sym = parts[-1]
      if sym.startswith("_"):
        add_symbol(sym[1:], symbols)


# Get symbols from a .so file.
def get_symbols_linux(dll, symbols):
  try:
    gen = os.popen("readelf --dyn-syms " + dll).readlines()
  except:
    gen = []

  for line in gen:
    if "FUNC" in line:
      parts = line.strip(" \r\n\t").split(" ")
      sym = parts[-1].split("@")[0]
      add_symbol(sym, symbols)


def O(*args):
  print "".join(args)


if "__main__" == __name__:
  with open(sys.argv[1]) as lines_:
    buff = "".join(lines_)
    tokens = CTokenizer(buff)
    parser = CParser()
    parser.parse(tokens)

    for var, ctype in parser.vars():
      if isinstance(ctype.base_type(), CTypeFunction):
        FUNCTION_NAMES.add(var)

  dlls = []
  for line in fileinput.input():
    m = FULL_PATH.search(line)
    if not m:
      continue
    dlls.append(m.group(1))

  symbols = set(FUNCTION_NAMES)
  for dll in dlls:
    if ".dylib" in dll:
      get_symbols_darwin(dll, symbols)
    elif ".so" in dll:
      get_symbols_linux(dll, symbols)

  for symbol in symbols & WHITELIST:
    #O("#ifndef CAN_WRAP_", symbol)
    #O("    DETACH(", symbol, ")")
    #O("#endif")

    if symbol in FUNCTION_NAMES:
      O("WRAP_FOR_DETACH(", symbol, ")")
    else:
      O("DETACH(", symbol, ")")
  O()
