#!/bin/sh
#
#	"@(#)add_install_client.sh 1.107 96/10/10"
#
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


#
# Description:
# 	Setup a server for SVR4 client to run install software
#
# The CDROM has everything needed on the first partition, so there is
# no need to create any partitions/hierachies on the server (besides /tftpboot).
#
# Files (maybe) changed on server:
# /tftpboot/ - directory created/populated with inetboot and links.
# /etc/ethers - if client Ethernet address isn't known AND not running NIS
# /etc/exports || /etc/dfs/dfstab - adds/update export entry for install filesys
# /etc/inetd.conf - to turn on tftpboot daemon
# /etc/bootparams - adds entry for this client
# /etc/hosts - if client IP address not already known to server

# make sure path is ok
PATH=/usr/bin:/usr/sbin:/sbin:${PATH}

#
# Variables 
#
CLIENT_NAME=""
PGRP=""
BOOT_METHOD=""
Board="smc" # Only one supported for now. also the default

# Functions:


#
# cleanup_and_exit
#
# Purpose : Get rid of temporary files and mount points and exit
#
# Arguments : 
#   exit code
#
cleanup_and_exit () {

	if [ -n "$Pkg_admin" -a -f "$Pkg_admin" ]; then
		rm $Pkg_admin
	fi

    if [ -n "$Pkg_mnt" -a -d "$Pkg_mnt" ]; then
        umount $Pkg_mnt 2> /dev/null
        rmdir $Pkg_mnt 2> /dev/null
    fi

    exit $1
}


#
# usage
#
# Purpose : Print the usage message in the event the user
#           has input illegal command line parameters and
#           then exit
#
# Arguments : 
#   none
#
usage () {
	echo "Usage: $0 [-i ipaddr] [-e ethernetid] [-s server:path]"
	echo "\t\t[-c server:path] [-p server:path]"
	echo "\t\t[-n [name_server]:name_service[(netmask)]]"
	echo "\t\t[-t install boot image path] client_name platform_group"
	cleanup_and_exit 1

}



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
# Requires two arguments: database and key
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
		    if [ "${HAVE_DNS}" -ne 0 ]; then
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
			    if [ "${status}" -eq 0 ]; then
				ANS=`echo "${ANS}" | awk '{
					if ($1 == "Name:") n=$2
					if (n != "") {
					    a=$2
					    if (substr(a, length(a), 1) == ",")
						a=substr(a, 1, length(a)-1)
				        }
				    }
				    END{print a, n}'`
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
			if [ "${status}" -eq 0 ]; then
			    break
			fi
		    fi
		    ;;

		nis)
		    if [ "${HAVE_NIS}" -ne 0 ]; then
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
			if [ "${status}" -eq 0 ]; then
			    break
			fi
		    fi
		    ;;

		nisplus)
		    if [ "${HAVE_NISPLUS}" -ne 0 ]; then
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
			if [ "${status}" -eq 0 ]; then
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

#
# Solaris 2.X system - determine which name service is
# being used
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
	if [ "${Iam}" = "FIVEX" ]; then
		eval `lookup hosts "${1}" byname "${2}"`
	else	# FOURX
		if [ "$NS" = "NIS+" ]; then
			ANS=`nismatch name=${1} hosts.org_dir 2>/dev/null`
			status=$?
		elif [ "$NS" = "NIS" ]; then
			ANS=`ypmatch ${1} hosts 2>/dev/null`
			status=$?
		else
			ANS=`grep "^[ 	]*[^# 	].*\<${1}\>" /etc/hosts \
				2>/dev/null`
			status=$?
		fi
	fi
	if [ "$status" -eq 0 ]; then
		if [ "$NS" = "NIS+" ]; then
			HOST_NAME=`echo ${ANS} | \
				(read cname name addr junk; echo $name)`
		elif [ "$NS" = "NIS" ]; then
			HOST_NAME=`echo ${ANS} | \
				(read addr name junk; echo $name)`
		elif [ "$NS" = "DNS" ]; then
			HOST_NAME=`echo ${ANS} | (read junk name; echo $name)`
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
	if [ "${Iam}" = "FIVEX" ]; then
		eval `lookup hosts "${1}" byname "${2}"`
	else	# FOURX
		if [ "$NS" = "NIS+" ]; then
			ANS=`nismatch name=${1} hosts.org_dir 2>/dev/null`
			status=$?
		elif [ "$NS" = "NIS" ]; then
			ANS=`ypmatch ${1} hosts 2>/dev/null`
			status=$?
		else
			ANS=`grep "^[ 	]*[^# 	].*\<${1}\>" /etc/hosts \
				2>/dev/null`
			status=$?
		fi
	fi
	if [ "$status" -eq 0 ]; then
		if [ "$NS" = "NIS+" ]; then
			HOST_ADDR=`echo ${ANS} | \
				(read cname name addr junk; echo $addr)`
		elif [ "$NS" = "NIS" ]; then
			HOST_ADDR=`echo ${ANS} | \
				(read addr name junk; echo $addr)`
		elif [ "$NS" = "DNS" ]; then
			HOST_ADDR=`echo ${ANS} | (read addr junk; echo $addr)`
		else
			HOST_ADDR=`echo ${ANS} | \
				(read addr name junk; echo $addr)`
		fi
	fi
	return $status
}

# set the HOST_ADDR variable, return status of match
get_etheraddr()
{
	ETHERNET_ADDR=""
	if [ "${Iam}" = "FIVEX" ]; then
		eval `lookup ethers "${1}" byname "${2}"`
	else	# FOURX
		if [ "$NS" = "NIS+" ]; then
			ANS=`nismatch name=${1} ethers.org_dir 2>/dev/null`
			status=$?
		elif [ "$NS" = "NIS" ]; then
			ANS=`ypmatch ${1} ethers 2>/dev/null`
			status=$?
		else
			# create a null /etc/ethers if it doesn't exist
			if [ ! -f /etc/ethers ] ; then
				cat /dev/null > /etc/ethers
			fi
			ANS=`grep "^[ 	]*[^# 	].*\<${1}\>" /etc/ethers \
				2>/dev/null`
			status=$?
		fi
	fi
	if [ "$status" -eq 0 ]; then
		if [ "$NS" = "NIS+" ]; then
			ETHERNET_ADDR=`echo ${ANS} | \
				(read addr name junk; echo $addr)`
		elif [ "$NS" = "NIS" ]; then
			ETHERNET_ADDR=`echo ${ANS} | \
				(read addr name junk; echo $addr)`
		else
			ETHERNET_ADDR=`echo ${ANS} | \
				(read addr name junk; echo $addr)`
		fi
	fi
	return $status
}

# return status of match
in_bootparams()
{
	if [ "${Iam}" = "FIVEX" ]; then
		eval `lookup bootparams ${1} byname ${2}`
	else	# FOURX
		if [ "$NS" = "NIS+" ]; then
			nismatch ${1} bootparams.org_dir 2>/dev/null
			status=$?
		elif [ "$NS" = "NIS" ]; then
			ypmatch ${1} bootparams 2>/dev/null
			status=$?
		else
			ANS=`grep "^[ 	]*[^# 	].*\<${1}\>" /etc/bootparams \
				2>/dev/null`
			status=$?
		fi
	fi
	return $status
}

# return files, nis or nisplus for the src of a table/map
table_src()
{
	if [ "$NS" = "NIS+" ]; then
		line=`egrep "^${1}:" /etc/nsswitch.conf 2>/dev/null`
		if [ $? -ne 0 ]; then
			NS_SRC="files"
		else
			NS_SRC=`echo $line | awk '{print $2}'`
		fi
	elif [ "$NS" = "NIS" ]; then
		# ypwhich always exits 0, even if no map exists
		NS_SRC=`ypwhich -m ${1} 2>&1 | ( read first junk; echo $first )`
		if [ "${NS_SRC}" = "ypwhich:" ] ; then
			# no map/server exists
			NS_SRC="files"
		else
			NS_SRC="nis"
		fi
	else
		NS_SRC="files"
	fi
}

