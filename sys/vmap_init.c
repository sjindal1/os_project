#include <sys/defs.h>
#include <sys/kprintf.h>
#include <sys/paging.h>

page_frame_t *head;
page_frame_t *free_page;

void create4KbPages(uint32_t *modulep,void *physbase, void *physfree){
  struct smap_t {
    uint64_t base, length;
    uint32_t type;
  }__attribute__((packed)) *smap;
  uint64_t free_mem_boundaries[40];
  uint32_t i = 0;
  //free_mem_boundaries[i++] = 0;
  //free_mem_boundaries[i++] = (uint64_t)physbase;
  free_mem_boundaries[i++] = (uint64_t)physfree;
  //int total_free_mem = 0;
  while(modulep[0] != 0x9001) modulep += modulep[1]+2;
  for(smap = (struct smap_t*)(modulep+2); smap < (struct smap_t*)((char*)modulep+modulep[1]+2*4); ++smap) {
    if (smap->type == 1 /* memory */ && smap->length != 0) {
      if(smap->base > (uint64_t)physfree){
        free_mem_boundaries[i++] = smap->base;  
        kprintf("Free boundary entry %p\n", free_mem_boundaries[i-1]);
      }
      if((smap->base + smap->length) > (uint64_t)physfree){
        free_mem_boundaries[i++] = smap->base + smap->length;
	//total_free_mem += free_mem_boundaries[i-1] - free_mem_boundaries[i-2];
	kprintf("Free boundary entry%p\n", free_mem_boundaries[i-1]);
      }
    }
  }
  head = (page_frame_t*)physfree;
  //kprintf("total no of pages %d\n",total_free_mem/4096);
  //uint64_t array_length = ((total_free_mem/4096)*32)/4096;
  //free_page = head + array_length;
  //kprintf("array length %d\n", array_length);
  int j=0;
  uint64_t page_des = 0, mem = free_mem_boundaries[j++];
  while(j<i){
    page_frame_t *t = head + page_des;
    /*if(m<array_length){
      t->start = (uint64_t*) n;
      t->info = (uint64_t) 1;
      //head++;
    }else{*/
      t->start = (uint64_t*) mem;
      t->info = (uint64_t) 0;
      //head++;
    //}
    page_des ++;
    if(mem + 4096 < free_mem_boundaries[j]){
      mem += 4096;
    }else{
      j++;
      mem = free_mem_boundaries[j++];
    }
  }
  //int total_mem = page_des * 4096;	// full ram value
  //int occupied_mem = *(head + page_des)->start; 
  //int array_mem = page_des * 32;
  int array_page = page_des * 32 / 4096 + 1; // +1 is buffer
  //int tot_m = m - array_page
  page_frame_t *link_start = head + array_page;

  kprintf("no of pages used to store array %d\n", array_page);
  for(int i=0;i<array_page;i++){
    page_frame_t *t = head + i;
    t->info=1;
  }
  
  // The link list will start after the page descriptor have finished
  // first page
  page_frame_t *t = link_start;
  t->prev = NULL;
  t->next = (page_frame_t*)(link_start + 1);
  
  // last element
  t = head + page_des - 1;
  t->prev = (page_frame_t *)(head+page_des-2);
  t->next = NULL;
  
  kprintf("prev -> %x, next -> %x t-> %x  \n", t->prev,t->next, t);
  
  for(int i = 1;i < page_des - 1 - array_page;i++){
    t = link_start + i;
    t->prev = t-1;
    t->next = t+1;
  }
  for(int i= 0; i < 6;i++){
    t = link_start + i;
    kprintf("prev -> %x, next -> %x t-> %x  \n", t->prev,t->next, t);
  }

  kprintf("no of page frames created %d\n", page_des);
  free_page = link_start;
  uint64_t* page = get_free_page();
  kprintf("get free page %x\n", page);
  free(page);
  page = get_free_page();
  kprintf("get free page 2nd time %x\n", page); 
}

uint64_t* get_free_page(){
  page_frame_t *t = free_page;
  t->info = 1;
  free_page = t->next;
  return t->start;
}

void free(uint64_t* address){
  page_frame_t *t = free_page;
  free_page = t->prev;
  free_page->info = 0;
  free_page->start = address;
}
