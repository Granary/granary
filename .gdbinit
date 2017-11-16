set logging off
set breakpoint pending on
set print demangle on
set print asm-demangle on
set print object on
set print static-members on
set disassembly-flavor intel
set language c++


# set-user-detect
#
# Uses Python support to set the variable `$in_user_space`
# to `0` or `1` depending on whether we are instrumenting in
# user space or kernel space, respectively.
define set-user-detect
  python None ; \
    gdb.execute( \
      "set $in_user_space = %d" % int(None is not gdb.current_progspace().filename), \
      from_tty=True, to_string=True)
end


# Detect if we're in user space and set `$in_user_space`
# appropriately.
set-user-detect


# Kernel setup
if !$in_user_space
  file ~/Code/linux-4.4.0/vmlinux
  target remote : 9999
  source ~/Code/granary/granary.syms
end


# Granary breakpoints
catch throw
b granary_fault
b granary_break_on_fault
b granary_break_on_translate
b granary_break_on_curiosity

# Kernel breakpoints
if !$in_user_space
  #b granary_break_on_interrupt
  b granary_break_on_nested_interrupt
  b granary_break_on_gp_in_granary
  #b granary_break_on_nested_task
  b granary_break_on_gs_zero
  b panic
  b show_fault_oops
  b invalid_op
  b do_invalid_op
  b do_general_protection
  b __schedule_bug
  b __stack_chk_fail
  b __die
  b do_spurious_interrupt_bug
  b report_bug
  b dump_stack
  b show_stack
  b show_trace
  b show_trace_log_lvl
  b do_simd_coprocessor_error
  #b kernel/hung_task.c:101
else
  b __assert_fail
end


# Just re-affirm it.
set language c++


# Simplify user-space debugging of a DLL using by doing the proper
# environment setup.
define dll
  set env LD_PRELOAD=./bin/libgranary.so
end


# p-module-of <module address>
#
# Given a native address within a module that Granary instruments,
# print the name of the module, and the offset within the module
# of the module address.
define p-module-of
  set language c++
  set $__func_address = (unsigned long) $arg0
  set $__module = (struct kernel_module *) LOADED_MODULES
  set $__module_name = (const char *) 0
  set $__module_begin = 0ull
  set $__module_offset = 0ull

  while $__module
    set $__this_module_begin = (unsigned long) $__module->text_begin
    set $__this_module_end = (unsigned long) $__module->max_text_end
    
    if $__func_address >= $__this_module_begin
      if $__func_address < $__this_module_end
        set $__module_begin = $__this_module_begin
        set $__module_name = $__module->name
      end
    end

    if $__module_begin
      set $__module = 0
    else
      set $__module = $__module->next
    end
  end

  if $__module_begin
    set $__module_offset = $__func_address - $__module_begin
    printf "Module containing address 0x%lx:\n", $__func_address
    printf "   Module name: %s\n", $__module_name
    printf "   Relative offset in module '.text' section: 0x%lx\n", $__module_offset
  else
    print "No Granary-instrumented module contains address 0x%lx\n", $__func_address
  end

  dont-repeat
end


