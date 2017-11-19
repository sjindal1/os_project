#include <sys/defs.h>
#include <sys/kernel.h>
#include <sys/kprintf.h>
#include <sys/syscalls.h>
#include <sys/kernel.h>
#include <sys/tarfs.h>
#include <sys/terminal.h>
#include <sys/vfs.h>

extern tarfsinfo ftarinfo[256];

//returning the addr where the file located
uint64_t _vfsopen(uint8_t* filename)
{
	// only tarfs is supported to open the file 
	return _tarfsopen(filename);
}

uint32_t _vfsread(uint16_t fd, uint8_t *buffer, uint16_t size)
{
	if(pcb_struct[current_process].mfdes[0].status == 0)
	{
	  	return 0; 
	}
	uint64_t retvalue = 0;
	//get the fd type
	uint32_t fdtype = pcb_struct[current_process].mfdes[fd].type;

	if(fdtype == TERMINAL)
	{
		retvalue = _termread((uint8_t *) buffer, size);
	}
	else if(fdtype == TARFS)
	{
		uint64_t start_add = pcb_struct[current_process].mfdes[fd].addr + pcb_struct[current_process].mfdes[fd].offset;
		retvalue = _tarfs_read(start_add, (uint8_t *)buffer, size);
		pcb_struct[current_process].mfdes[fd].offset += retvalue;	
	}
	else 
	{
		kprintf("_vfsread: It is not supported \n");
	}

	return retvalue;
}

uint32_t _vfswrite(uint16_t fd, uint8_t* buffer, uint16_t size)
{
	if(pcb_struct[current_process].mfdes[0].status == 0)
	{
  		return 0; 
  	}
	//get the fd type
	uint32_t fdtype = pcb_struct[current_process].mfdes[fd].type;
	uint32_t retvalue = 0;

	if(fdtype == TERMINAL)
	{
		retvalue = _termwrite((uint8_t *) buffer, size);
	}
	else if(fdtype == TARFS)
	{
		kprintf("_vfsread: Tarfs is not supported \n");
	}
	else 
	{
		kprintf("_vfswrite: It is not supported fd = %x \n", fd);
	}

	return retvalue;
}


