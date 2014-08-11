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
********************************************************************************
* mv_cph_sysfs.c
*
* DESCRIPTION: Marvell CPH(CPH Packet Handler) sysfs command definition
*
* DEPENDENCIES:
*               None
*
* CREATED BY:   VictorGu
*
* DATE CREATED: 22Jan2013
*
* FILE REVISION NUMBER:
*               Revision: 1.1
*
*
*******************************************************************************/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/capability.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/netdevice.h>

#include "mv_cph_header.h"

static ssize_t cph_spec_proc_help(char *b)
{
	int o = 0; /* buffer offset */
	int s = PAGE_SIZE; /* buffer size */

	o += scnprintf(b+o, s-o, "cat  help                                 - show this help\n");
	o += scnprintf(b+o, s-o, "cat  help_add                             - show additional help for parameters\n");
	o += scnprintf(b+o, s-o, "cat  show_app_db                          - show all information in application rule data base\n");
	o += scnprintf(b+o, s-o, "cat  show_parse_name                      - show sysfs parsing rule data base\n");
	o += scnprintf(b+o, s-o, "cat  show_mod_name                        - show sysfs modification rule data base\n");
	o += scnprintf(b+o, s-o, "cat  show_frwd_name                       - show sysfs modification rule data base\n");
#ifdef CONFIG_MV_CPH_UDP_SAMPLE_HANDLE
	o += scnprintf(b+o, s-o, "cat  udp_ports                            - show special udp source and destination port configuration\n");
#endif
#ifdef CONFIG_MV_CPH_FLOW_MAP_HANDLE
	o += scnprintf(b+o, s-o, "cat  show_flow_rule                       - show flow mapping rules\n");
	o += scnprintf(b+o, s-o, "cat  clear_flow_rule                      - clear all flow mapping rules\n");
	o += scnprintf(b+o, s-o, "cat  del_dscp_map                         - delete DSCP to P-bits mapping rules\n");
#endif
	o += scnprintf(b+o, s-o, "echo p dir en                             > set_port_func   - enable or disable cph function on physical port\n");
	o += scnprintf(b+o, s-o, "echo p                                    > get_port_func   - show cph function enabled status on physical port\n");
	o += scnprintf(b+o, s-o, "	p(dec): physical port | dir(dec): 0:Rx, 1:Tx, 2:both of dir | en(dec): 0:disable, 1:enable\n");
	o += scnprintf(b+o, s-o, "echo profile_id active_port               > set_complex     - Set TPM complex profile ID and active GMAC port, 0:GMAC0, 1:GMAC1, 2:PON MAC\n");
	o += scnprintf(b+o, s-o, "echo feature state                        > set_flag        - Set the support of CPH feature: refer to below additional info, state 0:disable, 1:enable\n");
	o += scnprintf(b+o, s-o, "echo tcont state                          > set_tcont       - Set T-CONT state in CPH, T-CONT 0~7, state, 1:enable, 0:disable\n");
	o += scnprintf(b+o, s-o, "echo hex                                  > trace_level     - Set cph trace level bitmap. 0x01:debug, 0x02:info, 0x04:warn, 0x08:error\n");
	o += scnprintf(b+o, s-o, "echo name bm(hex) dir rx(hex) mh(hex) ety(hex) esty ipv4ty nh1 nh2 icmpty > add_parse   - add parsing field, dir 0:U/S, 1:D/S, 2:Not care\n");
	o += scnprintf(b+o, s-o, "echo name                                 > del_parse       - delete parsing field\n");
	o += scnprintf(b+o, s-o, "echo name bm(hex) proto_type(hex) state   > add_mod         - add modification field, state 0:diable, 1:enable\n");
	o += scnprintf(b+o, s-o, "echo name                                 > del_mod         - delete modification field\n");
	o += scnprintf(b+o, s-o, "echo name bm(hex) trg_port trg_queue gem  > add_frwd        - add forwarding field\n");
	o += scnprintf(b+o, s-o, "echo name                                 > del_frwd        - delete forwarding field\n");
	o += scnprintf(b+o, s-o, "echo parse_name mod_name frwd_name        > add_app_rule    - add application rule\n");
	o += scnprintf(b+o, s-o, "echo parse_name                           > del_app_rule    - delete application rule\n");
	o += scnprintf(b+o, s-o, "echo parse_name mod_name frwd_name        > update_app_rule - update application rule\n");
	o += scnprintf(b+o, s-o, "echo parse_name                           > get_app_rule    - get application rule\n");
#ifdef CONFIG_MV_CPH_UDP_SAMPLE_HANDLE
	o += scnprintf(b+o, s-o, "echo p udp_src(dec) txp txq flags hw_cmd  > udp_src         - set udp source port special Tx behavior\n");
	o += scnprintf(b+o, s-o, "echo p udp_dst(dec) txp txq flags hw_cmd  > udp_dst         - set udp destination port special Tx behavior\n");
#endif
#ifdef CONFIG_MV_CPH_FLOW_MAP_HANDLE
	o += scnprintf(b+o, s-o, "---------------------------------------------------------------------------------------------------------------------------------------\n");
	o += scnprintf(b+o, s-o, "                         |Parse outer    |Parse inner             |Mod outer      |Mod Inner      |Forward\n");
	o += scnprintf(b+o, s-o, "echo dir default parse_bm mh ety  tpid vid pbits  tpid vid pbits  op_type  tpid vid pbits  tpid vid pbits  port queue hwf_queue gem > add_flow_rule - Add flow rule\n");
	o += scnprintf(b+o, s-o, "echo dir default parse_bm mh ety  tpid vid pbits  tpid vid pbits  > del_flow_rule   - delete flow mapping rule\n");
	o += scnprintf(b+o, s-o, "echo dir default parse_bm mh ety  tpid vid pbits  tpid vid pbits  > get_flow_rule   - get flow mapping rule\n");
	o += scnprintf(b+o, s-o, "echo pbits0 pbits1 ... pbits62 pbits63                    > set_dscp_map    - set DSCP to P-bits mapping rules\n");
#endif
	return o;
}

