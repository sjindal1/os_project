#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[], char *envp[]) {
  
  //printf("testing segmentation fault\n");
  int *p = (int *)0x1200000;
  *p =10;
  return 0;
}


