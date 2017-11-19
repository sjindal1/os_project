#include <sys/defs.h>
#include <sys/utils.h>

void memset(void* p, int v, int size){
  uint8_t *point  = p;
  while(size--){
    *point++ = (uint8_t)v;
  }
  return;
}


int strcmp(uint8_t *string1, uint8_t *string2){
  int i=0,j=0;
  for(;string1[i]!='\0' && string2[j] !='\0' && string1[i] == string2[j]; ++i,++j){
    ;
  }
  return string1[i]-string2[j];
}