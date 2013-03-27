"""Common functions used by generate_wrappers and generate_detach_table
for figuring out what should be wrapped."""


from cparser import *


MUST_WRAP = set([
  "vfork"
])


def is_function_pointer(ctype):
  if isinstance(ctype, CTypePointer):
    return isinstance(ctype.ctype.base_type(), CTypeFunction)
  return False


def is_wrappable_type(ctype):
  ctype = ctype.base_type()
  if isinstance(ctype, CTypePointer):
    internal = ctype.ctype.base_type()
    return isinstance(internal, CTypeStruct)
  return isinstance(ctype, CTypeStruct)


WILL_PRE_WRAP_CACHE = {}
WILL_POST_WRAP_CACHE = {}

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


def will_pre_wrap_type(ctype):
  global WILL_PRE_WRAP_CACHE
  if ctype in WILL_PRE_WRAP_CACHE:
    return WILL_PRE_WRAP_CACHE[ctype]

  WILL_PRE_WRAP_CACHE[ctype] = False

  intern_ctype = ctype.base_type()
  ret = False
  if not must_wrap([intern_ctype]):
    ret = False
  elif has_attribute(ctype, "const"):
    ret = False
  elif isinstance(intern_ctype, CTypeStruct):
    ret = will_pre_wrap_fields(intern_ctype)
  elif is_function_pointer(intern_ctype):
    ret = True
  elif isinstance(intern_ctype, CTypePointer):
    ret = will_pre_wrap_type(intern_ctype.ctype)

  WILL_PRE_WRAP_CACHE[ctype] = ret

  return ret


def will_pre_wrap_fields(ctype):
  global WILL_PRE_WRAP_CACHE
  if ctype in WILL_PRE_WRAP_CACHE:
    return WILL_PRE_WRAP_CACHE[ctype]

  ret = False
  for field_ctype, field_name in ctype.fields():
    if will_pre_wrap_type(field_ctype):
      ret = True
      break
  
  WILL_PRE_WRAP_CACHE[ctype] = ret
  return ret


def will_post_wrap_type(ctype):
  global WILL_POST_WRAP_CACHE
  if ctype in WILL_POST_WRAP_CACHE:
    return WILL_POST_WRAP_CACHE[ctype]

  WILL_POST_WRAP_CACHE[ctype] = False
  ret = False
  intern_ctype = ctype.base_type()

  if must_wrap([intern_ctype]):
    if has_attribute(ctype, "const"):
      ret = True
    elif isinstance(intern_ctype, CTypePointer):
      ret = will_post_wrap_type(intern_ctype.ctype)

  WILL_POST_WRAP_CACHE[ctype] = ret
  return ret


def will_post_wrap_fields(ctype):
  global WILL_POST_WRAP_CACHE
  if ctype in WILL_POST_WRAP_CACHE:
    return WILL_POST_WRAP_CACHE[ctype]

  ret = False
  for field_ctype, field_name in ctype.fields():
    if will_post_wrap_type(field_ctype):
      ret = True
      break
  
  WILL_POST_WRAP_CACHE[ctype] = ret
  return ret


def will_wrap_function(ret_type, arg_types):
  ctypes = [ret_type] + arg_types
  for ctype in ctypes:
    ctype = ctype.base_type()
    if is_function_pointer(ctype) or is_wrappable_type(ctype):
      return True
  return False

