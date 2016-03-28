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

#include "tm/core/tm_hw_configuration_interface.h"

/* below all NSS harware definitions */

/* Max number of nodes supported by HW */
/** Max number of queues supported by HW */
#define NSS_TM_MAX_QUEUES   512 /* SN 128*1024*/
/** Max number of A-nodes supported by HW */
#define NSS_TM_MAX_A_NODES  128 /* SN 32*1024 */
/** Max number of B-nodes supported by HW */
#define NSS_TM_MAX_B_NODES  32 /* SN 8*1024 */
/** Max number of C-nodes supported by HW */
#define NSS_TM_MAX_C_NODES  16 /* SN 1024 */
/** Max number of Ports supported by HW */
#define NSS_TM_MAX_PORTS    16 /* SN 192 */


#define NSS_QUEUE_MIN_PERIODIC_INTERVAL			512
#define NSS_A_LEVEL__MIN_PERIODIC_INTERVAL		512
#define NSS_B_LEVEL__MIN_PERIODIC_INTERVAL		1024
#define NSS_C_LEVEL__MIN_PERIODIC_INTERVAL		1024
#define NSS_PORT__MIN_PERIODIC_INTERVAL			48

static	unsigned int	__isInitialized = 1;

static	unsigned int	__totalPortCount;
static	unsigned int	__totalCnodesCount;
static	unsigned int	__totalBnodesCount;
static	unsigned int	__totalAnodesCount;
static	unsigned int	__totalQueuesCount;

static	unsigned int	__queueMinPeriodicInterval;
static	unsigned int	__a_LevelMinPeriodicInterval;
static	unsigned int	__b_LevelMinPeriodicInterval;
static	unsigned int	__c_LevelMinPeriodicInterval;
static	unsigned int	__portMinPeriodicInterval;


unsigned int init_tm_hardware_configuration(const char *cProductName)
{
/* here all parameters should be readed from database or hardware ...
 (in case of various h/w configuration)
 if failed - error > 0 is returned */

	/* currently it is assigned from hardcoded definitions. */
	__totalPortCount = NSS_TM_MAX_PORTS;
	__totalCnodesCount = NSS_TM_MAX_C_NODES;
	__totalBnodesCount = NSS_TM_MAX_B_NODES;
	__totalAnodesCount = NSS_TM_MAX_A_NODES;
	__totalQueuesCount = NSS_TM_MAX_QUEUES;

	__queueMinPeriodicInterval = NSS_QUEUE_MIN_PERIODIC_INTERVAL;
	__a_LevelMinPeriodicInterval = NSS_A_LEVEL__MIN_PERIODIC_INTERVAL;
	__b_LevelMinPeriodicInterval = NSS_B_LEVEL__MIN_PERIODIC_INTERVAL;
	__c_LevelMinPeriodicInterval = NSS_C_LEVEL__MIN_PERIODIC_INTERVAL;
	__portMinPeriodicInterval = NSS_PORT__MIN_PERIODIC_INTERVAL;


	/* successful initialization */
	__isInitialized = 1;
	return 0;
}


unsigned int	is_tm_initialized()
{
	return __isInitialized;
}

unsigned int	get_tm_port_count()
{
	return __totalPortCount;
}
unsigned int	get_tm_a_nodes_count()
{
	return __totalAnodesCount;
}
unsigned int	get_tm_b_nodes_count()
{
	return __totalBnodesCount;
}
unsigned int	get_tm_c_nodes_count()
{
	return __totalCnodesCount;
}
unsigned int	get_tm_queues_count()
{
	return __totalQueuesCount;
}


unsigned int	get_queue_min_periodic_interval()
{
	return __queueMinPeriodicInterval;
}
unsigned int	get_a_level_min_periodic_interval()
{
	return __a_LevelMinPeriodicInterval;
}
unsigned int	get_b_level_min_periodic_interval()
{
	return __b_LevelMinPeriodicInterval;
}
unsigned int	get_c_level_min_periodic_interval()
{
	return __c_LevelMinPeriodicInterval;
}
unsigned int	get_port_min_periodic_interval()
{
	return __portMinPeriodicInterval;
}

/** Minimum TM Frequency (in Hz) */
#define TM_MIN_FREQUENCY 250000000
/** Max TM Frequency (in Hz) */
#define TM_MAX_FREQUENCY 250000000

unsigned int	get_TM_min_frequency() { return TM_MIN_FREQUENCY; }
unsigned int	get_TM_max_frequency() { return TM_MAX_FREQUENCY; }

#define PROFILE_TD_THRESHOLD	0x7FFFF /* 19 bits */


unsigned int	get_drop_threshold_definition() { return PROFILE_TD_THRESHOLD; }
