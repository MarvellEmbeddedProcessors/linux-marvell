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

#ifndef TM_HW_CONFIGURATION_INTERFACE_H
#define TM_HW_CONFIGURATION_INTERFACE_H

/* interface for access to all hardware resources - interface is platform independent,
 implementation is platform dependent, different files for different platforms
*/

/* initialization of all hardware resources returning result - 0 if success >0 if failed */
unsigned int	init_tm_hardware_configuration(const char *cProductName);

/* following function returns 1 if hardware resources were successfully initialized
 otherwise 0 (finitialization failed or not performed) */
unsigned int	is_tm_initialized(void);

/* following functions return appropriate hardware value  or 0 if hardware initialization missed or failed */
unsigned int	get_tm_port_count(void);
unsigned int	get_tm_c_nodes_count(void);
unsigned int	get_tm_b_nodes_count(void);
unsigned int	get_tm_a_nodes_count(void);
unsigned int	get_tm_queues_count(void);

/* TM Frequency (in Hz) */
unsigned int	get_TM_min_frequency(void);
unsigned int	get_TM_max_frequency(void);

/* periodic process definitions */
unsigned int	get_queue_min_periodic_interval(void);
unsigned int	get_a_level_min_periodic_interval(void);
unsigned int	get_b_level_min_periodic_interval(void);
unsigned int	get_c_level_min_periodic_interval(void);
unsigned int	get_port_min_periodic_interval(void);

/* drop threshold definition */
unsigned int	get_drop_threshold_definition(void);


#endif   /* TM_HW_CONFIGURATION_INTERFACE_H */
