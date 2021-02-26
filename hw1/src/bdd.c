#include <stdlib.h>
#include <stdio.h>

#include "bdd.h"
#include "debug.h"
#include <math.h>

/*
 * Macros that take a pointer to a BDD node and obtain pointers to its left
 * and right child nodes, taking into account the fact that a node N at level l
 * also implicitly represents nodes at levels l' > l whose left and right children
 * are equal (to N).
 *
 * You might find it useful to define macros to do other commonly occurring things;
 * such as converting between BDD node pointers and indices in the BDD node table.
 */
#define LEFT(np, l) ((l) > (np)->level ? (np) : bdd_nodes + (np)->left)
#define RIGHT(np, l) ((l) > (np)->level ? (np) : bdd_nodes + (np)->right)

#define INDEX_TO_POINTER(i) *(bdd_nodes + i)
#define NODE_TO_INDEX(n) (n - bdd_nodes)

int actual_width;
int availableIndex = BDD_NUM_LEAVES;
int serialNumber = 0;

/**
 * Look up, in the node table, a BDD node having the specified level and children,
 * inserting a new node if a matching node does not already exist.
 * The returned value is the index of the existing node or of the newly inserted node.
 *
 * The function aborts if the arguments passed are out-of-bounds.
 */
int bdd_lookup(int level, int left, int right) {

    if(level == 0) {
        BDD_NODE node = {level, left, right};
        *(bdd_nodes + left) = node;
        return left;
    }

    if (left != right) {
        //Check hashmap first - find existing node
        int c = 0;
        while(*(bdd_hash_map + c)) {
            BDD_NODE currNode = **(bdd_hash_map + c);

            if(currNode.level == level && currNode.right == right && currNode.left == left) {
                return NODE_TO_INDEX(*(bdd_hash_map + c));
            }
            c += 1;
        }

        //Get unused node from table and initialize it with new node.
        BDD_NODE node = {level, left, right};
        *(bdd_nodes + availableIndex) = node;
        *(bdd_hash_map + c) = bdd_nodes + availableIndex;
        ++availableIndex;
        return availableIndex - 1;
    }else{
        //Go to bdd_node @ left and check if it's level is lower than given level.
        //If it is, replace child node with given level.
        //Give lowest possible level.
        BDD_NODE leftNodeStored = *(bdd_nodes + left);

        if (leftNodeStored.level > level) {
            leftNodeStored.level = level;
        }
        return left;
    }
    return -1;
}


int subdivideQuads(int x, int y, int currW, int currH, int isHorizontal, int level, unsigned char *img) {

    int index;

    if (currW == 1 && currH == 1) {
        int color = *(img + x + (y * actual_width));

        if(level == 0) {
            return color;
        }
        return bdd_lookup(level, color, color);
    }

    if(isHorizontal) {
        int left = subdivideQuads(x, y, currW, currH/2, 0, level - 1, img);
        int right = subdivideQuads(x, y + currH/2, currW, currH/2, 0, level - 1, img);
        index = bdd_lookup(level, left, right);
    }else{
        int left = subdivideQuads(x, y, currW/2, currH, 1, level - 1, img);
        int right = subdivideQuads(x + currW/2, y, currW/2, currH, 1, level - 1, img);
        index = bdd_lookup(level, left, right);
    }

    return index;
}

int bdd_min_level(int w, int h) {
    int res = 0;
    //Find larger num
    //  if larger num is pow(2) then use while loop
    //  if not, keep adding 1 to larger num until it is pow(2)
    //For pow(2) do bitwise operation with AND
    if(w == h) {
        int temp = w;
        while (temp >>= 1) {
            res++;
        }
        res *= 2;
    }

    return res;
}

BDD_NODE *bdd_from_raster(int w, int h, unsigned char *raster) {
    unsigned char *p = raster;
    int d = 0;
    int minLevel = bdd_min_level(w, h);

    actual_width = w;
    subdivideQuads(0, 0, w, h, 1, minLevel, p);
    return (bdd_nodes + availableIndex - 1);
}

void bdd_to_raster(BDD_NODE *node, int w, int h, unsigned char *raster) {
    for(int i = 0; i < (w * h); i ++) {
        *(raster + i) = bdd_apply(node, i / w, i % h);
    }
    return;
}

int decimalToOctal(int num) {
    int result = 0;
    int x = 1;
    while (num != 0) {
        result += (num % 8) * x;
        num = num / 8;
        x = x * 10;
    }
    return result;
}

void post_order_traversal(BDD_NODE *currentNode, FILE *out) {

    if (currentNode->level == 0) {
        return;
    }

    // ADD TEST CASES with children level.
    if (*(bdd_index_map + currentNode->left) == 0){

        if(currentNode->left <= 255) {
            fputc('@', out);
            fputc(currentNode->left, out);
        }else{
            post_order_traversal((bdd_nodes + currentNode->left), out);
        }
        *(bdd_index_map + currentNode->left) = ++serialNumber;
    }

    if (*(bdd_index_map + currentNode->right) == 0){

        if(currentNode->right <= 255) {
            fputc('@', out);
            fputc(currentNode->right, out);
        }else{
            post_order_traversal((bdd_nodes + currentNode->right), out);
        }
        *(bdd_index_map + currentNode->right) = ++serialNumber;
    }

    fputc((currentNode->level + 64), out);
    int nodeNumLeft = *(bdd_index_map + currentNode->left);
    fputc(nodeNumLeft & 0xFF, out);
    fputc((nodeNumLeft & (0xFF << 8)) >> 8, out);
    fputc((nodeNumLeft & (0xFF << 16)) >> 16, out);
    fputc((nodeNumLeft & (0xFF << 24)) >> 24, out);

    int nodeNumRight = *(bdd_index_map + currentNode->right);
    fputc(nodeNumRight & 0xFF, out);
    fputc((nodeNumRight & (0xFF << 8)) >> 8, out);
    fputc((nodeNumRight & (0xFF << 16)) >> 16, out);
    fputc((nodeNumRight & (0xFF << 24)) >> 24, out);
}

