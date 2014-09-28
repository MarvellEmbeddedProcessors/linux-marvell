#!/bin/bash

echo " * Version: 5.2"

# LOG:
# 5.2:
#   1. fix -j option when not using the -p option.
# 5.1:
#   1. add mkfs.btrfs the -f flag
# 5.0:
#   1. remove btrfs disabling use of strict allocate in smb.conf
# 4.9:
#   1. set fdisk alignment to better support SSDs and new 4K sectors HDDS.
#   2. remove default HDD_NUM set to 4. (each RAID has it's own default)
# 4.8:
#   1. setting tcp_limit_output_bytes on a38x.
# 4.7:
#   1. adding option none to -s parameters.
# 4.6:
#   1. a388 interrupt affinity.
#   2. a375 optimizations (network apadtive coalescing).
#   3. a375 interrupt affinity.
#   4. RAID stripe cache size changed to 4K.
# 4.4:
#   1. initial support for a380, a385, a388 configurations.
#   2. added support for 3 HDDs RD0 and RD5 setups.
# 4.3:
#   1. fixed wrong partnum usage.
#   2. min receivefile size set to 16k.
# 4.2:
#   1. removed strip_cache_size on RD1 configuration (no such entity in RD1).
#   2. fixing mdadm -S command.
# 4.1:
#   1. added support to encrypted single disk configuration (e.g. -t crypt_sd)
#   2. added new option -j that create offset partition in disk (e.g. -j 150)
# 4.0:
#   1. complete redesign of the nas_init script.
#   2. added new option -s that transfer SoC used, please see help.
#   3. new functions to consolidate mkfs and mount utilities.
#   4. removed old -b option, no support for rootfs copy.
#   5. removed old -s option, SSD support was broken anyway.
#   6. adding support for a370 adaptive coaleaing.
# 3.6:
#   1. fixing option u: will not start samba processes.
#   2. remove the NONE option for FS.
#   3. removing reboot command.
# 3.5:
#   1. moving raid-5 to 128 chunk size
# 3.4:
#   1. adding support for ubuntu systems by using -u param.
#   2. when using ubuntu, users will have to stop and start samba manually,
#      will be done automaticlly in later versions.
#   3. fixing bonding setup flow as needed by newer kernels.(both 2 and 4 links)
#   4. switching bond configuration to balance-xor (from balance-alb).
#   5. checking if smbd and nmbd exist on the filesystem.
# 3.3:
#   1. adding support for 5 HDDs RAID5.
#   2. reverting gso, tso to default state.
#   3. RD5 changes:
#      3.1. disk scheduler revert to deafult.
#      3.2. dirty_ratio, dirty_background_ratio reverted to default.
#      3.3. md read_ahead changed to 2048.
#      3.4. disks read_ahead changed to 192.
#   4. minor cosmetics changes.
# 3.2:
#   1. adding support for -l 0 to avoid network setup.
#   2. updating smb.conf to improve single link performance.
# 3.1:
#   1. mount point updated.
#   2. XOR affinity update.
#   3. RAID-6 support.
# 3.0:
#   1. first all-in-one script to include: affinity, bonding, nas_init.
#   2. PAGE_SIZE automatic set.
#   3. LINK_NUM support to identify number of links.
#   4. prints cleanup and better flow status report.
#   5. Error prints added.
#   6. Exit on error.
# 2.5:
#   1. XFS mount options updated.
#   2. RAID1 chunk size updated.
#   3. splice enabled for XFS.
# 2.4:
#   1. support RAID-1.
#   2. splice enabled by default.
# 2.3:
#   1. fixing the mkdir share for loop.
# 2.2:
#   1. automatic size settings for PARTSIZE.
# 2.1:
#   1. setting coal to 100 in net queue 0 and fixing for in NETQ.

PREPARE_HDD="no"
MKFS="no"
TOPOLOGY="sd"
HDD_NUM=""
PLATFORM=""
FS="ext4"
SYSDISKEXIST="no"
CPU_COUNT=`grep -c ^processor /proc/cpuinfo`
LARGE_PAGE=`/usr/bin/getconf PAGESIZE`
PARTNUM="1"
LINK_NUM=`dmesg |grep "Giga ports" |awk '{print $2}'`
PARTSIZE="55GB"
JUMPPARTSIZE="0"
NETQ="0"
SAMBASTATUS="enabled"
MNT_DIR="/mnt/public"
# Encryption variables
CRYPTO_NAME="cryptovol1"
KEY_SIZE="192"
ALGORITHIM="aes"


