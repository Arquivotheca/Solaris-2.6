#!/bin/sh
#
#	@(#)create_cd_root.sh 1.5 96/09/09
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

VERSION=1.0

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
	echo "Usage: "
	echo "\t" ${myname} " input_location output_location"
	echo
	echo  "input_location - location of bootable image to create cd root from"
	echo
	echo  "output_location - location to put the mini-root files"
	echo
	echo "Example: create_cd_root  \"
	echo "    /export/solaris/297/sparc/Solaris_2.6/Tools/Boot  \"
	echo "    /export/solaris/mini-root"
	echo

    exit 1
}

#
# verify_space_for_mini_root
#
# Purpose : Verifies the amount of space required to put the mini root
#           is available on the output location media.
#
verify_space_for_mini_root () {
    total=`df -b $output_location | (read junk; read junk total; echo $total)`
    total=`expr \( $total / 1024 \)`

    if [ ${total} -lt ${miniroot_mb} ]; then
	#its too small
	echo "Install Boot Image requires " $miniroot_mb ", available mb is " $total "\n"
	exit 1
    fi
}

#
# calc_total_usr_size
#
# Purpose : Calculates the size of the files to be placed in the 
#           mini-/usr area.  Input to the read comes from the call
#           using grep -v for # and *.
#
# Return Value : returns size of /usr in megabytes
#
calc_total_usr_size() {
while read type file dest; do
    if [ "${type}" = "f" -a -f ${input_location}/${file} ]; then
	file_size=`ls -l ${input_location}/${file} | \
	awk '{print $5}'`
	total_usr_size=`expr $total_usr_size + $file_size`
    fi
done
echo $total_usr_size
}

#
# check_miniroot_size
#
# Purpose : Dynamically determine that the files will fit into the
#           specified slice.  The base size of the root file system,
#           and /usr/platform are found by using du and rounding up
#           to the nearest megabyte boundary.  The /usr file system
#           is sized by adding up all of the sizes of the files
#           specified in .mini_root.copy and converting to megabytes.
#           In addition, the size of /dev and /devices are 
#           calculated.  If the disk is moved to a different
#           system it is possible that the boot of the mini root image
#           will fail because of insufficient space if there are
#           more devices on the destination system
# 
# Arguments :
#	$1 = the location of the files list file
#
# Side Effects :
#   none
#
check_miniroot_size() {

        # put all of the root directories that will be copied into the
	# mini root into root_dirs
	#
	cd ${input_location}
	root_dirs=`ls -a | awk '($1 != "usr" && $1 != "." && $1 != "..")  {print $1}'`

	#
	# Calculate the size of the root directories, the dev, and
	# devices directories that are necessary in the mini root.  
	#
	root_mb=`du -sk ${root_dirs} | awk '{total += $1} \
			END {print int((total + 1023) / 1024)}'`

	#
	# Calculate the size of the /usr/platform directory that will
	# be copied 
	#
	cd ${input_location}/usr/platform

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
	# into the mini root
	#
	total_usr_size=0
	usr_files=$1
	total_usr_size=`grep -v '^#|^[ 	]*$' $usr_files | calc_total_usr_size`

	#change total_usr_size from bytes to MB
	usr_mb=`expr  \( \( $total_usr_size / 1024 \) + 1023 \) / 1024`

	miniroot_mb=`expr $root_mb + $usrplatform_mb + $usr_mb`

	#
	# Verify there is enough space in output location and it is empty
	#
	verify_space_for_mini_root
}

#
# copy_root_to_miniroot
#
# Purpose : Copy the install boot root directories to the mini root
# 
# Arguments :
#	none
#
# Side Effects :
#  none
#
copy_root_to_miniroot() {
	# Copy everything from the boot image except for 
	# cdrom and usr
	#
	cd ${input_location}
	root_dirs=`ls -a | awk '($1 != "usr" && $1 != "." && $1 != "..")  {print $1}'`
	find $root_dirs -depth -print | cpio -pdm ${output_location}
	if [ $? != 0 ]; then
		echo  "Creation of the mini root - root failed"
		exit 1
	fi
}

#
# copy_usrplatform_to_miniroot
#
# Purpose : Copy the relevent files from /usr/platform to the miniroot
# 
# Arguments :
#	none
#
# Side Effects :
#  none
#
copy_usrplatform_to_miniroot() {
	echo "$myname:  Copying /usr/platform to mini root"

	#
	# if /usr/platform does not exist then create it
	#
    [ ! -d ${output_location}/usr/platform ] && mkdir -p ${output_location}/usr/platform

    cd ${input_location}/usr/platform

    find . '(' -type d \
		   '(' -name sbin -o -name include -o -name adb ')' \
		   ')' -prune -o -print | cpio -pdum ${output_location}/usr/platform

    return $?

}

