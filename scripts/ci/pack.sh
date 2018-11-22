#!/bin/bash
# SPDX-License-Identifier:           GPL-2.0
# https://spdx.org/licenses
# Copyright (C) 2018 Marvell International Ltd.
#
###############################################################################
## This is the pack script for linux kernel                                  ##
## This script is called by CI automated builds                              ##
###############################################################################
## WARNING: Do NOT MODIFY the CI wrapper code segments.                      ##
## You can only modify the config and compile commands                       ##
###############################################################################
## Prerequisites:       DESTDIR is the path to the destination directory
## Usage:               pack BUILD_NAME

## =v=v=v=v=v=v=v=v=v=v=v CI WRAPPER - Do not Modify! v=v=v=v=v=v=v=v=v=v=v= ##
set -exuo pipefail
shopt -s extglob

build_name=$1
echo "running pack.sh ${build_name}"
## =^=^=^=^=^=^=^=^=^=^=^=^  End of CI WRAPPER code -=^=^=^=^=^=^=^=^=^=^=^= ##

mkdir -p $DESTDIR
mkdir -p $DESTDIR/dtb/
mkdir -p $DESTDIR/rootfs

case $build_name in
	linux414_armv7* )
		cp arch/arm/boot/zImage $DESTDIR/
		cp arch/arm/boot/dts/armada-38*.dtb $DESTDIR/dtb/ || true
		export ARCH=arm
		;;
	linux414_armv8* )
		cp arch/arm64/boot/Image $DESTDIR/
		cp arch/arm64/boot/dts/marvell/armada-37*.dtb $DESTDIR/dtb/ || true
		cp arch/arm64/boot/dts/marvell/armada-39*.dtb $DESTDIR/dtb/ || true
		cp arch/arm64/boot/dts/marvell/armada-70*.dtb $DESTDIR/dtb/ || true
		cp arch/arm64/boot/dts/marvell/armada-80*.dtb $DESTDIR/dtb/ || true
		export ARCH=arm64
		;;
	* )	echo "Error: Unsupported build ${build_name}"; exit -1; ;;
esac


cp .config $DESTDIR/
cp System.map $DESTDIR/
make modules_install INSTALL_MOD_PATH=$DESTDIR/rootfs/
echo "kernel pack completed"
