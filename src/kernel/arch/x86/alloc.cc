#include "../../core/os.h"

extern "C" {
	/* Kernel Segment BReak - change memory segment size; dynamically expand kernel heap memory space */
	void *ksbrk(int n)
	{
		struct kmalloc_header *chunk;
		char *p_addr;
		int i;

		if ((kern_heap + (n * PAGESIZE)) > (char *) KERN_HEAP_LIM) {
			io.print
			    ("PANIC: ksbrk(): no virtual memory left for kernel heap !\n");
			return (char *) -1;
		}

		chunk = (struct kmalloc_header *) kern_heap;

		/* Allocation of a free page */
		for (i = 0; i < n; i++) {
			p_addr = get_page_frame();
			if ((int)(p_addr) < 0) {
				io.print
				    ("PANIC: ksbrk(): no free page frame available !\n");
				return (char *) -1;
			}

			/* Adding to page directory */
			pd0_add_page(kern_heap, p_addr, 0);

			kern_heap += PAGESIZE;
		}

		/* Marking memory chunk as unused for kmalloc */
		chunk->size = PAGESIZE * n;
		chunk->used = 0;

		return chunk;
	}

	/* allocate memory block */
	void *kmalloc(unsigned long size)
	{
		if (size==0)
			return 0;
		unsigned long realsize;	/* Total record size */
		struct kmalloc_header *chunk, *other;

		if ((realsize =
		     sizeof(struct kmalloc_header) + size) < KMALLOC_MINSIZE)
			realsize = KMALLOC_MINSIZE;

		/*
		 * We are searching for a free block of 'size' bytes by traversing the kernel HEAP from the beginning.
		 */
		chunk = (struct kmalloc_header *) KERN_HEAP;
		//We will keep looping until we find a chunk that is NOT used AND is >= the size we need (realsize)
		while (chunk->used || chunk->size < realsize) {
			if (chunk->size == 0) {
				io.print("\nPANIC: kmalloc(): corrupted chunk on %x with null size (heap %x) !\nSystem halted\n", chunk, kern_heap);
				//error
				asm("hlt");
				return 0;
			}

			//Move the chunk pointer to the next block in the kernel heap.
			chunk = (struct kmalloc_header *) ((char *) chunk + chunk->size);

			//We are at the end of the heap, no available chunks
			if (chunk == (struct kmalloc_header *) kern_heap) {
				if ((int)(ksbrk((realsize / PAGESIZE) + 1)) < 0) {
					io.print("\nPANIC: kmalloc(): no memory left for kernel !\nSystem halted\n");
					asm("hlt");
					return 0;
				}
			//There's a corruption in the heap - chunk pointer is beyond the allocated heap space
			} else if (chunk > (struct kmalloc_header *) kern_heap) {
				io.print("\nPANIC: kmalloc(): chunk on %x while heap limit is on %x !\nSystem halted\n",chunk, kern_heap);
				asm("hlt");
				return 0;
			}
		}

		/*
		 * Found free block with size >= 'size'
		 * We limit size block
		 */

		// If the leftover space is less than the minimum size, allocating the entire block is more efficient than creating another small free block.
		if (chunk->size - realsize < KMALLOC_MINSIZE)
			chunk->used = 1;
		// We have too much free memory in the block if we dedicate this entire chunk to this chunk. So let's split up this block.
		else {
			other =
			    (struct kmalloc_header *) ((char *) chunk + realsize);
			other->size = chunk->size - realsize;
			other->used = 0;

			chunk->size = realsize;
			chunk->used = 1;
		}

		kmalloc_used += realsize;

		/* Return a pointer to the memory area */
		return (char *) chunk + sizeof(struct kmalloc_header);
	}

	/* free memory block */
	void kfree(void *v_addr)
	{
		//If block is null, we do nothing
		if (v_addr==(void*)0)
			return;

		struct kmalloc_header *chunk, *other;

		/* We free the allocated block */
		chunk = (struct kmalloc_header *) ((u32)v_addr - sizeof(struct kmalloc_header));
		chunk->used = 0;

		kmalloc_used -= chunk->size;

		/*
		 * Merge free block with next free block (Coalescing, helps reduce fragmentation)
		 */
			//If we combine the current block with the next one - the new combined block must be valid, is within the valid kern_heap space and NOT used
		while ((other = (struct kmalloc_header *) ((char *) chunk + chunk->size)) && other < (struct kmalloc_header *) kern_heap && other->used == 0)
			chunk->size += other->size;
	}
}
