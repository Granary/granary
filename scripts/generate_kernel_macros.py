"""Invoke a makefile for a Linux kernel module that will purposefully
fail to compile. This is really ugly and there is certainly a better
way to do this.

The purpose of this is that in failing to make the module, the kernel's
configuration (in the form of macro #defines) will be dumped to an output
file, and then copied into granary/gen/kernel_macros.h."""

import os
from subprocess import call

if "__main__" == __name__:
  with open(os.devnull, "w") as fnull:
    call(
        "cd granary/kernel/linux/macros/ ; make macros", 
        stdout=fnull,
        stderr=fnull,
        shell=True)
