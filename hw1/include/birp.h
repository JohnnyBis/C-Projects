/*
 * DO NOT MODIFY THE CONTENTS OF THIS FILE.
 * IT WILL BE REPLACED DURING GRADING
 */
#ifndef BIRP_H
#define BIRP_H

#include <stdio.h>

/**
 * Read a PGM image file from an input stream, construct a BDD
 * representation of the image data, and serialize the BDD representation
 * to an output stream.
 *
 * @param in  Stream from which to read the PGM image data.
 * @param out  Stream to which to write the serialized BDD.
 * @return  0 if successful, -1 if any error occurs.
 */
int pgm_to_birp(FILE *in, FILE *out);

/**
 * Read a serialized BDD from an input stream, unpack the BDD into a
 * grayscale raster, and write the raster to an output stream in PGM
 * image format.
 *
 * @param in  Stream from which to read the serialized BDD.
 * @param out  Stream to which to write the PGM image.
 * @return  0 if successful, -1 if any error occurs.
 */
int birp_to_pgm(FILE *in, FILE *out);

/**
 * Read a serialized BDD from an input stream, apply a transformation
 * to the BDD according to global options settings, and serialize the
 * resulting BDD to an output stream.
 *
 * @param in  Stream from which to read serialized BDD input.
 * @param out  Stream to which to write serialized BDD output.
 * @return  0 if successful, -1 if any error occurs.
 */
int birp_to_birp(FILE *in, FILE *out);

/**
 * Read a PGM image file from an input stream and print an "ASCII art"
 * approximation of the image to a specified output stream.
 * The output consists of h rows of ASCII characters with w characters per
 * row, where w and h are the width and height of the PGM image.
 * Each output character represents one pixel in the image.
 * Pixel values are mapped to ASCII characters according to the following
 * scheme:
 *
 *     0 -  63: ' ' (space)
 *    64 - 127: '.'
 *   128 - 191: '*'
 *   192 - 255: '@'
 *
 * @param in  Stream from which to read the PGM image data.
 * @param out  Stream to which to write the ASCII art output
 * @return  0 if successful, -1 if any error occurs.
 */
int pgm_to_ascii(FILE *in, FILE *out);

/**
 * Read a serialized BDD from an input stream and print an "ASCII art"
 * approximation of the image to a specified output stream.
 * The output consists of 2^d rows of ASCII characters with 2^d characters per
 * row, where d is one-half the number of levels of the BDD.
 * Each output character represents one pixel in the image as in the
 * specification for pgm_to_ascii().
 *
 * @param in  Stream from which to read the serialized BDD.
 * @param out  Stream to which to write the ASCII art output
 * @return  0 if successful, -1 if any error occurs.
 */
int birp_to_ascii(FILE *in, FILE *out);

#endif