# get-bb-info ADDR
#
# Set the variable $bb to point to a `granary::basic_block_info`
# structure. This scans the bytes following ADDR until it discovers
# the correct magic value.
define get-bb-info
  set language c++
  set $bb = 0

  # Find the fragment locator
  set $__addr = (unsigned long) $arg0
  set $__pc = (granary::app_pc) $arg0
  set $__base_addr = (unsigned long) GRANARY_EXEC_START
  set $__offs = $__addr - $__base_addr
  set $__index = $__offs / granary::SLAB_SIZE
  set $__locator = (granary::fragment_locator *) granary::detail::FRAGMENT_SLABS[$__index]

  # Binary search in the locator to find the fragment's
  # basic block info.

  set $__first = 0
  set $__last = $__locator->next_index - 1
  set $__mid = ($__first + $__last) / 2
  set $bb = 0
  set $trace = 0
  
  while !$bb && !$trace && ($__first <= $__mid) && ($__mid <= $__last)
    set $__curr = $__locator->fragments[$__mid]
  
    if $__curr.is_trace
      set $__trace = (granary::trace_info *) (((uintptr_t) $__curr.trace) & ~0x1ULL)

      if $__trace->start_pc <= $__pc

        # Found the fragment
        if $__pc < ($__trace->start_pc + $__trace->num_bytes)
          set $trace = $__trace

        # Not in lower half
        else
          set $__first = $__mid + 1
        end
      
      # Not in upper half
      else
        set $__last = $__mid - 1
      end

    else
      set $__block = $__curr.block
      if $__block->start_pc <= $__pc

        # Found the fragment
        if $__pc < ($__block->start_pc + $__block->num_bytes)
          set $bb = $__block

        # Not in lower half
        else
          set $__first = $__mid + 1
        end

      # Not in upper half
      else
        set $__last = $__mid - 1
      end
    end

    set $__mid = ($__first + $__last) / 2
  end

  if $trace
    set $__i = 0
    while $__i < $trace->num_blocks && !$bb
      set $__bb = &($trace->info[$__i])
      if $__bb->start_pc <= $__pc && $__pc < ($__bb->start_pc + $__bb->num_bytes)
        set $bb = $__bb
      end
      set $__i = $__i + 1
    end
  end

  dont-repeat
end


# unmangle-address ADDR
#
# Returns an unmangled version of ADDR.
define unmangle-address
  if $in_user_space
    set $unmangled_address = ~(0xFFFFULL << 48) & $arg0
  else
    set $unmangled_address = (0xFFFFULL << 48) | $arg0
  end
end


# p-bb-info ADDR
#
# Print the basic block info structure information for the basic block
# whose meta-information is located at address ADDR.
define p-bb-info
  set language c++

  # Find the basic block.
  get-bb-info $arg0

  p-bb-info-impl
end
define p-bb-info-impl
  
  set $bb = $arg0
  set $__bb = $bb
  set $__bb_addr = (unsigned long) $bb 

  set $__policy_addr = &($__bb->generating_pc.as_policy_address.policy_bits)
  set $__policy = ((granary::instrumentation_policy *) $__policy_addr)
  set $__gen_pc = $__bb->generating_pc.as_uint

  unmangle-address $__gen_pc
  set $__gen_pc = $unmangled_address

  # Print the module info for the app pc of this basic block.
  if !$in_user_space
    p-module-of $__bb->generating_pc
    printf "\n"
  end

  # Print the info.
  printf "Basic block info:\n"
  printf "   State: %p\n", (void *) $__bb->state
  printf "   App:\n"
  printf "      Code: %p\n", (void *) $__gen_pc
  printf "      Num instructions: %d\n", $__bb->generating_num_instructions
  printf "   Code cache:\n"
  printf "      Code: %p\n", (void *) $__bb->start_pc
  printf "      Num instructions: %d\n", $__bb->num_instructions
  printf "   Policy properties:\n"
  printf "      Is in XMM context: %d\n", $__policy->u.is_in_xmm_context
  printf "      Is in host context: %d\n", $__policy->u.is_in_host_context
  printf "      Accesses user data: %d\n", $__policy->u.accesses_user_data
  printf "      Return address in code cache: %d\n", $__policy->u.can_direct_return
  printf "   Num blocks in trace: %d\n", $__bb->num_bbs_in_trace
  if !$in_user_space
    printf "   Exception table entry: %p\n", $__bb->user_exception_metadata
  end
  printf "   Policy ID: %d\n", $__policy->u.id
  printf "   Instrumentation Function:"
  printf "\n      "
  if 1 == $__policy->u.is_in_host_context
    info sym (void *) granary::instrumentation_policy::HOST_VISITORS[$__policy->u.id]
  else
    info sym (void *) granary::instrumentation_policy::APP_VISITORS[$__policy->u.id]
  end

  dont-repeat
