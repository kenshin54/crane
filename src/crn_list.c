#include "crane.h"
#include "crn_list.h"

crn_list_t *crn_list_create() {
	crn_list_t *list = malloc(sizeof(crn_list_t));
	list->head = NULL;
	list->tail = NULL;
	list->count = 0;
	return list;
}

void
crn_list_destroy(crn_list_t *list, void (*destroy)(void *)) {
	if (list != NULL) {
		crn_list_node_t *tmp;
		crn_list_node_t *head = list->head;
		while (head != NULL) {
		   tmp = head;
		   head = head->next;
		   if (destroy != NULL) {
			   destroy(tmp->data);
		   }
		   free(tmp);
		}
		free(list);
	}
}

void
crn_list_add(crn_list_t *list, void *data) {
	crn_list_node_t *node = malloc(sizeof(crn_list_node_t));
	node->data = data;
	node->next = NULL;
	if (list->head == NULL) {
		list->head = node;
		list->tail = node;
	} else {
		list->tail->next = node;
		list->tail = node;
	}
	list->count++;
}
