#!/bin/bash

VERSION="6.0"
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
SMBCONF="/etc/samba/smb.conf"
MNT_DIR="/mnt/public"
DEF_IP_ADDR="192.168.0.5"
DEF_IP_MASK="255.255.255.0"
# Encryption variables
CRYPTO_NAME="cryptovol1"
KEY_SIZE="192"
ALGORITHIM="aes"

STAMP="[$(date +%H:%M-%d%b%Y)]:"
echo -e "Script parameters has been updated! please read help!"

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


echo " * Version: ${VERSION}"

while getopts "l:ps:mzun:f:t:h:j:" flag; do
    case "$flag" in
	f)
	    FS=$OPTARG
	    case "$OPTARG" in
		ext4|btrfs|xfs|fat32)	echo "Filesystem: ${OPTARG}" ;;
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
		sd|rd0|rd1|rd5|rd6|crypt_sd|crypt_rd0|crypt_rd1|crypt_rd5|crypt_rd6) ;;
		*)	do_error "-t: wrong option" ;;
	    esac
	    ;;
	s)	PLATFORM=$OPTARG
	    case "$OPTARG" in
		a380|a385|a388|a37xx|none) ;;
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
	    echo "           -s <a380|a385|a388|a37xx|none>: platform used with nas_init"
	    echo "           -f <ext4|xfs|btrfs|fat32>: file system type ext4, xfs, btrfs or fat32"
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
    a380|a385|a388|a37xx|none)	;;
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
    crypt_rd0)	echo -ne "Encrypted RAID0\n" ;;
    crypt_rd1)	echo -ne "Encrypted RAID1\n" ;;
    crypt_rd5)	echo -ne "Encrypted RAID5\n" ;;
    crypt_rd6)	echo -ne "Encrypted RAID6\n" ;;
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


if [[ $TOPOLOGY == "crypt"* ]]; then
    [ ! -e "$(which cryptsetup)" ] && do_error "cryptsetup in not installed, can't use encrypted drives"
fi

if [ "$SAMBASTATUS" == "enabled" ]; then
    if [ -e "/etc/init.d/S91smb" ]; then
	/etc/init.d/S91smb stop
    else
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
    fi
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
if [[ $TOPOLOGY == *"sd" ]]; then
    if [ "$SYSDISKEXIST" == "yes" ]; then
	DRIVES="b"
    else
	DRIVES="a"
    fi
    PARTSIZE="55GB"
elif [[ $TOPOLOGY == *"rd0" ]]; then
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
elif [[ $TOPOLOGY == *"rd1" ]]; then
    if [ "$SYSDISKEXIST" == "yes" ]; then
	DRIVES="b c"
    else
	DRIVES="a b"
    fi
    PARTSIZE="55GB"
    HDD_NUM=2
    LEVEL=1
elif [[ $TOPOLOGY == *"rd5" ]]; then
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
elif [[ $TOPOLOGY == *"rd6" ]]; then
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
	fat32) [ ! -e "$(which mkfs.vfat)" ] \
	    && do_error "missing mkfs.vfat in rootfs" ;;
	*) do_error "no valid filesystem specified" ;;
    esac

    case "$1" in
	ext4)
	    case "$2" in
		*sd) STRIPE=0
		    ;;
		*rd1) STRIPE=$STRIDE
		    ;;
		*rd0) STRIPE=$((STRIDE * HDD_NUM))
	            ;;
		*rd5) HDD_NUM=$((HDD_NUM - 1))
		    STRIPE=$((STRIDE * HDD_NUM))
		    ;;
		*rd6) HDD_NUM=$(($HDD_NUM - 2))
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
	fat32) mkfs.vfat $3 -s 128 -S 512 -F 32
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
	mount -t ext4 $2 $3 -o lazytime,data=writeback,barrier=0
    elif [ "$1" == "xfs" ]; then
	mount -t xfs $2 $3 -o noatime,nodirspread
    elif [ "$1" == "btrfs" ]; then
	mount -t btrfs $2 $3 -o noatime,thread_pool=$CPU_COUNT
    elif [ "$1" == "fat32" ]; then
	mount -t vfat  $2 $3 -o rw,noatime,umask=0000
    else
	do_error "unsupported filesystem $1\n"
    fi

    if [ `mount | grep $3 | grep -v grep | wc -l` == 0 ]; then
	do_error "Failed to mount FS $1 from $2 to dir $3"
    fi
}

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

