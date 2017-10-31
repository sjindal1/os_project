#ifndef _KERNEL_H
#define _KERNEL_H

void main_task();

typedef struct pcb pcb;

struct pcb
{
  uint64_t pid;
  uint64_t rsp;
  uint64_t cr3;
  uint64_t *kstack;
  //enum { RUNNING, SLEEPING, ZOMBIE } state;
  int state; 
  int exit_status;
};

#endif
