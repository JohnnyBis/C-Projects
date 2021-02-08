/*
 * DO NOT MODIFY THE CONTENTS OF THIS FILE.
 * IT WILL BE REPLACED DURING GRADING
 */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "bdd.h"
#include "image.h"

static int skip_whitespace(FILE *f) {
    int c;
    while(isspace(c = fgetc(f)))
	;
    if(c == EOF)
	return EOF;
    ungetc(c, f);
    return 0;
}

static int skip_comment(FILE *f) {
    int c;
    if((c = fgetc(f)) == '#') {
	while((c = fgetc(f)) != '\n')
	    ;
    }
    if(c == EOF)
	return EOF;
    ungetc(c, f);
    return 0;
}

// Common code for reading the file header, after the magic.
static int img_read_header(FILE *file, char *type, int *wp, int *hp) {
    int c, err, max;
    // The GIMP puts a comment after P5\n, though the PGM/PBM spec doesn't mention this as valid.
    if(skip_comment(file) == EOF) {
	fprintf(stderr, "Invalid %s file (bad header)\n", type);
	goto bad;
    }
    err = fscanf(file, "%d %d %d", wp, hp, &max);
    if(err != 3) {
	fprintf(stderr, "Invalid %s file (bad header parameters)\n", type);
	goto bad;
    }
    // The PBM/PGM specification says a comment could be here.
    if(skip_comment(file) == EOF) {
	fprintf(stderr, "Invalid %s file (bad comment/no data)\n", type);
	goto bad;
    }
    // At this point, there should be a single whitespace character ("usually a newline").
    if(!isspace(c = fgetc(file))) {
	fprintf(stderr, "Invalid %s file (no data)\n", type);
	goto bad;
    }
    if(max >= 256) {
	fprintf(stderr, "%s file maximum pixel value %d is too large (255 max supported)\n",
		type, max);
	goto bad;
    }
    return 0;

bad:
    return -1;
}

// Spec: http://netpbm.sourceforge.net/doc/pgm.html
int img_read_pgm(FILE *file, int *wp, int *hp, unsigned char *raster, size_t size) {
    int c;
    unsigned int max;
    int err = fscanf(file, "P5 ");
    if(err < 0) {
	fprintf(stderr, "Invalid PGM file (missing/bad magic)\n");
	goto bad;
    }
    if((err = img_read_header(file, "PGM", wp, hp)) < 0)
	goto bad;

    // Check that there is enough space to hold the data.
    if(*wp * *hp * sizeof(unsigned char) > size)
	goto bad;

    // Read the raster.
    unsigned char *dp = raster;
    for(int i = 0; i < *hp; i++) {
	for(int j = 0; j < *wp; j++) {
	    if((c = fgetc(file)) == EOF) {
		fprintf(stderr, "PGM file image data truncated\n");
		goto bad;
	    }
	    *dp++ = c;
	}
    }
    return 0;

 bad:
    return -1;
}

int img_write_pgm(unsigned char *data, int w, int h, FILE *file) {
    if(file == NULL)
	return -1;
    fprintf(file, "P5 %d %d 255\n", w, h);
    for(int i = 0; i < h; i++) {
	for(int j = 0; j < w; j++) {
	    fputc(data[i * w + j], file);
	}
    }
    return fflush(file);
}

BDD_NODE *img_read_birp(FILE *file, int *wp, int *hp) {
    int c;
    unsigned int max;
    int err = fscanf(file, "B5 ");
    if(err < 0) {
	fprintf(stderr, "Invalid BIRP file (missing/bad magic)\n");
	goto bad;
    }
    if((err = img_read_header(file, "BIRP", wp, hp)) < 0)
	goto bad;

    // Read the serialized BDD.
    BDD_NODE *node = bdd_deserialize(file);
    return node;

 bad:
    return NULL;
}

int img_write_birp(BDD_NODE *node, int w, int h, FILE *file) {
    if(file == NULL)
	return -1;
    fprintf(file, "B5 %d %d 255\n", w, h);
    bdd_serialize(node, file);
    return fflush(file);
}
