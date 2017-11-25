#include <sys/defs.h>
#include <sys/kprintf.h>
#include <sys/kernel.h>
#include <sys/vfs.h>
#include <sys/paging.h>
#include <sys/syscalls.h>

uint64_t _syswrite(syscall_params *params);
uint64_t _sysread(syscall_params *params);
uint64_t _sysexit(syscall_params *params);
uint64_t _sysfork(syscall_params *params);

//void switch_to_child(uint32_t , uint64_t* , pcb*, pcb*);
void set_child_stack(uint64_t*, pcb*,uint64_t);

_syscallfunc_ sysfunc[100];

void wrmsr(uint32_t msrid, uint64_t msr_value){
  uint32_t msr_value_lo = (uint32_t) msr_value;
  uint32_t msr_value_hi = (uint32_t) (msr_value>>32);
  __asm__ __volatile__ ("wrmsr": : "c" (msrid), "a" (msr_value_lo), "d"(msr_value_hi));
}

uint64_t rdmsr(uint32_t msrid){
  uint64_t msr_value_lo;
  uint64_t msr_value_hi;
  __asm__ __volatile__ ("rdmsr": "=a" (msr_value_lo), "=d" (msr_value_hi): "c" (msrid));
  return (uint64_t)(msr_value_hi<<32) | (uint64_t)msr_value_lo;
}

void init_syscalls(){
  wrmsr(0xC0000081, ((uint64_t)0x1b)<<48  | ((uint64_t)0x8)<<32);
  wrmsr(0xC0000082, (uint64_t)&syscall_handle);
  uint64_t efer = rdmsr(0xC0000080);
  wrmsr(0xC0000080, (uint64_t)(efer|0x1));
  uint64_t star = rdmsr(0xC0000081);
  uint64_t lstar = rdmsr(0xC0000082);
  uint64_t cstar = rdmsr(0xC0000083);
  uint64_t sfmask = rdmsr(0xC0000084);
  sysfunc[0] = &_sysread;
  sysfunc[1] = &_syswrite;
  sysfunc[57] = &_sysfork;
  sysfunc[60] = &_sysexit;
  kprintf("efer ->%x, star -> %x, lstar -> %x, cstar -> %x, sfmask -> %x\n", efer, star, lstar, cstar, sfmask);
}


uint64_t kernel_syscall()
{
  uint64_t retval = 0;
	syscall_params *params = (syscall_params *)kmalloc(4096);

	__asm__ __volatile__ ("movq %%r15, %0\n\t"
		                "movq %%r14, %1\n\t"
		                "movq %%r13, %2\n\t"
		                "movq %%r12, %3\n\t"
		                "movq %%r10, %4\n\t"
		                :"=m"(params->sysnum), "=m"(params->p1),"=m"(params->p2), 
		                "=m"(params->p3), "=m"(params->p4) 
		                :
		                :"memory");
	
	  //kprintf("syscall_handle 1 sysnum -> %x, p1 - %x, p2- %x, p3- %x, p4 - %x\n",params->sysnum, params->p1, params->p2, params->p3, params->p4);
	retval = sysfunc[params->sysnum](params);

	if(params->sysnum == 57)
	{
		uint32_t childproc = free_pcb - 1;
		//make a copy of the parent stack
 		uint64_t *parent_stack = pcb_struct[current_process].kstack;
		uint64_t *child_stack = pcb_struct[childproc].kstack;

		for(int i = 0 ; i<512;i++){
			child_stack[i] = parent_stack[i];
		}
		uint64_t rip = 0;
		__asm__ __volatile__("lea (%%rip), %%rax\n\t"
							"movq %%rax, %0\n\t"
							:"=m"(rip));
		set_child_stack(pcb_struct[childproc].kstack, &pcb_struct[childproc],rip);
	}

	yield();

	if(params->sysnum == 57){
		uint64_t *process_stack = pcb_struct[current_process].kstack;
		retval = process_stack[511];
		save_rsp();
	}

	kfree((uint64_t *)params);

	return retval;
}

uint64_t _syswrite(syscall_params *params){
	//kprintf("you are in write p2 = %x, p3 = %d\n", params->p2, params->p3);
	//standard out and standard error terminal output.
/*	if(params->p1 == 1 || params->p1 == 2){
		_termwrite((uint8_t *)(params->p2), params->p3);
	}*/

	return _vfswrite(params->p1, (uint8_t *)params->p2, params->p3);
}

