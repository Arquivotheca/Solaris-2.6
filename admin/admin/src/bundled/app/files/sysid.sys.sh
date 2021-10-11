#! /bin/sh
#
#ident	"@(#)sysid.sys.sh	1.10	96/09/30 SMI"
#
# Copyright (c) 1991 by Sun Microsystems, Inc.
#
# /etc/init.d/sysid.sys
#
# Script to invoke sysidsys, sysidroot and sysidpm, which complete
# configuration of various system attributes.
#

if [ -f /etc/.UNCONFIGURED ]
then
	if [ -x /usr/sbin/sysidsys ]
	then
		/usr/sbin/sysidsys
	fi
	if [ -x /usr/sbin/sysidroot ]
	then
		/usr/sbin/sysidroot
	fi
	if [ -x /usr/sbin/sysidpm ]
	then
		/usr/sbin/sysidpm
	fi
elif [ -f /etc/.PM_RECONFIGURE ]
then
        if [ -x /usr/sbin/sysidpm ]
	then
		/usr/sbin/sysidpm
	fi
fi
