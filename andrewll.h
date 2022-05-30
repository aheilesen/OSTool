#ifndef ANDREWLL_H_
#define ANDREWLL_H_

#include <stdlib.h>


typedef struct LL_Element {
	void*			data;
	struct LL_Element*	next;
} LL_elem;


typedef struct LList {
	int	size;

	int	(*match)(const void* key1, const void* key2);
	void	(*destroy)(void* data);

	LL_elem*	head;
	LL_elem*	tail;
} LL;


void LL_Init(LL* list, void (*destroy)(void* data));

void LL_Destroy(LL* list);

int LL_Ins_Next(LL* list, LL_elem* elem, void* data);

int LL_Rem_Next(LL* list, LL_elem* elem, void** data);


#define LL_Size(list)	((list)->size)

#define LL_Head(list)	((list)->head)

#define LL_Tail(list)	((list)->tail)

#define LL_Is_Head(list, element)	((element) == (list)->head ? 1 : 0)

#define LL_Is_Tail(list, element)	((element) == (list)->tail ? 1 : 0)

#define LL_Data(element)	((element)->data)

#define LL_Next(element)	((element)->next)



#endif