uint64_t _sysread(syscall_params *params){
	/*//kprintf("you are in read p2 = %x, p3 = %d\n", params->p2, params->p3);
	//standard out and standard error terminal output.
	if(params->p1 == 0){
		_termread((uint8_t *)(params->p2), params->p3);
	}
	//kprintf("returning from sysread user buf - %x\n", (params->p2));
	return 1;*/

	return _vfsread(params->p1, (uint8_t *)params->p2, params->p3);;
}

//TODO
//Call yield and clean this process up after the yield.
uint64_t _sysexit(syscall_params *params){
	pcb_struct[current_process].exit_status = 1;
	return 0;
}

void create_pcb_copy(){
  pcb_struct[free_pcb].pid = free_pcb;
  pcb_struct[free_pcb].kstack = kmalloc(4096);
  pcb_struct[free_pcb].rsp = (uint64_t)pcb_struct[free_pcb].kstack + 4088;
  pcb_struct[free_pcb].cr3 = makepagetablecopy(pcb_struct[current_process].cr3);
  pcb_struct[free_pcb].user_rsp = pcb_struct[current_process].user_rsp;
  pcb_struct[free_pcb].state = pcb_struct[current_process].state;
  pcb_struct[free_pcb].exit_status = pcb_struct[current_process].exit_status;
  pcb_struct[free_pcb]._start_addr = pcb_struct[current_process]._start_addr;
  pcb_struct[free_pcb].numvma = pcb_struct[current_process].numvma;
  pcb_struct[free_pcb].elf_start = pcb_struct[current_process].elf_start;

  for(int i=0 ; i<16;i++){
  	pcb_struct[free_pcb].mfdes[i].type = pcb_struct[current_process].mfdes[i].type;
  	pcb_struct[free_pcb].mfdes[i].addr = pcb_struct[current_process].mfdes[i].addr;
  	pcb_struct[free_pcb].mfdes[i].offset = pcb_struct[current_process].mfdes[i].offset;
  	pcb_struct[free_pcb].mfdes[i].status = pcb_struct[current_process].mfdes[i].status;
  	pcb_struct[free_pcb].mfdes[i].permissions = pcb_struct[current_process].mfdes[i].permissions;
  }

  for(int i=0 ; i<32;i++){
  	pcb_struct[free_pcb].vma[i].startva = pcb_struct[current_process].vma[i].startva;
  	pcb_struct[free_pcb].vma[i].size = pcb_struct[current_process].vma[i].size;
  	pcb_struct[free_pcb].vma[i].next = pcb_struct[current_process].vma[i].next;
  	pcb_struct[free_pcb].vma[i].offset_fs = pcb_struct[current_process].vma[i].offset_fs;
  	pcb_struct[free_pcb].vma[i].permissions = pcb_struct[current_process].vma[i].permissions;
  }

  //make a copy of the parent stack
  uint64_t *parent_stack = pcb_struct[current_process].kstack;
  uint64_t *child_stack = pcb_struct[free_pcb].kstack;

  // copy user space stack
  uint64_t *parent_user_stack = (uint64_t*) ((uint64_t) pcb_struct[current_process].user_rsp & (uint64_t) 0xfffffffffffff000);
  uint64_t *child_user_stack = (uint64_t*) ((uint64_t) pcb_struct[free_pcb].user_rsp & (uint64_t) 0xfffffffffffff000);
  
  for(int i = 0 ; i<512;i++){
  	child_stack[i] = parent_stack[i];
  	child_user_stack[i] = parent_user_stack[i];
  }

  child_stack[511] = 0;
  parent_stack[511] = free_pcb;

  //switch_to_child(free_pcb, pcb_struct[free_pcb].kstack, &pcb_struct[current_process], &pcb_struct[free_pcb]);
  //set_child_stack(pcb_struct[free_pcb].kstack, &pcb_struct[free_pcb]);
}

uint64_t _sysfork(syscall_params *params){
	create_pcb_copy();
	free_pcb++;
	no_of_task++;
	return free_pcb;
}