#
# check_network() :
#    This function splits out the parts of each the netmask($1), the
# server ipaddress($2), and the client ipaddress($3).  It ANDs each
# part of the server ipaddress with the associated part of the
# netmask.  It also ANDs each part of the client ipaddress with the
# associated part of the netmask.  The final setp is to determine if
# the results of the two ANDs are equal.  If they are equal the client
# is on the same network interface as the server ipaddress.
#
# INPUTS:
#    netmask - netmask of the server network interface, $1
#    server ipaddress - network address of the server network interface, $2
#    client ipaddress - network address of the client machine $3
#
# OUTPUTS:
#    NETWORK_MATCHED -- 0, not matched
#                       1, match found
#
check_network()
{
        # split out the parts of the netmask
	nm1=`expr $1 : '\(..\)......'`
	nm2=`expr $1 : '..\(..\)....'`
	nm3=`expr $1 : '....\(..\)..'`
	nm4=`expr $1 : '......\(..\)'`

	# split out the parts of the server ip address
	SNO1=`expr $2 : '\([0-9]*\)\..*'`
	SNO2=`expr $2 : '[0-9]*\.\([0-9]*\)\..*'`
	SNO3=`expr $2 : '[0-9]*\.[0-9]*\.\([0-9]*\)\..*'`
	SNO4=`expr $2 : '[0-9]*\.[0-9]*\.[0-9]*\.\([0-9]*\)'`

	# AND the server ipaddress and the netmask
	SNF1=`echo "0t${SNO1}&${nm1}=d" | adb`
	SNF2=`echo "0t${SNO2}&${nm2}=d" | adb`
	SNF3=`echo "0t${SNO3}&${nm3}=d" | adb`
	SNF4=`echo "0t${SNO4}&${nm4}=d" | adb`

        tmp="${SNF1}.${SNF2}.${SNF3}.${SNF4}"
	MASKED_SRVR=`echo $tmp | sed 's/ //g'`

	# split out the parts of the client ip address
	CNO1=`expr $3 : '\([0-9]*\)\..*'`
	CNO2=`expr $3 : '[0-9]*\.\([0-9]*\)\..*'`
	CNO3=`expr $3 : '[0-9]*\.[0-9]*\.\([0-9]*\)\..*'`
	CNO4=`expr $3 : '[0-9]*\.[0-9]*\.[0-9]*\.\([0-9]*\)'`

	# AND the client ipaddress and the netmask
	CNF1=`echo "0t${CNO1}&${nm1}=d" | adb`
	CNF2=`echo "0t${CNO2}&${nm2}=d" | adb`
	CNF3=`echo "0t${CNO3}&${nm3}=d" | adb`
	CNF4=`echo "0t${CNO4}&${nm4}=d" | adb`

        tmp="${CNF1}.${CNF2}.${CNF3}.${CNF4}"
	MASKED_CLNT=`echo $tmp | sed 's/ //g'`

	NETWORK_MATCHED=0
	if [ ${MASKED_SRVR} = ${MASKED_CLNT} ]; then
	    NETWORK_MATCHED=1
        fi
}


# convert any host alias to real client name
get_realname()
{
	get_hostname ${CLIENT_NAME}

	if [ ! "${HOST_NAME}" ]; then
		echo "Error: unknown client \"${CLIENT_NAME}\""
		cleanup_and_exit 1
	fi

	CLIENT_NAME=${HOST_NAME}
}

setup_tftp()
{
	echo "rm /tftpboot/$1" >> ${CLEAN}

	if [ -h /tftpboot/$1 ]; then
    	    # save it, and stash the cleanup command
    	    mv /tftpboot/$1 /tftpboot/$1-
	    echo "mv /tftpboot/$1- /tftpboot/$1" >> ${CLEAN}
	fi

	ln -s ${Inetboot} /tftpboot/$1
}

#
# Warning message - initial line
#
warn()
{
	echo "WARNING:	$1"
}

#
# Warning message continuation routine
#
warnc()
{
	echo "		$1"
}

warn_export()
{
    echo "${EXP_FS} already has an entry in ${EXPORTS}."
    echo "However, ${EXP_FS} must be ${1} read-only with root access."
    echo "Use ro and either anon=0 or root=${CLIENT_NAME} for ${EXP_FS}."
    echo "The ${EXPORTS} file must be fixed and ${EXP_FS} ${1}"
    echo "before ${CLIENT_NAME} can boot."
}

#
# Check to see if directory is exported.  If not
# export it
#
export_fs()
{
LAST_EXPORT=$EXP_FS
unset EXP_FS
FS_TO_EXPORT=$1
#
# Set EXP_FS to be the filesystem/directory to use for export
#
# It may already be exported, if so fine. If not, derive valid EXP_FS
# from the current FS_TO_EXPORT directory.  If FS_TO_EXPORT in an exported fs, use that
# fs. If FS_TO_EXPORT in an exported subdir, use that subdir.  If FS_TO_EXPORT in an
# fs with exported subdirs, if FS_TO_EXPORT in one of those subdirs, use it, otherwise
# use FS_TO_EXPORT
#

# first check if the name is already in export file
awk '$0 !~ /^#/ {if ('${EXP_FLD}' == "'${FS_TO_EXPORT}'") {find=1; exit}}
            END {if (find == 1) exit 0; else exit 1}' ${EXPORTS}
if [ $? -eq 0 ]; then
    # FS_TO_EXPORT already there
    EXP_FS=$FS_TO_EXPORT
else
    # FS_TO_EXPORT is not already in export file
    # determine fs that FS_TO_EXPORT is in
    # assume that by sorting fs's in reverse order, the first match should be
    # the filesystem it belongs too.
    CD_FS=$FS_TO_EXPORT
    for i in `df ${Opts_df} | grep "^/dev" | awk '{ print $6 }' | sort -r`
    do
        if [ "$i" != "$FS_TO_EXPORT" ]; then
             dir=`echo $i | awk '{if (length($1) < length(FS_TO_EXPORT) )
                                      print substr(FS_TO_EXPORT,1,length($1))
                            }' FS_TO_EXPORT=$FS_TO_EXPORT`
         else
             dir=$FS_TO_EXPORT
         fi
         if [ "$i" = "$dir" ]; then
             CD_FS=$dir
             break
         fi
    done
    # CD_FS is now set to be the filesystem the CD boot image is in

    # if we're in a dir that is already exported, use that
    for i in `awk '$0 !~ /^#/ {print '${EXP_FLD}'}' ${EXPORTS} | \
              egrep "^${CD_FS}"`
    do
	# The expr command used to be 'expr ${FS_TO_EXPORT} : "${i}"', but that fails
	# when i is /, which is the case if root is exported.
        if ( expr ${FS_TO_EXPORT} : "\(${i}\).*" > /dev/null 2>&1 ); then
           EXP_FS=$i
           break
        fi
    done

    # just use the CD dir
    if [ -z "${EXP_FS}" ]; then
        EXP_FS=$FS_TO_EXPORT
    fi
fi
#echo "Using $EXP_FS in ${EXPORTS} file"

##########################################################################
# now we have gathered all the needed information, configure the server
# part. 1 - things that are shared by all clients
#
# check /etc/exports or /etc/dfs/dfstab for existing export of the CDROM
#
# grep for CDROM path, if not there, put it there and export it
if [ "${Iam}" = "FOURX" ]; then

    exportfs_needed=0
    if grep "^${EXP_FS}[ 	]" ${EXPORTS} > /dev/null 2>&1 ; then 
        # EXP_FS already there, see if exported with ro & root access
        awk '{
            if ($1 != "'${EXP_FS}'") {
                next
            }
            cnt=split($2, args, ",")
            for (i=1; i<=cnt; i++) {
                if (substr(args[i], 1, 1) == "-")
                    str=substr(args[i], 2, length(args[i])-1)
                else
                    str=args[i]

                if (str == "ro")
                    flag++

                if (str == "anon=0")
                    flag++

                if (substr(str, 1, 5) == "root=") {
                    tmp=substr(str, 6, length(str)-5)
                    rcnt=split(tmp, root, ":")
                        for (j=1; j<=rcnt; j++)
                            if (root[j] == "'${CLIENT_NAME}'") {
                                flag++
                            }
                }

		if (substr(str, 1, 3) == "rw=") {
			tmp=substr(str, 4, length(str)-3)
			rcnt=split(tmp, rw, ":")
			rw_found = 0
			for (j=1; j<=rcnt; j++)
				if (rw[j] == "'${CLIENT_NAME}'") 
					rw_found++
			if (rw_found == 0)
				flag++
		}
            }
        }
        END {
            if (flag >= 2)
                exit 0
            else
                exit 1
        }' ${EXPORTS}

		#
		# Only warn the user once that a file system is not
		# exported correctly.  If it has been done the first
		# time through then don't make the call again.
		#
        if [ $? -ne 0 -a "$EXP_FS" != "$LAST_EXPORT" ]; then
            warn_export "exported"
        fi

    # just tack on a line at the end.
    else
        if [ ! -f ${EXPORTS}.orig -a -f "${EXPORTS}" ]; then
            echo "saving original ${EXPORTS} in ${EXPORTS}.orig"
            cp ${EXPORTS} ${EXPORTS}.orig
        fi

        echo "Adding \"${EXP_FS}	-ro,anon=0\" to ${EXPORTS}"
        echo "${EXP_FS}	-ro,anon=0" >> ${EXPORTS}
	exportfs_needed=1
    fi

    if [ "$exportfs_needed" -eq 1 ]; then
	# re-export 
	exportfs -a
    fi

