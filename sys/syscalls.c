#include <sys/defs.h>
#include <sys/kprintf.h>
#include <sys/kernel.h>
#include <sys/vfs.h>
#include <sys/paging.h>
#include <sys/elf64.h>
#include <sys/utils.h>
#include <sys/tarfs.h>
#include <sys/syscalls.h>

uint64_t _syswrite(syscall_params *params);
uint64_t _sysread(syscall_params *params);
uint64_t _sysexit(syscall_params *params);
uint64_t _sysfork(syscall_params *params);
uint64_t _sysexec(syscall_params *params);
uint64_t _sysgetpid(syscall_params *params);
uint64_t _sysgetppid(syscall_params *params);
uint64_t _syswaitpid(syscall_params *params);
uint64_t _sysopen(syscall_params *params);
uint64_t _sysclose(syscall_params *params);
uint64_t _sysps(syscall_params *params);
uint64_t _sys_access(syscall_params *params);
uint64_t _sysopendir(syscall_params *params);
uint64_t _sysreaddir(syscall_params *params);
uint64_t _sysclosedir(syscall_params *params);
uint64_t _sysstartproc(syscall_params *params);
uint64_t _syskill(syscall_params *params);


void switch_to_ring3(uint64_t *, uint64_t);

//void switch_to_child(uint32_t , uint64_t* , pcb*, pcb*);
void set_child_stack(uint64_t*, pcb*, uint64_t);

_syscallfunc_ sysfunc[200];

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
  /*uint64_t star = rdmsr(0xC0000081);
  uint64_t lstar = rdmsr(0xC0000082);
  uint64_t cstar = rdmsr(0xC0000083);
  uint64_t sfmask = rdmsr(0xC0000084);*/
  sysfunc[0] = &_sysread;
  sysfunc[1] = &_syswrite;
  sysfunc[2] = &_sysopen;
  sysfunc[3] = &_sysclose;
  sysfunc[21] = &_sys_access;
  sysfunc[57] = &_sysfork;
  sysfunc[59] = &_sysexec;
  sysfunc[60] = &_sysexit;
  sysfunc[61] = &_syswaitpid;
  sysfunc[62] = &_syskill;

  sysfunc[77] = &_sysopendir;
  sysfunc[78] = &_sysreaddir;
  sysfunc[79] = &_sysclosedir;

  sysfunc[39] = &_sysgetpid;
  sysfunc[110] = &_sysgetppid;

  // Our OS functionalities
  sysfunc[10] = &_sysps;
  sysfunc[199] = &_sysstartproc;


  // kprintf("efer ->%x, star -> %x, lstar -> %x, cstar -> %x, sfmask -> %x\n", efer, star, lstar, cstar, sfmask);
}

uint64_t last = 0;

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
    uint32_t childproc = retval;
		//make a copy of the parent stack
 		uint64_t *parent_stack = pcb_struct[current_process].kstack;
		uint64_t *child_stack = pcb_struct[childproc].kstack;

		for(int i = 0 ; i<511;i++){
			child_stack[i] = parent_stack[i];
		}
		set_child_stack(pcb_struct[childproc].kstack, &pcb_struct[childproc], (uint64_t)&last);
    
    /*volatile uint64_t *process_stack = pcb_struct[current_process].kstack;
    retval = process_stack[511];*/
    save_rsp();
    __asm__ __volatile__ ("movq %%rsp, %%rax\n\t"
                          "andq $0xfffffffffffff000, %%rax\n\t"
                          "orq $0xff8, %%rax\n\t"
                          "movq (%%rax), %%rdx\n\t"
                          "movq %%rdx, %0\n\t"
                          :"=m"(retval)
                          :
                          :"memory");
    /*volatile uint64_t *p_stack = pcb_struct[current_process].kstack;
    retval = p_stack[511];*/
    //kprintf("retval %d\n", retval);
    yield();
    return retval;
    //return pcb_struct[current_process].kstack[511];
	}else{
    yield();
  }

	kfree((uint64_t *)params);

	return retval;
}

uint64_t _sysstartproc(syscall_params *params){
  return 0;
}

