#!/bin/sh
#
#	@(#)re-preinstall.sh 1.30 96/02/28
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

# This script makes a stub image on a disk partition, given a cdrom image.
#


#
# Usage : see usage function below

PATH=/usr/bin:/usr/sbin:/sbin
TEXTDOMAIN=SUNW_INSTALL_SCRIPTS
export TEXTDOMAIN PATH

VERSION=1.5

# relative to /usr
ADM_LIB=./lib/libadm.so.1
TMP_MNT=/tmp/mnt.$$
ROOT_MNT=/tmp/root_mnt.$$

myname=`basename $0`


#
# usage
#
# Purpose : Print the usage message in the event the user
#           has input illegal command line parameters and
#           then exit
#
# Arguments : 
#   none
#
usage () {
	gettext "Usage: "
	echo -n ${myname}
	echo `gettext " [-m <boot image mount pnt>]"`
	echo `gettext "                             [-k <platform group>] <target slice>"`
	echo
	echo
 	echo `gettext "<boot image mount pnt> - path to slice 1 of cdrom"`
	echo
      gettext "<platform group> - platform group of bootblk you wish to install"
	echo
	echo `gettext "- Default: platform group of current machine"`
	echo
	echo `gettext "<target slice>	- slice on which stub will be installed"`
	echo

    exit 1
}

#
# get_diskinfo
#
# Purpose : to set a lot of environment variables with disk information
# 
# Arguments :
#	$1 = disk device (eg. c0t2d0)
#
# Side Effects :
#  mbytes_p_disk - the number of megabytes per disk is set
#  bytes_p_sec - then number of bytes per sector is set
#
get_diskinfo() {
	g_part=`basename $1`
	g_disk=/dev/rdsk/"`expr ${g_part} : '\(c[0-9]*.*d[0-9]*\)[.]*'`"s2

	TMP_VTOC=/tmp/vtoc.$$
	prtvtoc  ${g_disk} > ${TMP_VTOC}
	eval `awk 'BEGIN { tot = 1 } 
		$3 == "bytes/sector" 	{ bytes_p_sec = $2 } 
		$3 == "sectors/track"	{ sec_p_track = $2 } 
		$3 == "tracks/cylinder"	{ track_p_cyl = $2 } 
		$3 == "sectors/cylinder" { sec_p_cyl  = $2 } 
		$4 == "cylinders"	{ cyls	      = $2 } 
		# round down, just to be sure we do not over estimate
	END { 
		mbytes_p_disk= ((bytes_p_sec * sec_p_track) / 1048576) * \
					track_p_cyl * cyls;

		# be accurate in the mbytes_p_cyl, but integerize
		# mbytes_p_disk after this
		printf("mbytes_p_cyl=%f;export  mbytes_p_cyl;", \
							mbytes_p_disk/cyls);
		mbytes_p_disk=int(mbytes_p_disk);
	     	printf("mbytes_p_disk=%d;export mbytes_p_disk;", mbytes_p_disk);
		printf("bytes_p_sec=%d;  export bytes_p_sec;", 	bytes_p_sec);
		printf("track_p_cyl=%d;  export track_p_cyl;", 	track_p_cyl);
		printf("sec_p_cyl=%d;    export sec_p_cyl;", 	sec_p_cyl);
		printf("sec_p_track=%d;  export sec_p_track;",	sec_p_track);
		printf("cyls=%d;	 export cyls",  	cyls);

	}' ${TMP_VTOC}`

	rm -f ${TMP_VTOC}
}

#
# slice_size
#
# Purpose : return the size in bytes of the designated disk slice
# 
# Arguments :
#	$1 = disk slice (eg. c0t2d0s0)
#
# Side Effects :
#   none
#
slice_size () {
	p_part=`basename $1`
	p_part_num=`expr ${p_part} : 'c[0-9]*.*d[0-9]*s\([0-9]*\)'`
	p_disk=/dev/rdsk/"`expr ${p_part} : '\(c[0-9]*.*d[0-9]*\)s[0-9]*'`"s2

	get_diskinfo ${p_part}
	prtvtoc -h ${p_disk} | \
	awk '$1 == '${p_part_num}' {print int(($5 * bytes_p_sec)/1048576)}' \
			bytes_p_sec=${bytes_p_sec}

}

