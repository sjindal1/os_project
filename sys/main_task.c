#include <sys/defs.h>
#include <sys/kprintf.h>
#include <sys/kernel.h>
#include <sys/paging.h>
#include <sys/gdt.h>
#include <sys/tarfs.h>
#include <sys/vfs.h>
#include <sys/syscalls.h>
#include <sys/elf64.h>


uint8_t bufpruthvi[] = {"hello\nsecond\nthird\nfourth"};

void kernel_1_thread();
void kernel_2_thread();
void switch_out(pcb*);
void switch_to_ring3(uint64_t *, uint64_t);
void user_process_init(uint64_t *func_add, uint32_t no_of_pages);
void create_user_ring3_kernel_thread(uint64_t* func_ptr);
void user_process_1();

void rdmsr_read(uint32_t);
void wrmsr_write(uint32_t, uint32_t, uint32_t);

void clear_pcb(){
  for(int i = 0; i<MAX_PROC; i++){
    pcb_struct[i].state = -1;
  }
}

void main_task(){

  init_syscalls();
	//setting main task PCB
  /*pcb_struct[free_pcb].pid = free_pcb;
  pcb_struct[free_pcb].cr3 = (uint64_t)kernel_cr3;
  pcb_struct[free_pcb].state = 0;
  pcb_struct[free_pcb].exit_status = -1;
  free_pcb++;
  no_of_task++;*/

  clear_pcb(); //set all processes state as -1
  create_kernel_thread((uint64_t *)&kernel_2_thread);
  create_kernel_thread((uint64_t *)&kernel_1_thread);

  //int j=0;
	//while(j<2){
    kprintf("This is the main_task of kernel thread\n");
    //j++;
    switch_out(&pcb_struct[current_process]);
    kprintf("returning to main thread\n");
	//}  


	return;
}

/*void map_user_process_init(uint64_t *func_add, uint32_t no_of_pages){
	uint64_t pa_func = ((uint64_t)func_add - (uint64_t)&kernmem + (uint64_t)&physbase);
	pa_func = pa_func & (uint64_t)0xFFFFFFFFFFFFF000;
	uint64_t va_func = 0xFFFFFEFF20000000;
  pa_func-=4096;
	uint64_t* user_cr3 = create_user_page_table(va_func,pa_func,3);
	va_func = va_func | ((uint64_t)func_add & (uint64_t)0xfff);
  va_func+=4096;
	create_pcb_stack(user_cr3,va_func);
}*/

void user_process_1(){
  //int j = 0;
  while(1){
    kprintf("This is user process 1\n");
    yield();
    kprintf("returning to user process 1\n");
  }
}

uint64_t trialwrite(uint8_t *t)
{
  uint64_t ret = 0;
    __asm__ __volatile__ ("movq $1, %%rax\n\t"
                        "movq $1, %%rdi\n\t"
                        "movq %1, %%rsi\n\t"
                        "movq $25, %%rdx\n\t"
                        "syscall\n\t"
                        "movq %%rax, %0\n\t"
                        :"=m"(ret)
                        :"m"(t)
                        :"rsp","rax","rdi","rsi","rdx", "rcx", "r11");
  return ret;
}

uint64_t trialread(uint8_t *tread)
{
  uint64_t ret = 0;
    __asm__ __volatile__ ("movq $0, %%rax\n\t"
                          "movq $0, %%rdi\n\t"
                          "movq %1, %%rsi\n\t"
                          "movq $128, %%rdx\n\t"
                          "syscall\n\t"
                          "movq %%rax, %0\n\t"
                          :"=m"(ret)
                          :"m"(tread)
                          :"rsp","rcx","r11","rdi","rsi","rdx","r10");
  return ret;
}


void user_ring3_process() {
  kprintf("This is ring 3 user process 1\n");
  /*uint64_t __err;
  __asm__ __volatile__ ("movq $0, %%rdi\n\t"
                        "movq $0, %%rsi\n\t"
                        "movq $0, %%rax\n\t"
                        "syscall\n\t"
                        :"=a"(__err)
                        :"0" (57));*/
  uint8_t bufee[] = "hello\nsecond\nthird\nfourth";
  uint8_t *buf1 = bufee;

  uint64_t ret = trialwrite(buf1);

  //kprintf("retrun value after first write %d\n",ret);
  //kprintf("buf1 add return- %p & %p %d %x %d\n", buf1, &buf1, a, intp, c);

  uint8_t buf2[256];
  uint8_t *bufptr = buf2;
  ret = trialread(bufptr);

  kprintf("retrun value after first read %d\n",ret);


  ret = trialwrite(bufptr);

  kprintf("retrun value after second write %d\n",ret);

  //kprintf("returned from syscall\n");
  while(1){};
  //yield();
  /*while(1) {
    kprintf("This is ring 3 user process 1\n");
    yield();
    kprintf("returning to ring 3 user process 1\n");
  }*/
}

void save_rsp(){
  //uint64_t rsp;
  //__asm__ __volatile__ ("movq %%rsp, %0" : "=m"(rsp) : : );
  //rsp = rsp & 0xFFFFFFFFFFFFF000;
  //rsp = rsp + 0xff8;
  //set_tss_rsp((void *)rsp);
  /*uint64_t *rsp = pcb_struct[current_process].kstack;
  set_tss_rsp((void *)rsp);*/
  uint64_t curr_proc_pcb = (uint64_t)&pcb_struct[current_process];
  wrmsr(0xC0000102, curr_proc_pcb);  
}

