#!/bin/sh
#
#	@(#)rm_install_client.sh 1.27 96/10/10 SMI
#
# Copyright (c) 1992 by Sun Microsystems, Inc. 
#
# Description:
# 	cleanup a server that previosly had add_install_client run on it
#
# XXX - swap space going away? if swap -d works ok we can swap off a fake file
#	on the CDROM
#

#
# Constants
#
USAGE="Usage: ${myname} clientname"
VERSION="5.0"

#
# Variables 
#
SERVER=`uname -n`
CLIENT_NAME=""
KARCH=""

# make sure path is ok
PATH=/usr/bin:/usr/etc:/usr/sbin:${PATH}
TMP=/tmp/tmp.$$

# Files Modified by the script

# Functions:

# dirname might not be installed on this (4.X) machine
dirname () {
	expr ${1-.}'/' : '\(/\)[^/]*//*$' \
	  \| ${1-.}'/' : '\(.*[^/]\)//*[^/][^/]*//*$' \
	  \| .
}

#
# Find the sources that the nsswitch.conf(4) file defines for the given database
#
# Takes a single argument, database.
#
#	e.g. - get_sources hosts
#
# Returns a list of sources on stdout
#
get_sources() {
	egrep "^${1}:" /etc/nsswitch.conf 2>/dev/null | \
	sed -e '/^$/d' \
	    -e '/^#/d' \
	    -e 's/.*://' \
	    -e 's/\[.*return\].*$//' \
	    -e 's/\[.*\].*$//' | \
	awk '{ for(i=1; i <= NF; i++) { print $i } }'
}

#
# Determine if the given database is served by the given source
#
# Takes two arguments, database, and service:
#
# 	e.g. - db_in_source hosts files
#
# Returns the status:
#		0 - database in listed source.
#		1 - database not in listed source.
#
db_in_source() {
	echo `get_sources "${1}"` | grep "\<${2}\>" > /dev/null 2>&1
	return $?
}

#
# Lookup the given key in the specified database, returns data retrieved, plus
# the source that returned it.
#
# Retuires two arguments: database and key
#
# 	e.g. - lookup hosts binky
#
# above example will return data for binky and the source it was found in, if
# it is found.
#
# Has two optional arguments: type_of_lookup and source
#
#	e.g. - lookup hosts 129.152.221.35 byaddr files
#
# above example will return data for host 129.152.221.35 using an address lookup
# in files only.
#
lookup() {
	D="${1}"
	K="${2}"
	T="${3}"
	S="${4}"
	ANS=""
	status=0
	if [ ! -z "${S}" ]; then
		srcs="${S}"
	else
		srcs=`get_sources "${D}"`
	fi

	for i in ${srcs}; do
	    SRC=${i}
	    case "${i}" in
		compat)
		    ;;

		dns)
		    if [ ${HAVE_DNS} -ne 0 ]; then
			NS="DNS"
			NS_NAME="domain name service"
			if [ "${D}" = "hosts" ]; then
			    ANS=`nslookup ${K} 2>&1`
			    echo "${ANS}" | grep '^\*\*\*' > /dev/null 2>&1
			    if [ $? -eq 0 ]; then
				status=1
			    else
				status=0
			    fi
			    if [ ${status} -eq 0 ]; then
				break
			    fi
			fi
		    fi
		    ;;

		files)
		    NS="local"
		    NS_NAME="file"
		    case "${D}" in
			bootparams)
			    dbfile=/etc/bootparams
			    key="^\<${K}\>"
			;;
			ethers)
			    dbfile=/etc/ethers
			    key="^[ 	]*[^# 	].*\<${K}\>"
			    [ "${T}" = "byaddr" ] && key="^\<${K}\>"
			;;
			hosts)
			    dbfile=/etc/hosts
			    key="^[ 	]*[^# 	].*\<${K}\>"
			    [ "${T}" = "byaddr" ] && key="^\<${K}\>"
			;;
		    esac
		    if [ -f "${dbfile}" ]; then
			ANS=`grep "${key}" "${dbfile}" 2>/dev/null`
			status=$?
			if [ ${status} -eq 0 ]; then
                            ANS=`echo "${ANS}" | sed -e 's/#.*$//'`
			    break
			fi
		    fi
		    ;;

		nis)
		    if [ ${HAVE_NIS} -ne 0 ]; then
			NS="NIS"
			NS_NAME="map"
			case "${D}" in
			    bootparams)
				mapname=${D}
			    ;;
			    ethers)
				mapname=${D}.byname
				[ "${T}" = "byaddr" ] && mapname=${D}.byaddr
			    ;;
			    hosts)
				mapname=${D}.byname
				[ "${T}" = "byaddr" ] && mapname=${D}.byaddr
				[ "${T}" = "byuser" ] && mapname=${D}.byuser
			    ;;
			esac
			ANS=`ypmatch "${K}" "${mapname}" 2>/dev/null`
			status=$?
			if [ ${status} -eq 0 ]; then
                            ANS=`echo "${ANS}" | sed -e 's/#.*$//'`
			    break
			fi
		    fi
		    ;;

		nisplus)
		    if [ ${HAVE_NISPLUS} -ne 0 ]; then
			NS="NIS+"
			NS_NAME="table"
			case "${D}" in
			    bootparams)
				keyname=name
			    ;;
			    ethers)
				keyname=name
				[ "${T}" = "byaddr" ] && keyname=addr
			    ;;
			    hosts)
				keyname=name
				[ "${T}" = "bycname" ] && keyname=cname
				[ "${T}" = "byaddr" ] && keyname=addr
			    ;;
			esac
			ANS=`nismatch "${keyname}=${K}" "${D}.org_dir" 2>/dev/null`
			status=$?
			if [ ${status} -eq 0 ]; then
                            ANS=`echo "${ANS}" | sed -e 's/#.*$//'`
			    break
			fi
		    fi
		    ;;
		*)
		    NS="Unknown"
		    NS_NAME="Unknown"
		    SRC="Unknown"
		    ANS=""
		    status=1
		    break
		    ;;
	    esac
	done
	echo "status=${status}; SRC=${SRC}; ANS=\"${ANS}\"; NS=\"${NS}\"; NS_NAME=\"${NS_NAME}\""
}

