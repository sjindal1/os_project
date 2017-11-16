#include <sys/defs.h>
#include <sys/kprintf.h>
#include <sys/kernel.h>
#include <sys/terminal.h>
#include <sys/syscalls.h>


extern pcb pcb_entries[];
extern int current_process;

uint64_t _syswrite(syscall_params *params);

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
  sysfunc[1] = &_syswrite;
  kprintf("efer ->%x, star -> %x, lstar -> %x, cstar -> %x, sfmask -> %x\n", efer, star, lstar, cstar, sfmask);
}

void syscall_handle(){
  __asm__ __volatile__ ("pushq %r15\n\t"
                        "pushq %rbx\n\t"
                        "movq %rax, %r15\n\t");
  uint64_t user_rsp, user_rcx, user_r11;
  uint64_t kernel_rsp = (&pcb_entries[current_process])->rsp;
  syscall_params params;
  //save the user stack into rax and load kernel stack
  __asm__ __volatile__ ("movq %%rsp, %%rbx\n\t"
                        "movq %0, %%rsp\n\t"
                        :
                        :"m"(kernel_rsp)
                        :"rbx");
  //save the user stack,rip and rflags that are stored in rbx, rcx and r11 respectively
  __asm__ __volatile__ ("movq %%rbx, %0\n\t"
                        "movq %%rcx, %1\n\t"
                        "movq %%r11, %2\n\t"
                        "movq %%r15, %3\n\t"
                        "movq %%rdi, %4\n\t"
                        "movq %%rsi, %5\n\t"
                        "movq %%rdx, %6\n\t"
                        "movq %%r10, %7\n\t"
                        :"=m"(user_rsp), "=m"(user_rcx), "=m"(user_r11), "=m"(params.sysnum), "=m"(params.p1),
                        "=m"(params.p2), "=m"(params.p3), "=m"(params.p4) 
                        :
                        :"rbx","rcx","r11","rax");

  kprintf("syscall handler\n");

  kprintf("syscall_num -> %d, p1 ->%d, p2 ->%d, p3 ->%d, p4 ->%d\n", params.sysnum, params.p1, params.p2, params.p3,params.p4);

  sysfunc[params.sysnum](&params);

  yeild();

  //restore the stack,rip and rflags that are stored in rbx, rcx and r11 respectively
  __asm__ __volatile__ ("movq %1, %%rcx\n\t"
                        "movq %2, %%r11\n\t"
                        "movq %0, %%rsp\n\t"
                        "popq %%rbx\n\t"
                        "popq %%r15\n\t"
                        :
                        :"m"(user_rsp), "m"(user_rcx), "m"(user_r11)
                        :"rcx","r11");
  __asm__ __volatile__ ("sysretq\n\t");
}


uint64_t _syswrite(syscall_params *params){
	kprintf("you are in write p2 = %x, p3 = %d\n", params->p2, params->p3);
	//standard out and standard error terminal output.
	if(params->p1 == 1 || params->p1 == 2){
		_termwrite((uint8_t *)(params->p2), params->p3);
	}
	return 1;
}