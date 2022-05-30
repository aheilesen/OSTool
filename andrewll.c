#include <stdlib.h>
#include <string.h>

#include "andrewll.h"

void LL_Init(LL* list, void (*destroy)(void* data))
{
	list->size = 0;
	list->destroy = destroy;
	list->head = NULL;
	list->tail = NULL;
}

void LL_Destroy(LL* list)
{
	void* data = NULL;

	while (LL_Size(list) > 0)
		if (LL_Rem_Next(list, NULL, (void**)&data) == 0)
			if (list->destroy)
				list->destroy(data);

	memset(list, 0, sizeof(LL));
	free(list);
}

int LL_Rem_Next(LL* list, LL_elem* elem, void** data)
{
	LL_elem* old_elem;

	if (LL_Size(list) == 0)
		return -1;

	// Removal of head
	if (elem == NULL) {
		*data = list->head->data;
		old_elem = list->head;
		list->head = list->head->next;

		if (LL_Size(list) == 1)
			list->tail = NULL;
	}
	else {
		if (elem->next == NULL)
			return -1;

		*data = elem->next->data;
		old_elem = elem->next;
		elem->next = elem->next->next;

		if (elem->next == NULL)
			list->tail = elem;
	}

	free(old_elem);
	list->size--;
	return 0;
}

int LL_Ins_Next(LL* list, LL_elem* elem, void* data)
{
	LL_elem* new_elem;

	if ((new_elem = malloc(sizeof(LL_elem))) == NULL)
		return -1;

	new_elem->data = data;

	// Insertion at head
	if (elem == NULL) {
		if (LL_Size(list) == 0)
			list->tail = new_elem;

		new_elem->next = list->head;
		list->head = new_elem;
	}
	else {
		if (elem->next == NULL)
			list->tail = new_elem;

		new_elem->next = elem->next;
		elem->next = new_elem;
	}

	list->size++;
	return 0;
}
