
KERNEL_DIR = /lib/modules/$(shell uname -r)/build
PWD = $(shell pwd)
UNAME = $(shell uname)

# Config
GR_NAME = granary

# Compilation toolchain
GR_CPP = cpp
GR_CC = gcc
GR_LD = gcc
GR_LDD = ldd
GR_CXX = g++-4.7
GR_CXX_STD = -std=gnu++0x

# Later used commands/options
GR_MAKE =
GR_CLEAN =
GR_OUTPUT_FORMAT =

# Compilation options
GR_DEBUG_LEVEL = -g0 -O0
GR_LD_PREFIX_FLAGS = 
GR_LD_SUFFIX_FLAGS = 
GR_ASM_FLAGS = -I$(PWD)
GR_CC_FLAGS = -I$(PWD) $(GR_DEBUG_LEVEL)
GR_CXX_FLAGS = -I$(PWD) $(GR_DEBUG_LEVEL) -fno-rtti
GR_CXX_FLAGS += -fno-exceptions -Wall -Werror -Wextra -Wstrict-aliasing=2
GR_CXX_FLAGS += -Wno-variadic-macros -Wno-long-long -Wno-unused-function
GR_CXX_FLAGS += -Wno-format-security -funit-at-a-time -Wshadow

GR_EXTRA_CC_FLAGS ?=
GR_EXTRA_CXX_FLAGS ?=
GR_EXTRA_LD_FLAGS ?=

# Options for generating type information.
GR_TYPE_CC = $(GR_CC)
GR_TYPE_CC_FLAGS =
GR_INPUT_TYPES =
GR_OUTPUT_TYPES =
GR_OUTPUT_WRAPPERS =

GR_TYPE_INCLUDE = 

# Conditional compilation for kernel code; useful for testing if Granary code
# compiles independent of the Linux kernel.
KERNEL ?= 1

# Enable address sanitizer on Granary? Default is 2, which means it is
# "unsupported", i.e. llvm was not compiled with compiler-rt.
GR_ASAN ?= 2

# Enable libc++? (llvm's c++ standard library)
GR_LIBCXX ?= 0


# Are we making a DLL for later dynamic linking?
GR_DLL ?= 0
GR_OUTPUT_PREFIX =
GR_OUTPUT_SUFFIX = .out


ifeq ($(UNAME),Darwin) # Mac OS X
	GR_LDD = otool -L
endif


# Compilation options that are conditional on the compiler
ifneq (,$(findstring clang,$(GR_CC))) # clang

	GR_CC_FLAGS += -Wno-null-dereference -Wno-unused-value -Wstrict-overflow=4
	GR_CXX_FLAGS += -Wno-gnu -Wno-attributes 
	GR_CXX_STD = -std=c++11
	
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
	GR_CC_FLAGS += -fdiagnostics-show-option
	GR_TYPE_CC_FLAGS += -fdiagnostics-show-option
	GR_CXX_FLAGS += -fdiagnostics-show-option
endif

ifneq (,$(findstring icc,$(GR_CC))) # icc
	GR_CC_FLAGS += -diag-disable 188 -diag-disable 186 
	GR_CXX_FLAGS += -D__GXX_EXPERIMENTAL_CXX0X__ -Dnullptr="((void *) 0)"
	GR_CXX_STD = -std=c++11
endif

GR_CXX_FLAGS += $(GR_CXX_STD)
GR_OBJS = 

# DynamoRIO dependencies
GR_OBJS += bin/deps/dr/dcontext.o
GR_OBJS += bin/deps/dr/x86/instr.o
GR_OBJS += bin/deps/dr/x86/decode.o
GR_OBJS += bin/deps/dr/x86/decode_fast.o
GR_OBJS += bin/deps/dr/x86/decode_table.o
GR_OBJS += bin/deps/dr/x86/encode.o
GR_OBJS += bin/deps/dr/x86/mangle.o
GR_OBJS += bin/deps/dr/x86/proc.o
GR_OBJS += bin/deps/dr/x86/x86.o

# MurmurHash3 dependencies
GR_OBJS += bin/deps/murmurhash/murmurhash.o

