"""Reorder the declarations in a header so that all dependencies
are met.

Author:       Peter Goodman (peter.goodman@gmail.com)
Copyright:    Copyright 2012-2013 Peter Goodman, all rights reserved.
"""

import sys
import collections

from cparser import *

parser = CParser()
order_numbers = collections.defaultdict(int)
seen = set()
COMPOUNDS = set()
CHANGED = False


def O(*args):
  print "".join(map(str, args))


def visit_expression(ctype, expr, order_num):
  global parser

  for ref_ctype in expr.parse_types(parser):
    if ctype.base_type() is ref_ctype.base_type():
      continue

    order_num = max(
        order_num,
        1 + visit_ctype(ref_ctype, True, True))
  return order_num


def visit_enum(ctype, is_value, need_value, order_num):
  for expr in ctype.fields.values():
    order_num = max(
        order_num, visit_expression(ctype, expr, order_num))
  
  order_numbers[ctype] = order_num
  return order_num


def visit_function(ctype, is_value, need_value, order_num):
  order_num = max(
      1 + visit_ctype(ctype.ret_type, True, True),
      order_num)

  for param_ctype in ctype.param_types:
    if param_ctype:
      param_order_num = order_numbers[param_ctype]
      order_num = max(
          1 + visit_ctype(param_ctype, True, True),
          order_num)

  return order_num


def visit_attributed(ctype, is_value, need_value, order_num):
  return visit_ctype(ctype.ctype, is_value, need_value)


def visit_typeof(ctype, is_value, need_value, order_num):
  return visit_expression(CType(), ctype.expr, order_num)


def visit_bitfield(*args):
  # todo: types in its expression
  return visit_array(*args)


def visit_array(ctype, is_value, need_value, order_num):
  return max(
      order_num,
      visit_ctype(ctype.ctype, is_value, need_value))


def visit_pointer(ctype, is_value, need_value, order_num):
  intern_type = ctype.ctype.unattributed_type()
  if isinstance(intern_type, CTypeUse):
    return max(order_num, visit_ctype(ctype.ctype, False, False))
  else:
    return max(order_num, visit_ctype(ctype.ctype, True, True))


def visit_typedef(ctype, is_value, need_value, order_num):
  intern_ctype = ctype.ctype.unattributed_type()

  # if it is a value, and it's just referring to a used type, then
  # make it appear not to be a value.
  if is_value:
    is_value = not isinstance(intern_ctype, CTypeUse)
    need_value = need_value or is_value

  # if is is not a value, but it is referring to an in-line defined
  # type then make it appear to be a value.
  elif not isinstance(intern_ctype, CTypeUse):
    is_value = True

  if not is_value and need_value:
    is_value = True

  return max(
      order_num,
      1 + visit_ctype(ctype.ctype, is_value, need_value))


def visit_use(ctype, is_value, need_value, order_num):
  return visit_ctype(ctype.ctype, is_value, need_value)


def visit_builtin(*args):
  return 1


def visit_union(*args):
  return visit_struct(*args)


def visit_struct(ctype, is_value, need_value, order_num):
  global order_numbers, COMPOUNDS

  COMPOUNDS.add(ctype)

  if not is_value:
    return 0

  for field_ctype, field_name in ctype.fields():
    order_num = max(order_num, 1 + visit_ctype(field_ctype, is_value, True))

  return order_num


VISITORS = {
  CTypeUse:           visit_use,
  CTypeEnum:          visit_enum,
  CTypeFunction:      visit_function,
  CTypeAttributed:    visit_attributed,
  CTypeExpression:    visit_typeof,
  CTypeBitfield:      visit_bitfield,
  CTypeArray:         visit_array,
  CTypePointer:       visit_pointer,
  CTypeDefinition:    visit_typedef,
  CTypeBuiltIn:       visit_builtin,
  CTypeUnion:         visit_union,
  CTypeStruct:        visit_struct,
}


TAB = ""

