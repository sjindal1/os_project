#include <sys/stdarg.h>
#include <unistd.h>
#include <string.h>

unsigned int sys_sleep();

unsigned int sleep(unsigned int seconds){
	unsigned int cur_time = sys_sleep();
	unsigned int time = cur_time;
  while(time <= cur_time + seconds){
  	time = sys_sleep();
  }
  return time;
}

pid_t wait(int *status)
{
	return waitpid(-1, status);
}