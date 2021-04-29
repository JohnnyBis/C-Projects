#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "debug.h"
#include "server.h"
#include "globals.h"
#define LOCALHOST "127.0.0.1"

static void terminate(int);
int registerSocket(void);
void sighup_handler(int);

/*
 * "Charla" chat server.
 *
 * Usage: charla <port>
 */

int main(int argc, char* argv[]){
    // Option processing should be performed here.
    // Option '-p <port>' is required in order to specify the port number
    // on which the server should listen.
    int opt;
    int req_port = 0;

    while ((opt = getopt(argc, argv, "p:")) != -1) {
        switch (opt) {
        case 'p':
            req_port = atoi(optarg);
            break;
        default:
            fprintf(stderr, "Usage: %s -p <port>\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    // Perform required initializations of the client_registry and
    // player_registry.
    if(req_port == 0) {
        exit(EXIT_FAILURE);
    }

    int c_socket;

    user_registry = ureg_init();
    client_registry = creg_init();

    struct sigaction action;
    action.sa_handler = sighup_handler;
    sigaction(SIGHUP, &action, NULL);

    int s_socket = registerSocket();
    if(s_socket == -1) {
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_setup;
    server_setup.sin_family = AF_INET;
    server_setup.sin_port = htons(req_port);
    server_setup.sin_addr.s_addr = inet_addr(LOCALHOST);

    if(bind(s_socket, (struct sockaddr*) &server_setup, sizeof(server_setup)) < 0){
        printf("Socket could not be binded to the port.\n");
        exit(EXIT_FAILURE);
    }

    if(listen(s_socket, 64) < 0){
        printf("Socket listening error.\n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr client_addr;
    socklen_t client_size = sizeof(client_addr);

    pthread_t thread_id;
    // int i = 0;
    while((c_socket = accept(s_socket, (struct sockaddr*)&client_addr, &client_size)) > 0 ){
        printf("Connection accepted...\n");
        int *connection_fd = malloc(sizeof(int));
        *connection_fd = c_socket;
        if(pthread_create(&thread_id, NULL, chla_client_service, connection_fd) < 0) {
            perror("could not create thread");
            exit(EXIT_FAILURE);
        }
    }

    if(c_socket < 0) {
        printf("Could not accept a connection.\n");
        exit(EXIT_FAILURE);
    }
    // TODO: Set up the server socket and enter a loop to accept connections
    // on this socket.  For each connection, a thread should be started to
    // run function charla_client_service().  In addition, you should install
    // a SIGHUP handler, so that receipt of SIGHUP will perform a clean
    // shutdown of the server.
    // chla_client_service(&c_socket);
    // fprintf(stderr, "You have to finish implementing main() "
	   //  "before the server will function.\n");
    // Closing the socket:

    terminate(EXIT_FAILURE);
}

//Socket is registered. Function returns -1 if registration fails.
//Otherwise, the socket fd is returned.
int registerSocket() {
    int new_socket;
    new_socket = socket(AF_INET, SOCK_STREAM, 0);
    return new_socket;
}

void sighup_handler(int sig) {
    terminate(EXIT_SUCCESS);
}

/*
 * Function called to cleanly shut down the server.
 */
static void terminate(int status) {
    // Shut down all existing client connections.
    // This will trigger the eventual termination of service threads.
    creg_shutdown_all(client_registry);

    // Finalize modules.
    creg_fini(client_registry);
    ureg_fini(user_registry);

    debug("%ld: Server terminating", pthread_self());
    exit(status);
}