#
# check_pbi_size
#
# Purpose : Dynamically determine that the PBI will fit into the
#           specified slice.  The base size of the root file system,
#           and /usr/platform are found by using du and rounding up
#           to the nearest megabyte boundary.  The /usr file system
#           is sized by adding up all of the sizes of the files
#           specified in .root.copy and converting to megabytes.
#           In addition, the size of /dev and /devices are 
#           calculated.  If the disk is moved to a different
#           system it is possible that the boot of the PBI image
#           will fail because of insufficient space if there are
#           more devices on the destination system
# 
# Arguments :
#	$1 = the location of the .root.copy file
#
# Side Effects :
#   none
#
check_pbi_size() {

	# 
	# put all of the root directories that will be copied into the
	# PBI into root_dirs
	#
	cd ${boot_image_mnt_pt}
	root_dirs=`ls -a | awk '($1 != "cdrom" && $1 != "usr" && $1 != "." && $1 != "..")  {print $1}'`

	#
	# Calculate the size of the root directories, the dev, and
	# devices directories that are necessary in the PBI.  
	#
	root_mb=`du -sk ${root_dirs} | awk '{total += $1} \
			END {print int((total + 1023) / 1024)}'`

	#
	# Calculate the size of the /usr/platform directory that will
	# be copied 
	#
	cd ${boot_image_mnt_pt}/usr/platform

	#
	# The following calculation overestimates the necessary 
	# size of /usr/platform by about 300K on the sparc.  This
	# is because it includes 'include' and 'sbin' for each of
	# the platform groups.  This should probably be fixed but
	# is not real high priority since when I do the actual
	# copy they are excluded.
	#
	usrplatform_mb=`du -sk . | awk '{total += $1} \
			END {print int((total + 1023) / 1024)}'`

	#
	# Calculate the size of the /usr files that will be copied
	# into the PBI
	#
	total_usr_size=0
	usr_files=$1
	grep -v '^#|^[ 	]*$' $usr_files |
	while read type file dest; do
		if [ "${type}" = "f" -a -f ${boot_image_mnt_pt}/${file} ]; then
			file_size=`ls -l ${boot_image_mnt_pt}/${file} | \
				awk '{print $5}'`
			total_usr_size=`expr $total_usr_size + $file_size`
		fi
	done
	usr_mb=`expr  \( \( $total_usr_size / 1024 \) + 1023 \) / 1024`

	pbi_mb=`expr $root_mb + $usrplatform_mb + $usr_mb`

	# check the calculated size against the size of the pbi slice
	#
	if [ `slice_size ${stub_part}` -lt ${pbi_mb} ]; then
		# its too small
		echo ${stub_part}`gettext ": too small"`
		echo ${pbi_mb} `gettext " Megabytes required"`
		exit 1
	fi

}

#
# copy_root_to_pbi
#
# Purpose : Copy the install boot root directories to the PBI
# 
# Arguments :
#	none
#
# Side Effects :
#  none
#
copy_root_to_pbi() {
	# Copy everything from the boot image except for 
	# cdrom and usr
	#
	cd ${boot_image_mnt_pt}
	root_dirs=`ls -a | awk '($1 != "cdrom" && $1 != "usr" && $1 != "." && $1 != "..")  {print $1}'`
	find $root_dirs -depth -print | cpio -pdm ${TMP_MNT}
	if [ $? != 0 ]; then
		echo `gettext "Creation of the PBI root failed"`
		exit 1
	fi
}

#
# copy_usrplatform_to_pbi
#
# Purpose : Copy the relevent files from /usr/platform to the PBI
# 
# Arguments :
#	none
#
# Side Effects :
#  none
#
copy_usrplatform_to_pbi() {
	echo "$myname:  Copying /usr/platform to PBI"

	#
	# if /usr/platform does not exist then create it
	#
    [ ! -d ${TMP_MNT}/usr/platform ] && mkdir -p ${TMP_MNT}/usr/platform

    cd ${boot_image_mnt_pt}/usr/platform

    find . '(' -type d \
		   '(' -name sbin -o -name include -o -name adb ')' \
		   ')' -prune -o -print | cpio -pdum ${TMP_MNT}/usr/platform

    return $?

}

