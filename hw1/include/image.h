/*
 * DO NOT MODIFY THE CONTENTS OF THIS FILE.
 * IT WILL BE REPLACED DURING GRADING
 */
#ifndef IMAGE_H
#define IMAGE_H

#include <stdio.h>

#include "bdd.h"

/**
 * Read an image in PGM format from an input stream, storing the width
 * and height of the raster using the "wp" and "hp" pointers passed
 * as arguments, and storing the grayscale raster data in the array
 * "raster", in row-major order.
 *
 * @param in  The stream from which to read PGM input.
 * @param wp  Pointer to a variable into which to store the raster width.
 * @param hp  Pointer to a variable into which to store the raster height.
 * @param raster  Pointer to an array into which to store the image data.
 * @param size  Size (in bytes) of the raster array.
 * @param return  0 if the image was read successfully; -1 if any error
 * occurred.  Examples of errors are formatting errors in the PGM file,
 * I/O errors in reading the PGM file, and insufficient size of the raster
 * array to hold the image data.
 */
int img_read_pgm(FILE *in, int *wp, int *hp, unsigned char *raster, size_t size);

/**
 * Write an image to an output stream in PGM format.  The stream
 * is flushed (but not closed) after the image has been written.
 *
 * @param raster  Pointer to an array that holds the image data,
 * stored in row-major order.
 * @param w  Width of the image raster.
 * @param h  Height of the image raster.
 * @param out  Stream to which to write the PGM data.
 */
int img_write_pgm(unsigned char *raster, int w, int h, FILE *out);

/**
 * Read an image in BIRP format from an input stream, storing the width
 * and height of the raster using the "wp" and "hp" pointers passed
 * as arguments, and deserializing the BDD into the bdd_nodes array.
 *
 * @param in  The stream from which to read BIRP input.
 * @param wp  Pointer to a variable into which to store the raster width.
 * @param hp  Pointer to a variable into which to store the raster height.
 * @param return  A pointer to the root node of the BDD that represents
 * the image raster, if the image was read successfully; NULL if any error
 * occurred.  Examples of errors are formatting errors in the BIRP file,
 * I/O errors in reading the BIRP file, and the BDD nodes table having
 * insufficient size for the deserialized BDD.
 */
BDD_NODE *img_read_birp(FILE *in, int *wp, int *hp);

/**
 * Write an image to an output stream in BIRP format.  The stream
 * is flushed (but not closed) after the image has been written.
 *
 * @param node  Pointer to the root node of the BDD that holds the
 * image data.
 * @param w  Width of the image raster.
 * @param h  Height of the image raster.
 * @param out  Stream to which to write the BIRP data.
 */
int img_write_birp(BDD_NODE *node, int w, int h, FILE *out);

#endif
