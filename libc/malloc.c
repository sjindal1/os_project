
#include <stdio.h>
#include <sys/defs.h>
#include <unistd.h>

void munmap(void *ptr);

void *malloc(size_t size)
{
	int* mem = brk(size);
	return (void*) mem;
}

void free(void *ap)
{
	// nothing to do
	munmap(ap);
}

