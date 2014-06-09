#!/bin/bash
# IPSEC Module build script
set -e

# Optionaly download the sources from web
#wget http://download.openswan.org/openswan/openswan-2.6.41.tar.gz

# Extract sources localy
tar -zxvf openswan-2.6.41.tar.gz
cd openswan-2.6.41

# Apply marvell patch over openswan sources
patch -p1 < ../mv_openswan_2_6_41.patch
# Add support for LKv3.10 (imported from libreswan 3.8)
patch -p1 < ../lk_3_10_support_openswan_2_6_41.patch

# Build ipsec module
make KERNELSRC=../../../../../../ module ARCH=arm

# Copy to modules output directory
if [ "$1" != "" ]; then
	mkdir -p ${1}/kernel/ipsec
	cp modobj26/ipsec.ko ${1}/kernel/ipsec
fi
