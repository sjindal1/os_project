#include <stdio.h>

int write(int fd, char * b, int len);
int fork();

int main(int argc, char *argv[], char *envp[]) {
  //puts("sbush> ");
  char buf[] = "This is the ls command";
  char *buf1 = buf;
  write(1, buf1, 22);

  printf("envp 0 - %s", envp[0]);
  printf("envp 1 - %s", envp[1]);
  return 0;
}


