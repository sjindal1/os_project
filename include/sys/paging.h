#ifndef _PAGING_H
#define _PAGING_H

#define HEAP_START_ADD 0xFFFFFFFF90010000
#define ONEMAPADD 0xFFFFFFFD00000000

extern char kernmem, physbase;

void create4KbPages(uint32_t *modulep,void *physbase, void *physfree);

uint64_t* get_free_page();

typedef struct page_frame_t page_frame_t;

void  update_global_pointers();

void free(uint64_t* address);

uint64_t* get_free_pages(uint32_t no_of_pages);

uint64_t* get_free_self__ref_page();

uint64_t* kernel_init();

uint64_t* kmalloc(uint64_t size);

void kfree(uint64_t*);

uint64_t* create_user_page_table(uint64_t va_func,uint64_t pa_func,uint32_t no_of_pages);

typedef struct page_dir page_dir;

void create_pf_pt_entry(uint64_t *p_add, uint64_t v_add);

uint64_t makepagetablecopy(uint64_t);

uint64_t get_va_add(uint64_t p_add);

void invalidate_tlb();

uint64_t get_pml4(uint64_t kermem);
uint64_t get_pdp(uint64_t kermem);
uint64_t get_pd(uint64_t kermem);
uint64_t get_pt(uint64_t kermem);

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

#define _PB 		0x1
#define _WB			0x2
#define _UB			0x4

//#define KERNPAG		((_PB)|(_WB))
#define KERNPAG   ((_PB)|(_WB))
#define USERPAG   ((_PB)|(_WB)|(_UB))

#define GDT_ENTRY_KERNEL_CS		2
#define GDT_ENTRY_KERNEL_DS		3
#define GDT_ENTRY_DEFAULT_USER_CS	14
#define GDT_ENTRY_DEFAULT_USER_DS	15
#define GDT_ENTRY_DEFAULT_USER32_CS	4


#define __KERNEL_CS			(GDT_ENTRY_KERNEL_CS*8)
#define __KERNEL_DS			(GDT_ENTRY_KERNEL_DS*8)
#define __USER_DS			(GDT_ENTRY_DEFAULT_USER_DS*8 + 3)
#define __USER_CS			(GDT_ENTRY_DEFAULT_USER_CS*8 + 3)
#define __USER32_CS			(GDT_ENTRY_DEFAULT_USER32_CS*8 + 3)


#endif
