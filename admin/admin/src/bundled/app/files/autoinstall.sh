#! /bin/sh
#
#       @(#)autoinstall.sh 1.6 96/01/31 SMI
#
# Copyright (c) 1992-1995 Sun Microsystems, Inc.  All Rights Reserved. Sun
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

# Script executed during stub JumpStart or /AUTOINSTALL JumpStart.
# The purpose is to reboot the system with the appropriate "FD=*"
# boot string, which is recognized by /sbin/rcS to mean "JumpStart".
# In either case, custom profiles are selected first, and if none
# are available, then the default profiles in are used.
# 
#

AUTO_NAME="/AUTOINSTALL"
PATH=/usr/sbin/install.d:${PATH}

cleanup()
{
	exit 1
}

if [ -f ${AUTO_NAME} ]; then
	rm -f ${AUTO_NAME}
	# check for the existence of install media
	. stubboot
	if [ ! "${BOOT_STRING}" ]; then
		echo "No network boot server."
		# just reboot the existing OS
	else
		reboot "${BOOT_STRING}"
	fi
fi
