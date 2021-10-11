#!/bin/sh
#
#       @(#)server_upgrade.sh 1.5 96/10/17 SMI
#
# Copyright (c) 1995 by Sun Microsystems, Inc.
#

print_usage() {
echo `gettext 'usage: server_upgrade -d <installation_medium> [ -p <profile> ]'`
echo
}

get_root_device() {
origdir=`pwd`
cd /
rootdev=`df . | sed 's,^.*(\([^ )]*\).*,\1,'`
rootdev=`basename $rootdev`
cd $origdir
}

medium=
profile=

while getopts d:p: c
do
	case $c in
	d)	medium=$OPTARG;;
	p)	profile=$OPTARG;;
	\?)	print_usage
		exit 1;;
	esac
done
shift `expr $OPTIND - 1`

if [ "$medium" = "" ] ; then
	print_usage
	exit 1
fi

if [ ! -d $medium ] ; then
	eval echo `gettext 'Error:  Could not access ${medium}.'`
	exit 2
fi

if [ ! -f ${medium}/.cdtoc ] ; then
	eval echo `gettext 'Error:  ${medium} is not in expected format.'`
	exit 3
fi

if [ "$profile" = "" ] ; then
	touch /tmp/prof.$$
	profile=/tmp/prof.$$
	echo "install_type upgrade" > $profile
	get_root_device
	echo "root_device $rootdev" >> $profile
fi

get_root_device

/usr/sbin/install.d/pfinstall -n -L / -c $medium $profile
exitcode=$?

rm -f /tmp/prof.$$

exit $exitcode
