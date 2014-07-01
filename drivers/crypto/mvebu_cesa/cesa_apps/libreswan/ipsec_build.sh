#!/bin/bash
# IPSEC Module build script
set -e

PKG=libreswan-3.8.tar.gz

# Optionaly download the sources from web
if [[ ! -f ${PKG} ]]; then
	wget https://download.libreswan.org/${PKG}
fi

# Extract sources localy
tar -zxvf ${PKG}

PDIR=${PKG%.*}
if [ ! -d "${PDIR}" ]; then
	PDIR=${PDIR%.*}
fi
cd ${PDIR}

# Apply marvell patch over libreswan sources
patch -p1 < ../mv_libreswan_3_8.patch

# Prepare path to source and kernel revision
KSRC=../../../../../../
KREV=`cat ${KSRC}/include/config/kernel.release`

# Build ipsec module
make clean
make KERNELSRC=${KSRC} module ARCH=arm

# Copy module to the main libreswan directory and rename
cp modobj/ipsec.ko ../ipsec_${KREV}.ko
cd ..

# Copy to modules output directory
if [ "${1}" != "" ]; then
	mkdir -p ${1}/kernel/ipsec
	cp ${PDIR}/modobj/ipsec.ko ${1}/kernel/ipsec/
fi
