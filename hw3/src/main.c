#include <stdio.h>
#include "sfmm.h"

int main(int argc, char const *argv[]) {

	sf_errno = 0;
	sf_malloc(8180);
	// sf_free(p);

	sf_show_heap();
    return EXIT_SUCCESS;
}
