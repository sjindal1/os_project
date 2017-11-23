#include <sys/defs.h>
#include <sys/kprintf.h>
#include <sys/kernel.h>
#include <sys/paging.h>

uint64_t get_pml4(uint64_t kermem);
uint64_t get_pdp(uint64_t kermem);
uint64_t get_pd(uint64_t kermem);
uint64_t get_pt(uint64_t kermem);

uint32_t pml4_off;
uint32_t pdp_off;
uint32_t pd_off;
uint32_t pt_off;

uint64_t free_virtual_address;
page_frame_t *free_page,*table_end;
uint64_t page_count;

page_frame_t *head;
extern char kernmem, physbase;
extern uint64_t *kernel_cr3;

page_dir kernel_page_info;

uint64_t heap_start = HEAP_START_ADD;

void create4KbPages(uint32_t *modulep,void *physbase, void *physfree){
  struct smap_t {
    uint64_t base, length;
    uint32_t type;
  }__attribute__((packed)) *smap;

  //Store all the available memory boundaries in an array free memory boundaries
  uint64_t free_mem_boundaries[40];
  uint32_t no_bound = 0;
  while(modulep[0] != 0x9001) modulep += modulep[1]+2;
  for(smap = (struct smap_t*)(modulep+2); smap < (struct smap_t*)((char*)modulep+modulep[1]+2*4); ++smap) {
    if (smap->type == 1 /* memory */ && smap->length != 0) {
      free_mem_boundaries[no_bound++] = smap->base;  
      //kprintf("Free boundary entry %p\n", free_mem_boundaries[i-1]);
      free_mem_boundaries[no_bound++] = smap->base + smap->length;
      //kprintf("Free boundary entry%p\n", free_mem_boundaries[i-1]);
    }
  }
  head = (page_frame_t*)physfree;
  int j=0,pages_till_physfree = 0;
  page_count = 0;
  uint64_t mem_start = free_mem_boundaries[0];
  while(mem_start < free_mem_boundaries[no_bound-1]){
    page_frame_t *t = head + page_count;
    t->start = (uint64_t*) mem_start;
    t->info = (uint64_t) 0;
    page_count++;
    if(mem_start < (uint64_t)physfree){
      pages_till_physfree++;
    }
    mem_start += 4096;
  }

  //mark all pages till physfree + array size as used.
  int array_page = page_count * sizeof(page_frame_t) / 4096 + 1 + pages_till_physfree; // +1 since we have 
  //a division so it will be floored we need to take 1 as a buffer
  
  kprintf("pages_till_physfree -> %d, page_count -> %d\n",pages_till_physfree,page_count);

  //mark pages till the array end as used
  for(int i=0;i<array_page;i++){
    page_frame_t *t = head + i;
    t->info = 1 | (uint64_t)1 << 32;
  }

  //mark all the holes as 3
  j=1;
  mem_start = free_mem_boundaries[j++];
  while(j<no_bound-1){
    if(mem_start + 4096 < free_mem_boundaries[j]){
      page_frame_t *t = head + (mem_start/4096);
      t->info = (uint64_t) 3;
      mem_start += 4096;
    }else{
      j++;
      mem_start = free_mem_boundaries[j++];
    }
  }
  
  // The link list will start after the page descriptor have finished
  // first page
  page_frame_t *link_start = head + array_page;
  page_frame_t *t = link_start;
  free_page = link_start;
  table_end = free_page;
  t->prev = NULL;
  t->next = (page_frame_t*)(link_start + 1); 
  //kprintf("prev -> %x, next -> %x t-> %x  \n", t->prev,t->next, t);
 
  // last element
  t = head + page_count - 1;
  t->prev = (page_frame_t *)(head+page_count-2);
  t->next = NULL;
  
  //kprintf("prev -> %x, next -> %x t-> %x  \n", t->prev,t->next, t);
  
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
  
  //kprintf("physbase -> %x, physfree -> %x, array_end -> %x, array_start -> %x\n", physbase, physfree, (free_page-1)->start, free_page->start);

  //uint64_t* page = get_free_page();
  //kprintf("get free page %x\n", page);
  //free(page);
  //page = get_free_page();
  //kprintf("get free page 2nd time %x\n", page);

  //kprintf("end of create4KbPages function");
  return;
}

