#ifndef _PAGING_H
#define _PAGING_H

void create4KbPages(uint32_t *modulep,void *physbase, void *physfree);

uint64_t* get_free_page();

typedef struct page_frame_t page_frame_t;

void free(uint64_t* address);

uint64_t* kernel_init();

void clear_page(uint64_t *page);

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
