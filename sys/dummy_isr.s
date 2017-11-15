

.text
.global _x86_64_isr
.global _x86_64_isr_32
.global _x86_64_isr_6
.global int_print

_x86_64_isr_32:
  pushf
  //cli
  call int_32_print 
  //sti
  mov $0x20, %al
  out %al, $0x20
  popf
  iretq

_x86_64_isr_6:
  pushf
  //cli
  call int_6_print 
  //sti
  mov $0x20, %al
  out %al, $0x20
  popf
  iretq

_x86_64_isr:
  pushf
  //cli
  jmp _call_print 
  //sti
  mov $0x20, %al
  out %al, $0x20
  popf
  iretq

_call_print:
  call int_print
  mov $0x20, %al
  out %al, $0x20

  //sti
  popf
  iretq

