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

uint64_t strlen(uint8_t *str)
{ 
  uint64_t len = 0, i;
  
  for(i = 0; str[i] != '\0'; i++)
  { 
    len++;
  }
  
  return len;
}

int strStartsWith(uint8_t origStr[], uint8_t checkStr[]){
  int i=0, result=0;
  while(checkStr[i] != '\0'){
    if(origStr[i] == '\0' || origStr[i]!=checkStr[i]){
      result = 1;
      break;
    }else{
      i++;
    }
  }
  return result;
}

int strspt(uint8_t * input, uint8_t str[][256], uint8_t delim){
  int i=0,j=0,k=0;
  for(;input[i] != '\0';i++){
    if(input[i] == delim){
      if(k>0){
        str[j][k]= '\0';
        k=0;
        j++;
      }
    }else{
      str[j][k] = input[i];
      k++;
    }
  }
  if(k>0){
    str[j][k]='\0';
    j++;
  }
  str[j][0]='\0';
  return j;
}


int strcontains(uint8_t *input, uint8_t uint8_tacter){
  int i = 0;
  for(;input[i] != '\0' && input[i] != uint8_tacter;i++){
    ;
  }
  if(input[i] == '\0'){
    return 0;
  }else{
    return i;
  }
  return 0;
}

uint8_t strlastuint8_t(uint8_t *input){
  int i = 0;
  for(;input[i] != '\0'; i++){
    ;
  }
  if(i == 0){
    return '\0';
  }
  return input[i-1];
}

uint8_t* trimwhitespaces(uint8_t *input){
  int i = 0;
  for(;input[i] == ' '; i++)
    input++;
  return input;
}

void strConcat(uint8_t *first, uint8_t *second, uint8_t *final){
  int i=0;
  for(;first[i] != '\0';i++){
    final[i] = first[i];
  }
  int j=0;
  for(;second[j] != '\0';j++){
    final[i+j] = second[j];
  }
  final[i+j] = '\0';
}
