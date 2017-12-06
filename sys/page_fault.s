.global pf_error_code
.global pf_cr2

.data
	pf_error_code: 
		.quad 0;
	pf_cr2:
	 .quad 0;

.text
.global _x86_64_page_fault
.global page_fault_handle

_x86_64_page_fault:
  popq pf_error_code;
  pushf;
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

	movq %cr2, %rax;
	movq %rax, pf_cr2;
  call page_fault_handle
  mov $0x20, %al
  out %al, $0x20

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

  popf
  iretq