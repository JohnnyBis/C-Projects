/*
 * Imprimer: Command-line interface
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

#include "imprimer.h"
#include "conversions.h"
#include "sf_readline.h"

#define READ 0
#define WRITE 1
volatile sig_atomic_t signal_status = 0;
volatile sig_atomic_t job_id = -1;
static int CURR_P_SIZE = 0;

void job_listener();

typedef struct printer {
    int id;
    char* name;
    PRINTER_STATUS status;
    FILE_TYPE* file_type;
    int pgid;
} PRINTER;

typedef struct job {
    int id;
    JOB_STATUS status;
    char* eligible;
    char *file_name;
    FILE_TYPE* file_type;
    int pgid;
    char **eligible_printers;
    int total_eligible;
    time_t created_at;
    time_t updated_at;
} JOB;

PRINTER printers[MAX_PRINTERS];
JOB jobs[MAX_JOBS];

int num_of_args(char *args) {
    int argc = 0;
    if (strlen(args) <= 0) {
        return 0;
    }

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

void sigchld_main_handler(int sig) {
    signal_status = 1;
}

int find_available_printer(char *from_type, char **eligible_printers, int total_size, int elig_printer_size) {
    if(sizeof(printers) == 0) {
        return -1;
    }

    for(int i = 0; i < total_size; i++) {
        CONVERSION **res = find_conversion_path(from_type, printers[i].file_type->name);
        if(res != NULL) {
            if(eligible_printers != NULL) {
                for(int j = 0; j < elig_printer_size; j++) {
                    if(strcmp(printers[i].name, eligible_printers[j]) == 0 && (printers[i].status == PRINTER_IDLE)) {
                        return i;
                    }
                }
            }else{
                if(printers[i].status == PRINTER_IDLE) {
                    return i;
                }
            }
        }
    }
    return -1;
}

void new_process_pipeline(int input, int output, CONVERSION current_conversion) {
    pid_t pid_child = fork();

    if (pid_child == 0){
        if (input != 0){
            dup2(input, STDIN_FILENO);
            close(input);
        }

        if (output != 1){
            dup2(output, STDOUT_FILENO);
            close(output);
        }

        if(execvp(current_conversion.cmd_and_args[0], current_conversion.cmd_and_args) < 0){
            abort();
        }
        // return res;
    }
    // return pid_child;
}

void run_job(int pid, int jid, char *from, char *to) {
    int fd_current[2];

    pid_t P;

    signal(SIGCHLD, sigchld_main_handler);

    CONVERSION **res = find_conversion_path(from, to);

    int i = 0;
    while(res[i] != NULL) {
        i++;
    }

    int size = i + 1;
    char *path[size];

    for(int i = 0; res[i] != NULL; i++) {
        path[i] = res[i]->cmd_and_args[0];
    }
    path[i] = '\0';

    P = fork();
    if (P == 0) {
        //Master process.
        //Set pgid of master process.
        setpgid(0, 0);

        int output_printer_fd = imp_connect_to_printer(printers[pid].name, printers[pid].file_type->name, 0);
        int read_printer_fd = open(jobs[jid].file_name, O_RDONLY); //Read file descriptor
        if(output_printer_fd == -1){
            abort();
        }

        if(res[0] == NULL) {
            //Same type --> no conversion needed.
            char *no_conversion[] = {"bin/cat"};
            execvp(jobs[jid].file_name, no_conversion);
            exit(0);
        }else{
            int current_input_fd;

            if(read_printer_fd < 0) {
                abort();
            }

            if(dup2(read_printer_fd, STDIN_FILENO) < 0) { //Redirect STDIN to read_printer_fd
                abort();
            }
            current_input_fd = read_printer_fd;
            close(read_printer_fd);

            for(int i = 0; i < size - 2; i++) {
                if(size == 2) {
                    break;
                }

                if(pipe(fd_current) == -1) {
                    abort();
                }

                CONVERSION conversion = *res[i];
                new_process_pipeline(current_input_fd, fd_current[WRITE], conversion);
                close(fd_current[READ]);
                close(fd_current[WRITE]);
                current_input_fd = fd_current[WRITE];

            }

            if(current_input_fd != 0) {
                dup2(current_input_fd, STDIN_FILENO);
                close(current_input_fd);
            }

            CONVERSION last_conv = *res[size - 2];
            // printf("CONVERSION %s\n", last_conv.from->name);

            dup2(output_printer_fd, 1);
            close(output_printer_fd);
            execvp(last_conv.cmd_and_args[0], last_conv.cmd_and_args);

            int p;
            int status;
            while ((p = wait(&status)) > 0) {
                if(p == -1 || WIFSIGNALED(status)) {
                    abort();
                }
                // if(WIFSTOPPED(status) == 1) {
                //     sf_job_status(job->id, JOB_PAUSED);
                //     job->status = JOB_PAUSED;
                //     break;
                // }else if(WIFCONTINUED(status) == 1) {
                //     sf_job_status(job->id, JOB_RUNNING);
                //     job->status = JOB_RUNNING;
                //     break;
                // }
            }
        }
        exit(0);

        // signal(SIGCHLD, sigchld_handler);
        // if(wait_status == -1) {
        //     exit(1);
        // }
        // if(WIFSTOPPED(signal_status) == 1 && wait_status != -1) {
        //     sf_job_status(job.id, JOB_PAUSED);
        //     job.status = JOB_PAUSED;
        //     signal_status = -1;
        // }else if(WIFCONTINUED(signal_status) == 1 && wait_status != -1) {
        //     sf_job_status(job.id, JOB_RUNNING);
        //     job.status = JOB_RUNNING;
        // }
    }else if (P > 0) {
        //Main Parent process.
        sf_job_status(jobs[jid].id, JOB_RUNNING);
        sf_printer_status(printers[pid].name, PRINTER_BUSY);
        sf_job_started(jobs[jid].id, printers[pid].name, P, path);

        jobs[jid].status = JOB_RUNNING;
        jobs[jid].pgid = P;
        printers[pid].pgid = P;
        printers[pid].status = PRINTER_BUSY;

    }
}

void job_listener() {
    for(int i = 0; i < MAX_JOBS; i++) {
        if(jobs[i].file_name != NULL && jobs[i].status == JOB_CREATED) {
            char *from = jobs[i].file_type->name;
            int pid = find_available_printer(from, jobs[i].eligible_printers, CURR_P_SIZE, jobs[i].total_eligible);

            if(pid != -1 && pid <= MAX_PRINTERS) {
                char *to = printers[pid].file_type->name;
                run_job(pid, i, from, to);
            }
        }
    }
}

void *scan_jobs(char *from, char *to) {
    for(int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].status == JOB_CREATED) {
            CONVERSION **list_of_conversions = find_conversion_path(from, to);
            int i = 0;
            while(list_of_conversions[i] != NULL) {
                return NULL;
            }
            return NULL;
        }
    }
    return NULL;
}

void signal_callback(){
    // clock_t start_time = 0;
    if(signal_status == 1) {
        int status;
        pid_t pid;
        job_id = -1;
        while((pid = waitpid(-1, &status, 0))) {
            if(pid == -1) {
                break;
            }
            int counter = 0;
            while(counter < MAX_JOBS) {
                if(jobs[counter].pgid == pid) {
                    job_id = counter;
                    break;
                }
                counter++;
            }
            counter = 0;
            if(job_id == -1) {
                break;
            }


            int printer_id = -1;
            while(counter < MAX_PRINTERS) {
                if(printers[counter].pgid == pid) {
                    printer_id = counter;
                    break;
                }
                counter++;
            }

            if(printer_id == -1){
                return;
            }

            if(WIFSIGNALED(status) == 1) {
                // start_time = clock();

                sf_job_status(job_id, JOB_ABORTED);
                sf_job_aborted(job_id, status);
                jobs[job_id].updated_at = time(NULL);
                jobs[job_id].status = JOB_ABORTED;
                sf_job_status(job_id, JOB_DELETED);
                sf_job_deleted(job_id);
                JOB empty_job = {};
                jobs[job_id] = empty_job;
            }else if (WIFEXITED(status) == 1) {
                // start_time = clock();

                sf_job_status(job_id, JOB_FINISHED);
                sf_job_finished(job_id, status);

                jobs[job_id].updated_at = time(NULL);
                jobs[job_id].status = JOB_FINISHED;
                sf_job_status(job_id, JOB_DELETED);
                sf_job_deleted(job_id);
                JOB empty_job = {};
                jobs[job_id] = empty_job;
            }

            sf_printer_status(printers[printer_id].name, PRINTER_IDLE);
            printers[printer_id].status = PRINTER_IDLE;
            break;

        }
    }else {
        // if (jobs[job_id].status == JOB_FINISHED || jobs[job_id].status == JOB_ABORTED) {
        //     if(start_time == 0) {
        //         return;
        //     }
        //     while((int) ((clock() - start_time)/1000000) < 10);
        //     jobs[job_id].updated_at = time(NULL);
        //     sf_job_deleted(job_id);
        //     job_id = -1;
        // }
    }
    job_listener();
    signal_status = 0;
}

int interactive_mode(char *in, FILE *out) {
    // while(1) {
        char* input = in;
        if(in == NULL) {
            input = sf_readline("imp>");
        }
        sf_set_readline_signal_hook(signal_callback);

        int argc = num_of_args(input);
        char* arguments[argc];
        char* parameter = strtok(input, " ");

        int count = 0;
        while(parameter != NULL) {
            arguments[count] = parameter;
            parameter = strtok(NULL, " ");
            count++;
        }

        // if(out != stdout) {
        //     continue;
        // }
        if(strcmp(input, "") == 0) {
            return 0;
        }else if (strcmp(input, "quit") == 0) {
            if (argc != 0) {
                error_message(argc, 0, "quit");
                sf_cmd_error("argc count");
                // continue;
            }
            sf_cmd_ok();
            return -1;

        }else if(strcmp(input, "help") == 0) {
            if (argc != 0) {
                error_message(argc, 0, "help");
                sf_cmd_error("argc count");
                // continue;
            }
            printf("Commands are:\nhelp quit type printer conversion printers jobs print cancel disable enable pause resume\n");
            sf_cmd_ok();
        }else if(strncmp(input, "type", 4) == 0) {
            if (argc != 1) {
                error_message(argc, 1, "type");
                sf_cmd_error("argc count");
                return 0;
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
                return 0;
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
                return 0;
            }

            char * printer_name = arguments[1];
            FILE_TYPE *file_res = find_type(arguments[2]);
            if(file_res == NULL) {
                printf("Unknown file type: %s\n", arguments[2]);
                return 0;
            }
            if (CURR_P_SIZE == MAX_PRINTERS) {
                error_message(argc, 2, "printer");
                sf_cmd_error("argc count");
                return 0;
            }

            PRINTER new_printer = {.id = CURR_P_SIZE, .name = printer_name, .status = PRINTER_DISABLED, .file_type = file_res};
            sf_printer_defined(new_printer.name, new_printer.file_type->name);
            printers[CURR_P_SIZE] = new_printer;
            printf("PRINTER: id=%d, name=%s, type=%s, status=%s\n", new_printer.id, new_printer.name, new_printer.file_type->name, printer_status_names[new_printer.status]);
            CURR_P_SIZE++;

            job_listener();
            sf_cmd_ok();

        }else if (strncmp(input, "print", 5) == 0) {

            if (argc < 1) {
                error_message(argc, 1, "print");
                sf_cmd_error("argc count");
                return 0;
            }

            char *file_name = arguments[1];
            FILE_TYPE *file_type = get_file_type(file_name);
            if (file_type == NULL) {
                sf_cmd_error("print (file type)");
                return 0;
            }
            int jid = -1;
            for(int i = 0; i < MAX_JOBS; i++) {
                if(jobs[i].file_name == NULL) {
                    jid = i;
                    break;
                }
            }

            if(jid == -1) {
                printf("Exceeded maximum amount of jobs.\n");
                sf_cmd_error("MAX_JOBS EXCEEDED");
                return 0;
            }

            char **eligible_printers = NULL;

            if(argc > 1){
                eligible_printers = arguments + 2;
            }

            JOB new_job = {
                    .id = jid, .status = JOB_CREATED, .eligible = "ffffffff",
                    .file_name = file_name, .file_type = file_type,
                    .eligible_printers = eligible_printers,
                    .total_eligible = (argc - 1),
                    .created_at = time(NULL),
                    .updated_at = time(NULL)
            };

            sf_job_created(new_job.id, new_job.file_name, new_job.file_type->name);
            jobs[jid] = new_job;
            JOB current_job = jobs[jid];


            job_listener();

            // int printer_id = find_available_printer(current_job.file_type->name, current_job.eligible_printers, CURR_P_SIZE, current_job.total_eligible);

            // if (printer_id != -1) {
            //     run_job(printer_id, jid, current_job.file_type->name, printers[printer_id].file_type->name);
            // }
            char created_at_buffer[30];
            char updated_at_buffer[30];

            strftime(created_at_buffer, 26, "%Y-%m-%d %H:%M:%S", localtime(&current_job.created_at));
            strftime(updated_at_buffer, 26, "%Y-%m-%d %H:%M:%S", localtime(&current_job.updated_at));

            printf("JOB[%d]: type=%s, creation(%s), status(%s)=%s, eligible=%s, file=%s\n",
                    current_job.id,
                    current_job.file_type->name,
                    created_at_buffer,
                    updated_at_buffer,
                    job_status_names[current_job.status],
                    current_job.eligible,
                    current_job.file_name);

            sf_cmd_ok();
        }else if(strncmp(input, "conversion", 10) == 0) {
            if (argc < 3) {
                error_message(argc, 3, "conversions");
                sf_cmd_error("argc count");
                return 0;
            }
            int counter = argc - 2;
            char *conv_name[counter];
            conv_name[counter] = '\0';
            *conv_name = arguments[3];

            //Copies command and arguments to conv_name pointer array.
            for(int i = 0 ; i < counter; i++) {
                conv_name[i] = arguments[3+i];
            }

            define_conversion(arguments[1], arguments[2], conv_name);

            sf_cmd_ok();

        }else if(strncmp(input, "jobs", 4) == 0) {
            if (argc != 0) {
                error_message(argc, 0, "jobs");
                sf_cmd_error("argc count");
                return 0;
            }
            for(int i = 0; i < MAX_JOBS; i++) {
                if(jobs[i].file_name == NULL) {
                    break;
                }
                JOB current_job = jobs[i];
                char created_at_buffer[30];
                char updated_at_buffer[30];

                strftime(created_at_buffer, 26, "%Y-%m-%d %H:%M:%S", localtime(&current_job.created_at));
                strftime(updated_at_buffer, 26, "%Y-%m-%d %H:%M:%S", localtime(&current_job.updated_at));
                printf("JOB[%d]: type=%s, creation(%s), status(%s)=%s, eligible=%s, file=%s\n",
                    current_job.id,
                    current_job.file_type->name,
                    created_at_buffer,
                    updated_at_buffer,
                    job_status_names[current_job.status],
                    current_job.eligible,
                    current_job.file_name);
            }
            sf_cmd_ok();

        }else if(strncmp(input, "enable", 6) == 0) {
            if (argc != 1) {
                error_message(argc, 1, "enabled");
                sf_cmd_error("argc count");
                return 0;
            }

            int printerFound = 0;
            for(int i=0; i < MAX_PRINTERS; i++) {
                if(printers[i].name == NULL) {
                    break;
                }else if (strcmp(printers[i].name, arguments[1]) == 0) {
                    printers[i].status = PRINTER_IDLE;
                    sf_printer_status(printers[i].name, PRINTER_IDLE);
                    printerFound = 1;
                    break;
                }
            }

            if(!printerFound) {
                sf_cmd_error("printer not found");
            }else{
                job_listener();
                sf_cmd_ok();
            }

        }else if(strncmp(input, "disable", 7) == 0) {
            if (argc != 1) {
                error_message(argc, 1, "disable");
                sf_cmd_error("argc count");
                return 0;
            }
            int printerFound = 0;
            for(int i=0; i < MAX_PRINTERS; i++) {
                if(printers[i].name == NULL) {
                    break;
                }else if (strcmp(printers[i].name, arguments[1]) == 0) {
                    printers[i].status = PRINTER_DISABLED;
                    sf_printer_status(printers[i].name, PRINTER_DISABLED);
                    printerFound = 1;
                    sf_cmd_ok();
                    return 0;
                }
            }

            if(!printerFound) {
                sf_cmd_error("Printer not found");
            }
        }else if(strncmp(input, "resume", 6) == 0) {

            if (argc != 0) {
                error_message(argc, 1, "resume");
                sf_cmd_error("argc count");
                return 0;
            }else if(isdigit(*arguments[1]) == 0) {
                printf("Only input the job number.\n");
                sf_cmd_error("argument error type");
                return 0;
            }


            int i = (int) *arguments[1];
            if(jobs[i].file_name != NULL) {
                if(jobs[i].status == JOB_PAUSED) {
                    if(killpg(jobs[i].pgid, SIGCONT) == 0) {
                        sf_job_status(jobs[i].id, JOB_RUNNING);
                        jobs[i].status = JOB_RUNNING;
                    }
                }else{
                    sf_cmd_error("Job is not running.");
                }
            }else{
                sf_cmd_error("Job not found.");
            }

        }else if(strncmp(input, "pause", 6) == 0) {
            if (argc != 0) {
                error_message(argc, 1, "pause");
                sf_cmd_error("argc count");
                return 0;
            }else if(isdigit(*arguments[1]) == 0) {
                printf("Only input the job number.\n");
                sf_cmd_error("argument error type");
                return 0;
            }

            int i = atoi(arguments[1]);
            if(jobs[i].file_name != NULL) {
                if(jobs[i].status == JOB_RUNNING) {
                    if(killpg(jobs[i].pgid, SIGSTOP) == 0) {
                        sf_job_status(jobs[i].id, JOB_PAUSED);
                        jobs[i].status = JOB_PAUSED;
                    }
                }else{
                    sf_cmd_error("Job is not running.");
                }
                sf_cmd_ok();
            }else{
                sf_cmd_error("Job not found.");
            }

        }else if(strncmp(input, "cancel", 6) == 0) {

            if (argc != 0) {
                error_message(argc, 1, "cancel");
                sf_cmd_error("argc count");
                return 0;
            }else if(isdigit(*arguments[1]) == 0) {
                printf("Only input the job number.\n");
                sf_cmd_error("argument error type");
                return 0;
            }

            int i = (int) *arguments[1];
            if(jobs[i].file_name != NULL) {
                if(killpg(jobs[i].pgid, SIGTERM) == 0) {
                    sf_job_status(jobs[i].id, JOB_DELETED);
                    sf_job_aborted(jobs[i].id, SIGTERM);
                    sf_job_deleted(jobs[i].id);
                    sf_job_status(jobs[i].id, JOB_DELETED);
                    JOB empty_job = {};
                    jobs[i] = empty_job;
                }
                sf_cmd_ok();
            }else{
                sf_cmd_error("Job not found.");
            }
        }else{
            printf("Unrecognized command %s\n", arguments[0]);
            sf_cmd_error("CMD_ERROR [unrecognized command]");
        }

    // }
    return 0;
}

int run_cli(FILE *in, FILE *out)
{
    if(in != stdin) {
        int bytes;
        size_t size = 0;
        char *in_line;
        while ((bytes = getline(&in_line, &size, in)) > 0) {
            char *res = in_line;
            *(res + bytes - 1) = '\0';
            // printf("%s\n", in_line);
            if(interactive_mode(res, out) != 0) {
                return 1;
            }
        }
    }else{
        while (1) {
            if(interactive_mode(NULL, out) == -1) {
                return 0;
            }
        }
    }
    return 1;
}