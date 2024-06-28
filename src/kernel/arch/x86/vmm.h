#ifndef VMM_H
#define VMM_H

#include "../../runtime/types.h"
#include "../../runtime/list.h"
#include "../../runtime/alloc.h"
#include "x86.h"


extern "C" {

struct page {
		char *v_addr;
		char *p_addr;
		list_head list;
};

struct page_directory {
		page *base;
		list_head pt;
};

struct vm_area {
		char *vm_start;	
		char *vm_end;	/* exclude */
		list_head list;
};

typedef page_directory proc_memory;

	/* Points to the top of the kernel heap */
	extern char *kern_heap;

	/* Points to the beginning of the list of free kernel pages */
	extern list_head kern_free_vm;


	extern u32 *pd0;
	extern u8 mem_bitmap[];

	extern u32 kmalloc_used;



	/*  Marks a page as used/free in the bitmap. */
	#define set_page_frame_used(page)	mem_bitmap[((u32) page)/8] |= (1 << (((u32) page)%8))
	#define release_page_frame(p_addr)	mem_bitmap[((u32) p_addr/PAGESIZE)/8] &= ~(1 << (((u32) p_addr/PAGESIZE)%8))

	/* Selects a free page from the bitmap. */
	char *get_page_frame(void);

	/* Allocates a free page from the bitmap and associates it with a free virtual page from the heap. */
	struct page *get_page_from_heap(void);
	int release_page_from_heap(char *);

	/* Initializes the memory management data structures. */
	void Memory_init(u32 high_mem);

	/* Creates a page directory for a process. */
	struct page_directory *pd_create(void);
	int pd_destroy(struct page_directory *);
	struct page_directory *pd_copy(struct page_directory * pdfather);
	
	
	/* Adds an entry to the kernel address space */
	int pd0_add_page(char *, char *, int);

	/* Adds/removes an entry in the current page directory */
	int pd_add_page(char *, char *, int, struct page_directory *);
	int pd_remove_page(char *);

	/* Returns the physical address associated with a virtual address */
	char *get_p_addr(char *);

	
	#define KMALLOC_MINSIZE		16

	struct kmalloc_header {
		unsigned long size:31;	/* total record size */
		unsigned long used:1;
	} __attribute__ ((packed));

}

class Vmm
{
	public:
		void			init(u32 high);
		proc_memory*	createPM();					/* Create page directory for a process */
		void			switchPM(proc_memory* ad);	/* Switch page directory for a process */
		void			map(proc_memory* ad,u32 phy,u32 adr);	/* map a physical page memory in virtual space */
		
		void			kmap(u32 phy,u32 virt);
		
};

extern Vmm vmm;

#endif