if [ -e "/dev/md"* ]; then
	echo -ne " * Stopping all MD devices:    "
	mdadm -S /dev/md*
	sleep 2
	echo -ne "[Done]\n"
fi

if [[ $TOPOLOGY == *"sd" ]]; then
    PARTITIONS="/dev/sd${DRIVES}${PARTNUM}"
    echo -ne " * Starting single disk:       "
    set -o verbose

    echo -e 1024 > /sys/block/sd${DRIVES}/queue/read_ahead_kb


    if [ "$MKFS" == "yes" ]; then
	for partition in `echo $DRIVES`; do mdadm --zero-superblock /dev/sd${partition}${PARTNUM}; done
	sleep 2

	if [[ $TOPOLOGY == "crypt"* ]]; then
	    echo -ne "Encrypted: "
            # create encryption key
	    dd if=/dev/urandom of=key bs=$KEY_SIZE count=1
	    cryptsetup -c $ALGORITHIM -d key -s $KEY_SIZE create $CRYPTO_NAME $PARTITIONS
	    PARTITIONS="/dev/mapper/$CRYPTO_NAME"
	fi

	create_fs $FS $TOPOLOGY $PARTITIONS

    elif [[ $TOPOLOGY == "crypt"* ]]; then
	echo -ne "Encrypted: "
	cryptsetup -c $ALGORITHIM -d key -s $KEY_SIZE create $CRYPTO_NAME $PARTITIONS
	PARTITIONS="/dev/mapper/$CRYPTO_NAME"
    fi

    mount_fs $FS $PARTITIONS $MNT_DIR

    set +o verbose
    echo -ne "[Done]\n"
else # RAID TOPOLOGY
    TARGET_DRIVE="/dev/md0"
    [ ! -e "$(which mdadm)" ] && do_error "missing mdadm in rootfs (aptitude install mdadm)"

    echo -ne " * Starting $TOPOLOGY build:       "
    for drive in `echo $DRIVES`; do PARTITIONS="${PARTITIONS} /dev/sd${drive}${PARTNUM}"; done

    set -o verbose

    if [ "$MKFS" == "yes" ]; then
	mdadm -S /dev/md*
	sleep 2
	for partition in `echo $DRIVES`; do mdadm --zero-superblock /dev/sd${partition}${PARTNUM}; done
	sleep 2

	echo "y" | mdadm --create -c 128 $TARGET_DRIVE --level=$LEVEL -n $HDD_NUM --force $PARTITIONS
	sleep 2

	if [ `cat /proc/mdstat  |grep md0 |wc -l` == 0 ]; then
	    do_error "Unable to create RAID device"
	fi

	if [[ $TOPOLOGY == "crypt"* ]]; then
	    echo -ne "Encrypted: "
	    # create encryption key
	    dd if=/dev/urandom of=key bs=$KEY_SIZE count=1
	    cryptsetup -c $ALGORITHIM -d key -s $KEY_SIZE create $CRYPTO_NAME $TARGET_DRIVE
	    TARGET_DRIVE="/dev/mapper/$CRYPTO_NAME"
	fi

	create_fs $FS $TOPOLOGY $TARGET_DRIVE $HDD_NUM

    else
	# need to reassemble the raid
	mdadm --assemble /dev/md0 --force $PARTITIONS

	if [ `cat /proc/mdstat  |grep md0 |wc -l` == 0 ]; then
	    do_error "Unable to assemble RAID device"
	fi

	if [[ $TOPOLOGY == "crypt"* ]]; then
	    echo -ne "Encrypted: "
	    [ ! -e "key" ] && do_error "no key file available, please locate or use -m to create a new key"
	    cryptsetup -c $ALGORITHIM -d key -s $KEY_SIZE create $CRYPTO_NAME /dev/md0
	    TARGET_DRIVE="/dev/mapper/$CRYPTO_NAME"
	fi
    fi

    mount_fs $FS $TARGET_DRIVE $MNT_DIR

    if [[ $TOPOLOGY != *"rd1" ]]; then
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

    ifconfig bond0 $DEF_IP_ADDR netmask $DEF_IP_MASK down
    echo layer2+3    > /sys/class/net/bond0/bonding/xmit_hash_policy
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

    ifconfig bond0 $DEF_IP_ADDR netmask $DEF_IP_MASK down
    echo layer2+3    > /sys/class/net/bond0/bonding/xmit_hash_policy
    echo balance-xor > /sys/class/net/bond0/bonding/mode
    ifconfig bond0 up
    ifenslave bond0 eth0 eth1 eth2 eth3

    set +o verbose
    echo -ne "[Done]\n"
