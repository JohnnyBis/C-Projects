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

typedef struct printer {
	int id;
	char* name;
	PRINTER_STATUS status;
	FILE_TYPE* file_type;
} PRINTER;

typedef struct job {
	int id;
	char* creation;
	JOB_STATUS status;
	char* eligible;
	char *file_name;
	FILE_TYPE* file_type;
} JOB;

PRINTER printers[MAX_PRINTERS];
JOB jobs[MAX_JOBS];

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

FILE_TYPE *get_file_type(char *file_name) {
    char *file_extension = strchr(file_name, '.');
    if(file_extension) {
        file_extension++;
        return find_type(file_extension);
    }
    return NULL;
}

PRINTER *find_available_printer(char *file_name, FILE_TYPE *file_type, char **eligible_printers, int printer_size) {
    if(sizeof(printers) == 0) {
        return NULL;
    }

    for(int i = 0; i < MAX_PRINTERS; i++) {
        if(eligible_printers != NULL) {
            for(int j = 0; j < printer_size; i++) {
                if(strcmp(printers[i].name, eligible_printers[j]) == 0 && printers[i].status == PRINTER_IDLE) {
                    return &printers[i];
                }
            }
        }else{
            if(printers[i].status == PRINTER_IDLE) {
                return &printers[i];
            }
        }
    }

    return NULL;
}

