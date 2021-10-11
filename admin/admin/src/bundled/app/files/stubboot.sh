#!/sbin/sh
#
#       @(#)stubboot.sh 1.16 95/12/01 SMI
#
# Copyright (c) 1992,1994 by Sun Microsystems, Inc. 
#
# This script finds Solaris installation media on either a local CD or from
# an install server on the net.  If media is found, the string BOOT_STRING
# is set with the device to boot from as well as the preinstall root
# device argument

PATH=${PATH}:/usr/sbin/install.d

found()
{
	# find the "from device" if possible
	# this works on both a normally booted system & on a preinstall system
	# with a CDROM style root (note: df output will differ)
	if [ "${Rootfs}" ]; then
		device=${Rootfs}
		devname=`expr ${Rootfs} : '/tmp/devices\(.*\)'`
	else
		dfentry=`/usr/sbin/df /`
		device=`expr "${dfentry}" : '.*(\(.*\) ).*'`
		line=`ls -l $device`
		longname=`set -- ${line}; while test $# -ne 1 ; do shift ; \
			done; echo ${1}`
		devname=`expr $longname : '.*devices\(.*\)'`
	fi
	if [ ! -b /devices"${devname}" ] ; then
		# could not find it, punt
		fdargs=""
	else
		mm=`ls -lL ${device} | \
			(read mod nl own grp maj min junk;echo "${maj}${min}")`
		major=`expr ${mm} : '\(.*\),.*'`
		minor=`expr ${mm} : '.*,\(.*\)'`
		fdargs="${devname}=${major}=${minor}=X"
	fi

	echo ${msg}
	BOOT_STRING="${bootfrom} - FD=${fdargs}"
}

#
# get the correct slice based on:
#	if slice map exists on CD, use it
#		if no slice map entry found, eject CD and halt
#	else use canonical slice assignments
#		if no canonical slice entry found, eject CD and halt
# INPUT: $1	name of install CDROM slice 0 block device
# OUPUT: $slice	set to slice number, range 1 thru 7.
# ERROR: will halt if error
set_slice()
{

	# try to get the slice map file off slice 0 of CDROM
	CDDEV=$1
	MNTPT=/tmp/.slicemapmount
	SMF=${MNTPT}/.slicemapfile

	mkdir -p ${MNTPT}
	/sbin/mount -F hsfs -m -o ro ${CDDEV} ${MNTPT} > /dev/null 2>&1
	if [ $? -ne 0 ]; then
		# try ufs, we may be on a test image
		/sbin/mount -F ufs -m -o ro ${CDDEV} ${MNTPT} > /dev/null 2>&1
		if [ $? -ne 0 ]; then
			# XXX punt -or- print "bad CD ?", halt
			# "never happens"
			echo "bad CD ?"
			eject ${CDDEV}
			uadmin 2 0
			##########
		fi
	fi

	if [ -f "${SMF}" ]; then
		# have a map file, follow its word
		# it has entries of the form:
		# x <dummy> <platform>
		# p <slice_number> <platform>
		# m <slice_number> <karch>

		# get "m" key: uname -m
		key_m=`uname -m`
		# get "p" or "x" key: uname -i (or equivalent), done earlier
		key_p=${platform}

		slice=`while read type slice key junk; do 
			case $type in
			'#'*|'')
				continue
				;;
			x)
				# try match on "p" key to exclude a platform
				if [ "${key}" = "${key_p}" ]; then
					echo ""		# NO SLICE
					break
				fi
				;;
			m)
				# try match on "m" key
				if [ "${key}" = "${key_m}" ]; then
					echo "${slice}"
					break
				fi
				;;
			p)
				# try match on "p" key
				if [ "${key}" = "${key_p}" ]; then
					echo "${slice}"
					break
				fi
				;;
			esac
			done < "${SMF}"`

		if [ ! "${slice}" ] ; then
			# XXX - HALT
			echo ""
			echo "invalid install CD-ROM for: \"${key_p}\" (${key_m})"
			echo "halting..."
			echo ""
			eject -f ${CDDEV}
			uadmin 2 0
			##########
		fi
	else
		# must be an oldish CD, use default
		case `uname -m` in
		sun4)
			slice=1
			;;
		sun4c) 
			slice=2
			;;
		sun4m) 
			slice=3
			;;
		sun4d) 
			slice=4
			;;
		sun4e) 
			slice=5
			;;
		*)
			# unknown karch, HALT
			echo ""
			echo "invalid install CD-ROM for karch \""`uname -m`\"
			echo "halting..."
			echo ""
			eject -f ${CDDEV}
			uadmin 2 0
			##########
			;;
		esac
	fi
}

