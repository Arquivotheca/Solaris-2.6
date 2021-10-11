#!/bin/sh
#
#       @(#)profind.sh 1.27 96/10/10 SMI
#
# Copyright (c) 1992 by Sun Microsystems, Inc. 
#
# Search for a JumpStart directory. If one is found, leave
# it mounted on SI_CONFIG_DIR directory.
#
# Exit values:
#	1	- no profile directory found
#	0	- profile directory with rules.ok file found
#
# NOTE: this script assumes that if there is a floppy in the
#	drive that it is not already mounted anywhere
# NOTE: LOFS file system drivers must be loaded for this
#	script to work
# NOTE: larger profind delay is because of bpgetfile delaying

# Shell Variables
#
SI_CONFIG_DIR=/tmp/install_config
CD_CONFIG_DIR=/cdrom/.install_config
RULES=rules.ok
MESSAGE=`gettext "not found"`
ESTATUS=1

# Profile directory verification routine
# Arguments:	$1 - location for usage message
#		$2 - location for failure message
#
verify_config()
{
    if [ -f ${SI_CONFIG_DIR}/${RULES} ]; then
	MESSAGE=`gettext "using $1"`
	ESTATUS=0
    else
	# There is no rules.ok file. Report the status
	# and unmount the remote directory.
	#
	MESSAGE=`gettext "no ${RULES} found on $2"`
	ESTATUS=1
	umount ${SI_CONFIG_DIR} >/dev/null 2>&1
    fi
}

# Custom JumpStart profile search
# Arguments:	none
#
floppy ()
{
	MOUNT_CONFIG_DIR=/tmp/config.$$

    # Check to see if there is a floppy in the drive (silently)
    #
    /usr/bin/eject -q floppy >/dev/null 2>&1

    if [ $? -eq 0 ]; then
		# Make the mount point directory used in searching
		# for the profile
		#
		if [ ! -d ${MOUNT_CONFIG_DIR} ]; then
			mkdir ${MOUNT_CONFIG_DIR} 2>/dev/null
		fi

		# Try to mount the floppy first as PCFS, then as UFS
		#
		mount -o ro -F pcfs /dev/diskette ${MOUNT_CONFIG_DIR} \
							>/dev/null 2>&1
		status=$?
		if [ ${status} -ne 0 ]; then
			mount -o ro -F ufs  /dev/diskette ${MOUNT_CONFIG_DIR} \
							>/dev/null 2>&1
			status=$?
		fi

		if [ ${status} -eq 0 ]; then
			old_dir=`pwd`
			cd ${MOUNT_CONFIG_DIR} 
			find . -depth -print | \
				cpio -pdmu ${SI_CONFIG_DIR} >/dev/null 2>&1
			status=$?
			cd $old_dir
			umount ${MOUNT_CONFIG_DIR}
		fi

		rmdir ${MOUNT_CONFIG_DIR}

		if [ ${status} -eq 0 ]; then
			verify_config "floppy" "floppy"
		fi
    fi
}

# Custom JumpStart profile search
# Arguments:	none
#
bootparams()
{
    # Check for an 'install_config' bootparams entry 
    #
    set -- `/sbin/bpgetfile -retries 1 install_config`
    if [ $1"X" != "X" ]; then
	mount -o ro -F nfs $2:$3 ${SI_CONFIG_DIR} >/dev/null 2>&1

	if [ $? -eq 0 ]; then
	    verify_config "$2:$3" "$2:$3"
        fi
    fi
}

# Factory JumpStart (default) profile search
# Arguments:	none
#
cdrom()
{
    # Factory JumpStart is only allowed with factory
    # stub images, indicated by the file /tmp/.preinstall
    #
    if [ -f /tmp/.preinstall ]; then
	mount -o ro -F lofs ${CD_CONFIG_DIR} ${SI_CONFIG_DIR} >/dev/null 2>&1

	if [ $? -eq 0 ]; then
	    verify_config "defaults" "CDROM"
        fi
    fi
}

#########################################
#		Main script		#
#########################################
# Make the mount point directory used in searching
# for the profile
#
if [ ! -d ${SI_CONFIG_DIR} ]; then
    mkdir ${SI_CONFIG_DIR} 2>/dev/null
else
    exit 0
fi

gettext "Searching for JumpStart directory..."

# Search for profiles in the order:
#	floppy   (Custom JumpStart)
#	network  (Custom JumpStart)
#	cdrom	 (Factory JumpStart)
floppy
if [ ${ESTATUS} = 1 ]; then
    bootparams
    if [ ${ESTATUS} = 1 ]; then
	cdrom
    fi
fi

# Print the message and exit with the status returned by 
# the functions
#
echo ${MESSAGE}

exit ${ESTATUS}
