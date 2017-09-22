
.data
count:
    .byte 10 

.text
.global _x86_64_timer_isr
.global timer_print


_x86_64_timer_isr:
  pushf
  cli
  call timer_print
  mov $0x20, %al
  out %al, $0x20
  sti
  popf
  iretq

