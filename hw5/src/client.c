#include "client_registry.h"
#include "client.h"
#include "string.h"
#include <pthread.h>
#include <time.h>

typedef struct client {
	USER *user;
	MAILBOX *mailbox;
	int fd;
	int reference;
	char *why;
	int status;
	pthread_mutex_t lock;
} CLIENT;

CLIENT *client_create(CLIENT_REGISTRY *creg, int fd) {
	CLIENT *client = malloc(sizeof(CLIENT));
	if(client == NULL) {
		return NULL;
	}
	char *why = malloc(sizeof(char));
	if(why == NULL) {
		return NULL;
	}

	if(pthread_mutex_init(&client->lock, NULL) < 0) {
		free(why);
		free(client);
		return NULL;
	}

	strcpy(why, "");
	client->why = why;
	client->fd = fd;
	client->reference = 1;
	client->mailbox = NULL;
	client->user = NULL;
	client->status = 0; //Logged out

	printf("CLIENT CREATE\n");

	return client;
}

CLIENT *client_ref(CLIENT *client, char *why) {

	if(client == NULL) {
		return NULL;
	}

	if(pthread_mutex_lock(&client->lock) < 0) {
		return NULL;
	}
	printf("TOTAL REF: %d\n", client->reference);

	client->reference += 1;
	client->why = why;

	if(pthread_mutex_unlock(&client->lock) < 0) {
		return NULL;
	}

	printf("CLIENT REF %s\n", user_get_handle(client->user));

	return client;
}

void client_unref(CLIENT *client, char *why) {
	if(client == NULL) {
		return;
	}

	if(pthread_mutex_lock(&client->lock) < 0) {
		return;
	}
	printf("TOTAL REF: %d\n", client->reference);
	client->reference -= 1;
	client->why = why;

	if(client->reference == 0) {
		printf("DONE\n");
		free(client);
	}

	if(pthread_mutex_unlock(&client->lock) < 0) {
		return;
	}
	printf("UNREF %s\n", user_get_handle(client->user));
}

int client_login(CLIENT *client, char *handle) {
	if(client == NULL || client->status == 1) {
		return -1;
	}

	if(pthread_mutex_lock(&client->lock) < 0) {
		return -1;
	}

	USER *user = user_create(handle);
	if(user == NULL) {
		return -1;
	}

	MAILBOX *mailbox = mb_init(handle);
	if(mailbox == NULL) {
		return -1;
	}

	client->status = 1;
	client->user = user;
	client->mailbox = mailbox;

	if(pthread_mutex_unlock(&client->lock) < 0) {
		return -1;
	}

	return 0;
}

int client_logout(CLIENT *client) {
	if(client == NULL || client->status == 0) {
		return -1;
	}

	if(pthread_mutex_lock(&client->lock) < 0) {
		return -1;
	}

	free(client->mailbox);
	free(client->user);
	client->mailbox = NULL;
	client->user = NULL;

	if(pthread_mutex_unlock(&client->lock) < 0) {
		return -1;
	}

	return 0;
}

USER *client_get_user(CLIENT *client, int no_ref) {
	if(client == NULL) {
		return NULL;
	}

	if(pthread_mutex_lock(&client->lock) < 0) {
		return NULL;
	}

	if(no_ref == 0) {
		client->reference += 1;
		printf("TOTAL REF: %d\n", client->reference);
	}

	if(pthread_mutex_unlock(&client->lock) < 0) {
		return NULL;
	}
	printf("GET USER %s\n", user_get_handle(client->user));
	return client->user;
}

MAILBOX *client_get_mailbox(CLIENT *client, int no_ref) {
	if(client == NULL) {
		return NULL;
	}

	if(pthread_mutex_lock(&client->lock) < 0) {
		return NULL;
	}

	if(no_ref == 0) {
		client->reference += 1;
	}

	if(pthread_mutex_unlock(&client->lock) < 0) {
		return NULL;
	}
	printf("CLIENT MAIL\n");
	return client->mailbox;
}

int client_get_fd(CLIENT *client) {
	if(client == NULL) {
		return -1;
	}

	return client->fd;
}

int client_send_packet(CLIENT *user, CHLA_PACKET_HEADER *pkt, void *data) {
	if(pthread_mutex_lock(&user->lock) < 0) {
		return -1;
	}
	int psp_result = proto_send_packet(user->fd, pkt, data);
	printf("--%s\n", user_get_handle(user->user));

	if(pthread_mutex_unlock(&user->lock) < 0) {
		return -1;
	}
	printf("SEND PACKET\n");
	return psp_result;
}

int client_send_ack(CLIENT *client, uint32_t msgid, void *data, size_t datalen) {

	CHLA_PACKET_HEADER *cp_header = malloc(sizeof(CHLA_PACKET_HEADER));
	if(cp_header == NULL) {
		return -1;
	}
	cp_header->type = CHLA_ACK_PKT;
	cp_header->payload_length = htonl(datalen);
	cp_header->msgid = htonl(msgid);
	cp_header->timestamp_sec = time(0);
	cp_header->timestamp_nsec = time(0); //Change to nanoseconds.
	printf("SEND ACK\n");

	return client_send_packet(client, cp_header, data);
}

int client_send_nack(CLIENT *client, uint32_t msgid) {
	CHLA_PACKET_HEADER *cp_header = malloc(sizeof(CHLA_PACKET_HEADER));
	if(cp_header == NULL) {
		return -1;
	}
	cp_header->type = CHLA_NACK_PKT;
	cp_header->payload_length = htonl(0);
	cp_header->msgid = htonl(msgid);
	cp_header->timestamp_sec = time(0);
	cp_header->timestamp_nsec = time(0); //Change to nanoseconds.

	return client_send_packet(client, cp_header, NULL);
}