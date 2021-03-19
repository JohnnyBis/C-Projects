#include <stdio.h>
#include "sfmm.h"

int main(int argc, char const *argv[]) {

	// void *x = sf_malloc(32704);
	// //cr_assert_not_null(x, "x is NULL!");
	// printf("RES: %p\n", x);

	// void *x = sf_malloc(524288);

	// printf("RES %p\n", x);
	// sf_show_blocks();

	char * ptr1 = sf_malloc(1);
    *ptr1 = 'A';
    sf_show_blocks();
    sf_show_free_lists();
    printf("\n");

    char * ptr2 = sf_malloc(1);
    *(ptr2) = 'B';

    sf_show_blocks();
    sf_show_free_lists();
    printf("\n");

    int * ptr3 = sf_malloc(24 * sizeof(int));
    *(ptr3 + 0) = 1;
    *(ptr3 + 1) = 69;
    *(ptr3 + 2) = 80;
    *(ptr3 + 23) = 69;

    sf_show_blocks();
    sf_show_free_lists();
    printf("\n");

    return EXIT_SUCCESS;
}
