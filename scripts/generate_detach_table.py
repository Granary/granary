"""Generate macros and code for use in the detach hash table function(s).

Each function is associated with a unique id.

Author:       Peter Goodman (peter.goodman@gmail.com)
Copyright:    Copyright 2012-2013 Peter Goodman, all rights reserved.
"""

from cparser import *
from ignore import should_ignore
from wrap import *

header, code = None, None


def C(*args):
  global code
  code.write("".join(map(str, args)) + "\n")


FUNCTIONS = {}


def visit_function(name, ctype):
  global FUNCTIONS
  if should_ignore(name):
    return

  func_ctype = ctype.base_type()

  will_wrap = True
  if False:
    if name in MUST_WRAP:
      will_wrap = True
    
    else:
      will_wrap = will_wrap_function(
          func_ctype.ret_type, func_ctype.param_types)
      
        # kthread_create_on_node is an issue here...
        #if has_extension_attribute(ctype, "printf"):
        #  will_wrap = False

  if will_wrap and func_ctype.is_variadic:
    will_wrap = must_wrap(
        [func_ctype.ret_type] + func_ctype.param_types)

  if will_wrap and has_extension_attribute(ctype, "deprecated"):
    will_wrap = False

  # put this before checking for things that we should ignore
  # so that these type-based rules propagate to the dll detach
  # stuff, where type info is not known.
  C("#ifndef CAN_WRAP_", name)
  C("#   define CAN_WRAP_", name, " ", int(will_wrap))
  C("#endif")

  if not will_wrap:
    C("DETACH(", name, ")")
    return

  # we don't want to add detach wrappers to these, but we do want
  # to detach on them. This is partially doing the work of the dll
  # detach thing, but also ignoring "hidden" versions of functions
  # that we already have, e.g. logf vs. __logf. 
  if name.startswith("__"):

    # TODO: is this check needed? It screws up wrapping for
    #       block_write_begin and __block_write_begin, which have
    #       different signatures in the kernel.
    
    if True: # if name[2:] not in FUNCTIONS:
      C("#if CAN_WRAP_", name)
      C("#   ifdef DETACH_ADDR_", name)
      C("        WRAP_FOR_DETACH(", name, ")")
      C("#   else")
      C("        TYPED_DETACH(", name, ")")
      C("#   endif")
      C("#endif")
    return

  C("#if CAN_WRAP_", name)
  if has_extension_attribute(ctype, "noreturn"):
    C("    DETACH(", name,")")
  else:
    C("    WRAP_FOR_DETACH(", name,")")
  C("#endif")

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

