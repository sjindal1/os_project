#ifndef _KERNEL_H
#define _KERNEL_H

#define MAX_PROC 1024
#define MAX_FDEFS 16

#define USER_HEAP_START 0x1000000
#define USER_HEAP_SIZE 0

void main_task();

typedef struct pcb pcb;
typedef struct filedes filedes;
typedef struct stvma vma_type;

typedef struct mem_page_info mem_page_info;

struct mem_page_info
{
  uint64_t startva;
  uint32_t no_of_pages;
  uint8_t status;
};

struct stvma{
	uint64_t startva;
	uint64_t size;
	uint64_t *next;
	uint64_t offset_fs;
	uint8_t permissions;
};

struct filedes{
	enum {TERMINAL, TARFS, FS, NETWORK} type;
	uint64_t addr;
	uint64_t offset;
	uint8_t status;
	uint8_t permissions;
  uint32_t size;
};

struct pcb
{
  uint64_t pid;
  uint64_t rsp;
  uint64_t user_rsp;
  uint64_t cr3;
  uint64_t *kstack;
  //enum { EXIT - -1, RUNNING - 0, SLEEPING - 1, WAITING -2, ZOMBIE -3 } state;
  int8_t state; 
  uint8_t exit_status;
  filedes mfdes[MAX_FDEFS];
  uint64_t _start_addr;			// Entry point from the elf header
  vma_type vma[32];
  vma_type vma_stack;
  vma_type heap_vma[7];
  uint8_t numvma;				// number of valid VMA entries
  uint64_t elf_start;
  uint64_t ppid;
  uint8_t my_child[MAX_PROC];
  uint8_t wait_child[MAX_PROC];
  uint8_t wait_for_any_proc;
  uint8_t cwd[256];
  uint64_t *mal_16_info;
  uint64_t *mal_32_info;
  uint64_t *mal_64_info;
  uint64_t *mal_256_info;
  uint64_t *mal_512_info;
  uint64_t *mal_4096_info;
  uint64_t *mpi; //typecast to struct mem_page_info
};

extern pcb pcb_struct[];
extern int current_process;
extern int no_of_task;
extern int free_pcb;
extern uint64_t *kernel_cr3;

void yield();

void save_rsp();

void create_kernel_thread(uint64_t* func_ptr);

void create_pcb_stack(uint64_t *user_cr3,uint64_t va_func);

uint64_t get_pt_va_add(uint64_t v_add);

typedef struct diropen diropen;
struct diropen
{
  uint8_t *fname;
  uint8_t *previous_name;
  uint32_t index;
};

#endif
