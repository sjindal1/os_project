
#if 1

#include <stdlib.h>

void startproc();

void _start(void){
  __asm__ (
/*  "callq startproc\n\t"*/
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

#endif