int run_cli(FILE *in, FILE *out)
{
	static int uid = 0;
    static int jid = 0;
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

    	if(out != stdout) {
    		//TODO
    	}
    	if (strcmp(input, "quit") == 0) {
    		if (argc != 0) {
    			error_message(argc, 0, "quit");
    			sf_cmd_error("argc count");
    			continue;
    		}
    		sf_cmd_ok();
    		return -1;

    	}else if(strcmp(input, "help") == 0) {
    		if (argc != 0) {
    			error_message(argc, 0, "help");
    			sf_cmd_error("argc count");
    			continue;
    		}
    		printf("Commands are:\nhelp quit type printer conversion printers jobs print cancel disable enable pause resume\n");
    		sf_cmd_ok();
    	}else if(strncmp(input, "type", 4) == 0) {
    		if (argc != 1) {
    			error_message(argc, 1, "type");
    			sf_cmd_error("argc count");
    			continue;
    		}
    		FILE_TYPE *res = define_type(arguments[1]);

    		if (res == NULL){
    			printf("New file could not be defined.");
    		}

    		sf_cmd_ok();

    	}else if(strncmp(input, "printers", 8) == 0) {
    		if (argc != 0) {
    			error_message(argc, 0, "conversions");
    			sf_cmd_error("argc count");
    			continue;
    		}

    		for(int i=0; i < MAX_PRINTERS; i++) {
    			if(printers[i].name == NULL) {
    				break;
    			}
    			printf("PRINTER: id=%d, name=%s, type=%s, status=%s\n", printers[i].id, printers[i].name, printers[i].file_type->name, printer_status_names[printers[i].status]);
    		}

    		sf_cmd_ok();
    	}else if(strncmp(input, "printer", 7) == 0) {

    		if (argc != 2) {
    			error_message(argc, 2, "printer");
    			sf_cmd_error("argc count");
    			continue;
    		}

    		char * printer_name = arguments[1];
    		FILE_TYPE *file_res = find_type(arguments[2]);
    		if(file_res == NULL) {
    			printf("Unknown file type: %s\n", arguments[2]);
    			continue;
    		}
    		if (uid == MAX_PRINTERS) {
    			error_message(argc, 2, "printer");
    			sf_cmd_error("argc count");
    			continue;
    		}

    		PRINTER new_printer = {.id = uid, .name = printer_name, .status = PRINTER_DISABLED, .file_type = file_res};
    		sf_printer_defined(new_printer.name, new_printer.file_type->name);
    		printers[uid] = new_printer;
    		uid++;
    		printf("PRINTER: id=%d, name=%s, type=%s, status=%s\n", new_printer.id, new_printer.name, printer_status_names[new_printer.status], new_printer.file_type->name);
    		sf_cmd_ok();

    	}else if (strncmp(input, "print", 5) == 0) {

            if (argc == 0) {
                error_message(argc, 0, "print");
                sf_cmd_error("argc count");
                continue;
            }else if (jid > MAX_JOBS) {
                printf("Exceeded maximum amount of jobs.\n");
                sf_cmd_error("MAX_JOBS EXCEEDED");
                continue;
            }

            char *file_name = arguments[1];
            FILE_TYPE *file_type = get_file_type(file_name);
            printf("HELLO %p\n", file_type);
            if (file_type == NULL) {
                sf_cmd_error("print (file type)");
                continue;
            }
            // char **eligible_printers = NULL;

            // if (argc >= 2) {
            //     eligible_printers = arguments + 2;
            // }

            // PRINTER *printer_res = find_available_printer(file_name, file_type, eligible_printers, argc);
            // if (printer_res == NULL) {
            //     sf_cmd_error("print");
            //     continue;
            // }

            JOB new_job = {.id = jid, .creation = "", .status = JOB_CREATED, .eligible = "ffffffff", .file_name = file_name, .file_type = file_type};
            sf_job_created(new_job.id, new_job.file_name, new_job.file_type->name);
            jobs[jid] = new_job;
            JOB current_job = jobs[jid];
            printf("JOB[%d]: type=%s, creation(%s), status(%s)=%s, eligible=%s, file=%s\n",
                    current_job.id,
                    current_job.file_type->name,
                    current_job.creation,
                    current_job.creation,
                    job_status_names[current_job.status],
                    current_job.eligible,
                    current_job.file_name);
            jid++;

            sf_cmd_ok();
        }else if(strncmp(input, "conversion", 10) == 0) {
    		if (argc != 3) {
    			error_message(argc, 3, "conversions");
    			sf_cmd_error("argc count");
    			continue;
    		}
    		sf_cmd_ok();

    	}else if(strncmp(input, "jobs", 4) == 0) {
    		if (argc != 0) {
    			error_message(argc, 0, "jobs");
    			sf_cmd_error("argc count");
    			continue;
    		}
            for(int i = 0; i < MAX_JOBS; i++) {
                if(jobs[i].file_name == NULL) {
                    break;
                }
                JOB current_job = jobs[i];
                printf("JOB[%d]: type=%s, creation(%s), status(%s)=%s, eligible=%s, file=%s\n",
                    current_job.id,
                    current_job.file_type->name,
                    current_job.creation,
                    current_job.creation,
                    job_status_names[current_job.status],
                    current_job.eligible,
                    current_job.file_name);
            }
    		sf_cmd_ok();

    	}else if(strncmp(input, "enable", 6) == 0) {
    		if (argc != 1) {
    			error_message(argc, 1, "enabled");
    			sf_cmd_error("argc count");
    			continue;
    		}

    		for(int i=0; i < MAX_PRINTERS; i++) {
    			if(printers[i].name == NULL) {
    				break;
    			}else if (printers[i].name == arguments[1]) {
    				printers[i].status = PRINTER_IDLE;
    				sf_printer_status(printers[i].name, PRINTER_IDLE);
    				sf_cmd_ok();
    				break;
    			}
    		}
    		sf_cmd_error("enable (no printer)");

    	}else if(strncmp(input, "disable", 7) == 0) {
    		if (argc != 1) {
    			error_message(argc, 1, "disable");
    			sf_cmd_error("argc count");
    			continue;
    		}

    		for(int i=0; i < MAX_PRINTERS; i++) {
    			if(printers[i].name == NULL) {
    				break;
    			}else if (printers[i].name == arguments[1]) {
    				printers[i].status = PRINTER_DISABLED;
    				sf_printer_status(printers[i].name, PRINTER_DISABLED);
    				sf_cmd_ok();
    				break;
    			}
    		}
    		sf_cmd_error("enable (no printer)");
    	}
    }
    return 0;
    //abort();
}


// FILE_TYPE *define_type(char *name) {
// 	FILE_TYPE *new_file = malloc(sizeof(FILE_TYPE));
// 	new_file->name = name;
// 	new_file->index = uid;
// 	return new_file;
// }
