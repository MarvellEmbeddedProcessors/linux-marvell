#!/bin/bash
leave() {
	cd $org_path
	rm -rf /tmp/slic
	exit $1
}

mod_path="${1}/kernel/slic"
kernel_path=$2
org_path=`pwd`
bin_path=$3
release_path="${4}/slic"
create_source=$4

if [ ! -n "$5" ]; then
	version_str="zarlink_vpapi"
else
	version_str="zarlink_vpapi-${5}"
fi

mkdir -p $mod_path/zarlink_vpapi
mkdir -p $bin_path


# Create directory for build
rm -rf /tmp/slic
mkdir -p /tmp/slic
cd /tmp/slic

echo "================ Checking out SLIC code =============="
git clone git://vgitil04.il.marvell.com/slic/zarlink_vpapi > /dev/null
if [ $? != 0 ]; then
	echo "Failed to clone Zarlink SLIC"
	leave 1
fi

# Make Zarlink SLIC
echo "================ Compiling SLIC objects =========="
cd zarlink_vpapi
git reset --hard origin/14t3
make KDIR=$kernel_path release BUILD_SLIC_LIBRARIES=1 > /dev/null
if [ $? != 0 ]; then
	echo "Failed to build Zarlink objects"
	leave 1
fi

# Package release sources
if [ ! -z $create_source ]; then
	echo "================ Creating source release  =========="
	mkdir -p $release_path/zarlink_vpapi
	# Internal version: sources and no objects
	git archive  --format=tar.gz --prefix=${version_str}/ -o $release_path/zarlink_vpapi/${version_str}_internal.tar.gz origin/14t3

	# External version: objects and no sources
	rm -rf ./.git
	cd /tmp/slic
	tar --transform "s/^zarlink_vpapi/${version_str}/" -czf $release_path/zarlink_vpapi/${version_str}.tar.gz zarlink_vpapi
	cd -
fi


# Compile modules and copy to output
echo "================ Compiling SLIC modules =========="
make KDIR=$kernel_path > /dev/null
if [ $? != 0 ]; then
	echo "Failed to build Zarlink modules"
	leave 1
fi

find ./ -name "*.ko" | xargs cp -t $mod_path/zarlink_vpapi
cd -


# Zarlink voice tool
echo "=============== Compiling voice tool =================="
cd $kernel_path
cd tools/voice/zarlink/kernel/
make clean
make ZARLINK_VPAPI=/tmp/slic/zarlink_vpapi > /dev/null
if [ $? != 0 ]; then
	echo "Failed to build Zarlink voice tool"
	leave 1
fi
cp mv_voice_tool-* $bin_path/.
cd ${org_path}

leave 0