STAMP="[$(date +%H:%M-%d%b%Y)]:"
echo -e "Script parameters has been updated! please read help!"

function do_error_hold {
    echo -e "\n*************************** Error ******************************\n"
    echo -e "${STAMP} Error: ${1}"
    echo -ne "Press Enter continue..."
    echo -e "\n****************************************************************\n"
    read TEMP
    exit 2
}

function do_error {
    echo -e "\n*************************** Error ******************************\n"
    echo -e " ${STAMP}: ${1}"
    echo -e " Please see help\n"
    echo -e "\n****************************************************************\n"
    exit 1
}

if [ "$CPU_COUNT" == "0" ]; then
    CPU_COUNT="1"
fi

while getopts "l:ps:mzun:f:t:h:j:" flag; do
    case "$flag" in
	f)
	    FS=$OPTARG
	    case "$OPTARG" in
		ext4|btrfs|xfs)	echo "Filesystem: ${OPTARG}" ;;
		*)   do_error "-f: wrong option" ;;
	    esac
	    ;;
	m)	MKFS="yes"
	    ;;
	z)	SYSDISKEXIST="yes"
	    ;;
	n)	PARTNUM="$OPTARG"
	    ;;
	p)	PREPARE_HDD="yes"
	    ;;
	l)	LINK_NUM=$OPTARG
	    case "$OPTARG" in
		0|1|2|4) ;;
		*)	do_error "-l: wrong option" ;;
	    esac
	    ;;
	t)	TOPOLOGY=$OPTARG
	    case "$OPTARG" in
		sd|rd0|rd1|rd5|rd6|crypt_sd) ;;
		*)	do_error "-t: wrong option" ;;
	    esac
	    ;;
	s)	PLATFORM=$OPTARG
	    case "$OPTARG" in
		a310|a310|axp|a370|a375|a380|a385|a388|none) ;;
		*)	do_error "-s: wrong option" ;;
	    esac
	    ;;
	h)	HDD_NUM=$OPTARG
	    case "$OPTARG" in
		2|3|4|5|8) ;;
		*)	do_error "-h: wrong option" ;;
	    esac
	    ;;
	u)      SAMBASTATUS="disabled"
	    ;;
	j)	JUMPPARTSIZE=$OPTARG
		if [ $OPTARG -lt 1 ]; then
			do_error "-j: wrong input"
		fi
		;;
	*)	echo "Usage: $0"
	    echo "           -s <a300|a310|axp|a370|a375|a380|a385|a388|none>: platform used with nas_init"
	    echo "           -f <ext4|xfs|btrfs>: file system type ext4, xfs, btrfs or fat32"
	    echo "           -t <sd|rd0|rd1|rd5|rd6>: drive topology"
	    echo "           -n <num>: partition number to be mounted"
	    echo "           -m create RAID and FD (mkfs/mdadm)"
	    echo "           -p prepare drives (fdisk)"
	    echo "           -h <num>: number of HDDs to use"
	    echo "           -l <num>: number of links to use"
	    echo "           -u adding this flag will disable SAMBA support"
	    echo "           -j <num>: offset in disk for the new partition to be created (in GB)"
	    exit 1
	    ;;
    esac
done

# verify supporting arch
case "$PLATFORM" in
    a300|a310|axp|a370|a375|a380|a385|a388|none)	;;
    *)	do_error "Platform ${PLATFORM} unsupported, please pass valid -s option" ;;
esac

echo -ne "************** System info ***************\n"
echo -ne "    Topology:      "
case "$TOPOLOGY" in
    sd)		echo -ne "Single drive\n" ;;
    rd0)	echo -ne "RAID0\n" ;;
    rd1)	echo -ne "RAID1\n" ;;
    rd5)	echo -ne "RAID5\n" ;;
    rd6)	echo -ne "RAID6\n" ;;
    crypt_sd) 	echo -ne "Encrypted single drive\n" ;;
    *)	do_error "Invalid drive topology" ;;
esac

echo -ne "    Filesystem:    $FS\n"
echo -ne "    Platform:      $PLATFORM\n"
echo -ne "    Cores:         $CPU_COUNT\n"
echo -ne "    Links:         $LINK_NUM\n"
echo -ne "    Page Size:     $LARGE_PAGE\n"
echo -ne "    Disk mount:    $SYSDISKEXIST\n"
echo -ne "    * Option above will used for the system configuration\n"
echo -ne "      if you like to change them, please pass different parameters to the nas_init.sh\n"
echo -ne "******************************************\n"