static ssize_t cph_spec_proc_help_add(char *b)
{
	int o = 0; /* buffer offset */
	int s = PAGE_SIZE; /* buffer size */

	o += scnprintf(b+o, s-o, "CPH additional help for parameters\n");
	o += scnprintf(b+o, s-o, "---------------------------------------------------------------------------------------------------------------------------------------\n");
	o += scnprintf(b+o, s-o, "[Generic Parameters]\n");
	o += scnprintf(b+o, s-o, "feature:\n");
	o += scnprintf(b+o, s-o, "   0:Generic application, 1:IGMP/MLD support, 2:Broadcast support, 3:Data flow mapping support, 4: UDP port mapping support\n");
	o += scnprintf(b+o, s-o, "[App Parameters]\n");
	o += scnprintf(b+o, s-o, "parse bm:\n");
	o += scnprintf(b+o, s-o, "   0x01:PARSE_FIELD_DIR              0x02:PARSE_FIELD_MH               0x04:PARSE_FIELD_ETH_TYPE         0x08:PARSE_FIELD_ETH_SUBTYPE\n");
	o += scnprintf(b+o, s-o, "   0x10:PARSE_FIELD_IPV4_TYPE        0x20:PARSE_FIELD_IPV6_NH1         0x40:PARSE_FIELD_IPV6_NH2         0x80:PARSE_FIELD_ICMPV6_TYPE\n");
	o += scnprintf(b+o, s-o, "dir: 0: U/S, 1:D/S, 2: Not care\n");
	o += scnprintf(b+o, s-o, "rx: 0: RX, 1:TX\n");
	o += scnprintf(b+o, s-o, "mod bm:\n");
	o += scnprintf(b+o, s-o, "   0x01:RX_MOD_ADD_GMAC              0x02:RX_MOD_REPLACE_PROTO_TYPE    0x04:RX_MOD_STRIP_MH              0x08:TX_MOD_ADD_MH_BY_DRIVER\n");
	o += scnprintf(b+o, s-o, "   0x10:CPH_APP_TX_MOD_NO_PAD        0x20:MOD_SET_STATE\n");
	o += scnprintf(b+o, s-o, "frwd bm:\n");
	o += scnprintf(b+o, s-o, "   0x01:FRWD_SET_TRG_PORT            0x02:FRWD_SET_TRG_QUEUE           0x04:FRWD_SET_GEM_PORT\n");
	o += scnprintf(b+o, s-o, "[Flow Parameters]\n");
	o += scnprintf(b+o, s-o, "dir: 0: U/S, 1:D/S, 2: Not care\n");
	o += scnprintf(b+o, s-o, "default: 0: not default, 1:default\n");
	o += scnprintf(b+o, s-o, "bm:\n");
	o += scnprintf(b+o, s-o, "   0x01:PARSE_MH                     0x02:PARSE_EXT_VLAN               0x04:PARSE_TWO_VLAN               0x08:PARSE_ETH_TYPE\n");
	o += scnprintf(b+o, s-o, "mh(hex), ety(hex), tpid(hex), vid(dec), pbits(dec)\n");
	o += scnprintf(b+o, s-o, "op_type:\n");
	o += scnprintf(b+o, s-o, "   00:ASIS                           01:DISCARD                        02:ADD                            03:ADD_COPY_DSCP\n");
	o += scnprintf(b+o, s-o, "   04:ADD_COPY_OUTER_PBIT            05:ADD_COPY_INNER_PBIT            06:ADD_2_TAGS                     07:ADD_2_TAGS_COPY_DSCP\n");
	o += scnprintf(b+o, s-o, "   08:ADD_2_TAGS_COPY_PBIT           09:REM                            10:REM_2_TAGS                     11:REPLACE\n");
	o += scnprintf(b+o, s-o, "   12:REPLACE_VID                    13:REPLACE_PBIT                   14:REPLACE_INNER_ADD_OUTER        15:REPLACE_INNER_ADD_OUTER_COPY_PBIT\n");
	o += scnprintf(b+o, s-o, "   16:REPLACE_INNER_REM_OUTER        17:REPLACE_2TAGS                  18:REPLACE_2TAGS_VID              19:SWAP\n");

	return o;
}


/********************************************************************************/
/*                          Parsing field table                                 */
/********************************************************************************/
static struct CPH_SYSFS_PARSE_T cph_sysfs_parse_table[CPH_SYSFS_FIELD_MAX_ENTRY];

static struct CPH_SYSFS_RULE_T cph_parse_rule_db = {
	.max_entry_num    = CPH_SYSFS_FIELD_MAX_ENTRY,
	.entry_num        = 0,
	.entry_size       = sizeof(struct CPH_SYSFS_PARSE_T),
	.entry_ara        = cph_sysfs_parse_table
};

static void cph_sysfs_init_parse_db(void)
{
	struct CPH_SYSFS_PARSE_T  *p_entry = (struct CPH_SYSFS_PARSE_T *)cph_parse_rule_db.entry_ara;
	int               idx     = 0;

	for (idx = 0; idx < cph_parse_rule_db.max_entry_num; idx++, p_entry++)
		p_entry->name[0] = 0;
}

struct CPH_SYSFS_PARSE_T *cph_sysfs_find_parse_entry_by_name(char *name)
{
	struct CPH_SYSFS_PARSE_T *p_entry = (struct CPH_SYSFS_PARSE_T *)cph_parse_rule_db.entry_ara;
	int              idx     = 0;

	for (idx = 0; idx < cph_parse_rule_db.max_entry_num; idx++, p_entry++) {
		if (strcmp(p_entry->name, name) == 0)
			return p_entry;
	}
	return 0;
}

struct CPH_SYSFS_PARSE_T *cph_sysfs_find_free_parse_entry(void)
{
	struct CPH_SYSFS_PARSE_T *p_entry = (struct CPH_SYSFS_PARSE_T *)cph_parse_rule_db.entry_ara;
	int              idx     = 0;

	for (idx = 0; idx < cph_parse_rule_db.max_entry_num; idx++, p_entry++) {
		if (p_entry->name[0] == 0)
			return p_entry;
	}
	return 0;
}

bool cph_sysfs_del_parse_entry_by_name(char *name)
{
	struct CPH_SYSFS_PARSE_T *p_entry = (struct CPH_SYSFS_PARSE_T *)cph_parse_rule_db.entry_ara;
	int              idx     = 0;

	for (idx = 0; idx < cph_parse_rule_db.max_entry_num; idx++, p_entry++) {
		if (strcmp(p_entry->name, name) == 0) {
			p_entry->name[0]  = 0;
			p_entry->parse_bm = 0;
			memset(&p_entry->parse_key, 0, sizeof(p_entry->parse_key));
			return TRUE;
		}
	}
	return FALSE;
}

void cph_sysfs_show_parse_db(void)
{
	struct CPH_SYSFS_PARSE_T *p_entry = (struct CPH_SYSFS_PARSE_T *)cph_parse_rule_db.entry_ara;
	int              idx     = 0;

	for (idx = 0; idx < cph_parse_rule_db.max_entry_num; idx++, p_entry++) {
		if (p_entry->name[0] != 0) {
			pr_info("Parse entry(%d) name(%s)\n", idx, p_entry->name);
			cph_db_display_parse_field(p_entry->parse_bm, &p_entry->parse_key);
		}
	}
}

