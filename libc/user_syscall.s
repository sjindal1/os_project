
.global write
.global read
.global fork
.global execvpe
.global getpid
.global getppid
.global waitpid
.global exit
.global open
.global pscmd
.global close
.global access
.global brk

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

getpid:
	movq $39, %rax;
	syscall;
	retq;

getppid:
	movq $110, %rax;
	syscall;
	retq;

waitpid:
	movq $61, %rax
	syscall;
	retq;

exit:
	movq $60, %rax
	syscall;
	retq;

open:
	movq $2, %rax;
	syscall;
	retq;

pscmd:
	movq $10, %rax;
	syscall;
	retq;

close:
	movq $3, %rax;
	syscall;
	retq;

access:
	movq $21, %rax;
	syscall;
	retq;

brk:
	movq $12, %rax;
	syscall;
	retq;