# Granary (C++) dependencies
GR_OBJS += bin/granary/instruction.o
GR_OBJS += bin/granary/basic_block.o
GR_OBJS += bin/granary/attach.o
GR_OBJS += bin/granary/detach.o
GR_OBJS += bin/granary/state.o
GR_OBJS += bin/granary/mangle.o
GR_OBJS += bin/granary/code_cache.o
GR_OBJS += bin/granary/emit_utils.o
GR_OBJS += bin/granary/hash_table.o
GR_OBJS += bin/granary/cpu_code_cache.o
GR_OBJS += bin/granary/register.o
GR_OBJS += bin/granary/policy.o
GR_OBJS += bin/granary/predict.o
GR_OBJS += bin/granary/perf.o
GR_OBJS += bin/granary/init.o

# Granary wrapper dependencies
GR_OBJS += bin/granary/wrapper.o

# Granary (x86) dependencies
GR_OBJS += bin/granary/x86/utils.o
GR_OBJS += bin/granary/x86/direct_branch.o
GR_OBJS += bin/granary/x86/attach.o

# Granary (C++) auto-generated dependencies
GR_OBJS += bin/granary/gen/instruction.o

# Client code dependencies
GR_OBJS += bin/clients/instrument.o

# user space
ifneq ($(KERNEL),1)

	GR_INPUT_TYPES = granary/user/posix/types.h
	GR_OUTPUT_TYPES = granary/gen/user_types.h
	GR_OUTPUT_WRAPPERS = granary/gen/user_wrappers.h
	
	GR_OBJS += bin/granary/test.o
	
	# user-specific versions of granary functions
	GR_OBJS += bin/granary/user/allocator.o
	GR_OBJS += bin/granary/user/state.o
	GR_OBJS += bin/granary/user/init.o
	GR_OBJS += bin/granary/user/utils.o
	GR_OBJS += bin/granary/user/printf.o
	
	ifneq ($(GR_DLL),1)
		GR_OBJS += bin/main.o
	else
		GR_OBJS += bin/dlmain.o
		GR_ASM_FLAGS += -fPIC
		GR_LD_PREFIX_FLAGS += -fPIC
		GR_CC_FLAGS += -fPIC -DGRANARY_USE_PIC -fvisibility=hidden
		GR_CXX_FLAGS += -fPIC -DGRANARY_USE_PIC -fvisibility=hidden
		GR_CXX_FLAGS += -fvisibility-inlines-hidden
		GR_OUTPUT_PREFIX = lib
		
		ifeq ($(UNAME),Darwin) # Mac OS X
			GR_OUTPUT_SUFFIX = .dylib
			GR_LD_PREFIX_FLAGS += -dynamiclib
		else # Linux
			GR_OUTPUT_SUFFIX = .so
			GR_LD_PREFIX_FLAGS += -shared 
		endif
	endif

	# Granary tests
	GR_OBJS += bin/tests/test_direct_cbr.o
	GR_OBJS += bin/tests/test_direct_call.o
	GR_OBJS += bin/tests/test_lock_inc.o
	GR_OBJS += bin/tests/test_direct_rec.o
	GR_OBJS += bin/tests/test_indirect_cti.o
	GR_OBJS += bin/tests/test_mat_mul.o
	GR_OBJS += bin/tests/test_md5.o
	GR_OBJS += bin/tests/test_sigsetjmp.o
	#GR_OBJS += bin/tests/test_pthreads.o

	# figure out how to link in various libraries that might be OS-specific
	GR_LD_PREFIX_SPECIFIC =
	GR_LD_SUFFIX_SPECIFIC =

	ifeq ($(UNAME), Darwin)
		GR_LD_PREFIX_SPECIFIC = -lpthread
	endif
	
	ifeq ($(UNAME), Linux)
		GR_LD_PREFIX_SPECIFIC = -pthread
		GR_LD_SUFFIX_SPECIFIC = -lrt -ldl -lcrypt
	endif
	
	GR_LD_PREFIX_FLAGS += $(GR_EXTRA_LD_FLAGS) $(GR_LD_PREFIX_SPECIFIC)
	GR_LD_SUFFIX_FLAGS += -lm $(GR_LD_SUFFIX_SPECIFIC)
	GR_ASM_FLAGS += -DGRANARY_IN_KERNEL=0
	GR_CC_FLAGS += -DGRANARY_IN_KERNEL=0
	GR_CXX_FLAGS += -DGRANARY_IN_KERNEL=0
	
	GR_MAKE += $(GR_CC) -c bin/deps/dr/x86/x86.S -o bin/deps/dr/x86/x86.o ; 
	GR_MAKE += $(GR_CC) $(GR_DEBUG_LEVEL) $(GR_LD_PREFIX_FLAGS) $(GR_OBJS) $(GR_LD_SUFFIX_FLAGS) -o $(GR_OUTPUT_PREFIX)$(GR_NAME)$(GR_OUTPUT_SUFFIX)
	GR_CLEAN =
	GR_OUTPUT_FORMAT = o

	# Compile an assembly file to an object file in user space. In kernel space,
	# the kernel makefile will do this for us.
	define GR_COMPILE_ASM
		$$($(GR_CC) $(GR_ASM_FLAGS) -c bin/granary/x86/$1.S -o bin/granary/x86/$1.o)
