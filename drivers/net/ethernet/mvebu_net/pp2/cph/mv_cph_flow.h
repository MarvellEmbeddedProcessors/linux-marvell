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
* mv_cph_flow.h
*
* DESCRIPTION: Marvell CPH(CPH Packet Handler) flow module to handle the
*              flow mapping, VLAN modification of data traffic
*
* DEPENDENCIES:
*               None
*
* CREATED BY:   VictorGu
*
* DATE CREATED: 12Dec2011
*
* FILE REVISION NUMBER:
*               Revision: 1.1
*
*
*******************************************************************************/
#ifndef _MV_CPH_FLOW_H_
#define _MV_CPH_FLOW_H_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
*                        Data Enum and Structure
******************************************************************************/
#define MV_CPH_TPID_NOT_CARE_VALUE       (0)     /* Does not care for TPID     */
#define MV_CPH_ZERO_VALUE                (0)     /* zero value                 */
#define MV_CPH_VID_NOT_CARE_VALUE        (4096)  /* Does not care for VID      */
#define MV_CPH_PBITS_NOT_CARE_VALUE      (8)     /* Does not care for P-bits   */
#define MV_CPH_DSCP_NOT_CARE_VALUE       (64)    /* Does not care for DSCP     */

#define MV_CPH_TPID_INVALID_VALUE        (0xFFFF)/* No valid TPID              */
#define MV_CPH_VID_INVALID_VALUE         (0xFFFF)/* No valid VID               */
#define MV_CPH_PBITS_INVALID_VALUE       (0xFF)  /* No valid P-bits            */
#define MV_CPH_DSCP_INVALID_VALUE        (0xFF)  /* No valid DSCP              */

#define MV_CPH_DEFAULT_UNTAG_RULE        (4096+1)/* Default untagged  rule     */
#define MV_CPH_DEFAULT_SINGLE_TAG_RULE   (4096+2)/* Default sinlge tagged  rule*/
#define MV_CPH_DEFAULT_DOUBLE_TAG_RULE   (4096+3)/* Default double tagged  rule*/

#define MV_CPH_PBITS_TABLE_INVALID_INDEX (0xFF)     /* Invalid Pbits table index value in VID index table*/

/* VLAN ID index table definition for flow mapping */
#define MV_CPH_VID_INDEX_TABLE_MAX_SIZE  (4096+4)
struct CPH_VID_IDX_TBL_T {
	unsigned char pbit_tbl_idx[MV_CPH_VID_INDEX_TABLE_MAX_SIZE];
};

/* P-bits flow mapping table definition */
#define MV_CPH_RULE_NUM_PER_ENTRY   (16)
struct CPH_PBITS_ENTRY_T {
	unsigned short    num;        /* total valid cph flow rule number */
	unsigned short    rule_idx[MV_CPH_RULE_NUM_PER_ENTRY]; /* index to flow rule */
};

#define MV_CPH_PBITS_MAP_MAX_ENTRY_NUM   (8+1)
#define MV_CPH_MAX_PBITS_MAP_TABLE_SIZE  (64)
#define MV_CPH_RESERVED_PBITS_TABLE_NUM  (4)

struct CPH_PBITS_TABLE_T {
	bool               in_use;
	struct CPH_PBITS_ENTRY_T  flow_rule[MV_CPH_PBITS_MAP_MAX_ENTRY_NUM];
	struct CPH_PBITS_ENTRY_T  def_flow_rule[MV_CPH_PBITS_MAP_MAX_ENTRY_NUM];
};

/* CPH flow mapping rule definition
------------------------------------------------------------------------------*/
enum CPH_VLAN_OP_TYPE_E {
	CPH_VLAN_OP_ASIS                               = 0,
	CPH_VLAN_OP_DISCARD                            = 1,
	CPH_VLAN_OP_ADD                                = 2,
	CPH_VLAN_OP_ADD_COPY_DSCP                      = 3,
	CPH_VLAN_OP_ADD_COPY_OUTER_PBIT                = 4,
	CPH_VLAN_OP_ADD_COPY_INNER_PBIT                = 5,
	CPH_VLAN_OP_ADD_2_TAGS                         = 6,
	CPH_VLAN_OP_ADD_2_TAGS_COPY_DSCP               = 7,
	CPH_VLAN_OP_ADD_2_TAGS_COPY_PBIT               = 8,
	CPH_VLAN_OP_REM                                = 9,
	CPH_VLAN_OP_REM_2_TAGS                         = 10,
	CPH_VLAN_OP_REPLACE                            = 11,
	CPH_VLAN_OP_REPLACE_VID                        = 12,
	CPH_VLAN_OP_REPLACE_PBIT                       = 13,
	CPH_VLAN_OP_REPLACE_INNER_ADD_OUTER            = 14,
	CPH_VLAN_OP_REPLACE_INNER_ADD_OUTER_COPY_PBIT  = 15,
	CPH_VLAN_OP_REPLACE_INNER_REM_OUTER            = 16,
	CPH_VLAN_OP_REPLACE_2TAGS                      = 17,
	CPH_VLAN_OP_REPLACE_2TAGS_VID                  = 18,
	CPH_VLAN_OP_SWAP                               = 19
};

