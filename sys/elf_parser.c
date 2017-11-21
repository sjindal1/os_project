#include <sys/defs.h>
#include <sys/kprintf.h>
#include <sys/kernel.h>
#include <sys/syscalls.h>
#include <sys/utils.h>
#include <sys/elf64.h>
#include <sys/vfs.h>

// file should already to opened
// pass/set the FD to read from that file
void elfreadheader(int16_t efd, Elf64_Ehdr *eh)
{
	_vfsread(efd, (uint8_t*) eh, sizeof(Elf64_Ehdr));
	return;
}

void parsetheprogramheader(int16_t efd, uint64_t pgoffset, uint32_t size, uint32_t num)
{
	int i;
	uint32_t offset = 0;
	Elf64_Phdr phhdr;

	for(i = 0; i < num; i++)
	{
		offset = offset + pgoffset * size;
		//_vfsseek(efd, offset);

		_vfsread(efd, (uint8_t *) &phhdr, size);

		kprintf("p_type =%x p_flags = %x \n", phhdr.p_type, phhdr.p_flags);
		kprintf("p_offset =%x p_vaddr = %x \n", phhdr.p_offset, phhdr.p_vaddr);
		kprintf("p_paddr =%x p_filesz = %x \n", phhdr.p_paddr, phhdr.p_filesz);
		kprintf("p_memsz =%x p_align = %x \n", phhdr.p_memsz, phhdr.p_align);
	}

}
uint32_t loadelffile(pcb *p, int16_t efd)
{
	Elf64_Ehdr elfhdr;

	if(p->mfdes[efd].status == 0)		// file is not opened
		return 0;

	elfreadheader(efd, &elfhdr);

	kprintf("elfhdr.e_type =%x\n", elfhdr.e_type);
	kprintf("elfhdr.e_phoff PG =%x\n", elfhdr.e_phoff);
	kprintf("efelfhdrd.e_phentsize PG size =%x\n", elfhdr.e_phentsize);
	kprintf("efelfhdrd.e_phnum PG elements =%x\n", elfhdr.e_phnum);

	kprintf("elfhdr.e_shoff =%x\n", elfhdr.e_shoff);
	kprintf("elfhdr.e_ehsize =%x\n", elfhdr.e_ehsize);
	kprintf("elfhdr.e_flags =%x\n", elfhdr.e_flags);
	
	//_vfsseek(efd, elfhdr.e_phoff);
	parsetheprogramheader(efd, elfhdr.e_phoff, elfhdr.e_phentsize, elfhdr.e_phnum);

	return 0;
}


void parseelf(pcb *curpcb)
{
	// Not much check about supported files, since file is from tarfs, built by us.
	//read the header or direcly map the header as it is memory
	//Elf64_Ehdr *elfhdr = curpcb->fstartaddr;

	//uint8_t hdr[sizeof(Elf64_Ehdr)];







}



