set logging off
set breakpoint pending on
set print demangle on
set print asm-demangle on
set print object on
set print static-members on
set disassembly-flavor att
set language c++


# set-user-detect
#
# Uses Python support to set the variable `$__in_user_space`
# to `0` or `1` depending on whether we are instrumenting in
# user space or kernel space, respectively.
define set-user-detect
  set logging file tmp.gdb
  set logging overwrite on
  set logging redirect on
  set logging on
  python print "set $__in_user_space = %d" % int(None is not gdb.current_progspace().filename)
  set logging off
  set logging redirect off
  set logging overwrite off
  source tmp.gdb
end


# Detect if we're in user space and set `$__in_user_space`
# appropriately.
set-user-detect


# Kernel setup
if !$__in_user_space
  file ~/Code/linux-3.8.2/vmlinux
  target remote : 9999
  source ~/Code/Granary/granary.syms
end


# Granary breakpoints
b granary_fault
b granary_break_on_fault
b granary_break_on_predict


# Kernel breakpoints
if !$__in_user_space
  b panic
  b show_fault_oops
  b do_invalid_op
  b do_general_protection
  b __schedule_bug
  b __stack_chk_fail
  b do_spurious_interrupt_bug
  b report_bug
end


# p-bb-info ADDR
#
# Print the basic block info structure information for the basic block
# whose meta-information is located at address ADDR.
define p-bb-info
  set language c++
  set $__bb = ((granary::basic_block_info *) $arg0)
  printf "Basic block info:\n"
  printf "  App address: %p\n", $__bb.generating_pc
  printf "  Stub instructions: %p\n", ($arg0 - $__bb.num_bytes)
  printf "  Instructions: %p\n", ($arg0 - $__bb.num_bytes + $__bb.num_patch_bytes)
  dont-repeat
end


# g-in-length ADDR
#
# Return


# x-ins START END
#
# Examine the instructions in the range [START, END).
define x-ins
  set $__start = (uint64_t) $arg0
  set $__end = (uint64_t) $arg1
  set $__dont_exit = 1
  set $__len = 0
  set $__in = (uint8_t *) 0

  while $__dont_exit
    set logging file /dev/null
    set logging overwrite on
    set logging redirect on
    set logging on
    x/2i $__start
    set $__start = $_

    # we're at the end
    

    set logging off
    set logging redirect off
    set logging overwrite off

    
    set $__in = (uint8_t *) $__start
    if $__start == $__end || 0xEA == *$__in || 0xD4 == *$__in || 0x82 == *$__in
      set $__dont_exit = 0
    else 
      x/i $__start
    end
  end
end


# p-bb ADDR
#
# Print the info and instructions of a basic block given the address
# of an instruction in the basic block.
define p-bb
  set language c++
  set $__a = (uint64_t) $arg0
  set $__a = $__a + (($__a % 8) ? 8 - ($__a % 8): 0)
  set $__int = (uint32_t *) $__a
  while *$__int != granary::basic_block_info::HEADER
    set $__int = $__int + 1
  end

  set $__bb_info = (granary::basic_block_info *) $__int
  set $__bb_info_addr = (uint64_t) $__int
  set $__bb_stub = $__bb_info_addr - $__bb_info->num_bytes
  set $__bb_start = $__bb_stub + $__bb_info->num_patch_bytes

  if $__bb_stub != $__bb_start
    printf "Stub instructions:\n"
    x-ins $__bb_stub $__bb_start
    printf "\n"
  end

  printf "Translated instructions:\n"
  x-ins $__bb_start $__bb_info_addr
  printf "\n"
  p-bb-info $__bb_info
end


# p-wrapper ID
#
# Prints the information about a function wrapper with id ID.
define p-wrapper
  set language c++
  set $__w = &(granary::FUNCTION_WRAPPERS[(int) $arg0])
  printf "Function wrapper %s (%d):\n", $__w->name, (int) $arg0
  printf "  Original address: %p\n", $__w->original_address
  printf "  Wrapper address: %p\n", $__w->wrapper_address
  dont-repeat
end


# p-trace N
#
# Prints at most N elements from the trace log.
define p-trace
  set language c++
  set $__i = (int) $arg0
  set $__head = (granary::trace_log_item *) granary::TRACE._M_b._M_p
  printf "Global code cache lookup trace:\n"
  while $__i > 0 && 0 != $__head
    if $__i < $arg0
      printf "\n"
    end
    printf "  App address: %p\n", $__head->app_address
    printf "  Code cache address: %p\n", $__head->cache_address
    printf "  Kind: "
    p $__head->kind
    set $__i = $__i - 1
    set $__head = $__head->prev
  end
  dont-repeat
end
