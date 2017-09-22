

.text
.global _x86_64_keyboard_isr
.global key_press_handle


_x86_64_keyboard_isr:
  pushf
  call key_press_handle
  mov $0x20, %al
  out %al, $0x20
  popf
  iretq

