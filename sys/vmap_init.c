#include <sys/defs.h>
#include <sys/kprintf.h>
#include <sys/paging.h>

uint64_t get_pml4(uint64_t kermem);
uint64_t get_pdp(uint64_t kermem);
uint64_t get_pd(uint64_t kermem);
uint64_t get_pt(uint64_t kermem);


page_frame_t *free_page,*table_end;
extern char kernmem, physbase;

void create4KbPages(uint32_t *modulep,void *physbase, void *physfree){
  struct smap_t {
    uint64_t base, length;
    uint32_t type;
  }__attribute__((packed)) *smap;

  //Store all the available memory boundaries in an array free memory boundaries
  uint64_t free_mem_boundaries[40];
  uint32_t i = 0;
  while(modulep[0] != 0x9001) modulep += modulep[1]+2;
  for(smap = (struct smap_t*)(modulep+2); smap < (struct smap_t*)((char*)modulep+modulep[1]+2*4); ++smap) {
    if (smap->type == 1 /* memory */ && smap->length != 0) {
      free_mem_boundaries[i++] = smap->base;  
      kprintf("Free boundary entry %p\n", free_mem_boundaries[i-1]);
      free_mem_boundaries[i++] = smap->base + smap->length;
      kprintf("Free boundary entry%p\n", free_mem_boundaries[i-1]);
    }
  }
  page_frame_t *head = (page_frame_t*)physfree;
  int j=0,pages_till_physfree = 0;
  uint64_t page_count = 0, mem_start = free_mem_boundaries[j++];
  while(j<i){
    if(mem_start + 4096 < free_mem_boundaries[j]){
      page_frame_t *t = head + page_count;
      t->start = (uint64_t*) mem_start;
      t->info = (uint64_t) 0;
      page_count++;
      if(mem_start < (uint64_t)physfree){
        pages_till_physfree++;
      }
      mem_start += 4096;
    }else{
      j++;
      mem_start = free_mem_boundaries[j++];
    }
  }

  int array_page = page_count * 32 / 4096 + 1 + pages_till_physfree; // +1 since we have a division so it will be floored we need to take 1 as a buffer
  kprintf("no of pages used to store array %d\n", array_page);

  for(int i=0;i<array_page;i++){
    page_frame_t *t = head + i;
    t->info=1;
  }
  
  // The link list will start after the page descriptor have finished
  // first page
  page_frame_t *link_start = head + array_page;
  page_frame_t *t = link_start;
  free_page = link_start;
  table_end = free_page;
  t->prev = NULL;
  t->next = (page_frame_t*)(link_start + 1); 
  kprintf("prev -> %x, next -> %x t-> %x  \n", t->prev,t->next, t);
 
  // last element
  t = head + page_count - 1;
  t->prev = (page_frame_t *)(head+page_count-2);
  t->next = NULL;
  
  kprintf("prev -> %x, next -> %x t-> %x  \n", t->prev,t->next, t);
  
  for(int i = 1; i < page_count - 1 - array_page ;i++){
    t = link_start + i;
    t->prev = t-1;
    t->next = t+1;
  }
  /*for(int i= 0; i < 6;i++){
    t = link_start + i;
    kprintf("prev -> %x, next -> %x t-> %x\n", t->prev,t->next, t);
  }
  kprintf("no of page frames created %d\n", page_count);*/
  
  kprintf("physbase -> %x, physfree -> %x, array_end -> %x, array_start -> %x\n", physbase, physfree, (free_page-1)->start, free_page->start);

  uint64_t* page = get_free_page();
  kprintf("get free page %x\n", page);
  free(page);
  page = get_free_page();
  kprintf("get free page 2nd time %x\n", page);
  kernel_init();
}

//returns the first free page from the free_list
uint64_t* get_free_page(){
  page_frame_t *t = free_page;
  t->info = 1;
  free_page = t->next;
  return t->start;
}

//add the page to the head of the linked list
void free(uint64_t* address){
  page_frame_t *t = free_page;
  free_page = t->prev;
  free_page->info = 0;
  free_page->start = address;
}

