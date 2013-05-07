set language c++
set logging off
set breakpoint pending on


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
  set $__bb = ((granary::basic_block_info *) $arg0)
  printf "Basic block info:\n"
  printf "  App address: %p\n", $__bb.generating_pc
  printf "  Stub instructions: %p\n", ($arg0 - $__bb.num_bytes)
  printf "  Instructions: %p\n", ($arg0 - $__bb.num_bytes + $__bb.num_patch_bytes)
  dont-repeat
end


# p-wrapper ID
#
# Prints the information about a function wrapper with id ID.
define p-wrapper
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
  set $__i = (int) $arg0
  set $__head = (granary::trace_log_item *) granary::TRACE._M_b._M_p
  printf "Global code cache lookup trace:\n"
  while $__i >= 0 && 0 != $__head
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
