Granary
=======

Note: you can substitute gcc/g++ for clang/clang++; however, make sure that
you're running an up-to-date version of both compilers as Granary depends on
several C++11 features.

Installing
----------
This is a necessary first step. This step creates a number of folders, generates
files, etc.

```basemake
make install GR_CC=gcc
```

Compiling for user space
------------------------

```basemake
make detach KERNEL=0
make wrappers KERNEL=0
make clean KERNEL=0 ; make all KERNEL=0 GR_CC=gcc GR_CXX=g++
```

### Compiling with clang
Other options for clang are `GR_ASAN` and `GR_LIBCXX`.

Compiling for kernel space
--------------------------
Note: If you are using a remote machine, e.g. a VM, then specify `--remote`,
otherwise leave it absent.

```basemake
python scripts/load.py --remote --symbols
make detach KERNEL=1
make wrappers KERNEL=1
make clear KERNEL=1 ; make all KERNEL=1 GR_CC=gcc GR_CXX=g++
```

Instrumenting user space programs
---------------------------------

### Linux
```basemake
LD_PRELOAD=./libgranary.so my_program
```

### Mac OS X
```basemake
DYLD_INSERT_LIBRARIES=./libgranary.dylib my_program
```