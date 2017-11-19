#include <sys/defs.h>
#include <sys/kprintf.h>
#include <sys/kernel.h>
#include <sys/syscalls.h>
#include <sys/utils.h>
#include <sys/elf64.h>
#include <sys/vfs.h>

// file should already to opened
// pass/set the FD to read from that file
void elfreadheader(uint32_t efd, Elf64_Ehdr *eh)
{
	_vfsread(efd, (uint8_t*) eh, sizeof(Elf64_Ehdr));
	return;
}


uint32_t loadelffile(pcb *p, uint32_t efd)
{
	Elf64_Ehdr elfhdr;

	if(p->mfdes[efd].status == 0)		// file is not opened
		return 0;

	elfreadheader(efd, &elfhdr);

	kprintf("elfhdr.e_type =%x\n", elfhdr.e_type);
	kprintf("elfhdr.e_phoff =%x\n", elfhdr.e_phoff);
	kprintf("elfhdr.e_shoff =%x\n", elfhdr.e_shoff);
	kprintf("elfhdr.e_flags =%x\n", elfhdr.e_flags);
	kprintf("elfhdr.e_ehsize =%x\n", elfhdr.e_ehsize);
	kprintf("efelfhdrd.e_phentsize =%x\n", elfhdr.e_phentsize);

	return 0;
}


void parseelf(pcb *curpcb)
{
	// Not much check about supported files, since file is from tarfs, built by us.
	//read the header or direcly map the header as it is memory
	//Elf64_Ehdr *elfhdr = curpcb->fstartaddr;

	//uint8_t hdr[sizeof(Elf64_Ehdr)];







}



