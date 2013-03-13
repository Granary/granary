"""Reorder the declarations in a header so that all dependencies
are met."""

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
# output. This mainly looks to filter out void-variables.
def should_include_unit(unit_decls, unit_toks, is_typedef):
  global parser

  if is_typedef:
    return True

  for ctype, name in unit_decls:
    if ctype.unattributed_type() is parser.T_V:
      return False

  return True


def process_units(units):
  global order_numbers, seen, CHANGED
  decls = []

  # initialise the order numbers
  unit_num = 0
  units = list(units)
  for unit_decls, unit_toks, is_typedef in units:
    decls.extend(unit_decls)

  # refine until we have an ordering.
  CHANGED = True
  while CHANGED:
    CHANGED = False
    for ctype, name in decls:
      old_order_num = order_numbers[ctype, True, False]
      new_order_num = visit_ctype(ctype, True, False)
    seen.clear()

  # emit the ordered units
  new_toks = collections.defaultdict(list)
  for unit_decls, unit_toks, is_typedef in units:
    if not should_include_unit(unit_decls, unit_toks, is_typedef):
      continue

    # get a canonical order number for this unit that accounts
    # for the ordering of any types defined within the unit
    max_order_num = 0
    has_name = False
    for ctype, name in unit_decls:
      if name and not is_typedef:
        has_name = True
      max_order_num = max(
          max_order_num, order_numbers[True, False, ctype])

    if has_name:
      max_order_num = sys.maxint
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
    process_units(units)