if [ "$SAMBASTATUS" == "enabled" ]; then

    [ ! -e "$(which smbd)" ] && do_error "SAMBA in not installed on your filesystem (aptitude install samba)"
    [ ! -e "$(which nmbd)" ] && do_error "SAMBA in not installed on your filesystem (aptitude install samba)"

    echo -ne " * Stopping SAMBA processes:   "
    if [ "$(pidof smbd)" ]; then
	killall smbd
    fi

    if [ "$(pidof nmbd)" ]; then
	killall nmbd
    fi
    echo -ne "[Done]\n"
    sleep 2

    echo -ne " * Checking SAMBA is not running:  "
    if [ `ps -ef |grep smb |grep -v grep |wc -l` != 0 ]; then
	do_error "Unable to stop Samba processes, to stop them manually use -u flag"
    fi
    echo -ne "[Done]\n"
fi

mkdir -p $MNT_DIR
chmod 777 $MNT_DIR

echo -ne " * Unmounting $MNT_DIR:            "
if [ `mount | grep $MNT_DIR | grep -v grep | wc -l` != 0 ]; then
    umount $MNT_DIR
fi

sleep 2

if [ `mount | grep mnt | grep -v grep | wc -l` != 0 ]; then
    do_error "Unable to unmount $MNT_DIR"
fi
echo -ne "[Done]\n"

# examine disk topology
if [ "$TOPOLOGY" == "sd" -o "$TOPOLOGY" == "crypt_sd" ]; then
    if [ "$SYSDISKEXIST" == "yes" ]; then
	DRIVES="b"
    else
	DRIVES="a"
    fi
    PARTSIZE="55GB"
elif [ "$TOPOLOGY" == "rd0" ]; then
    if [ "$HDD_NUM" == "5" ]; then
	if [ "$SYSDISKEXIST" == "yes" ]; then
	    DRIVES="b c d e f"
	else
	    DRIVES="a b c d e"
	fi
	PARTSIZE="20GB"
    elif [ "$HDD_NUM" == "4" ]; then
	if [ "$SYSDISKEXIST" == "yes" ]; then
	    DRIVES="b c d e"
	else
	    DRIVES="a b c d"
	fi
	PARTSIZE="20GB"
    elif [ "$HDD_NUM" == "3" ]; then
	if [ "$SYSDISKEXIST" == "yes" ]; then
	    DRIVES="b c d"
	else
	    DRIVES="a b c"
	fi
	PARTSIZE="25GB"
    else # [ "$HDD_NUM" == "2" ] default
	if [ "$SYSDISKEXIST" == "yes" ]; then
	    DRIVES="b c"
	else
	    DRIVES="a b"
	fi
	HDD_NUM=2
	PARTSIZE="50GB"
    fi
    LEVEL=0
elif [ "$TOPOLOGY" == "rd1" ]; then
    if [ "$SYSDISKEXIST" == "yes" ]; then
	DRIVES="b c"
    else
	DRIVES="a b"
    fi
    PARTSIZE="55GB"
    HDD_NUM=2
    LEVEL=1
elif [ "$TOPOLOGY" == "rd5" ]; then
    if [ "$HDD_NUM" == "8" ]; then
	if [ "$SYSDISKEXIST" == "yes" ]; then
	    DRIVES="b c d e f g h i"
	else
	    DRIVES="a b c d e f g h"
	fi
	PARTSIZE="10GB"
    elif [ "$HDD_NUM" == "5" ]; then
	if [ "$SYSDISKEXIST" == "yes" ]; then
	    DRIVES="b c d e f"
	else
	    DRIVES="a b c d e"
	fi
	PARTSIZE="20GB"
    elif [ "$HDD_NUM" == "3" ]; then
	if [ "$SYSDISKEXIST" == "yes" ]; then
	    DRIVES="b c d"
	else
	    DRIVES="a b c"
	fi
	PARTSIZE="25GB"
    else # [ "$HDD_NUM" == "4" ] default HDD_NUM
	if [ "$SYSDISKEXIST" == "yes" ]; then
	    DRIVES="b c d e"
	else
	    DRIVES="a b c d"
	fi
	PARTSIZE="20GB"
	HDD_NUM=4
    fi
    LEVEL=5
