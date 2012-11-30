
KERNEL_DIR = /lib/modules/$(shell uname -r)/build
PWD = $(shell pwd)

# Config
GR_NAME = granary

# Compilation toolchain
GR_CPP = cpp
GR_CC = gcc
GR_CXX = g++-4.7
GR_CXX_STD = -std=gnu++0x

# Later used commands/options
GR_MAKE =
GR_CLEAN =
GR_OUTPUT_FORMAT =

# Compilation options
GR_DEBUG_LEVEL = -g3 -O0
GR_CC_FLAGS = -I$(PWD) $(GR_DEBUG_LEVEL)
GR_CXX_FLAGS = -I$(PWD) $(GR_DEBUG_LEVEL) -fno-rtti -fno-exceptions
GR_CXX_FLAGS += -Wall -Werror -Wextra -Wstrict-aliasing=2
GR_CXX_FLAGS += -Wno-variadic-macros -Wno-long-long -Wno-unused-function


# Compilation options that are conditional on the compiler
ifneq (,$(findstring clang,$(GR_CC))) # clang
	
	# Enable address sanitizer on Granary?
	GR_ASAN ?= 0

	GR_CC_FLAGS += -Wno-null-dereference -Wno-unused-value
	GR_CXX_FLAGS += -Wno-gnu
	GR_CXX_STD = -std=c++0x
	
    ifeq ('0','$(GR_ASAN)')
    	GR_CC_FLAGS += -fno-sanitize=address
    	GR_CXX_FLAGS += -fno-sanitize=address
	else
		GR_CC_FLAGS += -fsanitize=address
		GR_CXX_FLAGS += -fsanitize=address
    endif
endif

ifneq (,$(findstring gcc,$(GR_CC))) # gcc
	GR_CC_FLAGS += 
	GR_CXX_FLAGS +=
endif

ifneq (,$(findstring icc,$(GR_CC))) # icc
	GR_CC_FLAGS += -diag-disable 188 -diag-disable 186 
	GR_CXX_FLAGS += 
	GR_CXX_STD = -std=c++11
endif

GR_CXX_FLAGS += $(GR_CXX_STD)
GR_OBJS = 

# DynamoRIO dependencies
GR_OBJS += bin/deps/dr/instrlist.o
GR_OBJS += bin/deps/dr/dcontext.o
GR_OBJS += bin/deps/dr/x86/instr.o
GR_OBJS += bin/deps/dr/x86/decode.o
GR_OBJS += bin/deps/dr/x86/decode_fast.o
GR_OBJS += bin/deps/dr/x86/decode_table.o
GR_OBJS += bin/deps/dr/x86/encode.o
GR_OBJS += bin/deps/dr/x86/mangle.o
GR_OBJS += bin/deps/dr/x86/proc.o
GR_OBJS += bin/deps/dr/x86/instrument.o
GR_OBJS += bin/deps/dr/x86/x86.o

# MurmurHash3 dependencies
GR_OBJS += bin/deps/murmurhash/murmurhash.o

# Granary (C++) dependencies
GR_OBJS += bin/granary/instruction.o
GR_OBJS += bin/granary/basic_block.o
GR_OBJS += bin/granary/detach.o
GR_OBJS += bin/granary/state.o
GR_OBJS += bin/granary/mangle.o
GR_OBJS += bin/granary/code_cache.o
GR_OBJS += bin/granary/test.o

# Granary (x86) dependencies
GR_OBJS += bin/granary/x86/utils.o
GR_OBJS += bin/granary/x86/direct_branch.o

# Granary (C++) auto-generated dependencies
GR_OBJS += bin/granary/gen/instruction.o

# Client code dependencies
GR_OBJS += bin/clients/instrument.o

# Conditional compilation for kernel code; useful for testing if Granary code
# compiles independent of the Linux kernel.
KERNEL ?= 1

# user space
ifneq ($(KERNEL),1)
	GR_OBJS += bin/granary/user/allocator.o
	GR_OBJS += bin/granary/user/state.o
	GR_OBJS += bin/granary/user/init.o
	GR_OBJS += bin/granary/user/utils.o
	GR_OBJS += bin/main.o

	# Granary tests
	GR_OBJS += bin/tests/test_direct_cbr.o

	GR_CC_FLAGS += -DGRANARY_IN_KERNEL=0
	GR_CXX_FLAGS += -DGRANARY_IN_KERNEL=0
	
	GR_MAKE += $(GR_CC) -c bin/deps/dr/x86/x86.S -o bin/deps/dr/x86/x86.o ; 
	GR_MAKE += $(GR_CXX) $(GR_CXX_FLAGS) $(GR_OBJS) -o $(GR_NAME).out
	GR_CLEAN =
	GR_OUTPUT_FORMAT = o

    define GR_COMPILE_ASM
    	$$($(GR_CC) -c bin/granary/x86/$1.S -o bin/granary/x86/$1.o)
endef

# kernel space
else
	GR_OBJS += bin/granary/kernel/module.o
	GR_OBJS += bin/granary/kernel/allocator.o
	GR_OBJS += bin/granary/kernel/printf.o
	GR_OBJS += bin/granary/kernel/state.o
	GR_OBJS += bin/granary/kernel/init.o
	GR_OBJS += bin/granary/kernel/utils.o

	# Module objects
	obj-m += $(GR_NAME).o
	$(GR_NAME)-objs = $(GR_OBJS) module.o
	
	GR_MAKE = make -C $(KERNEL_DIR) M=$(PWD) modules
	GR_CLEAN = make -C $(KERNEL_DIR) M=$(PWD) clean
	GR_OUTPUT_FORMAT = S
	
	GR_CC_FLAGS += -mcmodel=kernel -S -DGRANARY_IN_KERNEL=1
	GR_CXX_FLAGS += -mcmodel=kernel -S -DGRANARY_IN_KERNEL=1

	define GR_COMPILE_ASM