uint64_t _sysopen(syscall_params *params){
  uint8_t *filename = (uint8_t *)params->p1;
  filename++; // tarfs starts from bin and not /bin so removing one character
  return _vfsopen(filename);
}

uint64_t _sysopendir(syscall_params *params){
  uint8_t *filename = (uint8_t *)params->p1;
  if(strcmp(filename, (uint8_t *)"/") == 0){ //root is special case
    return 1;
  }else{    
    filename++; // tarfs starts from bin and not /bin so removing one character
    return _vfsexists(filename);
  }
}

uint64_t _sysclosedir(syscall_params *params){  
  return 0;
}

uint64_t _sysreaddir(syscall_params *params){
  uint8_t *path = (uint8_t *)params->p1;
  if(strcmp(path, (uint8_t *)"/") != 0){
    path++;
  }
  _vfsreaddir(path);
  return 0;
}

uint64_t _sys_access(syscall_params *params){
  uint8_t *filename = (uint8_t *)params->p1;
  filename++; // tarfs starts from bin and not /bin so removing one character
  return _vfsexists(filename);
}

uint64_t _sysclose(syscall_params *params){
  uint32_t fd = (uint32_t)params->p1;
  if(fd < 0 || fd > MAX_FDEFS || pcb_struct[current_process].mfdes[fd].status == 0){
    return -1;
  }else{
    pcb_struct[current_process].mfdes[fd].status = 0;
    pcb_struct[current_process].mfdes[fd].type = TARFS;
    pcb_struct[current_process].mfdes[fd].addr = 0;
    pcb_struct[current_process].mfdes[fd].offset = 0;
    pcb_struct[current_process].mfdes[fd].permissions = 0;
    pcb_struct[current_process].elf_start = 0;
  }
  return 0;
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

uint64_t _syskill(syscall_params *params){
  uint64_t pid = params->p1;
  if(pcb_struct[pid].state != -1 && pid != 1){ // is a running process;
    pcb_struct[pid].exit_status = params->p2;
    pcb_struct[pid].state = 3;
    return 0;
  }
  
  return 1; // no process to kill
}


uint64_t _sysexit(syscall_params *params){
	pcb_struct[current_process].exit_status = params->p1;
  pcb_struct[current_process].state = 3;
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
  pcb_struct[free_pcb].ppid = current_process;

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

  pcb_struct[free_pcb].vma_stack.startva = pcb_struct[current_process].vma_stack.startva;
  pcb_struct[free_pcb].vma_stack.size = pcb_struct[current_process].vma_stack.size;
  pcb_struct[free_pcb].vma_stack.next = pcb_struct[current_process].vma_stack.next;
  pcb_struct[free_pcb].vma_stack.offset_fs = pcb_struct[current_process].vma_stack.offset_fs;
  pcb_struct[free_pcb].vma_stack.permissions = pcb_struct[current_process].vma_stack.permissions;

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

uint64_t _syswaitpid(syscall_params *params){
  int64_t pid = (int64_t)params->p1;
  if(pid == -1){
    pcb_struct[current_process].wait_for_any_proc = 1;
    pcb_struct[current_process].state = 2;
  }else if(pid >= 0){    
    if(pcb_struct[current_process].my_child[pid] == 0){
      return 0;
    }
    pcb_struct[current_process].wait_child[pid] = 1;
    pcb_struct[current_process].state = 2;
  }
  return 0;
}

void update_free_pcb(){
  int i;
  for(i = free_pcb+1; i<MAX_PROC ; i++){
    if(pcb_struct[i].state == -1){
      free_pcb = i;
      return;
    }
  }
  
  for(i = 2; i<free_pcb; i++){
    if(pcb_struct[i].state == -1){
      free_pcb = i;
      break;
    }
  }
}

uint64_t _sysfork(syscall_params *params){
	create_pcb_copy();
  pcb_struct[current_process].my_child[free_pcb] = 1;
	//free_pcb++;
  int child_pid = free_pcb;
	update_free_pcb();
  no_of_task++;
	return child_pid;
}

uint64_t _sysexec(syscall_params *params){
  uint8_t* path = (uint8_t *)params->p1;
  uint8_t **argv = (uint8_t **)params->p2;
  uint8_t **exec_envp = (uint8_t **)params->p3;

  uint8_t filename[256]; // assuming that path is the absolute path.

  uint8_t *fileptr;

  if(strStartsWith(path, (uint8_t *)"/") == 0){
    fileptr = path;
  }else{
    int i = 0;
    while(exec_envp[i] != 0){
      uint8_t *env_var = exec_envp[i];
      if(strStartsWith(env_var, (uint8_t *)"PATH=") == 0){
        uint8_t path_parts[3][256];
        strspt(env_var, path_parts, '=');
        strConcat(path_parts[1], path, filename);
        fileptr = (uint8_t *)filename;
      }
      i++;
    } 
  }

  fileptr++; // to remove the slash at the start

  int16_t fd = _vfsopen(fileptr);

  uint8_t *envp_ker[50]; 

  int i=0;
  while(exec_envp[i] != 0){
    envp_ker[i] = (uint8_t *)kmalloc(4096);
    int j = 0;
    while(exec_envp[i][j] != '\0'){
      envp_ker[i][j] = exec_envp[i][j];
      j++;
    }
    envp_ker[i][j] = '\0';
    i++;
  }
  envp_ker[i] = 0;
  i--;

  uint8_t *arg_ker[10]; 

  int arg_i=0;
  while(argv[arg_i] != 0){
    arg_ker[arg_i] = (uint8_t *)kmalloc(4096);
    int j = 0;
    while(argv[arg_i][j] != '\0'){
      arg_ker[arg_i][j] = argv[arg_i][j];
      j++;
    }
    arg_ker[arg_i][j] = '\0';
    arg_i++;
  }
  arg_ker[arg_i] = 0;
  arg_i--;

  clear_load_file(&pcb_struct[current_process], fd);

  uint64_t stackadd = pcb_struct[current_process].user_rsp;

  stackadd = ((stackadd & 0xfffffffffffff000) | 0xff8);

  uint64_t func_start_add = (uint64_t)pcb_struct[current_process]._start_addr;

  uint64_t args_block = (func_start_add  & 0xfffffffffffff000) - 2*4096;

  stackadd = copy_environ(args_block, (uint64_t *)stackadd, envp_ker);

  stackadd = copy_argv(args_block + 0x800, (uint64_t *)stackadd, arg_ker);

  for( ; i >=0;i--){
    kfree((uint64_t *)envp_ker[i]);
  }
  for( ; arg_i >=0;arg_i--){
    kfree((uint64_t *)arg_ker[arg_i]);
  }
  //save_rsp();
  //switch_to_ring3((uint64_t *)&user_ring3_process, stack);
  invalidate_tlb();
  switch_to_ring3((uint64_t *)func_start_add, stackadd);
  

  return 0;
}

uint64_t _sysgetpid(syscall_params *params)
{
	return pcb_struct[current_process].pid;
}

uint64_t _sysgetppid(syscall_params *params)
{
	return pcb_struct[current_process].ppid;
}


uint64_t _sysps(syscall_params *params)
{
 uint16_t i;
 uint8_t buf[] = "4554445534";

  _vfswrite(1, (uint8_t *)"PID   TTY    NAME  \n", 20);
 for(i = 1; i < MAX_PROC; i++)
 {
   if(pcb_struct[i].state != -1)
   {
     itoa(pcb_struct[i].pid, (uint8_t*)&buf[0]);

     _vfswrite(1, (uint8_t*) buf, strlen(buf));
     _vfswrite(1, (uint8_t*) "     TTY/0", 11);

     if(i == 1)
       _vfswrite(1, (uint8_t*) "  init", 8);
     else if(i == pcb_struct[current_process].ppid){
       _vfswrite(1, (uint8_t*) "  sbush", 8);
     }
     else if(i == current_process)
       _vfswrite(1, (uint8_t*) "  ps", 5);
     else
       _vfswrite(1, (uint8_t*) "  Kthread", 10);

     _vfswrite(1, (uint8_t*) "\n", 1);
   }
 }

 return 0;
}

