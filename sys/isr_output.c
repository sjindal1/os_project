#include <sys/defs.h>
#include <sys/kprintf.h>

int ps2_ascii_mapping[256] = {0,0,49,50,51,52,53,54,55,56,57,48,45,61,8,9,113,119,101,114,116,121,117,105,111,112,91,93,10,0,97,115,100,102,103,104,106,107,108,59,39,96,0,92,122,120,99,118,98,110,109,44,46,47,0,42,0,32,0};
int ps2_ascii_shift_mappings[256]= {0,0,33,64,35,36,37,94,38,42,40,41,95,43,8,0,81,87,69,82,84,89,85,73,79,80,123,125,10,0,65,83,68,70,71,72,74,75,76,58,34,126,0,124,90,88,67,86,66,78,77,60,62,63,0,42,0,32,0};

int shift_pressed = 0;
int ctrl_pressed = 0;

#define COUNT_VAL 100

void print_time(uint64_t time);

int timer_counter = COUNT_VAL;
uint64_t time = 0;
char timer_pos = 0;

void timer_print()
{
  if(timer_counter == 0){
    time++;
    print_time(time);
    timer_counter = COUNT_VAL;
  }else{
    timer_counter--;
  }
    return;
}

void int_print()
{
  kprintf("interrupt occurs");
}

void int_32_print()
{
  kprintf("one of the fisrt 6 interrupt occured");
}

void int_6_print()
{
  kprintf("6th interrupt occured\n");
}

void print_time(uint64_t time){
  int sec = time % 60;
  int min = time / 60;
  char time_str[] = "Time Since Boot : 00:00";
  long cursor_pos = ( (PRINT_BUF_ADDRESS+160*24)-2 - 22*2);
  register char *addr = (char *) cursor_pos;
  if(min <= 9){
    time_str[19] = min + 48;
  }else if(min > 9){
    int rem=min%10;
    time_str[19] = rem + 48;
    time_str[18] = (min/10) + 48;
  }
  if(sec <= 9){
    time_str[22] = sec + 48;
  }else if(sec > 9){
    int rem=sec%10;
    time_str[22] = rem + 48;
    time_str[21] = (sec/10) + 48;
  }
  for(int i =0; time_str[i] != '\0'; i++, addr+=2){
    *addr = time_str[i];
  }
}
 

void key_press_handle()
{
   long cursor_pos = ( (PRINT_BUF_ADDRESS+160*24)-2 - 40*2);
   register char *addr = (char *)cursor_pos;
   unsigned char scan_code;
   __asm__ __volatile__ ("inb $0x60, %%al\n\t"
                         "movb %%al, %0"
                         :"=r"(scan_code)
                         :
                         :"memory");
   char str[] = "Key Pressed :   " ;
   if(scan_code < 129){
     if(scan_code == 42 || scan_code == 54){
       shift_pressed = 1;
     } 
     else if(scan_code == 29){
       ctrl_pressed = 1;
     }else{
       if(shift_pressed == 1){
         str[13] = ' ';
         str[14] =  ps2_ascii_shift_mappings[scan_code];
       }else if(ctrl_pressed == 1){
         str[13] = '^';
         str[14] =  ps2_ascii_mapping[scan_code];
       }
       else{
        str[13] = ' ';
         str[14] =  ps2_ascii_mapping[scan_code];
       }
       for(int i = 0;str[i]!='\0';i++, addr+=2){
         *addr = str[i];
       }
     }
   }else{
     if(scan_code == 170 || scan_code == 182){
       shift_pressed = 0;
     } 
     else if(scan_code == 157){
       ctrl_pressed = 0;
     }
   }
}
 