struct CPH_FLOW_FRWD_T {
	unsigned char  trg_port;
	unsigned char  trg_queue;
	unsigned char  trg_hwf_queue;
	unsigned short gem_port;
};

struct CPH_FLOW_TCI_T {
	unsigned short  tpid;
	unsigned short  vid;
	unsigned char   pbits;
};

enum CPH_FLOW_PARSE_E {
	CPH_FLOW_PARSE_MH        = 0x01,  /* parsing Marvell header                          */
	CPH_FLOW_PARSE_EXT_VLAN  = 0x02,  /* parsing external VLAN tag                       */
	CPH_FLOW_PARSE_TWO_VLAN  = 0x04,  /* parsing both of external and internal VLAN tags */
	CPH_FLOW_PARSE_ETH_TYPE  = 0x08,  /* parsing Ethernet type                           */
	CPH_FLOW_PARSE_MC_PROTO  = 0x10,  /* parsing multicast protocol                      */
};

enum CPH_TCI_FIELD_E {
	CPH_TCI_FIELD_VID,
	CPH_TCI_FIELD_CFI,
	CPH_TCI_FIELD_PBIT,
	CPH_TCI_FIELD_VID_PBIT,
	CPH_TCI_FIELD_ALL,
};

struct CPH_FLOW_ENTRY_T {
	bool               valid;
	enum CPH_DIR_E          dir;
	enum CPH_FLOW_PARSE_E   parse_bm;
	bool               is_default;
	unsigned short             mh;
	struct CPH_FLOW_TCI_T     parse_outer_tci;
	struct CPH_FLOW_TCI_T     parse_inner_tci;
	unsigned short             eth_type;
	enum CPH_VLAN_OP_TYPE_E op_type;
	struct CPH_FLOW_TCI_T     mod_outer_tci;
	struct CPH_FLOW_TCI_T     mod_inner_tci;
	struct CPH_FLOW_FRWD_T    pkt_frwd;
	unsigned int             count;
};

#define CPH_FLOW_ENTRY_NUM   (512)

struct CPH_FLOW_TABLE_T {
	unsigned int             rule_num;
	struct CPH_FLOW_ENTRY_T   flow_rule[CPH_FLOW_ENTRY_NUM];
};

/* DSCP to P-bits mapping table definition
------------------------------------------------------------------------------*/
#define MV_CPH_DSCP_PBITS_TABLE_MAX_SIZE  (64)
struct CPH_DSCP_PBITS_T {
	unsigned int in_use;
	unsigned char  pbits[MV_CPH_DSCP_PBITS_TABLE_MAX_SIZE];
};

/* CPH flow database
------------------------------------------------------------------------------*/
struct CPH_FLOW_DB_T {
	spinlock_t         flow_lock;
	struct CPH_VID_IDX_TBL_T  vid_idx_tbl[CPH_DIR_NUM];
	struct CPH_PBITS_TABLE_T  pbits_tbl[CPH_DIR_NUM][MV_CPH_MAX_PBITS_MAP_TABLE_SIZE];
	struct CPH_FLOW_TABLE_T   flow_tbl;
	struct CPH_DSCP_PBITS_T   dscp_tbl;
};

/******************************************************************************
 *                        Function Declaration
 ******************************************************************************/