end


# p-bt <num>
#
# Print a backtrace.
def p-bt
  set $__num_frames = $arg0
  set $__rsp = (uint64_t *) $rsp
  set $__last_addr = 0
  set $__max_trackback = 200
  while $__num_frames > 0 && $__max_trackback > 0
    set $__max_trackback = $__max_trackback - 1
    set $__addr = *$__rsp
    if GRANARY_EXEC_START <= $__addr && $__addr < granary::detail::CODE_CACHE_END && $__addr != $__last_addr
      set $__last_addr = $__addr

      printf "Code Cache Address: %lx\n", $__addr
      get-bb-info $__addr

      set $__gen_pc = $__bb->generating_pc.as_uint
      unmangle-address $__gen_pc
      info sym $unmangled_address
      set $__num_frames = $__num_frames - 1
      printf "\n"

    end
    set $__rsp = $__rsp + 1
  end
end


# x-ins START NUM
#
# Examine NUM instructions starting from START
define x-ins
  set $__start = (uint64_t) $arg0
  set $__len = (int) $arg1

  while $__len > 0
    set $__len = $__len - 1
    set $__orig_start = $__start

    python None ; \
      gdb.execute( \
        "x/2i $__start\n", \
        from_tty=True, to_string=True) ; \
      gdb.execute( \
        "set $__start = $_\n", \
        from_tty=True, to_string=True)

    x/i $__orig_start
  end

  dont-repeat
end


# p-bb ADDR
#
# Print the info and instructions of a basic block given the address
# of an instruction in the basic block.
define p-bb
  set language c++

  set $__num_stub_ins = 0
  set $__num_trans_ins = 0
  set $__num_ins = 0

  get-bb-info $arg0
  set $__bb_info = $bb
  set $__bb_start = $__bb_info->start_pc

  printf "Translated instructions:\n"
  x-ins $__bb_info->start_pc $__bb_info->num_instructions
  printf "\n"

  unmangle-address $__bb_info->generating_pc.as_uint
  set $__in_start_pc = $unmangled_address

  printf "Original instructions:\n"
  x-ins $__in_start_pc $__bb_info->generating_num_instructions
  printf "\n"

  p-bb-info-impl $__bb_info
  
  dont-repeat
end


# p-wrapper ID
#
# Prints the information about a function wrapper with id ID.
define p-wrapper
  set language c++
  set $__w = &(granary::FUNCTION_WRAPPERS[(int) $arg0])
  printf "Function wrapper %s (%d):\n", $__w->name, (int) $arg0
  printf "   Original address: %p\n", $__w->original_address
  printf "   App Wrapper address: %p\n", $__w->app_wrapper_address
  printf "   Host Wrapper address: %p\n", $__w->host_wrapper_address
  dont-repeat
end


# p-trace N
#
# Prints at most N elements from the trace log.
define p-trace
  set language c++
  set $__i = (int) $arg0
  set $__j = 1
  set $__head = (granary::trace_log_item *) granary::TRACE
  printf "Global code cache lookup trace:\n"
  while $__i > 0 && 0 != $__head
    printf "   [%d] %p\n", $__j, $__head->code_cache_addr
    set $__j = $__j + 1
    set $__i = $__i - 1
    set $__head = $__head->prev
  end
  dont-repeat
end


# get-trace-entry N
#
# Gets a pointer to the Nth trace entry from the trace log and
# put its into the $trace_entry variable.
define get-trace-entry
  set language c++
  set $trace_entry = (granary::trace_log_item *) 0
  set $__i = (int) $arg0
  set $__head = (granary::trace_log_item *) granary::TRACE
  while $__i > 0 && 0 != $__head
    if $__i == 1
      set $trace_entry = $__head
    end
    set $__i = $__i - 1
    set $__head = $__head->prev
  end
  dont-repeat
