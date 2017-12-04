#include <stdio.h>

int write(int fd, char * b, int len);
int fork();

int main(int argc, char *argv[], char *envp[]) {
  //puts("sbush> ");
  char buf[] = "This is the ls command\n";
  write(1, buf, 22);
  /*char *buf1 = buf;

  printf("envp 0 - %s", envp[0]);
  printf("envp 1 - %s", envp[1]);
  for(int i =0 ;i<3;i++){
  	printf("in child");
  }*/
  return 0;
}


