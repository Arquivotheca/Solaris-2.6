#!/bin/sh
#
#	"@(#)setup_install_server.sh 1.29 96/10/09"
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
# Used to copy all or part of the install CD onto a system to set it up as
# a long term install server with the install media available on local disk.
#


#
# use -b flag to set up a root-only boot server; slice 0 obtained elsewhere.
# use -t flag to specify the location of the install boot image 
# (required if running this utility from the cdrom)

dial_pid=0
trap "cleanup_and_exit 1" 1 2 3 15

# make sure path is ok
PATH=/usr/bin:/usr/etc:/usr/sbin:${PATH}

#
# cleanup_and_exit
#
# Purpose : Call cleanup and exit with the passed parameter
#
# Arguments : 
#	exit code
#
cleanup_and_exit()
{
	dial_off
	exit $1
}

#
# usage
#
# Purpose : Print the usage message in the event the user
#           has input illegal command line parameters and
#           then exit
#
# Arguments : 
#	none
#
usage()
{
	echo "Usage: $myname [-b] [-t <boot image path>] <destination directory>"
	cleanup_and_exit 1
}

#
# dial_on
#
# Purpose : Turn the spinner on so that the user sees some activity
#			Log the process id so that it can be killed later
#
# Arguments : 
#	none
#
# Side Effects :
#	dial_id - set to the process id of the dial process
#
dial_on()
{

	if [ -x ${TOOLS_DIR}/dial ]; then 
		${TOOLS_DIR}/dial &
		dial_pid=$!
	fi
}


#
# dial_off
#
# Purpose : Turn the spinner off
#
# Arguments : 
#	none
#
# Side Effects :
#	dial_id - set to 0
#
dial_off()
{
	if [ -x ${TOOLS_DIR}/dial -a $dial_pid -ne 0 ]; then 
		${TOOLS_DIR}/dial $dial_pid
	fi
	dial_pid=0

}


#
# check_target
#
# Purpose : Create the directory that will contain the boot image
#           and the product distribution.  If the target is not on
#           the local system then print an error message and exit
#           because it cannot be exported if it is not local.
#
# Arguments : 
#	$1 - pathname to the directory 
#
# Side Effects :
#	diskavail - the amount of space available for the the install
#               server is set
#
check_target()
{
	echo $1 | grep '^/.*' >/dev/null 2>&1
	status=$?
	if [ "$status" != "0" ] ; then
	    echo "ERROR: A full pathname is required for the <destination directory>"
	    usage
	fi
	if [ ! -d $1 ]; then
		mkdir -p -- $1
		if [ $? -ne 0 ]; then
			echo "ERROR: unable to create $1"
			cleanup_and_exit 1
		fi
	fi
	# check to see if target is a local filesystem
	# because we cannot export it otherwise.
	dfout=`df ${Opts_df} $1 | ( read junk; read where junk; echo $where )`
	if [ ! -b ${dfout} ] ; then
		echo "ERROR: ${dfout} is not a local filesystem, cannot export $1 for install clients"
		cleanup_and_exit 1
	fi
	diskavail=`df ${Opts_df} $1 | \
			( read junk; read j1 j2 j3 size j5; echo $size )`
}

#
# set_netmask
#
# Purpose : Get the net mask for the local machine and setup
#			the netmask file for the install server.
#
# Arguments : 
#	none
# 
# Side Effects :
#	NETMASK - the value of the net mask is set
#
set_netmask()
{
	HEX_MASK=`/sbin/ifconfig -a 2>/dev/null | \
		while read ifname flags ; do
			read inet ipaddr netmask mask broadcast broadaddr
			if [ "${ifname}" = "lo0:" ]; then
				continue;
			fi
			echo ${mask}
			break
		done`

	f1=`expr $HEX_MASK : '\(..\)......'`
	f2=`expr $HEX_MASK : '..\(..\)....'`
	f3=`expr $HEX_MASK : '....\(..\)..'`
	f4=`expr $HEX_MASK : '......\(..\)'`

	F1=`echo "$f1=d" | adb`
	F2=`echo "$f2=d" | adb`
	F3=`echo "$f3=d" | adb`
	F4=`echo "$f4=d" | adb`

	tmp="${F1}.${F2}.${F3}.${F4}"
	NET_MASK=`echo $tmp | sed 's/ //g'`

	echo $NET_MASK > ${target}/${VERSION}/Tools/Boot/netmask
}

