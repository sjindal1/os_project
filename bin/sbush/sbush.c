#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
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

void display_prompt()
{
  char *buf = NULL;//, *buf1 = NULL;
  int pwd, err;
  char *pwdstr = NULL;
  retps1pwd(&buf, &pwd, &pwdstr);

  if(pwd)
  {
    err = write(1, buf, strlen(buf)-2);
    err = write(1, ":", 1);
    err = write(1, pwdstr, strlen(pwdstr));
  }
  else
    err = write(1, buf, strlen(buf));

  err = write(1, "> ", 2);
  mprint(err);

  return;
}

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

int chdir(const char *path){
  char *fileptr = NULL;

  char filename[256];
  int i = 0;

  if(strStartsWith((char *)path, "/") == 0){
    fileptr = (char *)path;
  }else{    
    while(penv[i] != 0){
      char *env_var = penv[i];
      if(strStartsWith(env_var, "PWD=") == 0){
        char path_parts[3][256];
        strspt(env_var, path_parts, '=');
        strconcat(path_parts[1], (char *)path, filename);
        fileptr = (char *)filename;
        break;
      }
      i++;
    }
  }

  if(strcmp((char *)path, "..") == 0){
    int lastchar_path = strlen(penv[i]);
    lastchar_path--; 
    if(strcmp(penv[i], "PWD=/") != 0){
      while(penv[i][--lastchar_path] != '/'); //lastchar_path-- would be "/" so start from one less 
      penv[i][++lastchar_path] = '\0';
      return 1;
    }else{
      return 0;
    }
  }

  int lastchar = strlen(fileptr);
  if(fileptr[lastchar -1] != '/'){
    fileptr[lastchar] = '/';
    fileptr[++lastchar] = '\0';
  }
  
  int result = access(fileptr);
  if(result == 1){
    penv[i][0] = 'P';
    penv[i][1] = 'W';
    penv[i][2] = 'D';
    penv[i][3] = '=';
    int j = 4,k=0;
    while(fileptr[k] != '\0'){
      penv[i][j] = fileptr[k];
      j++;
      k++;
    }
    penv[i][j] = '\0';
  }

  return result; 
}


int8_t check_command_exists(char *path){
  char *fileptr = NULL;

  char filename[256];

  if(strStartsWith(path, (char *)"/") == 0){
    fileptr = path;
  }else{
    int i = 0;
    while(penv[i] != 0){
      char *env_var = penv[i];
      if(strStartsWith(env_var, "PATH=") == 0){
        char path_parts[3][256];
        strspt(env_var, path_parts, '=');
        strconcat(path_parts[1], path, filename);
        fileptr = (char *)filename;
        break;
      }
      i++;
    } 
  }

  int result = access(fileptr);
  return result;
}

int get_int_val(char * str){
  int len = strlen(str);
  int result = 0, i=1;
  len--;
  result = (str[len--]%48);
  while(len >= 0){
    result += (str[len]%48)*(i*10);
    len--;
    i++;
  }
  return result;
}

void call_kill(int no_of_arguments){
  if(no_of_arguments != 3){
    printf("invalid command\n");
  }else{
    int sig = get_int_val((char *)&args[1][1]);
    pid_t pid = (pid_t) get_int_val(args[2]);
    int status = kill(pid, sig);
    if(status == 0){
      printf("killed process %d", (int)pid);
    }else{
      printf("invalid pid\n");
    }
  }
}

int bg = 1;

void checkforbackground(int no_of_arguments)
{
  int i = 0;

  bg = 1;

  if(no_of_arguments == 1)
  {
    int len = strlen(args[0]);

    if(args[0][len - 1] == '&')
    {
      bg = 2;
      args[0][len - 1] = '\0';
    }
  }
  else
  {
    for(i = 0; ; i++)
    {
      if(args[i][0] == '\0')
        break;

      if((args[i][0] == '&') && (args[i][1] == '\0'))
      {
        bg = 2;
        break;
      }
    }

    if(bg == 2)        // it is background remove the & from cmd buffer and update pointers
    {
      ///args[i][0] = '\0';
     //pcmd[i] = NULL;
      args[i] = NULL;
    }
  }

  return;
}


void execute_commands(int no_of_arguments){
  if(no_of_arguments == 0){
    return;
  }if(strStartsWith(args[0], "echo") == 0){
    for(int i=1;i<no_of_arguments;i++){
      printf("%s ", args[i]);
    }
    printf("\n");
  }else if(strStartsWith(args[0], "pwd") == 0){
    print_pwd();
  }else if(strStartsWith(args[0], "cd") == 0){
    int result = chdir(args[1]);
    if(result == 0){
      printf("invalid directory\n");
    }
  }else if(strcmp(args[0], "exit") == 0){
    exit(0);
  }else if(strcmp(args[0], "sleep") == 0){
    unsigned int secs = (unsigned int)get_int_val(args[1]);
    sleep(secs);
  }else if(strStartsWith(args[0], "kill") == 0){
    call_kill(no_of_arguments);
  }
  else
  {
    checkforbackground(no_of_arguments); 
    if(check_command_exists(args[0]) == 1){
      int pid = fork();
      if(pid == 0){
        myexec(no_of_arguments);
        exit(0);
      }else{
        if(bg == 1)
          waitpid(pid, NULL);
        bg = 1;
      }
    }else{
      printf("invalid command\n");
    }
  }
}

int main(int argc, char *argv[], char *envp[]) {
	int err;
	settheenviron(envp);
	char input_buf[256];
  printf("Welcome to SBUIX\n");
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

