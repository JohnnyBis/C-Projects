#include <stdio.h>
#include <stdlib.h>

#include "const.h"
#include "debug.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

int main(int argc, char **argv) {
	//Set all nodes to NULL.
    for(int i = 0; i < BDD_HASH_SIZE; i++) {
        *(bdd_hash_map + i) = NULL;
    }

    if(validargs(argc, argv)) {
        USAGE(*argv, EXIT_FAILURE);
    }

    if (global_options == 0x21) {
    	if(pgm_to_birp(stdin, stdout) == 0) {
        	return EXIT_SUCCESS;
    	}
    }else if(global_options == 0x80000000) {
    	USAGE(*argv, EXIT_SUCCESS);
    }else if((global_options & 0xFF) == 0x22) {
    	if(birp_to_birp(stdin, stdout) == 0) {
            return EXIT_SUCCESS;
        }
    }else if(global_options == 0x32) {
        if(birp_to_ascii(stdin, stdout) == 0){
            return EXIT_SUCCESS;
        }
    }else if(global_options == 0x31) {
        if(pgm_to_ascii(stdin, stdout) == 0) {
            return EXIT_SUCCESS;
        }
    }else if(global_options == 0x12) {
        if(birp_to_pgm(stdin, stdout) == 0){
            return EXIT_SUCCESS;
        }
    }
    return EXIT_FAILURE;
}
/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
