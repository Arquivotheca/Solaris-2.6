#!/usr/bin/sh
#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)diff3.sh	1.11	96/10/14 SMI"	/* SVr4.0 1.4	*/

usage="usage: diff3 file1 file2 file3"

e=
case $1 in
-*)
	e=$1
	shift;;
esac
if [ $# != 3 ]; then
	echo ${usage} 1>&2
	exit 1
fi
if [ \( -f $1 -o -c $1 \) -a \( -f $2 -o -c $2 \) -a \( -f $3 -o -c $3 \) ]; then
	:
else
	echo ${usage} 1>&2
	exit 1
fi
f1=$1 f2=$2 f3=$3
if [ -c $f1 ]
then
	/usr/bin/cat $f1 >/tmp/d3c$$
	f1=/tmp/d3c$$
fi
if [ -c $f2 ]
then
	/usr/bin/cat $f2 >/tmp/d3d$$
	f2=/tmp/d3d$$
fi
if [ -c $f3 ]
then
	/usr/bin/cat $f3 >/tmp/d3e$$
	f3=/tmp/d3e$$
fi

trap "/usr/bin/rm -f /tmp/d3[a-e]$$ /tmp/d3[ab]$$.err" 0 1 2 13 15

/usr/bin/diff $f1 $f3 >/tmp/d3a$$ 2>/tmp/d3a$$.err
STATUS=$?
if [ $STATUS -gt 1 ]
then
	/usr/bin/cat /tmp/d3a$$.err
	exit $STATUS
fi

/usr/bin/diff $f2 $f3 >/tmp/d3b$$ 2>/tmp/d3b$$.err
STATUS=$?
if [ $STATUS -gt 1 ]
then
	/usr/bin/cat /tmp/d3b$$.err
	exit $STATUS
fi

/usr/lib/diff3prog $e /tmp/d3[ab]$$ $f1 $f2 $f3