#
# copy_boot_image
#
# Purpose : Copy the install boot image to the net install
#           boot directory.  This is located in the Tools
#           directory under boot.
#
# Arguments : 
#	none
#
copy_boot_image()
{

	#
	# Only copy the Tools directory if it doesn't already exist
	#
	if [ ! -d ${target}/${VERSION}/Tools ]; then
		# copy the contents of the Tools directory
		echo "Copying ${VERSION} Tools hierarchy..."
		dial_on

		mkdir -p ${target}/${VERSION}/Tools
		old_dir=`pwd`
		cd ${TOOLS_DIR}
		if [ -f /etc/vfstab ]; then
			find . -depth -print | \
				cpio -pdmu ${target}/${VERSION}/Tools >/dev/null 2>&1
		else
			# cpio on 4.1.x has an obscure bug with symlinks on
			# hsfs media.  Use bar instead.
			bar cf - * | (cd ${target}/${VERSION}/Tools; bar xfBp -)
		fi
		copy_ret=$?

		dial_off

		if [ $copy_ret -ne 0 ]; then
			echo "ERROR: copy of ${VERSION} Tools directory failed"
			cleanup_and_exit 1
		fi
		cd ${old_dir}
	fi


	if [ ${BOOT_DIR} != ${TOOLS_DIR}/Boot ]; then
		echo "Copying Install Boot Image hierarchy..."
		dial_on

		mkdir -p ${target}/${VERSION}/Tools/Boot
		old_dir=`pwd`
		cd ${BOOT_DIR}
		if [ -f /etc/vfstab ]; then
			find . -depth -print | \
				cpio -pdmu ${target}/${VERSION}/Tools/Boot >/dev/null 2>&1
		else
			# cpio on 4.1.x has an obscure bug with symlinks on
			# hsfs media.  Use bar instead.
			bar cf - * | (cd ${target}/${VERSION}/Tools/Boot; bar xfBp -)
		fi
		copy_ret=$?

		dial_off

		if [ $? -ne 0 ]; then
			echo "ERROR: copy of install boot image failed"
		cleanup_and_exit 1
		fi
	fi

	#
	# Copy the product directory control files
	# .install_config and .slicemapfile into the
	# boot image
	#
	[ -f ${PROD_DIR}/.slicemapfile ] && \
			cp ${PROD_DIR}/.slicemapfile ${target}
	cp -r ${PROD_DIR}/.install_config ${target}

	cd ${old_dir}
}

#################################################################
# MAIN
#

if [ -f /etc/vfstab ]; then
    Opts_df="-k"
    Opts_du="-ks"
    Arch=`uname -p`
else
    Opts_df=""
    Opts_du="-s"
    Arch=`mach`
fi                  


myname=$0
#
# Check the arguments
#	-b - optional argument that if present
#	     will cause this script to create
#	     only the information necessary to
#	     create a boot server
#	-t - optional argument that allows the 
#	     user to specify the location of the
#	     install boot image
#	     this is required if this utility is 
#	     being run from CD since there is no
#	     way to automatically locate the boot
#	     image
#
#   -p - debug argument that allows the user to
#		 specify the location of the path to the
#		 Tools directory.  This included to allow
#		 the testing of installation tools with
#		 an existing CD
#

boot_server=0
while [ "$1"x != "x" -a $# -gt 1 ]; do
	case $1 in
	-b) boot_server=1;
		shift;;
	-t) boot_image=$2;
		if [ -z "$boot_image" ]; then
			usage;
		fi
		shift 2;;
	-p) tools_path=$2;
		if [ -z "$tools_path" ]; then
			usage;
		fi
		shift 2;;
	-*) # -anything else is an unknown argument
		usage;;
	*)  # not a command-line argument - get out
		break ;;
	esac
done

