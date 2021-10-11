#!/bin/sh
#
# @(#)kdmconfig.sh	1.2 - 95/06/01
#
# Copyright (c) 1995 Sun Microsystems, Inc.
# All Rights Reserved
#

OWCONFIG=/usr/openwin/server/etc/OWconfig

if [ -f $OWCONFIG ]
  then
	cp $OWCONFIG /etc/openwin/server/etc
		
  else
	echo "Error: /usr/openwin/server/etc/OWconfig does not exist" > /dev/null  
fi
