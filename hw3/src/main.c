#include <stdio.h>
#include "sfmm.h"

int main(int argc, char const *argv[]) {

	// size_t sz_x = 80, sz_y = 64;
	// void *x = sf_malloc(sz_x);
	// printf("HELLOOOO %p", x);
	// sf_show_heap();
	// void *y = sf_realloc(x, sz_y);
	// printf("HELLO %p", y);
	// sf_show_heap();

	size_t sz_x = sizeof(int), sz_y = 10, sz_x1 = sizeof(int) * 20;
	void *x = sf_malloc(sz_x);
	/* void *y = */ sf_malloc(sz_y);
	sf_show_heap();
	x = sf_realloc(x, sz_x1);
	sf_show_heap();

    return EXIT_SUCCESS;
}
