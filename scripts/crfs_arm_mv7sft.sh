#!/bin/bash

##############################################################
#                                                            #
#          crfs_arm.sh  - Create Root File System            #
#                                                            #
#       Written by Aharon Gadot <aharong@marvell.com>        #
#                     Sep 01, 2006                           #
#                                                            #
#      Building root filesystem for embedded Linux system    #
#                                                            #
##############################################################
#
## functions
#

usage_exit() {
	echo
	echo "Incorrect parameters"
        echo "Usage: crfs_arm_mv7sft.sh <ROOTFS_PATH> <MV7SFT_PATH> <BUSYBOX_PATH>"
	echo
        echo "Examples: "
				echo
        echo "1. Little endian: "
				echo
        echo " ./crfs_arm_mv7sft.sh /tftpboot/rootfs_arm-mv7sft /swtools/devtools/gnueabi/arm_le/armv7-marvell-linux-gnueabi-softfp /swtools/devsources/root_fs/files/busybox "
#        echo ""
#        echo "2. Big endian: "
#				echo
#        echo " ./crfs_arm_mv7sft.sh /tftpboot/rootfs_arm-mv7sft-be /swtools/devtools/gnueabi/arm_be/armeb-linux-gnueabi /swtools/devsources/root_fs/files/busybox "
#        echo ""
        exit 1
}

#
## main
#

HERE=`dirname $0`
HERE="`cd \"$HERE\" 2>/dev/null && pwd || echo \"$HERE\"`"
echo ""
if [ -n "$1" ]
then
  if [ "$1" = "--help" ]
  then
    echo "Usage: crfs_arm_mv7sft.sh <ROOTFS_PATH> <MV7SFT_PATH> <BUSYBOX_PATH>."
    echo " Create root filesystem for ARM CPU in path given by <ROOTFS_PATH>."
    echo " <BUSYBOX_PATH> must contain busybox-1.01.tar.bz2 or busybox-1.01.tar.gz file"
    echo " Busybox source can be downloaded from http://busybox.net/downloads/."
    echo " <MV7SFT_PATH> - path where MV7SFT toolchain or newer package was installed."
    echo ""
    echo "Example: "
    echo " ./crfs_arm_mv7sft.sh /tftpboot/rootfs_arm-mv7sft /swtools/devtools/gnueabi/arm_le/armv7-marvell-linux-gnueabi-softfp /swtools/devsources/root_fs/files/busybox "
    echo ""
    exit 0
  fi
fi
  
