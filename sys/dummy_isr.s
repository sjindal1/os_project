

.text
.global _x86_64_isr
.global timer_print


_x86_64_isr:
  pushf
  cli
  jmp _call_print 
  sti
  popf
  iretq

_call_print:
  call timer_print

  sti
  popf
  iretq

