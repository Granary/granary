# Copyright 2012-2013 Peter Goodman, all rights reserved.

KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
PWD = $(shell pwd)
UNAME = $(shell uname)

# Conditional compilation for kernel code; useful for testing if Granary code
# compiles independent of the Linux kernel.
KERNEL ?= 1

# Config
GR_NAME = granary

# Client
GR_CLIENT ?= null

# Compilation toolchain
GR_CPP = cpp
GR_CC = gcc-4.8
GR_LD = gcc-4.8
GR_LDD = ldd
GR_CXX = g++-4.8
GR_CXX_STD = -std=gnu++0x
GR_PYTHON = python

# Later used commands/options
GR_MAKE =
GR_CLEAN =
GR_OUTPUT_FORMAT =

# Compilation options
GR_DEBUG_LEVEL = -g3 

# Optimisation level.
ifeq ($(KERNEL),0)
	GR_DEBUG_LEVEL += -O0
else
	GR_DEBUG_LEVEL += -O3
endif

GR_LD_PREFIX_FLAGS = 
GR_LD_SUFFIX_FLAGS = 
GR_ASM_FLAGS = -I$(PWD) -DGRANARY_IN_ASSEMBLY
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
GR_OUTPUT_SCANNERS =

GR_TYPE_INCLUDE = 

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

# Non-Clang; try to make libcxx work if specified.
else
	
	# Enable the newer standard and use it with libc++ by giving a path
	# to its library files.
	ifeq ('1','$(GR_LIBCXX)')
		GR_CXX_FLAGS += -I$(shell locate libcxx/include | head -n 1)
	endif
endif

ifneq (,$(findstring gcc,$(GR_CC))) # gcc
	GR_CC_FLAGS += -fdiagnostics-show-option
	GR_TYPE_CC_FLAGS += -fdiagnostics-show-option
	GR_CXX_FLAGS += -fdiagnostics-show-option -fno-check-new
endif

ifneq (,$(findstring icc,$(GR_CC))) # icc
	GR_CC_FLAGS += -diag-disable 188 -diag-disable 186 
	GR_CXX_FLAGS += -D__GXX_EXPERIMENTAL_CXX0X__
	GR_CXX_FLAGS += -D_GLIBCXX_USE_C99_STDINT_TR1
	GR_CXX_FLAGS += -Dnullptr="((void *) 0)"
	GR_CXX_STD = -std=c++11
endif

GR_CXX_FLAGS += $(GR_CXX_STD)
GR_OBJS = 
GR_MOD_OBJS =

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
GR_OBJS += bin/granary/trace_log.o
GR_OBJS += bin/granary/dynamic_wrapper.o
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
ifeq ($(GR_CLIENT),null)
	GR_CXX_FLAGS += -DCLIENT_NULL
	GR_OBJS += bin/clients/null/instrument.o
endif
ifeq ($(GR_CLIENT),null_plus)
	GR_CXX_FLAGS += -DCLIENT_NULL_PLUS
	GR_OBJS += bin/clients/null_plus/instrument.o
endif
ifeq ($(GR_CLIENT),track_entry_exit)
	GR_CXX_FLAGS += -DCLIENT_ENTRY
	GR_OBJS += bin/clients/track_entry_exit/instrument.o
endif
ifeq ($(GR_CLIENT),cfg)
	GR_CXX_FLAGS += -DCLIENT_CFG
	GR_OBJS += bin/clients/cfg/instrument.o
	GR_OBJS += bin/clients/cfg/events.o
	GR_OBJS += bin/clients/cfg/report.o