/********************************************************************************/
/*                          Modification field table                            */
/********************************************************************************/
static struct CPH_SYSFS_MOD_T CPH_SYSFS_MOD_Table[CPH_SYSFS_FIELD_MAX_ENTRY];

static struct CPH_SYSFS_RULE_T cph_mod_rule_db = {
	.max_entry_num    = CPH_SYSFS_FIELD_MAX_ENTRY,
	.entry_num        = 0,
	.entry_size       = sizeof(struct CPH_SYSFS_MOD_T),
	.entry_ara        = CPH_SYSFS_MOD_Table
};

static void cph_sysfs_init_mod_db(void)
{
	struct CPH_SYSFS_MOD_T  *p_entry = (struct CPH_SYSFS_MOD_T *)cph_mod_rule_db.entry_ara;
	int             idx     = 0;

	for (idx = 0; idx < cph_mod_rule_db.max_entry_num; idx++, p_entry++)
		p_entry->name[0] = 0;
}

struct CPH_SYSFS_MOD_T *cph_sysfs_find_mod_entry_by_name(char *name)
{
	struct CPH_SYSFS_MOD_T  *p_entry = (struct CPH_SYSFS_MOD_T *)cph_mod_rule_db.entry_ara;
	int             idx     = 0;

	for (idx = 0; idx < cph_mod_rule_db.max_entry_num; idx++, p_entry++) {
		if (strcmp(p_entry->name, name) == 0)
			return p_entry;
	}
	return 0;
}

struct CPH_SYSFS_MOD_T *cph_sysfs_find_free_mod_entry(void)
{
	struct CPH_SYSFS_MOD_T  *p_entry = (struct CPH_SYSFS_MOD_T *)cph_mod_rule_db.entry_ara;
	int             idx     = 0;

	for (idx = 0; idx < cph_mod_rule_db.max_entry_num; idx++, p_entry++) {
		if (p_entry->name[0] == 0)
			return p_entry;
	}
	return 0;
}

bool cph_sysfs_del_mod_entry_by_name(char *name)
{
	struct CPH_SYSFS_MOD_T  *p_entry = (struct CPH_SYSFS_MOD_T *)cph_mod_rule_db.entry_ara;
	int             idx     = 0;

	for (idx = 0; idx < cph_mod_rule_db.max_entry_num; idx++, p_entry++) {
		if (strcmp(p_entry->name, name) == 0) {
			p_entry->name[0] = 0;
			p_entry->mod_bm  = 0;
			memset(&p_entry->mod_value, 0, sizeof(p_entry->mod_value));
			return TRUE;
		}
	}
	return FALSE;
}

void cph_sysfs_show_mod_db(void)
{
	struct CPH_SYSFS_MOD_T  *p_entry = (struct CPH_SYSFS_MOD_T *)cph_mod_rule_db.entry_ara;
	int             idx     = 0;

	for (idx = 0; idx < cph_mod_rule_db.max_entry_num; idx++, p_entry++) {
		if (p_entry->name[0] != 0) {
			pr_info("Mod entry(%d) name(%s)\n", idx, p_entry->name);
			cph_db_display_mod_field(p_entry->mod_bm, &p_entry->mod_value);
		}
	}
}

/********************************************************************************/
/*                          Forwarding field table                              */
/********************************************************************************/
static struct CPH_SYSFS_FRWD_T cph_sysfs_frwd_table[CPH_SYSFS_FIELD_MAX_ENTRY];

static struct CPH_SYSFS_RULE_T cph_frwd_rule_db = {
	.max_entry_num    = CPH_SYSFS_FIELD_MAX_ENTRY,
	.entry_num        = 0,
	.entry_size       = sizeof(struct CPH_SYSFS_FRWD_T),
	.entry_ara        = cph_sysfs_frwd_table
};

static void cph_sysfs_init_frwd_db(void)
{
	struct CPH_SYSFS_FRWD_T *p_entry = (struct CPH_SYSFS_FRWD_T *)cph_frwd_rule_db.entry_ara;
	int             idx     = 0;

	for (idx = 0; idx < cph_frwd_rule_db.max_entry_num; idx++, p_entry++)
		p_entry->name[0] = 0;
}

struct CPH_SYSFS_FRWD_T *cph_sysfs_find_frwd_entry_by_name(char *name)
{
	struct CPH_SYSFS_FRWD_T *p_entry = (struct CPH_SYSFS_FRWD_T *)cph_frwd_rule_db.entry_ara;
	int             idx     = 0;

	for (idx = 0; idx < cph_frwd_rule_db.max_entry_num; idx++, p_entry++) {
		if (strcmp(p_entry->name, name) == 0)
			return p_entry;
	}
	return 0;
}

struct CPH_SYSFS_FRWD_T *cph_sysfs_find_free_frwd_entry(void)
{
	struct CPH_SYSFS_FRWD_T *p_entry = (struct CPH_SYSFS_FRWD_T *)cph_frwd_rule_db.entry_ara;
	int             idx     = 0;

	for (idx = 0; idx < cph_frwd_rule_db.max_entry_num; idx++, p_entry++) {
		if (p_entry->name[0] == 0)
			return p_entry;
	}
	return 0;
}

bool cph_sysfs_del_frwd_entry_by_name(char *name)
{
	struct CPH_SYSFS_FRWD_T *p_entry = (struct CPH_SYSFS_FRWD_T *)cph_frwd_rule_db.entry_ara;
	int             idx     = 0;

	for (idx = 0; idx < cph_frwd_rule_db.max_entry_num; idx++, p_entry++) {
		if (strcmp(p_entry->name, name) == 0) {
			p_entry->name[0] = 0;
			p_entry->frwd_bm = 0;
			memset(&p_entry->frwd_value, 0, sizeof(p_entry->frwd_value));
			return TRUE;
		}
	}
	return FALSE;
}

void cph_sysfs_show_frwd_db(void)
{
	struct CPH_SYSFS_FRWD_T *p_entry = (struct CPH_SYSFS_FRWD_T *)cph_frwd_rule_db.entry_ara;
	int             idx     = 0;

	for (idx = 0; idx < cph_frwd_rule_db.max_entry_num; idx++, p_entry++) {
		if (p_entry->name[0] != 0) {
			pr_info("Frwd entry(%d) name(%s)\n", idx, p_entry->name);
			cph_db_display_frwd_field(p_entry->frwd_bm, &p_entry->frwd_value);
		}
	}
}

