#include "mvCopyright.h"

/********************************************************************************
* mvList.c - Implementation File for Linked List.
*
* DESCRIPTION:
*     This file implements basic Linked List functionality.
*
*******************************************************************************/

#include "mvList.h"

/* Create a Linked List by allocating the list head */
/* Returns the head of the list if successful, NULL otherwise */
MV_LIST_ELEMENT *mvListCreate(MV_VOID)
{
	MV_LIST_ELEMENT *head = (MV_LIST_ELEMENT *)mvOsMalloc(sizeof(MV_LIST_ELEMENT));

	if (head) {
		head->prev = NULL;
		head->next = NULL;
		head->data = 0;
	}

#ifdef MV_LIST_SANITY_CHECKS
	if (!head)
		mvOsPrintf("%s ERROR: memory allocation for new list failed\n", __func__);
#endif /* MV_LIST_SANITY_CHECKS */

	return head;
}

/* Delete all elements in a given list and free the list head */
MV_STATUS mvListDestroy(MV_LIST_ELEMENT *head)
{
	MV_LIST_ELEMENT *curr, *tmp;

#ifdef MV_LIST_SANITY_CHECKS
	/* sanity check */
	if (!head) {
		mvOsPrintf("%s ERROR: trying to destroy uninitialized list\n", __func__);
		return MV_ERROR;
	}
#endif /* MV_LIST_SANITY_CHECKS */

	/* delete all elements in the list */
	/* skip list head, it never contains real data */
	curr = head->next;
	while (curr) {
		tmp = curr;
		curr = curr->next;
		mvListDel(tmp);
	}

	/* free the list head */
	mvOsFree(head);

	return MV_OK;
}

/* Count the number of elements in the list (not including the head) */
MV_LONG mvListElementsCount(MV_LIST_ELEMENT *head)
{
	MV_LONG count = 0;
	MV_LIST_ELEMENT *curr;

#ifdef MV_LIST_SANITY_CHECKS
	/* sanity check */
	if (!head) {
		mvOsPrintf("%s ERROR: trying to count elements in an uninitialized list\n", __func__);
		return -1;
	}
#endif /* MV_LIST_SANITY_CHECKS */

	/* skip list head, it's not a real element */
	for (curr = head->next; curr != NULL; curr = curr->next)
		count++;

	return count;
}

/* Print all list elements */
MV_VOID mvListPrint(MV_LIST_ELEMENT *head)
{
	MV_LIST_ELEMENT *curr;

	/* skip list head, it never contains real data */
	for (curr = head->next; curr != NULL; curr = curr->next) {
		mvOsPrintf("%lu ", curr->data);
		MV_LIST_DBG("element = %p, prev = %p, next = %p, data = %lu\n", curr, curr->prev, curr->next, curr->data);
	}
	mvOsPrintf("\n");
}

/* simple self-contained test */
MV_VOID mvListTest(MV_VOID)
{
	int i;
	MV_LIST_ELEMENT *list_elements[10];
	MV_LIST_ELEMENT *head = mvListCreate();

	mvOsPrintf("\n\n----- mvListTest -----\n\n");

	for (i = 0; i < 10; i++)
		list_elements[i] = mvListAddHead(head, i);

	mvListPrint(head);

	mvListDel(list_elements[0]);
	mvListDel(list_elements[9]);
	mvListDel(list_elements[4]);

	mvListPrint(head);

	mvListDestroy(head);
}