init_lookup() {
	HAVE_DNS=0
	HAVE_NIS=0
	HAVE_NISPLUS=0

	#
	# figure out what options the server has for its database(s)
	#
	if [ -f /etc/resolv.conf ]; then
	    # DNS Available for host lookups.
	    HAVE_DNS=1
	fi

	if [ -f /var/nis/NIS_COLD_START ]; then
	    # NIS+_client
	    HAVE_NISPLUS=1
	fi

	if ( ps -ef | grep "[ /]nisd" > /dev/null ); then
	    # NIS+_server
	    HAVE_NISPLUS=1
	fi

	if ( ps -ef | grep "[ /]ypserv" > /dev/null ); then
	    # NIS_server
	    HAVE_NIS=1
	fi

	if ( ps -ef | grep "[ /]ypbind" > /dev/null ); then
	    # NIS_client
	    HAVE_NIS=1
	fi
}

# set the HOST_NAME variable, return status of match
get_hostname()
{
	HOST_NAME=""
	if [ ${Iam} = "FIVEX" ]; then
		eval `lookup hosts "${1}" byname "${2}"`
	else	# FOURX
		if [ $NS = "NIS+" ]; then
			ANS=`nismatch ${1} hosts.org_dir 2>/dev/null`
			status=$?
		elif [ $NS = "NIS" ]; then
			ANS=`ypmatch ${1} hosts 2>/dev/null`
			status=$?
		else
			ANS=`grep "^[ 	]*[^# 	].*\<${1}\>" /etc/hosts \
				2>/dev/null`
			status=$?
		fi
	fi
	if [ $status -eq 0 ]; then
		if [ $NS = "NIS+" ]; then
			HOST_NAME=`echo ${ANS} | \
				(read cname name addr junk; echo $name)`
		elif [ $NS = "NIS" ]; then
			HOST_NAME=`echo ${ANS} | \
				(read addr name junk; echo $name)`
		elif [ $NS = "DNS" ]; then
			HOST_NAME=`echo ${ANS} | sed -e 's/^Server:.*Name://' |\
				(read name junk; echo $name)`
		else
			HOST_NAME=`echo ${ANS} | \
				(read addr name junk; echo $name)`
		fi
	fi
	return $status
}

# set the HOST_ADDR variable, return status of match
get_hostaddr()
{
	HOST_ADDR=""
	if [ ${Iam} = "FIVEX" ]; then
		eval `lookup hosts "${1}" byname "${2}"`
	else	# FOURX
		if [ $NS = "NIS+" ]; then
			ANS=`nismatch ${1} hosts.org_dir 2>/dev/null`
			status=$?
		elif [ $NS = "NIS" ]; then
			ANS=`ypmatch ${1} hosts 2>/dev/null`
			status=$?
		else
			ANS=`grep "^[ 	]*[^# 	].*\<${1}\>" /etc/hosts \
				2>/dev/null`
			status=$?
		fi
	fi
	if [ $status -eq 0 ]; then
		if [ $NS = "NIS+" ]; then
			HOST_ADDR=`echo ${ANS} | \
				(read cname name addr junk; echo $addr)`
		elif [ $NS = "NIS" ]; then
			HOST_ADDR=`echo ${ANS} | \
				(read addr name junk; echo $addr)`
		elif [ $NS = "DNS" ]; then
			HOST_ADDR=`echo ${ANS} | sed -e 's/^Server:.*Address://' |\
				(read addr junk; echo $addr)`
		else
			HOST_ADDR=`echo ${ANS} | \
				(read addr name junk; echo $addr)`
		fi
	fi
	return $status
}

