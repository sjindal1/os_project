#include <sys/defs.h>
#include <sys/idt.h>

#define MAX_IDT 256

void _x86_64_timer_isr();

void _x86_64_keyboard_isr();

void _x86_64_isr();

void _x86_64_isr_32();

struct idt_descriptor {
  uint16_t offset_1;
  uint16_t selector;
  uint8_t ist;
  uint8_t type_attr;
  uint16_t offset_2;
  uint32_t offset_3;
  uint32_t zero;
}__attribute__((packed));

struct idtr_t {
  uint16_t size;
  uint64_t addr;
}__attribute__((packed));

void set_idt_values(uint64_t isr_pointer, uint16_t selector, uint8_t type, struct idt_descriptor *idt_element);

struct idt_descriptor idt[MAX_IDT];

static struct idtr_t idtr = { (sizeof(struct idt_descriptor) * MAX_IDT) -1, (uint64_t)idt };

void init_idt(){
  uint64_t isr_point_32 = (uint64_t)&_x86_64_isr_32;
  uint64_t isr_point = (uint64_t)&_x86_64_isr;
  uint64_t isr_timer_point = (uint64_t)&_x86_64_timer_isr;
  uint64_t isr_keyboard_pointer = (uint64_t)&_x86_64_keyboard_isr;
  for(int i=0;i<7;i++){
    struct idt_descriptor *idt_element = &idt[i];
    set_idt_values(isr_point_32, 0x08, 0x8E, idt_element); 
  }
  for(int i=7;i<MAX_IDT;i++){
    struct idt_descriptor *idt_element = &idt[i];
    set_idt_values(isr_point, 0x08, 0x8E, idt_element);
  }

  struct idt_descriptor *timer = &idt[32];
  set_idt_values(isr_timer_point, 0x08, 0x8E, timer);

  struct idt_descriptor *keyboard = &idt[33];
  set_idt_values(isr_keyboard_pointer, 0x08, 0x8E, keyboard);

  __asm__ __volatile__ ("lidt (%0)\n\t"
                        "sti\n\t"
                        :
                        :"r"(&idtr)
                        :"memory");
   
}

void set_idt_values(uint64_t isr_pointer, uint16_t selector, uint8_t type, struct idt_descriptor *idt_element){
  idt_element->zero = 0;
  idt_element->type_attr = type;
  idt_element->ist = 0;
  idt_element->selector = selector; 
  idt_element->offset_1=isr_pointer & 0xffff;
  idt_element->offset_2=(isr_pointer >> 16) & 0xffff;
  idt_element->offset_3=(isr_pointer >> 32) & 0xffffffff; 
}
