import sys
import re

NL = re.compile(r"@N@")
PASTE = re.compile(r"(.*?)\s*##\s*(.*?)")

# open up a post-processed assembly file and replace each @N@ with a new line
# and if the preprocessor didn't do token pasting, then automatically perform
# it.
with open(sys.argv[1]) as lines:
  for line in lines:
    line = NL.sub("\n", line.rstrip(" \t\r\n")).rstrip("\n")
    while True:
      old_line = line
      line = PASTE.sub(r"\1\2", line)
      if old_line == line:
        break
    print line