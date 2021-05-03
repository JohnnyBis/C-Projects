#include "client_registry.h"
#include "client.h"
#include <pthread.h>
#include <unistd.h>

typedef struct client_node CLIENT_NODE;

typedef struct client_node {
	CLIENT *client;
	CLIENT_NODE *next;
} CLIENT_NODE;

typedef struct client_registry {
	CLIENT_NODE *head;
	int total;
	pthread_mutex_t lock;
} CLIENT_REGISTRY;

CLIENT_REGISTRY *creg_init() {
	CLIENT_REGISTRY *client_reg = malloc(sizeof(CLIENT_REGISTRY));
	if(client_reg == NULL) {
		return NULL;
	}

	if(pthread_mutex_init(&client_reg->lock, NULL) < 0) {
		free(client_reg);
		return NULL;
	}

	client_reg->head = NULL;
	client_reg->total = 0;

	return client_reg;
}

void creg_fini(CLIENT_REGISTRY *cr) {

	if(pthread_mutex_lock(&cr->lock) < 0) {
		return;
	}

	if(cr == NULL) {
		return;
	}

	free(cr);

	if(pthread_mutex_unlock(&cr->lock) < 0) {
		return;
	}

}

CLIENT *creg_register(CLIENT_REGISTRY *cr, int fd) {
	if(pthread_mutex_lock(&cr->lock) < 0) {
		return NULL;
	}

	if(cr == NULL) {
		return NULL;
	}

	CLIENT *new_client = client_create(cr, fd);

	if(new_client == NULL) {
		return NULL;
	}

	if(client_ref(new_client, "pointer retained by the registr") == NULL) {
		return NULL;
	} //Total refs = 2

	CLIENT_NODE *node_head = cr->head;
	CLIENT_NODE *prev_node = cr->head;
	while(node_head != NULL) {
		prev_node = node_head;
		node_head = node_head->next;
	}

	CLIENT_NODE *new_node = malloc(sizeof(CLIENT_NODE));
	new_node->client = new_client;
	new_node->next = NULL;

	if(cr->total == 0) {
		cr->head = new_node;
	}else{
		node_head = new_node;
		prev_node->next = node_head;
	}

	cr->total += 1;

	if(pthread_mutex_unlock(&cr->lock) < 0) {
		return NULL;
	}

	return new_client;
}

int creg_unregister(CLIENT_REGISTRY *cr, CLIENT *client) {

	if(pthread_mutex_lock(&cr->lock) < 0) {
		return -1;
	}

	CLIENT_NODE *current_node = cr->head;
	CLIENT_NODE *prev_node = NULL;

	while(current_node != NULL) {
		if(client_get_fd(current_node->client) == client_get_fd(client)) {
			//If registered client is found.
			client_unref(client, "Unreference to account for the pointer being discarded");
			CLIENT_NODE *next_node = current_node->next;
			prev_node->next = next_node;

			if(pthread_mutex_unlock(&cr->lock) < 0) {
				return -1;
			}
			//call creg shutdown
			return 0;
		}
		prev_node = current_node;
		current_node = current_node->next;
	}

	if(pthread_mutex_unlock(&cr->lock) < 0) {
		return -1;
	}

	return -1; //Client not found.
}

CLIENT **creg_all_clients(CLIENT_REGISTRY *cr) {

	if(pthread_mutex_lock(&cr->lock) < 0) {
		return NULL;
	}

	CLIENT **active_clients = malloc(sizeof(cr->total) * sizeof(CLIENT *));

	CLIENT_NODE *current_node = cr->head;
	int i = 0;
	while(current_node != NULL && i <= MAX_CLIENTS) {
		active_clients[i] = current_node->client;
		client_ref(current_node->client, "Decrease reference to account for the pointer stored in the array.");
		current_node = current_node->next;
		i++;
	}

	active_clients[i] = NULL;

	if(pthread_mutex_unlock(&cr->lock) < 0) {
		return NULL;
	}

	return active_clients;
}

void creg_shutdown_all(CLIENT_REGISTRY *cr) {
	//TODO
}