"""Common functions used by generate_wrappers and generate_detach_table
for figuring out what should be wrapped."""


from cparser import *


def is_function_pointer(ctype):
  if isinstance(ctype, CTypePointer):
    internal = ctype.ctype.unattributed_type()
    return isinstance(internal, CTypeFunction)
  return False


def is_wrappable_type(ctype):
  ctype = ctype.base_type()
  if isinstance(ctype, CTypePointer):
    internal = ctype.ctype.base_type()
    return isinstance(internal, CTypeStruct)
  return isinstance(ctype, CTypeStruct)


WILL_WRAP_CACHE = {}


def must_wrap(ctypes, seen_=None):
  seen = seen_ or set()
  must = False
  for ctype in ctypes:
    if must:
      break

    ctype = ctype.base_type()
    if ctype in seen:
      continue
    else:
      seen.add(ctype)

    if isinstance(ctype, CTypeStruct):
      for t, n in ctype.fields():
        must = must_wrap([t.base_type()], seen)
        if must:
          break

    elif is_function_pointer(ctype):
      must = True

    elif isinstance(ctype, CTypePointer):
      must = must_wrap([ctype.ctype.base_type()], seen)

  return must


def will_pre_wrap_feilds(ctype):
  global WILL_WRAP_CACHE
  if ctype in WILL_WRAP_CACHE:
    return WILL_WRAP_CACHE[ctype]

  ret = False
  for ctype, field_name in ctype.fields():
    intern_ctype = ctype.base_type()
    if not field_name:
      if isinstance(intern_ctype, CTypeStruct):
        ret = will_pre_wrap_feilds(intern_ctype)
    elif is_function_pointer(intern_ctype):
      ret = True
    elif is_wrappable_type(intern_ctype):
      ret = True
  
  WILL_WRAP_CACHE[ctype] = ret
  return ret


def will_wrap_function(ret_type, arg_types):
  ctypes = [ret_type] + arg_types
  for ctype in ctypes:
    ctype = ctype.base_type()
    if is_function_pointer(ctype) or is_wrappable_type(ctype):
      return True
  return False

