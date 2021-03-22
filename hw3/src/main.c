#include <stdio.h>
#include "sfmm.h"

int main(int argc, char const *argv[]) {

	size_t sz_x = 64, sz_y = 8;
	void *x = sf_malloc(sz_x);
	sf_show_heap();
	void *y = sf_realloc(x, sz_y);
	sf_show_heap();
	printf("HELLO %p, %p", x, y);

	printf("RES %p\n", y);
    return EXIT_SUCCESS;
}
