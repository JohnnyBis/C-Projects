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
	printf("START INDX %d\n", currentClassIndex);
	while (currentClassIndex < 8) {
		sf_block *classPtr = sf_free_list_heads[currentClassIndex].body.links.prev;
		usedClassSizePtr = classPtr;
		sf_block *nextBlockPtr = sf_free_list_heads[currentClassIndex].body.links.next;

		while ((*nextBlockPtr).body.links.next != classPtr) {
			int blockSize = (nextBlockPtr->header) & ~0xf;
			printf("BLOCK SIZE: %d\n, %ld\n", blockSize, requiredSize);
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

	//Not free space in the lists.
	return NULL;
}

void *coalesc(void *ptr, size_t neededSize, int classSize) {
	sf_block *class_head_node = &sf_free_list_heads[7];
	sf_block *next_ptr = sf_free_list_heads[7].body.links.next;
	if (class_head_node == next_ptr) {
		//No wilderness block. No need to coalesce.
		freeBlockAllocator(classSize, ptr);
	}else{
		next_ptr->header += PAGE_SZ;
		int newSize = (next_ptr->header) & ~0xf;
		sf_header *footer = (void *) next_ptr + newSize - 8;
		*footer = next_ptr->header;
		return footer;
	}
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

		printf("AL: %d\n", alignedSize);
		int heapSize = sf_mem_end() - sf_mem_start();

		while (heapSize < alignedSize) {
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

		//Allocate memory of size alignedSize.
		sf_block *bp = (sf_block *) ptr;
		bp->header = alignedSize | 1;
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
		printf("Class %d\n", currentClassSize);

		printf("REMAINIG SIZE %d\n", bp_size);

		if (bp_size != 0 || bp_size >= 32) {
			freeBlockAllocator(currentClassSize, ptr);
		}

		return (void *) bp->body.payload;
	}else{
		int currentClassSize = getClassSize(alignedSize);
		sf_block *freeFoundBlock = freeBlockFinder(currentClassSize, alignedSize);

		if (freeFoundBlock != NULL) {
			printf("HELLO\n");
			int headerSize = freeFoundBlock->header & ~0xf;
			if ((headerSize - alignedSize) % 16 == 0 && (headerSize - alignedSize) >= 32) {
				//Split the block by changing the header size and alloc.

				//Allocate memory of size alignedSize.
				freeFoundBlock->header = alignedSize | 1;
				sf_block *free_split_block = (void *) freeFoundBlock + alignedSize;

				//Set header of remaining free list.
				free_split_block->header = (headerSize - alignedSize) | 0x2;

				sf_block *previousPtr = (*freeFoundBlock).body.links.prev;
				(*previousPtr).body.links.next = (void *) freeFoundBlock + alignedSize;
				(*free_split_block).body.links.prev = previousPtr;
				(*free_split_block).body.links.next = usedClassSizePtr;

				//Set the footer.
				sf_header *new_footer = (void *)free_split_block + headerSize - alignedSize - 8;
				*new_footer = free_split_block->header;
				//sf_show_blocks();
				//sf_show_free_lists();
				printf("HELLO\n");
				return (void *) freeFoundBlock->body.payload;
			} else {
				//Splitting not allowed.
			}

		}else{
			//Add new memory page.
			int currentClassSize = getClassSize(alignedSize);
			void *new_page = sf_mem_grow();
			coalesc(new_page, alignedSize, currentClassSize);
			//sf_show_blocks();
			//sf_show_free_lists();

			printf("HELLO 3\n");
			//Allocate new memory page.
			return sf_malloc(size);
		}
	}

	return NULL;
}

void sf_free(void *pp) {
    return;
}

void *sf_realloc(void *pp, size_t rsize) {
    return NULL;
}

void *sf_memalign(size_t size, size_t align) {
    return NULL;
}
