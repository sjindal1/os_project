#include <stdio.h>
#include <string.h>
#include <unistd.h>


int main(int argc, char *argv[], char *envp[]) {

	char *filepath;	
	char filename[256];
	if(strStartsWith(argv[1], "/") == 0){ //absolute path
		filepath = argv[1];
	}else{                           //relative path
		int i = 0;
		while(envp[i] != 0){
      char *env_var = envp[i];
      if(strStartsWith(env_var, (char *)"PWD=") == 0){
        char path_parts[3][256];
        strspt(env_var, path_parts, '=');
        strconcat(path_parts[1], argv[1], filename);
        filepath = filename;
      }
      i++;
    } 
	}
	int fd = open(filepath, 0);
	//printf("%s fd - %d\n", filepath, fd);
	if(fd == -1){
		printf("file does not exist please check the filename\n");
	}else{
		char filecontent[256];
		int read_count = read(fd, filecontent, 256);
		if(read_count > 0){
			printf("%s size - %d\n", filecontent, read_count);
		}
	}
}