elif [ "$TOPOLOGY" == "rd6" ]; then
    if [ "$HDD_NUM" == "8" ]; then
	if [ "$SYSDISKEXIST" == "yes" ]; then
	    DRIVES="b c d e f g h i"
	else
	    DRIVES="a b c d e f g h"
	fi
	PARTSIZE="10GB"
    elif [ "$HDD_NUM" == "5" ]; then
	if [ "$SYSDISKEXIST" == "yes" ]; then
	    DRIVES="b c d e f"
	else
	    DRIVES="a b c d e"
	fi
	PARTSIZE="20GB"
    else #[ "$HDD_NUM" == "4" ] default HDD_NUM
	if [ "$SYSDISKEXIST" == "yes" ]; then
	    DRIVES="b c d e"
	else
	    DRIVES="a b c d"
	fi
	PARTSIZE="20GB"
	HDD_NUM=4
    fi
    LEVEL=6
fi

# create_fs function variables:
# $1 filesystem type
# $2 RAID topology
# $3 device to use
# $4 number of HDDs (optional, for RAID only)
function create_fs {
    STRIDE=32
    STRIPE=0
    HDD_NUM=$4
    case "$1" in
	ext4)	[ ! -e "$(which mkfs.ext4)" ] \
	    && do_error "missing mkfs.ext4 in rootfs (aptitude install e2fsprogs)" ;;
	xfs)	[ ! -e "$(which mkfs.xfs)" ] \
	    && do_error "missing mkfs.xfs in rootfs (aptitude install xfsprogs)" ;;
	btrfs)	[ ! -e "$(which mkfs.btrfs)" ] \
	    && do_error "missing mkfs.btrfs in rootfs (aptitude install btrfs-tools)" ;;
	*) do_error "no valid filesystem specified" ;;
    esac

    case "$1" in
	ext4)
	    case "$2" in
		sd) STRIPE=0
		    ;;
		rd1) STRIPE=$STRIDE
		    ;;
		rd0) STRIPE=$((STRIDE * HDD_NUM))
	            ;;
		rd5) HDD_NUM=$((HDD_NUM - 1))
		    STRIPE=$((STRIDE * HDD_NUM))
		    ;;
		rd6) HDD_NUM=$(($HDD_NUM - 2))
		    STRIPE=$((STRIDE * HDD_NUM))
		    ;;
		*) do_error "unsupported topology $2\n"
		    ;;
	    esac
	    if [ $STRIPE -gt 0 ]; then
		E_OPTION="-E stride=$STRIDE,stripe-width=$STRIPE"
	    else
		E_OPTION=""
	    fi
	    # create the filesystem
	    mkfs.ext4 -j -m 0 -O large_file,extent -b 4096 $E_OPTION -F $3
	    ;;
        xfs) mkfs.xfs -f $3
	    ;;
        btrfs) mkfs.btrfs -f $3
	    ;;
	*) do_error "unsupported filesystem $1\n"
	    ;;
    esac
}

# mount_fs function variables:
# $1 filesystem type
# $2 device to use
# $3 mount dir
function mount_fs {
    if [ "$1" == "ext4" ]; then
	mount -t ext4 $2 $3 -o noatime,data=writeback,barrier=0,nobh
    elif [ "$1" == "xfs" ]; then
	mount -t xfs $2 $3 -o noatime,nodirspread
    elif [ "$1" == "btrfs" ]; then
	mount -t btrfs $2 $3 -o noatime,thread_pool=$CPU_COUNT
    else
	do_error "unsupported filesystem $1\n"
    fi

    if [ `mount | grep $3 | grep -v grep | wc -l` == 0 ]; then
	do_error "Failed to mount FS $1 from $2 to dir $3"
    fi
}

# Checking drives existence
#for drive in `echo $DRIVES`; do
 #   if [ `fdisk -l |grep "Disk /dev/sd$drive" |grep -v grep |wc -l` == 0 ]; then
#	do_error "Drives assigned (/dev/sd$drive) is not persent!"
 #   fi
#done

[ ! -e "$(which mdadm)" ] && do_error "missing mdadm in rootfs (aptitude install mdadm)"

if [ "$JUMPPARTSIZE" -ne "0" ]; then
	PARTNUM="2"
fi

