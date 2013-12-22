#include "mvCopyright.h"

/********************************************************************************
* mvList.h - Header File for Linked List.
*
* DESCRIPTION:
*     This file defines basic Linked List functionality.
*
*******************************************************************************/

#ifndef __mvList_h__
#define __mvList_h__

#include "mvCommon.h"
#include "mvOs.h"

/* Un-comment the next line to use sanity checks in the code */
/* #define MV_LIST_SANITY_CHECKS */

/* Un-comment the next line to enable debug prints */
/* #define MV_LIST_DEBUG */

#ifdef MV_LIST_DEBUG
#define MV_LIST_DBG(fmt, arg...) mvOsPrintf(fmt, ##arg)
#else
#define MV_LIST_DBG(fmt, arg...)
#endif

typedef struct mv_list_element {
	struct mv_list_element *prev;
	struct mv_list_element *next;
	MV_ULONG data;

} MV_LIST_ELEMENT;

/* Returns the first matching element in the list, NULL if not found */
static INLINE MV_LIST_ELEMENT *mvListFind(MV_LIST_ELEMENT *head, MV_ULONG data)
{
	MV_LIST_ELEMENT *curr;

	/* skip list head, it never contains real data */
	for (curr = head->next; curr != NULL; curr = curr->next) {
		if (curr->data == data)
			return curr;
	}
	return NULL;
}

/* Add a new element at the top of the list (right after head) */
/* The list head will point to this new element */
/* Returns pointer to new element if successful, NULL otherwise */
static INLINE MV_LIST_ELEMENT *mvListAddHead(MV_LIST_ELEMENT *head, MV_ULONG data)
{
	MV_LIST_ELEMENT *element;

#ifdef MV_LIST_SANITY_CHECKS
	/* sanity check */
	if (!head) {
		mvOsPrintf("%s ERROR: trying to add an element to an uninitialized list\n", __func__);
		return NULL;
	}
#endif /* MV_LIST_SANITY_CHECKS */

	element = mvOsMalloc(sizeof(MV_LIST_ELEMENT));
	if (element) {
		element->data = data;
		element->next = head->next;
		element->prev = head;
		if (head->next)
			head->next->prev = element;

		head->next = element;

		MV_LIST_DBG("Adding new element %p: data = %lu, next = %p, prev = %p\n",
				element, element->data, element->next, element->prev);
	}

#ifdef MV_LIST_SANITY_CHECKS
	if (!element)
		mvOsPrintf("%s ERROR: memory allocation for new element failed\n", __func__);
#endif /* MV_LIST_SANITY_CHECKS */

	return element;
}

/* Delete an element from a list */
/* Return the deleted element data */
static INLINE MV_ULONG mvListDel(MV_LIST_ELEMENT *element)
{
	MV_LIST_ELEMENT *prev;
	MV_LIST_ELEMENT *next;
	MV_ULONG data;

#ifdef MV_LIST_SANITY_CHECKS
	/* sanity check */
	if (!element) {
		mvOsPrintf("%s ERROR: trying to delete a NULL element\n", __func__);
		return 0;
	}
#endif /* MV_LIST_SANITY_CHECKS */

	prev = element->prev;
	next = element->next;
	data = element->data;

	MV_LIST_DBG("Deleting element %p, data = %lu, prev = %p, next = %p\n", element, element->data, prev, next);

	mvOsFree(element);

	if (prev)
		prev->next = next;
	else
		mvOsPrintf("%s ERROR: trying to delete an element when prev == NULL\n", __func__);

	if (next)
		next->prev = prev;

	return data;
}

MV_LIST_ELEMENT *mvListCreate(MV_VOID);
MV_STATUS mvListDestroy(MV_LIST_ELEMENT *head);
MV_LONG mvListElementsCount(MV_LIST_ELEMENT *head);
MV_VOID mvListPrint(MV_LIST_ELEMENT *head);
MV_VOID mvListTest(MV_VOID);

#endif /* __mvList_h__ */
