#include "user.h"
#include "globals.h"
#include "string.h"

typedef struct user {
	char *handle;
	int reference;
	char *why;
} USER;

USER *user_create(char *handle) {
	if(handle == NULL) {
		return NULL;
	}

	USER *new_user = malloc(sizeof(USER));
	if(new_user == NULL) {
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
	user->reference += 1;

	user->why = why;
	return user;
}

void user_unref(USER *user, char *why) {
	user->reference -= 1;

	user->why = why;

	if(user->reference == 0) {
		free(user);
	}
	return;
}

char *user_get_handle(USER *user) {
	if(user == NULL) {
		return NULL;
	}
	return user->handle;
}