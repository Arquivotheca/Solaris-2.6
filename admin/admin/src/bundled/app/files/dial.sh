#!/bin/sh
#
#	@(#)dial.sh 1.2 96/01/24
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
# This script does nothing more than spin a dial once every
# second
#

#
# If a parameter is passed in it will be the pid of
# another dial process that should be killed.
#
if [ $# -ge 1 ] 
then
	kill $1
	exit
fi

state=0

while [ 1 ]
do
	case $state in
	  0)
	  echo "|\b\c"
	  state=1
	  ;;
	  1)
	  echo "/\b\c"
	  state=2
	  ;;
	  2)
	  echo "-\b\c"
	  state=3
	  ;;
	  3)
	  echo "\\" "\b\b\c"
	  state=0 
	  ;;
	esac

	sleep 1
done
