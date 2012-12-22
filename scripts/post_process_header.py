
import sys
import re

def O(*args):
  line = "".join(map(str, args))
  if "^" not in line:
    print line

NON_BRACES = re.compile(r"[^{}]")

def get_lines():
  """Get the lines of a file as a list of strings such that no line has
  more than one brace, and that every line is C/C++ and not pre-processor
  line directives."""
  all_lines = []

  # remove empty lines and new lines
  with open(sys.argv[1]) as lines:
    for line in lines:
      strip_line = line.strip(" \n\r\t")
      if not strip_line:
        continue
      
      # get rid of pre-processor line numbers
      if strip_line.startswith("#"):
        continue

      all_lines.append(strip_line + " ")

  # inject new lines in a structured manner
  buff = "".join(all_lines)
  buff = buff.replace("{", "{\n")
  buff = buff.replace("}", "}\n")
  buff = buff.replace(";", ";\n")
  buff = buff.replace("}\n;", "};\n")
  buff = buff.replace("extern", "\nextern")
  buff = buff.replace("namespace", "\nnamespace")
  buff = buff.replace("template", "\ntemplate")
  buff = buff.replace("typedef", "\ntypedef")

  # now there is only one brace ({ or }) per line.
  return buff.split("\n")

def match_next_brace_group(lines, i, internal_lines):
  """Try to determine the line index (i) of the line that ends a brace
  group. E.g. if a function is defined, """

  # skip lines until we find out first brace
  while i < len(lines):
    if "{" in lines[i]:
      break
    i += 1

  num = 0
  done, seen_first_line = False, False
  while i < len(lines):
    only_braces = NON_BRACES.sub("", lines[i])
    for b in only_braces:
      if "{" == b:
        num += 1
      else:
        num -= 1
      if not num:
        done = True
        break
    if done:
      return i

    if seen_first_line:
      internal_lines.append(lines[i])
    seen_first_line = True
    
    i += 1
  
  return len(lines)

def process_lines(lines):
  i = -1
  if not lines:
    return

  while True:
    i += 1
    if i >= len(lines):
      return

    line = lines[i]
    strip_line = line.strip(" \n\r\t")
    if not strip_line:
      continue

    if strip_line.startswith("using"):
      continue

    if strip_line.startswith("return"):
      continue

    if "$" in strip_line:
      continue

    if strip_line.startswith("namespace") \
    or strip_line.startswith("template") \
    or "inline" in strip_line:
      ignore = []
      i = match_next_brace_group(lines, i, ignore)
      continue

    if strip_line.startswith("extern"):
      code_lines = []
      i = match_next_brace_group(lines, i, code_lines)
      process_lines(code_lines)
      continue

    O(strip_line)

process_lines(get_lines())