end


# p-trace-entry N
#
# Prints a specific entry from the trace log. The most recent entry
# is entry 1.
define p-trace-entry
  set language c++
  get-trace-entry $arg0
  printf "Global code cache lookup trace:\n"
  printf "   [%d] %p\n", $arg0, $trace_entry->code_cache_addr
  dont-repeat
end


# p-trace-entry-bb N
#
# Prints the basic block for the Nth trace log entry.
define p-trace-entry-bb
  set language c++
  get-trace-entry $arg0
  p-bb $trace_entry->code_cache_addr
  dont-repeat
end


# p-trace-entry-regs N
#
# Prints the value of the registers on entry to the Nth trace log entry.
define p-trace-entry-regs
  set language c++
  get-trace-entry $arg0
  printf "Regs:\n"
  printf "   r15: 0x%lx\n", $trace_entry->state.r15.value_64
  printf "   r14: 0x%lx\n", $trace_entry->state.r14.value_64
  printf "   r13: 0x%lx\n", $trace_entry->state.r13.value_64
  printf "   r12: 0x%lx\n", $trace_entry->state.r12.value_64
  printf "   r11: 0x%lx\n", $trace_entry->state.r11.value_64
  printf "   r10: 0x%lx\n", $trace_entry->state.r10.value_64
  printf "   r9:  0x%lx\n", $trace_entry->state.r9.value_64
  printf "   r8:  0x%lx\n", $trace_entry->state.r8.value_64
  printf "   rdi: 0x%lx\n", $trace_entry->state.rdi.value_64
  printf "   rsi: 0x%lx\n", $trace_entry->state.rsi.value_64
  printf "   rbp: 0x%lx\n", $trace_entry->state.rbp.value_64
  printf "   rbx: 0x%lx\n", $trace_entry->state.rbx.value_64
  printf "   rdx: 0x%lx\n", $trace_entry->state.rdx.value_64
  printf "   rcx: 0x%lx\n", $trace_entry->state.rcx.value_64
  printf "   rax: 0x%lx\n", $trace_entry->state.rax.value_64
  printf "   rsp: 0x%lx\n", $trace_entry->state.rsp.value_64
  dont-repeat
end


# get-trace-cond-reg <min_trace> <str_reg_name> <str_bin_op> <int_value>
#
# Gets a pointer to a trace entry whose register meets a condition
# specified by `str_reg_name str_bin_op int_value` and where the
# trace number (1-indexed) is greater-than or equal to the `min_trace`.
#
# Puts its results into `$trace_entry` and `$trace_entry_num`.
define get-trace-cond-reg
  set language c++
  set $__min_trace = (int) $arg0
  set $__str_reg_name = $arg1
  set $__str_bin_op = $arg2
  set $__int_value = (unsigned long) $arg3
  set $trace_entry = 0
  set $trace_entry_num = 0
  set $__i = 1
  set $__head = (granary::trace_log_item *) granary::TRACE._M_b._M_p
  set $__reg_value = 0
  set $__cond = 0

  while 0 != $__head
    if $__i >= $__min_trace

      # Evaluate the condition using Python
      python None ; \
        reg = str(gdb.parse_and_eval("$__str_reg_name")).lower()[1:-1] ; \
        bin_op = str(gdb.parse_and_eval("$__str_bin_op"))[1:-1] ; \
        gdb.execute( \
          "set $__reg_value = $__head->state.%s.value_64\n" % reg, \
          from_tty=True, to_string=True) ; \
        gdb.execute( \
          "set $__cond = !!($__reg_value %s $__int_value)\n" % bin_op, \
          from_tty=True, to_string=True)

      if $__cond
        set $trace_entry = $__head
        set $trace_entry_num = $__i
      end
    end

    # Either go to the next trace entry or bail out.
    set $__i = $__i + 1
    if !$trace_entry
      set $__head = $__head->prev
    else
      set $__head = 0
    end

  end
  dont-repeat
