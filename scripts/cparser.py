"""C Parser.

This module defines a C tokenizer and parser. The result of parsing is an
abstract syntax tree.

This module assumes that the input source code is valid, pre-processed C. That
is, this parser makes no attempts to discover or report syntax or semantic
errors. For debugging / sanity checks, it often asserts that something must be
the case.

In some cases, this parser will accept programs that are not semantically valid.
For example, certain storage classes (register, auto) are not allowed outside of
functions. This parser makes no attempt to check/enforce that.

This parser will also "cheat" in some cases where the assumption of valid C
input will make it's life easier. For example, it is always assumed that
brackets, parentheses, and braces are matched and balanced. It uses this
(assumed) fact to quickly determine the end of declarations, etc. Thus, if the
input is not valid then this cheating will result in undefined behaviour.

This parser will attempt to parse a number of GNU C extensions. For example,
it will attempt to recognize __attribute__s, inline assembly, etc. It also
accepts zero-sized arrays, among other things.

A peculiarity of this parser is that it will not attempt to calculate the size
or offsets of C types. It leaves such things as future computations to be run
through an actual C compiler (presumably the one being used for normal
compilation) so that such information can be tied back in to the AST (by a
supposed other module) for perfectly matching object layouts with what the
targeted compiler would generate.

Author:       Peter Goodman (peter.goodman@gmail.com)
Copyright:    Copyright 2012 Peter Goodman, all rights reserved.
"""


# Sets of all operators, grouped in terms of the length of the operators.
OPERATORS = {
  1: set([
    "[", "]", "(", ")", ".", "&", "*", "+", "-", "~", "!", "/", "%", "<", ">",
    "^", "|", "?", ":", "=", ",",
  ]),

  2: set([
    "->", "++", "--", "<<", ">>", "<=", ">=", "==", "!=", "&&", "||", "*=", "/=",
    "%=", "+=", "-=", "&=", "^=", "|=", "<:", ":>", "<%", "%>", "%:",
  ]),

  3: set(["<<=", ">>=", ]),

  4: set(["%:%:", ]),
}


class CToken(object):
  """Represents a token in a C program."""

  __slots__ = ('str', 'kind', 'carat')

  OPERATOR                  = 0

  LITERAL_NUMBER            = 10
  LITERAL_STRING            = 11 # string and character literals
  LITERAL_IDENTIFIER        = 12 # can be converted into a TYPE_USER

  TYPE_BUILT_IN             = 20
  TYPE_MAYBE_BUILT_IN       = 21
  TYPE_USER                 = 22 # determined during parsing
  TYPE_QUALIFIER            = 23
  TYPE_SPECIFIER            = 24 # e.g. struct, union, enum
  SPECIFIER_STORAGE         = 25
  SPECIFIER_FUNCTION        = 26

  ELLIPSIS                  = 31

  STATEMENT_BEGIN           = 40
  STATEMENT_END             = 41

  EXTENSION                 = 50
  EXTENSION_NO_PARAM        = 51
  TYPEOF                    = 52

  EOF                       = 60

  # mapping of reserved words to token kinds
  RESERVED = {
    "auto":           SPECIFIER_STORAGE,
    "break":          STATEMENT_BEGIN,
    "case":           STATEMENT_BEGIN,
    "char":           TYPE_BUILT_IN,
    "const":          TYPE_QUALIFIER,
    "__const":        TYPE_QUALIFIER,
    "continue":       STATEMENT_BEGIN,
    "default":        STATEMENT_BEGIN,
    "do":             STATEMENT_BEGIN,
    "double":         TYPE_BUILT_IN,
    "else":           STATEMENT_BEGIN,
    "enum":           TYPE_SPECIFIER,
    "extern":         SPECIFIER_STORAGE,
    "float":          TYPE_BUILT_IN,
    "for":            STATEMENT_BEGIN,
    "goto":           STATEMENT_BEGIN,
    "if":             STATEMENT_BEGIN,
    "inline":         SPECIFIER_FUNCTION,
    "__inline__":     SPECIFIER_FUNCTION,
    "__inline":       SPECIFIER_FUNCTION,
    "int":            TYPE_BUILT_IN,
    "long":           TYPE_BUILT_IN,
    "register":       SPECIFIER_STORAGE,
    "restrict":       TYPE_QUALIFIER,
    "__restrict":     TYPE_QUALIFIER,
    "return":         STATEMENT_BEGIN,
    "short":          TYPE_BUILT_IN,
    "signed":         TYPE_BUILT_IN,
    "sizeof":         OPERATOR,
    "static":         SPECIFIER_STORAGE,
    "struct":         TYPE_SPECIFIER,
    "switch":         STATEMENT_BEGIN,
    "typedef":        SPECIFIER_STORAGE,
    "union":          TYPE_SPECIFIER,
    "unsigned":       TYPE_BUILT_IN,
    "void":           TYPE_BUILT_IN,
    "volatile":       TYPE_QUALIFIER,
    "while":          STATEMENT_BEGIN,
    "_Bool":          TYPE_BUILT_IN,
    "_Complex":       TYPE_BUILT_IN,
    "_Imaginary":     TYPE_BUILT_IN,

    "wchar_t":        TYPE_MAYBE_BUILT_IN,
    
    # extensions    
    "asm":            EXTENSION,
    "_asm":           EXTENSION,
    "__asm":          EXTENSION,
    "asm_":           EXTENSION,
    "asm__":          EXTENSION,
    "__asm__":        EXTENSION,
        
    "_volatile":      EXTENSION_NO_PARAM,
    "__volatile":     EXTENSION_NO_PARAM,
    "volatile_":      EXTENSION_NO_PARAM,
    "volatile__":     EXTENSION_NO_PARAM,
    "__volatile__":   EXTENSION_NO_PARAM,
    
    "__attribute__":  EXTENSION,
    "noinline":       EXTENSION_NO_PARAM,
    
    # note: not an exhaustive list
    "__extension__":  EXTENSION_NO_PARAM,
    "__alias__":      EXTENSION,
    "__aligned__":    EXTENSION,
    "__alloc_size__": EXTENSION,
    "__always_inline__":EXTENSION_NO_PARAM,
    "__gnu_inline__": EXTENSION_NO_PARAM,
    "__artificial__": EXTENSION_NO_PARAM,
    "__flatten__":    EXTENSION_NO_PARAM,
    "__error__":      EXTENSION,
    "__warning__":    EXTENSION,
    "__cdecl__":      EXTENSION_NO_PARAM,
    "__const__":      EXTENSION_NO_PARAM,
    "__format__":     EXTENSION,
    "__format_arg__": EXTENSION,
    "__leaf__":       EXTENSION_NO_PARAM,
    "__malloc__":     EXTENSION_NO_PARAM,
    "__noinline__":   EXTENSION_NO_PARAM,
    "__noclone__":    EXTENSION_NO_PARAM,
    "__nonnull__":    EXTENSION,
    "__noreturn__":   EXTENSION_NO_PARAM,
    "__nothrow__":    EXTENSION_NO_PARAM,
    "__pure__":       EXTENSION_NO_PARAM,
    "__hot__":        EXTENSION_NO_PARAM,
    "__cold__":       EXTENSION_NO_PARAM,
    "__no_address_safety_analysis__": EXTENSION_NO_PARAM,
    "__returns_twice__": EXTENSION_NO_PARAM,
    "__section__":    EXTENSION_NO_PARAM,
    "__stdcall__":    EXTENSION_NO_PARAM,
    "__syscall_linkage__": EXTENSION_NO_PARAM,
    "__target__":     EXTENSION,
    "__unused__":     EXTENSION_NO_PARAM,
    "__used__":       EXTENSION_NO_PARAM,
    "__visibility__": EXTENSION,
    "__warn_unused_result__": EXTENSION_NO_PARAM,
    "__weak__":       EXTENSION_NO_PARAM,

    "typeof":         TYPEOF,
    "__typeof__":     TYPEOF,
    "decltype":       TYPEOF,

    # os/compiler-specific extensions
    "__builtin_va_list":TYPE_BUILT_IN,
    "__signed__" :      TYPE_BUILT_IN,
  }

  def __init__(self, str_, kind_):
    self.str = str_
    self.kind = kind_
    self.carat = None # this is filled in by the tokenizer

  def __repr__(self):
    return self.str

  def __nonzero__(self):
    return len(self.str)


