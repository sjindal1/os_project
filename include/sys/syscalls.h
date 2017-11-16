#ifndef _SYSCALLS_H
#define _SYSCALLS_H

void wrmsr(uint32_t msrid, uint64_t msr_value);

uint64_t rdmsr(uint32_t);

void init_syscalls();

void syscall_handle();

typedef struct syscall_params{
	uint64_t p1;
	uint64_t p2;
	uint64_t p3;
	uint64_t p4;
	uint64_t sysnum;
} syscall_params;

typedef uint64_t (*_syscallfunc_)(syscall_params *);

#endif