end


# internal-bb-by-reg-cond
#
# Internal command only; prints out the basic block for a condition-
# based register lookup.
define internal-bb-by-reg-cond
  if $trace_entry
    set $__str_reg_name = $arg0
    set $__str_bin_op = $arg1
    set $__int_value = (unsigned long) $arg2

    python None ; \
      reg = str(gdb.parse_and_eval("$__str_reg_name")).lower()[1:-1] ; \
      bin_op = str(gdb.parse_and_eval("$__str_bin_op"))[1:-1] ; \
      val = str(gdb.parse_and_eval("$__int_value")).lower() ; \
      print "Global code cache lookup where %s %s %s:" % ( \
        reg, bin_op, val)

    printf "   [%d] %p\n\n", $trace_entry_num, $trace_entry->code_cache_addr

    p-trace-entry-regs $trace_entry_num
    printf "\n"
    p-bb $trace_entry->code_cache_addr
  end
end


# p-bb-where-reg <str_reg_name> <str_bin_op> <int_value>
#
# Prints out the basic block where the register condition specified
# by `str_reg_name str_bin_op int_value` is satisfied.
define p-bb-where-reg
  set $__str_reg_name = $arg0
  set $__str_bin_op = $arg1
  set $__int_value = (unsigned long) $arg2
  get-trace-cond-reg 1 $arg0 $arg1 $arg2
  internal-bb-by-reg-cond $__str_reg_name $__str_bin_op $__int_value
end


# p-next-bb-where-reg <str_reg_name> <str_bin_op> <int_value>
#
# Prints out the next basic block where the register condition
# specified by `str_reg_name str_bin_op int_value` is satisfied.
# This assumes that either `p-bb-by-reg-cond` or
# `p-next-bb-by-reg-cond` has been issued.
define p-next-bb-where-reg
  set $__str_reg_name = $arg0
  set $__str_bin_op = $arg1
  set $__int_value = (unsigned long) $arg2
  if 0 < $trace_entry_num
    set $__i = $trace_entry_num + 1
    get-trace-cond-reg $__i $__str_reg_name $__str_bin_op $__int_value
    internal-bb-by-reg-cond $__str_reg_name $__str_bin_op $__int_value
  end
end


# p-operand <addr>
#
# Pretty-print an operand object.
define p-operand
  set language c++
  set $__op = (opnd_t *) $arg0
  
  if $__op->kind == dynamorio::BASE_DISP_kind

  end

  if $__op->kind == dynamorio::REL_ADDR_kind

  end

  if $__op->kind == dynamorio::INSTR_kind

  end
end


# p-instr <addr>
#
# Pretty-print an instruction object.
define p-instr
  set language c++
  set $__instr = (instr_t *) $arg0
  printf "Instruction:\n"
  p (op_code_type) $__instr->opcode
  dont-repeat
end


# Saved machine state.
set $__reg_r15 = 0
set $__reg_r14 = 0
set $__reg_r13 = 0
set $__reg_r12 = 0
set $__reg_r11 = 0
set $__reg_r10 = 0
set $__reg_r9  = 0
set $__reg_r8  = 0
set $__reg_rdi = 0
set $__reg_rsi = 0
set $__reg_rbp = 0
set $__reg_rbx = 0
set $__reg_rdx = 0
set $__reg_rcx = 0
set $__reg_rax = 0
set $__reg_rsp = 0
set $__reg_eflags = 0
set $__reg_rip = 0
set $__regs_saved = 0


