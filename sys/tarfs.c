#include <sys/defs.h>
#include <sys/kprintf.h>
#include <sys/kernel.h>
#include <sys/tarfs.h>
#include <sys/utils.h>

uint16_t tarfsfilecount = 0;

tarfsinfo ftarinfo[256];

uint32_t get_int_size(char *size){
  uint32_t int_size=0,fac = 1;
  for(int i=10;i>=0;i--){
    int_size += ((size[i]-48)*fac);
    fac = fac *8;
    //kprintf("hhh->%d",(size[i]-48)*fac);
  }
  return int_size;
}

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


uint64_t _tarfs_read(){
  return 0;
}

void init_tarfs(){
  kprintf("tarfs start %x tarfs end %x\n", &_binary_tarfs_start,&_binary_tarfs_end);
  posix_header_ustar* temp = (posix_header_ustar*)&_binary_tarfs_start;
  uint8_t* byteptr = (uint8_t*) temp;
  int i=0;
  while((uint64_t) temp < (uint64_t) &_binary_tarfs_end){//|| (uint64_t) temp < (uint64_t) &_binary_tarfs_end){
    if(temp->name[0] == '\0' || temp->size[0] == '\0'){
      break;
    }
    char *size = temp->size;
    uint32_t int_size = get_int_size(size);
    kprintf("n -> %s, s_string - > %s , s_int -> %d, temp - %x \n", temp->name, size, int_size, temp);
    if(int_size>0){  
      uint32_t last = int_size % 512;
      int_size = int_size + 512 - last;
    }

    byteptr += int_size + 512;

    //save the file info
    ftarinfo[tarfsfilecount].fname = (uint8_t *)temp->name;
    ftarinfo[tarfsfilecount].fsize = int_size;
    ftarinfo[tarfsfilecount].fstartaddr = (uint64_t)(byteptr - int_size);

    temp = (posix_header_ustar*) byteptr;
    i++;
  }
}