if [ "$PREPARE_HDD" == "yes" ]; then
    echo -ne " * Preparing disks partitions: "
    [ ! -e "$(which fdisk)" ] && do_error "missing fdisk in rootfs"

    mdadm -S /dev/md*
    sleep 2

    for partition in `echo $DRIVES`; do mdadm --zero-superblock /dev/sd${partition}${PARTNUM}; done
    sleep 2

    set -o verbose

    FDISK_LINE="o\nn\np\n1\n\n+$PARTSIZE\nt\n83\nw\n"
    if [ "$JUMPPARTSIZE" -ne "0" ]; then
	FDISK_LINE="o\nn\np\n1\n\n+${JUMPPARTSIZE}GB\nt\n83\nn\np\n2\n\n+$PARTSIZE\nt\n2\n83\nw\n"
    fi

    for drive in `echo $DRIVES`; \
		do echo -e "${FDISK_LINE}" | fdisk -c -u /dev/sd${drive}; done
    sleep 3
    mdadm -S /dev/md*
    sleep 2
    set +o verbose

    blockdev --rereadpt /dev/sd${drive} \
    || do_error "The partition table has been altered, please reboot device and run nas_init again"
    echo -ne "[Done]\n"
fi

echo -ne " * Stopping all MD devices:    "
mdadm -S /dev/md*
sleep 2
echo -ne "[Done]\n"

if [ "$TOPOLOGY" == "sd" ]; then
    PARTITIONS="/dev/sd${DRIVES}${PARTNUM}"
    echo -ne " * Starting single disk:       "
    set -o verbose

    echo -e 1024 > /sys/block/sd${DRIVES}/queue/read_ahead_kb

    if [ "$MKFS" == "yes" ]; then
	for partition in `echo $DRIVES`; do mdadm --zero-superblock /dev/sd${partition}${PARTNUM}; done
	sleep 2
	create_fs $FS $TOPOLOGY $PARTITIONS
    fi

    mount_fs $FS $PARTITIONS $MNT_DIR

    set +o verbose
    echo -ne "[Done]\n"
elif [ "$TOPOLOGY" == "crypt_sd" ]; then
    PARTITIONS="/dev/sd${DRIVES}${PARTNUM}"
    CRYPTO_PARTITIONS="/dev/mapper/$CRYPTO_NAME"
    echo -ne " * Starting encrypted single disk:       "
    set -o verbose

    echo -e 1024 > /sys/block/sd${DRIVES}/queue/read_ahead_kb

    # create encryption key
    dd if=/dev/urandom of=key bs=$KEY_SIZE count=1
    cryptsetup -c $ALGORITHIM -d key -s $KEY_SIZE create $CRYPTO_NAME $PARTITIONS

    if [ "$MKFS" == "yes" ]; then
	create_fs $FS "sd" $CRYPTO_PARTITIONS
    fi

    mount_fs $FS $CRYPTO_PARTITIONS $MNT_DIR

    set +o verbose
    echo -ne "[Done]\n"
else # RAID TOPOLOGY
    [ ! -e "$(which mdadm)" ] && do_error "missing mdadm in rootfs (aptitude install mdadm)"

    echo -ne " * Starting $TOPOLOGY build:       "
    for drive in `echo $DRIVES`; do PARTITIONS="${PARTITIONS} /dev/sd${drive}${PARTNUM}"; done

    set -o verbose

    if [ "$MKFS" == "yes" ]; then
	mdadm -S /dev/md*
	sleep 2
	for partition in `echo $DRIVES`; do mdadm --zero-superblock /dev/sd${partition}${PARTNUM}; done
	sleep 2

	echo "y" | mdadm --create -c 128 /dev/md0 --level=$LEVEL -n $HDD_NUM --force $PARTITIONS
	sleep 2

	if [ `cat /proc/mdstat  |grep md0 |wc -l` == 0 ]; then
	    do_error "Unable to create RAID device"
	fi

	create_fs $FS $TOPOLOGY /dev/md0 $HDD_NUM

    else
	# need to reassemble the raid
	mdadm --assemble /dev/md0 --force $PARTITIONS

	if [ `cat /proc/mdstat  |grep md0 |wc -l` == 0 ]; then
	    do_error "Unable to assemble RAID device"
	fi
    fi

    mount_fs $FS /dev/md0 $MNT_DIR

    if [ "$TOPOLOGY" != "rd1" ]; then
       # no stripe_cache_size support in rd1
	if [ "$LARGE_PAGE" == "65536" ]; then
            echo 256 > /sys/block/md0/md/stripe_cache_size
	else
            echo 4096 > /sys/block/md0/md/stripe_cache_size
	fi
    fi

    for drive in `echo $DRIVES`; do echo -e 192 > /sys/block/sd${drive}/queue/read_ahead_kb; done

    echo -e 2048 > /sys/block/md0/queue/read_ahead_kb
    echo 100000 > /sys/block/md0/md/sync_speed_min

    set +o verbose
    echo -ne "[Done]\n"

