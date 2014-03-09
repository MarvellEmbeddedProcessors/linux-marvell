# This is an example how to setup the network interfaces in order to run IPSec routing

ifconfig eth0 192.168.0.1 hw ether 00:00:00:00:61:92 netmask 255.255.0.0
ifconfig eth1 192.167.0.1 hw ether 00:00:00:00:62:81 netmask 255.255.0.0
echo 1 > /proc/sys/net/ipv4/ip_forward
arp -s 192.167.0.250 00:00:00:00:00:12
arp -s 192.168.1.1 00:00:00:00:00:11
#IPSec Configuration
insmod /ipsec.ko

#SmartBit Configuration
#eth0 - 192.168.1.1 --> 192.167.0.250
#         00:00:00:00:00:12 --> 00:00:00:00:61:92
#eth1   - 192.167.0.250 --> 192.168.1.1
#         00:00:00:00:00:11 -> 00:00:00:00:62:81


# unmark this to disble flow control for Yukon/E1000 NICs
#ethtool -A eth0 tx off
#ethtool -A eth0 rx off
#ethtool -A eth2 tx off
#ethtool -A eth2 rx off

# Here we build static SA database since it is not supported anymore from OpenSWAN 2.6.18 and on
# This is example setup for ESP 3DES/SHA1
ipsec spi --clear
ipsec eroute --clear
enckey=0x0123456789abcdef02468ace13579bdf123456789abcdef0
authkey=0x0123456789abcdef02468ace13579bdf12345678
ipsec spi --af inet --edst 192.168.1.1 --spi 0x12345678 --proto esp --src 192.168.0.1 --esp 3des-sha1 --enckey $enckey --authkey $authkey
ipsec spi --af inet --edst 192.168.1.1 --spi 0x12345678 --proto tun --src 192.168.0.1 --dst 192.168.1.1 --ip4
ipsec spigrp inet 192.168.1.1 0x12345678 tun inet 192.168.1.1 0x12345678 esp
ipsec eroute --add --eraf inet --src 192.167.0.0/16 --dst 192.168.1.0/24 --said tun0x12345678@192.168.1.1
ipsec tncfg --attach --virtual ipsec0 --physical eth0
ifconfig ipsec0 inet 192.168.0.1 netmask 255.255.0.0 broadcast 192.168.255.255 up
route add -host 192.168.1.1 gw 192.168.0.1 dev ipsec0