else   # FIVEX

    # Check to see if EXP_FS exists within the EXPORTS file
    # if so then verify that if is exported with ro & root access
    awk 'BEGIN { flag = -1 }
        {
        if ($NF != "'${EXP_FS}'") {
            next
        }

        flag = 0
        for (f=1; f <= NF; f++) {
            if (substr($f, 1, 2) == "-o") {
                if (length($f) == 2)
                    str=$(f+1)
                else
                    str=substr($f, 3, length($f)-2)

                cnt=split(str, args, ",")
                for (i=1; i<=cnt; i++) {

                    if (args[i] == "ro")
                        flag++

                    if (args[i] == "anon=0")
                        flag++

                    if (substr(args[i], 1, 5) == "root=") {
                        tmp=substr(args[i], 6, length(args[i])-5)
                        rcnt=split(tmp, root, ":")
                            for (j=1; j<=rcnt; j++)
                                if (root[j] == "'${CLIENT_NAME}'") {
                                    flag++
                                }
                    }
                }
            }
        }

    }
    END {
        if (flag == -1)
            exit 2

        if (flag >= 2)
            exit 0
        else
            exit 1
    }' ${EXPORTS}

    ret_code=$?

	#
	# Only warn the user once that a file system is not
	# exported correctly.  If it has been done the first
	# time through then don't make the call again.
	#
	if [ $ret_code -eq 1 -a "$EXP_FS" != "$LAST_EXPORT" ]; then
		warn_export "shared"
	fi

    if [ $ret_code -eq 2 ]; then
        if [ ! -f ${EXPORTS}.orig -a -f "${EXPORTS}" ]; then
            echo "saving original ${EXPORTS} in ${EXPORTS}.orig"
            cp ${EXPORTS} ${EXPORTS}.orig
        fi

        echo "Adding \"share -F nfs -o ro,anon=0 ${EXP_FS}\" to ${EXPORTS}"
        echo "share -F nfs -o ro,anon=0 ${EXP_FS}" >> ${EXPORTS}
        shareall
    fi
fi


}
#
# Install a package from the CD or netinstall path
# install_pkg pkg_dir pkg
#
install_pkg()
{
    if [ "X${PRODUCT_SERVER}" = "X" ]; then
        Pkg_mnt=""
        Pkg_dir="${PRODUCT_PATH}/${VERSION}/${1}"
    else
        #
        # Package is on a remote server
        #
        Pkg_mnt="/tmp/pkg_install.$$"
        mkdir $Pkg_mnt
        mount ${PRODUCT_SERVER}:${PRODUCT_PATH} $Pkg_mnt
        if [ $? -ne 0 ]; then
            return $?
        fi
        Pkg_dir="${Pkg_mnt}/${VERSION}/${1}"
    fi

	#
	# Attempt to install the package unconditionally
	#
	Pkg_admin=/tmp/admin.$$
	cat <<- EOF > $Pkg_admin
		mail=
		runlevel=nocheck
		conflict=nocheck
		setuid=nocheck
		action=nocheck
	EOF

	pkgadd -a $Pkg_admin -d $Pkg_dir $2 > /dev/null 2>&1
	Pkg_status=$?
	rm $Pkg_admin
 
    if [ "X${Pkg_mnt}" != "X" ]; then
        umount $Pkg_mnt
        rmdir $Pkg_mnt
    fi    
 
	return $Pkg_status
}

abort()
{

	echo "${myname}: Aborted"
	cleanup_and_exit 1
}

#
# MAIN - Program
#
myname=$0
ID=`id`
USER=`expr "${ID}" : 'uid=\([^(]*\).*'`

trap abort 1 2 3 15

# Verify user ID before proceeding - must be root
#
if [ "${USER}" != "0" ]; then
	echo "You must be root to run $0"
	cleanup_and_exit 1
fi

# Set up parameters depending on whether this is a
# Solaris 1.X (FOURX) or a Solaris 2.X (FIVEX) server
#
if [ -f /etc/vfstab ]; then
	Iam="FIVEX";
	Opts_df="-k"
	Opts_du="-sk"
	Opts_ps="-ef"
	Opts_ps_pid="-e"
	Cmd_rarpd="/usr/sbin/in.rarpd"
	Cmd_bpd="/usr/sbin/rpc.bootparamd"
	Cmd_dhcpd="/opt/SUNWpcnet/sbin/in.dhcpd"
	Startup_dhcpd="/etc/rc3.d/S34dhcp"
	Pkg_dhcpd="SUNWpcdhc"
	PATH=/sbin:/usr/sbin:/usr/bin:/etc:/etc/nfs
	SERVER=`uname -n`
	EXPORTS=/etc/dfs/dfstab
	EXP_FLD='$NF'
elif [ -f /etc/fstab ]; then
	Iam="FOURX";
	Opts_df=""
	Opts_du="-s"
	Opts_ps="-ax"
	Opts_ps_pid="-acx"
	Cmd_rarpd="/usr/etc/rarpd"
	Cmd_bpd="/usr/etc/rpc.bootparamd"
	PATH=/bin:/usr/bin:/usr/ucb:/etc:/usr/etc
	SERVER=`hostname`
	EXPORTS=/etc/exports
	EXP_FLD='$1'
else
	echo "no /etc/fstab or /etc/vfstab, bailing out"
	cleanup_and_exit 1      
fi                  

# export this so shareall can be found
export PATH

# since NIS_PATH won't be set when client boots, don't use it now
unset NIS_PATH

