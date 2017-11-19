#include <sys/defs.h>
#include <sys/kprintf.h>
#include <sys/kernel.h>
#include <sys/terminal.h>
#include <sys/syscalls.h>
#include <sys/utils.h>
#include <sys/elf64.h>

uint64_t va_start;
uint32_t elffd;
;

// file should already to opened
// pass/set the FD to read from that file
void elfheader()
{
	//__sysread(elffd, &elfhdr, sizeof(Elf64_Ehdr));

}

uint16_t tarfsfilecount;
typedef struct tarfsinfo tarfsinfo;

struct tarfsinfo
{
	uint8_t fname[256];
	uint16_t fsize;
	uint64_t fstartaddr;
};

tarfsinfo ftarinfo[256];

// input - file name
// output - start address
// if file not found returns 0
//uint64_t openelf(uint8_t filename, int *size)
uint64_t _tarfsopen(uint8_t *filename)
{
	uint16_t i, res = 1;

	for(i = 0; i < tarfsfilecount; i++)
	{
		res = strcmp(filename, ftarinfo[i].fname);
		if(res == 0)
			return ftarinfo[i].fstartaddr;
	}

	return 0;
}

void parseelf(pcb *curpcb)
{
	// Not much check about supported files, since file is from tarfs, built by us.
	//read the header or direcly map the header as it is memory
	//Elf64_Ehdr *elfhdr = curpcb->fstartaddr;

	//uint8_t hdr[sizeof(Elf64_Ehdr)];







}