# convert any host alias to real client name
get_realname()
{
	get_hostname ${CLIENT_NAME}

	if [ ! "${HOST_NAME}" ]; then
		echo "Error: unknown client \"${CLIENT_NAME}\""
		exit 1
	fi

	CLIENT_NAME=${HOST_NAME}
}

#
# main
#
myname=$0
ID=`id`
USER=`expr "${ID}" : 'uid=\([^(]*\).*'`

trap 'echo "${myname}: Aborted"; exit 1' 1 2 3 15

if [ "${USER}" != "0" ]; then
	echo "You must be root to run $0"
	exit 1
fi

if [ -f /etc/vfstab ]; then
        Iam="FIVEX";
        Opts_df="-k"
        Opts_ps="-e"
        Opts_ps_pid="-e"
	Cmd_bpd="/usr/sbin/rpc.bootparamd"
elif [ -f /etc/fstab ]; then
        Iam="FOURX";
        Opts_df=""
        Opts_ps="-ax"
        Opts_ps_pid="-x"
	Cmd_bpd="/usr/etc/rpc.bootparamd"
else
        echo "${myname}: no /etc/fstab or /etc/vfstab, bailing out"
        exit 1      
fi                  

# Parse the command line options. All options must be seperated by a space
#
if [ $# -ne 1 ] ; then
	echo $USAGE; exit 1;
fi
CLIENT_NAME="$1";

# XXX verify CLIENT_NAME ascii ?

#
# no check to see if cdrom is mounted, since it may not be a real CDROM
# Someone might copy all/part of it to some disk someplace.
# We do not care, as long as they maintain
# the hierarchy structure and RUN THIS COMMAND FROM THAT HIERARCHY.
#
# (1) find the path of the hierarchy.
# it may be absolute path
# it may be some relative path
# it may be given in PATH
case ${myname} in
/*)	# absolute path, or found via $PATH (shells turn into abs path)
    CDROM=`dirname ${myname}`
    myname=`expr ${myname} : '.*/\(.*\)' `
    ;;
../*)	# relative path from "../", so we do a bit of clean up
    CDROM=`pwd`/`dirname ${myname}`
    CDROM=`(cd ${CDROM} ; pwd )`
    myname=`expr ${myname} : '.*/\(.*\)' `
    ;;
./*)	# relative path from "./", toss the "./"
    longname=`dirname ${myname}`
    CDROM=`pwd`/` expr ${longname} : '\.\/\(.*\)' `
    # get rid of any strange appendages, like "/" or dot-dots
    CDROM=`(cd ${CDROM} ; pwd )`
    myname=`expr ${myname} : '.*/\(.*\)' `
    ;;
*)	# name found via "." in $PATH
    CDROM=`pwd`
    ;;
esac

#
# figure out what the server is using for its database(s)
#
if [ ${Iam} = "FOURX" ]; then
	if [ -f /var/nis/NIS_COLD_START ]; then
	    # NIS+_client
	    NS="NIS+"
	    NS_NAME="table"
	elif ( ps ${Opts_ps} | grep "[ /]nisd" > /dev/null ); then
	    # NIS+_server
	    NS="NIS+"
	    NS_NAME="table"
	elif ( ps ${Opts_ps} | grep "[ /]ypserv" > /dev/null ); then
	    # NIS_server
	    NS="NIS"
	    NS_NAME="map"
	elif ( ps ${Opts_ps} | grep "[ /]ypbind" > /dev/null ); then
	    # NIS_client
	    NS="NIS"
	    NS_NAME="map"
	else
	    NS="local"
	    NS_NAME="file"
	fi
else	# FIVEX
	init_lookup
fi

get_realname

#
# Check to see if IP and ETHER address and BOOTPARAMS have been determined
#

#
# find the IP_ADDR of this client, if not known, complain
# this is needed to find the /tftpboot/config.<ipaddr> file
#

get_hostaddr ${CLIENT_NAME}
IP_ADDR=${HOST_ADDR}

