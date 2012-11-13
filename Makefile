
KERNEL_DIR = /lib/modules/$(shell uname -r)/build
PWD = $(shell pwd)

# Config
GR_NAME = granary

# Compilation toolchain
GR_CPP = cpp
GR_CC = gcc
GR_CXX = g++-4.6

# Later used commands/options
GR_MAKE =
GR_CLEAN =
GR_OUTPUT_FORMAT =
GR_PP_CC =

# Compilation options
GR_DEBUG_LEVEL = -g3
GR_CC_FLAGS = -I$(PWD) $(GR_DEBUG_LEVEL)
GR_CXX_FLAGS = -I$(PWD) $(GR_DEBUG_LEVEL) -fno-rtti -fno-exceptions -std=c++0x
GR_CXX_FLAGS += -Wall -Werror -Wextra
GR_CXX_FLAGS += -Wno-variadic-macros -Wno-long-long

# Compilation options that are conditional on the compiler
ifneq (,$(findstring clang,$(GR_CC))) # clang
	GR_CC_FLAGS += -Wno-null-dereference -Wno-unused-value
	GR_CXX_FLAGS += -Wno-gnu
	GR_PP_CC = __clang__
endif

ifneq (,$(findstring gcc,$(GR_CC))) # clang
	GR_CC_FLAGS += -Wno-null-dereference -Wno-unused-value
	GR_CXX_FLAGS += -Wno-gnu
	GR_PP_CC = __GNUC__
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

# Granary (C++) auto-generated dependencies
GR_OBJS += bin/granary/gen/instruction.o

# Conditional compilation for kernel code; useful for testing if Granary code
# compiles independent of the Linux kernel.
KERNEL ?= 1

# user space
ifneq ($(KERNEL),1)
	GR_CC_FLAGS += -DGRANARY_IN_KERNEL=0
	GR_CXX_FLAGS += -DGRANARY_IN_KERNEL=0
	
	GR_MAKE += gcc -c bin/dr/x86/x86.S -o bin/dr/x86/x86.o ; 
	GR_MAKE += $(GR_CC) $(GR_CC_FLAGS) $(GR_OBJS) -o $(GR_NAME).out
	GR_CLEAN =
	GR_OBJS += bin/main.o
	GR_OUTPUT_FORMAT = o

# kernel space
else
	# Module objects
	obj-m += $(GR_NAME).o
	$(GR_NAME)-objs = $(GR_OBJS) module.o
	
	GR_MAKE = make -C $(KERNEL_DIR) M=$(PWD) modules
	GR_CLEAN = make -C $(KERNEL_DIR) M=$(PWD) clean
	GR_OUTPUT_FORMAT = S
	
	GR_CC_FLAGS += -mcmodel=kernel -S -DGRANARY_IN_KERNEL=1
	GR_CXX_FLAGS += -mcmodel=kernel -S -DGRANARY_IN_KERNEL=1
endif

# DynamoRIO rules for C files
bin/dr/%.o: dr/%.c
	$(GR_CC) $(GR_CC_FLAGS) -c $< -o bin/dr/$*.$(GR_OUTPUT_FORMAT)

# DynamoRIO rules for assembly files
bin/dr/%.o: dr/%.asm
	$(GR_CPP) -D$(GR_PP_CC) -I$(PWD) -E $< > bin/dr/$*.1.S
	python post_process_asm.py bin/dr/$*.1.S > bin/dr/$*.S
	rm bin/dr/$*.1.S

# Granary rules for C++ files
bin/granary/%.o: granary/%.cc
	$(GR_CXX) $(GR_CXX_FLAGS) -c $< -o bin/granary/$*.$(GR_OUTPUT_FORMAT)

# Granary user space "harness" for testing compilation, etc. This is convenient
# for coding Granary on non-Linux platforms because it allows for debugging the
# build process, and partial testing of the code generation process.
bin/main.o: main.cc
	$(GR_CXX) $(GR_CXX_FLAGS) -c main.cc -o bin/main.$(GR_OUTPUT_FORMAT)

install:
	python process_instr_create.py
	-mkdir bin
	-mkdir bin/granary
	-mkdir bin/granary/gen
	-mkdir bin/dr
	-mkdir bin/dr/x86

all: $(GR_OBJS)
	$(GR_MAKE)

# Remove all generated files. This goes and touches some file in all of the
# directories to make sure cleaning doesn't report any errors; depending on
# the compilation mode (KERNEL=0/1), not all bin folders will have objects
clean:
	touch bin/_.$(GR_OUTPUT_FORMAT)
	touch bin/dr/_.$(GR_OUTPUT_FORMAT)
	touch bin/dr/x86/_.$(GR_OUTPUT_FORMAT)
	touch bin/granary/_.$(GR_OUTPUT_FORMAT)
	touch bin/granary/gen/_.$(GR_OUTPUT_FORMAT)
	
	-rm bin/*.$(GR_OUTPUT_FORMAT)
	-rm bin/dr/*.$(GR_OUTPUT_FORMAT)
	-rm bin/dr/x86/*.$(GR_OUTPUT_FORMAT)
	-rm bin/granary/*.$(GR_OUTPUT_FORMAT)
	-rm bin/granary/gen/*.$(GR_OUTPUT_FORMAT)
	$(GR_CLEAN)
