/*
 * DO NOT MODIFY THE CONTENTS OF THIS FILE.
 * IT WILL BE REPLACED DURING GRADING
 */
#ifndef BDD_H
#define BDD_H

#include <stdio.h>

/*
 * Definition of BDD node structure type.
 */
typedef struct bdd_node {
    char level;
    int left;
    int right;
} BDD_NODE;

/*
 * Each BDD node represents a function on some number of boolean arguments.
 * We refer to the number of arguments as the "level" of the node.
 * The following defines, for our purposes here, the maximum level of a
 * BDD node.
 */
#define BDD_LEVELS_MAX 32

/*
 * We are using a variant of BDDs called "multi-terminal" BDDs or MTBDDs,
 * for short.  Traditional BDDs represent boolean-valued functions of boolean
 * arguments, but here we want to represent functions whose values are 8-bit
 * pixel values for grayscale images.  So there are a total of 256 possible
 * values that might be represented by a leaf node of a BDD.  Node indices
 * less than this value are reserved to represent leaf nodes, so that the
 * value of a leaf node is given by its index, rather than having to store
 * the value in the node itself.
 */
#define BDD_NUM_LEAVES 256

/*
 * Space to store BDD nodes.
 * The first BDD_NUM_LEAVES entries are unused, as these entries correspond
 * to leaf nodes, which are not stored explicitly, but whose values are given
 * by their indices.  So the first entry that can actually be used to store
 * a non-leaf node is at index BDD_NUM_LEAVES.
 */
#define BDD_NODES_MAX (1<<20) // 1048576
BDD_NODE bdd_nodes[BDD_NODES_MAX];

/*
 * Open-addressed hash map mapping pairs (l, r) of BDD node indices
 * to BDD node indices.  Each node can be present at most once, so
 * there will be at most BDD_NODES_MAX entries in the table.
 * We define the table size to yield a load factor of no more than 0.5;
 * in particular the table will never be full.  So the table size must
 * be a prime number >= 2 * BDD_NODES_MAX.
 */
#define BDD_HASH_SIZE 2097169
BDD_NODE *bdd_hash_map[BDD_HASH_SIZE];

/*
 * Map used in BDD serialization and deserialization.
 * In serialization, it is used to store the mapping from indices of BDD
 * nodes that have already been serialized, to the serial numbers of those
 * nodes in the output stream.
 * In deserialization, it is used to store the mapping from serial numbers
 * of BDD nodes in the input stream, to the indices of these nodes in the
 * BDD node table.
 */
int bdd_index_map[BDD_NODES_MAX];

/**
 * Determine the minimum number of levels required to cover a raster
 * with a specified width w and height h.
 *
 * @param w  The width of the raster to be covered.
 * @param h  The height of the raster to be covered.
 * @return  The least value l >=0 such that w <= 2^(l/2) and h <= 2^(l/2).
 */
int bdd_min_level(int w, int h);

/**
 * Given a height h, a width w, and an array of h x w one-byte values,
 * build a BDD representing the smallest 2^d x 2^d square array A such that
 * h <= 2^d and w <= 2^d, whose values agree with those of the given array
 * for indices in [0, h) x [0, w) (i.e. A[i, j] = data[i * w + j] for
 * (i, j) in [0, h) x [0, w)), and whose values are zero for indices outside
 * of this set.
 *
 * @param w  The width (number of columns) of the array of data.
 * @param h  The height (number of rows) of the array of data.
 * @param raster  An array of h x w one-byte values, stored in row-major order,
 * so that the entry at row i and column j is at data[i * w + j].
 * @return  A BDD node that represents a 2^d x 2^d array A of one-byte values
 * as specified above, or NULL if any error occurs.
 */
BDD_NODE *bdd_from_raster(int w, int h, unsigned char *raster);

/**
 * Given a BDD node with level 2*d, a nonnegative integer w, and a nonnegative
 * integer h, interpret the BDD node as representing a 2^d x 2^d square array
 * of values and store the values from the sub-array having indices in
 * [0, w) x [0, h) into a specified array "data" as a w x h array in row-major
 * order.
 *
 * @param node  The BDD node.
 * @param w  The width (number of columns) of the raster to be stored.
 * @param h  The height (number of rows) of the raster to be stored.
 * @param raster  An array, having at least w x h entries, into which the
 * raster is to be stored in row-major order.
 */
void bdd_to_raster(BDD_NODE *node, int w, int h, unsigned char *raster);

/**
 * Serialize a BDD as a sequence of instructions for building the BDD in a
 * bottom-up fashion.  Each instruction begins with a 1-byte opcode, with
 * the following meanings:
 *
 *   '@' - build a level 0 node (i.e. a leaf)
 *   'A' - build a level 1 node
 *   ...
 *   '`' - build a level 31 node
 *
 * Following the level number is either a one-byte value, in the case of
 * a level 0 node, or else two 4-byte values specified in little-endian byte
 * order, in the case of a node at a level > 0.  For a level 0 node, the
 * one-byte value specifies the value of the leaf node of the BDD.
 * For a node at a level > 0, the first 4-byte value specifies the serial
 * number in the input stream of the left child node and the second 4-byte
 * value specifies the serial number in the input stream of the right child
 * node.  Nodes have implicit serial numbers that start from 1 and increase by 1
 * as each successive node is constructed.  In a well-formed serialized BDD,
 * the serial numbers of the children of a node will always be less than
 * the serial number of the node itself.
 *
 * @param node  The node at the root of the BDD to be serialized.
 * @param out  Stream on which to output the serialized BDD.
 * @return  0 if successful, -1 if any error occurs.
 */
