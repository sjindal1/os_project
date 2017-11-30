#include <sys/defs.h>
#include <sys/kprintf.h>
#include <sys/kernel.h>
#include <sys/tarfs.h>
#include <sys/utils.h>

uint16_t tarfsfilecount = 0;

tarfsinfo ftarinfo[256];


uint8_t *envp[10]; 
uint8_t env1_path[] = {"PATH=/bin/"};
uint8_t env2_username[] = {"PWD=/"};

uint8_t *argvuser[4];
uint8_t argvuserpar1[] = {"sbush"};

void set_kernel_environ()
{
  envp[0] = &env1_path[0];
  envp[1] = &env2_username[0];
  envp[2] = NULL;

  argvuser[0] = &argvuserpar1[0];
  argvuser[1] = NULL;
}

uint64_t copy_argv(uint64_t user_arg_va, uint64_t *user_st_va, uint8_t **argv_proc)
{
  uint8_t i = 0, j = 0, len, k;
  uint8_t *st_add = (uint8_t*) (user_arg_va);
  uint64_t ret = (uint64_t) user_st_va;

  user_st_va--;
  user_st_va[0] = 0;

  while(argv_proc[j] != NULL)
  {
    j++;
    user_st_va--;
  }

  user_st_va--;
  user_st_va[0] = j;    // number of arguments
  user_st_va++;

  ret = ret - 16 - j * 8;

  // j is the number of arguments - copy in reverse order
  while(j != 0)
  {
    len = strlen(argv_proc[i]);     // start copy from the 0th element

    user_st_va[0] = (uint64_t) st_add;
    user_st_va++;

    for(k = 0; k < len; k++)
    {
      st_add[k] = argv_proc[i][k];
    }
    st_add[k++] = '\0';

    st_add = st_add + len + 8;
    j--;
  }

  return ret;
}

uint64_t copy_environ(uint64_t user_en_va, uint64_t *user_st_va, uint8_t ** envp_proc)
{
  uint8_t i = 0, j, len;
  uint8_t *st_add = (uint8_t*) (user_en_va + 64);

  user_st_va--;
  user_st_va[0] = 0;

  while(envp_proc[i] != NULL)
  {
    len = strlen(envp_proc[i]);

    user_st_va--;
    user_st_va[0] = (uint64_t) st_add;
    for(j = 0; j < len; j++)
    {
      st_add[j] = envp_proc[i][j];
    }
    st_add[j++] = '\0';

    st_add = st_add + len + 8;
    i++;
  }
  return (uint64_t)user_st_va;
}

uint32_t get_int_size(char *size){
  uint32_t int_size=0,fac = 1;
  for(int i=10;i>=0;i--){
    int_size += ((size[i]-48)*fac);
    fac = fac *8;
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

//TODO
//make sure the size is correct this function does not care for boundaries.
//It reads the size number of bytes no matter what.
uint64_t _tarfs_read(uint64_t start_add, uint8_t *buf, uint32_t size){
  uint8_t *file_pointer = (uint8_t *)start_add;
  for(int i=0;i<size;i++){
    buf[i] = file_pointer[i];
  }
  return size;
}

void init_tarfs(){
  kprintf("tarfs start %x tarfs end %x\n", &_binary_tarfs_start,&_binary_tarfs_end);
  posix_header_ustar* temp = (posix_header_ustar*)&_binary_tarfs_start;
  uint8_t* byteptr = (uint8_t*) temp;
  while((uint64_t) temp < (uint64_t) &_binary_tarfs_end){//|| (uint64_t) temp < (uint64_t) &_binary_tarfs_end){
    if(temp->name[0] == '\0' || temp->size[0] == '\0'){
      break;
    }
    char *size = temp->size;
    uint32_t int_size = get_int_size(size);
    if(int_size>0){  
      uint32_t last = int_size % 512;
      int_size = int_size + 512 - last;
    }

    byteptr += int_size + 512;

    //save the file info
    ftarinfo[tarfsfilecount].fname = (uint8_t *)temp->name;
    ftarinfo[tarfsfilecount].fsize = int_size;
    ftarinfo[tarfsfilecount].fstartaddr = (uint64_t)(byteptr - int_size);

    kprintf("n -> %s, s_int -> %d, temp - %x, fs -> %x \n", temp->name, int_size, temp, ftarinfo[tarfsfilecount].fstartaddr);

    temp = (posix_header_ustar*) byteptr;
    tarfsfilecount++;
  }

  //set the environment variables
  set_kernel_environ();
}