/******************************************************************************
* cph_flow_add_rule()
* _____________________________________________________________________________
*
* DESCRIPTION: Sets flow mapping rule
*
* INPUTS:
*       cph_flow - VLAN ID, 802.1p value, pkt_fwd information.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
int cph_flow_add_rule(struct CPH_FLOW_ENTRY_T *cph_flow);

/******************************************************************************
* cph_flow_del_rule()
* _____________________________________________________________________________
*
* DESCRIPTION: Deletes flow mapping rule
*
* INPUTS:
*       cph_flow - VLAN ID, 802.1p value, pkt_fwd information.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
int  cph_flow_del_rule(struct CPH_FLOW_ENTRY_T *cph_flow);

/******************************************************************************
* cph_flow_clear_rule()
* _____________________________________________________________________________
*
* DESCRIPTION: Clears all flow mapping rules
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
int cph_flow_clear_rule(void);

/******************************************************************************
* cph_flow_clear_rule_by_mh()
* _____________________________________________________________________________
*
* DESCRIPTION: Clears flow mapping rules by MH
*
* INPUTS:
*       mh   -  Marvell header.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
int cph_flow_clear_rule_by_mh(unsigned short mh);

/******************************************************************************
* cph_flow_get_tag_rule()
* _____________________________________________________________________________
*
* DESCRIPTION: Gets flow mapping rule for tagged frames.
*
* INPUTS:
*       cph_flow - Input vid, pbits, dir
*
* OUTPUTS:
*       cph_flow - output packet forwarding information, including GEM port,
*                  T-CONT, queue and packet modification for VID, P-bits.
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
int  cph_flow_get_rule(struct CPH_FLOW_ENTRY_T *cph_flow);

/******************************************************************************
* cph_flow_db_get_rule_by_vid()
* _____________________________________________________________________________
*
* DESCRIPTION: Get CPH flow mapping rule by VID, only used to compare packet and db rule.
*
* INPUTS:
*       cph_flow   - Flow parsing field values
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
MV_STATUS cph_flow_db_get_rule_by_vid(struct CPH_FLOW_ENTRY_T *cph_flow);

/******************************************************************************
* cph_flow_set_dscp_map()
* _____________________________________________________________________________
*
* DESCRIPTION: Sets DSCP to P-bits mapping rules
*
* INPUTS:
*       dscp_map  - DSCP to P-bits mapping rules.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
int cph_flow_set_dscp_map(struct CPH_DSCP_PBITS_T *dscp_map);

/******************************************************************************
* cph_flow_del_dscp_map()
* _____________________________________________________________________________
*
* DESCRIPTION: Deletes DSCP to P-bits mapping rules
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
int cph_flow_del_dscp_map(void);

/******************************************************************************
* cph_flow_add_vlan()
* _____________________________________________________________________________
*
* DESCRIPTION: Add one VLAN tag behind of source MAC address.
*
* INPUTS:
*       mh     - Whether has MH or not
*       p_data - Pointer to packet
*       tpid   - Type of VLAN ID
*       vid    - VLAN to be added
*       pbits  - P-bits value
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       The shift of SKB data.
*******************************************************************************/
INLINE int cph_flow_add_vlan(bool mh, unsigned char *p_data, unsigned short tpid,
	unsigned short vid, unsigned char pbits);

/******************************************************************************
* cph_flow_del_vlan()
* _____________________________________________________________________________
*
* DESCRIPTION: Delete one VLAN tag behind of source MAC address.
*
* INPUTS:
*       mh     - Whether has MH or not
*       p_data - Pointer to packet.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       The shift of SKB data.
*******************************************************************************/
INLINE int cph_flow_del_vlan(bool mh, unsigned char *p_data);

/******************************************************************************
* cph_flow_replace_vlan()
* _____________________________________________________________________________
*
* DESCRIPTION: Replace one VLAN tag behind of source MAC address.
*
* INPUTS:
*       mh     - Whether has MH or not
*       p_data - Pointer to packet
*       tpid   - Type of VLAN ID
*       vid    - VLAN to be added
*       pbits  - P-bits value
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       The shift of SKB data.
*******************************************************************************/
INLINE int cph_flow_replace_vlan(bool mh, unsigned char *p_data, unsigned short tpid,
	unsigned short vid, unsigned char pbits);

/******************************************************************************
* cph_flow_swap_vlan()
* _____________________________________________________________________________
*
* DESCRIPTION: Swap between two VLAN tag.
*
* INPUTS:
*       mh     - Whether has MH or not
*       p_data - Pointer to packet
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       The shift of SKB data.
*******************************************************************************/
INLINE int cph_flow_swap_vlan(bool mh, unsigned char *p_data);

/******************************************************************************
* cph_flow_strip_vlan()
* _____________________________________________________________________________
*
* DESCRIPTION: Delete all VLAN tags behind of source MAC address.
*
* INPUTS:
*       mh     - Whether has MH or not
*       p_data - Pointer to packet.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       The shift of SKB data.
*******************************************************************************/
INLINE int cph_flow_strip_vlan(bool mh, unsigned char *p_data);

/******************************************************************************
* cph_flow_compare_rules()
* _____________________________________________________________________________
*
* DESCRIPTION: Comparse two flow rules.
*
* INPUTS:
*       parse_rule  - The parsing field values come from the packets
*       db_rule     - The flow rule stored in flow database
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       In case same, return TRUE,
*       In case different, return FALSE.
*******************************************************************************/
bool cph_flow_compare_rules(struct CPH_FLOW_ENTRY_T *parse_rule, struct CPH_FLOW_ENTRY_T *db_rule);

