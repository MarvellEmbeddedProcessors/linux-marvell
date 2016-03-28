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

#ifndef TM_RW_REGISTERS_INTERFACE_H
#define TM_RW_REGISTERS_INTERFACE_H

extern uint8_t tm_debug_on;

int set_hw_connection(void *environment_handle);
int flush_hw_connection(void *environment_handle);
int reset_hw_connection(void *environment_handle, int error);
int close_hw_connection(void *environment_handle);

int tm_register_read(void *environment_handle, void *vpAddress, void *vpData);
int tm_register_write(void *environment_handle, void *vpAddress, void *vpData);

int tm_table_entry_read(void *environment_handle, void *vpAddress, long int index, void *vpData);
int tm_table_entry_write(void *environment_handle, void *vpAddress, long int index, void *vpData);


#endif   /* TM_RW_REGISTERS_INTERFACE_H */
