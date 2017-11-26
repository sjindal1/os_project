
.global write
.global fork
.global execvpe

// cc - rdi, rsi, rdx, rcx, r8, r9
// stdin - rdi
// 
//output
// rax = stdin
// write(stdout - rdi, buf - rsi , len - rdx )


write:
	//movq fd, %%rdi;
	//movq buf, %%rsi
	//movq len, %%rdx

	movq $1, %rax;
	syscall;
	retq;


fork:
	movq $57, %rax;
	syscall;
	retq;

execvpe:
	movq $59, %rax;
	syscall;
	retq;


read:
	movq $0, %rax;
	syscall;
	retq;
