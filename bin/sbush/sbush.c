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

char *pch[10];
int pptr = 1;

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

#if 1
  if(pptr < 10)
  {
    pch[pptr] = (char *) malloc (256);
    pch[pptr][0] = 'p';
    pch[pptr][1] = 'r';
    pch[pptr][2] = 'u';

    free(pch[pptr]);
    pptr++;


  }
#endif

  return;
}

void myexec(int no_of_arguments){
  //ls_args[2] = NULL;
  execvpe(args[0],args,penv);
}

void print_pwd(){
  char path[100];
  char *path_ptr;
  path_ptr = getcwd((char *)&path[0], 0);
  printf("%s\n", path_ptr);
}

void change_directory(){
  int result = chdir(args[1]);
  if(result != 0){
    printf("invalid directory\n");
  }else{
    int i = 0;
    while(penv[i] != 0){
      char *env_var = penv[i];
      if(strStartsWith(env_var, "PWD=") == 0){
        int len = strlen(env_var);
        int k=0;
        if(strcmp(args[1], "..") == 0){
          while(penv[i][--len] != '/'); //lastchar_path-- would be "/" so start from one less 
          penv[i][++len] = '\0';
        }else{          
          while(args[1][k] != '\0'){
            penv[i][len] = args[1][k];
            len++;
            k++;
          }
          penv[i][len] = '\0';
        }
        break;
      }
      i++;
    }
  }
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
      if(args[i] == NULL)
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
    change_directory();
  }else if(strcmp(args[0], "exit") == 0){
    clearscr();
    exit(0);
  }else if(strcmp(args[0], "clear") == 0){
    clearscr();
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



int executescript(int no_of_arguments)
{
  //return 0;     // 0 - not a script

  char *filepath; 
  char filename[100], cwd[100];

  char data[256], ch;
  int len = 0, i = 0;

  if(no_of_arguments > 1)
    return 0;

  if(strStartsWith(args[0], "/") == 0){ //absolute path
    filepath = args[0];
  }else{                           //relative path
    getcwd((char *)&cwd[0], 0);
    strconcat(cwd, args[0], filename);
    filepath = filename;
  }

  int fd = open(filepath, 0);
  if(fd < 0)
  {
    //printf("Script: File not found\n");
    close(fd);
    return 0;
  }

  len = read(fd, data, 2);
  if(len == 0) {
    close(fd);
    return 0;
  }

  if(data[0] == '!' && data[1] == '#')
  {
    // it is a script
    len = read(fd, (char *) &ch, 1);
    if(len == 0) { close(fd); return 1; }      // bad script

    i = 0;
    while(ch != '\n')
    {
      data[i++] = ch;
      // capture data and check for  /bin/sbush
      len = read(fd, (char *) &ch, 1);
        if(len == 0) { close(fd); return 1; }      // bad script
    }
    data[i] = '\0';
    if(strcmp(data, " /bin/sbush") != 0)
    {
      printf("Script:Invalid sbush path\n");
      close(fd);
      return 1;
    }

    len = read(fd, (char *) &ch, 1);
        if(len == 0) { close(fd); return 1; }      // bad script   
    while(ch != '\n')
    {
      // blank line is mandatory
      len = read(fd, (char *) &ch, 1);
        if(len == 0) { close(fd); return 1; }      // bad script
    }

    i = 0;
    while(read(fd, (char *) &ch, 1) != 0)
    {
      if(ch != '\n')
      {
        data[i++] = ch;
      }
      else
      {
        // execute the command
        data[i] = '\0';
        i = 0;

        int arguments = strspt(data, input_args, ' ');
        for(int arg_no = 0; arg_no < arguments; arg_no++){
          args[arg_no] = input_args[arg_no];
          //printf("%s\n", args[arg_no]);
        }
        args[arguments] = NULL;

        execute_commands(arguments);
      }
    }

    if(i != 0)
    {
      // execute the command
      data[i] = '\0';
      i = 0;

      int arguments = strspt(data, input_args, ' ');
      for(int arg_no = 0; arg_no < arguments; arg_no++){
        args[arg_no] = input_args[arg_no];
        //printf("%s\n", args[arg_no]);
      }
      args[arguments] = NULL;

      execute_commands(arguments);
    }
  }
  else
  {
    close(fd);
    return 0;
  }

  close(fd);
  return 1;
}



int main(int argc, char *argv[], char *envp[]) {
	int err;
	settheenviron(envp);
	char input_buf[256];
  printf("Welcome to SBUIX\n");

  /*int *pru;
  pru = (int*)malloc(24);
  
  *pru = 10;

  printf(" success %d\n", *pru); */

	while(1)
	{
		display_prompt();
		err = read(0,input_buf,256);
    if(err > 0){
  		input_buf[err] = '\0';
  		int no_of_arguments = strspt(input_buf, input_args, ' ');
  	  for(int arg_no = 0; arg_no < no_of_arguments; arg_no++){
        args[arg_no] = input_args[arg_no];
  	    //printf("%s\n", args[arg_no]);
  	  }
      args[no_of_arguments] = NULL;

      err = executescript(no_of_arguments);
      if(err == 0)
        execute_commands(no_of_arguments);
  		//I have to call read now
  		//while(1);
    }
	}

	return 0;
}

