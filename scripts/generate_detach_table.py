"""Generate macros and code for use in the detach hash table function(s).

Each function is associated with a unique id."""

from cparser import *

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


SEEN = set()

# TODO: currently have linker errors for these.
IGNORE = set([
  "add_profil",
  "alloca",
  "profil",
  "unwhiteout",
  "zopen",
  "_IO_cookie_init",
  "matherr",
  "setkey",
  "zopen",

  # apple-only?
  "pthread_rwlock_downgrade_np",
  "pthread_rwlock_upgrade_np",
  "pthread_rwlock_tryupgrade_np",
  "pthread_rwlock_held_np",
  "pthread_rwlock_rdheld_np",
  "pthread_rwlock_wrheld_np",
  "pthread_getname_np",
  "pthread_setname_np",
  "pthread_rwlock_longrdlock_np",
  "pthread_rwlock_yieldwrlock_np",

  # 'dangerous' Linux symbols
  "sigreturn",
  "tmpnam",
  "tmpnam_r",
  "tempnam",
  "gets", # hrmm
  "mktemp",

  # non-portable?
  "pthread_mutexattr_setrobust",
  "pthread_mutexattr_getrobust",

  # things that are wacky to wrap
  "setjmp",
  "_setjmp",
  "__setjmp",

  "sigsetjmp",
  "_sigsetjmp",
  "__sigsetjmp",

  "longjmp",
  "_longjmp",
  "__longjmp",

  "siglongjmp",
  "_siglongjmp",
  "__siglongjmp"
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
  
  if has_extension_attribute(ctype, "deprecated"):
    return

  if name.startswith("__"):
    return

  C("WRAP_FOR_DETACH(", name,")")


def visit_var_def(var, ctype):

  # don't declare enumeration constants
  if isinstance(ctype.base_type(), CTypeFunction):
    visit_function(ctype, var)


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

    C()