int bdd_serialize(BDD_NODE *node, FILE *out);

/**
 * Deserialize a BDD from an input stream, which is assumed to have the
 * format described in the documentation for bdd_serialize.  This function
 * should validate the input stream and return an error indication if the
 * input stream does not have the proper format.
 *
 * @param in  Input stream from which to read the serialized BDD.
 * @return  The BDD node with the greatest serial number in the input stream
 * (which will be the last BDD node constructed while reading the input),
 * if deserialization was successful, or NULL if there was any error.
 */
BDD_NODE *bdd_deserialize(FILE *in);

/**
 * Given a BDD node that represents an array of values, construct a new
 * BDD node that represents the result of applying a specified function
 * to each entry of the array.  In general, this will require building
 * a new BDD, because the function need not be one-to-one and in that
 * case nodes that were distinct in the original BDD could map to the
 * same node in the new BDD.
 *
 * @param node  The BDD node that represents the input array.
 * @param func  The function to be applied to each entry.
 * @return  The BDD node that represents the result of applying the
 * function to each entry.
 */
BDD_NODE *bdd_map(BDD_NODE *node, unsigned char (*func)(unsigned char));

/**
 * Given a BDD node with level 2*d, representing a 2^d x 2^d square image,
 * construct a new BDD node that represents the result of rotating the
 * original image counterclockwise by 90 degrees.  This is easily
 * accomplished by the following recursive procedure:
 *
 *   Given a node at level 2*d that represents a "big square" composed
 *   of four "smaller squares", represented by nodes at level 2*(d-1):
 *
 *      A  B
 *      C  D
 *
 *   recursively rotate the smaller squares to obtain A', B', C', D',
 *   then combine them to produce the large square:
 *
 *      B' D'
 *      A' C'
 *
 *   The base cases of the recursion, obviously, are leaf nodes, which
 *   are left unchanged because they represent single pixels in the
 *   image.
 *
 * @param node  The BDD node to transform.
 * @param level  The level at which to interpret the node,
 * which (due to "skipped levels") might be larger than the level
 * recorded in the node itself.
 * @return  The BDD node resulting from the transformation.
 */
BDD_NODE *bdd_rotate(BDD_NODE *node, int level);

/**
 * Given a BDD node that represents a 2^d x 2^d image, construct a new
 * BDD node that represents the result of "zooming in" by a specified
 * nonnegative "factor" k.  Zooming in is an operation that increases
 * the number of pixels by replacing each pixel in the individual image
 * by a 2^k x 2^k array of identical pixels in the transformed image.
 * Zooming by a factor of 0 is an identity transformation.
 *
 * @param node  The BDD node to transform.
 * @param level  The level at which to interpret the node,
 * which (due to "skipped levels") might be larger than the level
 * recorded in the node itself.
 * @param factor   The "zoom factor", which must be a nonnegative value
 * satisfying 0 <= d+k <= BDD_LEVELS_MAX.
 * @return  the BDD node that represents the result of the "zoom"
 * operation.
 */
BDD_NODE *bdd_zoom(BDD_NODE *node, int level, int factor);

/**
 * Look up, in the node table, a BDD node having the specified level and children,
 * inserting a new node if a matching node does not already exist.
 * The returned value is the index of the existing node or of the newly inserted node.
 * It is the caller's responsibility to ensure that the arguments are valid.
 *
 * @param level  The level number, in the range [0, BDD_LEVELS_MAX], of the
 * BDD node to be looked up.
 * @param left  The index, in the bdd_nodes array, of the left (i.e. "0") child
 * of the BDD node to be looked up.
 * @param left  The index, in the bdd_nodes array, of the right (i.e. "1") child
 * of the BDD node to be looked up.
 * @return  An index in the bdd_nodes array, either of an existing node or
 * of a newly inserted node.
 */
int bdd_lookup(int level, int left, int right);

/**
 * Given a BDD node representing a 2^d x 2^d square array of values,
 * obtain the value at a specified row index r and column index c,
 * where it is assumed that 0 <= r < 2^d and 0 <= c < 2^d.
 * This value is obtained by traversing a path from the given BDD node
 * to a leaf node, using alternating bits of the row and column indices
 * to choose the child node to visit at each stage.  In more detail,
 * the child of the leaf node is chosen using bit d-1 (i.e. the most
 * significant bit) of the row index r (where 0 means left and 1 means
 * right), the child of that node is chosen using bit d-1 of the column
 * index c, the child of that node is chosen using bit d-2 of the
 * row index r, and so on until finally bit 0 of the column index 0
 * has been used, resulting in a leaf node.
 *
 * Note that a child of a BDD node level k can have a level less than k-1;
 * i.e. it is possible to "skip" levels along a path from a node to a leaf.
 * This can occur due to the fact that BDD nodes are not explicitly constructed
 * constructed when their left and right children would be identical.
 * The procedure of tracing the path from the given node to a leaf must
 * take possible "level skipping" into account.
 *
 * @param node  A BDD node, representing a 2^d x 2^d square array of values.
 * @param r  Row index of the value to be obtained, where 0 <= r < 2^d.
 * @param c  Column index of the value to be obtained, where 0 <= c < 2^d.
 * @return  The value in the array at the specified row and column index.
 */
unsigned char bdd_apply(BDD_NODE *node, int r, int c);

#endif