/*void kernel_init(){
  uint64_t *pml4 = get_free_page();
  uint64_t *pdp = get_free_page();
  uint64_t *pd = get_free_page();
  uint64_t *pt = get_free_page();
  for(int i=0; i < 1024; i++){
    pml4[i] = 0x00000002;
    pdp[i] = 0x00000002;
    pd[i] = 0x00000002;
    pt[i] = 0x00000002;
  } 
}*/

void kernel_init(){
  // kernel memory address = 0xffff ffff 8020 0000
  // 63 - 48 = 0xffff
  // PML4 - 9 bits - 47 - 39 = 1111 1111 1    // binary   -> hex - 0x1ff
  // PDP  - 9 bits - 38 - 30 = 111 1111 10    // binary   -> hex - 0x1fe
  // PD   - 9 bits - 29 - 21 = 00 0000 001    // binary   -> hex - 0x001
  // PT   - 9 bits - 12 - 20 = 0 0000 0000    // binary   -> hex - 0x000
  // 12 bits - 0 - 11 = 0000 0000 0000 // binary   -> hex - 0x000
  page_dir page_info;
  
  page_info.pml4 = get_free_page();
  page_info.pdp = get_free_page();
  page_info.pd = get_free_page();
  page_info.pt = get_free_page();

  kprintf("array_start -> %x, physbase ->%x, kernmem ->%x\n",table_end->start, (uint64_t)&physbase, (uint64_t)&kernmem);
  uint64_t size = (uint64_t)(table_end->start) - (uint64_t)&physbase;

  kprintf("size of the kernel %x, no of pages used %x\n", size, size/4096);
  //Make all the entries in the pages as writeable but set the pages as not used.
  for(int i=0; i < 512; i++){
    page_info.pml4[i] = 0x00000002;
    page_info.pdp[i] = 0x00000002;
    page_info.pd[i] = 0x00000002;
    page_info.pt[i] = 0x00000002;
  }

  uint32_t pml4_init = get_pml4((uint64_t)&kernmem);
  uint32_t pdp_init = get_pdp((uint64_t)&kernmem);
  uint32_t pd_init = get_pd((uint64_t)&kernmem);
  uint32_t pt_init = get_pt((uint64_t)&kernmem);
  
  // set pml4[511] = pdp | 0x1;
  page_info.pml4[pml4_init] = (uint64_t)pdp | 0x3;

  //PDP setting
  // so PDP offset is 510 ~ 0x1fe, set others to zero
  page_info.pdp[pdp_init] = (uint64_t)pd | 0x3;

  //PD setting
  // so PD offset is 1 ~ 0x1, set others to zero
  page_info.pd[pd_init] = (uint64_t)pt | 0x3;

  size /= 4096;
  for(uint64_t i=pt_init;i<size+pt_init;i++){
    if(i!=0 && (i)%(512*512*512)==0){
      uint64_t *page = get_free_page();
      page_info.pml4[pml4_init] = (uint64_t)pdp | 0x3;
    }
    if(i!=0 && (i)%512 == 0){
      uint64_t* page = get_free_page();
      pd[]
    }
  }  

}

void clear_page(uint64_t *page){
  for(int i=0; i < 512; i++){
    page[i] = 0x00000002;
  }
}

uint64_t get_pml4(uint64_t kermem)
{
  uint64_t mask = 0x1ff;//0x0000ff8000000000;
  uint64_t shift = 39;
  return ((kermem >> shift) & mask);
}

uint64_t get_pdp(uint64_t kermem)
{
  uint64_t mask = 0x1ff;//0x0000ff8000000000;
  uint64_t shift = 30;
  return ((kermem >> shift) & mask);
}

uint64_t get_pd(uint64_t kermem)
{
  uint64_t mask = 0x1ff;//0x0000ff8000000000;
  uint64_t shift = 21;
  return ((kermem >> shift) & mask);
}

uint64_t get_pt(uint64_t kermem)
{
  uint64_t mask = 0x1ff;//0x0000ff8000000000;
  uint64_t shift = 12;
  return ((kermem >> shift) & mask);
}