endef

	define GR_GENERATE_INIT_FUNC
endef
	
	# Get a list of dynamically loaded libraries from the compiler being used.
	# This is so that we can augment the detach table with internal libc symbols,
	# etc.
	define GR_GET_LD_LIBRARIES
		$$($(GR_LDD) $(shell which $(GR_CC)) | python scripts/generate_dll_detach_table.py >> granary/gen/detach.inc)
endef

	define GR_GET_TYPE_DEFINES
endef

# kernel space
else
	ifneq (,$(findstring clang,$(GR_CC))) # clang
		GR_TYPE_CC_FLAGS += -mkernel
		GR_TYPE_CXX_FLAGS += -mkernel
	else
		GR_TYPE_CC_FLAGS += -mcmodel=kernel
		GR_TYPE_CXX_FLAGS += -mcmodel=kernel
	endif
	
	GR_TYPE_CC_FLAGS += -mno-red-zone -nostdlib -nostartfiles
	GR_TYPE_CXX_FLAGS += -mno-red-zone -nostdlib 
	
	GR_INPUT_TYPES = granary/kernel/linux/types.h
	GR_OUTPUT_TYPES = granary/gen/kernel_types.h
	GR_OUTPUT_WRAPPERS = granary/gen/kernel_wrappers.h
	
	# kernel-specific versions of granary functions
	GR_OBJS += bin/granary/kernel/module.o
	GR_OBJS += bin/granary/kernel/allocator.o
	GR_OBJS += bin/granary/kernel/printf.o
	GR_OBJS += bin/granary/kernel/state.o
	GR_OBJS += bin/granary/kernel/init.o
	GR_OBJS += bin/granary/kernel/utils.o
	
	# C++ ABI-specific stuff
	GR_OBJS += bin/deps/icxxabi/icxxabi.o

	# Must be last!!!!
	GR_OBJS += bin/granary/x86/init.o
	
	# Module objects
	obj-m += $(GR_NAME).o
	$(GR_NAME)-objs = $(GR_OBJS) module.o
	
	GR_MAKE = make -C $(KERNEL_DIR) M=$(PWD) modules
	GR_CLEAN = make -C $(KERNEL_DIR) M=$(PWD) clean
	GR_OUTPUT_FORMAT = S
	
	GR_ASM_FLAGS += -DGRANARY_IN_KERNEL=1
	GR_CC_FLAGS += -mcmodel=kernel -mno-red-zone -nostdlib -nostartfiles -S -DGRANARY_IN_KERNEL=1
	GR_CXX_FLAGS += -mcmodel=kernel -mno-red-zone -nostdlib -nostartfiles -S -DGRANARY_IN_KERNEL=1
	
	GR_TYPE_INCLUDE = -I./ -isystem $(KERNEL_DIR)/include
	GR_TYPE_INCLUDE += -isystem $(KERNEL_DIR)/arch/x86/include 
	
	define GR_COMPILE_ASM
