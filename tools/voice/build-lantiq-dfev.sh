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

mkdir -p $bin_path
mkdir -p $mod_path/lantiq/dfev/

# Create directory for build
rm -rf /tmp/slic
mkdir -p /tmp/slic
cd /tmp/slic

echo "================ Checking out SLIC code =============="
git clone git://vgitil04.il.marvell.com/slic/lantiq > /dev/null
if [ $? != 0 ]; then
	echo "Failed to clone Lantiq SLIC"
	leave 1
fi

cd lantiq/dfev
git reset --hard origin/14t3

echo "================ Compiling kernel modules =========="

make KDIR=$kernel_path BUILD_SLIC_LIBRARIES=1 > /dev/null
if [ $? != 0 ]; then
	echo "Failed to build Lantiq objects"
	leave 1
fi

# Copy objects
find ./ -name "*.ko" | xargs cp -t $mod_path/lantiq/dfev


# Lantiq voice tool
echo "=============== Compiling voice tool =================="
cd $kernel_path
cd tools/voice/lantiq-dfev
make EXTRA_CFLAGS="-I/tmp/slic/lantiq/dfev/lib_ifxos/src/include	\
		-I/tmp/slic/lantiq/dfev/drv_tapi/include		\
		-I/tmp/slic/lantiq/dfev/drv_sdd/include			\
                -I/$kernel_path/arch/arm/plat-armada/mv_drivers_lsp/mv_phone" clean > /dev/null
make EXTRA_CFLAGS="-I/tmp/slic/lantiq/dfev/lib_ifxos/src/include	\
		-I/tmp/slic/lantiq/dfev/drv_tapi/include		\
		-I/tmp/slic/lantiq/dfev/drv_sdd/include			\
		-I/$kernel_path/arch/arm/plat-armada/mv_drivers_lsp/mv_phone" > /dev/null

if [ $? != 0 ]; then
	echo "Failed to build Lantiq DFEV voice tool"
	leave 1
fi
cp mv_voice_tool $bin_path/.

if [ ! -n "$4" ]; then
	leave 0
fi

# Create source release
echo "================ Creating source release  =========="
if [ ! -n "$5" ]; then
	version_str="lantiq"
else
	version_str="lantiq-${5}"
fi

mkdir -p $release_path/lantiq
cd /tmp/slic/lantiq

# Make internal version
git archive --format=tar.gz --prefix=${version_str}/ -o $release_path/lantiq/${version_str}_internal.tar.gz origin/14t3

cd ${org_path}

leave 0