# Save all of the registers to some global GDB variables.
define save-regs
  set $__regs_saved = 1
  set $__reg_r15 = $r15
  set $__reg_r14 = $r14
  set $__reg_r13 = $r13
  set $__reg_r12 = $r12
  set $__reg_r11 = $r11
  set $__reg_r10 = $r10
  set $__reg_r9  = $r9 
  set $__reg_r8  = $r8 
  set $__reg_rdi = $rdi
  set $__reg_rsi = $rsi
  set $__reg_rbp = $rbp
  set $__reg_rbx = $rbx
  set $__reg_rdx = $rdx
  set $__reg_rcx = $rcx
  set $__reg_rax = $rax
  set $__reg_rsp = $rsp
  set $__reg_eflags = $eflags
  set $__reg_rip = $rip
  dont-repeat
end


# Restore all of the registers from some global GDB variables.
define restore-regs
  set $__regs_saved = 0
  set $r15 = $__reg_r15
  set $r14 = $__reg_r14
  set $r13 = $__reg_r13
  set $r12 = $__reg_r12
  set $r11 = $__reg_r11
  set $r10 = $__reg_r10
  set $r9  = $__reg_r9 
  set $r8  = $__reg_r8 
  set $rdi = $__reg_rdi
  set $rsi = $__reg_rsi
  set $rbp = $__reg_rbp
  set $rbx = $__reg_rbx
  set $rdx = $__reg_rdx
  set $rcx = $__reg_rcx
  set $rax = $__reg_rax
  set $rsp = $__reg_rsp
  set $eflags = $__reg_eflags
  set $rip = $__reg_rip
  dont-repeat
end

# restore-interrupted-state <isf pointer>
#
# Restore the interrupted register state.
define restore-interrupted-state
  set language c++

  set $__isf = (granary::interrupt_stack_frame *) $arg0

  # Save the current register state so that if we want to, we can
  # restore it later on to continue execution.
  if !$__regs_saved
    save-regs
  end

  set $rsp = $__isf->stack_pointer
  set $rip = $__isf->instruction_pointer

  # Okay, now we need to find where we saved the other registers.
  set $__top = (uint64_t *) $__isf
  
  set $rdi = *($__top - 2)
  set $rsi = *($__top - 3)
  set $eflags = (uint32_t) (*($__top - 4) & 0xFFFF)
  set $rax = *($__top - 7)
  set $rcx = *($__top - 8)
  set $rdx = *($__top - 9)
  set $rbx = *($__top - 10)
  set $rbp = *($__top - 11)
  set $r8  = *($__top - 12)
  set $r9  = *($__top - 13)
  set $r10 = *($__top - 14)
  set $r11 = *($__top - 15)
  set $r12 = *($__top - 16)
  set $r13 = *($__top - 17)
  set $r14 = *($__top - 18)
  set $r15 = *($__top - 19)

  dont-repeat
end


# restore-regs-state <kernel regs pointer>
#
# Restore the machine state described by the Linux kernel `struct pt_regs`.
define restore-regs-state
  set $__regs = (struct pt_regs *) $arg0

  # Save the current register state so that if we want to, we can
  # restore it later on to continue execution.
  if !$__regs_saved
    save-regs
  end

  set $r15 = $__regs->r15
  set $r14 = $__regs->r14
  set $r13 = $__regs->r13
  set $r12 = $__regs->r12
  set $r11 = $__regs->r11
  set $r10 = $__regs->r10
  set $r9  = $__regs->r9
  set $r8  = $__regs->r8
  set $rdi = $__regs->di
  set $rsi = $__regs->si
  set $rbp = $__regs->bp
  set $rbx = $__regs->bx
  set $rdx = $__regs->dx
  set $rcx = $__regs->cx
  set $rax = $__regs->ax
  set $rsp = $__regs->sp
  set $eflags = $__regs->flags
  set $rip = $__regs->ip
end

# p-descriptor-of <addr>
#
# Print the watchpoint descriptor for a given watched address.
def p-descriptor-of
  set $__addr = (unsigned long long) $arg0
  set $__index = $__addr >> 49
  p client::wp::DESCRIPTORS[$__index]
end