endif
ifeq ($(GR_CLIENT),watchpoint_null)
	GR_CXX_FLAGS += -DCLIENT_WATCHPOINT_NULL
	GR_OBJS += bin/clients/watchpoints/instrument.o
	GR_OBJS += bin/clients/watchpoints/clients/null/instrument.o
	GR_OBJS += bin/clients/watchpoints/clients/null/tests/test_mov.o
	GR_OBJS += bin/clients/watchpoints/clients/null/tests/test_xlat.o
	GR_OBJS += bin/clients/watchpoints/clients/null/tests/test_arithmetic.o
	GR_OBJS += bin/clients/watchpoints/clients/null/tests/test_string.o
	GR_OBJS += bin/clients/watchpoints/clients/null/tests/test_atomic.o
	GR_OBJS += bin/clients/watchpoints/clients/null/tests/test_cti.o
	GR_OBJS += bin/clients/watchpoints/clients/null/tests/test_push_pop.o
	GR_OBJS += bin/clients/watchpoints/clients/null/tests/test_random.o
	GR_OBJS += bin/clients/watchpoints/clients/null/tests/test_frame_pointer.o
	GR_OBJS += bin/clients/watchpoints/clients/null/tests/test_auto_data.o
	GR_OBJS += bin/clients/watchpoints/clients/null/tests/test_auto.o
	
	ifeq ($(KERNEL),1)
		GR_OBJS += bin/clients/watchpoints/kernel/interrupt.o
	endif
endif
ifeq ($(GR_CLIENT),watchpoint_stats)
	GR_CXX_FLAGS += -DCLIENT_WATCHPOINT_STATS 
	GR_OBJS += bin/clients/watchpoints/instrument.o
	GR_OBJS += bin/clients/watchpoints/clients/stats/instrument.o
	GR_OBJS += bin/clients/watchpoints/clients/stats/report.o
	
	ifeq ($(KERNEL),1)
		GR_OBJS += bin/clients/watchpoints/kernel/interrupt.o
	endif
endif
ifeq ($(GR_CLIENT),everything_watched)
	GR_CXX_FLAGS += -DCLIENT_WATCHPOINT_WATCHED
	GR_OBJS += bin/clients/watchpoints/instrument.o
	GR_OBJS += bin/clients/watchpoints/clients/everything_watched/instrument.o
	
	ifeq ($(KERNEL),1)
		GR_OBJS += bin/clients/watchpoints/kernel/interrupt.o
		GR_OBJS += bin/clients/watchpoints/kernel/linux/detach.o
	else
		GR_OBJS += bin/clients/watchpoints/user/posix/signal.o
	endif
endif
ifeq ($(GR_CLIENT),bounds_checker)
	GR_CXX_FLAGS += -DCLIENT_WATCHPOINT_BOUND
	GR_OBJS += bin/clients/watchpoints/instrument.o
	GR_OBJS += bin/clients/watchpoints/utils.o
	GR_OBJS += bin/clients/watchpoints/clients/bounds_checker/instrument.o
	GR_OBJS += bin/clients/watchpoints/clients/bounds_checker/bounds_checkers.o
	
	ifeq ($(KERNEL),1)
		GR_OBJS += bin/clients/watchpoints/kernel/interrupt.o
		GR_OBJS += bin/clients/watchpoints/kernel/linux/detach.o
	else
		GR_OBJS += bin/clients/watchpoints/user/posix/signal.o
	endif
endif
ifeq ($(GR_CLIENT),leak_detector)
    GR_CXX_FLAGS += -DCLIENT_WATCHPOINT_LEAK
    GR_OBJS += bin/clients/watchpoints/instrument.o
    GR_OBJS += bin/clients/watchpoints/clients/leak_detector/access_descriptor.o
    GR_OBJS += bin/clients/watchpoints/clients/leak_detector/instrument.o
    GR_OBJS += bin/clients/watchpoints/clients/leak_detector/descriptor.o
    GR_OBJS += bin/clients/watchpoints/clients/leak_detector/thread.o
    
    ifeq ($(KERNEL),1)
		GR_OBJS += bin/clients/watchpoints/kernel/interrupt.o
		GR_OBJS += bin/clients/watchpoints/kernel/linux/detach.o
		GR_MOD_OBJS += clients/watchpoints/clients/leak_detector/kernel/leakpolicy_scan.o
	else
		GR_OBJS += bin/clients/watchpoints/user/posix/signal.o
	endif
