#!/sbin/sh
#
#        "@(#)rcS.sh 1.130 96/11/11"
#
# Copyright (c) 1992-1996 Sun Microsystems, Inc.  All Rights Reserved.
#

#
# the 2.X install "rcS" file
# run by init(8) to create the environment for doing the installation
# replaces normal rcS
#
#* init starts
# *     mount tmpfs onto /tmp
# *     copy any needed files into /tmp...
# *     device reconfigure
# *     figure out root and /cdrom
# *     mount /cdrom
#

PLATFORM=`/sbin/uname -p`
export PLATFORM

I386="i386"
SPARC="sparc"
PPC="ppc"

ROOT=/tmp/root
LOGS=${ROOT}/var/sadm/system/logs

Network="no"
FJS_BOOT="no"
DoDrvconfig="yes"
SINGLE_USER=0

#
# use shell to snarf the base device name so we can make slice 0's name
# now we (only) do OBP style names
# INPUT: $1 - the rootfs device name
# OUTPUT: spits out on stdout the short device name of the cdrom device
#
basedevof() {
    D=$1    # Device
    T=$2    # Type

    # if we had expr on root, this is what we would simply do
    # U=`expr $D : '\(.*\):.*'
    # echo "${U}:a"

    # but we don't have expr and would like to live without it
    # so we use set to separate at all ":"
    IFS=":"
    set -- ${D}
    # get everything up till last ":" into U
    U=$1
    shift ;
    while [ $# -gt 1 ] ; do
        U=${U}:$1
        shift ;
    done
    # now we got everything except the stuff after the last ":",
    # which we assume was just a partition letter

    # x86 & ppc use p0 device for hsfs.  sparc uses s0 device.
    # These map to :q and :a respectively under /devices/...

    if [ "${PLATFORM}" = "${SPARC}" ]; then
        echo "${U}:a"
    else
        if [ "${T}" = "ufs" ]; then
            echo "${U}:b"
        else
            echo "${U}:q"
        fi
    fi
}

###################################################
###################################################
#           Main
#

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

#
# Check for no reboot flag
#
if [ "${RB_NOBOOTRC}" = "YES" ]; then
	RB_NOBOOTRC="no"
	export RB_NOBOOTRC
	/sbin/sh
fi

if [ -f /tmp/.rcSrun ]; then
	exit 0
fi

DownRevRoss=`echo "downrev_ross_detected/X" | adb /dev/ksyms /dev/kmem | \
	 ( read junk ; read junk ; read junk rev; echo $rev )`

if [ "$DownRevRoss" = "1" ]; then
    echo " "
    echo "The ROSS 605 CPU modules installed in this system are not fully"
    echo "compatible with this version of Solaris.  If this version of Solaris"
    echo "software is installed, this system will run only in uni-processor"
    echo "mode, which will affect performance."
    echo " "
    echo "Please upgrade your CPU module(s) in order to run in multi-processor"
    echo "mode."
    echo " "
    echo "    Do you want to continue with the installation?  If you choose"
    echo "    not to continue, previous system software will be left intact."
    echo "    Continue installation (y/n)?"
    read answer
    if [ "$answer" = "n" ]; then
    echo "Installation stopped. Previous system software has been left intact."
    echo "Please reboot."
	sync;sync
	/sbin/uadmin 2 0  # force it down 
    fi
    echo "WARNING: System performance will be affected. Are you sure (y/n)?"
    read answer
    if [ "$answer" = "n" ]; then
    echo "Installation stopped. Previous system software has been left intact."
    echo "Please reboot."
	sync;sync
	/sbin/uadmin 2 0  # force it down 
    fi
fi

#
# Start the twirling dial
#
if [ -x /sbin/dial ]; then
	dial &
	dial_pid=$!
fi

#    
# Unpack writeable initialized files into /tmp.
# NOTE: send output to /tmp (not /dev/null) to avoid nfs bug on remote
#       ro mounted fs
( cd /.tmp_proto; find . -print -depth | cpio -pdm /tmp 2>/tmp/cpio.out )

mkdir -p /tmp/root/var/sadm/system/logs

MEMSIZE=`/sbin/mem`
echo "Memory free after tmpfs initialization: ${MEMSIZE}" >> ${LOGS}/sysidtool.log
if [ ${MEMSIZE} -lt 4000 ]; then 
    echo "ERROR: ${MEMSIZE}KB is not enough free memory to install Solaris 2"
    /sbin/uadmin 2 0  # force it down
fi

########
# Configured "/" writeable files may not be updated.
#
# create /etc/vfstab (/tmp/root/etc/vfstab)
echo "swap - /tmp tmpfs - no -" >> /etc/vfstab

#
# add the procfs mount line
#
echo "/proc - /proc proc - no -" >> /etc/vfstab

# configure devfs in /tmp/dev and /tmp/devices
# using find | cpio to preserve the directory attributes
echo "Configuring devices..."
find devices dev -depth -print | cpio -pdum /tmp >/dev/null 2>&1

# drvconfig requires you be in "/" (/tmp) or the pathnames
# will be messed up
cd /tmp
/usr/sbin/drvconfig -r devices -p /tmp/root/etc/path_to_inst

cd /
/usr/sbin/devlinks -r /tmp
/usr/sbin/disks -r /tmp

if [ "${PLATFORM}" = "${SPARC}" ]; then
       /usr/sbin/tapes -r /tmp
fi

#
# Loopback mount the newly plumbed devices onto the main
# file systems
#
/sbin/mount -F lofs /tmp/devices /devices
Setmnt="${Setmnt}/tmp/devices /devices\n"

/sbin/mount -F lofs /tmp/dev /dev
Setmnt="${Setmnt}/tmp/dev /dev\n"

#
# PPC/Intel Unbundled Driver support - Sun private interface
# Copy driver scripts and data files onto the system from
# floppy (if they exist). We are looking for the rc.d 
# directory in the top level of the floppy.
#
if [ "${PLATFORM}" = "${I386}" -o "${PLATFORM}" = "${PPC}" ];
then

	/sbin/mount -F pcfs -o ro /dev/diskette /mnt >/dev/null 2>&1
	status=$?

	if [ ${status} -ne 0 ]; then
		/sbin/mount -F ufs -o ro /dev/diskette /mnt >/dev/null 2>&1
		status=$?
	fi

	if [ ${status} -eq 0 ]; then
		if [ -d /mnt/rc.d ]; then
			FILES=`/usr/bin/ls /mnt/rc.d`
			if [ ! -z "${FILES}" ]; then
				/usr/bin/mkdir /tmp/diskette_rc.d
				/usr/bin/cp -p -r /mnt/rc.d/* /tmp/diskette_rc.d
			fi
		fi
		if [ -d /mnt/kernel ]; then
			FILES=`/usr/bin/ls /mnt/kernel`
			if [ ! -z "${FILES}" ]; then
				/usr/bin/mkdir -p /tmp/kernel
				/usr/bin/cp -p -r /mnt/kernel/* /tmp/kernel
			fi
		fi
		/sbin/umount /dev/diskette
	fi

	if [ -r /tmp/diskette_rc.d/rcs1.sh ]; then
		/sbin/sh /tmp/diskette_rc.d/rcs1.sh
	fi
fi

#
# read bootargs for the boot device
# if fails - ignore and assume regular install
# if ok - see if preinstall bootargs set
#	and if possible, copy the /devices from the stub

# read prom for default boot dev
# looking for "FD=..."
set -- ""
set -- `/sbin/getbootargs 2>/dev/null`
if [ $# -gt 0 ] ; then
	while [ $# -gt 0 ] ; do
		case $1 in
		FD=*)
			
			# at end of script, save root dev in /tmp/.preinstall
			# this is an unambiguous indication of stub boot
			FJS_BOOT="yes"
			From=`(IFS="="; set -- $1; echo "$2 $3 $4 $5" )`
			break
			;;
		browser)
                        cat < /dev/null > /tmp/.install_boot
                        cat < /dev/null > /tmp/.smi_boot
                        shift
                        ;;

		install)
			INSTALL_BOOT="yes"
			cat < /dev/null > /tmp/.install_boot
			shift
			;;
		w)
			cat < /dev/null > /tmp/.nowin
			shift
			;;
		*)
			shift
			;;
		esac
	done
fi


#
# figure out root file system information (ie local or remote)
#
eval `/sbin/get_root -t Roottype -b Rootfs /`

case ${Roottype} in
	ufs|hsfs)            # we are on a ufs or hsfs (local machine)
		# get the name of the device for the Solaris distribution
		Installfs=`basedevof ${Rootfs} ""`
		if [ -b ${Installfs} ] ; then
			break
        else
            # "this never happens" :-)
			echo "ERROR: The Solaris Distribution, ${Installfs} does not exist"
			echo "             Exiting to shell."
			/sbin/sh
		fi
		# the slice 0 is type hsfs
		Installtype=hsfs
		;;
	nfs*)            # we are over the network (nfs, nfs2, nfs3)
		Roottype=nfs
		Network="yes"
		# set Installfs=... from config file from server
		Installtype=nfs
		;;
	*)              # fatal error - unknown "/" filesystem
		# we cannot use it as-is - note all the fs specific changes
		/sbin/dial $dial_pid
		while : ; do
			echo "FATAL ERROR: "/" file system type \"${Roottype}\" is unknown"
			echo "             Exiting to shell."
			/sbin/sh
		done
		;;
esac

#
# Configure network interfaces:
#	- software loopback interface
#	- hardware interfaces
# Complete the network configuration
#
/sbin/ifconfig lo0 127.0.0.1 up

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
/sbin/hostconfig -p bootparams 2> /dev/null


#
# if not booting from the net add root entry to 
# /etc/vfstab 
#
if [ "${Network}" != "yes" ]; then
	echo "${Rootfs} - / ${Roottype} - no ro" >> /etc/vfstab
	Setmnt="${Setmnt}${Rootfs} /\n"
else
	#
	# Look for the root boot parameter;
	# bpgetfile returns $1 = server name, $2 = server IP addr, $3 = path
	#
	set -- ""
	set -- `/sbin/bpgetfile`
	SERVER_IPADDR=$2
	if [ $2"x" != "x" ]; then
		Rootfs=$1:$3
		echo "${Rootfs} - / ${Roottype} - no ro" >> /etc/vfstab
		# NOTE: the root filesystem will just *stay* read-only
		Setmnt="${Setmnt}${Rootfs} /\n"
	else
		echo "ERROR: bpgetfile unable to access network information"
		exec /sbin/sh
	fi

	# get the server's netmask
	if [ -x /sbin/get_netmask ]; then
		netmask=`/sbin/get_netmask $SERVER_IPADDR 2>/dev/null`
		if [ -n "$netmask" ]; then
			/sbin/ifconfig -a netmask 0x${netmask}  >/dev/null 2>&1

		fi
	fi

	#
	# Look for the install boot parameter;
	# bpgetfile returns $1 = server name, $2 server IP addr, $3 = path
	#
    set -- ""
    set -- `/sbin/bpgetfile install`
    if [ $2"x" != "x" ]; then
        Installfs=$2:$3
    else
		echo "ERROR: bpgetfile unable to access network install information"
		exec /sbin/sh
    fi
fi

/sbin/mount -F ${Installtype} -m -o ro ${Installfs} /cdrom >/dev/null 2>&1
if [ $? -ne 0 ]; then
    if [ "${Installtype}" = "hsfs" ]; then
        # only used internally for testing
        echo "hsfs mount failed, trying ufs..."
        Installfs=`basedevof ${Rootfs} ufs`
        if [ -b ${Installfs} ] ; then
            /sbin/mount -F ufs -m -o ro ${Installfs} /cdrom
        else
            echo "ERROR: ${Installfs} does not exist, unable to mount /cdrom"
        fi
    else
        # there is a bad entry in the bootparams map
		# display what information is available and 
		# exit to allow the user to correct the situation
		#
        echo "install entry: "`/sbin/bpgetfile install`
        echo "root entry: "`/sbin/bpgetfile`
		echo "ERROR:  Unable to NFS mount ${Installfs}"
		echo "             Exiting to shell."
		exec /sbin/sh
    fi
fi
 
# Verify that the mounted /cdrom is a legal Solaris distribution
# Do this by looking for .cdtoc
# 
if [ ! -f /cdrom/.cdtoc ]; then
    /sbin/umount /cdrom 2>/dev/null
    echo "ERROR: The product distribution does not contain "
	echo "       a product table of contents"
    echo ""
    echo "Type: ${Installtype}, Path: ${Installfs}"
    if [ "${Network}" = "yes" ]; then
        echo "bootparams entries:"
        echo "install entry: "`/sbin/bpgetfile install`
        echo "root entry: "`/sbin/bpgetfile`
    fi
    exec /sbin/sh
fi

# Loopback mount the full version of /usr on the cdrom
# 
if [ "${Roottype}" != "nfs" ]; then
    /sbin/mount -F lofs /cdrom/Solaris_*/Tools/Boot/usr /usr
    mount_return_code=$?
    if [ $mount_return_code -ne 0 ]; then
        echo "Unable to mount /cdrom/Solaris_*/Tools/Boot/usr"
        exec /sbin/sh
    fi
fi

Setmnt="${Setmnt}${Installfs} /cdrom\n"
echo "${Installfs} - /cdrom ${Installtype} - no ro" >> /etc/vfstab


echo "fd - /dev/fd fd - no -" >> /etc/vfstab
/sbin/mount -F fd -m /dev/fd
Setmnt="${Setmnt}fd /dev/fd\n"

# Initialize the mnttab
#
echo "${Setmnt}" | /usr/sbin/setmnt

#####
### Intel/PPC private interface for unbundled device drivers
### Execute diskette shell script
if [ ${PLATFORM} = ${PPC} -o ${PLATFORM} = ${I386} ]; then
	if [ -r /tmp/diskette_rc.d/rcs3.sh ]; then
		/sbin/sh /tmp/diskette_rc.d/rcs3.sh
	fi
fi

##########
# now we have /usr, we can load our 8859 fonts and
# keyboard mappings (architecture specific mechanisms)

if [ "${PLATFORM}" = "${I386}" ]
then
        # Load the default fonts. 
        LOADFONT=/usr/bin/loadfont
        ETC_DEFAULTFONT=/etc/defaultfont
        SYS_DEFAULTFONT=/usr/share/lib/fonts/8859.bdf

        if [ -x ${LOADFONT} ]
        then
                if [ -f ${ETC_DEFAULTFONT} ]
                then
                        ${LOADFONT} -f `cat ${ETC_DEFAULTFONT}` < /dev/console
                else
                        if [ -f ${SYS_DEFAULTFONT} ]
                        then
                                ${LOADFONT} -f ${SYS_DEFAULTFONT} < /dev/console
                        else
                                echo "Failed to set the default fonts."
                                echo "Neither ${ETC_DEFAULTFONT} nor ${SYS_DEFAULTFONT} exist."
                        fi
                fi
        else
                echo "Failed to set the default font."
                echo "${LOADFONT} does not exist or not executable."
        fi

        # Load the default keyboard mappings. 
        PCMAPKEYS=/usr/bin/pcmapkeys
        ETC_DEFAULTKB=/etc/defaultkb
        SYS_DEFAULTKB=/usr/share/lib/keyboards/8859/en_US

        if [ -x ${PCMAPKEYS} ]
        then
                if [ -f ${ETC_DEFAULTKB} ]
                then
                        ${PCMAPKEYS} -f `cat ${ETC_DEFAULTKB}` < /dev/console
                else
                        if [ -f ${SYS_DEFAULTKB} ]
                        then
                                ${PCMAPKEYS} -f ${SYS_DEFAULTKB} < /dev/console
                        else
                                echo "ERROR: Failed to set the default keyboard mappings."
                                echo "       Neither ${ETC_DEFAULTKB} nor ${SYS_DEFAULTKB} exist."
                        fi
                fi
        else
                echo "ERROR: Failed to set the default keyboard mappings."
                echo "       ${PCMAPKEYS} does not exist or is not executable."
        fi
else
	# Load the keymap for the attached keyboard.
	if [ -x /usr/bin/loadkeys ]
	then
		/usr/bin/loadkeys -e
	fi
fi

# save the name of the root device in /tmp/.preinstall
if [ "${FJS_BOOT}" = "yes" -a "${From}" ]; then
	FromDisk=`echo ${From} | ( read d n junk ; echo $d )`
	if [ -b /devices/${FromDisk} ]; then
		lsline=`ls -l /dev/dsk | grep ${FromDisk}`
		if [ "${lsline}" ]; then
			shortname=`( set -- ${lsline} ;
				while [ $# -gt 3 ]; do
					shift;
				done
			if [ "$2" = "->" ]; then echo "$1";
			    else echo "" ; fi )`
			if [ -b /dev/dsk/${shortname} ]; then
				echo /dev/dsk/${shortname} > \
				/tmp/.preinstall
			fi
		fi
	fi
fi

##########
# Intel config cleanup logic
if [ -x /usr/sbin/install.d/atconfig ]; then
	/usr/sbin/install.d/atconfig
fi

#####
### Intel/PPC - Sun private device driver update interface
if [ ${PLATFORM} = ${PPC} -o ${PLATFORM} = ${I386} ]; then
	if [ -r /tmp/diskette_rc.d/rcs9.sh ]; then
		/sbin/sh /tmp/diskette_rc.d/rcs9.sh
	fi
fi

########
# PowerPC Virtual OpenFirmware validation
VOFCHECK=/usr/sbin/install.d/vofcheck
if [ -x ${VOFCHECK} ]; then
	${VOFCHECK}
	if [ $? != 0 ]; then
		if [ ! -z "${dial_pid}" ]; then
			kill $dial_pid >/dev/null 2>&1
		fi
		echo
		echo "ERROR: The Solaris boot diskette must be inserted in the diskette"
		echo "       drive to continue the Solaris installation program."
		echo
		echo "       > Insert the Solaris diskette and press the Enter key."
		echo "         The system will reboot."
		read a < /dev/console
		uadmin 2 0
	fi
fi

###########################################################################
# now we exit rcS, and the rc files bring the system further
# on up (ie networking comes up...)
###########################################################################

if [ ! -z "${dial_pid}" ]; then
	kill $dial_pid >/dev/null 2>&1
fi

cat < /dev/null > /tmp/.rcSrun

exit 0