/********************************************************************************/
/*                          SYS FS Parsing Functions                            */
/********************************************************************************/
static ssize_t cph_spec_proc_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int       off  = 0;
	const char *name = attr->attr.name;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (!strcmp(name, "help"))
		off = cph_spec_proc_help(buf);

	if (!strcmp(name, "help_add"))
		off = cph_spec_proc_help_add(buf);
	else if (!strcmp(name, "show_app_db"))
		cph_db_display_all();

	else if (!strcmp(name, "show_parse_name"))
		cph_sysfs_show_parse_db();

	else if (!strcmp(name, "show_mod_name"))
		cph_sysfs_show_mod_db();

	else if (!strcmp(name, "show_frwd_name"))
		cph_sysfs_show_frwd_db();

#ifdef CONFIG_MV_CPH_UDP_SAMPLE_HANDLE
	else if (!strcmp(name, "udp_ports"))
		cph_udp_spec_print_all();
#endif
#ifdef CONFIG_MV_CPH_FLOW_MAP_HANDLE
	else if (!strcmp(name, "show_flow_rule"))
		cph_flow_display_all();

	else if (!strcmp(name, "clear_flow_rule"))
		cph_flow_clear_rule();

	else if (!strcmp(name, "del_dscp_map"))
		cph_flow_del_dscp_map();
#endif
	else
		off = cph_spec_proc_help(buf);

	return off;
}

static ssize_t cph_spec_proc_1_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t len)
{
	const char *name  = attr->attr.name;
	unsigned int      v1    = 0;
	unsigned long       flags = 0;
	MV_STATUS   rc    =  MV_OK;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	/* Read input */
	sscanf(buf, "%x", &v1);

	raw_local_irq_save(flags);

	if (!strcmp(name, "trace_level")) {
		rc = cph_set_trace_flag(v1);
		if (rc == MV_OK)
			pr_err("Succeed to set trace level<0x%x>\n", v1);
		else
			pr_err("Fail to set trace level<0x%x>\n", v1);
	} else
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);

	raw_local_irq_restore(flags);

	return len;
}

static ssize_t cph_spec_proc_2_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t len)
{
	const char *name  = attr->attr.name;
	unsigned int      v1    = 0;
	unsigned int      v2    = 0;
	unsigned long       flags = 0;
	MV_STATUS   rc    =  MV_OK;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	/* Read input */
	sscanf(buf, "%d %d", &v1, &v2);

	raw_local_irq_save(flags);

	if (!strcmp(name, "set_complex")) {
		rc = cph_set_complex_profile(v1, v2);
		if (rc == MV_OK)
			pr_err("Succeed to set complex profile<%d> active port<%d>\n", v1, v2);
		else
			pr_err("Fail to set complex profile<%d> active port<%d>\n", v1, v2);
	} else if (!strcmp(name, "set_flag")) {
		rc = cph_set_feature_flag(v1, v2);
		if (rc == MV_OK)
			pr_err("Succeed to set feature<%d> to <%d>\n", v1, v2);
		else
			pr_err("Fail to set feature<%d> to<%d>\n", v1, v2);
	} else if (!strcmp(name, "set_tcont")) {
		rc = cph_set_tcont_state(v1, v2);
		if (rc == MV_OK)
			pr_err("Succeed to set tcont<%d> to <%d>\n", v1, v2);
		else
			pr_err("Fail to set tcont<%d> to<%d>\n", v1, v2);
	} else
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);

	raw_local_irq_restore(flags);

	return len;
}

static ssize_t cph_spec_proc_name_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t len)
{
	const char *name  = attr->attr.name;
	char        name1[CPH_SYSFS_FIELD_MAX_LEN+1];
	unsigned int      v1    = 0;
	unsigned int      v2    = 0;
	unsigned int      v3    = 0;
	unsigned int      v4    = 0;
	unsigned int      v5    = 0;
	unsigned int      v6    = 0;
	unsigned int      v7    = 0;
	unsigned int      v8    = 0;
	unsigned int      v9    = 0;
	unsigned int      v10   = 0;
	unsigned long       flags = 0;
	struct CPH_SYSFS_PARSE_T *p_parse_entry = NULL;
	struct CPH_SYSFS_MOD_T   *p_mod_entry   = NULL;
	struct CPH_SYSFS_FRWD_T  *p_frwd_entry  = NULL;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (!strcmp(name, "add_parse")) {
		/* Read input */
		sscanf(buf, "%s %x %d %d %d %x %d %d %d %d %d", name1, &v1, &v2, &v3, &v4, &v5, &v6, &v7, &v8, &v9, &v10);

		raw_local_irq_save(flags);

		p_parse_entry = cph_sysfs_find_parse_entry_by_name(name1);
		if (p_parse_entry) {
			pr_err("Already has the parse field by name <%s>\n", name1);
			return -EPERM;
		}

		p_parse_entry = cph_sysfs_find_free_parse_entry();
		if (!p_parse_entry) {
			pr_err("No free parse entry\n");
			return -EPERM;
		}

		strcpy(p_parse_entry->name, name1);
		p_parse_entry->parse_bm              = v1;
		p_parse_entry->parse_key.dir         = v2;
		p_parse_entry->parse_key.rx_tx       = v3;
		p_parse_entry->parse_key.mh          = v4;
		p_parse_entry->parse_key.eth_type    = v5;
		p_parse_entry->parse_key.eth_subtype = v6;
		p_parse_entry->parse_key.ipv4_type   = v7;
		p_parse_entry->parse_key.ipv6_nh1    = v8;
		p_parse_entry->parse_key.ipv6_nh2    = v9;
		p_parse_entry->parse_key.icmpv6_type = v10;

		pr_err("Succeed to add parse field by name <%s>\n", name1);

		raw_local_irq_restore(flags);
	} else if (!strcmp(name, "add_mod")) {
		/* Read input */
		sscanf(buf, "%s %x %x %d", name1, &v1, &v2, &v3);

		raw_local_irq_save(flags);

		p_mod_entry = cph_sysfs_find_mod_entry_by_name(name1);
		if (p_mod_entry) {
			pr_err("Already has the mod field by name <%s>\n", name1);
			return -EPERM;
		}

		p_mod_entry = cph_sysfs_find_free_mod_entry();
		if (!p_mod_entry) {
			pr_err("No free mod entry\n");
			return -EPERM;
		}

		strcpy(p_mod_entry->name, name1);
		p_mod_entry->mod_bm                = v1;
		p_mod_entry->mod_value.proto_type  = v2;
		if (v3)
			p_mod_entry->mod_value.state   = TRUE;
		else
			p_mod_entry->mod_value.state   = FALSE;

		pr_err("Succeed to add mod field by name <%s>\n", name1);

		raw_local_irq_restore(flags);
	} else if (!strcmp(name, "add_frwd")) {
		/* Read input */
		sscanf(buf, "%s %x %d %d %d", name1, &v1, &v2, &v3, &v4);

		raw_local_irq_save(flags);

		p_frwd_entry = cph_sysfs_find_frwd_entry_by_name(name1);
		if (p_frwd_entry) {
			pr_err("Already has the frwd field by name <%s>\n", name1);
			return -EPERM;
		}

		p_frwd_entry = cph_sysfs_find_free_frwd_entry();
		if (!p_frwd_entry) {
			pr_err("No free frwd entry\n");
			return -EPERM;
		}

		strcpy(p_frwd_entry->name, name1);
		p_frwd_entry->frwd_bm              = v1;
		p_frwd_entry->frwd_value.trg_port  = v2;
		p_frwd_entry->frwd_value.trg_queue = v3;
		p_frwd_entry->frwd_value.gem_port  = v4;

		pr_err("Succeed to add frwd field by name <%s>\n", name1);

		raw_local_irq_restore(flags);
	} else
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);

	return len;
}

