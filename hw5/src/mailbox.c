#include "mailbox.h"
#include "string.h"
#include <pthread.h>
#include <semaphore.h>
#include "debug.h"

typedef struct mailbox_node MAILBOX_NODE;

typedef struct mailbox_node {
	MAILBOX_NODE *next;
	MAILBOX_ENTRY *entry;
} MAILBOX_NODE;

typedef struct mailbox {
	MAILBOX_DISCARD_HOOK *disc_hook;
	MAILBOX_NODE *front;
	MAILBOX_NODE *rear;
	int count;
	int reference;
	int is_defunct;
	char *why;
	char *handle;
	pthread_mutex_t lock;
	sem_t sem;
} MAILBOX;

MAILBOX *mb_init(char *handle) {
	if(handle == NULL) {
		return NULL;
	}

	MAILBOX *mailbox = malloc(sizeof(MAILBOX));

	if(mailbox == NULL) {
		return NULL;
	}

	char *why = malloc(sizeof(char));
	if(why == NULL) {
		return NULL;
	}

	char *new_handle = malloc(sizeof(handle));
	if(new_handle == NULL) {
		return NULL;
	}

	strcpy(new_handle, handle);

	if(pthread_mutex_init(&mailbox->lock, NULL) < 0) {
		free(mailbox);
		return NULL;
	}

	if(sem_init(&mailbox->sem, 0, 0) < 0) {
		free(mailbox);
		return NULL;
	}

	strcpy(why, "");
	mailbox->count = 0;
	mailbox->front = NULL;
	mailbox->rear = NULL;
	mailbox->why = why;
	mailbox->handle = new_handle;

	return mailbox;
}

void mb_set_discard_hook(MAILBOX *mb, MAILBOX_DISCARD_HOOK *new_hook) {
	if(mb == NULL) {
		return;
	}

	mb->disc_hook = new_hook;
}

void mb_ref(MAILBOX *mb, char *why) {
	if(mb == NULL) {
		return;
	}

	// if(pthread_mutex_lock(&mb->lock) < 0) {
	// 	return;
	// }
	debug("INCREASE\n");
	mb->reference += 1;
	mb->why = why;

	// if(pthread_mutex_unlock(&mb->lock) < 0) {
	// 	return;
	// }
}

void mb_unref(MAILBOX *mb, char *why) {
	if(mb == NULL) {
		return;
	}

	// if(pthread_mutex_lock(&mb->lock) < 0) {
	// 	return;
	// }
	debug("DECREASE\n");
	printf("INFO: %d\n", mb->reference);

	mb->reference -= 1;

	if(mb->reference == 0) {
		mb_shutdown(mb);
	}

	mb->why = why;

	// if(pthread_mutex_unlock(&mb->lock) < 0) {
	// 	return;
	// }
}

void mb_shutdown(MAILBOX *mb) {
	if(mb == NULL) {
		return;
	}
	mb->is_defunct = 1;
	sem_post(&mb->sem);
}

char *mb_get_handle(MAILBOX *mb) {
	if(mb == NULL) {
		return NULL;
	}

	return mb->handle;
}

void mb_add_message(MAILBOX *mb, int msgid, MAILBOX *from, void *body, int length) {
	if(mb == NULL) {
		return;
	}

	if(pthread_mutex_lock(&mb->lock) < 0) {
		return;
	}
	debug("ADD MESSAGE\n");

	MESSAGE *message = malloc(sizeof(MESSAGE));
	message->msgid = msgid;
	message->from = from;
	message->body = body;
	message->length = length;

	if(from != mb) {
		mb->reference += 1;
		//TODO: CHANGE TO MB_REF
	}

	MAILBOX_NODE *mailbox_node = malloc(sizeof(MAILBOX_NODE));
	MAILBOX_ENTRY *entry = malloc(sizeof(MAILBOX_ENTRY));
	entry->type = MESSAGE_ENTRY_TYPE;
	entry->content.message = *message;
	mailbox_node->entry = entry;
	mailbox_node->next = NULL;

	if(mb->count == 0) {
		mb->front = mb->rear = mailbox_node;
		mb->count += 1;
		pthread_mutex_unlock(&mb->lock);
		sem_post(&mb->sem);
		return;
	}

	mb->rear->next = mailbox_node;
	mb->rear = mailbox_node;
	mb->count += 1;

	pthread_mutex_unlock(&mb->lock);

	sem_post(&mb->sem);
}

void mb_add_notice(MAILBOX *mb, NOTICE_TYPE ntype, int msgid) {
	if(mb == NULL) {
		return;
	}

	if(pthread_mutex_lock(&mb->lock) < 0) {
		return;
	}

	NOTICE *notice = malloc(sizeof(NOTICE));
	notice->msgid = msgid;
	notice->type = ntype;

	MAILBOX_NODE *mailbox_node = malloc(sizeof(MAILBOX_NODE));
	MAILBOX_ENTRY *entry = malloc(sizeof(MAILBOX_ENTRY));
	entry->type = NOTICE_ENTRY_TYPE;
	entry->content.notice = *notice;

	if(mb->count == 0) {
		mb->front = mb->rear = mailbox_node;
		pthread_mutex_unlock(&mb->lock);
		return;
	}

	mb->rear->next = mailbox_node;
	mb->rear = mailbox_node;
	mb->count += 1;

	pthread_mutex_unlock(&mb->lock);
	sem_post(&mb->sem);
}

MAILBOX_ENTRY *mb_next_entry(MAILBOX *mb) {

	sem_wait(&mb->sem);

	if(mb->is_defunct == 1) {
		return NULL;
	}

	if(pthread_mutex_lock(&mb->lock) < 0) {
		return NULL;
	}

	MAILBOX_ENTRY *dequed_entry = mb->front->entry;
    mb->front = mb->front->next;
    mb->count -= 1;

	if(pthread_mutex_unlock(&mb->lock) < 0) {
		return NULL;
	}

	return dequed_entry;
}