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

mkdir -p $mod_path/silabs_proslic_api
mkdir -p $bin_path

if [ ! -n "$5" ]; then
	version_str="silabs_proslic_api"
else
	version_str="silabs_proslic_api-${5}"
fi


# Create directory for build
rm -rf /tmp/slic
mkdir -p /tmp/slic
cd /tmp/slic

# Make Silabs SLIC
echo "================ Checking out SLIC code =============="
git clone git://vgitil04.il.marvell.com/slic/silabs_proslic_api > /dev/null
if [ $? != 0 ]; then
	echo "Failed to clone Silabs SLIC"
	leave 1
fi

# Compile release version od SLIC
cd silabs_proslic_api
git reset --hard origin/14t3
echo "================ Compiling SLIC objects =========="
make release KDIR=$kernel_path BUILD_SLIC_LIBRARIES=1 > /dev/null
if [ $? != 0 ]; then
	echo "Failed to build Silabs release"
	leave 1
fi

# Package release sources
if [ ! -z $create_source ]; then
	echo "================ Creating source release  =========="
	mkdir -p $release_path/silabs_proslic_api
	# Internal version: sources and no objects
	git archive  --format=tar.gz --prefix=${version_str}/ -o $release_path/silabs_proslic_api/${version_str}_internal.tar.gz origin/14t3

	# External version: objects and no sources
	rm -rf ./.git
	cd /tmp/slic
	tar --transform "s/^silabs_proslic_api/${version_str}/" -czf $release_path/silabs_proslic_api/${version_str}.tar.gz silabs_proslic_api
	cd -
fi

# Compile modules and copy to output
echo "================ Compiling SLIC modules =========="
make KDIR=$kernel_path > /dev/null
if [ $? != 0 ]; then
	echo "Failed to build Silabs release"
	leave 1
fi

find ./ -name "*.ko" | xargs cp -t $mod_path/silabs_proslic_api

# Silabs voice tool
echo "=============== Compiling voice tool =================="
cd $kernel_path
cd tools/voice/silabs
make SILABS_SLIC=/tmp/slic/silabs_proslic_api clean > /dev/null
make SILABS_SLIC=/tmp/slic/silabs_proslic_api > /dev/null
if [ $? != 0 ]; then
	echo "Failed to build Silabs voice tool"
	leave 1
fi
cp mv_voice_tool-* $bin_path/.

cd ${org_path}

leave 0
