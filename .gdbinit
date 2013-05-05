set language c++
set breakpoint pending on

b granary_fault
b granary_break_on_fault

define p-bb-info
  p *((granary::basic_block_info *) $arg0)
  dont-repeat
end