def visit_ctype(ctype, is_value, need_value):
  global order_numbers, seen, CHANGED
 
  seen_id = (is_value, need_value, ctype)
  order_num = order_numbers[seen_id]
  old_order_num = order_num

  if seen_id not in seen:
    seen.add(seen_id)
    order_num = max(
        old_order_num,
        VISITORS[ctype.__class__](
            ctype, is_value, need_value, old_order_num))

    if order_num > old_order_num:
      CHANGED = True
      order_numbers[seen_id] = order_num

  return order_num


# Returns True iff this unit should be included in the file's
# output. This mainly looks to filter out variables.
def should_include_unit(unit_decls, unit_toks, is_typedef):
  global parser

  if is_typedef:
    return True

  for ctype, name in unit_decls:
    if name:
      base_type = ctype.base_type()
      if not isinstance(base_type, CTypeFunction):
        return False

      # Don't include functions returning floating point values.
      else:
        base_return_type = base_type.ret_type.base_type()
        if isinstance(base_return_type, CTypeBuiltIn) \
        and base_return_type.is_float:
          return False

    # Try not to include forward definitions of enums.
    base_ctype = ctype.base_type()
    if not isinstance(base_ctype, CTypeEnum):
      continue

    #if base_ctype.original_name == "ip_conntrack_infoip_conntrack_info":
    #  print unit_toks
    #  exit(ip_conntrack_infoip_conntrack_info)

    is_forward_decl = True
    for tok in unit_toks:
      if "{" == tok.str:
        is_forward_decl = False
        break

    if is_forward_decl:
      return False

  return True


# Look for duplicate global enumerator constants and remove enums
# with duplicate constants. This is a simplistic solution.
#
# This is mostly to address trivial cases of the following three
# specific problems:
#     i)   typedef struct foo { ... } foo;
#     ii)  enum { X }; ... enum { X }; 
#     iii) struct foo { }; ... struct foo { };
def process_redundant_decls(units):
  units = list(units)

  enum_constants = set()
  structs = set()
  unions = set()
  compounds = {
    CTypeUnion: set(),
    CTypeStruct: set()
  }

  open_comment = CToken("/*", CToken.COMMENT)
  close_comment = CToken("*/", CToken.COMMENT)

  T = 0

  for unit_decls, unit_toks, is_typedef in units:
    for ctype, name in unit_decls:
      base_ctype = ctype.base_type()
      
      # look for duplicate definitions of enumerator constants
      # and delete one of the enums (containing the duplicate
      # constant).
      if isinstance(base_ctype, CTypeEnum):
        if ctype.is_type_use():
          continue
        
        # the union has no name; delete it. This helps us avoid
        # issues in the kernel where an enumerator constant's value
        # is dependent on the return from an inline function, which
        # itself is stripped from the types.
        #
        # we just hope that we haven't deleted an enum where one of
        # the deleted enumerator constants is referenced elsewhere.
        #if not base_ctype.has_name:
        #  assert len(unit_decls) == 1
        #  #unit_toks.insert(0, open_comment)
        #  #unit_toks.append(close_comment)
        #  del unit_toks[:]
        #  break

        for name in base_ctype.field_list:
          if name in enum_constants:
            assert len(unit_decls) == 1
            #unit_toks.insert(0, open_comment)
            #unit_toks.append(close_comment)
            del unit_toks[:]
            break
          else:
            enum_constants.add(name)

      # look for duplicate definitions of structs / unions
      # and delete the most recently found type.
      elif isinstance(base_ctype, CTypeStruct) \
      or   isinstance(base_ctype, CTypeUnion):

        handled = False
        while not ctype.is_type_use():

          # try to distinguish a forward declaration from the real
          # definition. The types of the two things will be resolved
          if ";" == unit_toks[-1].str \
          and CToken.TYPE_USER == unit_toks[-2].kind \
          and CToken.TYPE_SPECIFIER == unit_toks[-3].kind:
            break

          names = compounds[base_ctype.__class__]
          if base_ctype.internal_name in names:
            assert len(unit_decls) == 1
            #unit_toks.insert(0, open_comment)
            #unit_toks.append(close_comment)
            del unit_toks[:]
            handled = True
          else:
            names.add(base_ctype.internal_name)
          break

        # no naming conflict
        if handled or name != base_ctype.original_name:
          continue

        # hard case, we need to preserve the struct/union
        # definition; we will try to do this by just renaming
        # the typedef'd name.
        if not ctype.is_type_use():
          #print name, base_ctype.original_name, unit_toks
          #assert CToken.TYPE_USER == unit_toks[-2].kind
          unit_toks[-2].str += str(T)
          T += 1
          continue

        # easy case, delete the typedef: because it's used in
        # C++, it means the compiler will resolve the correct
        # type.
        else:
          #unit_toks.insert(0, open_comment)
          #unit_toks.append(close_comment)
          del unit_toks[:]
          continue


