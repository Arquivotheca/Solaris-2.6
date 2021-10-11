#!/usr/bin/sh
#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#! /usr/bin/sh

#       Copyright(c) 1988, Sun Microsystems, Inc.
#       All Rights Reserved

#pragma ident	"@(#)indxbib.sh	1.4	93/01/11 SMI" 
#
#	indxbib sh script
#
if test $1
	then /usr/lib/refer/mkey $* | /usr/lib/refer/inv _$1
	mv _$1.ia $1.ia
	mv _$1.ib $1.ib
	mv _$1.ic $1.ic
else
	echo 'Usage:  indxbib database [ ... ]
	first argument is the basename for indexes
	indexes will be called database.{ia,ib,ic}'
fi
