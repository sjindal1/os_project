#include <sys/defs.h>
#include <sys/kprintf.h>
#include <sys/kernel.h>
#include <sys/vfs.h>
#include <sys/paging.h>
#include <sys/elf64.h>
#include <sys/utils.h>
#include <sys/tarfs.h>
#include <sys/terminal.h>
#include <sys/syscalls.h>

extern uint64_t time;

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
uint64_t _syssleep(syscall_params *params);
uint64_t _syschdir(syscall_params *params);
uint64_t _sysgetcwd(syscall_params *params);
uint64_t _sysclear(syscall_params *params);
uint64_t _sysbrk(syscall_params *params);
uint64_t _sys_munmap(syscall_params *params); 

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
  sysfunc[11] = &_sys_munmap;
  sysfunc[12] = &_sysbrk;
  sysfunc[21] = &_sys_access;
  sysfunc[57] = &_sysfork;
  sysfunc[59] = &_sysexec;
  sysfunc[60] = &_sysexit;
  sysfunc[61] = &_syswaitpid;
  sysfunc[62] = &_syskill;

  
  sysfunc[79] = &_sysgetcwd;
  sysfunc[80] = &_syschdir;

  sysfunc[39] = &_sysgetpid;
  sysfunc[110] = &_sysgetppid;

  // Our OS functionalities
  sysfunc[10] = &_sysps;
  sysfunc[177] = &_sysopendir;
  sysfunc[178] = &_sysreaddir;
  sysfunc[179] = &_sysclosedir;
  sysfunc[197] = &_sysclear;
  sysfunc[198] = &_syssleep;
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
    uint32_t childproc = retval - 1;
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
    //yield();
    return retval;
    //return pcb_struct[current_process].kstack[511];
	}else{
    yield();
  }

	kfree((uint64_t *)params);

	return retval;
}

uint64_t _sysstartproc(syscall_params *params){
  __asm__ __volatile__ ("sti\n\t");
  return 0;
}

uint64_t _sysopen(syscall_params *params){
  uint8_t *filename = (uint8_t *)params->p1;
  filename++; // tarfs starts from bin and not /bin so removing one character
  return _vfsopen(filename);
}

uint64_t _sysopendir(syscall_params *params){
	uint8_t *filename = (uint8_t *)params->p1;
	uint8_t *filepath;
	uint8_t filebuf[256];
	uint32_t i;

	if(strcmp(filename, (uint8_t *)"/") == 0){
		filepath = filename;
	}else{
		if(strStartsWith(filename, (uint8_t*) "/") == 0){ //absolute path
			filepath = filename;
		}else{                           //relative path
			uint8_t *env_var = &pcb_struct[current_process].cwd[0];
			strConcat(env_var, filename, filebuf);
			filepath = filebuf;
	    } 

	    filepath++;		// to remove "/"
	    if(_vfsexists(filepath) == 0)
	    	return 0;
	}
    
  	uint64_t *p_add = get_free_page();
  	uint64_t va_add = (pcb_struct[current_process]._start_addr & 0xfffffffffffff000) - 4096;
    create_pf_pt_entry(p_add, (uint64_t)va_add);
    uint8_t *namebuf = (uint8_t*) va_add + 24;

    diropen* dir = (diropen*)va_add;

    for(i = 0 ; i < strlen(filepath);i++){
		namebuf[i] = filepath[i];
	}
	namebuf[i] = '\0';

	dir->fname = namebuf;
	namebuf[1024] = '\0';
	dir->previous_name = namebuf + 1024;
	dir->index = 0;

	return (uint64_t) dir;

#if 0
  uint8_t *filename = (uint8_t *)params->p1;
  if(strcmp(filename, (uint8_t *)"/") == 0){ //root is special case
    return 1;
  }else{    
    filename++; // tarfs starts from bin and not /bin so removing one character
    return _vfsexists(filename);
  }
 #endif
}

uint64_t _sysclosedir(syscall_params *params){  
  return 0;
}

uint64_t _sysclear(syscall_params *params){  
  _termclear();
  return 0;
}

