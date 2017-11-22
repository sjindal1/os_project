#include <stdlib.h>

void exit(int a){
	__asm__ __volatile__ ("movq $60, %rax\n\t"
												"movq $0, %rdi\n\t"
												"syscall\n\t");
}

void _start(void) {
  main(0,NULL,NULL);
  exit(0);
}
