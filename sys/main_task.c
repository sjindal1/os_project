#include <sys/defs.h>
#include <sys/kprintf.h>
#include <sys/kernel.h>
#include <sys/paging.h>

extern uint64_t *kernel_cr3;

void kernel_1_thread();
void yeild();
void switch_to(pcb* , pcb*);

pcb pcb_entries[1024];
uint64_t thread_st[512];
int free_pcb=0;
int no_of_task=0;
int current_process=0;

void push_asm()
{
	//__asm__{
	//	""
	//}
}

uint64_t get_rflags_asm()
{
	uint64_t rflags;
	__asm__ __volatile__(
		"PUSHFQ \n\t"
		"POPQ %%rax\n\r"
		"MOVQ %%rax, %0"
		:"=m"(rflags)
		:
		:"%rax");
	return rflags;
}

void main_task(){
	uint64_t *tmp;
	//setting main task PCB
    pcb_entries[free_pcb].pid = free_pcb;
    pcb_entries[free_pcb].cr3 = (uint64_t)kernel_cr3;
    pcb_entries[free_pcb].state = 0;
    pcb_entries[free_pcb].exit_status = -1;
    free_pcb++;
    no_of_task++;
    
    //Kernel Thread 1 PCB
    pcb_entries[free_pcb].pid = free_pcb;
    pcb_entries[free_pcb].cr3 = (uint64_t)kernel_cr3;
    pcb_entries[free_pcb].state = 0;
    pcb_entries[free_pcb].exit_status = -1;
    pcb_entries[free_pcb].kstack = (uint64_t*)&thread_st;
    pcb_entries[free_pcb].rsp = (uint64_t)(pcb_entries[free_pcb].kstack) + 0xF80;

    //kprintf("kstack of second thread kstack->%x rsp-> %x\n",pcb_entries[free_pcb].kstack,pcb_entries[free_pcb].rsp);

     
    //Initialize Thread 1
    // set structures of reg = 0;
    tmp = (uint64_t*) pcb_entries[free_pcb].rsp;
    //push the start address of thread
    *tmp-- = (uint64_t) &kernel_1_thread;
    pcb_entries[free_pcb].rsp -= 8;
    for(int i = 14; i > 0; i--)
    {
    	*(tmp--) = 0;
    	pcb_entries[free_pcb].rsp -= 8;
    }

    uint64_t rflags = get_rflags_asm();
    *tmp-- = rflags;
    *tmp-- = (uint64_t)&pcb_entries[free_pcb];
    pcb_entries[free_pcb].rsp -= 8;		// finaly value of RSP
    
    //kprintf("rsp after i have Initialize process 2 -> %x\n",pcb_entries[free_pcb].rsp);

 	// update the pcb structure
    free_pcb++;
    no_of_task++;

    int j=0;
  	while(j<2){
      kprintf("This is the main_task of kernel thread\n");
      j++;
      yeild();
      kprintf("returning to main thread\n");
 	}  

	return;
}

void kernel_1_thread(){
  //int j = 0;
  while(1){
    kprintf("This is the first kernel thread\n");
    yeild();
    kprintf("returning to kernel_1_thread\n");
  }
}

void yeild(){
  //round robin scheduler
  pcb *me = &pcb_entries[current_process],*next = NULL;
  if(no_of_task == 1){
  	return;
  }
  if(current_process+1 == no_of_task){
    next = &pcb_entries[0];
    current_process = 0;
  }else{
  	int i;
    for(i=current_process+1; i<1024; i++){
      if(pcb_entries[i].state == 0){
      	next = &pcb_entries[i];
      	current_process = i;
      	break;
      }
    }
    if(i==1024){
      next = &pcb_entries[0];
      current_process = 0;
    }
  }
  //kprintf("next.rsp -> %x\n",next->rsp, me->rsp);
  switch_to(me, next);
}
