
#include <stdio.h>
#include <sys/defs.h>
#include <unistd.h>

#if 1
void *malloc(size_t size)
{
	//int *mem = mmap(size);
	//int* mem = brk(size);

	//return (void*) mem;
	return (void*) 0;
}

void free(void *ap)
{
	// nothing to do
	;
}


#else
//****************************************************************************************
// BELOW CODE IS TAKEN FROM THE BOOK
//
// THE C PROGRAMMING LANGUAGE 2ND EDITION
//
// AUTHOR - BRIAN W. KERNIGHAN & DENNIS M. RITCHIE
//
// We dont claim any part of the below code and all rights are with the original authors.
// 
// The code is used for non commercial and academic purpose only.
//
//****************************************************************************************

// size of union is 128 bits, so alignment 64bit is only overlapping with half part

typedef union head header;

union head
{
	struct 
	{
		header *ptr;
		uint64_t size;
	} s;
	uint64_t alignment;
};



static header base;
static header *freep = (header*) 1;			// for trial

// putting the block pointed by ap to the free list
void free(void *ap)
{
	header *bp, *p;

	bp = (header *)ap - 1;
	for(p = freep; !(((uint64_t)bp > (uint64_t)p) && ((uint64_t)bp < (uint64_t)p->s.ptr)); p = p->s.ptr)
		if(p >= p->s.ptr && (((uint64_t)bp > (uint64_t)p) || (uint64_t)bp < (uint64_t)p-> s.ptr))
			break;

	if((bp + bp->s.size) == p->s.ptr)
	{
		header *tmp = (header *)p->s.ptr;
		bp->s.size += tmp->s.size;
		bp->s.ptr = tmp->s.ptr;
	}
	else
		bp->s.ptr = p->s.ptr;

	if(/*(uint64_t)*/ (p + p->s.size) == /*(uint64_t)*/ bp)
	{
		p->s.size += bp->s.size;
		p->s.ptr = bp->s.ptr;
	}
	else
		p->s.ptr = bp;

	freep = p;
}

header *getmorememory(uint32_t units)
{
	int *cp;
	header *up;

	// always allocate huge block of memory like 100 MB and later use in chunks
	cp = brk(units);		// 100 MB given by Kernel
	if(cp == 0)
		return 0;

	up = (header *) cp;

	up->s.size = (1000 * 4096) / sizeof(header);

	free((void *) (up + 1));
	return freep;
}

void *malloc(size_t size)
{
	header *p, *prevp = NULL;
	uint32_t units;

	// do the brk 1 time, it will allcoate heap of 500 mb
	//brk();

	units = (size + sizeof(header) - 1)/sizeof(header) + 1;

	if((prevp == NULL) && (freep == (header*)1))		// dont know why NULL is not working for freep
	{
		base.s.ptr = &base;
		freep = &base;
		prevp = &base;
		base.s.size = 0;
	}

	for(p = (header*) prevp->s.ptr; ; prevp = p, p = (header*)p->s.ptr)
	{
		if(p->s.size >= units)
		{
			if(p->s.size == units)
			{
				prevp->s.ptr = p->s.ptr;
			}
			/*else
			{
				p->s.size -= units;
				p += p->s.size;
				p->s.size = units;
			}*/
			freep = prevp;
			return (void *) (p + 1);
		}
		if(p == freep)
		{
			p = getmorememory(units);
			if(p == NULL)
				return NULL;
		}
	}

	return NULL;	// should not come here
}

#endif