if [ $# -ne 3 ]
then
	usage_exit
fi

dname=`dirname "$1"`
bname=`basename "$1"`
start_path="`cd \"$dname\" 2>/dev/null && pwd || echo \"$dname\"`/$bname"
if [ -e $1 ]
then
	echo "File system root: $start_path"
else
	echo "Creating file system root: $start_path"
	/bin/mkdir $start_path
fi 

cd $HERE
runtime_files_bz2_uu=runtime_files.tar.bz2.uu
runtime_files=/tmp/__runtime_files_$$
rm -rf ${runtime_files}
mkdir -p ${runtime_files}
cp $HERE/runtime_files.tar.bz2.uu ${runtime_files}
cd ${runtime_files}
uudecode ${runtime_files_bz2_uu}
bunzip2 -dc runtime_files.tar.bz2 | tar -xvf -
mv runtime_files/* .
cd ..

dname=`dirname "$2"`
bname=`basename "$2"`
mv7sft="`cd \"$dname\" 2>/dev/null && pwd || echo \"$dname\"`/$bname"
if [ -e $mv7sft ] 
then
	echo "MV7SFT path: $mv7sft"
else
	echo "MV7SFT not found in $2. Exiting..."
	echo ""
	exit 1  
fi  

if echo $mv7sft | grep 'arm_be'  || echo $mv7sft | grep 'armeb' || echo $mv7sft | grep 'armbe' ; then
	eb=eb
	endian=be
	endianess_string="big endian"
else
	eb=
	endian=le
	endianess_string="little endian"
fi
echo "Endianess $endianess_string"
dname=`dirname "$3"`
bname=`basename "$3"`
busybox_path="`cd \"$dname\" 2>/dev/null && pwd || echo \"$dname\"`/$bname" 

if [ `whoami` = "root" ]; then
	sudo=
else
	sudo=sudo
fi

user_path=$(pwd)
cd $start_path

if [ -e $busybox_path/busybox-1.01.tar.bz2 ]
then
  echo "Uncompressing Busybox tar.bz2 archive..."
  tar xjf $busybox_path/busybox-1.01.tar.bz2
  echo -e "\bDone.\n"
elif [ -e $busybox_path/busybox-1.01.tar.gz ]   
then 
  echo "Uncompressing Busybox tar.gz archive..."  
  tar xzf $busybox_path/busybox-1.01.tar.gz
  echo -e "\bDone.\n"    
else
  echo "Busybox 1.01 archive not found at path $busybox_path. Exiting..."    
  echo ""
  exit 1
fi

#create file structure
#---------------------
mkdir bin
mkdir sbin
mkdir root
mkdir home
mkdir home/user
mkdir mnt
mkdir mnt/flash
mkdir mnt/nfs
mkdir etc
mkdir proc
mkdir sys
mkdir tmp
mkdir lib
mkdir lib/modules
mkdir usr
mkdir usr/lib
mkdir usr/bin
mkdir usr/local
mkdir usr/sbin
mkdir usr/share
mkdir var
mkdir var/lib
mkdir var/lock
mkdir var/log
mkdir var/run
mkdir var/tmp

echo "Creating devices"
#create structure of devices
#---------------------------
cd $user_path
cd $start_path
mkdir dev
cd dev

#create dev folders
mkdir pts
mkdir shm
mkdir net

# create block devices
$sudo mknod loop0     b  7 0
$sudo mknod loop1     b  7 1
$sudo mknod ram0      b  1 0
$sudo mknod ram1      b  1 1
$sudo mknod mtdblock0 b 31 0
$sudo mknod mtdblock1 b 31 1
$sudo mknod mtdblock2 b 31 2

# create character devices
$sudo mknod mem     c   1   1
$sudo mknod null    c   1   3
$sudo mknod zero    c   1   5
$sudo mknod random  c   1   8
$sudo mknod ptyp0   c   2   0
$sudo mknod ptyp1   c   2   1
$sudo mknod ptyp2   c   2   2
$sudo mknod ttyp0   c   3   0
$sudo mknod ttyp1   c   3   1
$sudo mknod ttyp2   c   3   2
$sudo mknod tty0    c   4   0
$sudo mknod ttyS0   c   4  64
$sudo mknod tty     c   5   0
$sudo mknod console c   5   1
$sudo mknod ptmx    c   5   2
$sudo mknod sda     b   8   0
$sudo mknod sda1    b   8   1
$sudo mknod sda2    b   8   2
$sudo mknod sda3    b   8   3
$sudo mknod net/tun c  10 200
$sudo mknod mtd0    c  90   0 
$sudo mknod mtd0ro  c  90   1
$sudo mknod mtd1    c  90   2 
$sudo mknod mtd1ro  c  90   3
$sudo mknod mtd2    c  90   4 
$sudo mknod mtd2ro  c  90   5
$sudo mknod mvROS   c 250   0
$sudo mknod mvPP    c 244   0
$sudo mknod mvKernelExt c 244  1
$sudo mknod i2c-0 c 89  0
$sudo mknod i2c-1 c 89  1

# create links
ln -s ttyS0 ttys0
ln -s ttyS0 tty5
ln -s ttyp1 tty1
ln -s mtd0  mtd

echo "Building libraries"
# copy libraries
# --------------
cd $user_path
cd $start_path

cd usr/bin
usr_bin_path=`pwd`

cd ../../lib
lib_path=`pwd`

mv7sft_lib=$mv7sft/arm-marvell-linux-gnueabi/libc/lib
mv7sft_lib2=$mv7sft_lib
mv7sft_prefix=$mv7sft/bin/arm${eb}-marvell-linux-gnueabi-

cd  $mv7sft_lib
cp -d libc-* libc.* libm*  ld-* libcrypt* libpthread* libdl* libSegFault* $lib_path

cp $runtime_files/usr/bin/*  ${lib_path}/../usr/bin
chmod +x ${lib_path}/../usr/bin/*

cd  $mv7sft_lib2
cp -d libgcc_s* $lib_path

# strip the libraries except libthread_db
${mv7sft_prefix}strip ${lib_path}/l* > /dev/null 2>&1

cp -d $mv7sft_lib/libthread_db* $lib_path

cd $lib_path
rm -f *orig*

echo "Creating etc files"
# create init files 
# -----------------
cd $user_path
cd $start_path
cd etc

# creating passwd
echo -e "root::0:0:root:/root:/bin/sh\n\nuser::500:500:Linux User,,,:/home/user:/bin/sh\n" >./passwd

# creating group
echo -e "root:x:1:root\nuser:x:500:\n" >./group
# creating inittab
echo -e "\n\t# autoexec\n\t::respawn:/etc/init.sh\n\n\t# Stuff to do when restarting the init process\n\t::restart:/sbin/init\n" >./inittab

# creating motd
cat << EOF > ./motd


Welcome to Embedded Linux
           _  _
          | ||_|
          | | _ ____  _   _  _  _ 
          | || |  _ \\| | | |\\ \\/ /
          | || | | | | |_| |/    \\
          |_||_|_| |_|\\____|\\_/\\_/

          On Marvell's ARMADAXP board

For further information on the Marvell products check:
http://www.marvell.com/

toolchain=mv7sft ${endianess_string}

Enjoy!

EOF


# creating welcome file for telnet
cat << EOF > ./welcome


Welcome to Embedded Linux Telnet
           _  _
          | ||_|
          | | _ ____  _   _  _  _ 
          | || |  _ \\| | | |\\ \\/ /
          | || | | | | |_| |/    \\
          |_||_|_| |_|\\____|\\_/\\_/

          On Marvell's ARM board

toolchain=mv7sft ${endianess_string}

For further information on the Marvell products check:
http://www.marvell.com/
EOF

# creating init.sh
current_date=$(date +%m%d%H%M%Y)
current_date1=$(date)


# creating README.txt
cat << EOF > ./README.txt

File system building information
--------------------------------

build_date = ${current_date1}

sdk_prefix = ${mv7sft}

lib        = \${sdk_prefix}/arm${eb}-mv7sft-linux-gnueabi/libc/lib

cmd_prefix = \${sdk_prefix}/bin/arm${eb}-mv7sft-linux-gnueabi-

gdbserver  = ${gdbserver_file}

EOF

# creating init.sh

cat << EOF > ./init.sh
#!/bin/sh
if test -e /proc/version
then
 echo
else

 hostname MARVELL_LINUX
 HOME=/root

 mount -t proc   proc /proc
 mount -t sysfs  none /sys
 mount -t tmpfs  none /dev/shm -n size=64M
 mount -t devpts none /dev/pts mode=0622

 /usr/sbin/telnetd -l /bin/sh -f /etc/welcome

 if test -e /lib/modules/mvKernelExt.ko
 then
   insmod /lib/modules/mvKernelExt.ko
 fi

# if test -e /lib/modules/mvPpDrv.ko
# then
#   insmod /lib/modules/mvPpDrv.ko
# fi

date $current_date

 # Start the network interface
 /sbin/ifconfig lo 127.0.0.1

 rm -f /tmp/tasks

fi

export LD_PRELOAD=/lib/libSegFault.so

# print logo
# clear
echo
echo
echo
echo
uname -nrsv
cat /etc/motd

if test -e /usr/bin/appDemo
then
  if /sbin/lsmod |grep -iq mvPpDrv
  then 
   /usr/bin/appDemo
  else
    echo mvPpDrv not loaded
  fi
else
  echo /usr/bin/appDemo not found
fi

exec /bin/sh
EOF

chmod 777 init.sh

# creating fstab
cat << EOF > ./fstab
/dev/nfs                /                       nfs     defaults        0 0
none                    /proc                   proc    defaults        0 0
none                    /sys                    sysfs   defaults        0 0
none                    /dev/shm                tmpfs   size=64M        0 0
none                    /dev/pts                devpts  mode=0622       0 0
EOF

# creating .config file for busybox
cd $user_path
cd $start_path/busybox-1.01/

#patch cmdedit.c - add backspace support
patch -p1 << EOF
--- busybox-1.01/shell/cmdedit_old.c	2008-04-03 19:19:32.000000000 +0300
+++ busybox-1.01/shell/cmdedit.c	2008-04-03 19:22:23.000000000 +0300
@@ -1427,8 +1427,8 @@
 				input_backward(1);
 				break;
 			case '3':
-				/* Delete */
-				input_delete();
+				/* Backspace */
+				input_backspace();
 				break;
 			case '1':
 			case 'H':
EOF

#patch ping6.c - fix compile errors with arm-marvell-linux-gnueabi-gcc
patch -p1 <<EOF
--- busybox-1.01/networking/ping6_old.c	2012-01-31 20:22:21.000000000 +0200
+++ busybox-1.01/networking/ping6.c	2012-01-31 20:19:28.000000000 +0200
@@ -243,9 +243,9 @@
 	case ICMP6_PARAM_PROB:				return "Parameter Problem";
 	case ICMP6_ECHO_REPLY:				return "Echo Reply";
 	case ICMP6_ECHO_REQUEST:			return "Echo Request";
-	case ICMP6_MEMBERSHIP_QUERY:		return "Membership Query";
-	case ICMP6_MEMBERSHIP_REPORT:		return "Membership Report";
-	case ICMP6_MEMBERSHIP_REDUCTION:	return "Membership Reduction";
+// case ICMP6_MEMBERSHIP_QUERY:		return "Membership Query";
+// case ICMP6_MEMBERSHIP_REPORT:		return "Membership Report";
+// case ICMP6_MEMBERSHIP_REDUCTION:	return "Membership Reduction";
 	default: 							return "unknown ICMP type";
 	}
 }
