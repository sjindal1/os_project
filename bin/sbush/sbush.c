#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

void output(int num)
{
;
}

/*void exit(int a){
  __asm__ __volatile__ ("movq $60, %%rax\n\t"
                        "movq %0, %%rdi\n\t"
                        "syscall\n\t"
                        :
                        :"m"(a));
}
*/
#if 1
  

  char buf[] = "hello from sbush";
  char ch[] = "in child";
  char par[] = "in parent";

int main(int argc, char *argv[], char *envp[]) {
  //puts("sbush> ");
  int pid = 0, rw = 0;
  //int rw = 0;
  char *buf1 = buf;
  //rw = write(1, buf1, 16);
  printf("printf ls %d", rw);
  //rw = write(1, buf, 16);
  pid = fork();
  if(pid == 0)
  {
    int i = 0;
    while(i<2){
      printf("%s\n", ch, rw);
      i++;
    }
  }else{
    int j = 0;
    while(j < 2){
      printf("%s\n", par);
      j++;
    }

  }

  write(1, buf1, 16);

  /*int *p = (int *)100;
  *p = 10;*/
  while(1);

}

#else
int main(int argc, char *argv[], char *envp[]) {
  //puts("sbush> ");
  char buf[] = "hello from sbush";
  //char ch[] = "in child";
  char par[] = "in parent";
  int pid = 0, rw = 0;
  char *buf1 = buf;
  rw = write(1, buf1, 16);
  printf("printf working \n");
  pid = fork();

  char *ls_args[] = {"ls", 0};
  /*char *arg1[] ;
  ls_args[0] = 
*/
  if(pid == 0)
  {
    //rw = write(1, &ch[0], 8);
    //output(rw);
    //execvpe(ls_args[0],ls_args,envp);
    //while(1) ;

    int child_pid = 0;

    child_pid = fork();

    if(child_pid == 0)
    {
      while(1){
        printf("grandchild %d\n", child_pid);
      }
    }
    else
    {
      int i = 0;
      while(i<5){
        printf("father %d\n", child_pid);
        i++;
      }
      exit(1);
    }
  }
  else
  {
    rw = write(1, &par[0], 9);

    waitpid(pid, NULL);

    int j =5;
    while( j >=0){
      printf(" parent ");
      j--;
    }
    output(rw);
  }

  pid = fork();

  if(pid == 0)
  {
    //rw = write(1, &ch[0], 8);
    //output(rw);
   execvpe(ls_args[0],ls_args,envp);
  }
  else
  {
    rw = write(1, &par[0], 9);

    waitpid(pid, NULL);
    printf("second time\n");
  }

  while(1){
    //printf("grandparent\n");
    ;
  }
  return 0;
}

#endif

