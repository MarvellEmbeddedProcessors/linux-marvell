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
*******************************************************************************/

/* includes */
#include <linux/io.h>
#include <linux/slab.h>
#include "mv_stack.h"

/* defines  */

/* Public functions */

/* Purpose: Create new stack
 * Inputs:
 *	- u32	num_of_elements	- maximum number of elements in the stack.
 *                              Each element 4 bytes size
 * Return: void* - pointer to created stack.
 */
void *mv_stack_create(int num_of_elements)
{
	struct  mv_stack *p_stack;
	u32 *p_stack_elements;

	p_stack = kmalloc(sizeof(struct mv_stack), GFP_KERNEL);
	p_stack_elements = kmalloc(num_of_elements * sizeof(u32), GFP_KERNEL);
	if ((p_stack == NULL) || (p_stack_elements == NULL)) {
		pr_err("mvStack: Can't create new stack\n");
		kfree(p_stack);
		kfree(p_stack_elements);
		return NULL;
	}
	memset(p_stack_elements, 0, num_of_elements * sizeof(u32));
	p_stack->num_of_elements = num_of_elements;
	p_stack->stack_idx = 0;
	p_stack->stack_elements = p_stack_elements;

	return p_stack;
}

/* Purpose: Delete existing stack
 * Inputs:
 *	- void*		stack_hndl	- Stack handle as returned by "mvStackCreate()" function
 *
 * Return: none.
 */
void mv_stack_delete(void *stack_hndl)
{
	struct mv_stack *p_stack = (struct mv_stack *) stack_hndl;

	if ((p_stack == NULL) || (p_stack->stack_elements == NULL))
		return;

	kfree(p_stack->stack_elements);
	kfree(p_stack);
}
