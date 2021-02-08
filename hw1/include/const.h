/*
 * DO NOT MODIFY THE CONTENTS OF THIS FILE.
 * IT WILL BE REPLACED DURING GRADING
 */
#ifndef CONST_H
#define CONST_H

#include <stdio.h>

#include "bdd.h"
#include "birp.h"

#define USAGE(program_name, retcode) do { \
fprintf(stderr, "USAGE: %s %s\n", program_name, \
"[-h] [-i FORMAT] [-o FORMAT] [-n|-r|-t THRESHOLD|-z FACTOR|-Z FACTOR]\n" \
"   -h       Help: displays this help menu.\n" \
"   -i       Input format: `pgm` or `birp` (default `birp`)\n" \
"   -o       Output format: `pgm`, `birp`, or `ascii` (default `birp`)\n\n" \
"In all cases, the program reads image data from the standard input and writes\n" \
"image data to the standard output.  If the input and output formats are both `birp`,\n" \
"then one of the following transformations may be specified (the default is an\n" \
"identity transformation; *i.e.* the image is passed unchanged):\n" \
"   -n\tComplement each pixel value\n" \
"   -r\tRotate the image 90-degrees counterclockwise\n" \
"   -t\tApply a threshold filter (with THRESHOLD in [0, 255]) to the image\n" \
"   -z\tZoom out (by FACTOR in [0, 16]), producing a smaller raster\n" \
"   -Z\tZoom in, (by FACTOR in [0, 16]), producing a larger raster\n" \
); \
exit(retcode); \
} while(0)

/* Options info, set by validargs. */
#define HELP_OPTION (0x80000000)

int global_options;  // Bitmap specifying mode of program operation.

/*
 * The following global variables have been provided for you.
 * You MUST use them for their stated purposes, because you are not permitted
 * to declare any arrays (or use any array brackets at all) in your own code.
 * Also, some of the tests we make on your program may rely on being able to
 * inspect the contents of these variables.
 */

/* Space for a 64-megapixel 8-bit grayscale image. */
#define RASTER_SIZE_MAX (8192 * 8192 * sizeof(unsigned char))
unsigned char raster_data[RASTER_SIZE_MAX];

/* See bdd.h for more information about these arrays. */
extern BDD_NODE bdd_nodes[BDD_NODES_MAX];
extern BDD_NODE *bdd_hash_map[BDD_HASH_SIZE];
extern int bdd_index_map[BDD_NODES_MAX];

/*
 * Below this line are prototypes for functions that MUST occur in your program.
 * Non-functioning stubs for all these functions have been provided in the various source
 * files, where detailed specifications for the required behavior of these functions have
 * been given.
 *
 * Your implementations of these functions MUST have exactly the specified interfaces and
 * behave exactly according to the specifications, because we may choose to test any or all
 * of them independently of your main program.
 */

/* See birp.c for the specification of the following function. */
int validargs(int argc, char **argv);

/* See birp.h for specifications of the following functions. */
int pgm_to_birp(FILE *in, FILE *out);
int birp_to_pgm(FILE *in, FILE *out);
int birp_to_birp(FILE *in, FILE *out);
int pgm_to_ascii(FILE *in, FILE *out);
int birp_to_ascii(FILE *in, FILE *out);

/* See bdd.h for specifications of the following functions. */
BDD_NODE *bdd_from_raster(int w, int h, unsigned char *raster);
void bdd_to_raster(BDD_NODE *node, int w, int h, unsigned char *raster);
int bdd_serialize(BDD_NODE *node, FILE *out);
BDD_NODE *bdd_deserialize(FILE *in);
BDD_NODE *bdd_map(BDD_NODE *node, unsigned char (*func)(unsigned char));
BDD_NODE *bdd_rotate(BDD_NODE *node, int level);
BDD_NODE *bdd_zoom(BDD_NODE *node, int level, int factor);
int bdd_lookup(int level, int left, int right);
unsigned char bdd_apply(BDD_NODE *node, int r, int c);

#endif
