"""Generate Macro-ized C++ code for wrapping detach functions.

This works by iterating over all functions, and for each function, visiting the
types of its arguments and return value. Type visitors are transitive.
Eventually, all reachable types are visited. Visitors of certain kinds of types
will emit type wrappers if and only if a wrapper is necessary to maintain
attach/detach requirements."""

from cparser import *
from cprinter import pretty_print_type
from ignore import should_ignore
from wrap import *
import re

def OUT(*args):
  print "".join(map(str, args))


def NULL(*args):
  pass


VA_LIST_FUNCS = set()


def pre_wrap_var(ctype, var_name, O, indent="        "):
  intern_ctype = ctype.base_type()
  if not var_name:
    if isinstance(intern_ctype, CTypeStruct):
      pre_wrap_fields(intern_ctype, O)
  elif is_function_pointer(intern_ctype):
    O(indent, "WRAP_FUNCTION(", var_name, ");")
  elif is_wrappable_type(intern_ctype):
    O(indent, "PRE_WRAP(", var_name, ");")


def pre_wrap_fields(ctype, O):
  for field_ctype, field_name in ctype.fields():
    pre_wrap_var(field_ctype, field_name and ("arg.%s" % field_name) or None, O)


def post_wrap_fields(ctype, O):
  pass


def scoped_name(ctype):
  parts = []
  while True:

    # if the type is scoped, then we use the internal name
    # to omit 'struct', 'union', and 'enum' so that we don't
    # end up with things like "foo::struct bar".
    if ctype.parent_ctype:
      parts.append(ctype.internal_name)
      ctype = ctype.parent_ctype

    # if the type is not scoped, then we use the full name
    # to disambiguate it from, e.g. functions with the same
    # name
    else:
      parts.append(ctype.name)
      break
  return "::".join(reversed(parts))


def wrap_struct(ctype):
  if not will_pre_wrap_feilds(ctype):
    return

  # make sure we're not trying to wrap a struct that is
  # embedded in an anonymous union
  parent_ctype = ctype.parent_ctype
  while parent_ctype:
    if isinstance(parent_ctype, CTypeUnion) \
    and not parent_ctype.had_name:
      return
    parent_ctype = parent_ctype.parent_ctype

  name = scoped_name(ctype)

  O = ctype.has_name and OUT or NULL
  O("TYPE_WRAPPER(", name, ", ", "{")
  O("    PRE {")
  pre_wrap_fields(ctype, O)
  O("    }")
  O("    NO_POST")
  O("    NO_RETURN")
  O("})")
  O("")


def wrap_typedef(ctype, name):
  O = OUT

  # e.g. "typedef struct foo foo;" is somewhat ambiguous (from the perspective
  # of C++ template partial specialization), so we omit such typedefs.
  #if name != ctype.internal_name:
  #  O("TYPEDEF_WRAPPER(", name, ", ", ctype.name, ")")
  #O("")


