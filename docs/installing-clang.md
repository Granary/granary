WARNING!!! This document is outdated.

Installing LLVM and Clang/Clang++
=================================

1. Getting an up-to-date GCC on Linux
-------------------------------------
If you are running Linux and intend to compile Granary, then having an up-to-date
version of the GCC is essential.

```bash
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt-get update
sudo apt-get upgrade
sudo apt-get install gcc-4.7
sudo apt-get install g++-4.7
```

If gcc 4.7 is not the latest version (with g++ support) then get the latest
version instead.

2. Download LLVM
----------------

### 2.1. Make sure you have Subversion.
```bash
sudo apt-get install subversion
```

### 2.2. Assuming you have a `Code` folder, download the latest LLVM, Clang, and
compiler-rt from the online repositories.
```bash
cd ~/Code
svn co http://llvm.org/svn/llvm-project/llvm/trunk llvm
cd ~/Code/llvm/tools
svn co http://llvm.org/svn/llvm-project/cfe/trunk clang
cd ~/Code/llvm/projects
svn co http://llvm.org/svn/llvm-project/compiler-rt/trunk compiler-rt
```

### 2.3. Configure Clang to use the latest GCC, and make sure that a line is commented.
```bash
cd ~/Code/llvm/tools/clang/lib/Frontend
gedit InitHeaderSearch.cpp &
```

Search for the following approximate code.

```c++
// FIXME: temporary hack: hard-coded paths.
//AddPath("/usr/local/include", System, true, false, false);

AddPath("/usr/include/c++/4.6", System, true, false, false);
AddPath("/usr/include/c++/4.6/x86_64-linux-gnu/.", System, true, false, false);
AddPath("/usr/include/c++/4.6/backward", System, true, false, false);
AddPath("/usr/lib/gcc/x86_64-linux-gnu/4.6.1/include", System, true, false, false);
AddPath("/usr/local/include", System, true, false, false);
AddPath("/usr/lib/gcc/x86_64-linux-gnu/4.6.1/include-fixed", System, true, false, false);
AddPath("/usr/include/x86_64-linux-gnu", System, true, false, false);
AddPath("/usr/include", System, true, false, false);
```

If you find it then make sure that the call to `AddPath` following the `FIXME`
comment is commented out (as shown above). Also change all of the GCC versions
to the latest 4.7 (or newer).

### 2.4. Build LLVM, Clang, and compiler-rt.
```bash
cd ~/Code
mkdir build
cd build
mkdir llvm
cd llvm
../../llvm/configure --enable-optimized --enable-targets=host-only
make -j 8
sudo make install
```

### 2.5. Go download libc++, Clang's new C++ standard library.
```bash
cd ~/Code
svn co http://llvm.org/svn/llvm-project/libcxx/trunk libcxx
cd libcxx/lib
./buildit
```

### 2.6. Link libc++
```bash
cd /usr/lib
sudo ln -sf /home/pag/Code/libcxx/lib/libc++.so.1.0 libc++.so 
sudo ln -sf /home/pag/Code/libcxx/lib/libc++.so.1.0 libc++.so.1
cd /usr/include/c++
sudo ln -sf /home/pag/Code/libcxx/include v1
```
