#ifndef _KERNEL_H
#define _KERNEL_H

void main_task();

typedef struct pcb pcb;
typedef struct filedes filedes;
typedef struct stvma vma_type;

struct stvma{
	uint64_t startva;
	uint64_t size;
	uint64_t *next;
	uint64_t offset_fs;
	uint32_t permissions;
};

struct filedes{
	enum {TERMINAL, TARFS, FS, NETWORK} type;
	uint64_t addr;
	uint64_t offset;
	uint8_t status;
	uint8_t permissions;
};

struct pcb
{
  uint64_t pid;
  uint64_t rsp;
  uint64_t user_rsp;
  uint64_t cr3;
  uint64_t *kstack;
  //enum { RUNNING, SLEEPING, ZOMBIE } state;
  uint8_t state; 
  uint8_t exit_status;
  filedes mfdes[16];

  uint64_t _start_addr;			// Entry point from the elf header

  vma_type vma[32];
  uint8_t numvma;				// number of valid VMA entries
  uint64_t elf_start;
};

extern pcb pcb_struct[];
extern int current_process;
extern int no_of_task;
extern int free_pcb;
extern uint64_t *kernel_cr3;

void yield();

void create_kernel_thread(uint64_t* func_ptr);

void create_pcb_stack(uint64_t *user_cr3,uint64_t va_func);

#endif
