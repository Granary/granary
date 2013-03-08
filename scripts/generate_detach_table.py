"""Generate macros and code for use in the detach hash table function(s).

Each function is associated with a unique id."""

from cparser import *
from ignore import IGNORE

header, code = None, None


def C(*args):
  global code
  code.write("".join(map(str, args)) + "\n")


def has_extension_attribute(ctype, attr_name):
  ctype = ctype.unaliased_type()
  while isinstance(ctype, CTypeAttributed):
    attrs = ctype.attrs
    if isinstance(attrs, CTypeNameAttributes):
      attr_toks = attrs.attrs[0][:]
      attr_toks.extend(attrs.attrs[1])
      for attr in attr_toks:
        if attr_name in attr.str:
          return True
    ctype = ctype.ctype.unaliased_type()
  return False


FUNCTIONS = {}


def has_internal_linkage(ctype):
  is_inline, is_static = False, False
  if isinstance(ctype, CTypeAttributed):
    pass

def visit_function(name, ctype):
  global IGNORE, FUNCTIONS
  if name in IGNORE:
    return

  # put this before checking for things that we should ignore
  # so that these type-based rules propagate to the dll detach
  # stuff, where type info is not known.
  C("#ifndef CAN_WRAP_", name)
  C("#   define CAN_WRAP_", name)
  C("#endif")
  
  if has_extension_attribute(ctype, "deprecated"):
    return

  # we don't want to add detach wrappers to these, but we do want
  # to detach on them. This is partially doing the work of the dll
  # detach thing, but also ignoring "hidden" versions of functions
  # that we already have, e.g. logf vs. __logf. 
  if name.startswith("__"):
    if name[2:] not in FUNCTIONS:
      C("TYPED_DETACH(", name, ")")
    return

  C("WRAP_FOR_DETACH(", name,")")


def visit_var_def(var, ctype):
  global FUNCTIONS

  # don't declare enumeration constants
  if isinstance(ctype.base_type(), CTypeFunction):
    FUNCTIONS[var] = ctype


if "__main__" == __name__:
  import sys
  code = open(sys.argv[2], "w")
  with open(sys.argv[1]) as lines_:
    buff = "".join(lines_)
    tokens = CTokenizer(buff)
    parser = CParser()
    parser.parse(tokens)

    for var, ctype in parser.vars():
      visit_var_def(var, ctype)

    for var, ctype in FUNCTIONS.items():
      visit_function(var, ctype)

    C()