fi

sleep 2
echo -ne " * Network setup:              "
if [ "$LINK_NUM" == "2" ]; then
    [ ! -e "$(which ifenslave)" ] && do_error "missing ifenslave in rootfs (aptitude install ifenslave)"
    set -o verbose
    ifconfig eth0 0.0.0.0 down
    ifconfig eth1 0.0.0.0 down

    ifconfig bond0 192.168.0.5 netmask 255.255.255.0 down
    echo balance-xor > /sys/class/net/bond0/bonding/mode
    ifconfig bond0 up
    ifenslave bond0 eth0 eth1

    set +o verbose
    echo -ne "[Done]\n"
elif [ "$LINK_NUM" == "4" ]; then
    [ ! -e "$(which ifenslave)" ] && do_error "missing ifenslave in rootfs (aptitude install ifenslave)"
    set -o verbose
    ifconfig eth0 0.0.0.0 down
    ifconfig eth1 0.0.0.0 down
    ifconfig eth2 0.0.0.0 down
    ifconfig eth3 0.0.0.0 down

    ifconfig bond0 192.168.0.5 netmask 255.255.255.0 down
    echo balance-xor > /sys/class/net/bond0/bonding/mode
    ifconfig bond0 up
    ifenslave bond0 eth0 eth1 eth2 eth3

    set +o verbose
    echo -ne "[Done]\n"
elif [ "$LINK_NUM" == "1" ]; then
    set -o verbose
    ifconfig eth0 192.168.0.5 netmask 255.255.255.0 up
    set +o verbose
    echo -ne "[Done]\n"
elif [ "$LINK_NUM" == "0" ]; then
    echo -ne "[Skip]\n"
fi

echo -ne " * Network optimization:       "

if [ "$PLATFORM" == "a300" ] || [ "$PLATFORM" == "a310" ]; then
# KW section, LSP_5.1.3
    [ ! -e "$(which mv_eth_tool)" ] && do_error "missing mv_eth_tool in rootfs"
    set -o verbose
    mv_eth_tool -txdone 8
    mv_eth_tool -txen 0 1
    mv_eth_tool -reuse 1
    mv_eth_tool -recycle 1
    mv_eth_tool -lro 0 1
    mv_eth_tool -rxcoal 0 160
    set +o verbose
elif [ "$PLATFORM" == "axp" ]; then
    [ ! -e "$(which ethtool)" ] && do_error "missing ethtool in rootfs"
    set -o verbose
    echo 1 > /sys/devices/platform/neta/gbe/skb
    for (( j=0; j<$LINK_NUM; j++ )); do
        # set RX coalecing to zero on all port 0 queues
	for (( i=0; i<=$NETQ; i++ )); do
	    echo "$j $i 100" > /sys/devices/platform/neta/gbe/rxq_time_coal
	done

	ethtool -k eth$j > /dev/null
    done
    set +o verbose
elif [ "$PLATFORM" == "a370" ]; then
    [ ! -e "$(which ethtool)" ] && do_error "missing ethtool in rootfs"
    set -o verbose
    ethtool -C eth0 pkt-rate-low 20000 pkt-rate-high 3000000 rx-frames 100 \
	rx-usecs 1500 rx-usecs-high 1500 adaptive-rx on
    set +o verbose
elif [ "$PLATFORM" == "a375" ]; then
    [ ! -e "$(which ethtool)" ] && do_error "missing ethtool in rootfs"
    set -o verbose
    ethtool -C eth0 pkt-rate-low 20000 pkt-rate-high 3000000 rx-frames 32 \
        rx-usecs 1150 rx-usecs-high 1150 rx-usecs-low 100 rx-frames-low 32 \
	rx-frames-high 32 adaptive-rx on
    set +o verbose
elif [ "$PLATFORM" == "a380" ]; then
    [ ! -e "$(which ethtool)" ] && do_error "missing ethtool in rootfs"