EOF

#patch top.c - fix compile errors with arm-marvell-linux-gnueabi-gcc
patch -p1 <<EOF
--- busybox-1.01/procps/top_old.c	2012-01-31 20:24:46.000000000 +0200
+++ busybox-1.01/procps/top.c	2012-01-31 20:17:40.000000000 +0200
@@ -34,7 +34,7 @@
 #include <string.h>
 #include <sys/ioctl.h>
 /* get page info */
-#include <asm/page.h>
+//#include <asm/page.h>
 #include "busybox.h"
 
 //#define FEATURE_CPU_USAGE_PERCENTAGE  /* + 2k */
EOF


#patch insmod.c for kernel 3.0.6
patch -p1 <<EOF
--- busybox-1.01/modutils/insmod_old.c	2012-01-31 20:26:34.000000000 +0200
+++ busybox-1.01/modutils/insmod.c	2012-01-31 20:19:50.000000000 +0200
@@ -3714,6 +3714,8 @@
 		}
 	}
 
+  k_version = 7;
+
 #if defined(CONFIG_FEATURE_2_6_MODULES)
 	if (k_version > 4 && len > 3 && tmp[len - 3] == '.' &&
 			tmp[len - 2] == 'k' && tmp[len - 1] == 'o') {
EOF

#patch procps.c - fix compile errors with arm-marvell-linux-gnueabi-gcc
patch -p1 <<EOF
--- busybox-1.01/libbb/procps_old.c	2012-01-31 20:32:32.000000000 +0200
+++ busybox-1.01/libbb/procps.c	2012-01-19 21:09:56.000000000 +0200
@@ -12,7 +12,7 @@
 #include <string.h>
 #include <stdlib.h>
 #include <unistd.h>
-#include <asm/page.h>
+//#include <asm/page.h>
 
 #include "libbb.h"
 
EOF

cat << EOF > ./.config
#
# Automatically generated make config: don't edit
#
HAVE_DOT_CONFIG=y

#
# General Configuration
#
# CONFIG_FEATURE_BUFFERS_USE_MALLOC is not set
CONFIG_FEATURE_BUFFERS_GO_ON_STACK=y
# CONFIG_FEATURE_BUFFERS_GO_IN_BSS is not set
CONFIG_FEATURE_VERBOSE_USAGE=y
# CONFIG_FEATURE_INSTALLER is not set
# CONFIG_LOCALE_SUPPORT is not set
# CONFIG_FEATURE_DEVFS is not set
# CONFIG_FEATURE_DEVPTS is not set
# CONFIG_FEATURE_CLEAN_UP is not set
CONFIG_FEATURE_SUID=y
# CONFIG_FEATURE_SUID_CONFIG is not set
# CONFIG_SELINUX is not set

#
# Build Options
#
# CONFIG_STATIC is not set
# CONFIG_LFS is not set
USING_CROSS_COMPILER=y
CROSS_COMPILER_PREFIX="arm-marvell-linux-gnueabi-"
EXTRA_CFLAGS_OPTIONS=""

#
# Installation Options
#
# CONFIG_INSTALL_NO_USR is not set
PREFIX="$start_path"

#
# Archival Utilities
#
# CONFIG_AR is not set
CONFIG_BUNZIP2=y
# CONFIG_CPIO is not set
# CONFIG_DPKG is not set
# CONFIG_DPKG_DEB is not set
CONFIG_GUNZIP=y
# CONFIG_FEATURE_GUNZIP_UNCOMPRESS is not set
CONFIG_GZIP=y
# CONFIG_RPM2CPIO is not set
# CONFIG_RPM is not set
CONFIG_TAR=y
CONFIG_FEATURE_TAR_CREATE=y
CONFIG_FEATURE_TAR_BZIP2=y
# CONFIG_FEATURE_TAR_FROM is not set
CONFIG_FEATURE_TAR_GZIP=y
# CONFIG_FEATURE_TAR_COMPRESS is not set
CONFIG_FEATURE_TAR_OLDGNU_COMPATABILITY=y
CONFIG_FEATURE_TAR_GNU_EXTENSIONS=y
# CONFIG_FEATURE_TAR_LONG_OPTIONS is not set
# CONFIG_UNCOMPRESS is not set
CONFIG_UNZIP=y

#
# Common options for cpio and tar
#
# CONFIG_FEATURE_UNARCHIVE_TAPE is not set

#
# Coreutils
#
CONFIG_BASENAME=y
# CONFIG_CAL is not set
CONFIG_CAT=y
CONFIG_CHGRP=y
CONFIG_CHMOD=y
CONFIG_CHOWN=y
CONFIG_CHROOT=y
CONFIG_CMP=y
CONFIG_CP=y
CONFIG_CUT=y
CONFIG_DATE=y
CONFIG_FEATURE_DATE_ISOFMT=y
CONFIG_DD=y
CONFIG_DF=y
CONFIG_DIRNAME=y
# CONFIG_DOS2UNIX is not set
CONFIG_DU=y
CONFIG_FEATURE_DU_DEFALT_BLOCKSIZE_1K=y
CONFIG_ECHO=y
CONFIG_FEATURE_FANCY_ECHO=y
CONFIG_ENV=y
CONFIG_EXPR=y
CONFIG_FALSE=y
# CONFIG_FOLD is not set
CONFIG_HEAD=y
# CONFIG_FEATURE_FANCY_HEAD is not set
# CONFIG_HOSTID is not set
CONFIG_ID=y
CONFIG_INSTALL=y
# CONFIG_LENGTH is not set
CONFIG_LN=y
# CONFIG_LOGNAME is not set
CONFIG_LS=y
CONFIG_FEATURE_LS_FILETYPES=y
CONFIG_FEATURE_LS_FOLLOWLINKS=y
CONFIG_FEATURE_LS_RECURSIVE=y
CONFIG_FEATURE_LS_SORTFILES=y
CONFIG_FEATURE_LS_TIMESTAMPS=y
CONFIG_FEATURE_LS_USERNAME=y
CONFIG_FEATURE_LS_COLOR=y
# CONFIG_MD5SUM is not set
CONFIG_MKDIR=y
# CONFIG_MKFIFO is not set
CONFIG_MKNOD=y
CONFIG_MV=y
# CONFIG_OD is not set
# CONFIG_PRINTF is not set
CONFIG_PWD=y
# CONFIG_REALPATH is not set
CONFIG_RM=y
CONFIG_RMDIR=y
# CONFIG_SEQ is not set
# CONFIG_SHA1SUM is not set
CONFIG_SLEEP=y
# CONFIG_FEATURE_FANCY_SLEEP is not set
CONFIG_SORT=y
# CONFIG_STTY is not set
CONFIG_SYNC=y
CONFIG_TAIL=y
CONFIG_FEATURE_FANCY_TAIL=y
CONFIG_TEE=y
CONFIG_FEATURE_TEE_USE_BLOCK_IO=y
CONFIG_TEST=y

#
# test (forced enabled for use with shell)
#
# CONFIG_FEATURE_TEST_64 is not set
CONFIG_TOUCH=y
CONFIG_TR=y
CONFIG_TRUE=y
CONFIG_TTY=y
CONFIG_UNAME=y
CONFIG_UNIQ=y
CONFIG_USLEEP=y
# CONFIG_UUDECODE is not set
# CONFIG_UUENCODE is not set
# CONFIG_WATCH is not set
CONFIG_WC=y
# CONFIG_WHO is not set
CONFIG_WHOAMI=y
CONFIG_YES=y

#
# Common options for cp and mv
#
CONFIG_FEATURE_PRESERVE_HARDLINKS=y

#
# Common options for ls and more
#
CONFIG_FEATURE_AUTOWIDTH=y

#
# Common options for df, du, ls
#
CONFIG_FEATURE_HUMAN_READABLE=y

#
# Console Utilities
#
CONFIG_CHVT=y
CONFIG_CLEAR=y
CONFIG_DEALLOCVT=y
# CONFIG_DUMPKMAP is not set
# CONFIG_LOADFONT is not set
# CONFIG_LOADKMAP is not set
CONFIG_OPENVT=y
CONFIG_RESET=y
# CONFIG_SETKEYCODES is not set

#
# Debian Utilities
#
CONFIG_MKTEMP=y
# CONFIG_PIPE_PROGRESS is not set
CONFIG_READLINK=y
# CONFIG_RUN_PARTS is not set
# CONFIG_START_STOP_DAEMON is not set
CONFIG_WHICH=y

#
# Editors
#
# CONFIG_AWK is not set
# CONFIG_PATCH is not set
CONFIG_SED=y
# CONFIG_VI is not set

#
# Finding Utilities
#
CONFIG_FIND=y
CONFIG_FEATURE_FIND_MTIME=y
CONFIG_FEATURE_FIND_PERM=y
CONFIG_FEATURE_FIND_TYPE=y
CONFIG_FEATURE_FIND_XDEV=y
# CONFIG_FEATURE_FIND_NEWER is not set
# CONFIG_FEATURE_FIND_INUM is not set
CONFIG_GREP=y
CONFIG_FEATURE_GREP_EGREP_ALIAS=y
CONFIG_FEATURE_GREP_FGREP_ALIAS=y
CONFIG_FEATURE_GREP_CONTEXT=y
CONFIG_XARGS=y
# CONFIG_FEATURE_XARGS_SUPPORT_CONFIRMATION is not set
CONFIG_FEATURE_XARGS_SUPPORT_QUOTES=y
CONFIG_FEATURE_XARGS_SUPPORT_TERMOPT=y
CONFIG_FEATURE_XARGS_SUPPORT_ZERO_TERM=y

#
# Init Utilities
#
CONFIG_INIT=y
CONFIG_FEATURE_USE_INITTAB=y
CONFIG_FEATURE_INITRD=y
# CONFIG_FEATURE_INIT_COREDUMPS is not set
# CONFIG_FEATURE_INIT_SWAPON is not set
CONFIG_FEATURE_EXTRA_QUIET=y
CONFIG_HALT=y
CONFIG_POWEROFF=y
CONFIG_REBOOT=y
# CONFIG_MESG is not set

#
# Login/Password Management Utilities
#
CONFIG_USE_BB_PWD_GRP=y
CONFIG_ADDGROUP=y
CONFIG_DELGROUP=y
CONFIG_ADDUSER=y
CONFIG_DELUSER=y
CONFIG_GETTY=y
# CONFIG_FEATURE_UTMP is not set
# CONFIG_FEATURE_WTMP is not set
CONFIG_LOGIN=y
# CONFIG_FEATURE_SECURETTY is not set
# CONFIG_PASSWD is not set
CONFIG_SU=y
CONFIG_SULOGIN=y
CONFIG_VLOCK=y

#
# Common options for adduser, deluser, login, su
#
CONFIG_FEATURE_SHADOWPASSWDS=y
CONFIG_USE_BB_SHADOW=y

#
# Miscellaneous Utilities
#
# CONFIG_ADJTIMEX is not set
# CONFIG_CROND is not set
# CONFIG_CRONTAB is not set
# CONFIG_DC is not set
# CONFIG_DEVFSD is not set
# CONFIG_LAST is not set
# CONFIG_HDPARM is not set
# CONFIG_MAKEDEVS is not set
# CONFIG_MT is not set
# CONFIG_RX is not set
CONFIG_STRINGS=y
CONFIG_TIME=y
# CONFIG_WATCHDOG is not set

#
# Linux Module Utilities
#
CONFIG_INSMOD=y
CONFIG_FEATURE_2_4_MODULES=y
CONFIG_FEATURE_2_6_MODULES=y
CONFIG_FEATURE_INSMOD_VERSION_CHECKING=y
CONFIG_FEATURE_INSMOD_KSYMOOPS_SYMBOLS=y
CONFIG_FEATURE_INSMOD_LOADINKMEM=y
CONFIG_FEATURE_INSMOD_LOAD_MAP=y
CONFIG_FEATURE_INSMOD_LOAD_MAP_FULL=y
CONFIG_LSMOD=y
CONFIG_MODPROBE=y
CONFIG_RMMOD=y
CONFIG_FEATURE_CHECK_TAINTED_MODULE=y

#
# Networking Utilities
#
CONFIG_FEATURE_IPV6=y
CONFIG_ARPING=y
CONFIG_FTPGET=y
CONFIG_FTPPUT=y
CONFIG_HOSTNAME=y
# CONFIG_HTTPD is not set
CONFIG_IFCONFIG=y
CONFIG_FEATURE_IFCONFIG_STATUS=y
# CONFIG_FEATURE_IFCONFIG_SLIP is not set
CONFIG_FEATURE_IFCONFIG_MEMSTART_IOADDR_IRQ=y
CONFIG_FEATURE_IFCONFIG_HW=y
# CONFIG_FEATURE_IFCONFIG_BROADCAST_PLUS is not set
CONFIG_IFUPDOWN=y
# CONFIG_FEATURE_IFUPDOWN_IP is not set
CONFIG_FEATURE_IFUPDOWN_IP_BUILTIN=y
CONFIG_FEATURE_IFUPDOWN_IPV4=y
# CONFIG_FEATURE_IFUPDOWN_IPV6 is not set
# CONFIG_FEATURE_IFUPDOWN_IPX is not set
# CONFIG_FEATURE_IFUPDOWN_MAPPING is not set
CONFIG_INETD=y
CONFIG_FEATURE_INETD_SUPPORT_BILTIN_ECHO=y
CONFIG_FEATURE_INETD_SUPPORT_BILTIN_DISCARD=y
CONFIG_FEATURE_INETD_SUPPORT_BILTIN_TIME=y
CONFIG_FEATURE_INETD_SUPPORT_BILTIN_DAYTIME=y
CONFIG_FEATURE_INETD_SUPPORT_BILTIN_CHARGEN=y
CONFIG_IP=y
CONFIG_FEATURE_IP_ADDRESS=y
CONFIG_FEATURE_IP_LINK=y
CONFIG_FEATURE_IP_ROUTE=y
# CONFIG_FEATURE_IP_TUNNEL is not set
# CONFIG_IPCALC is not set
# CONFIG_IPADDR is not set
# CONFIG_IPLINK is not set
# CONFIG_IPROUTE is not set
# CONFIG_IPTUNNEL is not set
# CONFIG_NAMEIF is not set
# CONFIG_NC is not set
CONFIG_NETSTAT=y
CONFIG_NSLOOKUP=y
CONFIG_PING=y
CONFIG_FEATURE_FANCY_PING=y
CONFIG_PING6=y
CONFIG_FEATURE_FANCY_PING6=y
CONFIG_ROUTE=y
CONFIG_TELNET=y
CONFIG_FEATURE_TELNET_TTYPE=y
CONFIG_FEATURE_TELNET_AUTOLOGIN=y
CONFIG_TELNETD=y
# CONFIG_FEATURE_TELNETD_INETD is not set
CONFIG_TFTP=y
CONFIG_FEATURE_TFTP_GET=y
CONFIG_FEATURE_TFTP_PUT=y
# CONFIG_FEATURE_TFTP_BLOCKSIZE is not set
# CONFIG_FEATURE_TFTP_DEBUG is not set
# CONFIG_TRACEROUTE is not set
# CONFIG_VCONFIG is not set
CONFIG_WGET=y
CONFIG_FEATURE_WGET_STATUSBAR=y
CONFIG_FEATURE_WGET_AUTHENTICATION=y
CONFIG_FEATURE_WGET_IP6_LITERAL=y

#
# udhcp Server/Client
#
# CONFIG_UDHCPD is not set
# CONFIG_UDHCPC is not set

#
# Process Utilities
#
CONFIG_FREE=y
CONFIG_KILL=y
CONFIG_KILLALL=y
CONFIG_PIDOF=y
CONFIG_PS=y
# CONFIG_RENICE is not set
CONFIG_TOP=y
FEATURE_CPU_USAGE_PERCENTAGE=y
CONFIG_UPTIME=y
# CONFIG_SYSCTL is not set

#
# Another Bourne-like Shell
#
CONFIG_FEATURE_SH_IS_ASH=y
# CONFIG_FEATURE_SH_IS_HUSH is not set
# CONFIG_FEATURE_SH_IS_LASH is not set
# CONFIG_FEATURE_SH_IS_MSH is not set
# CONFIG_FEATURE_SH_IS_NONE is not set
CONFIG_ASH=y

#
# Ash Shell Options
#
CONFIG_ASH_JOB_CONTROL=y
CONFIG_ASH_ALIAS=y
CONFIG_ASH_MATH_SUPPORT=y
CONFIG_ASH_MATH_SUPPORT_64=y
# CONFIG_ASH_GETOPTS is not set
# CONFIG_ASH_CMDCMD is not set
# CONFIG_ASH_MAIL is not set
CONFIG_ASH_OPTIMIZE_FOR_SIZE=y
# CONFIG_ASH_RANDOM_SUPPORT is not set
# CONFIG_HUSH is not set
# CONFIG_LASH is not set
# CONFIG_MSH is not set

#
# Bourne Shell Options
#
# CONFIG_FEATURE_SH_EXTRA_QUIET is not set
# CONFIG_FEATURE_SH_STANDALONE_SHELL is not set
CONFIG_FEATURE_COMMAND_EDITING=y
CONFIG_FEATURE_COMMAND_HISTORY=15
CONFIG_FEATURE_COMMAND_SAVEHISTORY=y
CONFIG_FEATURE_COMMAND_TAB_COMPLETION=y
# CONFIG_FEATURE_COMMAND_USERNAME_COMPLETION is not set
CONFIG_FEATURE_SH_FANCY_PROMPT=y

#
# System Logging Utilities
#
CONFIG_SYSLOGD=y
CONFIG_FEATURE_ROTATE_LOGFILE=y
# CONFIG_FEATURE_REMOTE_LOG is not set
# CONFIG_FEATURE_IPC_SYSLOG is not set
CONFIG_KLOGD=y
CONFIG_LOGGER=y

#
# Linux System Utilities
#
CONFIG_DMESG=y
# CONFIG_FBSET is not set
# CONFIG_FDFLUSH is not set
# CONFIG_FDFORMAT is not set
# CONFIG_FDISK is not set
# CONFIG_FREERAMDISK is not set
# CONFIG_FSCK_MINIX is not set
# CONFIG_MKFS_MINIX is not set
# CONFIG_GETOPT is not set
CONFIG_HEXDUMP=y
# CONFIG_HWCLOCK is not set
# CONFIG_LOSETUP is not set
# CONFIG_MKSWAP is not set
CONFIG_MORE=y
CONFIG_FEATURE_USE_TERMIOS=y
CONFIG_PIVOT_ROOT=y
# CONFIG_RDATE is not set
CONFIG_SWAPONOFF=y
CONFIG_MOUNT=y
CONFIG_NFSMOUNT=y
CONFIG_UMOUNT=y
CONFIG_FEATURE_MOUNT_FORCE=y

#
# Common options for mount/umount
#
CONFIG_FEATURE_MOUNT_LOOP=y
# CONFIG_FEATURE_MTAB_SUPPORT is not set

#
# Debugging Options
#
# CONFIG_DEBUG is not set
EOF

# make and install Busybox
echo -e "Compiling Busybox application. This process may take several minutes.\nPlease wait...\n\n"
#make TARGET_ARCH=arm CROSS=$mv7sft_prefix PREFIX=../. all install >/dev/null 2>/dev/null
make TARGET_ARCH=arm CROSS=$mv7sft_prefix PREFIX=../. all install
ln -s ./bin/busybox $start_path/init

echo -e "\nCompilation completed.\n"

# remove Busybox sources
cd ..
rm -rf busybox*
rm -rf ${runtime_files}

echo ""
echo "Filesystem created successfuly"
echo ""

