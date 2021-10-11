#!/sbin/sh
#
#       @(#)rcS.stub.sh 1.18 96/04/24 SMI
#
# Copyright (c) 1992-1996 Sun Microsystems, Inc.  All Rights Reserved. Sun
# considers its source code as an unpublished, proprietary trade secret, and
# it is available only under strict license provisions.  This copyright
# notice is placed here only to protect Sun in the event the source is
# deemed a published work.  Dissassembly, decompilation, or other means of
# reducing the object code to human readable form is prohibited by the
# license agreement under which this code is provided to the user or company
# in possession of this copy.
#
# RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the Government
# is subject to restrictions as set forth in subparagraph (c)(1)(ii) of the
# Rights in Technical Data and Computer Software clause at DFARS 52.227-7013
# and in similar clauses in the FAR and NASA FAR Supplement.
#

#
# 2.X stub "rcS" file
# run by init(8) to figure where to really boot from to do the install
# replaces normal rcS
#
#* init starts
# *     mount tmpfs onto /tmp
# *     copy any needed files into /tmp...
# *     device reconfigure
# *     figure out where to boot from
# *     reboot
#

################# main ###################

#
# Mount tmpfs and procfs and start accumulating mnttab
# entries for when mnttab is writeable
#
if [ ! -f /tmp/.rcSmnt ]; then
	# mount tmpfs, for now it can't swap so we can't fill it too full
	/sbin/mount -m -F tmpfs swap /tmp
	if [ $? -ne 0 ]; then
		echo "tmpfs mount failed."
		/sbin/sh
	fi
	# start accumulating mnttab entries - no writable files yet
	Setmnt="${Setmnt}/tmp /tmp\n"
	# mount proc
	/sbin/mount -F proc -m proc /proc
	if [ $? -ne 0 ]; then
		echo "proc mount failed."
		/sbin/sh
	fi
	Setmnt="${Setmnt}/proc /proc\n"
	cat < /dev/null > /tmp/.rcSmnt
fi

if [ "${RB_NOBOOTRC}" = "YES" ]; then
	RB_NOBOOTRC="no"
	export RB_NOBOOTRC
	/sbin/sh
fi

#
# * copy over any writeable (prototype) files to /tmp from "/root"
# NOTE: canonical paths are /tmp/root, but use cpio to do mkdir
# NOTE: send output to /tmp (not /dev/null) to avoid nfs bug on remote
#       ro mounted fs
#
( cd /.tmp_proto; find . -print -depth | cpio -pdm /tmp 2>/tmp/cpio.out )

################################################
# We now have writable configuration files.
################################################

#
# configure devfs in /tmp/devices
#
cd /tmp
mkdir devices
echo "Configuring the /devices directory"
/usr/sbin/drvconfig -r devices -p /tmp/path_to_inst
if [ $? -ne 0 ]; then
	/usr/sbin/drvconfig -r devices
fi

#
# Get the root device
#
eval `/sbin/get_root -r /tmp/devices -e -b Rootfs /`
echo "${Rootfs} - / ufs - no -" >> /etc/vfstab 

#
# Make sure the root FS is clean
#
/etc/fsck -F ufs -o p ${Rootfs} >/dev/null
if [ $? -ne 0 ]; then
	echo "fsck of ${Rootfs} failed, re-preinstall needs to be re-run."
	/usr/sbin/halt
fi
 
#
# Remount the root FS R/W
#
mount -m -o remount / 

#
# If a new path_to_inst was created, put it in place.
#
if [ -f /tmp/path_to_inst ]; then
	cat </tmp/path_to_inst >/etc/.path_to_inst
fi

#
# Move the drvconfig results to /devices
#
cd /tmp/devices
find . -depth -print | cpio -pdm /devices 2> /tmp/cpio.out

#
# Complete the device configuration
#
echo "Configuring the /dev directory"
cd /
/usr/sbin/devlinks
/usr/sbin/disks

#
# Configure all network interfaces
#
/sbin/ifconfig -a plumb > /dev/null 2>&1

#
# Get the complete list of network devices
# so that we can revarp them individually
# since the -ad option seems to stop after
# the first failure (unconnected net device)
# that it encounters
#
for i in `ifconfig -a |grep "^[a-z0-9]*:"`
do
	echo $i |grep "^[a-z0-9]*:" >/dev/null 2>&1
	if [ $? -eq 1 ]; then
		continue
	fi
	net_device_list="${i}${net_device_list}"

done

#
# net_device_list contains a ":" delimited list
# of network devices
# do an auto-revarp on each of them with the
# exception of the loopback device
#
old_ifs=$IFS
IFS=":"
set -- $net_device_list
for i 
do
	#
	# skip the auto-revarp for the loopback device
	#
	if [ "$i" = "lo0" ]; then
		continue
	fi
	/sbin/ifconfig $i auto-revarp -trailers up >/tmp/dev.$$ 2>&1
done
IFS=$old_ifs

. /sbin/stubboot
 
if [ "${BOOT_STRING}" ]; then 
	reboot "${BOOT_STRING}"
else 
	echo
	echo "No network boot server. Unable to install the system."
	echo "See installation instructions."
	echo
	sync;sync
	/sbin/uadmin 2 0  # force it down 
fi
