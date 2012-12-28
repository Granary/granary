"""Generate macros and code for use in the detach hash table function(s).

Each function is associated with a unique id."""

from cparser import *


def I(*args):
  print "".join(map(str, args))


def unattributed_type(ctype):
  while isinstance(ctype, CTypeAttributed):
    ctype = ctype.ctype
  return ctype


def unaliased_type(ctype):
  while isinstance(ctype, CTypeDefinition):
    ctype = ctype.ctype
  return ctype


def base_type(ctype):
  prev_type = None
  while prev_type != ctype:
    prev_type = ctype
    ctype = unaliased_type(unattributed_type(ctype))
  return ctype


SEEN = set()


def visit_function(ctype, name):
  global SEEN
  if name in SEEN:
    return
  SEEN.add(name)
  I("    DETATCH_ID_", name, ",")


def visit_var_def(var, ctype):
  ctype = base_type(ctype)

  # don't declare enumeration constants
  if isinstance(ctype, CTypeFunction):
    visit_function(ctype, var)


if "__main__" == __name__:
  import sys
  with open(sys.argv[1]) as lines_:
    buff = "".join(lines_)
    tokens = CTokenizer(buff)
    parser = CParser()
    parser.parse(tokens)

    I("/* Auto-generated detach IDs. */")
    I("#ifndef GRANARY_DETACH_IDS")
    I("#define GRANARY_DETACH_IDS")
    I("enum detach_id {")
    for var, ctype in parser.vars():
      visit_var_def(var, ctype)
    I("    LAST_DETACH_ID")
    I("};")
    I("#endif /* GRANARY_DETACH_IDS */")
    I()