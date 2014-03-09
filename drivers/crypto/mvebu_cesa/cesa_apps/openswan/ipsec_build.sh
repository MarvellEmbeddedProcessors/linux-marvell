# IPSEC Module build script

# Optionaly download the sources from web
#wget http://download.openswan.org/openswan/openswan-2.6.37.tar.gz

# Extract sources localy
tar -zxvf openswan-2.6.37.tar.gz
cd openswan-2.6.37

# Apply marvell patch over openswan sources
patch -p1 < ../0001-mv_openswan_2_6_37.patch

# Build ipsec module
make KERNELSRC=../../../ module ARCH=arm

# Copy to modules output directory
mkdir -p ${1}/kernel/ipsec
cp modobj26/ipsec.ko ${1}/kernel/ipsec