uint64_t* get_free_self_ref_user_page(){
  page_frame_t *t = free_page;
  t->info = 1 | (uint64_t)1 << 32;
  free_page = t->next;
  create_page_table_entry(t->start, 1, 0xFFFFFFFF90001000); //Take care of infinite loop while changing address
  uint64_t *temp = (uint64_t *)0xFFFFFFFF90001000;
  temp[511] = (uint64_t)t->start | KERNPAG;
  return t->start;
}

void create_page_table_entry(uint64_t *physical_add, uint64_t no_of_pages , uint64_t virtual_add){

  uint32_t pd_off_new = get_pd(virtual_add);
  uint32_t pt_off_new = get_pt(virtual_add);
  

  if((((uint64_t)kernel_page_info.pd[pd_off_new])>>12) == 0){
    uint64_t *page = get_free_self_ref_user_page();
    kernel_page_info.pd[pd_off_new] = (uint64_t)page | KERNPAG;
  }

  uint64_t *pt_address = (uint64_t *)((virtual_add & 0xFFFFFFFFFFE00000) | (uint64_t)0x1ff<<12 ) ;

  uint64_t curkermem = (uint64_t) physical_add;
  for(uint64_t j=0;j<no_of_pages;j++){
    if(pt_off_new < 512)
    {
      /*if(j < 10)
        kprintf("j = %d, pt_off = %d, kmst = %x, kmend = %x\n", j, pt_off, curkermem, curkermem + 4096);*/
      pt_address[pt_off_new++] = (curkermem|KERNPAG);
      curkermem += 4096;
    }
    else // pt table is full
    {
      uint64_t *page = get_free_self_ref_user_page();
      pt_off_new = 0;
      kernel_page_info.pd[++pd_off_new] = (uint64_t)page | KERNPAG;
      kprintf("Kernel PT table is full, creating new\n");
      pt_address[pt_off_new++] = (curkermem|KERNPAG);
      curkermem += 4096;
    } 
  }
}

void kfree(uint64_t *v_add){
  uint64_t pt_off = get_pt((uint64_t)v_add);
  int64_t sv_add = (int64_t)v_add;
  sv_add = sv_add >> 9;
  uint64_t *pt_va = (uint64_t *)(sv_add & 0xFFFFFFFFFFFFF000);
  uint64_t p_add = pt_va[pt_off];
  //kprintf("v_addr - %x, sv_add - %x, pt-off - %x, p_add - %x, p_add_prev - %x\n",v_add, sv_add, pt_off, p_add, pt_va[pt_off-1]);
  free((uint64_t *)p_add);
  pt_va[pt_off] = 0x2;
}

uint64_t* kmalloc(uint64_t size, uint64_t *pa_add){
  uint32_t no_of_pages = (size + 4095)/4096;
  uint64_t *start_add = get_free_pages(no_of_pages);
  if(pa_add != NULL){
    *pa_add = *start_add;
  }
  create_page_table_entry(start_add,no_of_pages,heap_start);
  heap_start = heap_start + no_of_pages*4096;
  return (uint64_t *)(heap_start - no_of_pages*4096);
}

//returns the first free page from the free_list
uint64_t* get_free_page(){
  page_frame_t *t = free_page;
  t->info = 1 | (uint64_t)1 << 32;
  free_page = t->next;
  return t->start;
}

//returns the first free page from the free_list
uint64_t* get_free_self__ref_page(){
  page_frame_t *t = free_page;
  t->info = 1 | (uint64_t)1 << 32;
  free_page = t->next;
  t->start[511] = (uint64_t)t->start | KERNPAG;
  return t->start;
}