# Remove __attribute__ ( ... ) forms from functions.
#
# Args:
#   toks:         A list of tokens representing a function
#                 declaration and which contains zero-or-more
#                 function attributes.
# 
# Returns:
#   A list of tokens without any function attributes.
def remove_function_attributes(decls, toks):

  for (ctype, _) in decls:
    if not isinstance(ctype.base_type(), CTypeFunction):
      return toks

  new_toks = []
  count_parens = False
  paren_count = 0
  i = 0

  for tok in toks:
    i += 1

    if count_parens:
      if "(" == tok.str:
        paren_count += 1
      elif ")" == tok.str:
        paren_count -= 1
        count_parens = paren_count > 0
      continue

    if tok.str in ("__attribute__", "attribute__", "__attribute"):
      count_parens = "(" == toks[i].str
      paren_count = 0
      continue

    new_toks.append(tok)

  return new_toks


# Process the units of C type, function, and variable declarations
# as generated by the CParser.
#
# Outputs:
#   Re-ordered declarations where ordering is determined by type
#   dependencies.
def process_units(units):
  global order_numbers, seen, CHANGED
  decls = []

  # Initialise the order numbers
  unit_num = 0
  units = list(units)
  for unit_decls, _, _ in units:
    decls.extend(unit_decls)

  # Refine until we have an ordering.
  CHANGED = True
  while CHANGED:
    CHANGED = False
    for ctype, name in decls:
      old_order_num = order_numbers[ctype, True, False]
      new_order_num = visit_ctype(ctype, True, False)
    seen.clear()

  # Emit the ordered units.
  new_toks = collections.defaultdict(list)
  for unit_decls, unit_toks, is_typedef in units:

    if not should_include_unit(unit_decls, unit_toks, is_typedef):
      continue

    # Get a canonical order number for this unit that accounts
    # for the ordering of any types defined within the unit.
    max_order_num = 0
    has_name = False
    for ctype, name in unit_decls:
      if name and not is_typedef:
        has_name = True
      max_order_num = max(
          max_order_num, order_numbers[True, False, ctype])

    if has_name:
      max_order_num = sys.maxint

    # Remove attributes from non-type declarations.
    unit_toks = remove_function_attributes(unit_decls, unit_toks)

    # Add the tokens for this unit of declarations into the
    # total ordering.
    new_toks[max_order_num].append("\n")
    new_toks[max_order_num].extend(t.str for t in unit_toks)

  toks = []
  for i in sorted(new_toks.keys()):
    toks.extend(new_toks[i])

  # output forward declarations for structs and unions
  global COMPOUNDS
  for ctype in COMPOUNDS:
    if ctype.has_name and ctype.had_name:
      O("%s;" % ctype.name)

  # output the tokens
  buff = " ".join(toks).split("\n")
  for line in buff:
    O(line.strip(" \n"))


if "__main__" == __name__:
  import sys
  
  with open(sys.argv[1]) as lines_:
    buff = "".join(lines_)
    tokens = CTokenizer(buff)
    units = parser.parse_units(tokens)
    process_redundant_decls(units)
    process_units(units)