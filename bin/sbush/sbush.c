//#include <stdio.h>

void user_write(char * b, int len){
  __asm__ __volatile__ ("movq $1, %%rax\n\t"
                        "movq $1, %%rdi\n\t"
                        "movq %0, %%rsi\n\t"
                        "movq %1, %%rdx\n\t"
                        "movq $5, %%r10\n\t"
                        "syscall\n\t"
                        :
                        :"m"(b), "m"(len)
                        :"rsp","rcx","r11","rdi","rsi","rdx","r10");  
}

int user_fork(){
  int ret = 0;
  __asm__ __volatile__ ("movq $57, %%rax\n\t"
                        "syscall\n\t"
                        : "=m" (ret)
                        );  
  return ret;
}

int main(int argc, char *argv[], char *envp[]) {
  //puts("sbush> ");
  char buf[] = "hello from sbush";
  char ch[] = "in child";
  char par[] = "in parent";
  int pid = 0;
  char *buf1 = buf;
  user_write(buf1, 16);
  pid = user_fork();

  if(pid == 0)
  {
    user_write(&ch[0], 8);
  }
  else
  {
    user_write(&par[0], 9);
  }

  while(1){;}
  return 0;
}
