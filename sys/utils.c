#include <sys/defs.h>
#include <sys/utils.h>

void memset(void* p, int v, int size){
  uint8_t *point  = p;
  while(size--){
    *point++ = (uint8_t)v;
  }
  return;
}


