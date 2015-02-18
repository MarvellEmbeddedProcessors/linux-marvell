#!/bin/sh -x

#
## create flash images:
##	jffs2
##	ubifs for flash with erase block size of 256KB and 512KB
#

#
## parameters:
#
## 1. kernel image
## 2. root_fs
## 3. endianess: big_endian | little_endian
## 3. output directory
##
## example: ./cr_jffs2_img_arm_mv7sft.sh /tftpboot/uImage /tftpboot/rootfs_arm-mv7sft [big_endian | little_endian ] /tftpboot/<user>

usage_exit() {
	echo
	echo "Usage: $1 cr_jffs2_img_arm_mv7sft.sh <kernel_image> <rootfs_dir> <endianess> <output_dir>"
	echo "Example: ./cr_jffs2_img_arm_mv7sft.sh /tftpboot/uImage /tftpboot/rootfs_arm-mv7sft-be big_endian  /tftpboot<user>"
	exit
}

KERNEL=$1
ROOT_FS=$2
ENDIANESS=$3
OUTPUT_DIR=$4

#
### create jffs2 image
#

if [ ! -f "${KERNEL}" ]; then
	echo
	echo kernel missing or incorrect
	usage_exit
fi

if [ ! -d "${ROOT_FS}" ]; then
	echo
	echo rootfs_dir missing or incorrect
	usage_exit
fi
if [ "${ENDIANESS}" = "big_endian" ]; then
	endianess="-b"
elif [ "${ENDIANESS}" = "little_endian" ]; then
	endianess="-l"
else
	echo endianess missing or incorrect
	usage_exit
fi

if [ ! -d "${OUTPUT_DIR}" ]; then
	echo
	echo output_dir missing or incorrect
	usage_exit
fi

echo ">>>>> creating jffs2 image <<<<<"

IMAGE=${OUTPUT_DIR}/jffs2_arm.image
TEMP_IMAGE=${OUTPUT_DIR}/temp_image

rm -f ${IMAGE} ${OUTPUT_DIR}/temp_image ${OUTPUT_DIR}/temp_kernel

mkfs.jffs2 --eraseblock=16KiB ${endianess} -p -n -d ${ROOT_FS} -o ${TEMP_IMAGE}

bzip2 -c ${KERNEL} > ${IMAGE}
bzip2 -c ${TEMP_IMAGE} >> ${IMAGE}

rm ${TEMP_IMAGE}
echo
echo "file ${IMAGE} is ready"
echo "Use the mtdburn command to burn it to flash "
echo


echo ">>>>> creating 2 ubifs-nand images <<<<<"

# remove prev build outputs
rm -f ${OUTPUT_DIR}/temp_ubi*

# use same ubi config for all ubi images
UBI_CFG=${OUTPUT_DIR}/temp_ubinize.cfg
cat << EOF >  ${UBI_CFG}
[ubifs]
mode=ubi
image=temp_ubi_rootfs.img
vol_id=0
vol_type=dynamic
vol_name=rootfs_nand
vol_alignment=1
vol_flags=autoresize
EOF

IMAGE=${OUTPUT_DIR}/ubifs_arm_256eb_nand_v2_5.image
rm -f ${IMAGE}

# create ubi image
mkfs.ubifs -r ${ROOT_FS} -m 4096 -e 253952 -c 4096 -R 24 -o temp_ubi_rootfs.img -v
ubinize -o temp_ubifs_rootfs.img -m 4096  -p 256KiB -s 4096 ${UBI_CFG} -v

# concat kernel uImage and ubi image
bzip2 -c ${KERNEL} > ${IMAGE}
bzip2 -c temp_ubifs_rootfs.img >> ${IMAGE}

echo
echo "file ${IMAGE} is ready"

IMAGE=${OUTPUT_DIR}/ubifs_arm_512eb_nand.image

# remove prev build outputs
rm -f ${IMAGE} ${OUTPUT_DIR}/temp_ubi_rootfs.img

# create ubi image
mkfs.ubifs -r ${ROOT_FS} -m 4096 -e 516096 -c 4096 -R 24 -o temp_ubi_rootfs.img -v
ubinize -o temp_ubifs_rootfs.img -m 4096  -p 512KiB -s 4096 ${UBI_CFG} -v

# concat kernel uImage and ubi image
bzip2 -c ${KERNEL} > ${IMAGE}
bzip2 -c temp_ubifs_rootfs.img >> ${IMAGE}

# cleanup temporary files
rm -f ${OUTPUT_DIR}/temp_ubi*

echo
echo "file ${IMAGE} is ready"
echo "Use the mtdburn command to burn it to flash "
echo