static ssize_t cph_spec_proc_app_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t len)
{
	const char *name  = attr->attr.name;
	char        name1[CPH_SYSFS_FIELD_MAX_LEN+1];
	char        name2[CPH_SYSFS_FIELD_MAX_LEN+1];
	char        name3[CPH_SYSFS_FIELD_MAX_LEN+1];
	unsigned long       flags = 0;
	MV_STATUS   rc    =  MV_OK;
	struct CPH_SYSFS_PARSE_T *p_parse_entry = NULL;
	struct CPH_SYSFS_MOD_T   *p_mod_entry   = NULL;
	struct CPH_SYSFS_FRWD_T  *p_frwd_entry  = NULL;
	struct CPH_SYSFS_MOD_T    mod_entry;
	struct CPH_SYSFS_FRWD_T   frwd_entry;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	/* Read input */
	sscanf(buf, "%s %s %s", name1, name2, name3);

	raw_local_irq_save(flags);

	if (!strcmp(name, "add_app_rule")) {
		p_parse_entry = cph_sysfs_find_parse_entry_by_name(name1);
		if (!p_parse_entry) {
			pr_err("add_app_rule: invalid parse name <%s>\n", name1);
			return -EPERM;
		}
		p_mod_entry = cph_sysfs_find_mod_entry_by_name(name2);
		if (!p_mod_entry) {
			pr_err("add_app_rule: invalid mod name <%s>\n", name2);
			return -EPERM;
		}
		p_frwd_entry = cph_sysfs_find_frwd_entry_by_name(name3);
		if (!p_frwd_entry) {
			pr_err("add_app_rule: invalid frwd name <%s>\n", name3);
			return -EPERM;
		}

		rc = cph_add_app_rule(p_parse_entry->parse_bm, &p_parse_entry->parse_key,
					p_mod_entry->mod_bm, &p_mod_entry->mod_value,
					p_frwd_entry->frwd_bm, &p_frwd_entry->frwd_value);
		if (rc == MV_OK)
			pr_err("Succeed to add app rule\n");
		else
			pr_err("Fail to add app rule\n");
	} else if (!strcmp(name, "del_app_rule")) {
		p_parse_entry = cph_sysfs_find_parse_entry_by_name(name1);
		if (!p_parse_entry) {
			pr_err("add_app_rule: invalid parse name <%s>\n", name1);
			return -EPERM;
		}

		rc = cph_del_app_rule(p_parse_entry->parse_bm, &p_parse_entry->parse_key);
		if (rc == MV_OK)
			pr_err("Succeed to delete app rule\n");
		else
			pr_err("Fail to delete app rule\n");
	} else if (!strcmp(name, "update_app_rule")) {
		p_parse_entry = cph_sysfs_find_parse_entry_by_name(name1);
		if (!p_parse_entry) {
			pr_err("add_app_rule: invalid parse name <%s>\n", name1);
			return -EPERM;
		}
		p_mod_entry = cph_sysfs_find_mod_entry_by_name(name2);
		if (!p_mod_entry) {
			pr_err("add_app_rule: invalid mod name <%s>\n", name2);
			return -EPERM;
		}
		p_frwd_entry = cph_sysfs_find_frwd_entry_by_name(name3);
		if (!p_frwd_entry) {
			pr_err("add_app_rule: invalid frwd name <%s>\n", name3);
			return -EPERM;
		}

		rc = cph_update_app_rule(p_parse_entry->parse_bm, &p_parse_entry->parse_key,
					p_mod_entry->mod_bm, &p_mod_entry->mod_value,
					p_frwd_entry->frwd_bm, &p_frwd_entry->frwd_value);
		if (rc == MV_OK)
			pr_err("Succeed to update app rule\n");
		else
			pr_err("Fail to update app rule\n");
	} else if (!strcmp(name, "get_app_rule")) {
		p_parse_entry = cph_sysfs_find_parse_entry_by_name(name1);
		if (!p_parse_entry) {
			pr_err("add_app_rule: invalid parse name <%s>\n", name1);
			return -EPERM;
		}

		rc = cph_get_app_rule(p_parse_entry->parse_bm, &p_parse_entry->parse_key,
					&mod_entry.mod_bm, &mod_entry.mod_value,
					&frwd_entry.frwd_bm, &frwd_entry.frwd_value);
		if (rc == MV_OK) {
			cph_db_display_parse_field(p_parse_entry->parse_bm, &p_parse_entry->parse_key);
			cph_db_display_mod_field(mod_entry.mod_bm, &mod_entry.mod_value);
			cph_db_display_frwd_field(frwd_entry.frwd_bm, &frwd_entry.frwd_value);
		} else {
			pr_err("No valid CPH app rule\n");
		}

	} else if (!strcmp(name, "del_parse")) {
		rc = cph_sysfs_del_parse_entry_by_name(name1);
		if (rc == TRUE)
			pr_err("Succeed to delete parse field by name <%s>\n", name1);
		else
			pr_err("Fail to delete parse field by name <%s>\n", name1);
	} else if (!strcmp(name, "del_mod")) {
		rc = cph_sysfs_del_mod_entry_by_name(name1);
		if (rc == TRUE)
			pr_err("Succeed to delete mod field by name <%s>\n", name1);
		else
			pr_err("Fail to delete mod field by name <%s>\n", name1);
	} else if (!strcmp(name, "del_frwd")) {
		rc = cph_sysfs_del_frwd_entry_by_name(name1);
		if (rc == TRUE)
			pr_err("Succeed to delete frwd field by name <%s>\n", name1);
		else
			pr_err("Fail to delete frwd field by name <%s>\n", name1);
	} else
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);

	raw_local_irq_restore(flags);

	return len;
}

