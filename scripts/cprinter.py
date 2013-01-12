# Module for pretty-printing CType instances.
#
# Note: some of this isn't 100% right, especially where function pointers are
#       concerned.

from cparser import *

# Pretty-print a CType instance into valid C.
#
# Args:
#   ctype:            Instance of CType.
#   inner:            String representing either a variable name or some inner
#                     type info.
#
# Returns:
#   Pretty-printed string.
def pretty_print_type(ctype, inner="", lang="C"):
  s = ""

  def print_function(ctype, s, inner):
    param_types = []
      
    for param_ctype in ctype.param_types:
      param_types.append(pretty_print_type(param_ctype, lang=lang))
    
    if ctype.is_variadic and not ctype.is_old_style_variadic:
      param_types.append("...")

    if not param_types and not ctype.is_old_style_variadic:
      param_types.append("void")

    inner = "(%s%s)(%s)" % (s, inner, ", ".join(param_types))
    return pretty_print_type(ctype.ret_type, inner=inner, lang=lang)

  # pointers (including function pointers)
  if isinstance(ctype, CTypePointer):
    while isinstance(ctype, CTypePointer):
      s += "*"
      s += ctype.is_const     and " const " or "" 
      if "C" == lang:
        s += ctype.is_restrict  and " restrict " or ""
      s += ctype.is_volatile  and " volatile " or ""
      ctype = ctype.ctype

    # special case: function pointers
    if isinstance(ctype, CTypeFunction):
      s = print_function(ctype, s, inner)
    
    # non-function pointer
    else:
      s = "%s %s%s" % (pretty_print_type(ctype, lang=lang).strip(" "),
                        s,
                        inner.strip(" "))

  # function
  elif isinstance(ctype, CTypeFunction):
    s = print_function(ctype, s, inner)

  # built-in type
  elif isinstance(ctype, CTypeBuiltIn):
    s = "%s %s" % (ctype.name, inner)

  # typedef name
  elif isinstance(ctype, CTypeDefinition):
    s = "%s %s" % (ctype.name, inner) 

  # attributed type
  elif isinstance(ctype, CTypeAttributed):
    attrs = ctype.attrs
    tok_str = lambda tok: tok.str
    s = pretty_print_type(ctype.ctype, inner=inner, lang=lang)

    if isinstance(attrs, CTypeAttributes):
      for attr in CTypeAttributes.__slots__:
        if attr.startswith("is_"):
          if getattr(attrs, attr):
            s = "%s %s" % (attr[3:], s.strip(" "))
    else:

      # get the storage/visibility attributes
      for attr in CTypeAttributes.__slots__:
        if attr.startswith("is_"):
          if getattr(attrs, attr):
            s = "%s %s" % (attr[3:], s.strip(" "))

      # put left-hand side extension attributes before and the right-hand side
      # atributes after
      lhs = " ".join(t.str for t in attrs.attrs[attrs.LEFT])
      rhs = " ".join(t.str for t in attrs.attrs[attrs.RIGHT])
      s = "%s %s %s" % (lhs.strip(" "), s.strip(" "), rhs.strip(" "))

  # array
  elif isinstance(ctype, CTypeArray):
    s = "%s %s[%s]" % (pretty_print_type(ctype.ctype, lang=lang),
                       inner,
                       " ".join(ctype.size_expr_toks))

  # struct, union, enum; note: technically function and array as well; but these
  # should be caught elsewhere
  elif isinstance(ctype, CTypeCompound):
    s = "%s %s" % (ctype.name, inner)

  return s.strip(" ")