endif
ifeq ($(GR_CLIENT),shadow_memory)
    GR_CXX_FLAGS += -DCLIENT_SHADOW_MEMORY
    GR_OBJS += bin/clients/watchpoints/instrument.o
    GR_OBJS += bin/clients/watchpoints/clients/shadow_memory/shadow_update.o
    GR_OBJS += bin/clients/watchpoints/clients/shadow_memory/instrument.o
    GR_OBJS += bin/clients/watchpoints/clients/shadow_memory/descriptor.o
    GR_OBJS += bin/clients/watchpoints/clients/shadow_memory/thread.o
    GR_OBJS += bin/clients/watchpoints/clients/shadow_memory/shadow_report.o
    GR_OBJS += bin/clients/watchpoints/utils.o
    
    ifeq ($(KERNEL),1)
		GR_OBJS += bin/clients/watchpoints/kernel/interrupt.o
		GR_OBJS += bin/clients/watchpoints/kernel/linux/detach.o
	else
		GR_OBJS += bin/clients/watchpoints/user/posix/signal.o
	endif
endif
ifeq ($(GR_CLIENT),rcudbg)
    GR_CXX_FLAGS += -DCLIENT_RCUDBG
    GR_OBJS += bin/clients/watchpoints/instrument.o
    GR_OBJS += bin/clients/watchpoints/utils.o
    GR_OBJS += bin/clients/watchpoints/clients/rcudbg/carat.o
    GR_OBJS += bin/clients/watchpoints/clients/rcudbg/descriptor.o
    GR_OBJS += bin/clients/watchpoints/clients/rcudbg/events.o
    GR_OBJS += bin/clients/watchpoints/clients/rcudbg/instrument.o
    GR_OBJS += bin/clients/watchpoints/clients/rcudbg/log.o
    
    ifeq ($(KERNEL),1)
		GR_OBJS += bin/clients/watchpoints/kernel/interrupt.o
		GR_OBJS += bin/clients/watchpoints/kernel/linux/detach.o
	else
		GR_OBJS += bin/clients/watchpoints/user/posix/signal.o
	endif
endif

