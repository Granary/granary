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
  TYPE_USER                 = 21 # determined during parsing
  TYPE_QUALIFIER            = 22
  TYPE_SPECIFIER            = 23 # e.g. struct, union, enum
  SPECIFIER_STORAGE         = 24
  SPECIFIER_FUNCTION        = 25

  ELLIPSIS                  = 31

  STATEMENT_BEGIN           = 40
  STATEMENT_END             = 41

  EXTENSION                 = 50
  TYPEOF                    = 51

  EOF                       = 60

  # mapping of reserved words to token kinds
  RESERVED = {
    "auto":           SPECIFIER_STORAGE,
    "break":          STATEMENT_BEGIN,
    "case":           STATEMENT_BEGIN,
    "char":           TYPE_BUILT_IN,
    "const":          TYPE_QUALIFIER,
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
    "int":            TYPE_BUILT_IN,
    "long":           TYPE_BUILT_IN,
    "register":       SPECIFIER_STORAGE,
    "restrict":       TYPE_QUALIFIER,
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
    "wchar_t":        TYPE_BUILT_IN,

    # todo: check if these are okay
    #"ptrdiff_t":      TYPE_BUILT_IN,
    #"size_t":         TYPE_BUILT_IN,
    #"ssize_t":        TYPE_BUILT_IN,
    
    # extensions    
    "asm":            EXTENSION,
    "_asm":           EXTENSION,
    "__asm":          EXTENSION,
    "asm_":           EXTENSION,
    "asm__":          EXTENSION,
    "__asm__":        EXTENSION,
        
    "_volatile":      EXTENSION,
    "__volatile":     EXTENSION,
    "volatile_":      EXTENSION,
    "volatile__":     EXTENSION,
    "__volatile__":   EXTENSION,
    
    "__attribute__":  EXTENSION,
    
    "typeof":         TYPEOF,
    "__typeof__":     TYPEOF,
    "decltype":       TYPEOF,

    # os/compiler-specific extensions
    "__builtin_va_list":    TYPE_BUILT_IN,
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

  # Initialize the CCharacterReader.
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
      print idx, self.last_idx, diff
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
    self.buff = CCharacterReader(buff)
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
          tok.str += " " + next_tok.str
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
  pass


class CTypeCompound(CType):
  """Base class for user-defined compound types."""
  pass


class CTypeStruct(CTypeCompound):
  """Represents a structure type."""

  __slots__ = ('_id', 'name')
  ID = 0

  def __init__(self, name_=None):
    self._id = CTypeStruct.ID
    CTypeStruct.ID += 1
    if not name_:
      name_ = "anon_struct_%d" % self._id

    self.name = name_

  def __repr__(self):
    return "Struct(%s)" % self.name


class CTypeUnion(CTypeStruct):
  """Represents a union type."""

  __slots__ = ('_id', 'name')
  ID = 0
  
  def __init__(self, name_=None):
    self._id = CTypeUnion.ID
    CTypeUnion.ID += 1
    if not name_:
      name_ = "anon_union_%d" % self._id

    self.name = name_

  def __repr__(self):
    return "Union(%s)" % self.name


class CTypeEnum(CTypeCompound):
  """Represents an enumeration type."""

  __slots__ = ('_id', 'name')
  ID = 0

  def __init__(self, name_=None):
    self._id = CTypeEnum.ID
    CTypeEnum.ID += 1
    if not name_:
      name_ = "anon_enum_%d" % self._id

    self.name = name_


class CTypeBuiltIn(CType):
  """Represents a built-in type."""

  __slots__ = ('name',)

  def __init__(self, name_):
    self.name = name_

  def __repr__(self):
    return self.name


class CTypeDefinition(CType):
  """Represents a `typedef`d type name."""

  __slots__ = ('name', 'ctype')
  
  def __init__(self, name_, ctype_):
    self.name = name_
    self.ctype = ctype_


class CTypePointer(object):
  """Represents a C pointer type.
  
  This tracks a single indirection (*) and the flags following that
  indirection. """

  __slots__ = ('ctype', 'is_const', 'is_restrict', 'is_volatile')

  def __init__(self, ctype_):
    self.ctype = ctype_
    self.is_const = False
    self.is_restrict = False
    self.is_volatile = False

  def __repr__(self):
    return "TODO ptr"


class CTypeArray(CTypePointer):
  """Represents an array type in C.

  Some aspects of C99's parameterized (with qualifiers, dependent names, etc.)
  declarator features are incorporated."""

  __slots__ = ('is_vla', 'is_const', 'is_restrict', 'size_is_lower_bound',
               'size_expr_toks', 'direct_decl', 'is_dependent')

  def __init__(self, ctype_):

    # left-hand side of this array direct declarator
    self.ctype = ctype_
    self.is_dependent = False
    
    # true iff either an expression or a "*" is given as the size of the
    # array.
    self.is_vla = False

    # used for determining the decayed pointer type of this array.
    self.is_const = False
    self.is_restrict = False

    # e.g. if static is used, then this declares that the array is non-null
    #      and its size can be used as a lower bound
    self.size_is_lower_bound = False
    self.size_expr_toks = [] # some list of tokens for later evaluation


class CTypeAttributes(object):
  """Represents attributes applied to a type."""

  __slots__ = ('is_const', 'is_register', 'is_auto', 'is_volatile',
               'is_inline', 'is_extern', 'is_restrict', 'is_signed',
               'is_unsigned', 'gnuc_attrs')

  def __init__(self):
    self.is_const = False
    self.is_register = False
    self.is_auto = False
    self.is_volatile = False
    self.is_inline = False
    self.is_extern = False
    self.is_restrict = False
    self.is_signed = False
    self.is_unsigned = False
    self.gnuc_attrs = False


class CTypeAttributed(CType):
  """Represents an attributed type (i.e. a type with attributes)."""

  def __init__(self, ctype_, attrs_):
    self.ctype = ctype_
    self.attrs = attrs_


class CTypeExpression(CType):
  """Represents a type that is computed by some expression e.g.: typeof (...)."""

  def __init__(self, toks_):
    self.toks = toks_


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
    assert item not in self._dict
    self._dict[item] = val


class CTypeFunction(CTypeCompound):
  """Represents a function type."""

  def __init__(self, ret_type_):
    self.ret_type = ret_type_
    self.param_types = []
    self.param_names = []
    self.is_variadic = False # e.g. old style, or using ...
    self.is_old_style_variadic = False # old style, e.g. int foo();


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
    parent_vars, parent_types = None, None
    if parent_:
      parent_vars = parent_.vars
      parent_types = parent_.types

    self.vars = CStackedDict(parent_vars)
    self.types = {
      CTypeStruct:      CStackedDict(parent_types),
      CTypeUnion:       CStackedDict(parent_types),
      CTypeEnum:        CStackedDict(parent_types),
      CTypeDefinition:  CStackedDict(parent_types)
    }

  def has_var(self, name):
    return name in self.vars

  def has_type(self, name, const):
    assert const in self.types
    return name in self.types[const]

  def set_var(self, name, info):
    try:
      self.vars[name] = info
    except:
      pass # assume we are re-declaring the same thing.

  def set_type(self, name, ctype):
    assert ctype.__class__ in self.types
    self.types[ctype.__class__][name] = ctype

  def get_var(self, name):
    return self.vars[name]

  # Get a type by its name and it's type constructor
  def get_type(self, name, const):
    assert const in self.types
    assert name in self.types[const]
    return self.types[const][name]


class CParser(object):
  """Parse a token stream of something like C99 and generate an AST."""

  # Different (token) kinds of declaration specifiers.
  DECL_SPECIFIERS = set([
    CToken.TYPE_SPECIFIER,
    CToken.TYPE_QUALIFIER,
    CToken.SPECIFIER_STORAGE,
    CToken.SPECIFIER_FUNCTION,
    CToken.TYPE_BUILT_IN,
    CToken.TYPE_USER,
  ])

  # Constructors for various different compound types.
  COMPOUND_TYPE = {
    "union":  CTypeUnion, 
    "enum":   CTypeEnum,
    "struct": CTypeStruct,
  }

  # Constructor for making/choosing a symbol table to use when parsing the body
  # of a compound type.
  COMPOUND_TYPE_STAB = {
    "union":  lambda s: CSymbolTable(s), 
    "enum":   lambda s: s, # enum places its names into the enclosing symbol table
    "struct": lambda s: CSymbolTable(s),
  }

  # Built-in types.
  T_V   = CTypeBuiltIn("void")
  T_C   = CTypeBuiltIn("char")
  T_UC  = CTypeBuiltIn("unsigned char")
  T_S   = CTypeBuiltIn("short")
  T_US  = CTypeBuiltIn("unsigned short")
  T_I   = CTypeBuiltIn("int")
  T_UI  = CTypeBuiltIn("unsigned")
  T_L   = CTypeBuiltIn("long")
  T_UL  = CTypeBuiltIn("unsigned long")
  T_LL  = CTypeBuiltIn("long long")
  T_ULL = CTypeBuiltIn("unsigned long long")
  T_F   = CTypeBuiltIn("float")
  T_D   = CTypeBuiltIn("double")
  T_DL  = CTypeBuiltIn("long double")
  T_WC  = CTypeBuiltIn("wchar_t")

  #T_S   = CTypeBuiltIn("size_t")
  #T_SS  = CTypeBuiltIn("ssize_t")
  #T_PD  = CTypeBuiltIn("ptrdiff_t")

  VA_LIST = CTypeBuiltIn("__builtin_va_list")

  # Mapping of (sorted) token string sequences to built-in type objects.
  BUILT_IN_TYPES = {
    ("__builtin_va_list",):           VA_LIST,

    ("void",):                        T_V,

    ("char",):                        T_C,
    ("char", "unsigned"):             T_UC,
    ("char", "signed"):               T_C,

    ("short",):                       T_S,
    ("short", "unsigned"):            T_US,
    ("short", "signed"):              T_S,

    ("signed",):                      T_I,
    ("int",):                         T_I,
    ("auto",):                        T_I,
    ("register",):                    T_I,
    ("int", "signed"):                T_I,
    ("unsigned",):                    T_UI,
    ("int", "unsigned"):              T_UI,

    ("int", "long"):                  T_L,
    ("int", "long", "signed"):        T_L,
    ("int", "long", "unsigned"):      T_UL,
    ("long",):                        T_L,
    ("long", "signed"):               T_L,
    ("long", "unsigned"):             T_UL,

    ("int", "long", "long"):          T_LL,
    ("int", "long", "long", "signed"):T_LL,
    ("int", "long", "long", "unsigned"):T_ULL,
    ("long", "long"):                 T_LL,
    ("long", "long", "signed"):       T_LL,
    ("long", "long", "unsigned"):     T_ULL,

    ("float",):                       T_F,
    ("double",):                      T_D,
    ("double", "long"):               T_DL,

    ("wchar_t",):                     T_WC,

    #("size_t",):                      T_S,
    #("ssize_t",):                     T_SS,
    #("ptrdiff_t",):                   T_PD,
  }

  # Mapping of opening brace/bracket/paren chars to their closing chars.
  CLOSING = {
    "(": ")",
    "[": "]",
    "{": "}",
    "<": ">",
  }

  # Initialize the parser.
  def __init__(self):
    self.stab = CSymbolTable()

  # Parse a union, struct, or enum, where i is the position of the opening {
  # of the compound type definition.
  #
  # Args:
  #   stab:           Symbol table in which to place new names.
  #   ctype:          The type object representing the type that we are currently
  #                   parsing.
  #   outer_toks:     The token stream from which we will extract only those
  #                   tokens (`toks`) belonging to this compound type definition.
  #   j:              Our current cursor position in the `outer_toks`.
  #
  # Returns:
  #   The position in toks to continue parsing (immediately following the
  #   closing } of the compound type definition).
  def _parse_compound_type(self, stab, ctype, outer_toks, j):
    toks = []
    assert "{" == outer_toks[j].str
    j = self._get_up_to_balanced(outer_toks, toks, j, "{")
    return j

  # Parse a single declaration.
  #
  # Args:
  #   stab:           Symbol table in which to place new names.
  #   toks:           All tokens within a single declaration (and no more).
  #   i:              A cursor into `toks`.
  #
  # Returns:
  #   A cursor on the token in `toks` immediately following the last token of
  #   this declaration.
  def _parse_declaration(self, stab, toks, i):
    user_type = False
    built_in_type = False

    specs = []
    decls = []
    
    i, specs_ctype, defines_type = self._parse_specifiers(stab, toks, i)

    # parse the declarators until we make no progress
    while i < len(toks):
      t = toks[i]
      if t.str in ";)]}":
        i += 1
        break

      i, ctype, name = self._parse_declarator(stab, specs_ctype, toks, i)
      decls.append((ctype, name))

    # if this was a typedef then define some type(s).
    if defines_type:
      for (ctype, name) in decls:
        assert name
        ctype = CTypeDefinition(name, ctype)
        stab.set_type(name, ctype)
    else:
      for (ctype, name) in decls:
        if ctype and name:
          stab.set_var(name, ctype)

    print decls
    return i

  # Parse a list of specifiers.
  #
  # Args:
  #   stab:             Symbol table in which names will be found/placed.
  #   toks:             List of tokens.
  #   i:                Cursor into `toks`.
  #
  # Returns:
  #   A tuple `(i, ctype, defines_type)` representing the next cursor position
  #   `i` into `toks` and the `ctype` representing the type speficiers. If
  #   `defines_type` is True then we will be defining some type names.
  def _parse_specifiers(self, stab, toks, i):
    attrs = CTypeAttributes()
    built_in_names = []
    ctype = None
    defines_type = False

    while i < len(toks):
      t = toks[i]
      i += 1
      
      if "typedef" == t.str:
        defines_type = True

      # special case 1: these can either set the type, or modify it.
      elif "signed" == t.str:
        assert not attrs.is_unsigned
        assert not attrs.is_signed
        attrs.is_signed = True
        built_in_names.append(t.str)

      elif "unsigned" == t.str:
        assert not attrs.is_unsigned
        assert not attrs.is_signed
        attrs.is_unsigned = True
        built_in_names.append(t.str)

      # special case 2: both of these storage class specifiers can also be
      # used to define ints.
      elif "register" == t.str:
        assert not attrs.is_register
        attrs.is_register = True
        ctype = CParser.BUILT_IN_TYPES[t.str,]

      elif "auto" == t.str:
        assert not attrs.is_auto
        attrs.is_auto = True
        ctype = CParser.BUILT_IN_TYPES[t.str,]

      # normal built-in type
      elif CToken.TYPE_BUILT_IN == t.kind:
        built_in_names.append(t.str)

      # typedef name
      elif CToken.TYPE_USER == t.kind:
        ctype = stab.get_type(t.str, CTypeDefinition)
      
      # possible typedef name
      elif CToken.LITERAL_IDENTIFIER == t.kind:
        if stab.has_type(t.str, CTypeDefinition):
          t.kind = CToken.TYPE_USER
          ctype = stab.get_type(t.str, CTypeDefinition)
        else:
          i -= 1
          break

      # various qualifiers
      elif "const" == t.str:
        attrs.is_const = True
      elif "volatile" == t.str:
        attrs.is_volatile = True
      elif "extern" == t.str:
        attrs.is_extern = True
      elif "inline" == t.str:
        attrs.is_inline = True
      elif "restrict" == t.str:
        attrs.is_restrict = True

      # look for compound types first, and hopefully infer type names.
      elif t.str in CParser.COMPOUND_TYPE:
        assert (i + 1) < len(toks)
        t2 = toks[i]
        t3 = toks[i + 1]
        constructor = CParser.COMPOUND_TYPE[t.str]

        # type definition
        if t2.kind == CToken.LITERAL_IDENTIFIER and "{" == t3.str:
          t2.kind = CToken.TYPE_USER
          
          # get prev. instance from forward declaration
          if stab.has_type(t2.str, constructor):
            ctype = stab.get_type(t2.str, constructor)
          else:
            ctype = constructor(t2.str) # define the type

          stab.set_type(t2.str, ctype)
          inner_stab = CParser.COMPOUND_TYPE_STAB[t.str](stab)
          i = self._parse_compound_type(inner_stab, ctype, toks, i + 1)

        # cheat: user-defined type use
        elif t2.kind == CToken.LITERAL_IDENTIFIER:
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
          inner_stab = CParser.COMPOUND_TYPE_STAB[t.str](stab)
          i = self._parse_compound_type(inner_stab, ctype, toks, i)

      # typeof expression
      elif CToken.TYPEOF == t.kind:
        typeof_toks = []
        i = self._get_up_to_balanced(toks, typeof_toks, i, "(")
        ctype = CTypeExpression(typeof_toks)

      else:
        i -= 1
        break

    if built_in_names:
      ctype = CParser.BUILT_IN_TYPES[tuple(sorted(built_in_names))]
      
    return i, CTypeAttributed(ctype, attrs), defines_type

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
  #   toks:             List of tokens of a declaration statement.
  #   i:                Cursor into `toks`.
  #
  # Returns:
  #   A tuple `(i, ctype, name)` where `i` is the next cursor into `toks`,
  #   and `ctype` is the type of the name `name` in the declaration.
  def _parse_declarator(self, stab, ctype, toks, i):
    i, ctype = self._parse_pointers(ctype, toks, i)
    sub_decl_toks, call_recursive = [], False
    name = None

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
      elif "," == t.str:
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

      elif CToken.EXTENSION == t.kind:
        extension_toks = []
        i = self._get_up_to_balanced(toks, extension_toks, i, "(")
        print extension_toks

      else:
        print repr(t.str), t.carat.line, t.carat.column
        assert False

    if call_recursive:
      _, ctype, name = self._parse_declarator(stab, ctype, sub_decl_toks, 0)

    return (i, ctype, name)


  # Parse the pointers of a declarator. Parsing pointers is interesting because
  # there are essentially two cases: pointers to declaration specifiers, and
  # a sort of "pass-through" where the pointers apply to what's around them. The
  # implication is that if there is a direct declarator with a nested declarator
  # then we must first compute the type of the direct declarator, and pass it in
  # as the base ctype to which we might be pointing.
  #
  # Example:
  #   case 1:           "int *foo", the specifiers is "int" and the base `ctype`
  #                     will be "int, so the result is an "int *".
  #   case 2:           "int (*foo)(void)", the specifiers is "int", but because
  #                     "*foo" is a parenthesized declarator, first we must
  #                     compute the outer type, "int (void)", and pass it in to
  #                     the declarator, to get the final "int (*)(void)" type.
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
      elif "const" == t.str:
        assert isinstance(base, CTypePointer)
        assert not base.is_const
        base.is_const = True
      elif "restrict" == t.str:
        assert isinstance(base, CTypePointer)
        assert not base.is_restrict
        base.is_restrict = True
      elif "volatile" == t.str:
        assert isinstance(base, CTypePointer)
        assert not base.is_volatile
        base.is_volatile = True
      else:
        break
      i += 1

    return (i, base)

  # Parse a parameter list (list of names, list of types, or list of
  # declarations).
  def _parse_param_list(self, stab, ctype, toks):
    ctype = CTypeFunction(ctype)
    # todo
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
  #
  # Returns:
  #   A cursor position into `toks` that immediately follows the first balanced
  #   closing string associated with `open_str`.
  def _get_up_to_balanced(self, toks, sub_toks, i, open_str):
    close_str = CParser.CLOSING[open_str]
    num = 1
    while toks[i].str != open_str:
      i += 1
    i += 1

    while i < len(toks):
      t = toks[i]
      i += 1

      if open_str == t.str:
        num += 1
      elif close_str == t.str:
        num -= 1
      
      if not num:
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
      i = self._parse_declaration(self.stab, toks, i)

    # todo: return list of declarations.

if "__main__" == __name__:
  import sys
  with open(sys.argv[1]) as lines_:
    buff = "".join(lines_)
    tokens = CTokenizer(buff)
    parser = CParser()
    parser.parse(tokens)
    


