#! /bin/sh
#
#ident	"@(#)sysid.net.sh	1.10	96/06/14 SMI"
#
# Copyright (c) 1991 by Sun Microsystems, Inc.
#
# /etc/init.d/sysid.net
#
# Script to invoke sysidnet, which completes configuration of basic
# network parameters.
#

if [ -f /etc/.UNCONFIGURED -a -x /usr/sbin/sysidnet ]
then
	/usr/sbin/sysidnet -l
	if [ -x /usr/sbin/sysidconfig ]
	then
		/usr/sbin/sysidconfig
	fi
	#
	# Configure all network interfaces
	#
	/sbin/ifconfig -a plumb > /dev/null 2>&1

	#
	# Get the complete list of network devices
	# so that we can revarp them individually
	# since the -ad option seems to stop after
	# the first failure (unconnected net device)
	# that it encounters
	#
	for i in `ifconfig -a |grep "^[a-z0-9]*:"`
	do
		echo $i |grep "^[a-z0-9]*:" >/dev/null 2>&1
		if [ $? -eq 1 ]; then
			continue
		fi
		net_device_list="${i}${net_device_list}"

	done

	#
	# net_device_list contains a ":" delimited list
	# of network devices
	# do an auto-revarp on each of them with the
	# exception of the loopback device
	#
	old_ifs=$IFS
	IFS=":"
	set -- $net_device_list
	for i 
	do
		#
		# skip the auto-revarp for the loopback device
		#
		if [ "$i" = "lo0" ]; then
			continue
		fi
		/sbin/ifconfig $i auto-revarp netmask + broadcast + -trailers up >/dev/null 2>&1
	done
	IFS=$old_ifs
	/sbin/hostconfig -p bootparams
	/usr/sbin/sysidnet
fi