#ifdef CONFIG_MV_CPH_UDP_SAMPLE_HANDLE
static ssize_t cph_spec_proc_udp_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t len)
{
	const char *name  = attr->attr.name;
	unsigned int      v1    = 0;
	unsigned int      v2    = 0;
	unsigned int      v3    = 0;
	unsigned int      v4    = 0;
	unsigned int      v5    = 0;
	unsigned int      v6    = 0;
	unsigned long       flags = 0;
	MV_STATUS   rc    =  MV_OK;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	/* Read input */
	sscanf(buf, "%d %d %x %x %x %x", &v1, &v2, &v3, &v4, &v5, &v6);

	raw_local_irq_save(flags);

	if (!strcmp(name, "udp_src")) {
		rc = cph_udp_src_spec_set(v1, v2, v3, v4, v5, v6);
		if (rc == MV_OK)
			pr_err("Succeed to add UDP src rule\n");
		else
			pr_err("Fail to add UDP src rule\n");
	} else if (!strcmp(name, "udp_dst")) {
		rc = cph_udp_dest_spec_set(v1, v2, v3, v4, v5, v6);
		if (rc == MV_OK)
			pr_err("Succeed to add UDP dest rule\n");
		else
			pr_err("Fail to add UDP dest rule\n");
	} else
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);

	raw_local_irq_restore(flags);

	return len;
}
#endif

#ifdef CONFIG_MV_CPH_FLOW_MAP_HANDLE
static ssize_t cph_spec_proc_flow_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t len)
{
	const char    *name  = attr->attr.name;
	unsigned int         v0    = 0;
	unsigned int         v1    = 0;
	unsigned int         v2    = 0;
	unsigned int         v3    = 0;
	unsigned int         v4    = 0;
	unsigned int         v5    = 0;
	unsigned int         v6    = 0;
	unsigned int         v7    = 0;
	unsigned int         v8    = 0;
	unsigned int         v9    = 0;
	unsigned int         v10   = 0;
	unsigned int         v11   = 0;
	unsigned int         v12   = 0;
	unsigned int         v13   = 0;
	unsigned int         v14   = 0;
	unsigned int         v15   = 0;
	unsigned int         v16   = 0;
	unsigned int         v17   = 0;
	unsigned int         v18   = 0;
	unsigned int         v19   = 0;
	unsigned int         v20   = 0;
	unsigned int         v21   = 0;
	unsigned long          flags = 0;
	MV_STATUS      rc    =  MV_OK;
	struct CPH_FLOW_ENTRY_T cph_flow;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	/* Read input */
	sscanf(buf, "%d %d %x %x %x %x %d %d %x %d %d %d %x %d %d %x %d %d %d %d %d %d", &v0, &v1, &v2, &v3, &v4, &v5, &v6, &v7, &v8, &v9, &v10, &v11, &v12, &v13, &v14, &v15, &v16, &v17, &v18, &v19, &v20, &v21);

	raw_local_irq_save(flags);

#ifdef CONFIG_MV_CPH_FLOW_MAP_HANDLE
	if (!strcmp(name, "add_flow_rule")) {
		memset(&cph_flow, 0, sizeof(cph_flow));
		cph_flow.dir        = (enum CPH_DIR_E)v0;
		cph_flow.is_default = v1 ? TRUE : FALSE;
		cph_flow.parse_bm   = (enum CPH_FLOW_PARSE_E)v2;
		cph_flow.mh         = (unsigned short)v3;
		cph_flow.eth_type   = (unsigned short)v4;
		cph_flow.parse_outer_tci.tpid   = (unsigned short)v5;
		cph_flow.parse_outer_tci.vid    = (unsigned short)v6;
		cph_flow.parse_outer_tci.pbits  = (unsigned char)v7;
		cph_flow.parse_inner_tci.tpid   = (unsigned short)v8;
		cph_flow.parse_inner_tci.vid    = (unsigned short)v9;
		cph_flow.parse_inner_tci.pbits  = (unsigned char)v10;
		cph_flow.op_type                = (enum CPH_VLAN_OP_TYPE_E)v11;
		cph_flow.mod_outer_tci.tpid     = (unsigned short)v12;
		cph_flow.mod_outer_tci.vid      = (unsigned short)v13;
		cph_flow.mod_outer_tci.pbits    = (unsigned char)v14;
		cph_flow.mod_inner_tci.tpid     = (unsigned short)v15;
		cph_flow.mod_inner_tci.vid      = (unsigned short)v16;
		cph_flow.mod_inner_tci.pbits    = (unsigned char)v17;
		cph_flow.pkt_frwd.trg_port      = (unsigned char)v18;
		cph_flow.pkt_frwd.trg_queue     = (unsigned char)v19;
		cph_flow.pkt_frwd.trg_hwf_queue = (unsigned char)v20;
		cph_flow.pkt_frwd.gem_port      = (unsigned short)v21;

		rc = cph_flow_add_rule(&cph_flow);
		if (rc == MV_OK)
			pr_err("Succeed to add flow mapping rule\n");
		else
			pr_err("Fail to add flow mapping rule\n");
	} else if (!strcmp(name, "del_flow_rule")) {
		memset(&cph_flow, 0, sizeof(cph_flow));
		cph_flow.dir        = (enum CPH_DIR_E)v0;
		cph_flow.is_default = v1 ? TRUE : FALSE;
		cph_flow.parse_bm   = (enum CPH_FLOW_PARSE_E)v2;
		cph_flow.mh         = (unsigned short)v3;
		cph_flow.eth_type   = (unsigned short)v4;
		cph_flow.parse_outer_tci.tpid   = (unsigned short)v5;
		cph_flow.parse_outer_tci.vid    = (unsigned short)v6;
		cph_flow.parse_outer_tci.pbits  = (unsigned char)v7;
		cph_flow.parse_inner_tci.tpid   = (unsigned short)v8;
		cph_flow.parse_inner_tci.vid    = (unsigned short)v9;
		cph_flow.parse_inner_tci.pbits  = (unsigned char)v10;

		rc = cph_flow_del_rule(&cph_flow);
		if (rc == MV_OK)
			pr_err("Succeed to delete flow mapping rule\n");
		else
			pr_err("Fail to delete flow mapping rule\n");
	} else if (!strcmp(name, "get_flow_rule")) {
		memset(&cph_flow, 0, sizeof(cph_flow));
		cph_flow.dir        = (enum CPH_DIR_E)v0;
		cph_flow.is_default = v1 ? TRUE : FALSE;
		cph_flow.parse_bm   = (enum CPH_FLOW_PARSE_E)v2;
		cph_flow.mh         = (unsigned short)v3;
		cph_flow.eth_type   = (unsigned short)v4;
		cph_flow.parse_outer_tci.tpid   = (unsigned short)v5;
		cph_flow.parse_outer_tci.vid    = (unsigned short)v6;
		cph_flow.parse_outer_tci.pbits  = (unsigned char)v7;
		cph_flow.parse_inner_tci.tpid   = (unsigned short)v8;
		cph_flow.parse_inner_tci.vid    = (unsigned short)v9;
		cph_flow.parse_inner_tci.pbits  = (unsigned char)v10;

		rc = cph_flow_get_rule(&cph_flow);
		if (rc == MV_OK) {
			pr_err("Succeed to get flow rule\n");
			pr_info("                        |Parse outer       |Parse inner       |Mod outer         |Mod Inner         |Forward\n");
			pr_info("dir default tparse_bm mh   ety    tpid   vid  pbits  tpid   vid  pbits  tpid   vid  pbits  tpid   vid  pbits  port queue hwf_queue gem  op_type\n");
			pr_info(
			"%2.2s  %4.4s    0x%04x   %-4d 0x%04x 0x%04x %4d %1d      0x%04x %4d %1d      0x%04x %4d %1d      0x%04x %4d %1d      %1d    %1d     %1d         %4d %s\n",
			cph_app_lookup_dir(cph_flow.dir), (cph_flow.is_default == TRUE) ? "Yes" : "No",
			cph_flow.parse_bm, cph_flow.mh, cph_flow.eth_type,
			cph_flow.parse_outer_tci.tpid, cph_flow.parse_outer_tci.vid, cph_flow.parse_outer_tci.pbits,
			cph_flow.parse_inner_tci.tpid, cph_flow.parse_inner_tci.vid, cph_flow.parse_inner_tci.pbits,
			cph_flow.mod_outer_tci.tpid,   cph_flow.mod_outer_tci.vid,   cph_flow.mod_outer_tci.pbits,
			cph_flow.mod_inner_tci.tpid,   cph_flow.mod_inner_tci.vid,   cph_flow.mod_inner_tci.pbits,
			cph_flow.pkt_frwd.trg_port,    cph_flow.pkt_frwd.trg_queue,  cph_flow.pkt_frwd.trg_hwf_queue,
			cph_flow.pkt_frwd.gem_port, cph_flow_lookup_op_type(cph_flow.op_type));
		} else {
			pr_err("Fail to get flow\n");
		}
	} else
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);
#endif

	raw_local_irq_restore(flags);

	return len;
}