//returns continous pages
uint64_t* get_free_pages(uint32_t no_of_pages){
  if(no_of_pages == 1){
    return get_free_page();
  }
  page_frame_t *start_add = free_page;
  page_frame_t *t = start_add;
  for(int i=0;i<no_of_pages;){
    if(t->next != NULL && (uint64_t)t->next->start == (uint64_t)t->start + 4096){
      i++;
      t = t->next;
    }else if(t->next != NULL){
      i=0;
      start_add = t->next;
      t = t->next;
    }else{
      return NULL;
    }
  }
  for(int i=0;i<no_of_pages;i++){
    (start_add+i)->info = (uint64_t)0x1 | (uint64_t)no_of_pages<<32;
  }
  if(start_add == free_page){
    free_page = t;
    free_page->prev = NULL;
  }else{
    start_add->prev->next = t;
    start_add-> prev = NULL;
    t->prev->next = NULL;
    t->prev = start_add->prev;
  }
  return start_add->start;
}
//TODO it expects physical address but we will always pass virtual address the implementation needs to be changed
//we need something like kfree
//add the page to the required index in the array and set the links correctly
void free(uint64_t* address){
  uint64_t page_index = (uint64_t)address/4096;
  page_frame_t *t_start = head + page_index;
  int no_of_pages = t_start->info >> 32;
  page_frame_t *t_end = head + (page_index + no_of_pages -1);
  
  //linking all freed pages together
  page_frame_t *temp = t_start+1;
  for(int i = 1; i<no_of_pages;i++){
    temp->info = 0;
    temp->next = temp + 1;
    temp->prev = temp - 1;
    temp++;
  }

  t_start->info = 0;
  t_start->prev = NULL;
  t_start->next = t_start + 1;
  t_end->info = 0;
  t_end->next = NULL;

  if(address < free_page->start){
    t_end->next = free_page;
    free_page->prev = t_end;
    free_page = t_start;
  }else{
    for(int i=page_index-1;i>=0;i--){
      page_frame_t *des = head + i;
      if(des->info == 0){
        t_end->next = des->next;
        des->next->prev = t_end;
        des->next = t_start;
        t_start->prev = des;
        break;
      }
    }
  }
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

uint64_t* kernel_init(){
  // kernel memory address = 0xffff ffff 8020 0000
  // 63 - 48 = 0xffff
  // PML4 - 9 bits - 47 - 39 = 1111 1111 1    // binary   -> hex - 0x1ff
  // PDP  - 9 bits - 38 - 30 = 111 1111 10    // binary   -> hex - 0x1fe
  // PD   - 9 bits - 29 - 21 = 00 0000 001    // binary   -> hex - 0x001
  // PT   - 9 bits - 12 - 20 = 0 0000 0000    // binary   -> hex - 0x000
  // 12 bits - 0 - 11 = 0000 0000 0000 // binary   -> hex - 0x000

  //size of the kernel currently (array end - physbase)
  uint64_t size = (uint64_t)(table_end->start) - (uint64_t)&physbase;

  //alot pages for page tables and include these in the kernel for future access
  kernel_page_info.pml4 = get_free_self__ref_page();
  size+=4096;
  kernel_page_info.pdp = kernel_page_info.pml4;
  kernel_page_info.pd = get_free_page();
  size+=4096;
  kernel_page_info.pt = get_free_page();
  size+=4096;
  
  //kprintf("array_start -> %x, physbase ->%x, kernmem ->%x\n",table_end->start, (uint64_t)&physbase, (uint64_t)&kernmem);

  //kprintf("size of the kernel %x, no of pages used %x\n", size, size/4096);
  //Make all the entries in the pages as writeable but set the pages as not used.
  for(int i=0; i < 511; i++){
    kernel_page_info.pml4[i] = 0x00000002;
    //kernel_page_info.pdp[i] = 0x00000002;
  }
  for(int i=0; i < 511; i++){
    kernel_page_info.pd[i] = 0x00000002;
    kernel_page_info.pt[i] = 0x00000002;
  }
  pml4_off = get_pml4((uint64_t)&kernmem);
  pdp_off = get_pdp((uint64_t)&kernmem);
  pd_off = get_pd((uint64_t)&kernmem);
  pt_off = get_pt((uint64_t)&kernmem);
  
  // set pml4[511] = pdp | 0x1;
  /*if(pml4_off == 511)
    kernel_page_info.pdp = kernel_page_info.pml4;
  else
    kernel_page_info.pml4[pml4_off] = (uint64_t)kernel_page_info.pdp | KERNPAG;
*/
  //PDP setting
  // so PDP offset is 510 ~ 0x1fe, set others to zero
  /*if(pdp_off == 511)
    kernel_page_info.pd = kernel_page_info.pdp;
  else*/
  kernel_page_info.pdp[pdp_off] = (uint64_t)kernel_page_info.pd | KERNPAG;

  //PD setting
  // so PD offset is 1 ~ 0x1, set others to zero
  /*if(pd_off == 511)
    kernel_page_info.pt = kernel_page_info.pd;
  else*/
  kernel_page_info.pd[pd_off] = (uint64_t)kernel_page_info.pt | KERNPAG;

  size /= 4096;
  uint64_t curkermem = (uint64_t) &physbase;
  for(uint64_t j=0,i=pt_off;j<size;j++,i++){
    if(pt_off < 512)
    {
      /*if(j < 10)
        kprintf("j = %d, pt_off = %d, kmst = %x, kmend = %x\n", j, pt_off, curkermem, curkermem + 4096);*/
      kernel_page_info.pt[pt_off++] = (curkermem|KERNPAG);
      curkermem += 4096;
    }
    else // pt table is full
    {
      uint64_t *page = get_free_self__ref_page();
      size++;
      pt_off = 0;

      kprintf("Kernel PT table is full, creating new\n");
      kernel_page_info.pt = page;
      kernel_page_info.pt[pt_off++] = (curkermem|KERNPAG);
      curkermem += 4096;

      //add a pd entry
      if(++pd_off < 512)
      {
        kernel_page_info.pd[pd_off] = ((uint64_t)kernel_page_info.pt|KERNPAG);
      }
      else  // pd table is full
      {
        kprintf("Kernel PD table is full, creating new \n");
        pd_off = 0;
        page = get_free_self__ref_page();
        size++;

        kernel_page_info.pd = page;
        kernel_page_info.pd[pd_off++] =  ((uint64_t)kernel_page_info.pt|KERNPAG);

        //add a new pdp entry
        if(++pdp_off < 512)
        {
          kernel_page_info.pdp[pdp_off] = ((uint64_t)kernel_page_info.pd|KERNPAG);
        }
        else  // pdp table is full
        {
          kprintf("Kernel PDP table is full, creating new\n");
          pdp_off = 0;
          page = get_free_self__ref_page();
          size++;
          
          kernel_page_info.pdp = page;
          kernel_page_info.pdp[pdp_off++] = ((uint64_t) kernel_page_info.pd|KERNPAG);

          //add a new pml4 entry
          kernel_page_info.pml4[pml4_off++] = ((uint64_t) kernel_page_info.pdp|KERNPAG);
        }

      }
    }
    
  }
  
  //move video buffer 0xb8000 to 0xFFFFFFFF90000000
  uint64_t v_mem_address = 0xFFFFFFFF90000000;
  uint64_t v_mem_pd = get_pd(v_mem_address);
  uint64_t v_mem_pt = get_pt(v_mem_address);
  //kprintf("v_mem_pdp-> %x, v_mem_pd -> %x, v_mem_pt -> %x\n", v_mem_pdp, v_mem_pd, v_mem_pt);

  uint64_t *page = get_free_self__ref_page();
  //kernel_page_info.pt = page;
  kernel_page_info.pd[v_mem_pd] = (uint64_t)page | KERNPAG;

  page[v_mem_pt] = 0xb8000 | KERNPAG;


  return kernel_page_info.pml4;    // to be set to CR3 :)
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

//updates the global pointers according to the new kernel address i.e., 0xffffffff80200000
void  update_global_pointers(){
  //update head according to new kernel address
  uint64_t va_temp = ((uint64_t)&kernmem - (uint64_t)&physbase + (uint64_t)head);
  head = (page_frame_t *)va_temp;

  //update free pointer
  va_temp = ((uint64_t)&kernmem - (uint64_t)&physbase + (uint64_t)free_page);
  free_page = (page_frame_t *)va_temp;

  //update page table pointers to be accessed for kmalloc
  kernel_page_info.pml4 = (uint64_t *)((uint64_t)&kernmem - (uint64_t)&physbase + (uint64_t)kernel_page_info.pml4);
  kernel_page_info.pdp = (uint64_t *)((uint64_t)&kernmem - (uint64_t)&physbase + (uint64_t)kernel_page_info.pdp);
  kernel_page_info.pd = (uint64_t *)((uint64_t)&kernmem - (uint64_t)&physbase + (uint64_t)kernel_page_info.pd);
  kernel_page_info.pt = (uint64_t *)((uint64_t)&kernmem - (uint64_t)&physbase + (uint64_t)kernel_page_info.pt);


  //update the next and prev pointers of the free list
  page_frame_t *va_free_page = free_page;
  //update the next pointer of the first free element prev = NULL
  va_free_page->next = (page_frame_t *)((uint64_t)&kernmem - (uint64_t)&physbase + (uint64_t)(va_free_page->next));
  va_free_page++;
  //update next and free of all the elements in between
  while(va_free_page->next != NULL){
    va_free_page->next = (page_frame_t *)((uint64_t)&kernmem - (uint64_t)&physbase + (uint64_t)(va_free_page->next));
    va_free_page->prev = (page_frame_t *)((uint64_t)&kernmem - (uint64_t)&physbase + (uint64_t)(va_free_page->prev));
    va_free_page++;
  }
  //update the prev pointer of the last free element next = NULL
  va_free_page->prev = (page_frame_t *)((uint64_t)&kernmem - (uint64_t)&physbase + (uint64_t)(va_free_page->prev));

  //reclaim pages between 0 and physbase
  page_frame_t *reclaimed_pages;
  for(int i=0; i<(uint64_t)&physbase/4096; i++){
    reclaimed_pages = head + i;
    if((reclaimed_pages->info & (uint64_t)0x3) == 1){
      reclaimed_pages->info = 0;
      va_free_page->next = (page_frame_t *)((uint64_t)&kernmem - (uint64_t)&physbase + (uint64_t)reclaimed_pages);
      reclaimed_pages->prev = (page_frame_t *)((uint64_t)&kernmem - (uint64_t)&physbase + (uint64_t)va_free_page);
      va_free_page = reclaimed_pages;
    }
  }
  va_free_page->next = NULL;
}


uint64_t* create_user_page_table(uint64_t va_func,uint64_t pa_func,uint32_t no_of_pages){

  page_dir user_page_info;

  //alot pages for page tables and include these in the kernel for future access
  user_page_info.pml4 = get_free_self_ref_user_page();
  user_page_info.pdp = get_free_self_ref_user_page();
  user_page_info.pd = get_free_self_ref_user_page();
  user_page_info.pt = get_free_self_ref_user_page();

  create_page_table_entry(user_page_info.pml4, 1, 0xFFFFFFFF90002000);
  create_page_table_entry(user_page_info.pdp, 1, 0xFFFFFFFF90003000);
  create_page_table_entry(user_page_info.pd, 1, 0xFFFFFFFF90004000);
  create_page_table_entry(user_page_info.pt, 1, 0xFFFFFFFF90005000);
  
  uint64_t *va_pml4 = (uint64_t *)(0xFFFFFFFF90002000);
  uint64_t *va_pdp = (uint64_t *)(0xFFFFFFFF90003000);
  uint64_t *va_pd = (uint64_t *)(0xFFFFFFFF90004000);
  uint64_t *va_pt = (uint64_t *)(0xFFFFFFFF90005000);

  clear_page(va_pml4);
  clear_page(va_pdp);
  clear_page(va_pd);
  clear_page(va_pt);

  uint64_t pml4_off = get_pml4(va_func);
  va_pml4[pml4_off] = (uint64_t)user_page_info.pdp | KERNPAG;
  va_pml4[511] = (uint64_t)kernel_cr3 | KERNPAG; // mapping the kernel in higher address.

  uint64_t pdp_off = get_pdp(va_func);
  va_pdp[pdp_off] = (uint64_t)user_page_info.pd | KERNPAG;

  uint64_t pd_off = get_pd(va_func);
  va_pd[pd_off] = (uint64_t)user_page_info.pt | KERNPAG;

  uint64_t pt_off = get_pt(va_func);

  uint64_t curkermem = pa_func;
  for(uint64_t j=0;j<no_of_pages;j++){
    if(pt_off < 512)
    {
      /*if(j < 10)
        kprintf("j = %d, pt_off = %d, kmst = %x, kmend = %x\n", j, pt_off, curkermem, curkermem + 4096);*/
      va_pt[pt_off++] = (curkermem|KERNPAG);
      curkermem += 4096;
    }
    else // pt table is full
    {
      uint64_t *page = get_free_self_ref_user_page();
      create_page_table_entry(page, 1, 0xFFFFFFFF90005000);
      pt_off = 0;

      kprintf("Kernel PT table is full, creating new\n");
      user_page_info.pt = page;
      va_pt[pt_off++] = (curkermem|KERNPAG);
      curkermem += 4096;

      //add a pd entry
      if(++pd_off < 512)
      {
        va_pd[pd_off] = ((uint64_t)user_page_info.pt|KERNPAG);
      }
      else  // pd table is full
      {
        kprintf("Kernel PD table is full, creating new \n");
        pd_off = 0;
        page = get_free_self_ref_user_page();
        create_page_table_entry(page, 1, 0xFFFFFFFF90004000);

        user_page_info.pd = page;
        va_pd[pd_off++] =  ((uint64_t)user_page_info.pt|KERNPAG);

        //add a new pdp entry
        if(++pdp_off < 512)
        {
          va_pdp[pdp_off] = ((uint64_t)user_page_info.pd|KERNPAG);
        }
        else  // pdp table is full
        {
          kprintf("Kernel PDP table is full, creating new\n");
          pdp_off = 0;
          page = get_free_self_ref_user_page();
          create_page_table_entry(page, 1, 0xFFFFFFFF90003000);
          
          user_page_info.pdp = page;
          va_pdp[pdp_off++] = ((uint64_t) user_page_info.pd|KERNPAG);

          //add a new pml4 entry
          va_pml4[pml4_off++] = ((uint64_t) user_page_info.pdp|KERNPAG);
        }

      }
    }
  }
  return user_page_info.pml4;
}

void create_pf_pt_entry(uint64_t *p_add, uint32_t size, uint64_t v_add){
  uint32_t pf_pml4_off = get_pml4(v_add);
  uint32_t pf_pdp_off = get_pdp(v_add);
  uint32_t pf_pd_off = get_pd(v_add);
  uint32_t pf_pt_off = get_pt(v_add);

  uint64_t *temp_va = (uint64_t *)0xFFFFFFFF90002000;
  
  uint64_t *va_pml4 = (uint64_t *)0xFFFFFFFFFFFFF000;
  if((va_pml4[pf_pml4_off] & 0x1) == 0){ //check for present bit 
    uint64_t pf_pdp = (uint64_t)get_free_page();
    create_page_table_entry((uint64_t *)pf_pdp, 1, (uint64_t) temp_va);
    clear_page(temp_va);
    va_pml4[pf_pml4_off] = pf_pdp | USERPAG;
  }

  uint64_t *va_pdp = (uint64_t *)(0xFFFFFFFFFFE00000 | (uint64_t)pf_pml4_off <<12);
  if((va_pdp[pf_pdp_off] & 0x1) == 0){ 
    uint64_t pf_pd = (uint64_t)get_free_page();
    create_page_table_entry((uint64_t *)pf_pd, 1, (uint64_t) temp_va);
    clear_page(temp_va);
    va_pdp[pf_pdp_off] = pf_pd | USERPAG;
  }

  uint64_t *va_pd = (uint64_t *)(0xFFFFFFFFC0000000 | (uint64_t)pf_pml4_off <<21 | (uint64_t)pf_pdp_off <<12);
  if((va_pd[pf_pd_off] & 0x1) == 0){ 
    uint64_t pf_pt = (uint64_t)get_free_page();
    create_page_table_entry((uint64_t *)pf_pt, 1, (uint64_t) temp_va);
    clear_page(temp_va);
    va_pd[pf_pd_off] = pf_pt | USERPAG;
  }


  uint64_t *va_pt = (uint64_t *)(0xFFFFFF8000000000 | (uint64_t)pf_pml4_off << 30 | (uint64_t)pf_pdp_off << 21 | (uint64_t)pf_pd_off << 12);
  /*if((va_pt[pf_pt_off] & 0x1) == 0){ 
    va_pt[pf_pt_off] = (uint64_t)p_add | USERPAG;
  }*/
  for(uint64_t j=0;j<size;j++){
    va_pt[pf_pt_off] = (uint64_t)p_add | USERPAG;
    pf_pt_off++;
    p_add+=4096;
  }

}


// returns the new cr3 value
uint64_t makepagetablecopy()
{
  uint64_t new_cr3;
  uint16_t i, j, k, m;
  uint64_t *newpagepml4 = (uint64_t *)0xFFFFFFFF90002000;
  uint64_t *newpagepdp = (uint64_t *)0xFFFFFFFF90003000;
  uint64_t *newpagepd = (uint64_t *)0xFFFFFFFF90004000;
  uint64_t *newpagept = (uint64_t *)0xFFFFFFFF90005000;
  uint64_t *newstack = (uint64_t *)0xFFFFFFFF90006000;

  uint64_t user_stack_va = pcb_struct[current_process].vma_stack.startva;
  uint64_t user_stack_size = pcb_struct[current_process].vma_stack.startva/4096;

  uint64_t user_stack_pml4 = get_pml4(user_stack_va);
  uint64_t user_stack_pdp = get_pdp(user_stack_va);
  uint64_t user_stack_pd = get_pd(user_stack_va);
  uint64_t user_stack_pt = get_pt(user_stack_va);

  // get the page tables
  // pml4
  new_cr3 = (uint64_t) get_free_self_ref_user_page();
  // map the page to 0xFFFFFFFF90002000 temporarily to copy
  create_page_table_entry((uint64_t*)new_cr3, 1, (uint64_t) newpagepml4);
  
  uint64_t *va_cur_pml4 = (uint64_t *)0xFFFFFFFFFFFFF000;
  
  for(i = 0; i < 510; i++)    // dont overwrite the last entry
  {
    newpagepml4[i] = va_cur_pml4[i];
    if(newpagepml4[i] != 0x2)   // writable is set
    {
      uint64_t *va_cur_pdp = (uint64_t *)(0xFFFFFFFFFFE00000 | (uint64_t)i <<12);

      uint64_t *new_pdp = get_free_page();
      create_page_table_entry(new_pdp, 1, (uint64_t) newpagepdp);

      newpagepml4[i] = (uint64_t) new_pdp | 0x7;    // set the new entry and user accessible

      for(j = 0; j < 510; j++)
      {
        newpagepdp[j] = va_cur_pdp[j];
        if(newpagepdp[j] != 0x2)
        {
          uint64_t *va_cur_pd = (uint64_t *)(0xFFFFFFFFC0000000 | (uint64_t)i <<21 | (uint64_t)j <<12);

          uint64_t *new_pd = get_free_page();
          create_page_table_entry(new_pd, 1, (uint64_t) newpagepd);

          newpagepdp[j] = (uint64_t) new_pd | 0x7;    // set the new entry and user accessible

          for(k = 0; k < 512; k++)
          {
            newpagepd[k] = va_cur_pd[k];
            if(newpagepd[k] != 0x2)
            {
              uint64_t *va_cur_pt = (uint64_t*) (0xFFFFFF8000000000 | (uint64_t)i << 30 | (uint64_t)j << 21 | (uint64_t)k << 12);
              
              uint64_t *new_pt = get_free_page();
              create_page_table_entry(new_pt, 1, (uint64_t) newpagept);

              newpagepd[k] = (uint64_t) new_pt | 0x7;

              for(m = 0; m < 512; m++)
              {
                uint64_t n = 0;
                //if it is user stack we need to copy the data and create new pages so that it does not fault
                if(i == user_stack_pml4 && j == user_stack_pdp && k == user_stack_pd && m == user_stack_pt){
                  for(n = m ; n < m + user_stack_size; n++){
                    uint64_t *pt_stack = get_free_page();
                    uint64_t *va_stack = (uint64_t *)((uint64_t)i<<39 | (uint64_t)j<<30 | (uint64_t)k<<21 | (uint64_t)m<<12);
                    create_page_table_entry(pt_stack, 1, (uint64_t) newstack);
                    for(int p = 0; p<512;p++){
                      newstack[p] = va_stack[p];
                    }
                    newpagept[n] = (uint64_t)pt_stack | 0x7;
                  }
                  m = n;
                }else{
                  // setting 0X800 for COW
                  // setting 0x5 for USER | READONLY | PRESENT
                  va_cur_pt = (uint64_t*)((( (uint64_t) va_cur_pt & (uint64_t)0xFFFFFFFFFFFFF000) | 0x5) | 0x800); 
                  newpagept[m] = va_cur_pt[m];
                }
              }
            }
          }
        }
      }
    }
  }

  newpagepml4[510] = va_cur_pml4[510]; //mapping kernel to higher addresses

  return new_cr3;
}
