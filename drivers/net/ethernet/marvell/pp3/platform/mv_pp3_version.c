/*
* ***************************************************************************
* Copyright (C) 2015 Marvell International Ltd.
* ***************************************************************************
* This program is free software: you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the Free
* Software Foundation, either version 2 of the License, or any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ***************************************************************************
*/


/* includes */
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/slab.h>
#include "fw/mv_pp3_fw_msg_structs.h"

static struct mv_pp3_version mv_pp3_driver_version = {
	.name = "NSS",
	.major_x = 1,
	.minor_y = 1,
	.local_z = 5,
	.debug_d = 0
};

struct mv_pp3_version *mv_pp3_get_driver_version(void)
{
	return &mv_pp3_driver_version;
}