#
# copy_usrfiles_to_pbi
#
# Purpose : Copy the files specified in .root.copy from the /usr
#           file system to the PBI. 
# 
# Arguments :
#	$1 - the path to .root.copy
#
# Side Effects :
#  none
#
copy_usrfiles_to_pbi() {
    usr_files=$1

    # Fill in shared libs in root's "/usr/lib", et. al.
    echo "$myname:  Copying selected /usr files to PBI"


	err=0
    grep -v '^#|^[ 	]*$' $usr_files | 
    while read type file dest; do
        case ${type} in
        d)  # if dir already exists, skip it
            if [ ! -d ${TMP_MNT}/$file ]; then
                mkdir -p ${TMP_MNT}/$file || err=1
            fi
            ;;
        l)  # do links
            ln ${TMP_MNT}/$file ${TMP_MNT}/$dest || err=1
            ;;
        f)  # else just copy file to dest
            cp -p ${boot_image_mnt_pt}/$file ${TMP_MNT}/$dest || err=1
            ;;
        *)  echo "WARNING:  Illegal type $type found in $ROOT_COPY"
            err=1
            ;;
        esac
    done

	if [ ${err} -eq 1 ]; then
		echo "ERROR:  Unable to create usr files specified in .root.copy"
		exit 1
	fi
}