endef
endif

# MumurHash3 rules for C++ files
bin/deps/murmurhash/%.o: deps/murmurhash/%.cc
	$(GR_CXX) $(GR_CXX_FLAGS) -c $< -o bin/deps/murmurhash/$*.$(GR_OUTPUT_FORMAT)

# DynamoRIO rules for C files
bin/deps/dr/%.o: deps/dr/%.c
	$(GR_CC) $(GR_CC_FLAGS) -c $< -o bin/deps/dr/$*.$(GR_OUTPUT_FORMAT)

# DynamoRIO rules for assembly files
bin/deps/dr/%.o: deps/dr/%.asm
	$(GR_CPP) -I$(PWD) -E $< > bin/deps/dr/$*.1.S
	python scripts/post_process_asm.py bin/deps/dr/$*.1.S > bin/deps/dr/$*.S
	rm bin/deps/dr/$*.1.S

# Granary rules for C++ files
bin/granary/%.o: granary/%.cc
	$(GR_CXX) $(GR_CXX_FLAGS) -c $< -o bin/granary/$*.$(GR_OUTPUT_FORMAT)

# Granary rules for assembly files
bin/granary/x86/%.o: granary/x86/%.asm
	$(GR_CPP) -I$(PWD) -E $< > bin/granary/x86/$*.1.S
	python scripts/post_process_asm.py bin/granary/x86/$*.1.S > bin/granary/x86/$*.S
	rm bin/granary/x86/$*.1.S
	$(call GR_COMPILE_ASM,$*)

# Granary rules for client C++ files
bin/clients/%.o: clients/%.cc
	$(GR_CXX) $(GR_CXX_FLAGS) -c $< -o bin/clients/$*.$(GR_OUTPUT_FORMAT)

# Granary rules for test files
bin/tests/%.o: tests/%.cc
	$(GR_CXX) $(GR_CXX_FLAGS) -c $< -o bin/tests/$*.$(GR_OUTPUT_FORMAT)


# Granary user space "harness" for testing compilation, etc. This is convenient
# for coding Granary on non-Linux platforms because it allows for debugging the
# build process, and partial testing of the code generation process.
bin/main.o: main.cc
	$(GR_CXX) $(GR_CXX_FLAGS) -c main.cc -o bin/main.$(GR_OUTPUT_FORMAT)

install:
	# make the folders where binaries / generated assemblies are stored
	-mkdir bin
	-mkdir bin/granary
	-mkdir bin/granary/user
	-mkdir bin/granary/kernel
	-mkdir bin/granary/gen
	-mkdir bin/granary/x86
	-mkdir bin/clients
	-mkdir bin/tests
	-mkdir bin/deps
	-mkdir bin/deps/dr
	-mkdir bin/deps/dr/x86
	-mkdir bin/deps/murmurhash
	
	# convert DynamoRIO INSTR_CREATE_ and OPND_CREATE_ macros into Granary
	# instruction-building functions
	-mkdir granary/gen
	python scripts/process_instr_create.py

	# compile a dummy file to assembly to figure out which assembly syntax
	# to use for this compiler
	$(GR_CC) -g3 -S -c scripts/static/asm.c -o scripts/static/asm.S
	python scripts/make_asm_defines.py

all: $(GR_OBJS)
	$(GR_MAKE)

# Remove all generated files. This goes and touches some file in all of the
# directories to make sure cleaning doesn't report any errors; depending on
# the compilation mode (KERNEL=0/1), not all bin folders will have objects
clean:
	touch bin/_.$(GR_OUTPUT_FORMAT)
	touch bin/deps/dr/_.$(GR_OUTPUT_FORMAT)
	touch bin/deps/dr/x86/_.$(GR_OUTPUT_FORMAT)
	touch bin/deps/murmurhash/_.$(GR_OUTPUT_FORMAT)
	touch bin/granary/_.$(GR_OUTPUT_FORMAT)
	touch bin/granary/user/_.$(GR_OUTPUT_FORMAT)
	touch bin/granary/kernel/_.$(GR_OUTPUT_FORMAT)
	touch bin/granary/gen/_.$(GR_OUTPUT_FORMAT)
	touch bin/granary/x86/_.$(GR_OUTPUT_FORMAT)
	touch bin/clients/_.$(GR_OUTPUT_FORMAT)
	touch bin/tests/_.$(GR_OUTPUT_FORMAT)
	
	-rm bin/*.$(GR_OUTPUT_FORMAT)
	-rm bin/deps/dr/*.$(GR_OUTPUT_FORMAT)
	-rm bin/deps/dr/x86/*.$(GR_OUTPUT_FORMAT)
	-rm bin/deps/murmurhash/*.$(GR_OUTPUT_FORMAT)
	-rm bin/granary/*.$(GR_OUTPUT_FORMAT)
	-rm bin/granary/user/*.$(GR_OUTPUT_FORMAT)
	-rm bin/granary/kernel/*.$(GR_OUTPUT_FORMAT)
	-rm bin/granary/gen/*.$(GR_OUTPUT_FORMAT)
	-rm bin/granary/x86/*.$(GR_OUTPUT_FORMAT)
	-rm bin/clients/*.$(GR_OUTPUT_FORMAT)
	-rm bin/tests/*.$(GR_OUTPUT_FORMAT)

	$(GR_CLEAN)