static ssize_t cph_spec_proc_dscp_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t len)
{
	const char      *name = attr->attr.name;
	unsigned int           v[64];
	unsigned int           index = 0;
	unsigned long            flags = 0;
	MV_STATUS        rc    =  MV_OK;
	struct CPH_DSCP_PBITS_T dscp_map;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	/* Read input */
	sscanf(buf, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d ",
	       &v[0],  &v[1],  &v[2],  &v[3],  &v[4],  &v[5],  &v[6],  &v[7],
	       &v[8],  &v[9],  &v[10], &v[11], &v[12], &v[13], &v[14], &v[15],
	       &v[16], &v[17], &v[18], &v[19], &v[20], &v[21], &v[22], &v[23],
	       &v[24], &v[25], &v[26], &v[27], &v[28], &v[29], &v[30], &v[31],
	       &v[32], &v[33], &v[34], &v[35], &v[36], &v[37], &v[38], &v[39],
	       &v[40], &v[41], &v[42], &v[43], &v[44], &v[45], &v[46], &v[47],
	       &v[48], &v[49], &v[50], &v[51], &v[52], &v[53], &v[54], &v[55],
	       &v[56], &v[57], &v[58], &v[59], &v[60], &v[61], &v[62], &v[63]);
	for (index = 0; index < 64; index++)
		dscp_map.pbits[index] = (unsigned char)v[index];

	dscp_map.in_use = TRUE;

	raw_local_irq_save(flags);

	if (!strcmp(name, "set_dscp_map")) {
		rc = cph_flow_set_dscp_map(&dscp_map);
		if (rc == MV_OK)
			pr_err("Succeed to set DSCP to P-bits mapping\n");
		else
			pr_err("Fail to set DSCP to P-bits mapping\n");
	} else
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);

	raw_local_irq_restore(flags);

	return len;
}
#endif

static ssize_t cph_port_func_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t len)
{
	const char *name  = attr->attr.name;
	unsigned int      v1    = 0;
	unsigned int      v2    = 0;
	unsigned int      v3    = 0;
	unsigned long       flags = 0;
	MV_STATUS   rc    =  MV_OK;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	/* Read input */
	sscanf(buf, "%d %d %d", &v1, &v2, &v3);

	raw_local_irq_save(flags);

	if (!strcmp(name, "set_port_func")) {
		rc = cph_set_port_func(v1, v2, v3);
		if (rc == MV_OK)
			pr_err("Succeed to set cph port<%d> func\n", v1);
		else
			pr_err("Fail to set cph port port<%d> func\n", v1);
	} else
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);

	raw_local_irq_restore(flags);

	return len;
}

static ssize_t cph_port_func_get(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t len)
{
	const char *name        = attr->attr.name;
	unsigned int v1         = 0;
	unsigned long flags     = 0;
	bool rx_enable, tx_enable;
	MV_STATUS   rc    =  MV_OK;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	/* Read input */
	sscanf(buf, "%d", &v1);

	raw_local_irq_save(flags);

	if (!strcmp(name, "get_port_func")) {
		rc = cph_get_port_func(v1, &rx_enable, &tx_enable);
		if (rc != MV_OK)
			pr_err("input port<%d> is error!\n", v1);
		else {
			pr_err("port port<%d> cph func rx <%s>, tx<%s>.\n", v1,
								rx_enable ? "enabled" : "disbaled",
								tx_enable ? "enabled" : "disbaled");
		}
	} else
		pr_err("%s: illegal operation <%s>\n", __func__, attr->attr.name);

	raw_local_irq_restore(flags);

	return len;
}