#
# stamp_unix
#
# Purpose : Change the release string in each kernel from 
#           "SunOS Release X.Y" to "Jumpstart 1.4"
# 
# Arguments :
#	none
#
# Side Effects :
#  none
#
stamp_unix () {
	ascii=`echo "JumpStart ${VERSION} (%s)"`

	# Make sure that the motd is in sync with the kernel.
	echo "${ascii}" > ${TMP_MNT}/etc/motd

	Cmds=/tmp/adb.cmds.$$		# temp file for adb commands

	for kernel in ${TMP_MNT}/platform/*/kernel/unix
	do

		offset=`/bin/strings -o ${kernel} | grep "SunOS Release" | \
					(read junk junk1; echo ${junk})`

		if [ -z "$offset" ]; then
			continue
		fi
	
		rm -f ${Cmds}
	
		# awk used to create adb commands for patching the string above
	
		cat << ZZZ > ${Cmds}.awk
{ ptr = 1;
len = length( \$0 );
if (len > 48) len=48
while ( ptr < ( len + 3) ) {
	bytes = substr( \$0, ptr, 4 );
    while (length(bytes) < 4)
        bytes=bytes " "
	printf("0t%d?W '%s'\n", ptr - 1 + ${offset}, bytes );
	ptr = ptr + 4;
}
printf(".+4?w 0a00\n");
printf("\$q\n");
}
ZZZ

		echo '? m 0 100000 0' > ${Cmds}
		echo "${ascii}" | awk -f ${Cmds}.awk - >> ${Cmds}

		rm ${Cmds}.awk 
		cat ${Cmds} | adb -w ${kernel} >/dev/null
	done
}

#=============================== MAIN ======================================

case `uname -r` in
5.*)    ;;  #good
*)      echo `gettext "Must run on a SunOS 5.x system."`
        exit 1;;
esac


#
# Parse the command line options.
#
while getopts m:k:v: option
do
	case $option in
		m)	boot_image_mnt_pt=$OPTARG ;;
		k)  PGRP=$OPTARG ;;
		v)  VERSION=$OPTARG ;;
		\?) usage;;
	esac
done
shift `expr $OPTIND \- 1`

if [ $# -ne 1 ]; then
	usage
fi

stub_part=`basename $1`

#
# Get path to hierarchy
# must maintain the hierarchy structure and 
# RUN THIS COMMAND FROM THE HIERARCHY

#
# (1) find the path of the hierarchy.
# it may be absolute path
# it may be some relative path
# it may be given in PATH

myname=$0
case ${myname} in
/*)    
	# absolute path, or found via $PATH (shells turn into abs path)
    INSTALLD_DIR=`dirname ${myname}`
    myname=`basename ${myname}`
    ;; 

./* | ../*)  
	# relative path from  "./" or "../", so we do a bit of clean up
    INSTALLD_DIR=`pwd`/`dirname ${myname}`
    INSTALLD_DIR=`(cd ${INSTALLD_DIR} ; pwd )`
    myname=`basename ${myname}`
    ;; 
       
*)  
	# name found via "." in $PATH
    INSTALLD_DIR=`pwd`
    ;; 
esac   
 
#
# If the boot image is specified then use it otherwise check to
# make sure that the image that is being run is an install boot
# image
#
if [ -z "${boot_image_mnt_pt}" ]; then
	#
	# Mounted off of either a netinstall image or the CDROM
	# Get the location of the root mount point and mount it
	# for use in creating the PBI
	#
	root_dir=`/bin/df -b / | (read junk; read junk1 junk2; echo $junk1)`
	mkdir ${ROOT_MNT}
	/sbin/mount $root_dir ${ROOT_MNT}
	if [ $? -ne 0 ]; then
		echo $root_dir `gettext "mount failed"`
		exit 1
	fi
	boot_image_mnt_pt=${ROOT_MNT}
fi

# can use our platform name since it is either a real directory 
# or always points to our platform group
if [ -z "$PGRP" ]; then
    PGRP=`uname -i`
fi

# check out the boot image mount point
if [ ! -d ${boot_image_mnt_pt} ]; then
	echo ${boot_image_mnt_pt}`gettext ": does not exist"`
	exit 1
fi

# validate the boot image mount point
if [ ! -d ${boot_image_mnt_pt}/.tmp_proto ]; then
	echo ${boot_image_mnt_pt}`gettext ": is not a valid Solaris install boot image"`
	exit 1
fi

# check out the stub partition
if [ ! -b /dev/dsk/${stub_part} ]; then
	echo ${stub_part}`gettext ": does not exist"`
	exit 1
fi

if mount | grep /dev/dsk/${stub_part} >/dev/null 2>&1; then
	# its mounted, we want to newfs it, so complain
	echo /dev/dsk/${stub_part}`gettext ": mounted"`
	exit 1
fi


if [ ! -f $INSTALLD_DIR/.root.copy ]; then
	echo `gettext "ERROR: Root copy file not found"`
	exit 1
fi

gettext "Beginning Preinstall Boot Image (PBI) installation on "; echo "${stub_part}"

# 
# Check to make sure that the PBI will fit onto the designated slice
#
echo `gettext "- Verifying that there is sufficient disk space on "; echo "${stub_part}"`
check_pbi_size $INSTALLD_DIR/.root.copy


#
# Create a file system for the target stub root
#
echo `gettext "- Creating PBI root file system..."`
if [ "${MAKESTUB_FS_SIZE}" ]; then
	# make stub fs a specific # of sectors
	echo y | (newfs -s ${MAKESTUB_FS_SIZE} /dev/rdsk/${stub_part} \
		>/tmp/newfs.out.$$ 2>&1)
else
	echo y | (newfs /dev/rdsk/${stub_part} >/tmp/newfs.out.$$ 2>&1)
fi
if [ $? -ne 0 ]; then 
	echo "newfs(1m) failed"
	cat /tmp/newfs.out.$$
	exit 1
fi
echo
rm -f /tmp/newfs.out.$$

mkdir ${TMP_MNT}

#
# Mount the target root file system and copy over the stub
# root image
#
gettext "- Mounting PBI root file system..."
/sbin/mount /dev/dsk/${stub_part} ${TMP_MNT}
if [ $? -ne 0 ]; then 
	echo `gettext "mount failed"`
	exit 1
else
	echo
fi

echo `gettext "- Copying PBI root image..."`

copy_root_to_pbi
copy_usrfiles_to_pbi $INSTALLD_DIR/.root.copy
copy_usrplatform_to_pbi

#
# Install the boot blocks on the newly installed root image
#
echo `gettext "- Installing PBI boot block ($PGRP)..."`
BOOTBLK=${boot_image_mnt_pt}/usr/platform/${PGRP}/lib/fs/ufs/bootblk
/usr/sbin/installboot $BOOTBLK /dev/rdsk/${stub_part}
if [ $? != 0 ]; then
    echo `gettext "installboot(1m) failed"`
    exit 1
else
    echo
fi

#
# replace kernel string with one for Jumpstart, also fix the 
# motd file.
#
stamp_unix

gettext "- Unmounting PBI root image..."
/sbin/umount ${TMP_MNT}
if [ $? != 0 ]; then
	echo `gettext "unmount failed"`
else
	echo
fi
rmdir ${TMP_MNT}

echo `gettext "Preinstall Boot Image installation complete"`
exit 0