class CCarat(object):
  """Represents a character position in some file."""

  __slots__ = ('line', 'column')

  def __init__(self, line_, column_):
    self.line = line_
    self.column = column_


class CCharacterReader(object):
  """Character reader that tracks the current line and column within a
  buffer."""

  TAB_SIZE = 4

  # Initialise the CCharacterReader.
  #
  # Args:
  #   buff_:          Indexible sequence of characters.
  def __init__(self, buff_):
    self.buff = buff_
    self.last_idx = -1
    self.line = 1
    self.col = 0
    self.seen_carriage_return = False

  # Return the length of the internal character buffer.
  #
  # Returns:
  #   An integer representing the length of the buffer.
  def __len__(self):
    return len(self.buff)

  # Get a character from the reader. If the difference between the accessed
  # index and the previously accessed is greater than one, then it is assumed
  # that the consumer of the character reader is purposefully jumping over
  # multiple characters known to be on the same line.
  #
  # Note: This function assumes that the sequence of accessed indexes is
  #       non-decreasing.
  #
  # Args:
  #   idx:            The index (cursor) of the next character to read from the
  #                   buffer.
  #
  # Returns:
  #   A str instance representing a single character from the buffer.
  def __getitem__(self, idx):
    assert isinstance(idx, int)

    c = self.buff[idx]
    diff = idx - self.last_idx
    if not 0 <= diff:
      assert False

    if 0 < diff:
      self.last_idx = idx
      seen_cr, self.seen_carriage_return = self.seen_carriage_return, False

      if "\t" == c:
        self.col += (diff - 1) + CCharacterReader.TAB_SIZE
      elif "\r" == c:
        self.col = 1
        self.line += 1
        self.seen_carriage_return = True
      elif "\n" == c:
        if not seen_cr:
          self.col = 1
          self.line += 1
      else:
        self.col += diff

    return c

  # Peek at some character in the buffer without consuming the character or
  # changing the line/column.
  #
  # Args:
  #   idx:            Index of character to get.
  #
  # Returns:
  #   A str instance representing a single character from the buffer.
  def peek(self, idx):
    return self.buff[idx]

  # Returns the current line and column.
  #
  # Returns:
  #   A CCarat instance representing the location of the next character to be
  #   read.
  def next_checkpoint(self):
    return CCarat(self.line, self.col + 1)


class CTokenizer(object):
  """Tokenizer for something like GNU C99."""

  def __init__(self, buff_):
    self.buff = CCharacterReader(buff_)
    self.len = len(self.buff)
    self.pos = 0
    self.future_tokens = []

  # Generate over the tokens in the buffer. This will stop generating when the
  # end of the token stream has been reached.
  #
  # Note: An EOF token will not be generated.
  #
  # Generates:
  #   CToken instances.  
  def __iter__(self):
    while True:
      tok = self.get_token()
      if CToken.EOF == tok.kind:
        break
      yield tok
    raise StopIteration()

  # Put a token back into the token stream. The effect of this is that the
  # most recently pushed back token will be the next token returned by
  # `get_token`.
  #
  # Args:
  #   tok:          A CToken instance.
  def unget_token(self, tok):
    """Unget a token."""
    self.future_tokens.append(tok)

  # Get the next token. If one or more tokens has been pushed back into the
  # token stream then this will first get those.
  #
  # Note: This function will attempt to combine adjacent string literals into a
  #       single token.
  #
  # Returns:
  #   A CToken instance.
  def get_token(self):
    if self.future_tokens:
      return self.future_tokens.pop()

    self._consume_whitespace()
    
    carat = self.buff.next_checkpoint()
    tok = self._get_token()
    tok.carat = carat

    if CToken.LITERAL_STRING == tok.kind:
      while True:
        self._consume_whitespace()

        carat = self.buff.next_checkpoint()
        next_tok = self._get_token()
        next_tok.carat = carat

        # combine adjacent string literals into a single token
        if CToken.LITERAL_STRING == next_tok.kind:
          j = 1
          if "L" == next_tok.str[0]:
            j += 1

          tok.str = tok.str[:-1] + next_tok.str[j:]
          continue

        self.unget_token(next_tok)
        break

    return tok

  # Tokenize a number. This looks only for the digit part of a number,
  # scientific notation is handled by using _tokenize_number twice: once for
  # the base and once for the exponent.
  #
  # Note: this will gladly accept a number of illegal numbers, e.g. 10lll (long
  #       long long).
  #
  # Args:
  #   cs:           List in which to place the digits/parts of the number found.
  def _tokenize_number(self, cs):
    c = self.buff[self.pos]
    might_be_hex = ("0" == c)
    hex_x_pos = self.pos + 1
    hex_set_check = ""

    while self.pos < self.len:
      c = self.buff[self.pos]

      # change behaviour if we detect that this is a hexadecimal number
      if might_be_hex and hex_x_pos == self.pos and c in "xX":
        hex_set_check = "abcdefABCDEF"

      if not c.isdigit() and "." != c and c not in hex_set_check:
          break

      cs.append(c)
      self.pos += 1

    # handle trailing length specifier, e.g. ULL
    while self.pos < self.len:
      c = self.buff[self.pos]
      if c not in "lLuUfF":
        break
      cs.append(c)
      self.pos += 1

  # Skip over whitespace.
  def _consume_whitespace(self):
    while self.pos < self.len:
      if self.buff[self.pos] in " \n\r\t":
        self.pos += 1
        continue
      break

  # Return the next token in the string. This does not handle pushed back tokens
  # and updates the internal string cursor.
  #
  # Note: this function expects to be called with an internal cursor pointing to
  #       the end of the buffer, or pointing to the beginning of a token.
  #
  # Returns:
  #   An instance of CToken.
  def _get_token(self):

    # end of file
    if self.pos >= self.len:
      return CToken("", CToken.EOF)

    c = self.buff[self.pos]
    c2, c3, c4 = "", "", ""
    cs = []

    if self.pos + 1 < self.len:
      c2 = self.buff.peek(self.pos + 1)

    if self.pos + 2 < self.len:
      c3 = self.buff.peek(self.pos + 2)

    if self.pos + 3 < self.len:
      c4 = self.buff.peek(self.pos + 3)

    # long string / character literals
    if "L" == c and c2 in "\"'":
      self.pos += 1
      c = c2
      cs.append(c)
      # fall-through to stirng literal

    # string/character literal
    if c in "\"'":
      escape = False
      end_char = c
      self.pos += 1
      cs.append(end_char)
      while self.pos < self.len:
        c = self.buff[self.pos]
        if not escape and end_char == c:
          break
        escape = "\\" == c
        cs.append(c)
        self.pos += 1
      self.pos += 1
      cs.append(end_char)
      return CToken("".join(cs), CToken.LITERAL_STRING)

    # identifier
    if c.isalpha() or "_" == c:
      while self.pos < self.len:
        c = self.buff[self.pos]
        if not c.isalnum() and "_" != c:
          break
        cs.append(c)
        self.pos += 1
      tok = "".join(cs)
      if tok in CToken.RESERVED:
        return CToken(tok, CToken.RESERVED[tok])
      return CToken(tok, CToken.LITERAL_IDENTIFIER)

    # number, includes integrals, floating points, octal, and hex; includes
    # length specifiers (ull, etc.) and scientific notation.
    if c.isdigit():
      self._tokenize_number(cs)

      # handle scientific notation / binary exponents
      c = self.buff[self.pos]
      if c in "eEpP":
        cs.append(c)
        self.pos += 1

        assert self.pos < self.len
        c = self.buff[self.pos]
        if c in "+-":
          cs.append(c)
        else:
          assert c.isdigit()
        self._tokenize_number(cs)
      
      return CToken("".join(cs), CToken.LITERAL_NUMBER)

    # statement beginners
    if "{" == c:
      self.pos += 1
      return CToken(c, CToken.STATEMENT_BEGIN)

    # statement enders
    if c in "};":
      self.pos += 1
      return CToken(c, CToken.STATEMENT_END)

    # 4-letter operators
    op = c + c2 + c3 + c4
    if op in OPERATORS[4]:
      self.pos += 4
      return CToken(op, CToken.OPERATOR)

    # 3-letter operators
    op = c + c2 + c3
    if op in OPERATORS[3]:
      self.pos += 3
      return CToken(op, CToken.OPERATOR)

    # ellipsis
    if "..." == op:
      self.pos += 3
      return CToken(op, CToken.ELLIPSIS)

    # two-letter operators
    op = c + c2
    if op in OPERATORS[2]:
      self.pos += 2
      return CToken(op, CToken.OPERATOR)

    # one-letter operators
    if c in OPERATORS[1]:
      self.pos += 1
      return CToken(c, CToken.OPERATOR)

    assert False