#    set -o verbose
#    ethtool -C eth0 pkt-rate-low 20000 pkt-rate-high 3000000 rx-frames 100 \
#	rx-usecs 1500 rx-usecs-high 1500 adaptive-rx on
#    set +o verbose
    echo 250000 > /proc/sys/net/ipv4/tcp_limit_output_bytes
elif [ "$PLATFORM" == "a385" ]; then
    [ ! -e "$(which ethtool)" ] && do_error "missing ethtool in rootfs"
#    set -o verbose
#    ethtool -C eth0 pkt-rate-low 20000 pkt-rate-high 3000000 rx-frames 100 \
#	rx-usecs 1500 rx-usecs-high 1500 adaptive-rx on
#    set +o verbose
    echo 250000 > /proc/sys/net/ipv4/tcp_limit_output_bytes
elif [ "$PLATFORM" == "a388" ]; then
    [ ! -e "$(which ethtool)" ] && do_error "missing ethtool in rootfs"
#    set -o verbose
#    ethtool -C eth0 pkt-rate-low 20000 pkt-rate-high 3000000 rx-frames 100 \
#	rx-usecs 1500 rx-usecs-high 1500 adaptive-rx on
#    set +o verbose
    echo 250000 > /proc/sys/net/ipv4/tcp_limit_output_bytes
fi

echo -ne "[Done]\n"
sleep 2

chmod 777 /mnt/
mkdir -p /mnt/usb
chmod 777 /mnt/usb

for (( i=0; i<4; i++ )); do
    mkdir -p /mnt/public/share$i
    chmod 777 /mnt/public/share$i
done

# Samba
if [ "$SAMBASTATUS" == "enabled" ]; then

    echo -ne " * Starting Samba daemons:     "
    if [[ -e "$(which smbd)" && -e "$(which nmbd)" ]]; then
	chmod 0755 /var/lock
	rm -rf /etc/smb.conf
	touch  /etc/smb.conf
	echo '[global]' 				>>  /etc/smb.conf
	echo ' netbios name = debian-armada'		>>  /etc/smb.conf
	echo ' workgroup = WORKGROUP'			>>  /etc/smb.conf
	echo ' server string = debian-armada'		>>  /etc/smb.conf
	echo ' encrypt passwords = yes'			>>  /etc/smb.conf
	echo ' security = user'				>>  /etc/smb.conf
	echo ' map to guest = bad password'		>>  /etc/smb.conf
	echo ' use mmap = yes'				>>  /etc/smb.conf
	echo ' use sendfile = yes'			>>  /etc/smb.conf
	echo ' dns proxy = no'				>>  /etc/smb.conf
	echo ' max log size = 200'			>>  /etc/smb.conf
	echo ' log level = 0'				>>  /etc/smb.conf
	echo ' socket options = IPTOS_LOWDELAY TCP_NODELAY' >>  /etc/smb.conf
	echo ' local master = no'			>>  /etc/smb.conf
	echo ' dns proxy = no'				>>  /etc/smb.conf
	echo ' ldap ssl = no'				>>  /etc/smb.conf
	echo ' create mask = 0666'			>>  /etc/smb.conf
	echo ' directory mask = 0777'			>>  /etc/smb.conf
	echo ' show add printer wizard = No'		>>  /etc/smb.conf
	echo ' printcap name = /dev/null'		>>  /etc/smb.conf
	echo ' load printers = no'			>>  /etc/smb.conf
	echo ' disable spoolss = Yes'			>>  /etc/smb.conf
	echo ' max xmit = 131072'			>>  /etc/smb.conf
	echo ' disable netbios = yes'			>>  /etc/smb.conf
	echo ' csc policy = disable'			>>  /etc/smb.conf
	echo ' strict allocate = yes'			>>  /etc/smb.conf
	if [ "$FS" == "btrfs" ]; then
		# crash identified with btrfs
		echo '# min receivefile size = 16k'	>>  /etc/smb.conf
	else
		echo ' min receivefile size = 16k'	>>  /etc/smb.conf
	fi
	echo ''						>>  /etc/smb.conf
	echo '[public]'					>>  /etc/smb.conf
	echo ' comment = my public share'		>>  /etc/smb.conf
	echo ' path = /mnt/public'			>>  /etc/smb.conf
	echo ' writeable = yes'				>>  /etc/smb.conf
	echo ' printable = no'				>>  /etc/smb.conf
	echo ' public = yes'				>>  /etc/smb.conf
	echo ''						>>  /etc/smb.conf
	echo ''						>>  /etc/smb.conf
	echo '[usb]'					>>  /etc/smb.conf
	echo ' comment = usb share'			>>  /etc/smb.conf
	echo ' path = /mnt/usb'				>>  /etc/smb.conf
	echo ' writeable = yes'				>>  /etc/smb.conf
	echo ' printable = no'				>>  /etc/smb.conf
	echo ' public = yes'				>>  /etc/smb.conf
	echo ''						>>  /etc/smb.conf

	rm -rf /var/log/log.smbd
	rm -rf /var/log/log.nmbd

	$(which nmbd) -D -s /etc/smb.conf
	$(which smbd) -D -s /etc/smb.conf

	sleep 1
	echo -ne "[Done]\n"
    fi