if [ $# -ne 1 ]; then
    usage
fi

check_target $1
target=$1
shift

#
# Get path to hierarchy
# must maintain the hierarchy structure and RUN THIS COMMAND FROM THE HIERARCHY
#
# (1) find the path of the hierarchy.
# it may be absolute path
# it may be some relative path
# it may be given in PATH

if [ -n "$tools_path" ]; then
	TOOLS_DIR=$tools_path
else
	case ${myname} in
	# absolute path, or found via $PATH (shells turn into abs path)
	/*)
		TOOLS_DIR=`dirname ${myname}`
		myname=`basename ${myname}`
		;;

	# relative path from  "./" or ../, so we do a bit of clean up
	./* | ../*)
		TOOLS_DIR=`pwd`/`dirname ${myname}`
		TOOLS_DIR=`(cd ${TOOLS_DIR} ; pwd )`
		myname=`basename ${myname}`
		;;

	# name found via "." in $PATH
	*)
		TOOLS_DIR=`pwd`
		;; 
	esac
fi

# TOOLS_DIR is now an absolute path to the tools
# directory
## Set the PROD_DIR to the product directory
PROD_DIR=`(cd ${TOOLS_DIR}/../.. ; pwd )`

if [ ! -f $PROD_DIR/.cdtoc ]; then
    echo "ERROR: The product distribution does not contain "
	echo "       a product table of contents"
	cleanup_and_exit 1

fi

if [ -n "${boot_image}" ]; then
	BOOT_DIR=${boot_image}
else
	[ -d ${TOOLS_DIR}/Boot -a -d ${TOOLS_DIR}/Boot/.tmp_proto ]
	BOOT_DIR=${TOOLS_DIR}/Boot
fi

## Check to see whether or not the install boot image is in 
## the current directory structure
if [ -z "${BOOT_DIR}" ]; then
	echo "ERROR:  Install boot image not specified"
	echo
	usage 
fi

#
# Verify that BOOT_DIR is a valid boot image and exists
#
if [ ! -d ${BOOT_DIR} ]; then
	echo "ERROR: Install boot image ${BOOT_DIR} does not exist"
	cleanup_and_exit 1
fi

if [ ! -d ${BOOT_DIR}/.tmp_proto ]; then
	echo "ERROR: ${BOOT_DIR} is not a valid install boot image"
	cleanup_and_exit 1
fi
#
# Validate the target directory
#
echo "Verifying target directory..."

cnt=`find ${target} -print | wc -l`
if [ ${cnt} -gt 1 ]; then
	if [ ${cnt} -eq 2 -a -d ${target}/lost+found ]; then
		:
	else
		cat <<EOM
setup_install_server:
    The target directory ${target} is not empty. Please choose an empty
    directory or remove all files from the specified directory and run
    this program again.
EOM
		cleanup_and_exit 1
	fi
fi

dir_name=`/bin/ls -d ${PROD_DIR}/Solaris_*`
VERSION=`expr ${dir_name} : '.*/\(.*\)' `

if [ $boot_server -eq 1 ]; then
	echo "Calculating space required for the installation boot image"

	dial_on
	boot_space_required=`du ${Opts_du} ${BOOT_DIR} | ( read size name; echo $size )`
	dial_off

	# only copy the install boot image and various reqd. files
	if [ $boot_space_required -gt $diskavail ]; then
		echo "ERROR: Insufficient space to copy Install Boot image"
		echo "       ${boot_space_required} necessary - ${diskavail} available"
		cleanup_and_exit 1
	fi


	# install the Solaris install boot image
	copy_boot_image
else
	echo "Calculating the required disk space for the ${VERSION} product"

	dial_on
	prod_space_required=`du ${Opts_du} ${PROD_DIR} | ( read size name; echo $size )`
	dial_off

	#
	# Calculate the total disk space requirements for the full
	# distribution and boot image
	#
	if [ -d ${TOOLS_DIR}/Boot ]; then
		total_space_required=${prod_space_required}
	else
		echo "Calculating space required for the installation boot image"

		dial_on
		boot_space_required=`du ${Opts_du} ${BOOT_DIR} | ( read size name; echo $size )`
		dial_off

		total_space_required=`expr $prod_space_required + $boot_space_required`
	fi

	# copy the whole CD to disk
	if [ $total_space_required -gt $diskavail ]; then
		echo "ERROR: Insufficient space to copy CD image"
		echo "       ${total_space_required} necessary - ${diskavail} available"
		cleanup_and_exit 1
	fi
	echo "Copying the CD image to disk..."
	cd ${PROD_DIR}
	dial_on
	if [ -f /etc/vfstab ]; then
		find . -depth -print | cpio -pdmu ${target} >/dev/null 2>&1
	else
		# cpio on 4.1.x has an obscure bug with symlinks on
		# hsfs media.  Use bar instead.
		bar cf - . | (cd ${target}; bar xfBp -)
	fi
	copy_ret=$?

	dial_off

	if [ $copy_ret -ne 0 ]; then

		echo "ERROR: Copy of CD image failed"
		cleanup_and_exit 1
	fi

	#
	# if the boot image was not already copied
	# then copy it now
	if [ ! -d ${TOOLS_DIR}/Boot ]; then
		copy_boot_image
	fi
fi

set_netmask

echo "Install Server setup complete"

cleanup_and_exit 0
