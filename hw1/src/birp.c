/*
 * BIRP: Binary decision diagram Image RePresentation
 */

#include "image.h"
#include "bdd.h"
#include "const.h"
#include "debug.h"

int pgm_to_birp(FILE *in, FILE *out) {

    if(in == NULL || out == NULL) {
        return -1;
    }

    int width;
    int height;

    if(img_read_pgm(in, &width, &height, raster_data, RASTER_SIZE_MAX) == 0){

        BDD_NODE *result = bdd_from_raster(width, height, raster_data);

        if(result != NULL) {
            return img_write_birp(result, width, height, out);
        }

        return -1;
    }

    return -1;
}

int birp_to_pgm(FILE *in, FILE *out) {

    if(in == NULL || out == NULL) {
        return -1;
    }

    int width;
    int height;
    BDD_NODE *result;

    if((result = img_read_birp(in, &width, &height)) != NULL){
        bdd_to_raster(result, width, height, raster_data);
        return img_write_pgm(raster_data, width, height, out);
    }

    return -1;
}
int treshold;
unsigned char filter(unsigned char x) {
    if (x <= treshold) {
        return 255;
    }
    return 0;
}

unsigned char complement(unsigned char x) {
    return 255 - x;
}

int birp_to_birp(FILE *in, FILE *out) {
    BDD_NODE *result;

    int width;
    int height;

    if((result = img_read_birp(stdin, &width, &height)) != NULL) {
        int transformationType = (global_options & 0xF00) >> 8;
        int factor = (global_options & 0xFF0000) >> 16;

        if(transformationType == 0) {
            //No transformation.
        }else if (transformationType == 1) {
            //Apply complement filter.
            BDD_NODE *mapRes = bdd_map(result, complement);
            img_write_birp(mapRes, width, height, out);

        }else if (transformationType == 2) {
            //Apply treshold filter.
            treshold = factor;
            BDD_NODE *mapRes = bdd_map(result, filter);
            img_write_birp(mapRes, width, height, out);

        }else if(transformationType == 3) {
            //Apply zoom.
            if(factor == 0) {
                // bdd_map(result, func());
            }else if(factor < 240) {
                //-Z, zoom in used
                BDD_NODE *zoomRes = bdd_zoom(result, 0, factor);
                img_write_birp(zoomRes, width, height, out);

            }else{
                //-z, zoom out used
                int factorTwosCompl = (factor ^ 0xFF) + 1;
                BDD_NODE *res = bdd_zoom(result, 0, factorTwosCompl);
                img_write_birp(res, width, height, out);

            }
        }else{
            return -1;
        }
        return 0;
    }

    return -1;
}

int pgm_to_ascii(FILE *in, FILE *out) {

    if(in == NULL || out == NULL) {
        return -1;
    }

    int width;
    int height;

    if(img_read_pgm(in, &width, &height, raster_data, RASTER_SIZE_MAX) == 0){
        unsigned char *p = NULL;
        int heightCntr = 0;
        for(p = raster_data; p < raster_data + width * height; ++p) {
            int value = *p;

            if(heightCntr == width){
                printf("%c", '\n');
                heightCntr = 0;
            }

            if(value >= 0 && value <= 63) {
                printf("%c",' ');
            }else if(value >= 64 && value <= 127) {
                printf("%c",'.');
            }else if(value >= 128 && value <= 191) {
                printf("%c",'*');
            }else if(value >= 192 && value <= 255) {
                printf("%c",'@');
            }else{
                return -1;
            }
            ++heightCntr;
        }
        return 0;
    }
    return -1;
}

int birp_to_ascii(FILE *in, FILE *out) {
    if(in == NULL || out == NULL) {
        return -1;
    }

    int width;
    int height;

    BDD_NODE *result;
    if((result = img_read_birp(in, &width, &height)) != NULL) {
        int heightCntr = 0;
        for(int i = 0; i < (width * height); i ++) {
            int colorResult = bdd_apply(result, i / width, i % height);
            if(heightCntr == width){
                printf("%c", '\n');
                heightCntr = 0;
            }

            if(colorResult >= 0 && colorResult <= 63) {
                printf("%c",' ');
            }else if(colorResult >= 64 && colorResult <= 127) {
                printf("%c",'.');
            }else if(colorResult >= 128 && colorResult <= 191) {
                printf("%c",'*');
            }else if(colorResult >= 192 && colorResult <= 255) {
                printf("%c",'@');
            }else{
                return -1;
            }
            ++heightCntr;
        }
        return 0;
    }
    return -1;
}

