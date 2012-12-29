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
make clear KERNEL=0 ; make all KERNEL=0 GR_CC=gcc GR_CXX=g++
```

### Compiling with clang
Other options for clang are `GR_ASAN` and `GR_LIBCXX`.

Compiling for kernel space
--------------------------
Note: the last step is likely to fail because we are missing the kernel types
for all but one set of kernel versions.

```basemake
make detach KERNEL=1
make clear KERNEL=1 ; make all KERNEL=1 GR_CC=gcc GR_CXX=g++
```
