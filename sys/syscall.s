

.global syscall_handle
.global kernel_syscall

syscall_handle:
	swapgs;
	movq %rsp, %gs:16;
	movq %gs:8, %rsp;
	pushq %gs:16;
	swapgs;
//	pushq %rax;
	pushq %rbx;
	pushq %rcx;
	pushq %rdx;
	pushq %rsi;
	pushq %rdi;
	pushq %rbp;
	pushq %r8;
	pushq %r9;
	pushq %r10;
	pushq %r11;
	pushq %r12;
	pushq %r13;
	pushq %r14;
	pushq %r15;

	movq %rax, %r15;
	movq %rdi, %r14;
	movq %rsi, %r13;
	movq %rdx, %r12;

	call kernel_syscall;

	popq %r15;
	popq %r14;
	popq %r13;
	popq %r12;
	popq %r11;
	popq %r10;
	popq %r9;
	popq %r8;
	popq %rbp;
	popq %rdi;
	popq %rsi;
	popq %rdx;
	popq %rcx;
	popq %rbx;
//	popq %rax;
	popq %rsp;
	sysretq;