elif [ "$LINK_NUM" == "1" ]; then
    set -o verbose
    ifconfig eth0 $DEF_IP_ADDR netmask $DEF_IP_MASK up
    set +o verbose
    echo -ne "[Done]\n"
else
    echo -ne "[Skip]\n"
fi

echo -ne " * Network optimization:       "

if [ "$PLATFORM" == "a380" ]; then
	echo 250000 > /proc/sys/net/ipv4/tcp_limit_output_bytes
	echo -ne "[Done]\n"
elif [ "$PLATFORM" == "a385" ]; then
	echo 250000 > /proc/sys/net/ipv4/tcp_limit_output_bytes
	echo -ne "[Done]\n"
elif [ "$PLATFORM" == "a388" ]; then
	[ ! -e "$(which ethtool)" ] && do_error "missing ethtool in rootfs"
	set -o verbose
	for i in 0 1 ; do
		ethtool -C  eth$i pkt-rate-low 20000 pkt-rate-high 3000000	\
			rx-usecs-low  60       rx-frames-low  32		\
			rx-usecs      80       rx-frames      32		\
			rx-usecs-high 1150     rx-frames-high 32  adaptive-rx on
	done
	set +o verbose
	echo 250000 > /proc/sys/net/ipv4/tcp_limit_output_bytes
	echo -ne "[Done]\n"
else
	echo -ne "[Skip]\n"
fi

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
	rm -rf $SMBCONF
	touch  $SMBCONF
cat << EOF > $SMBCONF
[global]
	netbios name = marvell-nas
	workgroup = WORKGROUP
	server string = marvell-nas
	encrypt passwords = yes
	security = user
	map to guest = bad password
	use mmap = yes
	use sendfile = yes
	dns proxy = no
	max log size = 200
	log level = 0
	socket options = IPTOS_LOWDELAY TCP_NODELAY
	local master = no
	dns proxy = no
	ldap ssl = no
	create mask = 0777
	directory mask = 0777
	show add printer wizard = no
	printcap name = /dev/null
	load printers = no
	disable spoolss = yes
	max xmit = 131072
	disable netbios = yes
	csc policy = disable
	strict allocate = yes
EOF
	if [ "$FS" == "ext4" ] || [ "$FS" == "btrfs" ]; then
		echo -e '\tmin receivefile size = 16k' >>  $SMBCONF
	else
		echo -e '\t# min receivefile size = 16k' >>  $SMBCONF
	fi

cat << EOF >> $SMBCONF
[public]
	comment = my public share
	path = /mnt/public
	writeable = yes
	printable = no
	public = yes

[usb]
	comment = usb share
	path = /mnt/usb
	writeable = yes
	printable = no
	public = yes
EOF

	rm -rf /var/log/log.smbd
	rm -rf /var/log/log.nmbd

	if [ -e "/etc/init.d/S91smb" ]; then
		/etc/init.d/S91smb start
	else
		$(which nmbd) -D -s $SMBCONF
		$(which smbd) -D -s $SMBCONF
	fi
	sleep 1
	echo -ne "[Done]\n"
    else
	echo -ne "[Skip]\n"
    fi
fi

echo -ne " * Setting up affinity:        "
if [ "$PLATFORM" == "a385" ]; then
	set -o verbose

	# XOR Engines
	echo 1 > /proc/irq/54/smp_affinity
	echo 2 > /proc/irq/55/smp_affinity
	echo 1 > /proc/irq/97/smp_affinity
	echo 2 > /proc/irq/98/smp_affinity
	# ETH
	echo 1 > /proc/irq/363/smp_affinity
	echo 2 > /proc/irq/365/smp_affinity
	# PCI-E SATA controller
	echo 2  > /proc/irq/61/smp_affinity

	set +o verbose
	echo -ne "[Done]\n"
elif [ "$PLATFORM" == "a388" ]; then
	set -o verbose

	# XOR Engines
	echo 1 > /proc/irq/54/smp_affinity
	echo 1 > /proc/irq/55/smp_affinity
	echo 2 > /proc/irq/97/smp_affinity
	echo 2 > /proc/irq/98/smp_affinity
	# SATA
	echo 2 > /proc/irq/60/smp_affinity
	# PCI-E SATA controller
	echo 2  > /proc/irq/61/smp_affinity

	set +o verbose
	echo -ne "[Done]\n"
else
	echo -ne "[Skip]\n"
fi

echo -ne "\n==============================================\n"
