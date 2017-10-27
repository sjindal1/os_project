#ifndef _PAGING_H
#define _PAGING_H

void create4KbPages(uint32_t *modulep,void *physbase, void *physfree);

uint64_t* get_free_page();

typedef struct page_frame_t page_frame_t;

void free(uint64_t* address);

struct page_frame_t{
  uint64_t* start;
  page_frame_t *next,*prev;
  uint64_t info;
}__attribute__((packed));

#endif
