#the pit assemble code for 1 ms timer interval
# pit.s

#.data
#check if any data is required

.text

.global _x86_64_asm_pit_100ms

_x86_64_asm_pit_100ms:
  pushf
   
  mov $0x36, %al
  out %al, $0x43

  mov $11931, %ax
  #mov $0x2, %ax
  out %al, $0x40
  mov %ah, %al
  out %al, $0x40
  popf
  
  ret
