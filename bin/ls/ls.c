#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>

int main(int argc, char *argv[], char *envp[]) {
  char *fileptr = NULL;

  char filename[256];
  int i = 0;

  char *path = argv[1];
  char path_parts[3][256];

  if(strStartsWith((char *)path, "/") == 0){
    fileptr = (char *)path;
  }else{    
    while(envp[i] != 0){
      char *env_var = envp[i];
      if(strStartsWith(env_var, "PWD=") == 0){
        strspt(env_var, path_parts, '=');
        if(argc == 1){
          fileptr = path_parts[1];
        }else{
          strconcat(path_parts[1], (char *)path, filename);
          fileptr = (char *)filename;
        }
        break;
      }
      i++;
    }
  }

  int lastchar = strlen(fileptr);
  if(fileptr[lastchar -1] != '/'){
    fileptr[lastchar] = '/';
    fileptr[++lastchar] = '\0';
  }
  
  int result = opendir(fileptr);
  if(result == 1){
    readdir(fileptr);
  }else{
    printf("no such path exists\n");
  }

  return result; 
}


