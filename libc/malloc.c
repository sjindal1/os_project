
#include <stdio.h>
#include <sys/defs.h>
#include <unistd.h>

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
union head
{
	struct 
	{
		union head *ptr;
		uint64_t size;
	} s;
	uint64_t alignment;
};

typedef union head header;

static header base;
static header *freep = (header*) 1;			// for trial

// putting the block pointed by ap to the free list
void free(void *ap)
{
	header *bp, *p;

	bp = (header *)ap - 1;
	for(p = freep; !(bp > p && bp < (header *)p->s.ptr); p = (header *)p->s.ptr)
		if(p >= (header *)p->s.ptr && (bp > p || bp < (header *)p-> s.ptr))
			break;

	if(bp + bp->s.size == (header *)p->s.ptr)
	{
		header *tmp = (header *)p->s.ptr;
		bp->s.size += tmp->s.size;
		bp->s.ptr = tmp->s.ptr;
	}
	else
		bp->s.ptr = p->s.ptr;

	if((uint64_t) p + p->s.size == (uint64_t) bp)
	{
		p->s.size += bp->s.size;
		p->s.ptr = bp->s.ptr;
	}
	else
		p->s.ptr = (header*) bp;

	freep = p;
}

header *getmorememory(uint32_t units)
{
	char *cp;
	header *up;

	// always allocate huge block of memory like 100 MB and later use in chunks
	cp = (void*) brk();		// 100 MB given by Kernel
	if(cp == NULL)
		return NULL;

	up = (header *) cp;

	up->s.size = (100 * 1024 * 1024) / sizeof(header);

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
		base.s.ptr = (header*)&base;
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
			else
			{
				p->s.size -= units;
				p += p->s.size;
				p->s.size = units;
			}
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