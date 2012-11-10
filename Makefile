
KERNEL_DIR = /lib/modules/$(shell uname -r)/build
PWD = $(shell pwd)

# Compilation toolchain
GR_CPP = cpp
GR_CC = gcc
GR_CXX = g++-4.6

# Conditional compilation for kernel code; useful for testing if Granary code
# compiles independent of the Linux kernel.
KERNEL ?= 1
KERNEL_MAKE = make -C $(KERNEL_DIR) M=$(PWD) modules
KERNEL_CLEAN = make -C $(KERNEL_DIR) M=$(PWD) clean
KERNEL_MODEL = -mcmodel=kernel
ifneq ($(KERNEL),1)
	KERNEL_MAKE =
	KERNEL_CLEAN =
	KERNEL_MODEL =
endif

# Compilation options
GR_DEBUG_LEVEL = -g3
GR_CC_FLAGS = $(KERNEL_MODEL) -I$(PWD) $(GR_DEBUG_LEVEL)
GR_CXX_FLAGS = $(KERNEL_MODEL) -I$(PWD) $(GR_DEBUG_LEVEL) -fno-rtti -fno-exceptions -std=c++0x
GR_CXX_FLAGS += -ansi -Wall -Werror -Wextra
GR_CXX_FLAGS += -Wno-variadic-macros -Wno-long-long

# Compilation options that are conditional on the compiler
ifneq (,$(findstring clang,$(GR_CC))) # clang
	GR_CC_FLAGS += -Wno-null-dereference -Wno-unused-value
	GR_CXX_FLAGS += -Wno-gnu
endif

GR_OBJS = 

# DynamoRIO dependencies
GR_OBJS += bin/dr/instrlist.o
GR_OBJS += bin/dr/dcontext.o
GR_OBJS += bin/dr/x86/instr.o
GR_OBJS += bin/dr/x86/decode.o
GR_OBJS += bin/dr/x86/decode_fast.o
GR_OBJS += bin/dr/x86/decode_table.o
GR_OBJS += bin/dr/x86/encode.o
GR_OBJS += bin/dr/x86/mangle.o
GR_OBJS += bin/dr/x86/proc.o
GR_OBJS += bin/dr/x86/instrument.o
GR_OBJS += bin/dr/x86/x86.o

# Granary (C++) dependencies
GR_OBJS += bin/granary/module.o
GR_OBJS += bin/granary/heap.o
GR_OBJS += bin/granary/printf.o
GR_OBJS += bin/granary/instruction.o

# Module objects
obj-m += granary.o
granary-objs = $(GR_OBJS) module.o

# DynamoRIO rules for C files
bin/dr/%.o: dr/%.c
	$(GR_CC) $(GR_CC_FLAGS) -c $< -S -o bin/dr/$*.S

# DynamoRIO rules for assembly files
bin/dr/%.o: dr/%.asm
	$(GR_CPP) -I$(PWD) -E $< > bin/dr/$*.1.S
	sed 's/@N@/\n/g' bin/dr/$*.1.S > bin/dr/$*.S
	rm bin/dr/$*.1.S

# Granary rules for C++ files
bin/granary/%.o: granary/%.cc
	$(GR_CXX) $(GR_CXX_FLAGS) -c $< -S -o bin/granary/$*.S

install:
	-mkdir bin
	-mkdir bin/granary
	-mkdir bin/dr
	-mkdir bin/dr/x86

all: $(GR_OBJS)
	$(KERNEL_MAKE)

clean:
	-rm bin/dr/*.S
	-rm bin/dr/x86/*.S
	-rm bin/granary/*.S
	$(KERNEL_CLEAN)
