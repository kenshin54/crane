#ifndef CRN_LIST_H
#define CRN_LIST_H

typedef struct crn_list_node {
	void *data;
	struct crn_list_node *next;
} crn_list_node_t;

typedef struct {
	int count;
	crn_list_node_t *head;
	crn_list_node_t *tail;
} crn_list_t;

crn_list_t *crn_list_create();
void crn_list_destroy(crn_list_t *);

#define crn_list_count(A) ((A)->count)

void crn_list_add(crn_list_t *, void *);

#define CRN_LIST_FOREACH(L, N) crn_list_node_t *_node = NULL;\
    crn_list_node_t *N = NULL;\
    for(N = _node = L->head; _node != NULL; N = _node = _node->next)

#endif
