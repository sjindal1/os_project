#include <sys/defs.h>
#include <sys/kprintf.h>
#include <sys/kernel.h>
#include <sys/syscalls.h>
#include <sys/utils.h>
#include <sys/elf64.h>
#include <sys/paging.h>
#include <sys/vfs.h>

// file should already to opened
// pass/set the FD to read from that file
void elfreadheader(int16_t efd, Elf64_Ehdr *eh)
{
	_vfsread(efd, (uint8_t*) eh, sizeof(Elf64_Ehdr));
	return;
}

void parsetheprogramheader(pcb *p, int16_t efd, uint64_t pgoffset, uint32_t size, uint32_t num)
{
	int i, vma_count = 0;
	uint32_t offset = 0;
	Elf64_Phdr phhdr;

	for(i = 0; i < num; i++)
	{
		offset = pgoffset + i * size;
		//kprintf("header seek offset = %x", offset);
		_vfsseek(efd, offset);

		_vfsread(efd, (uint8_t *) &phhdr, size);

		kprintf("p_type =%x p_flags = %x \n", phhdr.p_type, phhdr.p_flags);
		kprintf("p_offset =%x p_vaddr = %x \n", phhdr.p_offset, phhdr.p_vaddr);
		kprintf("p_paddr =%x p_filesz = %x \n", phhdr.p_paddr, phhdr.p_filesz);
		kprintf("p_memsz =%x p_align = %x \n", phhdr.p_memsz, phhdr.p_align);

//uint64_t startva;
//	uint64_t size;
//	uint64_t *next;
//	uint64_t offset_fs;
//	uint32_t permissions;

		if(phhdr.p_type == 0x1)			// LOAD section
		{
			p->vma[vma_count].startva = phhdr.p_vaddr;
			p->vma[vma_count].size = phhdr.p_memsz;
			p->vma[vma_count].offset_fs = offset;			///*************** check this
			p->vma[vma_count].permissions = phhdr.p_flags;

			vma_count++;
		}
	}

	p->numvma = vma_count;

}
uint32_t loadelffile(pcb *p, int16_t efd)
{
	Elf64_Ehdr elfhdr;

	if(p->mfdes[efd].status == 0)		// file is not opened
		return 0;

	_vfsseek(efd, 0);
	elfreadheader(efd, &elfhdr);

	kprintf("elfhdr.e_entry =%x\n", elfhdr.e_entry);
	//kprintf("elfhdr.e_type =%x\n", elfhdr.e_type);
	kprintf("elfhdr.e_phoff PG =%x\n", elfhdr.e_phoff);
	kprintf("efelfhdrd.e_phentsize PG size =%x\n", elfhdr.e_phentsize);
	kprintf("efelfhdrd.e_phnum PG elements =%x\n", elfhdr.e_phnum);
	
	//_vfsseek(efd, elfhdr.e_phoff);
	parsetheprogramheader(p, efd, elfhdr.e_phoff, elfhdr.e_phentsize, elfhdr.e_phnum);

	// update the PCB
	p->_start_addr = elfhdr.e_entry;

	return 0;
}

void clear_page_table_entry(uint64_t va){
	uint32_t pml4_off = get_pml4(va);
	uint32_t pdp_off = get_pdp(va);
  uint32_t pd_off = get_pd(va);
  uint32_t pt_off = get_pt(va);

  uint64_t *va_pml4 = (uint64_t *)get_va_add(pcb_struct[current_process].cr3);
  if((va_pml4[pml4_off] & 0x1) == 1){
  	uint64_t *va_pdp = (uint64_t *)get_va_add((uint64_t)va_pml4[pml4_off] & 0xFFFFFFFFFFFFF000);
  	if((va_pdp[pdp_off] & 0x1) == 1){
  		uint64_t *va_pd = (uint64_t *)get_va_add((uint64_t)va_pdp[pdp_off] & 0xFFFFFFFFFFFFF000);
  		if((va_pd[pd_off] & 0x1) == 1){
  			uint64_t *va_pt = (uint64_t *)get_va_add((uint64_t)va_pd[pd_off] & 0xFFFFFFFFFFFFF000);
  			if((va_pt[pt_off] & 0x1) == 1){
  				va_pt[pt_off] = 0x2;
  			}
  		}
  	}
  }

}
uint32_t clear_load_file(pcb *p, int16_t efd){

	for(int i = p->numvma-1; i>=0 ;i--){
		clear_page_table_entry(p->vma[i].startva);

		p->vma[i].startva = 0;
		p->vma[i].size = 0;
		p->vma[i].offset_fs = 0;	
		p->vma[i].permissions = 0;
	}

	p->numvma = 0;

	loadelffile(p, efd);

	return 0;
}


void parseelf(pcb *curpcb)
{
	// Not much check about supported files, since file is from tarfs, built by us.
	//read the header or direcly map the header as it is memory
	//Elf64_Ehdr *elfhdr = curpcb->fstartaddr;

	//uint8_t hdr[sizeof(Elf64_Ehdr)];







}



