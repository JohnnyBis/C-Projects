/**
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "sfmm.h"
#include <errno.h>
#include <math.h>

sf_block *usedClassSizePtr = NULL;

int getAlignedSize(int size){

    int remainder;

    while (size % 16 != 0) {
    	remainder = size % 16;
        size += 16 - remainder;
    }

    if (size <= 32) {
    	return 32;
    }
    return size;
}

int getClassSize(size_t size) {
	int classIndex = 1;

	if (size <= 32) {
		return 0;
	}

	for(int i = 1; i < NUM_FREE_LISTS - 1; i++) {
		if (size <= (int) pow(2, i) * 32) {
			return classIndex;
		}
		classIndex += 1;
	}

	return 7;
}

int getBlockSize(sf_block *block) {
	return block->header & ~0xf;
}

void initializeFreeList() {
	for(int i = 0; i < NUM_FREE_LISTS; i++) {
		sf_free_list_heads[i].body.links.next = sf_free_list_heads[i].body.links.prev = &sf_free_list_heads[i];
	}
}

void freeBlockAllocator(size_t classSize, void *allocatedPtr) {
	sf_block *classPtr = sf_free_list_heads[classSize].body.links.next;
	sf_block *nextBlockPtr = sf_free_list_heads[classSize].body.links.next;

	//Traverse specific class until last sf_block.
	while (true) {
		if(classPtr != (*nextBlockPtr).body.links.next) {
			nextBlockPtr = (*nextBlockPtr).body.links.next;
		}else{
			break;
		}
	}
	sf_block *free_block = allocatedPtr;
	(*free_block).body.links.next = classPtr;
	(*free_block).body.links.prev = classPtr;
	(*nextBlockPtr).body.links.next = allocatedPtr;
}

sf_block *freeBlockFinder(int classIndex, size_t requiredSize) {
	int currentClassIndex = classIndex;
	while (currentClassIndex < 8) {
		sf_block *classPtr = &sf_free_list_heads[currentClassIndex];
		usedClassSizePtr = classPtr;
		sf_block *nextBlockPtr = sf_free_list_heads[currentClassIndex].body.links.next;

		while ((*nextBlockPtr).body.links.next != classPtr) {
			int blockSize = (nextBlockPtr->header) & ~0xf;
			if(blockSize >= requiredSize) {
				return nextBlockPtr;
			}
			nextBlockPtr = (*nextBlockPtr).body.links.next;
		}
		int blockSize = (nextBlockPtr->header) & ~0xf;

		//Check last block in the list.
		if (blockSize >= requiredSize) {
			return nextBlockPtr;
		}
		//No block found with enough size. Go to next class size.
		currentClassIndex += 1;
	}

	//No free space in the lists.
	return NULL;
}

void *coalesc_wilderness(void *ptr, size_t neededSize, int classSize) {

	sf_block *class_head_node = &sf_free_list_heads[7];
	sf_block *next_ptr = sf_free_list_heads[7].body.links.next;
	if (class_head_node == next_ptr) {
		//No wilderness block. No need to coalesce.

		sf_block *new_block = ptr - 8;
		new_block->header = PAGE_SZ | 0x2;
		sf_header *footer = ptr + PAGE_SZ - 8;
		*footer = new_block->header;
		class_head_node->body.links.next = new_block;
		new_block->body.links.next = class_head_node;
		new_block->body.links.prev = class_head_node;

	}else{
		next_ptr->header += PAGE_SZ;
		int newSize = (next_ptr->header) & ~0xf;
		sf_header *footer = (void *) next_ptr + newSize - 8;
		*footer = next_ptr->header;

		//sf_show_blocks();
		//sf_show_free_lists();
		return footer;
	}
	//sf_show_blocks();
	//sf_show_free_lists();
	return NULL;
}

void *sf_malloc(size_t size) {

	if (size <= 0) {
		return NULL;
	}

	int alignedSize = getAlignedSize(size + 8);
	//printf("HELLO %d\n", alignedSize);

	if (sf_mem_start() == sf_mem_end()) {
		initializeFreeList();
		void *ptr = sf_mem_grow() + 8; //Padding of the heap

		//Set prologue
		sf_block *prologue = (sf_block *) ptr;
		prologue->header = 32 | 1; //Add size 32 bytes to epilogue header
		ptr += 32; //Go to block header to be allocated.

		//printf("AL: %d\n", alignedSize);
		int heapSize = sf_mem_end() - sf_mem_start();

		while (heapSize <= alignedSize) {
			void *mem_grow_ptr = sf_mem_grow();

			if (mem_grow_ptr == NULL) {
				sf_block *freeBlockPtr = ptr;
				heapSize -= 48;
				freeBlockPtr->header = heapSize | 2;

				//Set epilogue.
				sf_block *epilogue_start = (sf_mem_end() - 8);
				epilogue_start->header = 1;

				//Adds footer to wilderness block.
				sf_header *footer = ptr + heapSize - 8;
				*footer = freeBlockPtr->header;

				freeBlockAllocator(7, ptr);
				sf_errno = ENOMEM;
				return NULL;
			}

			heapSize = sf_mem_end() - sf_mem_start();
		}

		if(alignedSize + 48 > heapSize) {
			void *mem_grow_ptr = sf_mem_grow();
			if (mem_grow_ptr == NULL) {
				sf_errno = ENOMEM;
				return NULL;
			}
		}

		//Allocate memory of size alignedSize.
		sf_block *bp = (sf_block *) ptr;
		bp->header = alignedSize | 0x3;
		ptr += alignedSize;

		//Set epilogue.
		sf_block *epilogue_start = (sf_mem_end() - 8);
		epilogue_start->header = 1;

		int bp_size = (sf_mem_end() - 8 - ptr);
		sf_block *wld_ptr = ptr;
		wld_ptr->header = bp_size | 0x2;

		//Adds footer to wilderness block.
		sf_header *footer = ptr + bp_size - 8;
		*footer = wld_ptr->header;

		//Add wilderness block to free_list.
		int currentClassSize = getClassSize(bp_size);

		if (bp_size != 0 || bp_size >= 32) {
			freeBlockAllocator(currentClassSize, ptr);
		}

		return (void *) bp->body.payload;
	}else{
		int currentClassSize = getClassSize(alignedSize);
		sf_block *freeFoundBlock = freeBlockFinder(currentClassSize, alignedSize);

		if (freeFoundBlock != NULL) {

			int headerSize = freeFoundBlock->header & ~0xf;
			if ((headerSize - alignedSize) % 16 == 0 && (headerSize - alignedSize) >= 32) {
				//Split the block by changing the header size and alloc.

				int oldSize = getBlockSize(freeFoundBlock);

				//Allocate memory of size alignedSize.
				freeFoundBlock->header = alignedSize | 0x3;
				sf_block *free_split_block = (void *) freeFoundBlock + alignedSize;

				int freeRemainder = headerSize - alignedSize;

				//If same class size between old free block and new free block.
				if (getClassSize(freeRemainder) == getClassSize(oldSize)) {

					//Set header of remaining free list.
					free_split_block->header = freeRemainder | 0x2;

					sf_block *previousPtr = (*freeFoundBlock).body.links.prev;
					sf_block *nextPtr = (*freeFoundBlock).body.links.next;
					(*previousPtr).body.links.next = free_split_block;

					(*free_split_block).body.links.prev = previousPtr;
					(*free_split_block).body.links.next = nextPtr;
					nextPtr->body.links.prev = free_split_block;

					//Set the footer.
					sf_header *new_footer = (void *)free_split_block + freeRemainder - 8;
					*new_footer = free_split_block->header;

				}else{
					//Remove old node.
					sf_block *before_block_p = freeFoundBlock->body.links.prev;
					sf_block *after_block_p = freeFoundBlock->body.links.next;
					before_block_p->body.links.next = after_block_p;
					after_block_p->body.links.prev = before_block_p;

					//Add new free block to new class size.

					//Set header of remaining free list.
					free_split_block->header = freeRemainder | 0x2;

					//Add to free list.
					sf_block *sentinel = &sf_free_list_heads[getClassSize(freeRemainder)];
					sf_block *next_sentinel_p = sentinel->body.links.next;
					sentinel->body.links.next = free_split_block;
					free_split_block->body.links.prev = sentinel;
					free_split_block->body.links.next = next_sentinel_p;

					//Set the footer.
					sf_header *new_footer = (void *)free_split_block + freeRemainder - 8;
					*new_footer = free_split_block->header;

				}

				return (void *) freeFoundBlock->body.payload;
			} else {
				//Splitting not allowed.

				//Allocate memory of size alignedSize.
				freeFoundBlock->header |= 1;

				usedClassSizePtr->body.links.next = usedClassSizePtr;
				usedClassSizePtr->body.links.prev = usedClassSizePtr;

				int size = getBlockSize(freeFoundBlock);
				sf_block *next_block = (void *) freeFoundBlock + size;
				next_block->header |= (1 << 1); //Set next block pal to 1.

				return (void *) freeFoundBlock->body.payload;
			}

		}else{
			//Add new memory page.
			//int currentClassSize = getClassSize(alignedSize);
			void *new_page = sf_mem_grow();
			if (new_page == NULL) {
				sf_errno = ENOMEM;
				return NULL;
			}
			coalesc_wilderness(new_page, alignedSize, currentClassSize);

			//Allocate new memory page.
			return sf_malloc(size);
		}
	}

	return NULL;
}

void *coalesc_blocks(void *p, int current_block_size) {
	p -= 8;
	sf_block *p_block = p;
	sf_block *next_block = (void*) p_block + current_block_size;
	int next_block_size = getBlockSize(next_block);
	int is_next_alloc = next_block->header & 0x1;
	sf_header *prev_block_footer = (void *) p_block - 8;


	int prev_alloc = (p_block->header & 0x2) >> 1;

	//Case 1: Prev and next block allocated --> no coalescing.
	if (prev_alloc == 1 && is_next_alloc == 1) {

		//Change status from alloc to free.
		p_block->header = current_block_size | 0x2;

		//Set the footer.
		sf_header *new_footer = (void *) p_block + current_block_size - 8;
		*new_footer = p_block->header;

		next_block->header &= ~(1 << 1); //Change next block header from pal = 1 -> pal = 0;

		return (void *) p_block;
	//Case 2: Prev block allocated, Next block free --> coalesce with next.
	}else if (prev_alloc == 1 && is_next_alloc == 0) {

		//Set header.
		p_block->header += next_block_size;
		p_block->header &= ~1;

		int new_size = getBlockSize(p_block);

		//Set the footer.
		sf_header *new_footer = p + new_size - 8;
		*new_footer = p_block->header;

		//Remove next block from free list.
		sf_block *before_next_block_p = next_block->body.links.prev;
		sf_block *after_next_block_p = next_block->body.links.next;
		before_next_block_p->body.links.next = after_next_block_p;

		sf_block *next_allocated = (void *) p_block + new_size;
		next_allocated->header &= ~(1 << 1); //Change pal for next allocated block.

		return (void *) p;
	//Case 3: Prev block free, Next block allocated --> coalesce with prev.
	}else if (prev_alloc == 0 && is_next_alloc == 1) {

		int previous_block_size = *prev_block_footer & ~0xF;
		sf_block *prev_block_p = p - previous_block_size;
		prev_block_p->header += current_block_size;
		prev_block_p->header &= ~1;

		int new_size = getBlockSize(prev_block_p);
		sf_header *new_block_footer = (void *) prev_block_p + new_size - 8;;
		*new_block_footer = prev_block_p->header;

		//Remove prev block from free list.
		sf_block *before_prev_block_p = prev_block_p->body.links.prev;
		sf_block *after_prev_block_p = prev_block_p->body.links.next;
		before_prev_block_p->body.links.next = after_prev_block_p;

		sf_block *next_allocated = (void *) prev_block_p + new_size;
		next_allocated->header &= ~(1 << 1); //Change pal for next allocated block.

		return (void *) prev_block_p;
	//Case 4: Prev and next block free.
	}else if (prev_alloc == 0 && is_next_alloc == 0) {

		int previous_block_size = *prev_block_footer & ~0xF;
		sf_block *prev_block = p - previous_block_size;

		//Remove prev block from free list.
		sf_block *before_prev_block_p = prev_block->body.links.prev;
		sf_block *after_prev_block_p = prev_block->body.links.next;
		before_prev_block_p->body.links.next = after_prev_block_p;

		//Remove next block from free list.
		sf_block *before_next_block_p = next_block->body.links.prev;
		sf_block *after_next_block_p = next_block->body.links.next;
		before_next_block_p->body.links.next = after_next_block_p;

		//Change header of prev block = prev_block_size + current_block_size + next_block_size
		prev_block->header += (current_block_size + next_block_size);


		int new_size = getBlockSize(prev_block);
		sf_header *new_block_footer = (void *) prev_block + new_size - 8;
		*new_block_footer = prev_block->header;

		sf_block *next_allocated = (void *) prev_block + new_size;
		next_allocated->header &= ~(1 << 1); //Change pal for next allocated block.

		return (void *) prev_block;
	}
	return NULL;
}

void sf_free(void *pp) {
	sf_block * pp_block = pp - 8;
	int blockSize = getBlockSize(pp_block);
	int alloc = pp_block->header & 0x1;

	if(pp == NULL || (size_t) pp % 16 != 0 ) {
		sf_errno = EINVAL;
		abort();
	}else if (blockSize % 16 != 0 || blockSize < 32 || alloc == 0) {
		sf_errno = EINVAL;
		abort();
	}else if (pp - 8 < sf_mem_start() || (pp - 8 + blockSize) > sf_mem_end()) {
		sf_errno = EINVAL;
		abort();
	}


	sf_block *new_coalesced_p = coalesc_blocks(pp, blockSize);
	int new_class_index = getClassSize((size_t) getBlockSize(new_coalesced_p));


	sf_block *sentinel_node = &sf_free_list_heads[new_class_index];
	sf_block *next_block_ptr = sentinel_node->body.links.next;

	sentinel_node->body.links.next = new_coalesced_p;
	new_coalesced_p->body.links.prev = sentinel_node;
	new_coalesced_p->body.links.next = next_block_ptr;

	if (next_block_ptr != sentinel_node) {
		next_block_ptr->body.links.prev = new_coalesced_p;
	}

}

void *sf_realloc(void *pp, size_t rsize) {
	sf_block * pp_block = pp - 8;

	int originalSize = getBlockSize(pp_block);
	int alloc = pp_block->header & 0x1;

	if(pp == NULL || (size_t) pp % 16 != 0 ) {
		sf_errno = EINVAL;
		abort();
	}else if (originalSize % 16 != 0 || originalSize < 32 || alloc == 0) {
		sf_errno = EINVAL;
		abort();
	}else if (pp - 8 < sf_mem_start() || (pp - 8 + originalSize) > sf_mem_end()) {
		sf_errno = EINVAL;
		abort();
	}

	int aligned_rsize = getAlignedSize(rsize + 8);

	if (rsize == 0) {
		sf_free(pp);
		return NULL;
	}

	//Reallocate to bigger size.
	if (originalSize < aligned_rsize) {
		void *new_block_p = sf_malloc(rsize);

		if (new_block_p) {
			memcpy(new_block_p, pp, originalSize);
			sf_free(pp);
      	}

      	return new_block_p;

	}else if(originalSize > aligned_rsize) {
		int splinterSize = originalSize - aligned_rsize;

		//If splinter less than 32 bytes, don't split.
		if (splinterSize < 32) {
			return pp;

		}else{
			int current_block_flags = pp_block->header & 0xF;
			sf_block *freed_block = pp - 8 + aligned_rsize;

			pp_block->header = aligned_rsize | current_block_flags;

			freed_block->header = splinterSize | 0x3;

			sf_free(freed_block->body.payload);

			return pp;
		}
	}

	//If same size
    return pp;
}

int alignedSizeBy(size_t size, size_t align) {
	int remainder;

    while (size % align != 0) {
    	remainder = size % align;
        size += align - remainder;
    }

    if (size <= 32) {
    	return 32;
    }
    return size;
}

void *sf_memalign(size_t size, size_t align) {
	if (align == 0 || align < 32 || size == 0) {
		sf_errno = EINVAL;
		return NULL;
	}

	int log2Res = log2(align);
	int isValid = floor(log2Res) == ceil(log2Res);

	if(isValid == 0) {
		sf_errno = EINVAL;
		return NULL;
	}

	int new_size = getAlignedSize(size) + align + 32;
	void *p = malloc(new_size);

	if ((size_t) p % align == 0) {
		return p;
	}else{
		//Payload not yet aligned.
		void *saved_p = p;
		sf_block *aligned_p = p + (align - ((size_t) p % align));
		int aligned_block_size = getAlignedSize(size);


		aligned_p->header = aligned_block_size | 0x1;
		//Free beginning pointer.
		sf_free(saved_p);

		//If current block size
		if (getBlockSize(aligned_p) > size) {
			void * to_be_freed_p = aligned_p + aligned_block_size;
			free(to_be_freed_p);
		}
		return aligned_p;
	}


    return NULL;
}
