.text
.global cr3_init_asm

cr3_init_asm:
  pushq %rax
  movq kernel_cr3, %rax
  movq %rax, %cr3

  movq %cr0, %rax
  or $0x80000001, %eax
  movq %rax, %cr0
  popq %rax
  ret