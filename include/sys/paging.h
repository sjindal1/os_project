#ifndef _PAGING_H
#define _PAGING_H

#define HEAP_START_ADD 0xFFFFFFFF90010000;

void create4KbPages(uint32_t *modulep,void *physbase, void *physfree);

uint64_t* get_free_page();

typedef struct page_frame_t page_frame_t;

void  update_global_pointers();

void free(uint64_t* address);

uint64_t* get_free_pages(uint32_t no_of_pages);

uint64_t* get_free_self__ref_page();

uint64_t* kernel_init();

uint64_t* kmalloc(uint64_t size);

typedef struct page_dir page_dir;

struct page_dir
{
  uint64_t *pml4;
  uint64_t *pdp;
  uint64_t *pd;
  uint64_t *pt;
};

struct page_frame_t{
  uint64_t* start;
  page_frame_t *next,*prev;
  uint64_t info;
}__attribute__((packed));

#endif
