/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.

********************************************************************************
Marvell Commercial License Option

If you received this File from Marvell and you have entered into a commercial
license agreement (a "Commercial License") with Marvell, the File is licensed
to you under the terms of the applicable Commercial License.

********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File in accordance with the terms and conditions of the General
Public License Version 2, June 1991 (the "GPL License"), a copy of which is
available along with the File in the license.txt file or by writing to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
on the worldwide web at http://www.gnu.org/licenses/gpl.txt.

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
DISCLAIMED.  The GPL License provides additional details about this warranty
disclaimer.
********************************************************************************
Marvell BSD License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File under the following licensing terms.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

	* Redistributions of source code must retain the above copyright notice,
	  this list of conditions and the following disclaimer.

	* Redistributions in binary form must reproduce the above copyright
	  notice, this list of conditions and the following disclaimer in the
	  documentation and/or other materials provided with the distribution.

	* Neither the name of Marvell nor the names of its contributors may be
	  used to endorse or promote products derived from this software without
	  specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/


/********************************************************************************
* mvStack.h - Header File for :
*
* FILENAME:    $Workfile: mvStack.h $
* REVISION:    $Revision: 1.1 $
* LAST UPDATE: $Modtime:  $
*
* DESCRIPTION:
*     This file defines simple Stack (LIFO) functionality.
*
*******************************************************************************/

#ifndef __mvStack_h__
#define __mvStack_h__


/* includes */
#include "mvTypes.h"


/* defines  */


/* typedefs */
/* Data structure describes general purpose Stack */
typedef struct
{
    int     stackIdx;
    int     numOfElements;
    MV_U32* stackElements;
} MV_STACK;

static INLINE MV_BOOL mvStackIsFull(void* stackHndl)
{
    MV_STACK*   pStack = (MV_STACK*)stackHndl;

    if(pStack->stackIdx == pStack->numOfElements)
        return MV_TRUE;

    return MV_FALSE;
}

static INLINE MV_BOOL mvStackIsEmpty(void* stackHndl)
{
    MV_STACK*   pStack = (MV_STACK*)stackHndl;

    if(pStack->stackIdx == 0)
        return MV_TRUE;

    return MV_FALSE;
}
/* Purpose: Push new element to stack
 * Inputs:
 *	- void* 	stackHndl 	- Stack handle as returned by "mvStackCreate()" function.
 *	- MV_U32	value		- New element.
 *
 * Return: MV_STATUS  	MV_FULL - Failure. Stack is full.
 *						MV_OK   - Success. Element is put to stack.
 */
static INLINE void mvStackPush(void* stackHndl, MV_U32 value)
{
    MV_STACK*   pStack = (MV_STACK*)stackHndl;

#ifdef MV_RT_DEBUG
    if(pStack->stackIdx == pStack->numOfElements)
    {
        mvOsPrintf("mvStackPush: Stack is FULL\n");
        return;
    }
#endif /* MV_RT_DEBUG */

    pStack->stackElements[pStack->stackIdx] = value;
    pStack->stackIdx++;
}

/* Purpose: Pop element from the top of stack and copy it to "pValue"
 * Inputs:
 *	- void* 	stackHndl 	- Stack handle as returned by "mvStackCreate()" function.
 *	- MV_U32	value		- Element in the top of stack.
 *
 * Return: MV_STATUS  	MV_EMPTY - Failure. Stack is empty.
 *						MV_OK    - Success. Element is removed from the stack and
 *									copied to pValue argument
 */
static INLINE MV_U32   mvStackPop(void* stackHndl)
{
    MV_STACK*   pStack = (MV_STACK*)stackHndl;

#ifdef MV_RT_DEBUG
    if(pStack->stackIdx == 0)
    {
        mvOsPrintf("mvStackPop: Stack is EMPTY\n");
        return 0;
    }
#endif /* MV_RT_DEBUG */

    pStack->stackIdx--;
    return pStack->stackElements[pStack->stackIdx];
}

static INLINE int       mvStackIndex(void* stackHndl)
{
    MV_STACK*   pStack = (MV_STACK*)stackHndl;

    return pStack->stackIdx;
}

static INLINE int       mvStackFreeElements(void* stackHndl)
{
    MV_STACK*   pStack = (MV_STACK*)stackHndl;

    return (pStack->numOfElements - pStack->stackIdx);
}

/* mvStack.h API list */

/* Create new Stack */
void*       mvStackCreate(int numOfElements);

/* Delete existing stack */
MV_STATUS   mvStackDelete(void* stackHndl);

/* Print status of the stack */
void        mvStackStatus(void* stackHndl, MV_BOOL isPrintElements);

#endif /* __mvStack_h__ */