class CType(object):
  """Base class for all types."""
  
  def __getitem__(self, name):
    if name in self.__class__.__slots__:
      return getattr(self, name)
    elif 'ctype' in self.__class__.__slots__:
      return self.ctype[name]
    elif name.startswith("is_"):
      return False
    else:
      assert False

  def unattributed_type(self):
    ctype = self
    while isinstance(ctype, CTypeAttributed):
      ctype = ctype.ctype
    return ctype

  def unaliased_type(self):
    ctype = self
    while isinstance(ctype, CTypeDefinition):
      ctype = ctype.ctype
    return ctype

  def base_type(self):
    prev_type = None
    ctype = self
    while prev_type != ctype:
      prev_type = ctype
      ctype = ctype.unattributed_type().unaliased_type()
    return ctype


class CTypeCompound(CType):
  """Base class for user-defined compound types."""
  pass


class CTypeStruct(CTypeCompound):
  """Represents a structure type."""

  __slots__ = ('_id', 'name', 'internal_name', 
               '_fields', '_field_list', 'has_name',
               'parent_ctype')
  ID = 0

  def __init__(self, name_=None):
    self._id = CTypeStruct.ID
    self.has_name = True
    CTypeStruct.ID += 1
    if not name_:
      self.has_name = False
      name_ = "anon_struct_%d" % self._id

    self.internal_name = name_
    self.name = "struct " + name_
    self.parent_ctype = None
    self._fields = {}
    self._field_list = []

  def __repr__(self):
    return "Struct(%s)" % self.name

  def add_field(self, ctype, name):
    self._field_list.append((ctype, name))
    if name:
      self._fields[name] = ctype
  
  def fields(self):
    return iter(self._field_list)


class CTypeUnion(CTypeCompound):
  """Represents a union type."""

  __slots__ = ('_id', 'name', 'internal_name',
               '_fields', '_field_list', 'has_name',
               'parent_ctype')
  ID = 0
  
  def __init__(self, name_=None):
    self._id = CTypeUnion.ID
    self.has_name = True
    CTypeUnion.ID += 1
    if not name_:
      self.has_name = False
      name_ = "anon_union_%d" % self._id

    self.internal_name = name_
    self.name = "union " + name_
    self.parent_ctype = None
    self._fields = {}
    self._field_list = []

  def __repr__(self):
    return "Union(%s)" % self.name

  def add_field(self, ctype, name):
    self._field_list.append((ctype, name))
    if name:
      self._fields[name] = ctype

  def fields(self):
    return iter(self._field_list)


class CTypeEnum(CTypeCompound):
  """Represents an enumeration type."""

  __slots__ = ('_id', 'name', 'internal_name',
               'fields', 'field_list', 'has_name',
               'parent_ctype')
  ID = 0

  def __init__(self, name_=None):
    self._id = CTypeEnum.ID
    self.has_name = True
    CTypeEnum.ID += 1
    if not name_:
      self.has_name = False
      name_ = "anon_enum_%d" % self._id

    self.internal_name = name_
    self.name = "enum " + name_
    self.parent_ctype = None
    self.field_list = []
    self.fields = {}

  def __repr__(self):
    return "Enum(%s)" % self.name


class CTypeBuiltIn(CType):
  """Represents a built-in type."""

  __slots__ = ('name',)

  def __init__(self, name_):
    self.name = name_

  def __repr__(self):
    return self.name


class CTypeDefinition(CType):
  """Represents a `typedef`d type name."""

  __slots__ = ('name', 'ctype', 'is_missing')
  
  def __init__(self, name_, ctype_, is_missing=False):
    self.name = name_
    self.ctype = ctype_
    self.is_missing = is_missing

    # update internal names of some compound types
    base_ctype = ctype_.base_type()
    if isinstance(base_ctype, CTypeStruct) \
    or isinstance(base_ctype, CTypeUnion) \
    or isinstance(base_ctype, CTypeEnum):
      if not base_ctype.has_name:
        base_ctype.name = name_
        base_ctype.internal_name = name_
        base_ctype.has_name = True

  def __repr__(self):
    return "TypeDef(%s)" % self.name


class CTypePointer(CType):
  """Represents a C pointer type.
  
  This tracks a single indirection (*) and the flags following that
  indirection. """

  __slots__ = ('ctype', 'is_const', 'is_restrict', 'is_volatile')

  def __init__(self, ctype_, **kargs):
    self.ctype = ctype_
    self.is_const = False
    self.is_restrict = False
    self.is_volatile = False
    for k in kargs:
      setattr(self, k, kargs[k])

  def __repr__(self):
    c = self.is_const     and "const "    or ""
    r = self.is_restrict  and "restrict " or ""
    v = self.is_volatile  and "volatile " or ""
    return "Pointer(%s%s%s%s)" % (c, r, v, repr(self.ctype))


class CTypeArray(CTypeCompound):
  """Represents an array type in C.

  Some aspects of C99's parameterized (with qualifiers, dependent names, etc.)
  declarator features are incorporated."""

  __slots__ = ('is_vla', 'is_const', 'is_restrict', 'size_is_lower_bound',
               'size_expr_toks', 'direct_decl', 'is_dependent', 'ctype')

  def __init__(self, ctype_):

    # left-hand side of this array direct declarator
    self.ctype = ctype_
    self.is_dependent = False
    
    # true iff either an expression or a "*" is given as the size of the array.
    self.is_vla = False

    # used for determining the decayed pointer type of this array.
    self.is_const = False
    self.is_restrict = False

    # e.g. if static is used, then this declares that the array is non-null
    #      and its size can be used as a lower bound
    self.size_is_lower_bound = False
    self.size_expr_toks = [] # some list of tokens for later evaluation

  def __repr__(self):
    return "Array(%s)" % repr(self.ctype)