#
# copy_the_usrfiles
#
# Purpose : Copy the files specified in $FILE_LIST to the miniroot
# 
# Arguments :
#	grep -v for # and * on the $FILE_LIST file
#
# Side Effects :
#  none
#
copy_the_usrfiles() {
    err=0

    # read the output from the grep on the calling line
    while read type file dest; do
	case ${type} in
	    d)  # if dir already exists, skip it
		if [ ! -d ${output_location}/$file ]; then
		    mkdir -p ${output_location}/$file || err=1
		fi
		;;
	    l)  # do links
		ln ${output_location}/$file ${output_location}/$dest || err=1
		;;
	    f)  # else just copy file to dest
		cp -p ${input_location}/$file ${output_location}/$dest || err=1
		;;
	    *)  echo "WARNING:  Illegal type $type found in "$FILE_LIST
		err=1
		;;
	esac
    done

    # stop on errors in the $FILE_LIST file
    if [ ${err} -eq 1 ]; then
	echo "ERROR:  Unable to create usr files specified in "$FILE_LIST
	exit 1
    fi
}


#
# copy_usrfiles_to_miniroot
#
# Purpose : Copy the files specified in .mini_root.copy from the /usr
#           file system to the miniroot. 
# 
# Arguments :
#	$1 - the path to .mini_root.copy
#
# Side Effects :
#  none
#
copy_usrfiles_to_miniroot() {
    usr_files=$1

    # Fill in shared libs in root's "/usr/lib", et. al.
    echo "$myname:  Copying selected /usr files to mini root"


    err=`grep -v '^#|^[ 	]*$' $usr_files | copy_the_usrfiles`
}



#=============================== MAIN ======================================

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
    FILE_LIST=$INSTALLD_DIR'/.mini_root.copy'
    myname=`basename ${myname}`
    ;; 

./* | ../*)  
	# relative path from  "./" or "../", so we do a bit of clean up
    INSTALLD_DIR=`pwd`/`dirname ${myname}`
    INSTALLD_DIR=`(cd ${INSTALLD_DIR} ; pwd )`
    FILE_LIST=$INSTALLD_DIR'/.mini_root.copy'
    myname=`basename ${myname}`
    ;; 
       
*)  
	# name found via "." in $PATH
    INSTALLD_DIR=`pwd`
    FILE_LIST=$INSTALLD_DIR'/.mini_root.copy'
    ;; 
esac   

#
# Parse the command line options.
#

input_location=$1
output_location=$2

echo "Beginning Create CD Root installation"

#
#     INPUT_LOCATION
# If an input location is specified then use it
# otherwise display the usage text and exit
#
if [ -z "${input_location}" ]; then
    echo "location of the installable boot image must be specified!"
    usage
    exit 1
fi

if [ ! -d "${input_location}" ]; then
    echo ${input_location} ": does not exist"
    exit 1
fi

#
#     OUTPUT_LOCATION
# If an output location is specified then use it
# otherwise display the usage text and exit
#
if [ -z "${output_location}" ]; then
    echo "ufs root install boot image output location must be specified!"
    usage
    exit 1
fi

if [ ! -d "${input_location}" ]; then
    echo ${input_location} ": does not exist"
    exit 1
fi

#
# Validate the output location
#
echo "Verifying output location..."
if [ ! -d "${output_location}" ]; then
    mkdir -p ${output_location}
else
    cnt=`find ${output_location} -print | wc -l`
    if [ ${cnt} -gt 1 ]; then
	if [ ${cnt} -eq 2 -a -d ${output_location}/lost+found ]; then
	    :
	else
	    echo "create_cd_root :"
	    echo "\tThe output location ${output_location} is not empty."
	    echo "\tPlease choose an empty directory or remove all files from"
	    echo "\tthe specified directory and run this program again."
            exit 1
	fi
    fi
fi

if [ ! -f $FILE_LIST ]; then
	echo 'ERROR: Files copy file '$FILE_LIST' not found'
	exit 1
fi

# 
# Check to make sure 
#
echo  "- Verifying that there is sufficient disk space for CD Boot Image"
check_miniroot_size $FILE_LIST

echo "$myname: Copying CD boot image root..."

copy_root_to_miniroot
copy_usrfiles_to_miniroot $FILE_LIST
copy_usrplatform_to_miniroot

echo "Create CD Root installation complete"
exit 0