# Output Granary code that will wrap a C function.
def wrap_function(ctype, orig_ctype, func):

  # only care about non-variadic functions if they return wrappable types.
  # otherwise, we always care about manually wrapping variadic functions
  # and functions that don't return.
  if not ctype.is_variadic \
  and not has_extension_attribute(orig_ctype, "noreturn"):
    #if not will_wrap_function(ctype.ret_type, []):
    return

  # don't wrap deprecated functions; the compiler will complain about them.
  if has_extension_attribute(orig_ctype, "deprecated"):
    return

  if not must_wrap([ctype.ret_type] + ctype.param_types):
    return

  # internal function
  #elif func.startswith("__"):
  #  return

  O = OUT

  internal_ret_type = ctype.ret_type.base_type()
  suffix, is_void = "", False
  if isinstance(internal_ret_type, CTypeBuiltIn) \
  and "void" == internal_ret_type.name:
    suffix, is_void = "_VOID", True

  variadic = ""
  if ctype.is_variadic:
    if ctype.param_types:
      variadic = ", "
    variadic += "..."

  arg_list = []
  num_params = [0]
  def next_param(p):
    if p:
      return p
    else:
      num_params[0] += 1
      return "_arg%d" % num_params[0]

  param_names = map(next_param, ctype.param_names)
  last_arg_name = ""
  for (arg_ctype, arg_name) in zip(ctype.param_types, param_names):
    if not arg_name:
      arg_name = ""
    last_arg_name = arg_name
    arg_list.append(pretty_print_type(arg_ctype, arg_name, lang="C++").strip(" "))
  args = ", ".join(arg_list)

  # get an output string for the return type.
  ret_type = ""
  if not is_void:
    ret_type = pretty_print_type(ctype.ret_type, "", lang="C++").strip(" ")
    ret_type = " (%s), " % ret_type

  addr_check = ""
  if func.startswith("__"):
    addr_check = " && defined(DETACH_ADDR_%s)" % func

  O("#if defined(CAN_WRAP_", func, ") && CAN_WRAP_", func, addr_check)
  O("#ifndef WRAPPER_FOR_", func)
  O("FUNCTION_WRAPPER", suffix, "(", func, ",", ret_type ,"(", args, variadic, "), {")

  if ctype.is_variadic:
    O("    va_list args__;")
    O("    va_start(args__, %s);" % last_arg_name)
  
  # assignment of return value; unattributed_type is used in place of base type
  # so that we don't end up with anonymous structs/unions/enums.
  a, r_v = "", ""
  if not is_void:
    r_v = "ret"
    a = pretty_print_type(ctype.ret_type.unattributed_type(), r_v, "C++") + " = "
  
  for (arg_ctype, arg_name) in zip(ctype.param_types, param_names):
    pre_wrap_var(arg_ctype, arg_name, O, indent="    ")

  global VA_LIST_FUNCS
  va_func = "v%s" % func

  special = False
  if ctype.is_variadic and va_func in VA_LIST_FUNCS:
    O("    IF_KERNEL( auto ", va_func, "((decltype(::", va_func, ") *) DETACH_ADDR_", va_func,"); ) ")
    O("    ", a, va_func, "(", ", ".join(param_names + ["args__"]), ");")
  else:
    if ctype.is_variadic:
      special = True
      O("    // TODO: variadic arguments")
    O("    D( granary_fault(); )")
    O("    ", a, func, "(", ", ".join(param_names), ");")

  if ctype.is_variadic:
    O("    va_end(args__);")

  #O("    D( granary::printf(\"function_wrapper(%s) %s\\n\"); )" % (func, special and "*" or ""))  

  if not is_void and not isinstance(ctype.ret_type.base_type(), CTypeBuiltIn):
    O("    RETURN_WRAP(", r_v, ");")

  if not is_void:
    O("    return ", r_v, ";")

  O("})")
  O("#endif")
  O("#endif")
  O()
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

  inner = ctype.ctype.base_type()
  if isinstance(inner, CTypeStruct) and will_pre_wrap_feilds(inner):
    if not inner.has_name:
      # todo: make sure some structures are not double wrapped
      wrap_struct(inner, ctype.name)
    else:
      wrap_typedef(inner, ctype.name)


def visit_builtin(ctype):
  pass


def visit_union(ctype):
  for field_ctype, field_name in ctype.fields():
    visit_type(field_ctype)


def visit_struct(ctype):
  for field_ctype, field_name in ctype.fields():
    visit_type(field_ctype)
  
  if ctype.has_name:
    wrap_struct(ctype)


def visit_use(ctype):
  visit_type(ctype.ctype)


TYPES = set()
VISITORS = {
  CTypeUse:           visit_use,
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
  orig_ctype = ctype
  ctype = orig_ctype.base_type()

  # don't declare enumeration constants
  if isinstance(ctype, CTypeFunction):
    wrap_function(ctype, orig_ctype, var)

    
def visit_possible_variadic_def(name, ctype, va_list_ctype):
  global VA_LIST_FUNCS
  if not isinstance(ctype, CTypeFunction):
    return

  if not ctype.param_types:
    return

  last_param_ctype = ctype.param_types[-1].base_type()

  if last_param_ctype is va_list_ctype:
    VA_LIST_FUNCS.add(name)


if "__main__" == __name__:
  import sys
  
  with open(sys.argv[1]) as lines_:
    buff = "".join(lines_)
    tokens = CTokenizer(buff)
    parser = CParser()
    parser.parse(tokens)
    va_list = None
    try:
      va_list = parser.get_type("va_list", CTypeDefinition)
      va_list = va_list.base_type()
    except:
      pass

    OUT("/* Auto-generated wrappers. */")
    OUT("#define D(...) __VA_ARGS__ ")
    for var, ctype in parser.vars():
      if not should_ignore(var) and var.startswith("v"):
        visit_possible_variadic_def(var, ctype.base_type(), va_list)

    for var, ctype in parser.vars():
      if not should_ignore(var):
        visit_var_def(var, ctype)