class CTypeAttributes(object):
  """Represents attributes applied to a type."""

  __slots__ = ('is_const', 'is_register', 'is_auto', 'is_volatile',
               'is_restrict', 'is_signed', 'is_unsigned')

  def __init__(self, **kargs):
    self.is_const = False
    self.is_register = False  # todo: put in NameAttributes?
    self.is_auto = False      # todo: put in NameAttributes?
    self.is_volatile = False
    self.is_restrict = False # todo: needed?
    self.is_signed = False
    self.is_unsigned = False
    
    for k in kargs:
      setattr(self, k, kargs[k])

  def has_default_attrs(self):
    if self.is_const or self.is_register or self.is_auto or self.is_volatile:
      return False
    elif self.is_restrict or self.is_signed or self.is_unsigned:
      return False
    return True
  
  def __repr__(self):
    s = ""
    s += self.is_const    and "const "    or ""
    s += self.is_register and "register " or ""
    s += self.is_auto     and "auto "     or ""
    s += self.is_volatile and "volatile " or ""
    s += self.is_restrict and "restrict " or ""
    s += self.is_signed   and "signed "   or ""
    s += self.is_unsigned and "unsigned " or ""
    
    return s


class CTypeNameAttributes(object):
  """Defines attributes specific to some named object. These attributes
  include both visibility specifiers as well as function/variable extended
  attributes."""

  __slots__ = ('attrs', 'is_inline', 'is_extern', 'is_static')

  LEFT, RIGHT = 0, 1

  def __init__(self, that_=None):
    self.attrs = [[], []]
    self.is_inline = that_ and that_.is_inline or False
    self.is_extern = that_ and that_.is_extern or False
    self.is_static = that_ and that_.is_static or False
    if that_:
      self.attrs[self.LEFT].extend(that_.attrs[self.LEFT])
      self.attrs[self.RIGHT].extend(that_.attrs[self.RIGHT])

  def has_default_attrs(self):
    if self.is_inline or self.is_extern or self.is_static:
      return False
    elif len(self.attrs[0]) or len(self.attrs[1]):
      return False
    return True

  def __repr__(self):
    s = ""
    s += self.is_inline   and " inline"   or ""
    s += self.is_extern   and " extern"   or ""
    s += self.is_static   and " static"   or ""
    attrs = self.attrs[self.LEFT][:]
    attrs.extend(self.attrs[self.RIGHT])
    return (" ".join(t.str for t in attrs) + s).strip(" ")


class CTypeAttributed(CType):
  """Represents an attributed type (i.e. a type with attributes)."""

  __slots__ = ('ctype', 'attrs')

  def __init__(self, ctype_, attrs_):
    assert ctype_
    assert attrs_
    self.ctype = ctype_
    self.attrs = attrs_


class CTypeExpression(CType):
  """Represents a type that is computed by some expression e.g.: typeof (...)."""

  __slots__ = ('toks',)

  def __init__(self, toks_):
    self.toks = toks_

  def __repr__(self):
    return " ".join(map(lambda t: t.str, self.attrs))


class CTypeBitfield(CType):
  """Represents a bitfield type."""

  __slots__ = ('ctype', 'bits')

  def __init__(self, ctype_, bits_):
    self.ctype = ctype_
    self.bits = bits_

  def __repr__(self):
    return "Bitfield(%s)" % repr(self.ctype)


class CTypeFunction(CTypeCompound):
  """Represents a function type."""

  def __init__(self, ret_type_):
    self.ret_type = ret_type_
    self.is_static = False
    self.is_extern = False
    self.is_inline = False
    self.param_types = []
    self.param_names = []
    self.is_variadic = False # e.g. old style, or using ...
    self.is_old_style_variadic = False # old style, e.g. int foo();

    # raise up these storage class specifiers applied to the return type
    # to be applied to the function type.
    for attr in ('is_static', 'is_extern', 'is_inline'):
      pass # todo

  def __repr__(self):
    types_str = ", ".join(map(repr, [self.ret_type] + self.param_types))
    return "Function(%s)" % types_str


class CStackedDict(object):
  """Represents a stack of dictionaries, where lookup proceeds down the stack.

  This is very convenient for implementing the various name lookup mechanisms
  within a symbol table."""

  def __init__(self, parent_=None):
    self._dict = {}
    self._parent = parent_
  
  def __contains__(self, item):
    if item in self._dict:
      return True
    if self._parent:
      return item in self._parent
    return False

  def __getitem__(self, item):
    if item in self._dict:
      return self._dict[item]
    if self._parent:
      return self._parent[item]
    raise IndexError("Item %s is not in the symbol table." % repr(item))

  def __setitem__(self, item, val):
    #assert item not in self._dict
    self._dict[item] = val

  def __iter__(self):
    return iter(self._dict)

  def keys(self):
    return iter(self._dict.keys())

  def values(self):
    return iter(self._dict.values())

  def items(self):
    return iter(self._dict.items())


class CSymbolTable(object):
  """Represents a symbol table for the C language.

  The symbol table is implemented in terms of stacked dictionaries
  (CStackedDict). Each kind of symbol (variable, typedef name, struct, union,
  and enum) has its own stacked dictionary. This separation is so that there
  are no ambiguities between two things like `foo`, `struct foo`, and 
  `union foo`, and `enum foo`.

  The symbol table maps type names to CType instances, and maps variable names
  to their types (CType instances)."""

  def __init__(self, parent_=None):
    parent_vars = None
    parent_structs, parent_unions, parent_enums, parent_defs = [None] * 4
    if parent_:
      parent_vars     = parent_._vars
      parent_structs  = parent_._types[CTypeStruct]
      parent_unions   = parent_._types[CTypeUnion]
      parent_enums    = parent_._types[CTypeEnum]
      parent_defs     = parent_._types[CTypeDefinition]

    self._vars = CStackedDict(parent_vars)
    self._types = {
      CTypeStruct:      CStackedDict(parent_structs),
      CTypeUnion:       CStackedDict(parent_unions),
      CTypeEnum:        CStackedDict(parent_enums),
      CTypeDefinition:  CStackedDict(parent_defs)
    }

  def vars(self):
    return iter(self._vars.items())

  def has_var(self, name):
    return name in self._vars

  def has_type(self, name, const):
    assert const in self._types
    return name in self._types[const]

  def set_var(self, name, info):
    assert name
    try:
      self._vars[name] = info
    except:
      pass # assume we are re-declaring the same thing.

  def set_type(self, name, ctype):
    assert ctype.__class__ in self._types

    # special case typedefs; we try as best as we can to handle undefined
    # types that are later (re)defined. To handle this gracefully,
    # everything uses the same typedef instance, but we will switch out
    # the internally aliased type.
    if isinstance(ctype, CTypeDefinition):
      tab = self._types[CTypeDefinition]
      if name not in tab:
        tab[name] = ctype
      else:
        tab[name].ctype = ctype
    else:
      self._types[ctype.__class__][name] = ctype

  def get_var(self, name):
    return self._vars[name]

  # Get a type by its name and it's type constructor
  def get_type(self, name, const):
    assert const in self._types
    assert name in self._types[const]
    return self._types[const][name]