uint64_t _sysreaddir(syscall_params *params){
	diropen *dir = (diropen*) params->p1;
	uint8_t *filename;
	int i;

	_vfsreaddir(dir, &filename);

	if(filename == NULL)
		return 0;

	uint8_t *buf = (uint8_t*)dir + 2048;

	for(i = 0 ; i < strlen(filename);i++){
		buf[i] = filename[i];
	}
	buf[i] = '\0';

	return (uint64_t) buf;

#if 0
  uint8_t *path = (uint8_t *)params->p1;
  if(strcmp(path, (uint8_t *)"/") != 0){
    path++;
  }
  _vfsreaddir(path);
  return 0;
#endif
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
    pcb_struct[current_process].mfdes[fd].size = 0;
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

uint64_t _syssleep(syscall_params *params){
  return time;
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
  	pcb_struct[free_pcb].mfdes[i].size = pcb_struct[current_process].mfdes[i].size;
  }

  for(int i=0 ; i<32;i++){
  	pcb_struct[free_pcb].vma[i].startva = pcb_struct[current_process].vma[i].startva;
  	pcb_struct[free_pcb].vma[i].size = pcb_struct[current_process].vma[i].size;
  	pcb_struct[free_pcb].vma[i].next = pcb_struct[current_process].vma[i].next;
  	pcb_struct[free_pcb].vma[i].offset_fs = pcb_struct[current_process].vma[i].offset_fs;
  	pcb_struct[free_pcb].vma[i].permissions = pcb_struct[current_process].vma[i].permissions;
  }

  int cwd_index = 0;
  while(pcb_struct[current_process].cwd[cwd_index] != '\0'){
    pcb_struct[free_pcb].cwd[cwd_index] = pcb_struct[current_process].cwd[cwd_index];
    cwd_index++;
  }

  pcb_struct[free_pcb].vma_stack.startva = pcb_struct[current_process].vma_stack.startva;
  pcb_struct[free_pcb].vma_stack.size = pcb_struct[current_process].vma_stack.size;
  pcb_struct[free_pcb].vma_stack.next = pcb_struct[current_process].vma_stack.next;
  pcb_struct[free_pcb].vma_stack.offset_fs = pcb_struct[current_process].vma_stack.offset_fs;
  pcb_struct[free_pcb].vma_stack.permissions = pcb_struct[current_process].vma_stack.permissions;

  // heap
  for(int i = 0; i < 7; i++){
    pcb_struct[free_pcb].heap_vma[i].startva = pcb_struct[current_process].heap_vma[i].startva;
    pcb_struct[free_pcb].heap_vma[i].size = pcb_struct[current_process].heap_vma[i].size;
    pcb_struct[free_pcb].heap_vma[i].next = pcb_struct[current_process].heap_vma[i].next;
    pcb_struct[free_pcb].heap_vma[i].offset_fs = pcb_struct[current_process].heap_vma[i].offset_fs;
    pcb_struct[free_pcb].heap_vma[i].permissions = pcb_struct[current_process].heap_vma[i].permissions;
  }

  pcb_struct[free_pcb].mal_16_info = (uint64_t*)kmalloc(4096);
  pcb_struct[free_pcb].mal_32_info = (uint64_t*)kmalloc(4096);
  pcb_struct[free_pcb].mal_64_info = (uint64_t*)kmalloc(4096);
  pcb_struct[free_pcb].mal_256_info = (uint64_t*)kmalloc(4096);
  pcb_struct[free_pcb].mal_512_info = (uint64_t*)kmalloc(4096);
  pcb_struct[free_pcb].mal_4096_info = (uint64_t*)kmalloc(4096);
  pcb_struct[free_pcb].mpi = (uint64_t*)kmalloc(4096);

  for(int i = 0; i < 512; i++){
    pcb_struct[free_pcb].mal_16_info[i] = pcb_struct[current_process].mal_16_info[i];
    pcb_struct[free_pcb].mal_32_info[i] = pcb_struct[current_process].mal_32_info[i];
    pcb_struct[free_pcb].mal_64_info[i] = pcb_struct[current_process].mal_64_info[i];
    pcb_struct[free_pcb].mal_256_info[i] = pcb_struct[current_process].mal_256_info[i];
    pcb_struct[free_pcb].mal_512_info[i] = pcb_struct[current_process].mal_512_info[i];
    pcb_struct[free_pcb].mal_4096_info[i] = pcb_struct[current_process].mal_4096_info[i];
    pcb_struct[free_pcb].mpi[i] = pcb_struct[current_process].mpi[i];

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
  int child_pid = free_pcb + 1;
  //free_pcb++;
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

uint64_t _syschdir(syscall_params *params){
  uint8_t *path = (uint8_t *)params->p1;

  if(strcmp((uint8_t *)path, (uint8_t *)"..") == 0){
    if(strcmp((uint8_t *)pcb_struct[current_process].cwd, (uint8_t *)"/") == 0){
      return -1;
    }else{      
      int lastchar_path = strlen(pcb_struct[current_process].cwd);
      lastchar_path--; 
      while(pcb_struct[current_process].cwd[--lastchar_path] != '/'); //lastchar_path-- would be "/" so start from one less 
      pcb_struct[current_process].cwd[++lastchar_path] = '\0';
      return 0;
    }
  }

  uint8_t *fileptr = NULL;

  uint8_t filename[256];

  if(strStartsWith((uint8_t *)path, (uint8_t *)"/") == 0){
    fileptr = (uint8_t *)path;
  }else{
    strConcat(pcb_struct[current_process].cwd, path, filename);  
    fileptr = (uint8_t *)filename;
  }


  int lastchar = strlen(fileptr);
  if(fileptr[lastchar -1] != '/'){
    fileptr[lastchar] = '/';
    fileptr[++lastchar] = '\0';
  }

  int fileptr_index = 0;
  if(_vfsexists((uint8_t *)&fileptr[1]) == 1){
    while(fileptr[fileptr_index] != '\0'){
      pcb_struct[current_process].cwd[fileptr_index] = fileptr[fileptr_index];
      fileptr_index++;
    }
    pcb_struct[current_process].cwd[fileptr_index] = '\0';
    return 0;
  }else{
    return -1;
  }
  
  return -1; 
}

uint64_t _sysgetcwd(syscall_params *params){
  uint8_t *buf = (uint8_t *)params->p1;
  int fileptr_index = 0;
  while(pcb_struct[current_process].cwd[fileptr_index] != '\0'){
    buf[fileptr_index] = pcb_struct[current_process].cwd[fileptr_index];
    fileptr_index++;
  }
  buf[fileptr_index] = '\0';
  return (uint64_t)buf;
}

uint64_t _sysbrk(syscall_params *params)
{
	uint64_t reqsize = (uint64_t) params->p1;
  uint32_t i;
  if(reqsize <= 16){
    uint8_t *info_16 = (uint8_t *)pcb_struct[current_process].mal_16_info; 
    for(i =0; i < 1024; i++){
      if(info_16[i] == 0){
        info_16[i] = 1;
        break;
      }
    }
    return (uint64_t)(pcb_struct[current_process].heap_vma[0].startva + 16*i);
  }else if(reqsize <= 32){
    uint8_t *info_32 = (uint8_t *)pcb_struct[current_process].mal_32_info;
    for(i =0; i < 1024; i++){
      if(info_32[i] == 0){
        info_32[i] = 1;
        break;
      }
    }
    return (uint64_t)(pcb_struct[current_process].heap_vma[1].startva + 32*i);
  }else if(reqsize <= 64){
    uint8_t *info_64 = (uint8_t *)pcb_struct[current_process].mal_64_info;
    for(i =0; i < 1024; i++){
      if(info_64[i] == 0){
        info_64[i] = 1;
        break;
      }
    }
    return (uint64_t)(pcb_struct[current_process].heap_vma[2].startva + 64*i);
  }else if(reqsize <= 256){
    uint8_t *info_256 = (uint8_t *)pcb_struct[current_process].mal_256_info;
    for(i =0; i < 1024; i++){
      if(info_256[i] == 0){
        info_256[i] = 1;
        break;
      }
    }
    return (uint64_t)(pcb_struct[current_process].heap_vma[3].startva + 256*i);
  }else if(reqsize <= 512){
    uint8_t *info_512 = (uint8_t *)pcb_struct[current_process].mal_512_info;
    for(i =0; i < 1024; i++){
      if(info_512[i] == 0){
        info_512[i] = 1;
        break;
      }
    }
    return (uint64_t)(pcb_struct[current_process].heap_vma[4].startva + 512*i);
  }else if(reqsize <= 4096){
    uint8_t *info_4096 = (uint8_t *)pcb_struct[current_process].mal_4096_info;
    for(i =0; i < 1024; i++){
      if(info_4096[i] == 0){
        info_4096[i] = 1;
        break;
      }
    }
    return (uint64_t)(pcb_struct[current_process].heap_vma[5].startva + 4096*i);
  }else{
  	uint64_t prev_size = pcb_struct[current_process].heap_vma[6].size;

    pcb_struct[current_process].heap_vma[6].size += ((reqsize+4095)/4096)*4096;

    mem_page_info *mpi = (mem_page_info *)pcb_struct[current_process].mpi;
    int i;
    for(i = 0;i < 256; i++){
      if(mpi[i].status == 0){
        mpi[i].status = 1;
        mpi[i].startva = pcb_struct[current_process].heap_vma[6].startva + prev_size;
        mpi[i].no_of_pages = ((reqsize+4095)/4096);
        break;
      }
    }
    if(i == 256){
      return 0;
    }
  	return pcb_struct[current_process].heap_vma[6].startva + prev_size;
  }
}


uint64_t _sys_munmap(syscall_params *params){
  uint64_t free_add = (uint64_t) params->p1;
  uint32_t i=0;
  for( ; i< 7; i++){
    if(free_add >= pcb_struct[current_process].heap_vma[i].startva 
      && free_add < pcb_struct[current_process].heap_vma[i].startva + pcb_struct[current_process].heap_vma[i].size){
      break;
    }
  }
  if(i == 7){
    return 1;
  }else if(i == 0){
    uint32_t index = (free_add%pcb_struct[current_process].heap_vma[i].startva)/16;
    uint8_t *info_16 = (uint8_t *)pcb_struct[current_process].mal_16_info; 
    info_16[index] = 0;
    invlpg((uint64_t *)free_add);    
  }else if(i == 1){
    uint32_t index = (free_add%pcb_struct[current_process].heap_vma[i].startva)/32;
    uint8_t *info_32 = (uint8_t *)pcb_struct[current_process].mal_32_info; 
    info_32[index] = 0;
    invlpg((uint64_t *)free_add);
  }else if(i == 2){
    uint32_t index = (free_add%pcb_struct[current_process].heap_vma[i].startva)/64;
    uint8_t *info_64 = (uint8_t *)pcb_struct[current_process].mal_64_info; 
    info_64[index] = 0;
    invlpg((uint64_t *)free_add);
  }else if(i == 3){
    uint32_t index = (free_add%pcb_struct[current_process].heap_vma[i].startva)/256;
    uint8_t *info_256 = (uint8_t *)pcb_struct[current_process].mal_256_info; 
    info_256[index] = 0;
    invlpg((uint64_t *)free_add);
  }else if(i == 4){
    uint32_t index = (free_add%pcb_struct[current_process].heap_vma[i].startva)/512;
    uint8_t *info_512 = (uint8_t *)pcb_struct[current_process].mal_512_info; 
    info_512[index] = 0;
    invlpg((uint64_t *)free_add);
  }else if(i == 5){
    uint32_t index = (free_add%pcb_struct[current_process].heap_vma[i].startva)/4096;
    uint8_t *info_4096 = (uint8_t *)pcb_struct[current_process].mal_4096_info; 
    info_4096[index] = 0;
    uint64_t pt_off = get_pt((uint64_t)free_add);
    uint64_t *pt_va = (uint64_t *)get_pt_va_add((uint64_t)free_add);
    uint64_t pt_p_add = pt_va[pt_off];
    if((pt_p_add & 0x1) == 1){
      free((uint64_t *)pt_p_add);
    }
    invlpg((uint64_t *)free_add);
  }else{
    mem_page_info *mpi = (mem_page_info *)pcb_struct[current_process].mpi;
    uint32_t i, no_of_pages = 0;
    for(i = 0;i < 256; i++){
      if(free_add >= mpi[i].startva && free_add <= mpi[i].startva + (mpi[i].no_of_pages)*4096){
        mpi[i].status = 0;
        mpi[i].startva = 0;
        no_of_pages = mpi[i].no_of_pages;
        mpi[i].no_of_pages = 0;
        break;
      }
    }
    if(i == 256){
      return 1;
    }

    uint64_t pt_off = get_pt((uint64_t)free_add);
    uint64_t *pt_va = (uint64_t *)get_pt_va_add((uint64_t)free_add);
    uint64_t pt_p_add = pt_va[pt_off];

    for(i = 0; i<no_of_pages;i++){
      pt_p_add = pt_va[pt_off+i];
      if((pt_p_add & 0x1) == 1){
        free((uint64_t *)pt_p_add);
      }
      invlpg((uint64_t *)free_add+i*4096);
    }
  }
  return 0;
}
