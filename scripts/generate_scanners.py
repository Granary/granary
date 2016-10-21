"""Generate Macro-ized C++ code for scanning kernel types.

This works by recursively iterating over all the fields of a 
kernel types scanning them for the watchpoints

"""


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


def ifdef_name(scoped_name):
  return scoped_name.replace(" ", "_") \
        .replace("::", "_") \
        .replace(".", "_")


def scan_var(ctype, var_name, O, indent="    "):
  intern_ctype = ctype.base_type()
  if is_buildin_type(intern_ctype):
    O(indent, "SCAN_FUNCTION(", var_name, ");")
  elif is_function_pointer(intern_ctype):
    O(indent, "SCAN_FUNCTION(", var_name, ");")
  elif is_wrappable_type(intern_ctype):
    if isinstance(ctype, CTypePointer):
      O(indent, "SCAN_RECURSIVE_PTR(", var_name, ");")
    else:
      if var_name is not None:
        O(indent, "SCAN_RECURSIVE(", var_name, ");")
  else:
    print "// ", intern_ctype, var_name


def pre_wrap_fields(ctype, O):
  for field_ctype, field_name in ctype.fields():
     scan_var(
        field_ctype, 
        field_name and ("arg.%s" % field_name) or None,
        O)


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
  will_pre = will_pre_wrap_fields(ctype)
  will_post = will_post_wrap_fields(ctype)
  if not will_pre and not will_post:
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


  O =  OUT or NULL

  #O("#define SCANNER_FOR_", ifdef_name(name))
  O("#ifndef SCANNER_FOR_", ifdef_name(name))
  O("#define SCANNER_FOR_", ifdef_name(name))
  O("TYPE_SCAN_WRAPPER(", name, ", ", "{")
  O("    S(granary::printf( \"",name, "\\n\");)")
  O("    S(SCAN_OBJECT(arg);)")
  pre_wrap_fields(ctype, O)
  O("})")
  O("#endif")
  O("")
  O("")


def wrap_typedef(ctype, name):
  O = OUT

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
  if isinstance(inner, CTypeStruct) and will_pre_wrap_fields(inner):
    if not inner.has_name:
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


def visit_type_def(var, ctype):
  visit_type(ctype)
  orig_ctype = ctype
  ctype = orig_ctype.base_type()


if "__main__" == __name__:
  import sys
  
  with open(sys.argv[1]) as lines_:
    tokens = CTokenizer(lines_)
    parser = CParser()

    OUT("/* Auto-generated scanning functions. */")
    OUT("#define S(...) __VA_ARGS__ ")
    OUT("")


    units = parser.parse_units(tokens)
    for unit_decls, unit_toks, is_typedef in units:
      for ctype, var in unit_decls:
        visit_type_def(var, ctype)
