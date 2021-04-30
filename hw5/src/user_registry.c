#include "user_registry.h"
#include "string.h"
#include <stdlib.h>
#include "user.h"



typedef struct registry_node REGISTRY_NODE;

typedef struct registry_node {
	USER *user;
	REGISTRY_NODE *next;
} REGISTRY_NODE;

typedef struct user_registry {
	REGISTRY_NODE *registry_head;
	int size;
} USER_REGISTRY;


USER_REGISTRY *ureg_init() {
	USER_REGISTRY *user_registry = malloc(sizeof(USER_REGISTRY));
	if(user_registry == NULL) {
		return NULL;
	}

	user_registry->size = 0;
	return user_registry;
}

void ureg_fini(USER_REGISTRY *ureg) {
	free(ureg);
}

USER *is_registered(USER_REGISTRY *ureg, char *handle) {

	REGISTRY_NODE *current_node = ureg->registry_head;

	while(current_node != NULL) {
		if(strcmp(user_get_handle(current_node->user), handle) == 0) {
			//If registered user is found.
			USER *new_user = user_ref(current_node->user, "New reference that is being exported from the registry.");
			return new_user;
		}
		current_node = current_node->next;
	}
	return NULL;
}

USER *ureg_register(USER_REGISTRY *ureg, char *handle) {

	if(handle == NULL) {
		return NULL;
	}

	//REGISTRY_NODE *current_node = ureg->registry_head;
	USER *new_user = is_registered(ureg, handle);

	if(new_user != NULL) {
		//User is already registered.
		return new_user;
	}

	REGISTRY_NODE *head = ureg->registry_head;
	while(head != NULL) {
		head = head->next;
	}

	new_user = user_create(handle);

	if(new_user == NULL) {
		return NULL;
	}

	new_user = user_ref(new_user, "Reference stored by the user registry.");

	REGISTRY_NODE *new_head = malloc(sizeof(REGISTRY_NODE));
	new_head->user = new_user;
	new_head->next = NULL;
	if(ureg->size == 0) {
		ureg->registry_head = new_head;
	}else{
		head = new_head;
	}

	ureg->size += 1;

	return new_user;
}

void ureg_unregister(USER_REGISTRY *ureg, char *handle) {
	USER *user = is_registered(ureg, handle);

	if(user == NULL) {
		return;
	}

	user_unref(user, "Decrease reference to unregister.");

	REGISTRY_NODE *current_node = ureg->registry_head;
	REGISTRY_NODE *prev_node = NULL;

	while(current_node != NULL) {
		if(strcmp(user_get_handle(current_node->user), handle) == 0) {
			//If registered user is found.
			REGISTRY_NODE *next_node = current_node->next;
			prev_node->next = next_node;
			return;
		}
		current_node = current_node->next;
		prev_node = current_node;
	}
}