int bdd_serialize(BDD_NODE *node, FILE *out) {
    serialNumber = 0;
    post_order_traversal(node, out);
    return 0;
}

BDD_NODE *bdd_deserialize(FILE *in) {
    int counter = 1;
    int c;
    int leaf;

    while((c = fgetc(in)) != EOF) {
        if(c == 64) {
            leaf = fgetc(in);
            if(*(bdd_index_map + counter) == 0) {

                *(bdd_index_map + counter) = leaf;
                counter++;
            }

        }else if(c >= 65) {
            int leftSerial = fgetc(in);
            leftSerial |= (fgetc(in) << 8);
            leftSerial |= (fgetc(in) << 16);
            leftSerial |= (fgetc(in) << 24);

            int rightSerial = fgetc(in);
            rightSerial |= (fgetc(in) << 8);
            rightSerial |= (fgetc(in) << 16);
            rightSerial |= (fgetc(in) << 24);

            //Return null if children serial numbers are larger than parent serial number.
            if(counter < leftSerial || counter < rightSerial) {
                return NULL;
            }

            int currentBddIndex = 255 + (counter - 2);

            //Return null if currentBddIndex goes out of bound in bdd_nodes array.
            if(currentBddIndex > BDD_NODES_MAX) {
                return NULL;
            }

            if(*(bdd_index_map + counter) == 0) {
                *(bdd_index_map + counter) = currentBddIndex; //Map serial # -> bdd index of current node.
            }

            BDD_NODE newNode = {c - 64, *(bdd_index_map + leftSerial), *(bdd_index_map + rightSerial)};

            *(bdd_nodes + currentBddIndex) = newNode;

            counter ++;
        }
    }
    return (bdd_nodes + 255 + (counter - 3));
}

unsigned char bdd_apply(BDD_NODE *node, int r, int c) {
    BDD_NODE *currentNode = node;
    while(currentNode->level != 0) {
        int level = currentNode->level;
        int bitFound;

        if(level % 2 == 0) {
            //Even level --> check row.
            int bitRow = (level - 2) / 2;
            int mask = 1 << bitRow;
            bitFound = (r & mask) >> bitRow;
        }else {
            //Odd level --> check col.
            int bitCol = (level - 1) / 2;
            int mask = 1 << bitCol;
            bitFound = (c & mask) >> bitCol;
        }

        if(bitFound == 0) {
            //Left child
            if ((currentNode -> left) <= 255) {
                return currentNode -> left;
            }
            currentNode = (bdd_nodes + currentNode->left);

        }else if(bitFound == 1){
            //Right child
            if ((currentNode -> right) <= 255) {
                return currentNode -> right;
            }
            currentNode = (bdd_nodes + currentNode->right);
        }
    }
    return -1;
}

void map_post_order_recursion(BDD_NODE *currentNode, unsigned char (*func)(unsigned char)) {
    if (currentNode->level == 0) {
        return;
    }

    map_post_order_recursion(bdd_nodes + currentNode->left, func);
    map_post_order_recursion(bdd_nodes + currentNode->right, func);
    int leftRes = currentNode->left;
    int rightRes = currentNode->right;

    if(leftRes <= 255) {
        leftRes = func(currentNode->left);
    }

    if(rightRes <= 255) {
        rightRes = func(currentNode->right);
    }
    bdd_lookup(currentNode->level, leftRes, rightRes);
}

BDD_NODE *bdd_map(BDD_NODE *node, unsigned char (*func)(unsigned char)) {
    map_post_order_recursion(node, func);

    return (bdd_nodes + availableIndex - 1);
}

BDD_NODE *bdd_rotate(BDD_NODE *node, int level) {
    // TO BE IMPLEMENTED
    return NULL;
}

void zoomTraversal(BDD_NODE *currentNode, int level, int factor) {
    if (level == 0) {
        return ;
    }

    if(currentNode->level != 0) {
        zoomTraversal((bdd_nodes + currentNode->left), (bdd_nodes + currentNode->left)->level, factor);
        zoomTraversal((bdd_nodes + currentNode->right), (bdd_nodes + currentNode->right)->level, factor);

        currentNode -> level += 2*factor;
    }
}

BDD_NODE *bdd_zoom(BDD_NODE *node, int level, int factor) {
    //Check factor range
    if((factor + actual_width) > BDD_LEVELS_MAX || (factor + actual_width) < 0) {
        return NULL;
    }

    zoomTraversal(node, (node->level), factor);
    //Add zoom out.
    return node;
}