endef
	
	# Parse an assembly file looking for functions that must be called
	# in order to initialise the static global data.
	define GR_GENERATE_INIT_FUNC
		$$(python scripts/generate_init_func.py $1 >> granary/gen/kernel_init.S)
endef

	# get the addresses of kernel symbols
	define GR_GET_LD_LIBRARIES
		$$(sudo python scripts/generate_detach_addresses.py)
endef
	
	# Get all pre-defined macros so that we can suck in the kernel headers
	# properly.
	define GR_GET_TYPE_DEFINES
		$(shell python scripts/generate_kernel_macros.py > /dev/null &> /dev/null ; cp granary/kernel/linux/macros/empty.o granary/gen/kernel_macros.h )
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
	$(GR_CC) $(GR_ASM_FLAGS) -E -o bin/deps/dr/$*.1.S -x c -std=c99 $<
	python scripts/post_process_asm.py bin/deps/dr/$*.1.S > bin/deps/dr/$*.S
	rm bin/deps/dr/$*.1.S

# Itanium C++ ABI rules for C++ files
bin/deps/icxxabi/%.o: deps/icxxabi/%.cc
	$(GR_CXX) $(GR_CXX_FLAGS) -c $< -o bin/deps/icxxabi/$*.$(GR_OUTPUT_FORMAT)


# Granary rules for C++ files
bin/granary/%.o: granary/%.cc
	$(GR_CXX) $(GR_CXX_FLAGS) -c $< -o bin/granary/$*.$(GR_OUTPUT_FORMAT)
	$(call GR_GENERATE_INIT_FUNC,bin/granary/$*.$(GR_OUTPUT_FORMAT))


# Granary rules for assembly files
bin/granary/x86/%.o: granary/x86/%.asm
	$(GR_CC) $(GR_ASM_FLAGS) -E -o bin/granary/x86/$*.1.S -x c -std=c99 $<
	python scripts/post_process_asm.py bin/granary/x86/$*.1.S > bin/granary/x86/$*.S
	rm bin/granary/x86/$*.1.S
	$(call GR_COMPILE_ASM,$*)


# Granary rules for client C++ files
bin/clients/%.o: clients/%.cc
	$(GR_CXX) $(GR_CXX_FLAGS) -c $< -o bin/clients/$*.$(GR_OUTPUT_FORMAT)
	$(call GR_GENERATE_INIT_FUNC,bin/clients/$*.$(GR_OUTPUT_FORMAT))

# Granary rules for test files
bin/tests/%.o: tests/%.cc
	$(GR_CXX) $(GR_CXX_FLAGS) -c $< -o bin/tests/$*.$(GR_OUTPUT_FORMAT)
	$(call GR_GENERATE_INIT_FUNC,bin/tests/$*.$(GR_OUTPUT_FORMAT))


# Granary user space "harness" for testing compilation, etc. This is convenient
# for coding Granary on non-Linux platforms because it allows for debugging the
# build process, and partial testing of the code generation process.
bin/main.o: main.cc
	$(GR_CXX) $(GR_CXX_FLAGS) -c main.cc -o bin/main.$(GR_OUTPUT_FORMAT)


bin/dlmain.o: dlmain.cc
	$(GR_CXX) $(GR_CXX_FLAGS) -c dlmain.cc -o bin/dlmain.$(GR_OUTPUT_FORMAT)


# pre-process then post-process type information; this is used for wrappers,
# etc.
types:
	$(call GR_GET_TYPE_DEFINES)
	$(GR_TYPE_CC) $(GR_TYPE_CC_FLAGS) $(GR_TYPE_INCLUDE) -E $(GR_INPUT_TYPES) > /tmp/ppt.h
	python scripts/post_process_header.py /tmp/ppt.h > /tmp/ppt2.h
	python scripts/reorder_header.py /tmp/ppt2.h > $(GR_OUTPUT_TYPES)


# auto-generate wrappers
wrappers: types
	python scripts/generate_wrappers.py $(GR_OUTPUT_TYPES) > $(GR_OUTPUT_WRAPPERS)


