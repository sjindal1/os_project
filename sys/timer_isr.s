
.data
count:
    .byte 10 

.text
.global _x86_64_timer_isr
.global timer_print
.global yield


_x86_64_timer_isr:
  pushf
  //cli
  // pushing all general purpose registers
	pushq %rax;
	pushq %rbx;
	pushq %rcx;
	pushq %rdx;
	pushq %rsi;
	pushq %rdi;
	pushq %rbp;
	pushq %rsp;

	pushq %r8;
	pushq %r9;
	pushq %r10;
	pushq %r11;
	pushq %r12;
	pushq %r13;
	pushq %r14;
	pushq %r15;

  call timer_print
  mov $0x20, %al
  out %al, $0x20

  callq yield
  
	popq %r15;
	popq %r14;
	popq %r13;
	popq %r12;
	popq %r11;
	popq %r10;
	popq %r9;
	popq %r8;

	popq %rsp;
	popq %rbp;
	popq %rdi;
	popq %rsi;
	popq %rdx;
	popq %rcx;
	popq %rbx;
	popq %rax;
	
  //sti
  popf
  iretq

