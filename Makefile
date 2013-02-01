
KERNEL_DIR = /lib/modules/$(shell uname -r)/build
PWD = $(shell pwd)

# Config
GR_NAME = granary

# Compilation toolchain
GR_CPP = cpp
GR_CC = gcc
GR_LD = gcc
GR_CXX = g++-4.7
GR_CXX_STD = -std=gnu++0x

# Later used commands/options
GR_MAKE =
GR_CLEAN =
GR_OUTPUT_FORMAT =

# Compilation options
GR_DEBUG_LEVEL = -g3 -O0
#-g3 -O0
GR_LD_FLAGS = 
GR_CC_FLAGS = -I$(PWD) $(GR_DEBUG_LEVEL) -mno-red-zone
GR_CXX_FLAGS = -I$(PWD) $(GR_DEBUG_LEVEL) -mno-red-zone -fno-rtti 
GR_CXX_FLAGS += -fno-exceptions -Wall -Werror -Wextra -Wstrict-aliasing=2
GR_CXX_FLAGS += -Wno-variadic-macros -Wno-long-long -Wno-unused-function

GR_EXTRA_CC_FLAGS ?=
GR_EXTRA_CXX_FLAGS ?=
GR_EXTRA_LD_FLAGS ?=

# Options for generating type information.
GR_TYPE_CC = $(GR_CC)
GR_TYPE_CC_FLAGS =
GR_INPUT_TYPES =
GR_OUTPUT_TYPES =
GR_OUTPUT_WRAPPERS =

# Conditional compilation for kernel code; useful for testing if Granary code
# compiles independent of the Linux kernel.
KERNEL ?= 1

# Enable address sanitizer on Granary? Default is 2, which means it's
# "unsupported", i.e. llvm wasn't compiled with compiler-rt.
GR_ASAN ?= 2

# Enable libc++? (llvm's c++ standard library)
GR_LIBCXX ?= 0

# Compilation options that are conditional on the compiler
ifneq (,$(findstring clang,$(GR_CC))) # clang

	GR_CC_FLAGS += -Wno-null-dereference -Wno-unused-value -Wstrict-overflow=4
	GR_CXX_FLAGS += -Wno-gnu -Wno-attributes
	GR_CXX_STD = -std=c++11
	GR_TYPE_CC = $(GR_CC)
	
	# explicitly enable/disable address sanitizer
    ifeq ('0','$(GR_ASAN)')
    	GR_CC_FLAGS += -fno-sanitize=address
    	GR_CXX_FLAGS += -fno-sanitize=address
	else
    	ifeq ('1','$(GR_ASAN)')
    		GR_CC_FLAGS += -fsanitize=address
    		GR_CXX_FLAGS += -fsanitize=address
		endif
    endif
    
    # enable the newer standard and use it with libc++
    ifeq ('1','$(GR_LIBCXX)')
		GR_CXX_STD = -std=c++11 -stdlib=libc++
    endif
endif

ifneq (,$(findstring gcc,$(GR_CC))) # gcc
	GR_CC_FLAGS += 
	GR_CXX_FLAGS +=
endif

ifneq (,$(findstring icc,$(GR_CC))) # icc
	GR_CC_FLAGS += -diag-disable 188 -diag-disable 186 
	GR_CXX_FLAGS += -D__GXX_EXPERIMENTAL_CXX0X__ -Dnullptr="((void *) 0)"
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
GR_OBJS += bin/granary/hash_table.o
GR_OBJS += bin/granary/register.o
GR_OBJS += bin/granary/policy.o
GR_OBJS += bin/granary/init.o

# Granary wrapper dependencies
GR_OBJS += bin/granary/wrapper.o

# Granary (x86) dependencies
GR_OBJS += bin/granary/x86/utils.o
GR_OBJS += bin/granary/x86/direct_branch.o

# Granary (C++) auto-generated dependencies
GR_OBJS += bin/granary/gen/instruction.o

# Client code dependencies
GR_OBJS += bin/clients/instrument.o

