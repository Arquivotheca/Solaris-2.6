#! /bin/sh
#
# ident	"@(#)vofcheck.sh	1.1	95/07/26 SMI"
#
# Copyright (c) 1995, by Sun Microsystems, Inc.
# All rights reserved.
# 
# NOTICE: THIS SCRIPT IS NOT PUBLIC.
#
# This script returns 1 if VOF is needed but is not available.
# This could happen if the user boot without the VOF floppy in
# the drive and is trying to do a system installation.
#
# This script is only available on the mini-root during install
# time.
#

prtconf -vp |
grep "model:" |
grep "SunSoft's Virtual Open Firmware" > /dev/null 2>&1
vof_is_running=$?

if [ $vof_is_running = 0 ]
then
	if [ ! -f /tmp/root/platform/`uname -i`/openfirmware.x41 ]
	then
		exit 1
	fi
fi

exit 0
