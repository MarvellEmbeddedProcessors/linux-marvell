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

    *   Redistributions of source code must retain the above copyright notice,
	this list of conditions and the following disclaimer.

    *   Redistributions in binary form must reproduce the above copyright
	notice, this list of conditions and the following disclaimer in the
	documentation and/or other materials provided with the distribution.

    *   Neither the name of Marvell nor the names of its contributors may be
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

#ifndef __mvStack_h__
#define __mvStack_h__

/* includes */
#include <linux/kernel.h>


/* LIFO Stack implementation */
/* Data structure describes general purpose Stack */
struct mv_stack {
	int stack_idx;
	int num_of_elements;
	u32 *stack_elements;
};

static inline bool mv_stack_is_full(void *stack_hndl)
{
	struct mv_stack *p_stack = (struct mv_stack *) stack_hndl;

	if (p_stack->stack_idx == p_stack->num_of_elements)
		return true;

	return false;
}

static inline bool mv_stack_is_empty(void *stack_hndl)
{
	struct mv_stack *p_stack = (struct mv_stack *) stack_hndl;

	if (p_stack->stack_idx == 0)
		return true;

	return false;
}

/* Purpose: Push new element to stack
 * Inputs:
 *	- void*		stack_hndl	- Stack handle as returned by "mv_stack_create()" function.
 *	- u32		value		- New element.
 *
 */
static inline int mv_stack_push(void *stack_hndl, u32 value)
{
	struct mv_stack *p_stack = (struct mv_stack *) stack_hndl;

	if (p_stack->stack_idx == p_stack->num_of_elements) {
		pr_info("%s: Stack is FULL\n", __func__);
		return 0;
	}

	p_stack->stack_elements[p_stack->stack_idx] = value;
	p_stack->stack_idx++;
	return 1;
}

/* Purpose: Pop element from the top of stack and copy it to "p_value"
 * Inputs:
 *	- void*		stack_hndl	- Stack handle as returned by "mv_stack_create()" function.
 *	- u32		value		- Element in the top of stack.
 *
 * Return: int		-1 - Failure. Stack is empty.
 *					value - Success. Element is removed from the stack and
 *							copied to pValue argument
 */
static inline int mv_stack_pop(void *stack_hndl)
{
	struct mv_stack *p_stack = (struct mv_stack *) stack_hndl;

	if (p_stack->stack_idx == 0) {
		pr_info("%s: Stack is EMPTY\n", __func__);
		return -1;
	}

	p_stack->stack_idx--;
	return p_stack->stack_elements[p_stack->stack_idx];
}

static inline int mv_stack_index(void *stack_hndl)
{
	struct mv_stack *p_stack = (struct mv_stack *) stack_hndl;

	return p_stack->stack_idx;
}

static inline int mv_stack_free_elements(void *stack_hndl)
{
	struct mv_stack *p_stack = (struct mv_stack *) stack_hndl;

	return p_stack->num_of_elements - p_stack->stack_idx;
}

/* mv_stack.h API list */

/* Create new Stack */
void *mv_stack_create(int num_of_elements);

/* Delete existing stack */
int mv_stack_delete(void *stack_hndl);

#endif /* __mvStack_h__ */
