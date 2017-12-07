#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char *argv[], char *envp[]) {
	argv[0] = "sbush";
	argv[1] = NULL;
	while(1){
		int pid = fork();
		if(pid == 0){
			execvpe(argv[0],argv,envp);
		}else{
			waitpid(pid, NULL);
		}
	}
}