# user space
ifneq ($(KERNEL),1)

	GR_TYPE_CC = $(GR_CXX)
	GR_INPUT_TYPES = granary/user/posix/types.h
	GR_OUTPUT_TYPES = granary/gen/user_types.h
	GR_OUTPUT_WRAPPERS = granary/gen/user_wrappers.h

	GR_OBJS += bin/granary/user/allocator.o
	GR_OBJS += bin/granary/user/state.o
	GR_OBJS += bin/granary/user/init.o
	GR_OBJS += bin/granary/user/utils.o
	GR_OBJS += bin/granary/test.o
	GR_OBJS += bin/main.o

	# Granary tests
	GR_OBJS += bin/tests/test_direct_cbr.o
	GR_OBJS += bin/tests/test_direct_call.o
	GR_OBJS += bin/tests/test_lock_inc.o
	GR_OBJS += bin/tests/test_direct_rec.o
	GR_OBJS += bin/tests/test_indirect_cti.o
	
	GR_LD_FLAGS += $(GR_EXTRA_LD_FLAGS) -pthread -lrt -lm
	GR_CC_FLAGS += -DGRANARY_IN_KERNEL=0
	GR_CXX_FLAGS += -DGRANARY_IN_KERNEL=0
	
	GR_MAKE += $(GR_CC) -c bin/deps/dr/x86/x86.S -o bin/deps/dr/x86/x86.o ; 
	GR_MAKE += $(GR_CC) $(GR_DEBUG_LEVEL) $(GR_OBJS) $(GR_LD_FLAGS) -o $(GR_NAME).out
	GR_CLEAN =
	GR_OUTPUT_FORMAT = o

    define GR_COMPILE_ASM
    	$$($(GR_CC) -c bin/granary/x86/$1.S -o bin/granary/x86/$1.o)
endef

# kernel space
else
	
	GR_TYPE_CC_FLAGS += -mcmodel=kernel
	GR_INPUT_TYPES = granary/kernel/linux/types.h
	GR_OUTPUT_TYPES = granary/gen/kernel_types.h
	GR_OUTPUT_WRAPPERS = granary/gen/kernel_wrappers.h

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

GR_CC_FLAGS += $(GR_EXTRA_CC_FLAGS)
GR_CXX_FLAGS += $(GR_EXTRA_CXX_FLAGS)


# MumurHash3 rules for C++ files
bin/deps/murmurhash/%.o: deps/murmurhash/%.cc
	$(GR_CXX) $(GR_CXX_FLAGS) -c $< -o bin/deps/murmurhash/$*.$(GR_OUTPUT_FORMAT)


# DynamoRIO rules for C files
bin/deps/dr/%.o: deps/dr/%.c
	$(GR_CC) $(GR_CC_FLAGS) -c $< -o bin/deps/dr/$*.$(GR_OUTPUT_FORMAT)


# DynamoRIO rules for assembly files
bin/deps/dr/%.o: deps/dr/%.asm
	$(GR_CC) -I$(PWD) -E -o bin/deps/dr/$*.1.S -x c -std=c99 $<
	python scripts/post_process_asm.py bin/deps/dr/$*.1.S > bin/deps/dr/$*.S
	rm bin/deps/dr/$*.1.S


# Granary rules for C++ files
bin/granary/%.o: granary/%.cc
	$(GR_CXX) $(GR_CXX_FLAGS) -c $< -o bin/granary/$*.$(GR_OUTPUT_FORMAT)


# Granary rules for assembly files
bin/granary/x86/%.o: granary/x86/%.asm
	$(GR_CC) -I$(PWD) -E -o bin/granary/x86/$*.1.S -x c -std=c99 $<
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


# pre-process then post-process type information; this is used for wrappers,
# etc.
types:
	$(GR_TYPE_CC) $(GR_TYPE_CC_FLAGS) -I./ -E $(GR_INPUT_TYPES) > /tmp/ppt.h
	python scripts/post_process_header.py /tmp/ppt.h > $(GR_OUTPUT_TYPES)


# auto-generate wrappers
wrappers: types
	python scripts/generate_wrappers.py $(GR_OUTPUT_TYPES) > $(GR_OUTPUT_WRAPPERS)


# auto-generate the hash table stuff needed for wrappers and detaching
detach: types
	python scripts/generate_detach_table.py $(GR_OUTPUT_TYPES) granary/gen/detach.h granary/gen/detach.cc 


# make the folders where binaries / generated assemblies are stored
install:
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


# Compile granary
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
