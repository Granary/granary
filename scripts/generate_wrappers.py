
from cparser import *


def OUT(*args):
  print "".join(map(str, args))


def IGNORE(*args):
  pass


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
    ctype = unattributed_type(unaliased_type(unattributed_type(ctype)))
  return ctype


def is_function_pointer(ctype):
  if isinstance(ctype, CTypePointer):
    internal = unattributed_type(ctype.ctype)
    return isinstance(internal, CTypeFunction)
  return False


def is_wrappable_type(ctype):
  ctype = base_type(ctype)
  if isinstance(ctype, CTypePointer):
    internal = base_type(ctype.ctype)
    return isinstance(internal, CTypeStruct)
  return isinstance(ctype, CTypeStruct)


def will_pre_wrap_fileds(ctype):
  for ctype, field_name in ctype.fields():
    intern_ctype = base_type(ctype)
    if not field_name:
      if isinstance(intern_ctype, CTypeStruct):
        return will_pre_wrap_fileds(intern_ctype)
    elif is_function_pointer(intern_ctype):
      return True
    elif is_wrappable_type(intern_ctype):
      return True
  return False


def pre_wrap_fields(ctype, O):
  for ctype, field_name in ctype.fields():
    intern_ctype = base_type(ctype)
    if not field_name:
      if isinstance(intern_ctype, CTypeStruct):
        pre_wrap_fields(intern_ctype, O)
    elif is_function_pointer(intern_ctype):
      O("        WRAP_FUNC(arg.", field_name, ");")
    elif is_wrappable_type(intern_ctype):
      O("        WRAP_RECURSIVE(arg.", field_name, ");")


def post_wrap_fields(ctype, O):
  pass


def wrap_struct(ctype, name):
  if not will_pre_wrap_fileds(ctype):
    return

  O = ctype.has_name and OUT or IGNORE
  O("MAKE_WRAPPER(", name, ", ", "{")
  O("    PRE {")
  pre_wrap_fields(ctype, O)
  O("    }")
  O("    NO_POST")
  O("})")
  O("")


def will_wrap_function(ret_type, arg_types):
  ctypes = [ret_type] + arg_types
  for ctype in ctypes:
    ctype = base_type(ctype)
    if is_function_pointer(ctype) or is_wrappable_type(ctype):
      return True
  return False

def wrap_function(ctype, func):

  # only care about non-variadic functions if they return wrappable types.
  if not ctype.is_variadic:
    if not will_wrap_function(ctype.ret_type, []):
      return

  # only care about variadic functions if they take wrappable parameters /
  # return values
  elif ctype.is_variadic:
    if not will_wrap_function(ctype.ret_type, ctype.param_types):
      return

  ret_type = base_type(ctype.ret_type)
  suffix = ""
  if isinstance(ret_type, CTypeBuiltIn) and "void" == ret_type.name:
    suffix = "_VOID"

  variadic = ""
  if ctype.is_variadic:
    if ctype.param_types:
      variadic = ", "
    variadic += "..."

  O = OUT
  O("FUNCTION_WRAPPER", suffix, "(", var, ", (", variadic, ") {")
  if ctype.is_variadic:
    O("    // TODO: variadic arguments")
  O("})")
  O()


def visit_enum(ctype):
  pass


def visit_function(ctype):
  visit_type(ctype.ret_type)
  for param_ctype in ctype.param_types:
    if param_ctype:
      visit_type(param_ctype)


def visit_attributed(ctype):
  visit_type(ctype.ctype)


def visit_expression(ctype):
  pass


def visit_bitfield(ctype):
  visit_type(ctype.ctype)


def visit_array(ctype):
  visit_type(ctype.ctype)


def visit_pointer(ctype):
  visit_type(ctype.ctype)


def visit_typedef(ctype):
  visit_type(ctype.ctype)

  inner = unattributed_type(ctype.ctype)
  if isinstance(inner, CTypeStruct) and not inner.has_name:
    wrap_struct(inner, ctype.name)

def visit_builtin(ctype):
  pass


def visit_union(ctype):
  for field_ctype, field_name in ctype.fields():
    visit_type(field_ctype)


def visit_struct(ctype):
  for field_ctype, field_name in ctype.fields():
    visit_type(field_ctype)
  wrap_struct(ctype, ctype.name)


TYPES = set()
VISITORS = {
  CTypeEnum:          visit_enum,
  CTypeFunction:      visit_function,
  CTypeAttributed:    visit_attributed,
  CTypeExpression:    visit_expression,
  CTypeBitfield:      visit_bitfield,
  CTypeArray:         visit_array,
  CTypePointer:       visit_pointer,
  CTypeDefinition:    visit_typedef,
  CTypeBuiltIn:       visit_builtin,
  CTypeUnion:         visit_union,
  CTypeStruct:        visit_struct,
}


def visit_type(ctype):
  if ctype in TYPES:
    return
  TYPES.add(ctype)
  VISITORS[ctype.__class__](ctype)


def visit_var_def(var, ctype):
  visit_type(ctype)
  ctype = unattributed_type(ctype)

  # don't declare enumeration constants
  if isinstance(ctype, CTypeFunction):
    wrap_function(ctype, var)

if "__main__" == __name__:
  import sys
  with open(sys.argv[1]) as lines_:
    buff = "".join(lines_)
    tokens = CTokenizer(buff)
    parser = CParser()
    parser.parse(tokens)

    for var, ctype in parser.vars():
      visit_var_def(var, ctype)
    
