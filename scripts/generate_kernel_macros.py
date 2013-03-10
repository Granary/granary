
from subprocess import call

if "__main__" == __name__:
  call("cd granary/kernel/linux/macros/ ; make macros", shell=True)