fi

echo -ne " * Setting up affinity:        "
if [ "$PLATFORM" == "axp" ]; then
    if [ "$CPU_COUNT" = "4" ]; then
	set -o verbose

	# XOR Engines
	echo 1 > /proc/irq/51/smp_affinity
	echo 2 > /proc/irq/52/smp_affinity
	echo 4 > /proc/irq/94/smp_affinity
	echo 8 > /proc/irq/95/smp_affinity
	# ETH
	echo 1 > /proc/irq/8/smp_affinity
	echo 2 > /proc/irq/10/smp_affinity
	echo 4 > /proc/irq/12/smp_affinity
	echo 8 > /proc/irq/14/smp_affinity
	# SATA
	echo 2 > /proc/irq/55/smp_affinity
	# PCI-E SATA controller
	echo 4 > /proc/irq/99/smp_affinity
	echo 8 > /proc/irq/103/smp_affinity

	set +o verbose
    elif [ "$CPU_COUNT" == "2" ]; then
	set -o verbose

	# XOR Engines
	echo 1 > /proc/irq/51/smp_affinity
	echo 2 > /proc/irq/52/smp_affinity
	echo 1 > /proc/irq/94/smp_affinity
	echo 2 > /proc/irq/95/smp_affinity
	# ETH
	echo 1 > /proc/irq/8/smp_affinity
	echo 2 > /proc/irq/10/smp_affinity
	# SATA
	echo 2 > /proc/irq/55/smp_affinity
	# PCI-E SATA controller
	echo 2  > /proc/irq/99/smp_affinity

	set +o verbose
    fi
elif [ "$PLATFORM" == "a375" ]; then
        set -o verbose

        # XOR Engines
        #echo 1 > /proc/irq/54/smp_affinity
        #echo 2 > /proc/irq/55/smp_affinity
        echo 2 > /proc/irq/97/smp_affinity
        echo 2 > /proc/irq/98/smp_affinity
        # SATA
	echo 2 > /proc/irq/58/smp_affinity

        set +o verbose
elif [ "$PLATFORM" == "a385" ]; then
	set -o verbose

	# XOR Engines
	echo 1 > /proc/irq/54/smp_affinity
	echo 2 > /proc/irq/55/smp_affinity
	echo 1 > /proc/irq/97/smp_affinity
	echo 2 > /proc/irq/98/smp_affinity
	# ETH
	echo 1 > /proc/irq/363/smp_affinity
	echo 2 > /proc/irq/365/smp_affinity
	# SATA
#	echo 2 > /proc/irq/61/smp_affinity
	# PCI-E SATA controller
	echo 2  > /proc/irq/61/smp_affinity

	set +o verbose
elif [ "$PLATFORM" == "a388" ]; then
	set -o verbose

	# XOR Engines
	echo 1 > /proc/irq/54/smp_affinity
	echo 1 > /proc/irq/55/smp_affinity
	echo 2 > /proc/irq/97/smp_affinity
	echo 2 > /proc/irq/98/smp_affinity
	# ETH
#	echo 1 > /proc/irq/363/smp_affinity
#	echo 2 > /proc/irq/365/smp_affinity
	# SATA
	echo 2 > /proc/irq/60/smp_affinity
	# PCI-E SATA controller
	echo 2  > /proc/irq/61/smp_affinity

	set +o verbose
fi
echo -ne "[Done]\n"

case "$TOPOLOGY" in
    rd1 | rd5 | rd6)
		#watch "cat /proc/mdstat|grep finish"
	;;
esac

echo -ne "\n==============================================\n"
