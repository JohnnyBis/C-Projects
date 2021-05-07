#include "user.h"
#include "globals.h"
#include "string.h"
#include <pthread.h>

typedef struct user {
	char *handle;
	int reference;
	char *why;
	pthread_mutex_t lock;
} USER;

USER *user_create(char *handle) {
	if(handle == NULL) {
		return NULL;
	}

	USER *new_user = malloc(sizeof(USER));
	if(new_user == NULL) {
		return NULL;
	}

	if(pthread_mutex_init(&new_user->lock, NULL) < 0) {
		free(new_user);
		return NULL;
	}

	char *new_handle = malloc(sizeof(handle));
	if(new_handle == NULL) {
		return NULL;
	}

	strcpy(new_handle, handle);
	new_user->handle = new_handle;
	new_user->reference = 1;

	char *why = malloc(sizeof(char));
	strcpy(why, "");
	new_user->why = why;

	return new_user;
}

USER *user_ref(USER *user, char *why) {
	if(user == NULL) {
		return user;
	}

	if(pthread_mutex_lock(&user->lock) < 0) {
		return NULL;
	}

	user->reference += 1;

	user->why = why;

	if(pthread_mutex_unlock(&user->lock) < 0) {
		return NULL;
	}

	return user;
}

void user_unref(USER *user, char *why) {

	if(pthread_mutex_lock(&user->lock) < 0) {
		return;
	}

	user->reference -= 1;

	user->why = why;

	if(user->reference == 0) {
		free(user);
	}

	pthread_mutex_unlock(&user->lock);

	return;
}

char *user_get_handle(USER *user) {
	if(user == NULL) {
		return NULL;
	}
	return user->handle;
}