# an OEM may need their own stubboot logic, in particular since they
# are not allowed to edit this file from edition to edition (since it
# is a generic SPARC file.  We leave then an escape route - they can provide
# /platform/`uname -i`/OEMstubboot in their SUNWcar which should duplicate
# the following in the "else ... fi".
# In particular, they must use the canonical "found()" and "set_slice()"
# functions, and fall off the end, as this is sourced by /sbin/rcS(.stub)
#

platform=`/sbin/uname -i`

if [ -x /platform/${platform}/sbin/stubboot ]; then
	. /platform/${platform}/sbin/stubboot
else	# "fi" is at END
bootfrom=`findcd`
if [ "${bootfrom}" ]; then
	# (new) findcd just returns the Solaris name of block device slice 0
	# need to get the appropriate slice, then xlate to prom name
	# get the appropriate slice
	set_slice ${bootfrom}

	# convert to slice name on Solaris
	# (have name of the form: /dev/dsk/c0t#d#s#)
	x=`expr ${bootfrom} : "\(.*/c[0-9][0-9]*t[0-9][0-9]*d[0-9][0-9]*\)s.*"`
	bootfrom="${x}s${slice}"

	msg="Installing from local CD-ROM"
	# convert Solaris name to PROM reboot name
# BEGIN PLATFORM DEPENDENT
	# XXX this could be...
	# romvec_vers=`echo 'obp_romvec_version/X' | adb /dev/ksyms /dev/kmem | \
	#                ( read junk; read junk val; echo $val )`
	# case $val in
	# ffffffff) # old ...
	# 0|2|3) # new ...
	#

	# OBP proms
	# Some Sun4c's still have old proms and need the SunMon style name
	# check obp_romvec_version in the kernel/unix.
	prom_ver=`echo 'obp_romvec_version/X' | adb /dev/ksyms /dev/kmem | \
		(read junk; read junk; read junk vers; echo $vers)`
	if [ $prom_ver -gt 0 ]; then
		# use /devices name, it is the canonical OBP name
		# ASSUMES /dev are symlinks to /devices
		# ASSUMES /devices match OBP dev tree
		line=`ls -l ${bootfrom}`
		longname=`set -- ${line}; while test $# -ne 1 ; do shift ; \
			done; echo ${1}`
		bootfrom=`expr $longname : '.*devices\(.*\)'`
		found	# set BOOT_STRING
	else
		# old SunMon proms, want "sd(#,#,#)"
		# NOTE: assume only single digits for controller, target, slice
		controller=`expr ${bootfrom} : "/dev/dsk/c\([0-9]\).*"`
		target=`expr ${bootfrom} : "/dev/dsk/c[0-9]t\([0-9]\).*"`
		# slice we already have
		bootfrom="sd(${controller},${target},${slice})"
		found	# set BOOT_STRING
	fi

# END PLATFORM DEPENDENT

else
	echo "no local boot CD, checking net..."

	# Configure the software loopback driver
	#/sbin/ifconfig lo0 127.0.0.1 up
	/sbin/ifconfig -ad auto-revarp -trailers up >/dev/null 2>&1
	/sbin/hostconfig -p bootparams 2> /dev/null
 
	set -- `/sbin/bpgetfile`
	if [ ! "$1" ]; then
		BOOT_STRING=""
	else
		msg="Found install server $1:$3"
# BEGIN PLATFORM DEPENDENT
		bootfrom="net"
# END PLATFORM DEPENDENT
		found
	fi
fi
fi	# the END
