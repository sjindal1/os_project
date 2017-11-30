

// String function for user space operations

#include <sys/defs.h>

int strcmp(int8_t *string1, int8_t *string2){
  int i=0,j=0;
  for(;string1[i]!='\0' && string2[j] !='\0' && string1[i] == string2[j]; ++i,++j){
    ;
  }
  return string1[i]-string2[j];
}

int strlen(int8_t *str)
{ 
  uint64_t len = 0, i;
  
  for(i = 0; str[i] != '\0'; i++)
  { 
    len++;
  }
  
  return len;
}