uint64_t power(uint64_t num, uint64_t pow){
  uint64_t result = 1;
  for(int i=0; i<pow; i++){
    result *= num;
  }
  return result;
}

void kernel_1_thread(){
  int j = 0;

  //kprintf("This is the first kernel thread\n");

  //user ring3 process
  //create_user_ring3_kernel_thread((uint64_t*) &user_ring3_process);

  while(j<2){
    j++;
    kprintf("This is the first kernel thread\n");
    //user process init
    //uint64_t func_ptr = (uint64_t)&user_process_1;
    //map_user_process_init((uint64_t*)func_ptr,1);

    yield();
    kprintf("returning to kernel_1_thread\n");
  }
  init_tarfs();

  char filename[] = "bin/init";
  uint8_t *fileptr = (uint8_t *)filename;

  int16_t fd = _vfsopen(fileptr);

  kprintf("bin/init fd - %d",fd);

  loadelffile(&pcb_struct[current_process], fd);  

  /*uint64_t *test_pt = (uint64_t *)0xFFFFFFFF90012000;

  *test_pt = 1000;*/

  uint64_t stackadd = (uint64_t)((uint64_t)pcb_struct[current_process]._start_addr & 0xFFFFFFFFFFFFF000) - 6*4096;

  uint64_t *pa_add = get_free_pages(5); // 4 for thr stack and 1 for envp

/*  pcb_struct[current_process].heap_vma.startva = USER_HEAP_START;
  pcb_struct[current_process].heap_vma.size = USER_HEAP_SIZE;
  pcb_struct[current_process].heap_vma.permissions = 0xff;*/
  
  pcb_struct[current_process].vma_stack.startva = stackadd;

  pcb_struct[current_process].vma_stack.size = 4*4096;

  pcb_struct[current_process].vma_stack.permissions = 0xff;

  for(uint32_t i = 0; i< 5; i++){
    create_pf_pt_entry(pa_add, stackadd);
    pa_add+=512;
    stackadd +=4096;
  }

  stackadd -= 4096; // remove for envp
  uint64_t args_block = stackadd; 

  stackadd -= 8;  

  pcb_struct[current_process].cwd[0] = '/';
  pcb_struct[current_process].cwd[1] = '\0';

  stackadd = copy_environ(args_block, (uint64_t *)stackadd, envp);

  stackadd = copy_argv(args_block + 0x800, (uint64_t *)stackadd, argvuser);


  pcb_struct[current_process].mal_16_info = (uint64_t*)kmalloc(4096);
  pcb_struct[current_process].mal_32_info = (uint64_t*)kmalloc(4096);
  pcb_struct[current_process].mal_64_info = (uint64_t*)kmalloc(4096);
  pcb_struct[current_process].mal_256_info = (uint64_t*)kmalloc(4096);
  pcb_struct[current_process].mal_512_info = (uint64_t*)kmalloc(4096);
  pcb_struct[current_process].mal_4096_info = (uint64_t*)kmalloc(4096);

  //switching to ring 3
  uint64_t tss_stack = (uint64_t)kmalloc(4096);
  //kprintf("stack - %x\n", stack);
  tss_stack+= 4088;
  set_tss_rsp((void *)tss_stack);
   
  save_rsp();

  //switch_to_ring3((uint64_t *)&user_ring3_process, stack);
  switch_to_ring3((uint64_t *)pcb_struct[current_process]._start_addr, stackadd);
  
  while(1){};
}

void kernel_2_thread(){
  while(1){
    //kprintf("This is the second kernel thread\n");
    //user process init
    /*uint64_t func_ptr = (uint64_t)&user_process_1;
    user_process_init((uint64_t*)func_ptr,1);*/
    yield();
    //kprintf("returning to kernel_2_thread\n");
  }
  while(1){};
}

/*void create_user_ring3_kernel_thread(uint64_t* func_ptr){
  uint64_t *tmp;

  //Kernel Thread PCB
  pcb_struct[free_pcb].pid = free_pcb;
  pcb_struct[free_pcb].cr3 = (uint64_t)kernel_cr3;
  pcb_struct[free_pcb].state = 0;
  pcb_struct[free_pcb].exit_status = -1;
  pcb_struct[free_pcb].kstack = kmalloc(4096);
  pcb_struct[free_pcb].rsp = (uint64_t)(pcb_struct[free_pcb].kstack) + 0xF80;
   
  //Initialize Thread 1
  // set structures of reg = 0;
  // TODO : need more elements to push to stack for ring3
  tmp = (uint64_t*) pcb_struct[free_pcb].rsp;

#if 1
  // put the magic number of ring3 switching
  *tmp-- = 0x23;
  *tmp-- = pcb_struct[free_pcb].rsp; //eax ... just pushing rsp for the heck of it
  uint64_t fflags = get_rflags_asm();
  *tmp-- = fflags;
  *tmp-- = 0x23;
  pcb_struct[free_pcb].rsp -= 32;
#endif

  //push the start address of thread
  *tmp-- = (uint64_t) func_ptr;
  pcb_struct[free_pcb].rsp -= 8;

  for(int i = 14; i > 0; i--)
  {
    *(tmp--) = 0;
    pcb_struct[free_pcb].rsp -= 8;
  }

  //push cr3 onto the stack
  *(tmp--) = (uint64_t)kernel_cr3;

  uint64_t rflags = get_rflags_asm();
  *tmp-- = rflags;
  *tmp-- = (uint64_t)&pcb_struct[free_pcb];
  pcb_struct[free_pcb].rsp -= 16;    // finaly value of RSP
 
  // update the pcb structure
  free_pcb++;
  no_of_task++;
}*/