int isEqual(char *stringOne, char *stringTwo) {
    int i = 0;
    while(*(stringOne + i) != '\0' || *(stringTwo + i) != '\0') {
        if(*(stringOne + i) != *(stringTwo + i)) {
            return -1;
        }
        i++;
    }
    return 0;
}

int charToInt(const char *s) {
  int num = 0;

  while(*s != '\0') {
    num = ((*s) - '0') + (num * 10);
    s++;
  }
  return num;
}


/**
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the
 * program, returning 0 if validation succeeds and -1 if validation fails.
 * Upon successful return, the various options that were specifed will be
 * encoded in the global variable 'global_options', where it will be
 * accessible elsewhere int the program.  For details of the required
 * encoding, see the assignment handout.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return 0 if validation succeeds and -1 if validation fails.
 * @modifies global variable "global_options" to contain an encoded representation
 * of the selected program options.
 */
int validargs(int argc, char **argv) {
    char **ptr;
    ptr = argv;
    ++ptr;
    ++*ptr;


    if (argc == 0) {
        return -1;
    }

    if (**ptr == 'h') {
        global_options = HELP_OPTION;
        return 0;
    }

    int i = 1;
    int isInputBirp = 1;
    int isOutputBirp = 1;
    unsigned char pgmValue = 0x1;
    unsigned char birpValue = 0x2;
    unsigned char asciiValue = 0x3;

    //Checks required specifications (-o and -i)
    while(i < argc - 1 && i < 5) {

        if (**ptr != 'i' && **ptr != 'o') {
            break;
        }

        if (**ptr == 'i') {
            ++ptr;
            ++i;

            char *value = *ptr;

            if(isEqual(value, "pgm") == 0) {
                global_options |= pgmValue;
            }else if(isEqual(value, "birp") == 0) {
                global_options |= birpValue;
            }else{
                return -1;
            }
            isInputBirp = 0;
        }

        if (**ptr == 'o') {
            ++ptr;
            ++i;

            char *value = *ptr;

            if(isEqual(value, "pgm") == 0) {
                global_options |= (pgmValue << 4);
            }else if(isEqual(value,"birp") == 0) {
                global_options |= (birpValue << 4);
            }else if(isEqual(value, "ascii") == 0){
                global_options |= (asciiValue << 4);
            }else{
                return -1;
            }

            isOutputBirp = 0;
        }

        ++ptr;
        ++*ptr;
        ++i;
    }

    if(isInputBirp) {
        global_options |= birpValue;
    }

    if(isOutputBirp) {
        global_options |= (birpValue << 4);
    }

    if(i == argc) {
        return 0;
    }

    char **temp = ptr;
    ++temp;

    //Checks optional values.
    switch(**ptr) {
        case 'n' :
            if(argc >= 3) {
                return -1;
            }
            global_options |= (0x1 << 8);
            break;
        case 'r' :
            if(argc >= 3) {
                return -1;
            }
            global_options |= (0x4 << 8);
            break;
        case 't' :
            if(argc < 3) {
                return -1;
            }
            if(charToInt(*temp) >= 0 && charToInt(*temp) <= 255) {
                global_options |= (0x2 << 8);
                global_options |= (charToInt(*temp) << 16);
            }else{
                return -1;
            }
            break;
        case 'z' :
            if(argc < 3) {
                return -1;
            }
            if(charToInt(*temp) >= 0 && charToInt(*temp) <= 16) {
                global_options |= (0x3 << 8);
                global_options |= (charToInt(*temp) * -1 << 16);
            }else{
                return -1;
            }
            break;
        case 'Z' :
            if(argc < 3) {
                return -1;
            }
            if(charToInt(*temp) >= 0 && charToInt(*temp) <= 16) {
                global_options |= (0x3 << 8);
                global_options |= (charToInt(*temp) << 16);
            }else{
                return -1;
            }
            break;
        default:
            return -1;
    }

    return 0;
}