class CParser(object):
  """Parse a token stream of something like C99 and generate an AST."""

  # Constructors for various different compound types.
  COMPOUND_TYPE = {
    "union":  CTypeUnion, 
    "enum":   CTypeEnum,
    "struct": CTypeStruct,
  }

  # Constructor for making/choosing a symbol table to use when parsing the body
  # of a compound type.
  COMPOUND_TYPE_STAB = {
    "union":  lambda _, parent_stab: CSymbolTable(parent_stab),
    "struct": lambda _, parent_stab: CSymbolTable(parent_stab),

    # enum gets and places its symbols in the enclosing scope. E.g. if we have
    # struct { enum { FOO } bar; };, then FOO is available in the scope in which
    # the struct is defined and not limited to being visible within the "{" and
    # "}" of the definition of the struct.
    "enum":   lambda self, _: self.scope_stack[-1], 
  }

  # Built-in types.
  T_V   = CTypeBuiltIn("void")
  T_C   = CTypeBuiltIn("char")
  T_UC  = CTypeBuiltIn("unsigned char")
  T_S   = CTypeBuiltIn("short")
  T_US  = CTypeBuiltIn("unsigned short")
  T_I   = CTypeBuiltIn("int")
  T_UI  = CTypeBuiltIn("unsigned int")
  T_L   = CTypeBuiltIn("long")
  T_UL  = CTypeBuiltIn("unsigned long")
  T_LL  = CTypeBuiltIn("long long")
  T_ULL = CTypeBuiltIn("unsigned long long")
  T_F   = CTypeBuiltIn("float")
  T_D   = CTypeBuiltIn("double")
  T_DL  = CTypeBuiltIn("long double")
  T_WC  = CTypeBuiltIn("wchar_t")

  T_CHR = CTypeAttributed(T_C, CTypeAttributes(is_const=True))
  T_STR = CTypePointer(T_C, is_const=True)
  T_INT = CTypeAttributed(T_LL, CTypeAttributes(is_const=True))
  T_FLT = CTypeAttributed(T_D, CTypeAttributes(is_const=True))

  VA_LIST = CTypeBuiltIn("__builtin_va_list")

  # Mapping of (sorted) token string sequences to built-in type objects.
  BUILT_IN_TYPES = {
    ("__builtin_va_list",):           VA_LIST,

    ("void",):                        T_V,

    ("char",):                        T_C,
    ("char", "unsigned"):             T_UC,
    ("char", "signed"):               T_C,
    ("__signed__", "char",):          T_I,

    ("short",):                       T_S,
    ("int", "short"):                 T_S,
    ("short", "unsigned"):            T_US,
    ("int", "short", "unsigned"):     T_US,
    ("short", "signed"):              T_S,
    ("__signed__", "short"):          T_S,

    ("signed",):                      T_I,
    ("__signed__",):                  T_I,
    ("int",):                         T_I,
    ("auto",):                        T_I,
    ("register",):                    T_I,
    ("int", "signed"):                T_I,
    ("__signed__", "int"):            T_I,
    ("unsigned",):                    T_UI,
    ("int", "unsigned"):              T_UI,

    ("int", "long"):                  T_L,
    ("int", "long", "signed"):        T_L,
    ("int", "long", "unsigned"):      T_UL,
    ("long",):                        T_L,
    ("long", "signed"):               T_L,
    ("__signed__", "long"):           T_L,
    ("long", "unsigned"):             T_UL,

    ("int", "long", "long"):          T_LL,
    ("int", "long", "long", "signed"):T_LL,
    ("int", "long", "long", "unsigned"):T_ULL,
    ("long", "long"):                 T_LL,
    ("long", "long", "signed"):       T_LL,
    ("__signed__", "long", "long"):   T_LL,
    ("long", "long", "unsigned"):     T_ULL,

    ("float",):                       T_F,
    ("double",):                      T_D,
    ("double", "long"):               T_DL,

    ("wchar_t",):                     T_WC,
  }

  # Mapping of opening brace/bracket/paren chars to their closing chars.
  CLOSING = {
    "(": ")",
    "[": "]",
    "{": "}",
    "<": ">",
  }

  # Mapping of compiler-specific specifiers to non-specific specifiers.
  SPECIFIER_MAP = {
    "__inline__":           "inline",
    "__inline":             "inline",
    "__const":              "const",
    "__restrict":           "restrict",
  }

  # Initialise the parser.
  def __init__(self):

    # The global symbol table.
    self.stab = CSymbolTable()
    
    # Stack of scopes (symbol tables) so that we can always talk about the
    # current scope using self.scope_stack[-1]
    self.scope_stack = [self.stab]

    # Stack of structure or union types. When parsing, e.g. a structure in a
    # union, it is good to know that this structure really is a sub-structure
    # just in case our use case involves interpreting C as if it were C++, where
    # this nesting is akin to namespacing.
    self.type_stack = []

  # Parse a union or struct.
  #
  # Args:
  #   stab:           Symbol table in which to place new names.
  #   ctype:          The type object representing the type that we are currently
  #                   parsing.
  #   outer_toks:     The token stream from which we will extract only those
  #                   tokens (`toks`) belonging to this compound type definition.
  #   j:              Our current cursor position in the `outer_toks` (the
  #                   position of the opening '{')
  #
  # Returns:
  #   The position in toks to continue parsing (immediately following the
  #   closing } of the compound type definition).
  def _parse_struct_union_type(self, stab, struct_ctype, outer_toks, j):
    toks = []
    assert "{" == outer_toks[j].str
    j = self._get_up_to_balanced(outer_toks, toks, j, "{")
    i = 0
    
    # add in a "breadcrumb" for scope resolution.    
    if self.type_stack:
      struct_ctype.parent_ctype = self.type_stack[-1]

    self.type_stack.append(struct_ctype)

    while i < len(toks):
      carat = toks[i].carat
      i, decls, is_typedef = self._parse_declaration(stab, toks, i)
      assert not is_typedef

      # note: might have an unnamed struct declaration field, so long as it's a
      # bitfield, or an unnamed union
      for (ctype, name) in decls:
        struct_ctype.add_field(ctype, name)

    self.type_stack.pop()

    return j

  # Parse an enum type.
  #
  # Args:
  #   stab:           Symbol table in which to place enumeration constants and
  #                   from where the symbols of constant expressions can be
  #                   found.
  #   ctype:          The CTypeEnum instance representing this enumeration.
  #   outer_toks:     The sequence of tokens within which the definition of the
  #                   union forms a sub-sequence.
  #   j:              A pointer into `outer_toks`, where the `j`th `outer_tok`
  #                   is the opening brace of the enum.
  #
  # Returns:
  #   A pointer into `outer_toks` that points to the token immediately following
  #   the last token of the enumeration.
  def _parse_enum_type(self, stab, ctype, outer_toks, j):
    # add in a "breadcrumb" for scope resolution.    
    if self.type_stack:
      ctype.parent_ctype = self.type_stack[-1]

    toks = []
    assert "{" == outer_toks[j].str
    j = self._get_up_to_balanced(outer_toks, toks, j, "{")
    i = 0
    building_expr = False
    while i < len(toks):
      t = toks[i]
      
      if CToken.LITERAL_IDENTIFIER == t.kind:
        if not building_expr:
          stab.set_var(t.str, ctype)
        i += 1
      elif "," == t.str:
        building_expr = False
        i += 1
      elif "=" == t.str:
        building_expr = True
        i += 1
      elif t.str in "[(":
        #if not building_expr:
        #  print t.str, t.carat.line, t.carat.column
        assert building_expr
        sub_expr_toks = []
        i = self._get_up_to_balanced(toks, sub_expr_toks, i, t.str)

      else:

        # this is some weird case with enums in the kernel where, after
        # processing the headers, we might observe things like `0 = 0`
        # in an enum, so we will skip over stuff until we hit a comma.
        if not building_expr:
          i += 1
          while i < len(toks):
            t = toks[i]
            i += 1
            if t.str in ",}":
              break
            elif t.str in "{[(":
              i = self._get_up_to_balanced(
                  toks, sub_expr_toks, i - 1, t.str)
        
        # assume that if we're building an expression then this is
        # a token that can validly belong to the expression.
        else:
          i += 1

    return j


  # Specific parsing functions for each type of compound user-defined type.
  COMPOUND_TYPE_PARSER = {
    "struct": _parse_struct_union_type,
    "union": _parse_struct_union_type,
    "enum": _parse_enum_type,
  }

  # Parse a single declaration.
  #
  # Args:
  #   stab:           Symbol table in which to place new names.
  #   toks:           All tokens within a single declaration (and no more).
  #   i:              A cursor into `toks`.
  #   end_on_comma:   True iff we should see "," as ending a declaration.
  #
  # Returns:
  #   A cursor on the token in `toks` immediately following the last token of
  #   this declaration.
  def _parse_declaration(self, stab, toks, i, end_on_comma=False):
    user_type, built_in_type, has_declarator = False, False, False

    specs = []
    decls = []
    
    # extraneous useless declaration enders
    if i < len(toks):
      t = toks[i]
      if ";" == t.str or (end_on_comma and "," == t.str):
        return i + 1, decls, False

    name_attrs = CTypeNameAttributes()
    i, specs_ctype, defines_type = self._parse_specifiers(stab, name_attrs, toks, i)

    # parse the declarators until we make no progress
    while i < len(toks):
      t = toks[i]
      if t.str in ";)]}" or (end_on_comma and "," == t.str):
        i += 1
        break

      has_declarator = True
      i, ctype, ctype_name_attrs, name = self._parse_declarator(
          stab, specs_ctype, CTypeNameAttributes(name_attrs), toks, i, end_on_comma)
      
      if not ctype_name_attrs.has_default_attrs():
        ctype = CTypeAttributed(ctype, ctype_name_attrs)

      decls.append((ctype, name))

    if not has_declarator: # e.g. function parameter
      decls.append((specs_ctype, None))

    return i, decls, defines_type

  # Parse a list of specifiers.
  #
  # Args:
  #   stab:             Symbol table in which names will be found/placed.
  #   name_attrs:       A CTypeNameAttributes instance representing the
  #                     visibility and extension attributes that should be
  #                     applied to this name.
  #   toks:             List of tokens.
  #   i:                Cursor into `toks`.
  #
  # Returns:
  #   A tuple `(i, ctype, defines_type)` representing the next cursor position
  #   `i` into `toks` and the `ctype` representing the type speficiers. If
  #   `defines_type` is True then we will be defining some type names.
  def _parse_specifiers(self, stab, name_attrs, toks, i):
    attrs = CTypeAttributes()
    built_in_names = []
    ctype = None
    defines_type = False
    might_have_type = False
    carat = toks[i].carat

    while i < len(toks):
      t = toks[i]
      i += 1
      
      if "typedef" == t.str:
        defines_type = True

      # special case 1: these can either set the type, or modify it.
      elif t.str in ("signed", "__signed__"):
        assert not attrs.is_unsigned
        assert not attrs.is_signed
        attrs.is_signed = True
        might_have_type = True

      elif "unsigned" == t.str:
        assert not attrs.is_unsigned
        assert not attrs.is_signed
        attrs.is_unsigned = True
        might_have_type = True

      # special case 2: both of these storage class specifiers can also be
      # used to define ints.
      elif t.str in ("register", "auto"):
        attr_name = "is_" + t.str
        assert not getattr(attrs, attr_name)
        setattr(attrs, attr_name, True)
        ctype = CParser.BUILT_IN_TYPES[t.str,]
        might_have_type = True

      # normal built-in type
      elif CToken.TYPE_BUILT_IN == t.kind:
        built_in_names.append(t.str)
        might_have_type = True

      # potentially built-in types; this will alter the behaviour of the
      # tokenizer.
      elif CToken.TYPE_MAYBE_BUILT_IN == t.kind:

        # if it was previously defined, then it must be a user-defined type.
        if stab.has_type(t.str, CTypeDefinition):
          t.kind = CToken.TYPE_USER
          ctype = stab.get_type(t.str, CTypeDefinition)
          CToken.RESERVED[t.str] = CToken.TYPE_USER
          might_have_type = True

        # this is the first thing we've seen in a list of types; and we have no
        # prior definition of the type, so it's likely not user-defined; let's
        # assume this is a typedef in terms of this built-in type.
        elif not might_have_type: # built_in_names:
          t.kind = CToken.TYPE_BUILT_IN
          CToken.RESERVED[t.str] = CToken.TYPE_BUILT_IN
          built_in_names.append(t.str)
          might_have_type = True

        # we've seen some other built-in types; let's assume that maybe built-
        # ins and built-ins can't combine, so this is likely a typedef of this
        # maybe-built-in type.
        else:
          t.kind = CToken.LITERAL_IDENTIFIER
          CToken.RESERVED[t.str] = CToken.TYPE_USER
          i -= 1
          break

      # typedef name
      elif CToken.TYPE_USER == t.kind:
        ctype = stab.get_type(t.str, CTypeDefinition)
        might_have_type = True
      
      # possible typedef name
      elif CToken.LITERAL_IDENTIFIER == t.kind:
        if stab.has_type(t.str, CTypeDefinition):

          # todo: assume it's a re-definition of the type in
          # terms of another typedef'd type.
          if isinstance(ctype, CTypeDefinition):
            i -= 1
            break

          found_ctype = stab.get_type(t.str, CTypeDefinition)

          # assume a definition of a type that was used before
          # it was defined, so we are defining it now
          if found_ctype.is_missing \
          and (might_have_type or ctype or built_in_names) \
          and defines_type:
            i -=1
            break

          # try extra hard to find things that might be defined
          # before use
          elif might_have_type or ctype or built_in_names:
            if defines_type and i < len(toks):
              t2 = toks[i]

              if t2.str in ":;,{}()[]" \
              or CToken.EXTENSION == t2.kind \
              or CToken.EXTENSION_NO_PARAM == t2.kind:
                i -= 1
                break

          # this is the type if what we are defining
          t.kind = CToken.TYPE_USER
          ctype = found_ctype
          might_have_type = True

        # we've seen an identifier before, e.g. a type name;
        # this is almost certainly the defined type
        elif ctype or might_have_type:
          i -= 1
          break

        # break the standard here; let's assume that this type
        # has yet to be defined, but WILL be defined, and we will
        # define it, on our own for now.
        else:
          t.kind = CToken.TYPE_USER
          ctype = CTypeDefinition(t.str, CParser.T_I, is_missing=True)
          stab.set_type(t.str, ctype)

      # qualifiers (const, volatile, restrict)
      elif hasattr(attrs, "is_" + t.str):
        setattr(attrs, "is_" + t.str, True)

      # visibility qualifiers (extern, inline, static)
      elif hasattr(name_attrs, "is_" + t.str):
        setattr(name_attrs, "is_" + t.str, True)

      # alternate name for some specifiers/qualifiers
      elif t.str in CParser.SPECIFIER_MAP:
        spec = CParser.SPECIFIER_MAP[t.str]
        attr_name = "is_" + spec
        if hasattr(attrs, attr_name):
          setattr(attrs, attr_name, True)
        elif hasattr(name_attrs, attr_name):
          setattr(name_attrs, attr_name, True)
        else:
          assert False

      # look for compound types first, and hopefully infer type names.
      elif t.str in CParser.COMPOUND_TYPE:
        assert i < len(toks)
        might_have_type = True
        t2 = toks[i]
        t3 = None

        if (i + 1) < len(toks): # might not be if we're in a param list
          t3 = toks[i + 1]

        constructor = CParser.COMPOUND_TYPE[t.str]
        parser = CParser.COMPOUND_TYPE_PARSER[t.str]

        # type definition
        if t2.kind == CToken.LITERAL_IDENTIFIER and t3 and "{" == t3.str:
          t2.kind = CToken.TYPE_USER
          
          # get prev. instance from forward declaration
          if stab.has_type(t2.str, constructor):
            ctype = stab.get_type(t2.str, constructor)
          else:
            ctype = constructor(t2.str) # define the type

          stab.set_type(t2.str, ctype)
          inner_stab = CParser.COMPOUND_TYPE_STAB[t.str](self, stab)
          i = parser(self, inner_stab, ctype, toks, i + 1)

        # cheat: user-defined type use
        elif t2.kind in (CToken.LITERAL_IDENTIFIER, CToken.TYPE_USER):
          t2.kind = CToken.TYPE_USER
          i += 1

          # get prev. instance from forward declaration
          if stab.has_type(t2.str, constructor):
            ctype = stab.get_type(t2.str, constructor)
          else:
            ctype = constructor(t2.str) # define the type

        # anonymous type definition
        elif "{" == t2.str:
          ctype = constructor()
          inner_stab = CParser.COMPOUND_TYPE_STAB[t.str](self, stab)
          i = parser(self, inner_stab, ctype, toks, i)

      # typeof expression; todo: not handling all typeof cases (i.e. ones
      # without parentheses)
      elif CToken.TYPEOF == t.kind:
        might_have_type = True
        typeof_toks = []
        i = self._get_up_to_balanced(toks, typeof_toks, i - 1, "(", include=True)
        expr_or_type = self._parse_expression_or_cast(stab, typeof_toks[1:], 0)
        if isinstance(expr_or_type, CType):
          ctype = expr_or_type
        else:
          # todo: unimplemented
          ctype = CTypeExpression(typeof_toks)

      # compiler-specific attributes (parameterized)
      elif CToken.EXTENSION == t.kind:
        extension_toks = []
        i = self._get_up_to_balanced(toks, extension_toks, i - 1, "(", include=True)
        name_attrs.attrs[name_attrs.LEFT].extend(extension_toks)

      # compiler-specific attributes (non-parameterized)
      elif CToken.EXTENSION_NO_PARAM == t.kind:
        name_attrs.attrs[name_attrs.LEFT].append(t)

      # probably done the specifiers :-P
      else:
        i -= 1
        break

    if not built_in_names and (attrs.is_signed or attrs.is_unsigned):
      built_in_names.append("int")

    if built_in_names:
      ctype = CParser.BUILT_IN_TYPES[tuple(sorted(built_in_names))]

    #if not ctype:
    #  print toks[i].str, toks[i].kind, stab.has_type(toks[i].str, CTypeDefinition)
    #  print carat.line, carat.column

    if not attrs.has_default_attrs():
      ctype = CTypeAttributed(ctype, attrs)

    assert ctype
    return i, ctype, defines_type

  # Parse an individual declarator. A declarator is the thing being declared,
  # e.g. a variable or a type. Declarators contain some type information that
  # must be later combined with the specifiers to form a complete type. For
  # example, "int foo[10];", the "int" is a specifier, and "foo[10]" is a
  # declarator; however, the type of "foo" is "int[10]".
  #
  # Args:
  #   stab:             Symbol table in which to place newly defined names.
  #   ctype:            CType instance representing the base type of whatever is
  #                     being declared.
  #   name_attrs:       Attributes (visibility/extensions) applied to this
  #                     named declarator.
  #   toks:             List of tokens of a declaration statement.
  #   i:                Cursor into `toks`.
  #   end_on_comma:     Should the declarator end on a comma? (YES: but, this
  #                     affects whether or not the declarator consumes the
  #                     comma or if a higher-level function consumes it.)
  #
  # Returns:
  #   A tuple `(i, ctype, name_attrs, name)` where `i` is the next cursor into 
  #   `toks`, `ctype` is the type of the name `name` in the declaration, and
  #   `name_attrs` are the visibility and extension attributes that apply to
  #   `name`.
  def _parse_declarator(self, stab, ctype, name_attrs, toks, i, end_on_comma=False):
    i, ctype = self._parse_pointers(ctype, toks, i)
    sub_decl_toks, call_recursive = [], False
    name = None

    if i >= len(toks): # e.g. unnamed function argument
      return i, ctype, name_attrs, name

    # go find the left corner of this declarator's direct declarator.
    t = toks[i]
    if CToken.LITERAL_IDENTIFIER == t.kind:
      i += 1
      name = t.str
    elif "(" == t.str:
      i = self._get_up_to_balanced(toks, sub_decl_toks, i, t.str)
      call_recursive = True

    while i < len(toks):
      t = toks[i]

      # cheat: these should end the declarator
      if t.str in ")]};":
        break

      # separates two declarators
      elif "," == t.str:
        if not end_on_comma:
          i += 1
        break

      # array specifiers; cheat: we will parse this *very* loosely, i.e. not
      # quite conforming to the standing, but should be roughly capturing most
      # things
      elif "[" == t.str:
        expr_toks = []
        i = self._get_up_to_balanced(toks, expr_toks, i, t.str)
        ctype = self._parse_array_direct_declarator(stab, ctype, expr_toks)

      # function parameter list
      elif "(" == t.str:
        expr_toks = []
        i = self._get_up_to_balanced(toks, expr_toks, i, t.str)
        ctype = self._parse_param_list(stab, ctype, expr_toks)

      # compiler-specific extensions (parameterized)
      elif CToken.EXTENSION == t.kind:
        extension_toks = []
        i = self._get_up_to_balanced(toks, extension_toks, i, "(", include=True)
        name_attrs.attrs[name_attrs.RIGHT].extend(extension_toks)

      # compiler-specific extensions (non-parameterized)
      elif CToken.EXTENSION_NO_PARAM == t.kind: 
        name_attrs.attrs[name_attrs.RIGHT].append(t)

      # cheat: assume we're in a struct or a union
      elif ":" == t.str:
        i, expr = self._parse_expression(stab, toks, i + 1, can_have_comma=False)
        ctype = CTypeBitfield(ctype, expr)

      else:
        #print repr(t.str), t.carat.line, t.carat.column, ctype
        assert False

    if call_recursive:
      _, ctype, name_attrs, name = self._parse_declarator(
          stab, ctype, name_attrs, sub_decl_toks, 0)

    return (i, ctype, name_attrs, name)


  # Parse the pointers of a declarator. Parsing pointers is interesting because
  # later specifiers and pointers apply to the type on the left.
  #
  # Args:
  #   base:             CType instance representing the type that will be
  #                     pointed to.
  #   toks:             List of tokens to parse.
  #   i:                Cursor into `toks`.
  #
  # Returns:
  #   A tuple `(i, ctype)` where `i` is a new cursor pointing into `toks`
  #   such that the cursor position points to the token immediately following
  #   the last component of the pointer declarators; `ctype` is potentially
  #   a wrapped version of `base`.
  def _parse_pointers(self, base, toks, i):
    assert base
    while i < len(toks):
      t = toks[i]
      if "*" == t.str:
        base = CTypePointer(base)

      # qualifiers: const, restrict, volatile
      elif hasattr(base, "is_" + t.str):
        assert isinstance(base, CTypePointer)
        attr_name = "is_" + t.str
        assert not getattr(base, attr_name)
        setattr(base, attr_name, True)

      # alternate named qualifiers
      elif t.str in CParser.SPECIFIER_MAP:
        spec = CParser.SPECIFIER_MAP[t.str]
        attr_name = "is_" + spec
        if not hasattr(base, attr_name):
          break

        assert isinstance(base, CTypePointer)
        assert not getattr(base, attr_name)
        setattr(base, attr_name, True)

      else:
        break
      i += 1

    return (i, base)

  # Parse a parameter list (list of names, list of types, or list of
  # declarations).
  def _parse_param_list(self, stab, ctype, toks):
    ctype = CTypeFunction(ctype)
    i = 0

    if not toks:
      ctype.is_variadic = True
      ctype.is_old_style_variadic = True

    while i < len(toks):
      t = toks[i]
      if "..." == t.str:
        ctype.is_variadic = True
        break

      i, decls, is_typedef = self._parse_declaration(
          self.stab, toks, i, end_on_comma=True)

      assert not is_typedef
      assert 1 == len(decls)
      param_ctype, param_name = decls[0]

      ctype.param_types.append(param_ctype)
      ctype.param_names.append(param_name)

    # handle the case where a function takes no arguments and is not old-style
    # variadic.
    if 1 == len(ctype.param_types):
      param_type = ctype.param_types[0]
      while isinstance(param_type, CTypeAttributed):
        param_type = param_type.ctype

      if param_type is CParser.T_V:
        ctype.param_types = []
        ctype.param_names = []

    return ctype

  # Parse the array size component of an array direct declarator. This does a
  # pseudo parse insofar as the correct syntax is not strictly checked; instead
  # a few things are recognized, and the rest is lumped in as being a sequence
  # of tokens (presumably representing either a dependent expression or a 
  # constant expression) that specifies the size of one of the dimensions of the
  # array.
  #
  # Args:
  #   stab:         The symbol table from which symbols can be found.
  #   ctype:        The type to which this array direct declarator will apply.
  #   expr_toks:    List of tokens within the "[" and "]" for this direct array
  #                 declarator.
  #
  # Returns:
  #   A CTypeArray instance.
  def _parse_array_direct_declarator(self, stab, ctype, expr_toks):
    i = 0
    ctype = CTypeArray(ctype)
    while i < len(expr_toks):
      t = expr_toks[i]
      if "*" == t.str:
        ctype.is_vla = True
      elif "static" == t.str:
        ctype.size_is_lower_bound = True
      elif "volatile" == t.str:
        ctype.is_volatile = True
      elif "const" == t.str:
        ctype.is_const = True
      else:
        ctype.size_expr_toks.append(t)
      i += 1
    return ctype

  # Parse an expression or a type cast.
  #
  # Args:
  #   stab:             Symbol table in which names are looked up.
  #   toks:             List of tokens to parse. It is assumed that these tokens
  #                     are exactly those contained between two parentheses,
  #                     and so form either a type name or an expression.
  #   i:                Cursor into `toks`.
  #
  # Returns:
  #   A CType instance if this is a type case, or a CExpression instance if this
  #   is a sub-expression.
  def _parse_expression_or_cast(self, stab, toks, i):

    t = toks[i]

    # left corner of a cast or a sub-expression might be an identifier that
    # might be an actual typedef name
    if CToken.LITERAL_IDENTIFIER == t.kind:
      if stab.has_type(t.str, CTypeDefinition):
        t.kind = CToken.TYPE_USER

    # this is a type cast
    if CToken.TYPE_SPECIFIER == t.kind \
    or CToken.TYPE_QUALIFIER == t.kind \
    or CToken.TYPE_BUILT_IN == t.kind \
    or CToken.TYPE_USER == t.kind \
    or CToken.SPECIFIER_STORAGE == t.kind \
    or CToken.SPECIFIER_FUNCTION == t.kind:
      name_attrs = CTypeNameAttributes()
      i, ctype, defines_type = self._parse_specifiers(
          stab, name_attrs, toks, 0)
      assert not defines_type
      i, ctype, name_attrs, name = self._parse_declarator(
          stab, ctype, name_attrs, toks, i)
      assert not name
      return ctype

    # this is a nested expression
    else:
      i, expr = self._parse_expression(stab, toks, 0, can_have_comma=True)
      return expr

  # Parse an expression.
  #
  # Args:
  #   stab:         The symbol table from which symbols can be found.
  #   toks:         List of tokens containing the expression to parse.
  #   i:            Cursor into the list of tokens, `toks`.
  #
  # Returns:
  #   A tuple `(i, expr)` where
  def _parse_expression(self, stab, toks, i, can_have_comma):
    expr_toks = []
    while i < len(toks):
      t = toks[i]
      if t.str in ")]};":
        break
      
      if not can_have_comma and "," == t.str:
        break

      if t.str in "([{":
        i = self._get_up_to_balanced(toks, expr_toks, i, t.str)
      else:
        i += 1

    # todo: actually parse expressions
    return i, expr_toks

  # Extract a sub-list of tokens (`sub_toks`) from `toks` such that 
  # `sub_toks` is everything contained between balanced `open_str` and the
  # associated closing string.
  #
  # Args:
  #   toks:         List of tokens from which we are extracting a sub-list.
  #   sub_toks:     List in which to place extracted tokens from `toks`.
  #   i:            Cursor position in `toks`.
  #   open_str:     The "opening" bracket type; the associated closing string
  #                 is found in CParser.CLOSING.
  #   include:      Optionally include the open/closing strings, and anything
  #                 that comes before the first opening string.
  #
  # Returns:
  #   A cursor position into `toks` that immediately follows the first balanced
  #   closing string associated with `open_str`.
  def _get_up_to_balanced(self, toks, sub_toks, i, open_str, include=False):
    close_str = CParser.CLOSING[open_str]
    num = 1
    while toks[i].str != open_str:
      if include:
        sub_toks.append(toks[i])
      i += 1

    if include:
      sub_toks.append(toks[i])
    i += 1

    while i < len(toks):
      t = toks[i]
      i += 1

      if open_str == t.str:
        num += 1
      elif close_str == t.str:
        num -= 1
      
      if not num:
        if include:
          sub_toks.append(t)
        break
      else:
        sub_toks.append(t)

    return i

  # Parse a C file.
  #
  # Returns:
  #   A list of C declarations (AST nodes).
  def parse(self, toks):
    decls = []
    toks = list(toks)
    i = 0
    while i < len(toks):
      carat = toks[i].carat
      i, decls, is_typedef = self._parse_declaration(self.stab, toks, i)
      
      for ctype, name in decls:
        if is_typedef:
          if not name:
            print carat.line, carat.column
          assert name
          ctype = CTypeDefinition(name, ctype)
          self.stab.set_type(name, ctype)
        else:
          if name: 
            self.stab.set_var(name, ctype)

  # Generate pairs of variables/functions and their types at the global scope.
  def vars(self):
    return self.stab.vars()


# for testing
if "__main__" == __name__:
  import sys
  with open(sys.argv[1]) as lines_:
    buff = "".join(lines_)
    tokens = CTokenizer(buff)
    parser = CParser()
    parser.parse(tokens)
    for var, ctype in parser.vars():
      print var, ctype