#
# Parse the command line options.
#
while [ "$1"x != "x" ]; do
    case $1 in
    -i)	IP_ADDR="$2";
        if [ ! "$IP_ADDR" ]; then
            usage ;
        fi
        # check with expr
        IP_ADDR=`expr $IP_ADDR : '\([0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\)'`
        if [ ! "${IP_ADDR}" ] ; then
                echo "mal-formed IP address: $2"
                cleanup_and_exit 1
        fi
        shift 2;;
    -e)	ETHER_ADDR="$2";
        if [ ! "$ETHER_ADDR" ]; then
            usage ;
        fi
        ETHER_ADDR=`expr $ETHER_ADDR : '\([0-9a-f][0-9a-f]*\:[0-9a-f][0-9a-f]*\:[0-9a-f][0-9a-f]*\:[0-9a-f][0-9a-f]*\:[0-9a-f][0-9a-f]*\:[0-9a-f][0-9a-f]*\)'`
        if [ ! "${ETHER_ADDR}" ] ; then
                echo "mal-formed Ethernet address: $2"
                cleanup_and_exit 1
        fi
        shift 2;;
    -s)	PRODUCT_SERVER=`expr $2 : '\(.*\):.*'`
    	PRODUCT_PATH=`expr $2 : '.*:\(.*\)'`
        if [ ! "$PRODUCT_SERVER" ]; then
            usage ;
        fi
        if [ ! "$PRODUCT_PATH" ]; then
            usage ;
        fi
        shift 2;;
    -t)	IMAGE_PATH=$2
        if [ ! "$IMAGE_PATH" ]; then
            usage ;
        fi
        shift 2;;
    -d)	tools_path=$2
        if [ ! "$tools_path" ]; then
            usage ;
        fi
        shift 2;;
    -p)	SYSID_CONFIG_SERVER=`expr $2 : '\(.*\):.*'`
    	SYSID_CONFIG_PATH=`expr $2 : '.*:\(.*\)'`
        if [ ! "${SYSID_CONFIG_SERVER}" ]; then
            usage ;
        fi
        if [ ! "${SYSID_CONFIG_PATH}" ]; then
            usage ;
        fi
        shift 2;;
    -c)	CONFIG_SERVER=`expr $2 : '\(.*\):.*'`
    	CONFIG_PATH=`expr $2 : '.*:\(.*\)'`
        if [ ! "$CONFIG_SERVER" ]; then
            usage ;
        fi
        if [ ! "$CONFIG_PATH" ]; then
            usage ;
        fi
        shift 2;;
    -n)	NS_POLICY=`echo "ns=$2"`;
	if [ ! "$2" ]; then
		usage ;
	fi
        shift 2;;
    -*)	# -anything else is an unknown argument
        usage ;
        ;;
    [a-zA-Z]*)	# must be the client name
        # ought to be last two things, eh?
        if [ $# -ne 2 ]; then
            usage ;
        fi
        CLIENT_NAME="$1";
        PGRP="$2";
        shift 2
        ;;
    *)	# then all else is spurious
        usage ;
        ;;
    esac
done

# Make sure the CLIENT_NAME is specified
#
if [ ! "${CLIENT_NAME}" ]; then
	echo "${myname}: No client name given."
	echo
	usage 
fi

# Make sure the platform group is specified
#
if [ ! "${PGRP}" ]; then
	echo "${myname}: No client platform group given."
	echo
	usage
fi

#
# Get path to the product hierarchy
# must maintain the hierarchy structure and RUN THIS COMMAND FROM THE HIERARCHY
#
# (1) find the path of the hierarchy.
# it may be absolute path
# it may be some relative path
# it may be given in PATH

if [ -n "${tools_path}" ]; then
	TOOLS_DIR=$tools_path