/******************************************************************************
* cph_flow_compare_packet_and_rule()
* _____________________________________________________________________________
*
* DESCRIPTION: Compare flow packet and rule.
*
* INPUTS:
*       packet_rule - The parsing field values come from the packets
*       db_rule     - The flow rule stored in flow database
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       In case same, return TRUE,
*       In case different, return FALSE.
*******************************************************************************/
bool cph_flow_compare_packet_and_rule(struct CPH_FLOW_ENTRY_T *packet_rule, struct CPH_FLOW_ENTRY_T *db_rule);

/******************************************************************************
* cph_flow_compare_packet_and_rule_vid()
* _____________________________________________________________________________
*
* DESCRIPTION: Compare flow packet and rule w/ only VID.
*
* INPUTS:
*       packet_rule - The parsing field values come from the packets
*       db_rule     - The flow rule stored in flow database
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       In case same, return TRUE,
*       In case different, return FALSE.
*******************************************************************************/
bool cph_flow_compare_packet_and_rule_vid(struct CPH_FLOW_ENTRY_T *packet_rule, struct CPH_FLOW_ENTRY_T *db_rule);

/******************************************************************************
* cph_flow_parse_packet()
* _____________________________________________________________________________
*
* DESCRIPTION: Parse packet and output flow information.
*
* INPUTS:
*       port - Source GMAC port
*       data - Pointer to packet
*       rx   - Whether in RX dir
*
* OUTPUTS:
*       flow - Flow parsing field values
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
MV_STATUS cph_flow_parse_packet(int port, unsigned char *data, bool rx, bool mh, struct CPH_FLOW_ENTRY_T *flow);

/******************************************************************************
* cph_flow_mod_packet()
* _____________________________________________________________________________
*
* DESCRIPTION: Modify packet according to flow rule
*
* INPUTS:
*       skb        - Pointer to packet
*       mh         - Whether has MH or not
*       flow       - Flow parsing field values
*       out_offset - Offset of packet
*       rx         - Whether RX or TX
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
MV_STATUS cph_flow_mod_packet(struct sk_buff *skb,  bool mh, struct CPH_FLOW_ENTRY_T *flow, int *out_offset);

/******************************************************************************
* cph_flow_mod_frwd()
* _____________________________________________________________________________
*
* DESCRIPTION: Modify forwarding parameter of transmiting packet according to flow rule
*
* INPUTS:
*       flow        - Flow parsing field values
*       tx_spec_out - TX descriptor
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
MV_STATUS cph_flow_mod_frwd(struct CPH_FLOW_ENTRY_T *flow, struct mv_eth_tx_spec *tx_spec_out);

/******************************************************************************
* cph_flow_send_packet()
* _____________________________________________________________________________
*
* DESCRIPTION: CPH function to handle the received application packets
*
* INPUTS:
*       dev_out     - Net device
*       pkt         - Marvell packet information
*       tx_spec_out - TX descriptor
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success, the function returns 1.
*       On error returns 0.
*******************************************************************************/
MV_STATUS cph_flow_send_packet(struct net_device *dev_out,  struct eth_pbuf *pkt,
	struct mv_eth_tx_spec *tx_spec_out);

/******************************************************************************
* cph_flow_db_get_rule()
* _____________________________________________________________________________
*
* DESCRIPTION: Get CPH flow mapping rule.
*
* INPUTS:
*       flow       - Flow parsing field values
*       for_packet - Whether get rule for packet or for new CPH rule
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
MV_STATUS cph_flow_db_get_rule(struct CPH_FLOW_ENTRY_T *cph_flow, bool for_packet);

/******************************************************************************
* cph_flow_lookup_op_type()
* _____________________________________________________________________________
*
* DESCRIPTION:lookup operation type string according to value
*
* INPUTS:
*       enum_value - The enum value to be matched
*
* OUTPUTS:
*       None
*
* RETURNS:
*       Enum string
*******************************************************************************/
char *cph_flow_lookup_op_type(int enum_value);

/******************************************************************************
* cph_flow_display_all()
* _____________________________________________________________________________
*
* DESCRIPTION: The function displays valid flow mapping tables and DSCP
*              to P-bits mapping tablefor untagged frames.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
int cph_flow_display_all(void);

/******************************************************************************
* cph_flow_init()
* _____________________________________________________________________________
*
* DESCRIPTION: Initializes CPH flow mapping data structure.
*
* INPUTS:
*       None.
*
* OUTPUTS:
*       None.
*
* RETURNS:
*       On success, the function returns MV_OK.
*       On error returns error code accordingly.
*******************************************************************************/
int cph_flow_init(void);

#ifdef __cplusplus
}
#endif

#endif /* _MV_CPH_FLOW_MAP_H_ */
