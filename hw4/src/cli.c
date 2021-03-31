/*
 * Imprimer: Command-line interface
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "imprimer.h"
#include "conversions.h"
#include "sf_readline.h"

int num_of_args(char *args) {
	int argc = 0;

	for (int i = 0; i < strlen(args) - 1; i++) {
		if (args[i] == ' ' && isalpha(args[i+1]) != 0){
			argc++;
		}
	}

	return argc;
}

void error_message(int argc, int req, char *command) {
	printf("Wrong number of args (given: %d, required: %d) for CLI command '%s'\n", argc, req, command);
}

int run_cli(FILE *in, FILE *out)
{
    while(1) {
    	char* input = sf_readline("imp>");
    	int argc = num_of_args(input);
    	char* arguments[argc];
    	char* parameter = strtok(input, " ");

    	int count = 0;
    	while(parameter != NULL) {
    		arguments[count] = parameter;
    		parameter = strtok(NULL, " ");
    		count++;
  		}

  		//printf("%p, %p", out, stdout);

    	if(out != stdout) {
    		//TODO
    	}
    	if (strcmp(input, "quit") == 0) {
    		return -1;

    	}else if(strcmp(input, "help") == 0) {
    		printf("Commands are:\nhelp quit type printer conversion printers jobs print cancel disable enable pause resume\n");

    	}else if(strncmp(input, "type", 4) == 0) {
    		if (argc != 1) {
    			error_message(argc, 1, "type");
    			continue;
    		}
    		if (define_type(arguments[0]) == NULL){
    			printf("New file could not be defined.");
    		}

    	}else if(strncmp(input, "printer", 7) == 0) {
    		if (argc != 2) {
    			error_message(argc, 2, "printer");
    			continue;
    		}
    		FILE_TYPE *file_res = find_type(arguments[2]);

    		if(file_res == NULL) {
    			printf("Unknown file type: %s\n", arguments[2]);
    			continue;
    		}


    	}

    }
    return 0;
    //abort();
}

//int uid = 1;


// FILE_TYPE *define_type(char *name) {
// 	FILE_TYPE *new_file = malloc(sizeof(FILE_TYPE));
// 	new_file->name = name;
// 	new_file->index = uid;
// 	return new_file;
// }