static DEVICE_ATTR(help,            S_IRUSR, cph_spec_proc_show, NULL);
static DEVICE_ATTR(help_add,        S_IRUSR, cph_spec_proc_show, NULL);
static DEVICE_ATTR(show_app_db,     S_IRUSR, cph_spec_proc_show, NULL);
static DEVICE_ATTR(show_parse_name, S_IRUSR, cph_spec_proc_show, NULL);
static DEVICE_ATTR(show_mod_name,   S_IRUSR, cph_spec_proc_show, NULL);
static DEVICE_ATTR(show_frwd_name,  S_IRUSR, cph_spec_proc_show, NULL);
#ifdef CONFIG_MV_CPH_UDP_SAMPLE_HANDLE
static DEVICE_ATTR(udp_ports,       S_IRUSR, cph_spec_proc_show, NULL);
#endif
#ifdef CONFIG_MV_CPH_FLOW_MAP_HANDLE
static DEVICE_ATTR(show_flow_rule,  S_IRUSR, cph_spec_proc_show, NULL);
static DEVICE_ATTR(clear_flow_rule, S_IRUSR, cph_spec_proc_show, NULL);
static DEVICE_ATTR(del_dscp_map,    S_IRUSR, cph_spec_proc_show, NULL);
#endif
static DEVICE_ATTR(set_port_func,   S_IWUSR, cph_spec_proc_show, cph_port_func_store);
static DEVICE_ATTR(get_port_func,   S_IWUSR, cph_spec_proc_show, cph_port_func_get);
static DEVICE_ATTR(set_complex,     S_IWUSR, cph_spec_proc_show, cph_spec_proc_2_store);
static DEVICE_ATTR(set_flag,        S_IWUSR, cph_spec_proc_show, cph_spec_proc_2_store);
static DEVICE_ATTR(add_parse,       S_IWUSR, cph_spec_proc_show, cph_spec_proc_name_store);
static DEVICE_ATTR(del_parse,       S_IWUSR, cph_spec_proc_show, cph_spec_proc_app_store);
static DEVICE_ATTR(add_mod,         S_IWUSR, cph_spec_proc_show, cph_spec_proc_name_store);
static DEVICE_ATTR(del_mod,         S_IWUSR, cph_spec_proc_show, cph_spec_proc_app_store);
static DEVICE_ATTR(add_frwd,        S_IWUSR, cph_spec_proc_show, cph_spec_proc_name_store);
static DEVICE_ATTR(del_frwd,        S_IWUSR, cph_spec_proc_show, cph_spec_proc_app_store);
static DEVICE_ATTR(add_app_rule,    S_IWUSR, cph_spec_proc_show, cph_spec_proc_app_store);
static DEVICE_ATTR(del_app_rule,    S_IWUSR, cph_spec_proc_show, cph_spec_proc_app_store);
static DEVICE_ATTR(update_app_rule, S_IWUSR, cph_spec_proc_show, cph_spec_proc_app_store);
static DEVICE_ATTR(get_app_rule,    S_IWUSR, cph_spec_proc_show, cph_spec_proc_app_store);
#ifdef CONFIG_MV_CPH_UDP_SAMPLE_HANDLE
static DEVICE_ATTR(udp_src,         S_IWUSR, cph_spec_proc_show, cph_spec_proc_udp_store);
static DEVICE_ATTR(udp_dst,         S_IWUSR, cph_spec_proc_show, cph_spec_proc_udp_store);
#endif
#ifdef CONFIG_MV_CPH_FLOW_MAP_HANDLE
static DEVICE_ATTR(add_flow_rule,   S_IWUSR, cph_spec_proc_show, cph_spec_proc_flow_store);
static DEVICE_ATTR(del_flow_rule,   S_IWUSR, cph_spec_proc_show, cph_spec_proc_flow_store);
static DEVICE_ATTR(get_flow_rule,   S_IWUSR, cph_spec_proc_show, cph_spec_proc_flow_store);
static DEVICE_ATTR(set_dscp_map,    S_IWUSR, cph_spec_proc_show, cph_spec_proc_dscp_store);
#endif
static DEVICE_ATTR(set_tcont,       S_IWUSR, cph_spec_proc_show, cph_spec_proc_2_store);
static DEVICE_ATTR(trace_level,     S_IWUSR, cph_spec_proc_show, cph_spec_proc_1_store);


static struct attribute *cph_spec_proc_attrs[] = {
	&dev_attr_help.attr,
	&dev_attr_help_add.attr,
	&dev_attr_show_app_db.attr,
	&dev_attr_show_parse_name.attr,
	&dev_attr_show_mod_name.attr,
	&dev_attr_show_frwd_name.attr,
#ifdef CONFIG_MV_CPH_UDP_SAMPLE_HANDLE
	&dev_attr_udp_ports.attr,
#endif
#ifdef CONFIG_MV_CPH_FLOW_MAP_HANDLE
	&dev_attr_show_flow_rule.attr,
	&dev_attr_clear_flow_rule.attr,
	&dev_attr_del_dscp_map.attr,
#endif
	&dev_attr_set_port_func.attr,
	&dev_attr_get_port_func.attr,
	&dev_attr_set_complex.attr,
	&dev_attr_set_flag.attr,
	&dev_attr_add_parse.attr,
	&dev_attr_del_parse.attr,
	&dev_attr_add_mod.attr,
	&dev_attr_del_mod.attr,
	&dev_attr_add_frwd.attr,
	&dev_attr_del_frwd.attr,
	&dev_attr_add_app_rule.attr,
	&dev_attr_del_app_rule.attr,
	&dev_attr_update_app_rule.attr,
	&dev_attr_get_app_rule.attr,
#ifdef CONFIG_MV_CPH_UDP_SAMPLE_HANDLE
	&dev_attr_udp_src.attr,
	&dev_attr_udp_dst.attr,
#endif
#ifdef CONFIG_MV_CPH_FLOW_MAP_HANDLE
	&dev_attr_add_flow_rule.attr,
	&dev_attr_del_flow_rule.attr,
	&dev_attr_get_flow_rule.attr,
	&dev_attr_set_dscp_map.attr,
#endif
	&dev_attr_set_tcont.attr,
	&dev_attr_trace_level.attr,

	NULL
};

static struct attribute_group cph_spec_proc_group = {
	.name = "proto",
	.attrs = cph_spec_proc_attrs,
};

int cph_sysfs_init(void)
{
	int          err = 0;
	struct device *pd  = NULL;

	pd = bus_find_device_by_name(&platform_bus_type, NULL, "cph");
	if (!pd) {
		platform_device_register_simple("cph", -1, NULL, 0);
		pd = bus_find_device_by_name(&platform_bus_type, NULL, "cph");
	}

	if (!pd) {
		pr_err("%s: cannot find cph device\n", __func__);
		pd = &platform_bus;
	}

	err = sysfs_create_group(&pd->kobj, &cph_spec_proc_group);
	if (err) {
		pr_info("sysfs group failed %d\n", err);
		goto out;
	}

	/* Init CPH SYS FS data base to hold parse/mod/frwd values */
	cph_sysfs_init_parse_db();
	cph_sysfs_init_mod_db();
	cph_sysfs_init_frwd_db();

out:
	return err;
}

void cph_sysfs_exit(void)
{
	struct device *pd = NULL;

	pd = bus_find_device_by_name(&platform_bus_type, NULL, "cph");
	if (!pd) {
		pr_err("%s: cannot find CPH device\n", __func__);
		return;
	}

	sysfs_remove_group(&pd->kobj, &cph_spec_proc_group);
}
