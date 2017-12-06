#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

char myenv[10][256];
char *penv[10];
int test_var = 1000;
int totalenv = 1;
char input_args[10][256];
char *args[10];

void settheenviron(char *envp[])
{
  int i, j;

  totalenv = 0;

  for(i = 0; i < 10; i++)
  {
    penv[i] = (char*)&myenv[i][0];

    if(envp[i] == 0 || i > 9)
      break;//return;

    totalenv++;
    for(j = 0; envp[i][j] != '\0'; j++)
    {
      if(j > 250) break;
      myenv[i][j] = envp[i][j];
    }
    myenv[i][j] = '\0';
  }

  penv[totalenv] = (char*)&myenv[totalenv][0];

  strcpy(&myenv[totalenv][0], "PS1=sbush-p");        // add "> " while display
  totalenv++;

  penv[totalenv] = NULL;

  //breakpath();
  return;
}

void mprint(int err)
{
	;
}

void mpprint(int * p)\
{
  ;
}

void retps1pwd(char **b, int *pwd, char **strpwd)
{
  int i, j;

  for(i = 0; i < totalenv; i++)
  {
    if(myenv[i][0] == 'P' &&
        myenv[i][1] == 'S' &&
         myenv[i][2] == '1' &&
          myenv[i][3] == '=')
    {
      j = strlen(&myenv[i][4]);

      *pwd = 0;
      if(myenv[i][4+j-2] == '-' && myenv[i][4+j-1] == 'p')
        *pwd = 1;

      *b = &myenv[i][4];
    }
  }

  for(i = 0; i < totalenv; i++) {
	  if(myenv[i][0] == 'P' &&
		  myenv[i][1] == 'W' &&
	  	 myenv[i][2] == 'D' &&
	    	myenv[i][3] == '='){
	  
	  	*strpwd = &myenv[i][4];
	  }

  }

  return;
}

int malflag = 1;

void display_prompt()
{
  char *buf = NULL;//, *buf1 = NULL;
  int pwd, err;
  char *pwdstr = NULL;
  retps1pwd(&buf, &pwd, &pwdstr);
  int *ptr = NULL;

  if(pwd)
  {
    err = write(1, buf, strlen(buf)-2);
    err = write(1, ":", 1);
    err = write(1, pwdstr, strlen(pwdstr));
  }
  else
    err = write(1, buf, strlen(buf));

  err = write(1, "> ", 2);

  if(malflag == 1)
    ptr = (int*) malloc(10);

  mprint(err);
  mpprint(ptr);

  return;
}

char *ls_args[3] = {"cat", "bin/sahil.txt"};

void myexec(int no_of_arguments){
  //ls_args[2] = NULL;
  execvpe(args[0],args,penv);
}

void print_pwd(){
  int i = 0;
  while(penv[i] != 0){
    char *env_var = penv[i];
    if(strStartsWith(env_var, (char *)"PWD=") == 0){
      char path_parts[3][256];
      strspt(env_var, path_parts, '=');
      printf("%s\n", path_parts[1]);
      break;
    }
    i++;
  } 
}


int8_t check_command_exists(){
  char *fileptr;

  char *path = args[0];

  char filename[256];

  if(strStartsWith(path, (char *)"/") == 0){
    fileptr = path;
  }else{
    int i = 0;
    while(penv[i] != 0){
      char *env_var = penv[i];
      if(strStartsWith(env_var, (char *)"PATH=") == 0){
        char path_parts[3][256];
        strspt(env_var, path_parts, '=');
        strconcat(path_parts[1], path, filename);
        fileptr = (char *)filename;
      }
      i++;
    } 
  }

  int result = access(fileptr);
  return result;
}

void execute_commands(int no_of_arguments){
  if(strStartsWith(args[0], "echo") == 0){
    for(int i=1;i<no_of_arguments;i++){
      printf("%s ", args[i]);
    }
    printf("\n");
  }else if(strStartsWith(args[0], "pwd") == 0){
    print_pwd();
  }else if(args[0][0] == '\0'){
    return;
  }else{
    if(check_command_exists() == 1){      
      int pid = fork();
      if(pid == 0){
        myexec(no_of_arguments);
        exit(0);
      }else{
        waitpid(pid, NULL);
      }
    }else{
      printf("Invalid command\n");
    }
  }
}

int main(int argc, char *argv[], char *envp[]) {
	int err;
	settheenviron(envp);
	char input_buf[256];
	while(1)
	{
		display_prompt();
		err = read(0,input_buf,256);
		input_buf[err] = '\0';
		int no_of_arguments = strspt(input_buf, input_args, ' ');
	  for(int arg_no = 0; arg_no < no_of_arguments; arg_no++){
      args[arg_no] = input_args[arg_no];
	    //printf("%s\n", args[arg_no]);
	  }
    args[no_of_arguments] = NULL;

    execute_commands(no_of_arguments);
		//I have to call read now
		//while(1);
	}

	return 0;
}

