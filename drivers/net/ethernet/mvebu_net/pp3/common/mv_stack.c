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
 * Return: -1 - Failure. Stack Handle is not valid.
 *			0 - Success.
 */
int mv_stack_delete(void *stack_hndl)
{
	struct mv_stack *p_stack = (struct mv_stack *) stack_hndl;

	if ((p_stack == NULL) || (p_stack->stack_elements == NULL))
		return -1;

	kfree(p_stack->stack_elements);
	kfree(p_stack);

	return 0;
}
