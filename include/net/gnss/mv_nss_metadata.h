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
/*  mv_nss_metadata.h */

#ifndef __MV_NSS_METADATA_H__
#define __MV_NSS_METADATA_H__

/*
* struct mv_nss_metadata
*
* Description:
*       Basic packet annotation.
*
* Fields:
*       port_src    - Source virtual port.
*       port_dst    - Destination virtual port.
*       type        - Packet information.
*                       bit [0] - Validity condition.
*                         0 - bits [4-31] are valid
*                         1 - bits [4-31] are invalid
*                       bits [1]: Frame type:
*                         0 - Standard Ethernet II frame
*                         1 - Proprietary A-MSDU frame
*                       bits [8-14] - L3 offset: IPv4 or IPv6 offset in the packet.
*                          This field indicates the beginning of Layer3 (in octets).
*                       bit [15] - MAC to me: MAC DA lookup results
*                         0 - MAC DA is unknown (not MAC to me)
*                         1 - MAC DA is known unicast or multicast MAC address
*                             (MAC to me)
*                       bits [16-20] - IP Header length: IP Header Length in
*                            4-octet words.
*                            IPv4: IPv4 header length including options
*                            if exist
*                            IPv6: IPv6 header length including extension
*                            header if exists
*                       bits [21-23] - L4 info: L4 parsing results
*                         0 - Unknown
*                         1 - TCP
*                         2 - TCP + checksum error
*                         3 - UDP
*                         4 - UDP lite
*                         5 - UDP + checksum error
*                         6 - IGMP
*                         7 - Other
*                       bits [24-25]: VLAN info - Number of VLANs in the packet
*                         0 - Untagged
*                         1 - Single Tag
*                         2 - Double tag
*                         3 - Reserved
*                       bit [26] - Management or non-management packet type
*                         0 - Non-management packet
*                         1 - Management packet
*                       bits [27-28] - L2 info: indicates type of L2 packet
*                         0 - Unicast
*                         1 - Multicast
*                         2 - IP Multicast
*                         3 - Broadcast
*                       bits [29-31] - L3 info: L3 parsing results
*                         0 - Unknown
*                         1 - IPv4
*                         2 - IPv4 fragment
*                         3 - IPv4 with options
*                         4 = IPv4 with errors (checksum, TTL, etc)
*                         5 = IPv6
*                         6 = IPv6 with extension(s) header
*                         7 = ARP
*       cos         - Class of service.
*       reason      - Packet origination context.
*       opaque      - Application opaque handle.
*       reserved    - Reserved fields.
*/
struct mv_nss_metadata {
	uint16_t port_src;
	uint16_t port_dst;
	uint32_t type;
	uint32_t opaque;
	uint8_t  cos;
	uint8_t  reason;
	uint8_t reserved[18];
};


/*
* typedef: struct mv_nss_metadata_wlan_t
*
* Description:
*       Packet annotation for packets toward WLAN.
*
* Fields:
*       port_src    - Source virtual port.
*       port_dst    - Destination virtual port.
*       type        - Packet information.
*                       bits [0-2]: Frame type:
*                         0 - Standard Ethernet II frame
*                         1 - Proprietary A-MSDU short frame
*                         2 - Proprietary A-MSDU basic frame
*                       bit [3] - Validity condition:
*                         0 - bits [4-31] are invalid
*                         1 - bits [4-31] are valid
*                       bits [8-14] - L3 offset: IPv4 or IPv6 offset in the packet.
*                          This field indicates the beginning of Layer3 (in octets).
*                       bit [15] - MAC to me: MAC DA lookup results
*                         0 - MAC DA is unknown (not MAC to me)
*                         1 - MAC DA is known unicast or multicast MAC address
*                             (MAC to me)
*                       bits [16-20] - IP Header length: IP Header Length in
*                            4-octet words.
*                            IPv4: IPv4 header length including options
*                            if exist
*                            IPv6: IPv6 header length including extension
*                            header if exists
*                       bits [21-23] - L4 info: L4 parsing results
*                         0 - Unknown
*                         1 - TCP
*                         2 - TCP + checksum error
*                         3 - UDP
*                         4 - UDP lite
*                         5 - UDP + checksum error
*                         6 - IGMP
*                         7 - Other
*                       bits [24-25]: VLAN info - Number of VLANs in the packet
*                         0 - Untagged
*                         1 - Single Tag
*                         2 - Double tag
*                         3 - Reserved
*                       bit [26] - Management or non-management packet type
*                         0 - Non-management packet
*                         1 - Management packet
*                       bits [27-28] - L2 info: indicates type of L2 packet
*                         0 - Unicast
*                         1 - Multicast
*                         2 - IP Multicast
*                         3 - Broadcast
*                       bits [29-31] - L3 info: L3 parsing results
*                         0 - Unknown
*                         1 - IPv4
*                         2 - IPv4 fragment
*                         3 - IPv4 with options
*                         4 = IPv4 with errors (checksum, TTL, etc)
*                         5 = IPv6
*                         6 = IPv6 with extension(s) header
*                         7 = ARP
*       opaque      - A-MSDU opaque application handle from A-MSDU context.
*       cos         - Class of service.
*       reason      - Packet origination context.
*       reserved1   - Reserved fields.
*       lifetime    - Total A-MSDU aggregation time, nanoseconds.
*       reserved2   - Reserved fields.
*/
struct mv_nss_metadata_wlan {
	uint16_t port_src;
	uint16_t port_dst;
	uint32_t type;
	uint32_t opaque;
	uint8_t  cos;
	uint8_t  reason;
	uint16_t reserved1;
	uint32_t lifetime;
	uint8_t  reserved2[12];
};




#endif /* __MV_NSS_METADATA_H__ */