# Client-generated C++ files.
GR_CLIENT_GEN_OBJS = $(patsubst clients/%.cc,bin/clients/%.o,$(wildcard clients/gen/*.cc))
GR_OBJS += $(GR_CLIENT_GEN_OBJS)

# C++ ABI-specific stuff
GR_OBJS += bin/deps/icxxabi/icxxabi.o

# Granary tests.
GR_OBJS += bin/granary/test.o
GR_OBJS += bin/tests/test_direct_cbr.o
GR_OBJS += bin/tests/test_direct_call.o
GR_OBJS += bin/tests/test_lock_inc.o
GR_OBJS += bin/tests/test_direct_rec.o
GR_OBJS += bin/tests/test_indirect_cti.o

# user space
ifeq ($(KERNEL),0)
	GR_INPUT_TYPES = granary/user/posix/types.h
	GR_OUTPUT_TYPES = granary/gen/user_types.h
	GR_OUTPUT_WRAPPERS = granary/gen/user_wrappers.h
	GR_DETACH_FILE = granary/gen/user_detach.inc
	
	# user-specific versions of granary functions
	GR_OBJS += bin/granary/user/allocator.o
	GR_OBJS += bin/granary/user/state.o
	GR_OBJS += bin/granary/user/init.o
	GR_OBJS += bin/granary/user/printf.o
	
	ifneq ($(GR_DLL),1)
		GR_OBJS += bin/main.o
		GR_ASM_FLAGS += -DGRANARY_USE_PIC=0
		GR_CC_FLAGS += -DGRANARY_USE_PIC=0
		GR_CXX_FLAGS += -DGRANARY_USE_PIC=0
	else
		GR_OBJS += bin/dlmain.o
		GR_ASM_FLAGS += -fPIC -DGRANARY_USE_PIC=1
		GR_LD_PREFIX_FLAGS += -fPIC
		GR_CC_FLAGS += -fPIC -DGRANARY_USE_PIC=1 -fvisibility=hidden
		GR_CXX_FLAGS += -fPIC -DGRANARY_USE_PIC=1 -fvisibility=hidden
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
	GR_OBJS += bin/tests/test_mat_mul.o
	GR_OBJS += bin/tests/test_md5.o
	GR_OBJS += bin/tests/test_sigsetjmp.o

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
	GR_CC_FLAGS += -DGRANARY_IN_KERNEL=0 -mno-mmx 
	GR_CXX_FLAGS += -DGRANARY_IN_KERNEL=0 -mno-mmx 
	
	GR_MAKE += $(GR_CC) -c bin/deps/dr/x86/x86.S -o bin/deps/dr/x86/x86.o ; 
	GR_MAKE += $(GR_CC) $(GR_DEBUG_LEVEL) $(GR_LD_PREFIX_FLAGS) $(GR_OBJS)
	GR_MAKE += $(GR_LD_SUFFIX_FLAGS)
	GR_MAKE += -o $(GR_OUTPUT_PREFIX)$(GR_NAME)$(GR_OUTPUT_SUFFIX)
	GR_CLEAN =
	GR_OUTPUT_FORMAT = o

	# Compile an assembly file to an object file in user space. In kernel space,
	# the kernel makefile will do this for us.
	define GR_COMPILE_ASM
		$$($(GR_CC) $(GR_ASM_FLAGS) -c $1.S -o $1.o)
endef

	define GR_GENERATE_INIT_FUNC
endef
	
	# Get a list of dynamically loaded libraries from the compiler being used.
	# This is so that we can augment the detach table with internal libc symbols,
	# etc.
	define GR_GET_LD_LIBRARIES
		$$($(GR_LDD) $(shell which $(GR_CC)) | $(GR_PYTHON) scripts/generate_dll_detach_table.py >> $(GR_DETACH_FILE))
endef

# kernel space
else
	GR_COMMON_KERNEL_FLAGS = -DGRANARY_IN_KERNEL=1 -DGRANARY_USE_PIC=0
	
	ifneq (,$(findstring clang,$(GR_CC))) # clang
		GR_COMMON_KERNEL_FLAGS += -mkernel -mcmodel=kernel -fno-builtin
		GR_COMMON_KERNEL_FLAGS += -ffreestanding
	else # gcc
		GR_COMMON_KERNEL_FLAGS += -mcmodel=kernel -maccumulate-outgoing-args
		GR_COMMON_KERNEL_FLAGS += -nostdlib -nostartfiles -nodefaultlibs
	endif
	
	# Common flags to disable certain user space features for the kernel
	GR_COMMON_KERNEL_FLAGS += -mno-red-zone -mno-3dnow -fno-stack-protector
	GR_COMMON_KERNEL_FLAGS += -mno-sse -mno-sse2 -mno-mmx -m64 -march=native
	GR_COMMON_KERNEL_FLAGS += -fno-asynchronous-unwind-tables -fno-common
	
	GR_INPUT_TYPES = granary/kernel/linux/types.h
	GR_OUTPUT_TYPES = granary/gen/kernel_types.h
	GR_OUTPUT_WRAPPERS = granary/gen/kernel_wrappers.h
	GR_DETACH_FILE = granary/gen/kernel_detach.inc
	GR_OUTPUT_SCANNERS = clients/gen/kernel_type_scanners.h
	
	# Kernel-specific test cases.
	GR_OBJS += bin/tests/test_interrupt_delay.o
	
	# Kernel-specific versions of granary functions.
	GR_OBJS += bin/granary/kernel/linux/module.o
	GR_OBJS += bin/granary/kernel/linux/state.o
	GR_OBJS += bin/granary/kernel/hotpatch.o
	GR_OBJS += bin/granary/kernel/state.o
	GR_OBJS += bin/granary/kernel/interrupt.o
	GR_OBJS += bin/granary/kernel/allocator.o
	GR_OBJS += bin/granary/kernel/printf.o
	GR_OBJS += bin/granary/kernel/init.o

	# Must be last!!!!
	GR_OBJS += bin/granary/x86/init.o
	
	# Extra C and assembler flags.
	cflags-y = $(GR_DEBUG_LEVEL) -Wl,-flat_namespace
	asflags-y = $(GR_DEBUG_LEVEL) -Wl,-flat_namespace
	
	# Module objects.
	obj-m += $(GR_NAME).o
	$(GR_NAME)-objs = $(GR_OBJS) $(GR_MOD_OBJS) module.o
	
	GR_MAKE = make -C $(KERNEL_DIR) M=$(PWD) modules
	GR_CLEAN = make -C $(KERNEL_DIR) M=$(PWD) clean
	GR_OUTPUT_FORMAT = S
	
	GR_TYPE_CC_FLAGS += $(GR_COMMON_KERNEL_FLAGS)
	GR_TYPE_CXX_FLAGS += $(GR_COMMON_KERNEL_FLAGS)
	GR_ASM_FLAGS += -DGRANARY_IN_KERNEL=1 -DGRANARY_USE_PIC=0
	GR_CC_FLAGS += $(GR_COMMON_KERNEL_FLAGS) -S
	GR_CXX_FLAGS += $(GR_COMMON_KERNEL_FLAGS) -S
	
	GR_TYPE_INCLUDE = -I./ -isystem $(KERNEL_DIR)/include
	GR_TYPE_INCLUDE += -isystem $(KERNEL_DIR)/arch/x86/include
	GR_TYPE_INCLUDE += -isystem $(KERNEL_DIR)/arch/x86/include/generated 
	
	define GR_COMPILE_ASM
endef
	
	# Parse an assembly file looking for functions that must be called
	# in order to initialise the static global data.
	define GR_GENERATE_INIT_FUNC
		$$($(GR_PYTHON) scripts/generate_init_func.py $1 >> granary/gen/kernel_init.S)
endef

	# get the addresses of kernel symbols
	define GR_GET_LD_LIBRARIES
		$$($(GR_PYTHON) scripts/generate_detach_addresses.py $(GR_DETACH_FILE))
endef
endif

GR_CC_FLAGS += $(GR_EXTRA_CC_FLAGS)
GR_CXX_FLAGS += $(GR_EXTRA_CXX_FLAGS)


# MumurHash3 rules for C++ files
bin/deps/murmurhash/%.o: deps/murmurhash/%.cc
	@echo "  CXX [MURMUR] $<"
	@$(GR_CXX) $(GR_CXX_FLAGS) -c $< -o bin/deps/murmurhash/$*.$(GR_OUTPUT_FORMAT)


# DynamoRIO rules for C files
bin/deps/dr/%.o: deps/dr/%.c
	@echo "  CC  [DR] $<"
	@$(GR_CC) $(GR_CC_FLAGS) -c $< -o bin/deps/dr/$*.$(GR_OUTPUT_FORMAT)


# DynamoRIO rules for assembly files
bin/deps/dr/%.o: deps/dr/%.asm
	@echo "  AS  [DR] $<"
	@$(GR_CC) $(GR_ASM_FLAGS) -E -o bin/deps/dr/$*.1.S -x c -std=c99 $<
	@$(GR_PYTHON) scripts/post_process_asm.py bin/deps/dr/$*.1.S > bin/deps/dr/$*.S
	@rm bin/deps/dr/$*.1.S


# Itanium C++ ABI rules for C++ files
bin/deps/icxxabi/%.o: deps/icxxabi/%.cc
	@echo "  CXX [ABI] $<"
	@$(GR_CXX) $(GR_CXX_FLAGS) -c $< -o bin/deps/icxxabi/$*.$(GR_OUTPUT_FORMAT)


# Granary rules for C++ files
bin/granary/%.o: granary/%.cc
	@echo "  CXX [GR] $<"
	@$(GR_CXX) $(GR_CXX_FLAGS) -c $< -o bin/granary/$*.$(GR_OUTPUT_FORMAT)
	@$(call GR_GENERATE_INIT_FUNC,bin/granary/$*.$(GR_OUTPUT_FORMAT))


# Granary rules for assembly files
bin/granary/x86/%.o: granary/x86/%.asm
	@echo "  AS  [GR] $<"
	@$(GR_CC) $(GR_ASM_FLAGS) -E -o bin/granary/x86/$*.1.S -x c -std=c99 $<
	@$(GR_PYTHON) scripts/post_process_asm.py bin/granary/x86/$*.1.S > bin/granary/x86/$*.S
	@rm bin/granary/x86/$*.1.S
	@$(call GR_COMPILE_ASM,bin/granary/x86/$*)


# Granary rules for client C++ files
bin/clients/%.o :: clients/%.cc
	@echo "  CXX [GR-CLIENT] $<"
	@$(GR_CXX) $(GR_CXX_FLAGS) -c $< -o bin/clients/$*.$(GR_OUTPUT_FORMAT)
	@$(call GR_GENERATE_INIT_FUNC,bin/clients/$*.$(GR_OUTPUT_FORMAT))


# Granary rules for client assembly files
bin/clients/%.o :: clients/%.asm
	@echo "  AS  [GR-CLIENT] $<"
	@$(GR_CC) $(GR_ASM_FLAGS) -E -o bin/clients/$*.1.S -x c -std=c99 $<
	@$(GR_PYTHON) scripts/post_process_asm.py bin/clients/$*.1.S > bin/clients/$*.S
	@rm bin/clients/$*.1.S
	@$(call GR_COMPILE_ASM,bin/clients/$*)


# Granary rules for test files
bin/tests/%.o: tests/%.cc
	@echo "  CXX [GR-TEST] $<"
	@$(GR_CXX) $(GR_CXX_FLAGS) -c $< -o bin/tests/$*.$(GR_OUTPUT_FORMAT)
	@$(call GR_GENERATE_INIT_FUNC,bin/tests/$*.$(GR_OUTPUT_FORMAT))


# Granary user space "harness" for testing compilation, etc. This is convenient
# for coding Granary on non-Linux platforms because it allows for debugging the
# build process, and inherited testing of the code generation process.
bin/main.o: main.cc
	@echo "  CXX [GR] $<"
	@$(GR_CXX) $(GR_CXX_FLAGS) -c main.cc -o bin/main.$(GR_OUTPUT_FORMAT)


bin/dlmain.o: dlmain.cc
	@echo "  CXX [GR] $<"
	@$(GR_CXX) $(GR_CXX_FLAGS) -c dlmain.cc -o bin/dlmain.$(GR_OUTPUT_FORMAT)


# pre-process then post-process type information; this is used for wrappers,
# etc.
types:
	@echo "  TYPES [GR] $(GR_OUTPUT_TYPES)"
	@$(GR_TYPE_CC) $(GR_TYPE_CC_FLAGS) $(GR_TYPE_INCLUDE) -E $(GR_INPUT_TYPES) > /tmp/ppt.h
	@$(GR_PYTHON) scripts/post_process_header.py /tmp/ppt.h > /tmp/ppt2.h
	@$(GR_PYTHON) scripts/reorder_header.py /tmp/ppt2.h > $(GR_OUTPUT_TYPES)


# auto-generate wrappers
wrappers: types
	@echo "  WRAPPERS [GR] $(GR_OUTPUT_WRAPPERS)"
	@$(GR_PYTHON) scripts/generate_wrappers.py $(GR_OUTPUT_TYPES) > $(GR_OUTPUT_WRAPPERS)
ifeq ($(GR_CLIENT),watchpoint_leak)
	@echo "  SCANNERS [GR] $(GR_OUTPUT_SCANNERS)"
	@$(GR_PYTHON) scripts/generate_scanners.py $(GR_OUTPUT_TYPES) > $(GR_OUTPUT_SCANNERS)
endif	


# auto-generate the hash table stuff needed for wrappers and detaching
detach: types
	@echo "  DETACH [GR] $(GR_DETACH_FILE)"
	@$(GR_PYTHON) scripts/generate_detach_table.py $(GR_OUTPUT_TYPES) $(GR_DETACH_FILE)
	@$(call GR_GET_LD_LIBRARIES)


# Make the folders where binaries / generated assemblies are stored. If new
# folders are added, then this step likely needs to be repeated.
env:
	@echo "  [1] Creating directories for binary/assembly files."
	@-mkdir bin > /dev/null 2>&1 ||:
	@-mkdir bin/granary > /dev/null 2>&1 ||:
	@-mkdir bin/granary/user > /dev/null 2>&1 ||:
	@-mkdir bin/granary/kernel > /dev/null 2>&1 ||:
	@-mkdir bin/granary/kernel/linux > /dev/null 2>&1 ||:
	@-mkdir bin/granary/gen > /dev/null 2>&1 ||:
	@-mkdir bin/granary/x86 > /dev/null 2>&1 ||:
	@-mkdir bin/clients > /dev/null 2>&1 ||:
	@-mkdir bin/clients/gen > /dev/null 2>&1 ||:
	@-mkdir bin/clients/null > /dev/null 2>&1 ||:
	@-mkdir bin/clients/null_plus > /dev/null 2>&1 ||:
	@-mkdir bin/clients/track_entry_exit > /dev/null 2>&1 ||:
	@-mkdir bin/clients/cfg > /dev/null 2>&1 ||:
	@-mkdir bin/clients/watchpoints > /dev/null 2>&1 ||:
	@-mkdir bin/clients/watchpoints/clients > /dev/null 2>&1 ||:
	@-mkdir bin/clients/watchpoints/user > /dev/null 2>&1 ||:
	@-mkdir bin/clients/watchpoints/user/posix > /dev/null 2>&1 ||:
	@-mkdir bin/clients/watchpoints/kernel > /dev/null 2>&1 ||:
	@-mkdir bin/clients/watchpoints/kernel/linux > /dev/null 2>&1 ||:
	@-mkdir bin/clients/watchpoints/clients/null/ > /dev/null 2>&1 ||:
	@-mkdir bin/clients/watchpoints/clients/null/tests > /dev/null 2>&1 ||:
	@-mkdir bin/clients/watchpoints/clients/stats/ > /dev/null 2>&1 ||:
	@-mkdir bin/clients/watchpoints/clients/shadow_memory > /dev/null 2>&1 ||:
	@-mkdir bin/clients/watchpoints/clients/rcudbg > /dev/null 2>&1 ||:
	@-mkdir bin/clients/watchpoints/clients/leak_detector > /dev/null 2>&1 ||:
	@-mkdir bin/clients/watchpoints/clients/leak_detector/kernel > /dev/null 2>&1 ||:
	@-mkdir bin/clients/watchpoints/clients/everything_watched > /dev/null 2>&1 ||:
	@-mkdir bin/clients/watchpoints/clients/bounds_checker > /dev/null 2>&1 ||:
	
	@-mkdir bin/tests > /dev/null 2>&1 ||:
	@-mkdir bin/deps > /dev/null 2>&1 ||:
	@-mkdir bin/deps/icxxabi > /dev/null 2>&1 ||:
	@-mkdir bin/deps/dr > /dev/null 2>&1 ||:
	@-mkdir bin/deps/dr/x86 > /dev/null 2>&1 ||:
	@-mkdir bin/deps/murmurhash > /dev/null 2>&1 ||:
	
	@echo "  [2] Converting DynamoRIO INSTR_CREATE_ and OPND_CREATE_ macros into"
	@echo "      Granary-compatible instruction-building functions."
	@-mkdir granary/gen > /dev/null 2>&1 ||:
	@-mkdir clients/gen > /dev/null 2>&1 ||:
	@$(GR_PYTHON) scripts/process_instr_create.py

	@echo "  [3] Determining the supported assembly syntax of $(GR_CC)."
	@$(GR_CC) -g3 -S -c scripts/static/asm.c -o scripts/static/asm.S
	@$(GR_PYTHON) scripts/make_asm_defines.py
	
	@echo "  [.] The current directory is now setup for compiling Granary."


# Compile granary
all: $(GR_OBJS)
	@$(GR_MAKE)
	@echo "  [.] Granary has been built."


# Remove all generated files. This goes and @-touches some file in all of the
# directories to make sure cleaning doesn't report any errors; depending on
# the compilation mode (KERNEL=0/1), not all bin folders will have objects
clean:
	@find bin -name \*.o -execdir rm {} \;
	@find bin -name \*.o.cmd -execdir rm {} \;
	@find bin -name \*.S -execdir rm {} \;
	@find bin -name \*.su -execdir rm {} \;

	@-rm granary/gen/kernel_init.S ||:
	@-touch granary/gen/kernel_init.S

	$(GR_CLEAN)
