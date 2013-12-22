#include "mvCopyright.h"

/********************************************************************************
* mvQueue.c
*
* FILENAME:    $Workfile: mvStack.c $
* LAST UPDATE: $Modtime:  $
*
* DESCRIPTION:
*     This file implements simple Stack LIFO functionality.
*******************************************************************************/

/* includes */
#include "mvCommon.h"
#include "mvOs.h"
#include "mvTypes.h"
#include "mvDebug.h"
#include "mvStack.h"

/* defines  */

/* Public functions */

/* Purpose: Create new stack
 * Inputs:
 *	- MV_U32	noOfElements	- maximum number of elements in the stack.
 *                              Each element 4 bytes size
 * Return: void* - pointer to created stack.
 */
void *mvStackCreate(int numOfElements)
{
	MV_STACK *pStack;
	MV_U32 *pStackElements;

	pStack = (MV_STACK *)mvOsMalloc(sizeof(MV_STACK));
	pStackElements = (MV_U32 *)mvOsMalloc(numOfElements * sizeof(MV_U32));
	if ((pStack == NULL) || (pStackElements == NULL)) {
		mvOsPrintf("mvStack: Can't create new stack\n");
		if (pStack)
			mvOsFree(pStack);
		if (pStackElements)
			mvOsFree(pStackElements);
		return NULL;
	}
	memset(pStackElements, 0, numOfElements * sizeof(MV_U32));
	pStack->numOfElements = numOfElements;
	pStack->stackIdx = 0;
	pStack->stackElements = pStackElements;

	return pStack;
}

/* Purpose: Delete existing stack
 * Inputs:
 *	- void* 	stackHndl 	- Stack handle as returned by "mvStackCreate()" function
 *
 * Return: MV_STATUS  	MV_NOT_FOUND - Failure. StackHandle is not valid.
 *			MV_OK        - Success.
 */
MV_STATUS mvStackDelete(void *stackHndl)
{
	MV_STACK *pStack = (MV_STACK *) stackHndl;

	if ((pStack == NULL) || (pStack->stackElements == NULL))
		return MV_NOT_FOUND;

	mvOsFree(pStack->stackElements);
	mvOsFree(pStack);

	return MV_OK;
}

/* PrintOut status of the stack */
void mvStackStatus(void *stackHndl, MV_BOOL isPrintElements)
{
	int i;
	MV_STACK *pStack = (MV_STACK *) stackHndl;

	mvOsPrintf("StackHandle=%p, pElements=%p, numElements=%d, stackIdx=%d\n",
		   stackHndl, pStack->stackElements, pStack->numOfElements, pStack->stackIdx);
	if (isPrintElements == MV_TRUE) {
		for (i = 0; i < pStack->stackIdx; i++)
			mvOsPrintf("%3d. Value=0x%x\n", i, pStack->stackElements[i]);
	}
}