if [ ! "${IP_ADDR}" ]; then
	echo "${myname}: ERROR: cannot find IP address for $CLIENT_NAME"
	echo "The client name may be misspelled, or this system may have been"
	echo "reconfigured after add_install_client was run."
	echo "Enter $CLIENT_NAME IP address into the $NS $NS_NAME"
	echo "then run ${myname} again."
	exit 1
fi

Bootdir=/tftpboot
Real_Bootdir=$Bootdir
if [ -h $Bootdir ] ; then
    CurrentPwd=`pwd`
    cd $Bootdir
    Real_Bootdir=`pwd`
    cd $CurrentPwd
fi

CLEAN="${Bootdir}/rm.${IP_ADDR}"
# config filename IP address is not the HEXIP, (for client's sake)
if [ ! -f "${CLEAN}" ]; then
	Bootdir=/rplboot
        Real_Bootdir=$Bootdir
        if [ -h $Bootdir ] ; then
            CurrentPwd=`pwd`
            cd $Bootdir
            Real_Bootdir=`pwd`
            cd $CurrentPwd
        fi
	CLEAN="${Bootdir}/rm.${IP_ADDR}"
	if [ ! -f "${CLEAN}" ]; then
		echo "${myname}: the file \"rm.${IP_ADDR}\" does not exist in"
		echo "/tftpboot or /rplboot, cannot do cleanup for \"${CLIENT_NAME}\""
		exit 1
	fi
fi

egrep -s "^${CLIENT_NAME}[ 	]" /etc/bootparams
if [ $? -eq 0 ]; then
	rm -f /etc/bootparams.old
	cp /etc/bootparams /etc/bootparams.old # Save old one
	echo "removing ${CLIENT_NAME} from bootparams"
	( echo "g/^${CLIENT_NAME}[ 	]/d" ; echo w; echo q ) | \
	  ed /etc/bootparams > /dev/null
fi

if [ -f /etc/bootparams -a ! -s /etc/bootparams ]; then
	echo "removing /etc/bootparams, since it is empty"
	rm /etc/bootparams
else
	# rpc.bootparamd - normally started from rc.local or equivalent
	cmd=`ps ${Opts_ps_pid} | \
	    grep '[ /]rpc.boot$' | \
	    awk '{ if ( "$1" != "" ) { print "kill -TERM " $1 ";" }}'`
	if [ $? -eq 0  -a "${cmd}" != "" ]; then
	    # found it, kill it to make sure it gets the new information.
	    eval $cmd
	fi
	# Start bootparamd
	if [ ! -x ${Cmd_bpd} ] ; then
	    echo "WARNING: ${Cmd_bpd} not found - client may not be bootable."
	fi
	OLDWD=`pwd`
	cd /
	${Cmd_bpd}
	cd ${OLDWD}
fi

. ${CLEAN}

rm -f ${CLEAN}

cnt=`ls ${Real_Bootdir} | wc -l`
case ${Bootdir} in
/tftpboot)
	if [ ${cnt} -eq 1 -a -h ${Real_Bootdir}/tftpboot ]; then
		if [ "${Bootdir}" = "${Real_Bootdir}" ] ; then
    			echo "removing /tftpboot"
    			rm ${Bootdir}/tftpboot
    			rmdir ${Bootdir}
		else
    			echo "removing ${Real_Bootdir}/tftpboot"
    			rm ${Real_Bootdir}/tftpboot
		fi

    	# clean up inetd.conf
    	if grep '^tftp[ 	]' /etc/inetd.conf > /dev/null ; then
        	# found it, so it must be enabled, use ed to fix it
        	echo "disabling tftp in /etc/inetd.conf"
        	( echo "/^tftp/" ; echo "s/^/#/" ; echo "w"; echo "w"; echo "q" ) | \
            	ed -s /etc/inetd.conf >/dev/null
        	# send a HUP to tell it to re-read inetd.conf
        	pid=`ps ${Opts_ps_pid} |grep '[ /]inetd' | ( read pid junk ; echo $pid )`
        	kill -HUP $pid
    	fi
	fi
	;;
/rplboot)
	if [ ${cnt} -eq 0 ]; then
		if [ "${Bootdir}" = "${Real_Bootdir}" ] ; then
    			echo "removing $Bootdir"
    			rmdir ${Bootdir}
		fi

		# kill the daemon if no longer anybody's server.
		pid=`ps ${Opts_ps_pid} |grep '[ /]rpld' | ( read pid junk; echo $pid )`
		echo "No longer a rpl server. Terminating rpld."
		kill -TERM $pid
	fi
	;;
esac

exit 0
