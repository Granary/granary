# Module for pretty-printing CType instances.
#
# Note: some of this isn't 100% right, especially where function pointers are
#       concerned.

from cparser import CTypePointer, CTypeFunction, CTypeAttributes, \
                    CTypeDefinition, CTypeAttributed, CTypeExtendedAttributes, \
                    CTypeBuiltIn, CTypeArray, CTypeCompound

# Pretty-print a CType instance into valid C.
#
# Args:
#   ctype:            Instance of CType.
#   inner:            String representing either a variable name or some inner
#                     type info.
#
# Returns:
#   Pretty-printed string.
def pretty_print_type(ctype, inner=""):
  s = ""

  # pointers (including function pointers)
  if isinstance(ctype, CTypePointer):
    s = "*"
    s += ctype.is_const     and " const " or "" 
    s += ctype.is_restrict  and " restrict " or ""
    s += ctype.is_volatile  and " volatile " or ""
    
    spec = ctype.ctype

    # special case: function pointers
    if isinstance(spec, CTypeFunction):
      param_types = []
      
      for param_ctype in spec.param_types:
        param_types.append(pretty_print_type(param_ctype))
      
      if spec.is_variadic and not spec.is_old_style_variadic:
        param_types.append("...")

      inner = "(%s%s)(%s)" % (s, inner, ", ".join(param_types))
      s = pretty_print_type(spec.ret_type, inner)
    
    # non-function pointer
    else:
      s = "%s %s%s" % (pretty_print_type(spec).strip(" "),
                        s.strip(" "),
                        inner.strip(" "))

  # built-in type
  elif isinstance(ctype, CTypeBuiltIn):
    s = "%s %s" % (ctype.name, inner)

  # typedef name
  elif isinstance(ctype, CTypeDefinition):
    s = "%s %s" % (ctype.name, inner) 

  # attributed type
  elif isinstance(ctype, CTypeAttributed):
    attrs = ctype.attrs
    s = pretty_print_type(ctype.ctype, inner)

    if isinstance(attrs, CTypeAttributes):
      for attr in CTypeAttributes.__slots__:
        if attr.startswith("is_"):
          if getattr(attrs, attr):
            s = "%s %s" % (attr[3:], s.strip(" "))

    elif CTypeExtendedAttributes.LEFT == attrs.side:
      s = "%s %s" % (" ".join(attrs.attrs), s.strip(" "))
    else:
      s = "%s %s" % (s.strip(" "), " ".join(attrs.attrs))

  # array
  elif isinstance(ctype, CTypeArray):
    s = "%s %s[%s]" % (pretty_print_type(ctype.ctype),
                       inner,
                       " ".join(ctype.size_expr_toks))

  # struct, union, enum; note: technically function and array as well; but these
  # should be caught elsewhere
  elif isinstance(ctype, CTypeCompound):
    s = "%s %s" % (ctype.name, inner)

  return s.strip(" ")
