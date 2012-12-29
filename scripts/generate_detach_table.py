"""Generate macros and code for use in the detach hash table function(s).

Each function is associated with a unique id."""

from cparser import *

header, code = None, None


def H(*args):
  global header
  header.write("".join(map(str, args)) + "\n")


def C(*args):
  global code
  code.write("".join(map(str, args)) + "\n")


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

# TODO: currently have linker errors for these.
IGNORE = set([
  "add_profil",
  "alloca",
  "profil",
  "unwhiteout",
  "zopen"
])


def has_internal_linkage(ctype):
  is_inline, is_static = False, False
  if isinstance(ctype, CTypeAttributed):
    pass

def visit_function(ctype, name):
  global SEEN, IGNORE
  if name in SEEN or name in IGNORE:
    return
  SEEN.add(name)
  H("        DETACH_ID_", name, ",")
  C("        {(app_pc) ::", name,", wrapper_of<DETACH_ID_", name, ">(::", name, ")},")


def visit_var_def(var, ctype):
  ctype = base_type(ctype)

  # don't declare enumeration constants
  if isinstance(ctype, CTypeFunction):
    visit_function(ctype, var)


if "__main__" == __name__:
  import sys
  header = open(sys.argv[2], "w")
  code = open(sys.argv[3], "w")
  with open(sys.argv[1]) as lines_:
    buff = "".join(lines_)
    tokens = CTokenizer(buff)
    parser = CParser()
    parser.parse(tokens)

    H("/* Auto-generated detach IDs. */")
    H("#ifndef GRANARY_DETACH_IDS")
    H("#define GRANARY_DETACH_IDS")
    H("namespace granary {")
    H("    enum function_wrapper_id {")

    C("namespace granary {")
    C("    const function_wrapper FUNCTION_WRAPPERS[] = {")

    for var, ctype in parser.vars():
      visit_var_def(var, ctype)

    C("        {nullptr, nullptr}")
    C("    }; /* function_wrapper_id */")
    C("} /* granary:: */ ")
    C()

    H("        LAST_DETACH_ID")
    H("    };")
    H("} /* granary:: */")
    H("#endif /* GRANARY_DETACH_IDS */")
    H()
