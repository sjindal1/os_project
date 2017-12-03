#include <stdlib.h>

void exit(int a){
	__asm__ __volatile__ ("movq $60, %%rax\n\t"
												"movq %0, %%rdi\n\t"
												"syscall\n\t"
                        :
                        :"m"(a));
}

void _start(void){
  __asm__ (
  "movq (%rsp), %rdi \n\t"
  "movq %rsp,   %r8 \n\t"
  "addq $0x8,   %r8 \n\t"
  "movq %r8,   %rsi \n\t"
  "_doadd: addq $0x8, %r8 \n\t"
  "movq (%r8), %r9 \n\t"
  "movq $0x0,   %r10\n\t"
  "cmpl %r9d,   %r10d\n\t"
  "jnz _doadd        \n\t"
  "addq $0x8,   %r8 \n\t"
  "movq %r8,   %rdx \n\t"
  "call main         \n\t"
  "movq %rax,   %rdi \n\t"
  "call exit         \n\t");
}