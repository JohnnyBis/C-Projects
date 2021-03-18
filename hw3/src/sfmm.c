/**
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "sfmm.h"
#include <math.h>

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

void blockPlacement(int classIndex, size_t size, void *ptr) {
	int currentClassIndex = classIndex;
	printf("CURR INDX %d\n", currentClassIndex);
	int notFound = 1;
	while (notFound && currentClassIndex < 8) {
		void *nextBlockPtr = sf_free_list_heads[classIndex].body.links.next;
		void *prevBlockPtr = sf_free_list_heads[classIndex].body.links.prev;
		while (nextBlockPtr == prevBlockPtr) {
			sf_header *currentBlockHeader = nextBlockPtr;
			int blockSize = *currentBlockHeader & ~0xf;
			printf("SIZE BLOCK: %d\n", blockSize);
			break;
		}
		break;
		// while ()
		// sf_free_list_heads[NUM_FREE_LISTS - 1].body.links.next = ptr;
		// }
	}
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
		sf_block *prologue = (sf_block *) ptr;
		prologue->header = 32 | 1; //Add size 32 bytes to epilogue header
		ptr += 32; //Go to block header to be allocated.
		sf_block *bp = (sf_block *) ptr;
		bp->header = alignedSize | 1;
		ptr += alignedSize;
		//printf("PAYLOAD: %p\n", bp->body.payload);

		//Set epilogue.
		sf_block *epilogue_start = (sf_mem_end() - 8);
		epilogue_start->header = 1;
		int bp_size = (sf_mem_end() - 8 - ptr);
		//printf("SIZE: %d\n", bp_size);
		sf_block *wld_ptr = ptr;
		wld_ptr->header = bp_size | 0x2;
		//wld_ptr->body.links.prev =

		//Adds footer to wilderness block.
		sf_header *footer = ptr + bp_size - 8;
		*footer = wld_ptr->header;

		//Add wilderness block to free_list.
		int currentClassSize = getClassSize(bp_size);
		printf("Class %d\n", currentClassSize);

		sf_block *classPtr = sf_free_list_heads[currentClassSize].body.links.next;
		sf_block *nextBlockPtr = sf_free_list_heads[currentClassSize].body.links.prev;

		//Traverse specific class until last sf_block.
		while (true) {
			if(classPtr != (*nextBlockPtr).body.links.next) {
				nextBlockPtr = (*nextBlockPtr).body.links.next;
			}else{
				break;
			}
		}
		(*wld_ptr).body.links.next = classPtr;
		(*wld_ptr).body.links.prev = classPtr;
		(*nextBlockPtr).body.links.next = ptr;

		//blockPlacement(currentClassSize, bp_size, ptr);

		//Get correct block size.
		//sf_block *allocatedBlock = (prologue + 32);
		//allocatedBlock -> header = ( << 4)| 1;
		sf_show_blocks();
		sf_show_free_lists();
		//return allocatedBlock;
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
