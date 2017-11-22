#include <sys/defs.h>
#include <sys/kernel.h>
#include <sys/kprintf.h>
#include <sys/syscalls.h>
#include <sys/kernel.h>
#include <sys/tarfs.h>
#include <sys/terminal.h>
#include <sys/vfs.h>

extern tarfsinfo ftarinfo[256];

int16_t get_first_free_fd(){
	int32_t fd = 3;
	for( ;fd<16;fd++){
		if(pcb_struct[current_process].mfdes[fd].status == 0)
		  break;
	}
	if(fd == 16){
		return -1;
	}else{
		return fd;
	}
}

//returning the addr where the file located
int16_t _vfsopen(uint8_t* filename)
{
	uint64_t fileaddr = _tarfsopen(filename);
	if(fileaddr == 0){
		return -1;
	}
	int16_t fd = get_first_free_fd();
	// only tarfs is supported to open the file
	if(fd == -1){
		return -1;
	}else{
		pcb_struct[current_process].mfdes[fd].status = 1;
		pcb_struct[current_process].mfdes[fd].type = TARFS;
		pcb_struct[current_process].mfdes[fd].addr = fileaddr;
		pcb_struct[current_process].mfdes[fd].offset = 0;
		pcb_struct[current_process].mfdes[fd].permissions = 0xff;
		pcb_struct[current_process].elf_start = fileaddr;
	} 
	return fd;
}

uint32_t _vfsread(int16_t fd, uint8_t *buffer, uint16_t size)
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
		//kprintf("vfs offsets read %x \n", pcb_struct[current_process].mfdes[fd].offset);
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

uint32_t _vfswrite(int16_t fd, uint8_t* buffer, uint16_t size)
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


void _vfsseek(int16_t fd, uint32_t offset)
{
       if(pcb_struct[current_process].mfdes[0].status == 0)
       {
               return; 
       }
       //get the fd type
       uint32_t fdtype = pcb_struct[current_process].mfdes[fd].type;

       if(fdtype == TERMINAL)
       {
               kprintf("_vfsseek: Seek is not supported for terminal = %x\n", fd);
       }
       else if(fdtype == TARFS)
       {
               pcb_struct[current_process].mfdes[fd].offset = offset;
       }

       return;
}


