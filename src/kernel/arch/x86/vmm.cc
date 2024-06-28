#include "../../core/os.h"

/*
     Implements a virtual memory manager (VMM) for a system with paging enabled. Here's a breakdown of the key components:

	Heap: The kernel heap is allocated during the initial memory setup phase (Memory_init function) and serves as on-demand memory for the kernel itself during runtime.
	When the system boots up, the Memory_init function is responsible for setting up the memory management structures. Within Memory_init, a portion of the reserved kernel
	memory is designated as the kernel heap. The kern_heap variable is then initialized to point to the starting address of this allocated region. A specific amount of physical
	memory in RAM will be dedicated for the kernel's exclusive use including reserving pages for kernel code, data structures and the kernel heap itself.

	Bitmap: The mem_bitmap is an array of bytes representing the physical memory allocation state. A set bit indicates a page is used, while a clear bit indicates it's free.

	Page Directory and Page Tables: The code uses a hierarchical page table structure. The pd0 variable points to the kernel page directory, which contains entries for page tables.
	Each page table entry points to a physical page frame.
*/
extern "C" {
	char *kern_heap;
	list_head kern_free_vm;
	u32 *pd0 = (u32 *) KERN_PDIR;			/* kernel page directory */
	char *pg0 = (char *) 0;					/* kernel page 0 (4MB) */
	char *pg1 = (char *) KERN_PG_1;			/* kernel page 1 (4MB) 0x400000*/
	char *pg1_end = (char *) KERN_PG_1_LIM;	/* limite de la page 1 0x800000*/
	u8 mem_bitmap[RAM_MAXPAGE / 8];			/* bitmap allocation de pages (1 Go) */

	u32 kmalloc_used = 0;

	/*
	 * Iterates through the bitmap to find a free page and marks it as used before returning its physical address.
	 */
	char* get_page_frame(void)
	{
		int byte, bit;
		int page = -1;

		for (byte = 0; byte < RAM_MAXPAGE / 8; byte++)
			if (mem_bitmap[byte] != 0xFF)
				for (bit = 0; bit < 8; bit++)
					if (!(mem_bitmap[byte] & (1 << bit))) {
						page = 8 * byte + bit;
						set_page_frame_used(page);
						return (char *) (page * PAGESIZE);
					}
		return (char *) -1;
	}


	/*
	 * Searches for a free virtual page in the kernel's virtual address space. The function then requests a free physical page to associate with it.
	 * NOTE: these pages are in the kernel's address space, which is updated.
	 */
	page* get_page_from_heap(void)
	{
		page *pg;
		vm_area *area;
		char *v_addr, *p_addr;

		/* Allocates a free physical page */
		p_addr = get_page_frame();
		if ((int)(p_addr) < 0) {
			io.print ("PANIC: get_page_from_heap(): no page frame available. System halted !\n");
		}

		/* Checks if there's a free virtual page */
		if (list_empty(&kern_free_vm)) {
			io.print ("PANIC: get_page_from_heap(): not memory left in page heap. System halted !\n");
		}

		/* Takes the first available free virtual page */
		area = list_first_entry(&kern_free_vm, vm_area, list);
		v_addr = area->vm_start;

		/* Updates the list of free pages in the kernel's virtual address space */
		area->vm_start += PAGESIZE;
		if (area->vm_start == area->vm_end) {
			list_del(&area->list);
			kfree(area);
		}

		/* Updates the kernel's address space */
		pd0_add_page(v_addr, p_addr, 0);

		/* Returns the page */
		pg = (page*) kmalloc(sizeof(page));
		pg->v_addr = v_addr;
		pg->p_addr = p_addr;
		pg->list.next = 0;
		pg->list.prev = 0;

		return pg;
	}

	int release_page_from_heap(char *v_addr)
	{
		struct vm_area *next_area, *prev_area, *new_area;
		char *p_addr;

		/* Find the page frame associated with v_addr and free it */
		p_addr = get_p_addr(v_addr);
		if (p_addr) {
			release_page_frame(p_addr);
		}
		else {
			io.print("WARNING: release_page_from_heap(): no page frame associated with v_addr %x\n", v_addr);
			return 1;
		}

		/* Update the page directory */
		pd_remove_page(v_addr);

		/* Update the list of free virtual addresses */
		list_for_each_entry(next_area, &kern_free_vm, list) {
			if (next_area->vm_start > v_addr)
				break;
		}

		prev_area = list_entry(next_area->list.prev, struct vm_area, list);

		if (prev_area->vm_end == v_addr) {
			prev_area->vm_end += PAGESIZE;
			if (prev_area->vm_end == next_area->vm_start) {
				prev_area->vm_end = next_area->vm_end;
				list_del(&next_area->list);
				kfree(next_area);
			}
		}
		else if (next_area->vm_start == v_addr + PAGESIZE) {
			next_area->vm_start = v_addr;
		}
		else if (next_area->vm_start > v_addr + PAGESIZE) {
			new_area = (struct vm_area*) kmalloc(sizeof(struct vm_area));
			new_area->vm_start = v_addr;
			new_area->vm_end = v_addr + PAGESIZE;
			list_add(&new_area->list, &prev_area->list);
		}
		else {
			io.print ("\nPANIC: release_page_from_heap(): corrupted linked list. System halted !\n");
			asm("hlt");
		}

		return 0;
	}




	/*
	 * Initialize the memory bitmap and create the kernel's page directory.
	 * Uses an identity mapping such that vaddr = paddr for the first 4MB.
	 */
	void Memory_init(u32 high_mem)
	{
		int pg, pg_limit;
		unsigned long i;
		struct vm_area *p;
		struct vm_area *pm;

		/* Last page number */
		pg_limit = (high_mem * 1024) / PAGESIZE;

		/*  Initialize the physical page bitmap */
		for (pg = 0; pg < pg_limit / 8; pg++)
			mem_bitmap[pg] = 0; // Mark lower memory as free

		for (pg = pg_limit / 8; pg < RAM_MAXPAGE / 8; pg++)
			mem_bitmap[pg] = 0xFF; // Mark upper memory as used

		/* Pages reserved for the kernel */
		for (pg = PAGE(0x0); pg < (int)(PAGE((u32) pg1_end)); pg++) {
			set_page_frame_used(pg);  // Mark kernel memory as used
		}

		/* Initialize the page directory */
		pd0[0] = ((u32) pg0 | (PG_PRESENT | PG_WRITE | PG_4MB));
		pd0[1] = ((u32) pg1 | (PG_PRESENT | PG_WRITE | PG_4MB));
		for (i = 2; i < 1023; i++)
			pd0[i] =
			    ((u32) pg1 + PAGESIZE * i) | (PG_PRESENT | PG_WRITE);

		// Page table mirroring magic trick!
		pd0[1023] = ((u32) pd0 | (PG_PRESENT | PG_WRITE));


		/* Enable paging mode */
		asm("	mov %0, %%eax \n \
			mov %%eax, %%cr3 \n \
			mov %%cr4, %%eax \n \
			or %2, %%eax \n \
			mov %%eax, %%cr4 \n \
			mov %%cr0, %%eax \n \
			or %1, %%eax \n \
			mov %%eax, %%cr0"::"m"(pd0), "i"(PAGING_FLAG), "i"(PSE_FLAG));

		/* Initialize the kernel heap used by kmalloc */
		kern_heap = (char *) KERN_HEAP;
		ksbrk(1);

		/* Initialize the list of free virtual addresses */
		p = (struct vm_area*) kmalloc(sizeof(struct vm_area));
		p->vm_start = (char*) KERN_PG_HEAP;
		p->vm_end = (char*) KERN_PG_HEAP_LIM;
		INIT_LIST_HEAD(&kern_free_vm);
		list_add(&p->list, &kern_free_vm);

		arch.initProc();

		return;
	}

	/*
	 * Creates and initializes a page directory for a process.
	 */
	struct page_directory *pd_create(void)
	{
		struct page_directory *pd;
		u32 *pdir;
		int i;

		/* Allocate and initialize a page for the Page Directory */
		pd = (struct page_directory *) kmalloc(sizeof(struct page_directory));
		pd->base = get_page_from_heap();

		/*
		 * Kernel space. Virtual addresses < USER_OFFSET are addressed by the
		 * kernel page table (pd0[]).
		 */
		pdir = (u32 *) pd->base->v_addr;
		for (i = 0; i < 256; i++)
			pdir[i] = pd0[i];

		/* User space */
		for (i = 256; i < 1023; i++)
			pdir[i] = 0;

		/* Page table mirroring magic trick !... */
		pdir[1023] = ((u32) pd->base->p_addr | (PG_PRESENT | PG_WRITE));


		/* Initialize the list of user space page tables */
		INIT_LIST_HEAD(&pd->pt);

		return pd;
	}

	void page_copy_in_pd(process_st* current,u32 virtadr){
			struct page *pg;
			pg = (struct page *) kmalloc(sizeof(struct page));
			pg->p_addr = get_page_frame();
			/* TODO: copy the contents of the other page  */
			pg->v_addr = (char *) (virtadr & 0xFFFFF000);
			list_add(&pg->list, &current->pglist);
			pd_add_page(pg->v_addr, pg->p_addr, PG_USER, current->pd);
	}


	/*
	 * Creates and initializes a page directory for a process by copying another one (not working yet)
	 */
	struct page_directory *pd_copy(struct page_directory * pdfather)
	{
		struct page_directory *pd;
		u32 *pdir;
		int i;

		/* Allocate and initialize a page for the Page Directory */
		pd = (struct page_directory *) kmalloc(sizeof(struct page_directory));
		pd->base = get_page_from_heap();

		/*
		 * Kernel space. Virtual addresses < USER_OFFSET are addressed by the
   		 * kernel page table (pd0[]).
		 */
		pdir = (u32 *) pd->base->v_addr;
		for (i = 0; i < 256; i++)
			pdir[i] = pd0[i];

		/* User Space */
		for (i = 256; i < 1023; i++)
			pdir[i] = 0;

		/* Page table mirroring magic trick !... */
		pdir[1023] = ((u32) pd->base->p_addr | (PG_PRESENT | PG_WRITE));


		/* Initialize the list of user space page tables */
		INIT_LIST_HEAD(&pd->pt);

		return pd;
	}

	int pd_destroy(struct page_directory *pd)
	{
		struct page *pg;
		struct list_head *p, *n;

		/* Free the pages corresponding to the tables  */
		list_for_each_safe(p, n, &pd->pt) {
			pg = list_entry(p, struct page, list);
			release_page_from_heap(pg->v_addr);
			list_del(p);
			kfree(pg);
		}

		/* Free the page corresponding to the directory */
		release_page_from_heap(pd->base->v_addr);
		kfree(pd);

		return 0;
	}

	/*
	 * Updates the kernel address space.
 	 * NOTE: This space is common to all page directories.
	 */
	int pd0_add_page(char *v_addr, char *p_addr, int flags)
	{
		u32 *pde;
		u32 *pte;

		if (v_addr > (char *) USER_OFFSET) {
			io.print("ERROR: pd0_add_page(): %p is not in kernel space !\n", v_addr);
			return 0;
		}

		/* Checks if the page table is present  */
		pde = (u32 *) (0xFFFFF000 | (((u32) v_addr & 0xFFC00000) >> 20));
		if ((*pde & PG_PRESENT) == 0) {
			//error
		}

		// TODO: Handle the case where the page table is not present (error?)
		pte = (u32 *) (0xFFC00000 | (((u32) v_addr & 0xFFFFF000) >> 10));
		*pte = ((u32) p_addr) | (PG_PRESENT | PG_WRITE | flags);
		set_page_frame_used(p_addr);
		return 0;
	}

	/*
	 * Updates the current page directory.
	 * Inputs:
	 *  v_addr: linear address of the page
	 *  p_addr: physical address of the allocated page
	 *  flags: additional flags for the page table entry
	 *  pd: the page directory that needs to be updated with the allocated pages
 	*/
	int pd_add_page(char *v_addr, char *p_addr, int flags, struct page_directory *pd)
	{
		u32 *pde;		/* virtual address of the page directory entry */
		u32 *pte;		/* virtual address of the page table entry */
		u32 *pt;		/* virtual address of the page table */
		struct page *pg;
		int i;

		//// io.print("DEBUG: pd_add_page(%p, %p, %d)\n", v_addr, p_addr, flags); /* DEBUG */

		/*
   		 * The last entry of the PageDir points to itself.
   		 * Addresses starting with 0xFFC00000 use this entry and it follows that:
   		 *  - the 10 bits in 0x003FF000 are an index in the PageDir and designate a
   		 *    PageTable. The last 12 bits allow modifying an entry in the PageTable
   		 *  - the address 0xFFFFF000 designates the PageDir itself
   		*/
		pde = (u32 *) (0xFFFFF000 | (((u32) v_addr & 0xFFC00000) >> 20));

		/*
		 * Create the corresponding page table if it's not present
		 */
		if ((*pde & PG_PRESENT) == 0) {

			/*
			 * Allocate a page to store the table. 
			 */
			pg = get_page_from_heap();

			/* Initialize the new page table  */
			pt = (u32 *) pg->v_addr;
			for (i = 1; i < 1024; i++)
				pt[i] = 0;

			/* Add the corresponding entry in the directory */
			*pde = (u32) pg->p_addr | (PG_PRESENT | PG_WRITE | flags);

			/* Add the new page to the structure passed as a parameter */
			if (pd) 
				list_add(&pg->list, &pd->pt);
		}

		pte = (u32 *) (0xFFC00000 | (((u32) v_addr & 0xFFFFF000) >> 10));
		*pte = ((u32) p_addr) | (PG_PRESENT | PG_WRITE | flags);

		return 0;
	}

	int pd_remove_page(char *v_addr)
	{
		u32 *pte;

		if (get_p_addr(v_addr)) {
			pte = (u32 *) (0xFFC00000 | (((u32) v_addr & 0xFFFFF000) >> 10));
			*pte = (*pte & (~PG_PRESENT));
			asm("invlpg %0"::"m"(v_addr));
		}

		return 0;
	}

	/*
	 * Returns the physical address of the page associated with the passed virtual address.
	 */
	char *get_p_addr(char *v_addr)
	{
		u32 *pde;		/* virtual address of the page directory entry */
		u32 *pte;		/* virtual address of the page table entry */

		pde = (u32 *) (0xFFFFF000 | (((u32) v_addr & 0xFFC00000) >> 20));
		if ((*pde & PG_PRESENT)) {
			pte = (u32 *) (0xFFC00000 | (((u32) v_addr & 0xFFFFF000) >> 10));
			if ((*pte & PG_PRESENT))
				return (char *) ((*pte & 0xFFFFF000) + (VADDR_PG_OFFSET((u32) v_addr)));
		}

		return 0;
	}
}

void Vmm::kmap(u32 phy,u32 virt){
	pd0_add_page((char*)phy,(char*)virt,PG_USER);
}

void Vmm::init(u32 high){
	Memory_init(high);
}
