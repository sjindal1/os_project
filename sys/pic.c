#include <sys/defs.h>
#include <sys/pic.h>

#define PIC1_INIT 0x20
#define PIC2_INIT 0xA0
#define PIC1_COM 0x21
#define PIC2_COM 0xA1

void init_pic(){

  __asm__ ("mov $0x11, %%al\n\t"
           "outb %%al, %0\n\t"
           "outb %%al, %1\n\t"
           "mov $0x20, %%al\n\t"
           "outb %%al, %2\n\t"
           "mov $0x28, %%al\n\t"
           "outb %%al, %3\n\t"
           "mov $0x04, %%al\n\t"
           "outb %%al, %2\n\t"
           "mov $0x02, %%al\n\t"
           "outb %%al, %3\n\t"
           "mov $0x01, %%al\n\t"
           "outb %%al, %2\n\t"
           "outb %%al, %3\n\t"
           : 
           : "Nd"(PIC1_INIT),"Nd"(PIC2_INIT),"Nd"(PIC1_COM),"Nd"(PIC2_COM)
           : "memory"
           );
}

