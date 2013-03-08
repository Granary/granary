
import re
import fileinput
import os
from ignore import IGNORE

FULL_PATH = re.compile("(/([^/]+/)+([^ \n\r\t=>()]*))")
SYMBOL_NAME = re.compile("^[a-zA-Z_][a-zA-Z_0-9]*$")


# Add a symbol to our set of symbols. This is mostly just
# a way to weed out any unusual symbols that may have slipped
# past any previous sanity checks.
def add_symbol(sym, symbols):
  global SYMBOL_NAME
  if SYMBOL_NAME.match(sym):
    symbols.add(sym)


# Get exported symbols from a .dylib file.
def get_symbols_darwin(dll, symbols):
  try:
    gen = os.popen("nm -g " + dll).readlines()
  except:
    pass

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
    pass

  for line in gen:
    if "FUNC" in line:
      parts = line.strip(" \r\n\t").split(" ")
      sym = parts[-1].split("@")[0]
      add_symbol(sym, symbols)


def O(*args):
  print "".join(args)


if "__main__" == __name__:
  dlls = []
  for line in fileinput.input():
    m = FULL_PATH.search(line)
    if not m:
      continue
    dlls.append(m.group(1))

  symbols = set()
  for dll in dlls:
    if ".dylib" in dll:
      get_symbols_darwin(dll, symbols)
    elif ".so" in dll:
      get_symbols_linux(dll, symbols)

  for symbol in symbols - IGNORE:
    O("#ifndef CAN_WRAP_", symbol)
    O("    DETACH(", symbol, ")")
    O("#endif")