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
	uint64_t retvalue = 0;
	//get the fd type
	uint32_t fdtype = pcb_struct[current_process].mfdes[fd].type;

	if(fdtype == TERMINAL)
	{
		retvalue = _termread((uint8_t *) buffer, size);
	}
	else if(fdtype == TARFS)
	{
		//retvalue = _tarfs((uint8_t *)buffer, size);
	}
	else 
	{
		kprintf("_vfsread: It is not supported \n");
	}

	return retvalue;
}

uint32_t _vfswrite(uint16_t fd, uint8_t* buffer, uint16_t size)
{
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
		kprintf("_vfsread: It is not supported \n");
	}

	return retvalue;
}


