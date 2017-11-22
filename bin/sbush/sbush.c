//#include <stdio.h>

void user_write(char * b){
  __asm__ __volatile__ ("movq $1, %%rax\n\t"
                        "movq $1, %%rdi\n\t"
                        "movq %0, %%rsi\n\t"
                        "movq $16, %%rdx\n\t"
                        "movq $5, %%r10\n\t"
                        "syscall\n\t"
                        :
                        :"m"(b)
                        :"rsp","rcx","r11","rdi","rsi","rdx","r10");  
}

int main(int argc, char *argv[], char *envp[]) {
  //puts("sbush> ");
  char buf[] = "hello from sbush";
  char *buf1 = buf;
  user_write(buf1);
  while(1){;}
  return 0;
}
