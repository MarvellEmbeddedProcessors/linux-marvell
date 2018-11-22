#!/bin/bash
# Copyright (C) 2018 Marvell International Ltd.
#
# SPDX-License-Identifier:           GPL-2.0
# https://spdx.org/licenses
###############################################################################
## This is the compile script for linux kernel                               ##
## This script is called by CI automated builds                              ##
## It may also be used interactively by users to compile the same way as CI  ##
###############################################################################
## WARNING: Do NOT MODIFY the CI wrapper code segments.                      ##
## You can only modify the config and compile commands                       ##
###############################################################################


## =v=v=v=v=v=v=v=v=v=v=v CI WRAPPER - Do not Modify! v=v=v=v=v=v=v=v=v=v=v= ##
set -euo pipefail
shopt -s extglob
##==================================== USAGE ================================##
function usage {
	echo """
Usage: compile [--no_configure] [--echo_only] BUILD_NAME
 or:   compile --list
 or:   compile --help

Compiles kernel similar to the given CI build

 -N, --no_configure   Skip configuration steps (make distclean)
 -e, --echo_only      Print out the compilation sequence but do not execute it
 -l, --list           List all supported BUILD_NAME values and exit
 -h, --help           Display this help and exit

Prerequisites:
  CROSS_COMPILE       path to cross compiler

"""
	exit 0
}
##============================ PARSE ARGUMENTS ==============================##
TEMP=`getopt -a -o Nelh --long no_configure,echo_only,list,help \
             -n 'compile' -- "$@"`

if [ $? != 0 ] ; then
	echo "Error: Failed parsing command options" >&2
	exit 1
fi
eval set -- "$TEMP"

no_configure=
echo_only=
list=

while true; do
	case "$1" in
		-N | --no_configure ) no_configure=true; shift ;;
		-e | --echo_only )    echo_only=true; shift ;;
		-l | --list ) 	      list=true; shift ;;
		-h | --help )         usage; ;;
		-- ) shift; break ;;
		* ) break ;;
	esac
done

if [[ $list ]] ; then
	DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"
	echo "Supported build names:"
	grep -v '^#' "$DIR/supported_builds.txt"
	echo
	echo
	exit 0
fi

[[ $# -ne 1 ]] && usage
build_name=$1

grep ^$build_name$ ./scripts/ci/supported_builds.txt >&/dev/null ||
	( echo "Error: Unsupported build ${build_name}"; exit -1 )
echo "running compile.sh ${build_name}"
## =^=^=^=^=^=^=^=^=^=^=^=^  End of CI WRAPPER code -=^=^=^=^=^=^=^=^=^=^=^= ##


########################### KERNEL CONFIGURATION ##############################
case $build_name in
	linux414_armv7* )          arch="arm"; ;;
	linux414_armv8* )          arch="arm64"; ;;
	* ) echo "Error: Could not configure arch."
		"Unsupported build ${build_name}"; exit -1; ;;
esac

case $build_name in
	linux414_armv7* )          defconfig="mvebu_v7_defconfig"; ;;
	linux414_armv8*_mainline ) defconfig="defconfig"; ;;
	linux414_armv8* )          defconfig="mvebu_v8_lsp_defconfig"; ;;
	* ) echo "Error: Could not configure defconfig."
		"Unsupported build ${build_name}"; exit -1; ;;
esac

kernel_config=""
make_config="next if /^\s*$/; print './scripts/config --file arch/$arch/configs/$defconfig --set-val ';s{=}{ };"
case $build_name in
	linux414_armv7be|linux414_armv8be|linux414_armv8be_mainline )
		kernel_config="./scripts/config --file arch/$arch/configs/$defconfig --set-val CONFIG_CPU_BIG_ENDIAN y"; ;;

	linux414_armv8le_net_bm )
		kernel_config=$(perl -pe "$make_config" tools/configs/net-bm); ;;

	linux414_armv8le_net_bm_nat )
		kernel_config=$(perl -pe "$make_config" tools/configs/net-bm-nat); ;;

	linux414_armv8le_strongswan_ipsec )
		kernel_config=$(perl -pe "$make_config" tools/configs/strongswan-ipsec); ;;
esac
###############################################################################


## =v=v=v=v=v=v=v=v=v=v=v CI WRAPPER - Do not Modify! v=v=v=v=v=v=v=v=v=v=v= ##
cmd="""
set -x
pwd"""
## =^=^=^=^=^=^=^=^=^=^=^=^  End of CI WRAPPER code -=^=^=^=^=^=^=^=^=^=^=^= ##


##################################### CONFIG ##################################
[[ $no_configure ]] || cmd=$cmd"""
export ARCH=$arch
make mrproper
$kernel_config
make $defconfig"""

#################################### COMPILE ##################################
##[ armv7
if [[ $build_name =~ armv7 ]]; then
cmd=$cmd"""
make zImage dtbs -j4
make modules -j4"""
##]
###############################################################################
##[ armv8
else
cmd=$cmd"""
make Image dtbs -j4
make modules -j4"""
##]
###############################################################################
fi


## =v=v=v=v=v=v=v=v=v=v=v CI WRAPPER - Do not Modify! v=v=v=v=v=v=v=v=v=v=v= ##
if [[ $echo_only ]]; then
	echo "$cmd"
	exit 0
fi

logfile="$$.make.log"
(eval "$cmd") > ${logfile}
cat $logfile
if grep -i "warning:" $logfile; then
        echo "Error: Build has warnings. Aborted"
        exit -1
fi
## =^=^=^=^=^=^=^=^=^=^=^=^  End of CI WRAPPER code -=^=^=^=^=^=^=^=^=^=^=^= ##
