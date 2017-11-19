//#include <stdio.h>

int main(int argc, char *argv[], char *envp[]) {
  //puts("sbush> ");
  char buf[] = "hello\nsecond\nthird";
  char *buf1 = buf;
  __asm__ __volatile__ ("movq $1, %%rax\n\t"
                        "movq $1, %%rdi\n\t"
                        "movq %0, %%rsi\n\t"
                        "movq $18, %%rdx\n\t"
                        "movq $5, %%r10\n\t"
                        "syscall\n\t"
                        :
                        :"m"(buf1)
                        :"rsp","rcx","r11","rdi","rsi","rdx","r10");
  return 0;
}
