#include <stdio.h>
#include "sfmm.h"

int main(int argc, char const *argv[]) {
	// char * ptr1 = sf_malloc(1);
 //    *ptr1 = 'A';
 //    //sf_show_blocks();
 //    printf("\n");
 //    char * ptr2 = sf_malloc(1);
 //    *(ptr2) = 'B';
    double* ptr = sf_malloc(sizeof(double));
    double* ptr_two = sf_malloc(10000);
    *ptr = 320320320e-320;

    printf("%f\n", *ptr);
    printf("%f\n", *ptr_two);
    // printf("%f\n", *ptr3);
    // printf("%f\n", *ptr4);
    // printf("%f\n", *ptr5);
    sf_free(ptr);

    return EXIT_SUCCESS;
}