# auto-generate the hash table stuff needed for wrappers and detaching
detach: types
	python scripts/generate_detach_table.py $(GR_OUTPUT_TYPES) granary/gen/detach.inc 
	$(call GR_GET_LD_LIBRARIES)


# make the folders where binaries / generated assemblies are stored
install:
	# Creating folders for binary/assembly files...
	@-mkdir bin &> /dev/null ||:
	@-mkdir bin/granary &> /dev/null ||:
	@-mkdir bin/granary/user &> /dev/null ||:
	@-mkdir bin/granary/kernel &> /dev/null ||:
	@-mkdir bin/granary/gen &> /dev/null ||:
	@-mkdir bin/granary/x86 &> /dev/null ||:
	@-mkdir bin/clients &> /dev/null ||:
	@-mkdir bin/tests &> /dev/null ||:
	@-mkdir bin/deps &> /dev/null ||:
	@-mkdir bin/deps/icxxabi &> /dev/null ||:
	@-mkdir bin/deps/dr &> /dev/null ||:
	@-mkdir bin/deps/dr/x86 &> /dev/null ||:
	@-mkdir bin/deps/murmurhash &> /dev/null ||:
	
	# Converting DynamoRIO INSTR_CREATE_ and OPND_CREATE_ macros into Granary
	# instruction-building functions...
	@-mkdir granary/gen &> /dev/null ||:
	python scripts/process_instr_create.py

	# Compile a dummy file to assembly to figure out which assembly syntax
	# to use for this compiler...
	$(GR_CC) -g3 -S -c scripts/static/asm.c -o scripts/static/asm.S
	python scripts/make_asm_defines.py


# Compile granary
all: $(GR_OBJS)
	$(GR_MAKE)


# Remove all generated files. This goes and @-touches some file in all of the
# directories to make sure cleaning doesn't report any errors; depending on
# the compilation mode (KERNEL=0/1), not all bin folders will have objects
clean:
	@-touch bin/_.$(GR_OUTPUT_FORMAT)
	@-touch bin/deps/icxxabi/_.$(GR_OUTPUT_FORMAT)
	@-touch bin/deps/dr/_.$(GR_OUTPUT_FORMAT)
	@-touch bin/deps/dr/x86/_.$(GR_OUTPUT_FORMAT)
	@-touch bin/deps/murmurhash/_.$(GR_OUTPUT_FORMAT)
	@-touch bin/granary/_.$(GR_OUTPUT_FORMAT)
	@-touch bin/granary/user/_.$(GR_OUTPUT_FORMAT)
	@-touch bin/granary/kernel/_.$(GR_OUTPUT_FORMAT)
	@-touch bin/granary/gen/_.$(GR_OUTPUT_FORMAT)
	@-touch bin/granary/x86/_.$(GR_OUTPUT_FORMAT)
	@-touch bin/clients/_.$(GR_OUTPUT_FORMAT)
	@-touch bin/tests/_.$(GR_OUTPUT_FORMAT)
	
	@-rm bin/*.$(GR_OUTPUT_FORMAT)
	@-rm bin/deps/icxxabi/*.$(GR_OUTPUT_FORMAT)
	@-rm bin/deps/dr/*.$(GR_OUTPUT_FORMAT)
	@-rm bin/deps/dr/x86/*.$(GR_OUTPUT_FORMAT)
	@-rm bin/deps/murmurhash/*.$(GR_OUTPUT_FORMAT)
	@-rm bin/granary/*.$(GR_OUTPUT_FORMAT)
	@-rm bin/granary/user/*.$(GR_OUTPUT_FORMAT)
	@-rm bin/granary/kernel/*.$(GR_OUTPUT_FORMAT)
	@-rm bin/granary/gen/*.$(GR_OUTPUT_FORMAT)
	@-rm bin/granary/x86/*.$(GR_OUTPUT_FORMAT)
	@-rm bin/clients/*.$(GR_OUTPUT_FORMAT)
	@-rm bin/tests/*.$(GR_OUTPUT_FORMAT)

	@-rm granary/gen/kernel_init.S ||:
	@-touch granary/gen/kernel_init.S

	$(GR_CLEAN)
