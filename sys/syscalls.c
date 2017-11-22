#include <sys/defs.h>
#include <sys/kprintf.h>
#include <sys/kernel.h>
#include <sys/vfs.h>
#include <sys/paging.h>
#include <sys/syscalls.h>

uint64_t _syswrite(syscall_params *params);
uint64_t _sysread(syscall_params *params);

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
  kprintf("efer ->%x, star -> %x, lstar -> %x, cstar -> %x, sfmask -> %x\n", efer, star, lstar, cstar, sfmask);
}


uint64_t kernel_syscall()
{
  	uint64_t retval = 0;
	syscall_params *params = (syscall_params *)kmalloc(4096, NULL);

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