else
	case ${myname} in
	/*)     # absolute path, or found via $PATH (shells turn into abs path)
		TOOLS_DIR=`dirname ${myname}`
		myname=`basename ${myname}`
		;;

	./* | ../*)   # relative path from  "./" or ../, so we do a bit of clean up
		TOOLS_DIR=`pwd`/`dirname ${myname}`
		TOOLS_DIR=`(cd ${TOOLS_DIR} ; pwd )`
		myname=`basename ${myname}`
		;;

	*)      # name found via "." in $PATH
		TOOLS_DIR=`pwd`
		;; 
	esac
fi

# TOOLS_DIR is now an absolute path to the tools
# directory

#
## Set the PROD_DIR to the product directory
## This may be either a full netinstall image
## (Product and Tools/Boot directories exist)
## or an install server image (only Tools and
## Tools/Boot exist)
#
DISTRIBUTION_DIR=`(cd ${TOOLS_DIR}/../.. ; pwd )`
SOLARIS_PROD_DIR=`(cd ${TOOLS_DIR}/.. ; pwd )`

if [ -n "${IMAGE_PATH}" ]; then
	if [ ! -d ${IMAGE_PATH} ]; then
		echo "${myname}: Install boot image ${IMAGE_PATH} does not exist"
		cleanup_and_exit 1
	fi
else
	IMAGE_PATH=${TOOLS_DIR}/Boot
fi

## Check to see whether or not the install boot image is in 
## the current directory structure
if [ -z "${IMAGE_PATH}" ]; then
	echo "ERROR:  Install boot image location could not be determined"
	echo
	usage 
fi

#
# Verify that IMAGE_PATH is a valid boot image and exists
#
if [ ! -d ${IMAGE_PATH} ]; then
	echo "ERROR: Install boot image ${IMAGE_PATH} does not exist"
	cleanup_and_exit 1
fi

# check to see if ${IMAGE_PATH} is a local filesystem
# because we cannot export it otherwise.
dfout=`df ${Opts_df} ${IMAGE_PATH} | ( read junk; read where junk; echo $where )`
if [ ! -b "${dfout}" ] ; then
    echo "${myname}: \"${dfout}\" is not a local mount of ${IMAGE_PATH}"
    echo "           cannot export \"${dfout}\" for install clients"
    cleanup_and_exit 1
fi

# 
# Check to make sure that some critical file exists
#
if [ ! -d ${IMAGE_PATH}/.tmp_proto ]; then
	echo "${myname}: ${IMAGE_PATH} is not a valid Solaris install boot image"
	cleanup_and_exit 1
fi


# Set the boot method and boot method particulars
# based on the client architecture
#
if [ "${PGRP}" = "i86pc" ]; then
	BOOT_METHOD="rpl";
	Bootdir="/rplboot"
	Bootprog="rpld"
	ExtraSpaceNeeded=13
else
	BOOT_METHOD="tftp"
	Bootdir="/tftpboot"
	Bootprog="tftp"
	ExtraSpaceNeeded=6
fi

# Determine which name service is being used - Solaris 1.X can
# only run one name service. Solaris 2.X can run multiple
#
if [ "${Iam}" = "FOURX" ]; then
	#
	# figure out what the server is using for its database(s)
	#
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

##############################################################
# locate the correct subnet hostname supported on the local machine.
# check the local host table for any entry that matches the ip address
# of the clients host name.

get_realname

get_hostaddr ${CLIENT_NAME}
CLIENT_ADDR=${HOST_ADDR}

SERVER_ADDR=`/sbin/ifconfig -a | \
	while read ifname flags ; do
		if [ "${ifname}" = "ether" ]; then
			# read ether line from prev interface
			read ifname flags
			if [ $? -ne 0 ]; then
				break
			fi
		fi
		read inet ipaddr netmask mask broadcast broadaddr
		if [ "${ifname}" = "lo0:" ]; then
			continue;
		fi

		check_network ${mask} ${ipaddr} ${CLIENT_ADDR}
		if [ ${NETWORK_MATCHED} -ne 0 ]; then
		    echo $ipaddr
		    break;
		fi
	    done`

	    if [ -z "$SERVER_ADDR" ]; then
		echo "Warning: no interface configured for address $CLIENT_ADDR"
		SUBNET_NAME=$SERVER
	    else
		# look for subnet host name in the hosts file
		SUBNET_NAME=`grep -v "^#" /etc/hosts | grep "^\<$SERVER_ADDR\>" | \
		line | awk '{ print $2 }'`
		if [ -z "$SUBNET_NAME" ]; then
		    echo "Warning: no hostname found for address $SERVER_ADDR"
		    SUBNET_NAME=$SERVER
		fi
	    fi

#
# If install server only then make sure that the location
# of the product directory has been specified
#
if [ ! -d "${SOLARIS_PROD_DIR}/Product" ] ; then
	if [ ${PRODUCT_SERVER}"x" = "x" ]; then
	echo "This system is only set up as a boot server for install clients."
		echo "Use -s to specify a path to an install server."
		cleanup_and_exit 1
	fi
else
	if [ ${PRODUCT_SERVER}"x" = "x" ]; then
		PRODUCT_SERVER=${SUBNET_NAME}
		PRODUCT_PATH=${DISTRIBUTION_DIR}
	fi
fi

#
VERSION=`basename ${SOLARIS_PROD_DIR}`

ROOT=${IMAGE_PATH}
USR=${ROOT}/usr

# check client kernel and user architecture file trees
if [ ! -d "${ROOT}" ] ; then
    echo "The install root for client \"${CLIENT_NAME}\" does not exist"
    echo "path: ${ROOT}"
    cleanup_and_exit 1
fi

if [ ! -x ${USR}/platform/${PGRP} ]; then
	echo "${myname}: Unsupported platform group."
	echo
	usage 
fi

#
# Do the IP ADDRESS checks
#

# if someone gives IP_ADDR and it doesn't match, complain, else
# update /etc/hosts
if [ "${IP_ADDR}" ] ; then
    get_hostaddr ${CLIENT_NAME}
    ip_addr=${HOST_ADDR}

    # check to see if it already exists
    if [ "${IP_ADDR}" != "${ip_addr}" ]; then
        echo "Error: Different IP address found in the $NS hosts $NS_NAME"
        echo "       Address for host: ${ip_addr} ${CLIENT_NAME}"
        cleanup_and_exit 1
    fi

    # if client not in host file, update host file
    grep "\<${CLIENT_NAME}\>" /etc/hosts >/dev/null 2>&1
    if [ $? -ne 0 ]; then
        echo "Adding IP address for ${CLIENT_NAME} to /etc/hosts"
        echo "${IP_ADDR} ${CLIENT_NAME}" >> /etc/hosts
	if [ "${Iam}" = "FIVEX" ]; then
	    get_hostaddr ${CLIENT_NAME}
	    if [ $? -ne 0 ]; then
		db_in_source hosts files
		if [ $? -ne 0 ]; then
		    warn "IP address for \"${CLIENT_NAME}\" was added to /etc/hosts,"
		    warnc "yet nsswitch.conf is not configured to search there."
		    warnc "See nsswitch.conf(4) for additional information."
		fi
	    fi
	fi
    fi
fi

# if we're not given the address, look it up
if [ ! "$IP_ADDR" ]; then
    get_hostaddr ${CLIENT_NAME}
    IP_ADDR=${HOST_ADDR}
fi

# if still don't have it, error
if [ ! "${IP_ADDR}" ]; then
    echo "Error: IP address for $CLIENT_NAME not found in the $NS hosts $NS_NAME"
    echo "       Add it to the $NS hosts $NS_NAME and rerun ${myname}."
    cleanup_and_exit 1
fi

#
# Do the ETHERNET ADDRESS checks
#

# if an ethernet address is supplied on the command line, -e 8:0:20:e:a:02,
# then do the following:
#
#	- if there is an existing entry (local files, NIS, etc.) and this
#	  entry doesn't match the supplied value, then complain & exit
#
#	- if no existing entry, then update /etc/ethers
#
if [ "${ETHER_ADDR}" ] ; then
    # check to see if it already exists
    get_etheraddr ${CLIENT_NAME}
    if [ "${ETHERNET_ADDR}" -a "${ETHER_ADDR}" != "${ETHERNET_ADDR}" ]; then
        echo "Error: Different Ethernet number found in the $NS ethers $NS_NAME"
        echo "       Address for host: ${ETHERNET_ADDR} ${CLIENT_NAME}"
        cleanup_and_exit 1
    fi

    # if client not in ethers file, update ethers file
    grep "^[ 	]*[^# 	].*\<${CLIENT_NAME}\>" /etc/ethers >/dev/null 2>&1
    if [ $? -ne 0 ]; then
        echo "Adding Ethernet number for ${CLIENT_NAME} to /etc/ethers"
        echo "${ETHER_ADDR} ${CLIENT_NAME}" >> /etc/ethers
	if [ "${Iam}" = "FIVEX" ]; then
	    get_etheraddr ${CLIENT_NAME}
	    if [ $? -ne 0 ]; then
		db_in_source ethers files
		if [ $? -ne 0 ]; then
		    warn "Ethernet address for \"${CLIENT_NAME}\" was added to /etc/ethers"
		    warnc "yet nsswitch.conf is not configured to search there."
		    warnc "See nsswitch.conf(4) for additional information."
		fi
	    fi
	fi
    fi

else	
    #
    # no ethernet address was supplied.  Check to see is an entry exists.
    # If one does, set ETHERNET_ADDR to that value.
    #
    if [ "${Iam}" = "FIVEX" ]; then
        eval `lookup ethers ${CLIENT_NAME} byname`
        # if we're not given the ether_addr & using a name service
        if [ "${NS}" != "local" ]; then
	    #
	    # A map exists.  See if we can find an entry.
	    #
	    get_etheraddr ${CLIENT_NAME}
	    if [ ! "${ETHERNET_ADDR}" ] ; then
	        echo "Error: ${CLIENT_NAME} does not exist in the $NS ethers $NS_NAME"
	        echo "       Add it, and re-run ${myname}"
	        cleanup_and_exit 1
	    fi
        fi
    else	# FOURX
        # if we're not given the ether_addr & using a name service
        if [ "${NS}" != "local" ]; then
	    table_src ethers
	    if [ "${NS_SRC}" = "files" ] ; then
	        echo "No $NS ethers $NS_NAME, using /etc/ethers."
	    else
	        # A map exists.  The default rarpd won't check /etc/ethers if 
                # YP is running and an ethers map exists.  We just have to 
                # tell the user to update the map.
	        get_etheraddr ${CLIENT_NAME}
	        if [ ! "${ETHERNET_ADDR}" ] ; then
		    echo "Error: ${CLIENT_NAME} does not exist in the $NS ethers $NS_NAME"
		    echo "       Add it, and re-run ${myname}"
		    cleanup_and_exit 1
	        fi
	    fi
        fi
    fi

    # either NIS was not running, or there is no ethers map...
    NO_NIS_OR_PROBLEM=
    if [ ! "$ETHERNET_ADDR" ]; then
        ETHERNET_ADDR=`grep "^[ 	]*[^# 	].*\<${CLIENT_NAME}\>" \
	    /etc/ethers 2>/dev/null | (read addr name; echo $addr)`
        NO_NIS_OR_PROBLEM=true
    fi

    # if still don't have it, error
    if [ ! "${ETHERNET_ADDR}" ]; then
        echo "Error: Ethernet number for $CLIENT_NAME not found."
        cleanup_and_exit 1
    fi

    # Update ethers file if the client is not in the ethers file AND:
    #
    #    - NIS is not running or there is no ethers map
    # or
    #    - This is a FOURX system
    #
    grep "^[ 	]*[^# 	].*\<${CLIENT_NAME}\>" /etc/ethers >/dev/null 2>&1
    if [ $? -ne 0 ]; then
        if [ -n "$NO_NIS_OR_PROBLEM" -o "$Iam" = "FOURX" ] ; then
            echo "Adding Ethernet number for ${CLIENT_NAME} to /etc/ethers"
            echo "${ETHERNET_ADDR} ${CLIENT_NAME}" >> /etc/ethers
            if [ "${Iam}" = "FIVEX" ]; then
	        get_etheraddr ${CLIENT_NAME}
	        if [ $? -ne 0 ]; then
	            db_in_source ethers files
	            if [ $? -ne 0 ]; then
		        warn "Ethernet address for \"${CLIENT_NAME}\" was added to /etc/ethers"
		        warnc "yet nsswitch.conf is not configured to search there."
		        warnc "See nsswitch.conf(4) for additional information."
	            fi
	        fi
            fi
        fi
    fi
fi

#
# done with ETHERNET ADDRESS
#

#
# Add the boot image directory and the products directory (if local)
# to the exports file.
#
if [ "${PRODUCT_SERVER}" = "${SUBNET_NAME}" ]; then
	export_fs $PRODUCT_PATH
fi
export_fs $IMAGE_PATH

# CLEAN file is used so rm_install_client can undo (most of) the setup
CLEAN="${Bootdir}/rm.${IP_ADDR}"

# if config file already exists, run the cleanup - before checking tftpboot
if [ -f "${CLEAN}" ] ; then
    # do the cleanup (of old failed stuff)
    if [ ! -x ${TOOLS_DIR}/rm_install_client ] ; then
        echo "WARNING: could not execute: ${TOOLS_DIR}/rm_install_client"
        echo "  cannot clean up preexisting install client \"${CLIENT_NAME}\""
        echo "  continuing anyway"
    else
        echo "cleaning up preexisting install client \"${CLIENT_NAME}\""
        ${TOOLS_DIR}/rm_install_client ${CLIENT_NAME}
    fi
fi

#
# Set up tftp or rpl booting common area
#
#
# see if the server has the space we need in ${Bootdir}
# clean file and inetboot
#

aInetboot=${USR}/platform/${PGRP}/lib/fs/nfs/inetboot
if [ ! -f ${aInetboot} ]; then
	aInetboot=${USR}/lib/fs/nfs/inetboot
fi

# Determine the name to use for the inetboot program in bootdir.
# Either use an existing inetboot or make up the name with a version
# number appended.
for i in ${Bootdir}/inetboot*
do
	if cmp -s $aInetboot $i; then
		inetboot_to_use=$i
		break
	fi
done

if [ "$inetboot_to_use" ]; then
	Inetboot=`basename $inetboot_to_use`
else
	# Make this name not a subset of the old style names, so old style
	# cleanup will work.
	CONV_GRP=`echo ${PGRP} | tr "[a-z]" "[A-Z]"`

	max=0
	for i in ${Bootdir}/inetboot.${CONV_GRP}.${VERSION}*
	do
		max_num=`expr $i : ".*inetboot.${CONV_GRP}.${VERSION}-\(.*\)"`

		if [ "$max_num" -gt $max ]; then
			max=$max_num
		fi
	done

	max=`expr $max + 1`

	Inetboot=inetboot.${CONV_GRP}.${VERSION}-${max}
fi

# if we already have the inetboot, only need about 6 more k
#                                  (or 13K if we are using rpld)

if [ -f ${Bootdir}/${Inetboot} ]; then
    diskneeded=0
else
    diskneeded=`du ${Opts_du} ${aInetboot} | ( read size name; echo $size )`
fi
diskneeded=`expr $diskneeded + ${ExtraSpaceNeeded}`

if [ ! -d "${Bootdir}" ]; then
    # see if / has the space
    diskavail=`df ${Opts_df} "/" | tail -1 | awk '{print $4}'`
    echo "making ${Bootdir}"
    mkdir ${Bootdir}
    chmod 775 ${Bootdir}
    test ${Bootdir} = "/tftpboot" && ln -s . ${Bootdir}/tftpboot
else
    diskavail=`df ${Opts_df} "${Bootdir}" | tail -1 | awk '{print $4}'`
fi
if [ "${diskneeded}" -gt "${diskavail}" ] ; then
    echo "Not enough space in/for ${Bootdir}"
    echo "  needed: ${diskneeded}K, have: ${diskavail}K"
    cleanup_and_exit 1
fi


#
# Check to see if ETHER address and BOOTPARAMS have been determined
#


# get server IP address - it has to be in the server /etc/hosts
SERVER_IP_ADDR=` grep "\<${SUBNET_NAME}\>" /etc/hosts | \
    ( read ip nm; echo $ip ) `
if [ ${SERVER_IP_ADDR}"x" = "x" ] ; then
    echo "No server ip address for ${SUBNET_NAME} in /etc/hosts"
    cleanup_and_exit 1
fi

#
# Check for ${Bootdir}, create if necessary
#
#
if [ ! -d "${Bootdir}" ]; then
    echo "making ${Bootdir}"
    mkdir ${Bootdir}
    chmod 775 ${Bootdir}
    test ${Bootdir} = "/tftpboot" && ln -s . ${Bootdir}/tftpboot
fi

# check if ${Bootprog}, rarpd, rpc.bootparamd enabled, running. if not, start
# it (tftpd started from inetd.conf)

if [ "${BOOT_METHOD}" = "tftp" ]; then
    if grep '^#tftp[ 	]' /etc/inetd.conf > /dev/null ; then
	# found it, so it must be disabled, use our friend ed to fix it
	echo "enabling tftp in /etc/inetd.conf"
	( echo "/^#tftp/" ; echo "s/#//" ; echo "w"; echo "w"; echo "q" ) | \
		ed -s /etc/inetd.conf >/dev/null

	# but wait, we have to send a HUP to tell it to re-read inetd.conf
	pid=`ps ${Opts_ps_pid} | grep '[ /]inetd' | ( read pid junk ; echo $pid )`
	kill -HUP $pid
    fi
    # check for correctness of entry in inetd.conf
    Tftpd=`grep '^tftp[ 	]' /etc/inetd.conf | \
    ( read svc sock prot wait who path junk ; echo $path ) `
    if [ ${Tftpd}"x" = "x" ] ; then
	echo "tftp entry not found in /etc/inetd.conf"
	cleanup_and_exit 1
    fi
    if [ ! -x "${Tftpd}" ] ; then
	echo "tftp program (${Tftpd}) not found or not executable"
	cleanup_and_exit 1
    fi
else	# BOOT_METHOD = rpl
    # rpld - normally started from r3.d script
    if ( ps ${Opts_ps} | grep "[ /]rpld[ 	]" > /dev/null ) ; then
	: # found it, no need to start
    else
	Cmd_rpld=/usr/sbin/rpld
	if [ ! -x "${Cmd_rpld}" ] ; then
		echo "rpld program (${Cmd_rpld}) not found or executable"
		cleanup_and_exit 1
	fi
	echo "starting rpld"
	OLDWD=`pwd`
	cd /
	${Cmd_rpld} -a
	cd ${OLDWD}
    fi
fi

# rarpd - normally started from rc.local or equivalent
if (ps ${Opts_ps_pid} | awk '{print $NF}' | \
    grep "\<`basename ${Cmd_rarpd}`\>" >/dev/null); then
    : # found it, no need to start
else
    if [ ! -x "${Cmd_rarpd}" ] ; then
        echo "${Cmd_rarpd} not found"
        cleanup_and_exit 1
    fi
    echo "starting rarpd"
    OLDWD=`pwd`
    cd /
    ${Cmd_rarpd} -a
    cd ${OLDWD}
fi

# if this system is 5.x, may need to fix nsswitch.conf for bootparams
if [ "${Iam}" = "FIVEX" ]; then
	srcs=`get_sources bootparams`
	if [ "${srcs}" != "" ]; then
		set -- ${srcs}
		if [ "$1" != "files" ]; then
			bpent="bootparams:	files"
			for i in ${srcs}; do
				if [ "$i" != "files" ]; then
					bpent="${bpent} $i"
				fi
			done
			ed -s /etc/nsswitch.conf <<-EOF
			/bootparams:/
			c
			${bpent}
			.
			w
			q
			EOF
			echo "changed bootparams entry in /etc/nsswitch.conf"
		fi
	else
		echo "bootparams:	files" >> /etc/nsswitch.conf
		echo "added bootparams entry to /etc/nsswitch.conf"
	fi
fi

# rpc.bootparamd - normally started from rc.local or equivalent
if ( ps ${Opts_ps} | grep '[ /]rpc.bootparamd$' > /dev/null ) ; then
    : # found it, no need to start
else
    if [ ! -x "${Cmd_bpd}" ] ; then
        echo "${Cmd_bpd} not found"
        cleanup_and_exit 1
    fi
    echo "starting bootparamd"
    OLDWD=`pwd`
    cd /
    ${Cmd_bpd}
    cd ${OLDWD}
fi

###
### If kill and restart is needed, the segment below should work,
### it is currently targeting bootparamd.
###
### # rpc.bootparamd - normally started from rc.local or equivalent
### cmd=`ps ${Opts_ps_pid} | \
###     grep '[ /]rpc.boot$' | \
###     awk '{ if ( "$1" != "" ) { print "kill -TERM " $1 ";" }}'`
### if [ $? -eq 0 -a "${cmd}" != "" ]; then
###     # found it, kill it to make sure it gets the new information.
###     eval $cmd
### fi
### # Start bootparamd
### if [ ! -x ${Cmd_bpd} ] ; then
###     echo "WARNING: ${Cmd_bpd} not found - client may not be bootable."
### fi
### OLDWD=`pwd`
### cd /
### ${Cmd_bpd}
### cd ${OLDWD}

# nfsd & mountd - only started when exports or dfs/dfstab are there
if [ "${Iam}" = "FOURX" ] ; then
    if ( ps -x | grep -s '[0-9][ 	]*(nfsd)' > /dev/null ) ; then
        : # found it
    else
        # start it - really ought to pay attention to number in rc.local
        nfsd 8 &
        echo "starting nfsd's"
    fi
    # kind of redundant to check separately for mountd
    if ( ps -x | grep '[0-9][ 	]*rpc.mountd' > /dev/null ) then
        : # found it
    else
        # start it - using the sequence cribbed from rc.local
        echo "starting mountd"
        if [ -f /etc/security/passwd.adjunct ] ; then
            # Warning! Turning on port checking may deny access to
            # older versions (pre-3.0) of NFS clients.
            rpc.mountd
            echo "nfs_portmon/W1" | adb -w /vmunix /dev/kmem >/dev/null 2>&1
        else
            rpc.mountd -n
        fi
    fi
else # Iam FIVEX
    if ( ps -e | grep '[ /]nfsd$' > /dev/null ) ; then
        : # found it already
    else
        # start it
        /usr/lib/nfs/nfsd -a 8 &
        echo "starting nfsd's"
    fi
    if ( ps -e | grep '[ /]mountd$' > /dev/null ) ; then
        : # found it already
    else
        # start it
        /usr/lib/nfs/mountd &
        echo "starting nfs mountd"
    fi
fi

##########################################################################
# now we have gathered all the needed information, configure the server
# part. 2 - things that are specific to this client


#
# creation of bootparams file entry
#
# /etc/bootparams should override YP bootparams entry if it is before
#   the "+" that says go to NIS.
# a previous /etc/bootparams entry should supersede a later entry
#
# However, if a different server responds to the rarp request, the client
# will go to that server for bootparam info & get wrong info out of NIS,
# thus, ask sysadm to update NIS bootparam map if needed.
#

if [ "${CONFIG_SERVER}" ]; then
	add_config="install_config=${CONFIG_SERVER}:${CONFIG_PATH}"
fi

if [ "${SYSID_CONFIG_SERVER}" ]; then
	add_sysid_config="sysid_config=${SYSID_CONFIG_SERVER}:${SYSID_CONFIG_PATH}"
fi

if [ "${BOOT_METHOD}" = "rpl" ]; then
	rpl_config="numbootfiles=3 \
	bootfile=/rplboot/${IP_ADDR}.hw.com:45000 \
	bootfile=/rplboot/${IP_ADDR}.glue.com:35000 \
	bootfile=/rplboot/${IP_ADDR}.inetboot:8000 \
	bootaddr=35000"
fi

BOOTPARAMS="${CLIENT_NAME} \
root=${SUBNET_NAME}:${ROOT} \
	install=${PRODUCT_SERVER}:${PRODUCT_PATH} boottype=:in ${add_sysid_config} ${add_config} ${rpl_config} ${NS_POLICY}"

if [ "${Iam}" = "FIVEX" ]; then
    eval `lookup bootparams ${CLIENT_NAME}`
    # check if an entry for this client already in map
    if [ "${NS}" != "local" ]; then
	table_src bootparams
	if [ "${NS_SRC}" != "files" ]; then
	    in_bootparams ${CLIENT_NAME}
	    if [ $? -eq 0 ] ; then
		echo "Warning: ${CLIENT_NAME} has an entry in the $NS bootparams $NS_NAME."
		echo "         Update it with the following entry:"
		echo ${BOOTPARAMS}
		echo
	    fi
	fi
    fi
else	# FOURX
    # check if an entry for this client already in map
    if [ "${NS}" != "local" ]; then
	table_src bootparams
	if [ "${NS_SRC}" != "files" ]; then
	    in_bootparams ${CLIENT_NAME}
	    if [ $? -eq 0 ] ; then
		echo "Warning: ${CLIENT_NAME} has an entry in the $NS bootparams $NS_NAME."
		echo "         Update it with the following entry:"
		echo ${BOOTPARAMS}
		echo
	    fi
	fi
    fi
fi

# does an old entry already exist?
echo "updating /etc/bootparams"
grep "^${CLIENT_NAME}[ 	]" /etc/bootparams > /dev/null 2>&1
#
# Delete any existing ${CLIENT_NAME} entries in an existing bootparams file.
#
if [ $? -eq 0 ]; then
	cp /etc/bootparams /etc/bootparams.orig
	sed -e "/^${CLIENT_NAME}[ 	]/d" /etc/bootparams.orig \
	    > /etc/bootparams
fi
#
# Add before + (NIS Tag) in file
#
if [ -s /etc/bootparams ] ; then
    bootp_last=`tail -1 /etc/bootparams`
    if [ "+" = "$bootp_last" ]; then 
        ed_cmd=i 
    else 
        ed_cmd=a 
    fi 
    # just tack ours on to end, but before "+"
    ( echo "/^+" ; echo $ed_cmd ; \
      echo "${BOOTPARAMS}" ; echo '.'; echo "w"; echo "q" ) | \
    ed /etc/bootparams > /dev/null
else
    # no /etc/bootparams, this is easy
    echo "${BOOTPARAMS}" >> /etc/bootparams
fi

#
# start creating clean up file
#
echo "#!/sbin/sh" > ${CLEAN}			# (re)create it
echo "# cleanup file for ${CLIENT_NAME} - sourced by rm_install_client" >> ${CLEAN}
# NYD turn off traps ??? probably so

# install inet boot program
if [ ! -f ${Bootdir}/${Inetboot} ]; then
    echo "copying inetboot to ${Bootdir}"
    cp ${aInetboot} ${Bootdir}/${Inetboot}
    chmod 755 ${Bootdir}/${Inetboot}
fi

#
# tftpboot symlinks for IP address
#
if [ "${BOOT_METHOD}" = "tftp" ]; then
	HEXIP=`echo $IP_ADDR |\
	awk -F. '{
        	IP_HEX = sprintf("%0.2x%0.2x%0.2x%0.2x", $1,$2,$3,$4)
        	print IP_HEX 
        	}' | tr '[a-z]' '[A-Z]'`
	
	HEXIPARCH=${HEXIP}.`echo ${PGRP} | tr '[a-z]' '[A-Z]'`

	# Some of the very early ss1's may have proms that do not append
	# the karch when requesting inetboot.  All other sun4c, sun4m and
	# sun4d machines append karch.  IEEE prom based machines do not
	# append karch.
	if [ "${PGRP}" = "sun4c" ]; then
		setup_tftp ${HEXIPARCH}
		setup_tftp ${HEXIP}
	elif [ "${PGRP}" = "sun4m" -o "${PGRP}" = "sun4d" -o \
	       "${PGRP}" = "prep" ]; then
		setup_tftp ${HEXIPARCH}
	else
		setup_tftp ${HEXIPARCH}
		setup_tftp ${HEXIP}
	fi

	#
	# put final clean up commands into clean file
	#
	echo 'cnt=`ls -l /tftpboot | grep -w '${Inetboot}' | wc -l `' \
		>> ${CLEAN}
	echo 'if [ ${cnt} -eq 1 ]; then' >> ${CLEAN}
	echo "    echo \"removing /tftpboot/${Inetboot}\"" >> ${CLEAN}
	echo "    rm /tftpboot/${Inetboot}" >> ${CLEAN}
	echo "fi" >> ${CLEAN}

else # rpl booting protocol
	Dir=${USR}/platform/${PGRP}/lib/fs/nfs
	if [ ! -f ${Dir}/gluecode.com ]; then
		Dir="${USR}/lib/fs/nfs/drv.${PGRP}"
	fi

	if [ ! -r ${Bootdir}/${Board}.com ]; then
		cp ${Dir}/${Board}.com ${Bootdir}
	fi
	if [ ! -r ${Bootdir}/gluecode.com ]; then
		cp ${Dir}/gluecode.com ${Bootdir}
	fi
	OLDWD=`pwd`
	cd ${Bootdir}

	echo " echo \"Removing ${Bootdir} ${CLIENT_NAME}-specific files\"" >>${CLEAN}
	echo "rm ${Bootdir}/${IP_ADDR}.hw.com" >> ${CLEAN}
	echo "rm ${Bootdir}/${IP_ADDR}.glue.com" >> ${CLEAN}
	echo "rm ${Bootdir}/${IP_ADDR}.inetboot" >> ${CLEAN}

	if [ -h ${Bootdir}/${IP_ADDR}.hw.com ]; then
    	# save it, and stash the cleanup command
    	mv ${Bootdir}/${IP_ADDR}.hw.com ${Bootdir}/${IP_ADDR}.hw.com-
    	echo "mv ${Bootdir}/${IP_ADDR}.hw.com- ${Bootdir}/${IP_ADDR}.hw.com" >> ${CLEAN}
	fi
	if [ -h ${Bootdir}/${IP_ADDR}.glue.com ]; then
    	# save it, and stash the cleanup command
    	mv ${Bootdir}/${IP_ADDR}.glue.com ${Bootdir}/${IP_ADDR}.glue.com-
    	echo "mv ${Bootdir}/${IP_ADDR}.glue.com- ${Bootdir}/${IP_ADDR}.glue.com" >> ${CLEAN}
	fi
	if [ -h ${Bootdir}/${IP_ADDR}.inetboot ]; then
    	# save it, and stash the cleanup command
    	mv ${Bootdir}/${IP_ADDR}.inetboot ${Bootdir}/${IP_ADDR}.inetboot-
    	echo "mv ${Bootdir}/${IP_ADDR}.inetboot- ${Bootdir}/${IP_ADDR}.inetboot" >> ${CLEAN}
	fi

	ln -s ./${Board}.com ${IP_ADDR}.hw.com
	ln -s ./gluecode.com  ${IP_ADDR}.glue.com
	ln -s ${Inetboot} ${IP_ADDR}.inetboot
	cd ${OLDWD}

	#
	# put final clean up commands into clean files
	#
	echo 'cnt=`ls -l /rplboot | egrep -v "^total" | wc -l `' >> ${CLEAN}
	echo 'if [ ${cnt} -eq 4 ]; then' >> ${CLEAN}
	echo "    echo \"removing rplboot files\"" >> ${CLEAN}
	echo "    rm /rplboot/${Inetboot}" >> ${CLEAN}
	echo "    rm /rplboot/gluecode.com" >> ${CLEAN}
	echo "    rm /rplboot/smc.com" >> ${CLEAN}
	echo "fi" >> ${CLEAN}
fi

#
# BOOTP configuration.
#	- Check for SUNWpcdhc presence and the correct version
#	- If not found, try to install the SUNWpcdhc package
#	- Start in.dhcpd, if needed
# Note:
#	in.dhcpd must be started *after* the /tftpboot entries have
#	been created.
#
#
if [ "${PGRP}" = "prep" ]; then
	#
	# Check to see if the DHCP support package is installed.
	# If not, try to install it.
	#
	pkginfo -q $Pkg_dhcpd
	pkgstatus=$?
	add_or_up="adding"
	if [ $pkgstatus -eq 0 ]; then
		#
		# Package exists - make sure that the version
		# is 2.0 or later.
		#
		pkgvers=`pkgparam ${Pkg_dhcpd} VERSION`
		mvers=`expr $pkgvers : '^\([1-9]*\)'`
		if [ $mvers -lt 2 ]; then
			pkgstatus=1
			add_or_up="updating"
		fi
	fi

	#
	# Install the new package if it doesn't exist, or is
	# the wrong version.
	#
	if [ $pkgstatus -ne 0 ]; then
		#
		# Package doesn't exist.  Attempt to add it.
		#
		echo "$add_or_up package ${Pkg_dhcpd}...\c"
		install_pkg "Tools/Supplements/`uname -p`" $Pkg_dhcpd
		if [ $? -eq 0 ]; then
			echo "done."
		else
			echo "failed.  Exit code $?"
			echo "Couldn't install $Pkg_dhcpd."
			echo "Please add $Pkg_dhcpd and rerun $myname."
			cleanup_and_exit $?
		fi
	fi

	#
	# The package should be there now.  Try to start the daemon.
	#
	if (ps ${Opts_ps_pid} | awk '{print $NF}' | \
		grep "\<`basename ${Cmd_dhcpd}`\>" >/dev/null); then
		: # found it, no need to start
	elif [ -x "${Startup_dhcpd}" ]; then
		echo "invoking `basename ${Startup_dhcpd}`"
		OLDWD=`pwd`
		cd /
		${Startup_dhcpd} start
		cd ${OLDWD}
	elif [ -x "${Cmd_dhcpd}" ]; then
		echo "starting `basename ${Cmd_dhcpd}`"
		OLDWD=`pwd`
		cd /
		${Cmd_dhcpd}
		cd ${OLDWD}
	else
		echo "Can't find ${Startup_dhcpd} or ${Cmd_dhcpd}"
		echo "Please install $Pkg_dhcpd and rerun $myname"
		cleanup_and_exit 1
	fi
fi

cleanup_and_exit 0
