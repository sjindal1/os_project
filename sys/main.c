#include <sys/defs.h>
#include <sys/gdt.h>
#include <sys/kprintf.h>
#include <sys/tarfs.h>
#include <sys/ahci.h>
#include <sys/pic.h>
#include <sys/idt.h>
#include <sys/paging.h>
#include <sys/kernel.h>

#define INITIAL_STACK_SIZE 4096

uint8_t initial_stack[INITIAL_STACK_SIZE]__attribute__((aligned(16)));
uint32_t* loader_stack;
extern char kernmem, physbase;
uint64_t *kernel_cr3;
void cr3_init_asm();
void checkAllBuses(void);
extern uint64_t free_virtual_address;
void _x86_64_asm_pit_100ms();
extern page_frame_t *free_page,*head;
extern page_dir kernel_page_info;

void start(uint32_t *modulep, void *physbase, void *physfree)
{
  init_gdt();
  create4KbPages(modulep,physbase,physfree);
  kernel_cr3 = kernel_init();
  cr3_init_asm();
  update_global_pointers();
  kprintf("success\n");
  kprintf("kernel_cr3 -> %x\n",kernel_cr3);
  init_pic();
  init_idt();
  _x86_64_asm_pit_100ms();
  
  struct smap_t {
    uint64_t base, length;
    uint32_t type;
  }__attribute__((packed)) *smap;
  
  
  //checkAllBuses();
  while(modulep[0] != 0x9001) modulep += modulep[1]+2;
  for(smap = (struct smap_t*)(modulep+2); smap < (struct smap_t*)((char*)modulep+modulep[1]+2*4); ++smap) {
    if (smap->type == 1 /* memory */ && smap->length != 0) {
      kprintf("Available Physical Memory [%p-%p]\n", smap->base, smap->base + smap->length);
    }
  }
  kprintf("physbase %p\n", (uint64_t)physbase);
  kprintf("physfree %p\n", (uint64_t)physfree);
  kprintf("head %p, free_page %p\n", head, free_page);
  kprintf("tarfs in [%p:%p]\n", &_binary_tarfs_start, &_binary_tarfs_end);
  kprintf("free_virtual_address -> %x\n",free_virtual_address);
  main_task();
  uint64_t *add1 = get_free_page();
  uint64_t *add2 = get_free_page();
  kprintf("free Physical address %p, %p\n",add1, add2);
  free(add1);
  //free(add2);
  uint64_t *addr_pages = get_free_pages(2);
  uint64_t *add1_after = get_free_page();
  uint64_t *add2_after = get_free_page();
  uint64_t *add3_after = get_free_page();
  free(addr_pages);
  add1_after = get_free_page();
  add2_after = get_free_pages(1);
  kprintf("get free pages %p\n", addr_pages);
  kprintf("free Physical address 2nd time %p %p %p\n",add1_after,add2_after,add3_after);
  kprintf("page table entries %p %p %p %p\n", kernel_page_info.pml4, kernel_page_info.pdp, kernel_page_info.pd, kernel_page_info.pt);
  uint64_t *pml4_table = (uint64_t *)kernel_page_info.pml4;
  kprintf("pml4 va address - > %p, 511 entry -> %x, 510 entry->%x\n", pml4_table, pml4_table[511], pml4_table[510]);
  uint64_t *km_add = kmalloc(16000, NULL);
  for(int i=0;i<2010;i++){
    km_add[i] = 0xDEADDEADDEADDEAD;
  }
  kprintf("km_add - > %p\n",km_add);
  //
  //checkAllBuses();
  /*init_pic();
  //_x86_64_asm_pit_100ms();
  init_idt();
  _x86_64_asm_pit_100ms();*/
  while(1);
}

void boot(void)
{
  // note: function changes rsp, local stack variables can't be practically used
  register char *temp1, *temp2;

  for(temp2 = (char*)0xb8001; temp2 < (char*)0xb8000+160*25; temp2 += 2) *temp2 = 7 /* white */;
  __asm__ volatile (
    "cli;"
    "movq %%rsp, %0;"
    "movq %1, %%rsp;"
    :"=g"(loader_stack)
    :"r"(&initial_stack[INITIAL_STACK_SIZE])
  );
  start(
    (uint32_t*)((char*)(uint64_t)loader_stack[3] + (uint64_t)&kernmem - (uint64_t)&physbase),
    (uint64_t*)&physbase,
    (uint64_t*)(uint64_t)loader_stack[4]
  );
  for(
    temp1 = "!!!!! start() returned !!!!!", temp2 = (char*)PRINT_BUF_ADDRESS;
    *temp1;
    temp1 += 1, temp2 += 2
  ) *temp2 = *temp1;
  while(1) __asm__ volatile ("hlt");
}
