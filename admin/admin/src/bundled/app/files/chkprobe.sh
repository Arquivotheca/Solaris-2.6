#!/bin/sh
#
#       @(#)chkprobe.sh 1.45 96/09/10 SMI
#
# Copyright (c) 1992-1996 by Sun Microsystems, Inc. All rights reserved.
#

# user config file
CHECK_INPUT="${SI_CONFIG_DIR}/${SI_CONFIG_PROG}"

export CHECK_INPUT

# custom probes and comparisons come from this file in config dir
SI_CUSTOM_PROBES_FILE=custom_probes.ok

# output file for install
PROBE_OUTPUT=/tmp/install.input

CMP_VAR=compare_$$
RULES_VAR=rules_$$
NUM_RULES=/tmp/rule.num.$$

TMP_FILE=/tmp/probe.tmp.$$
TMP_FORMAT_OUTPUT=/tmp/probe_format_out.$$
CONV_TMP_FILE=/tmp/conv_awk.script.$$ 

# ====================== MASK IP FUNCTION =======================
# This function takes input IP and netmask
# Eg: mask_ipaddress 192.33.44.55 ffffff00
#

mask_ipaddress()
{
final_res=`echo | nawk -v ipaddr=$1 -v netmask=$2 '
function bitwise_and(x, y) {
	a[4] = x % 2;
	x -= a[4];
	a[3] = x % 4;
	x -= a[3]
	a[2] = x % 8;
	x -= a[2];
	a[1] = x;
	
	b[4] = y % 2;
	y -= b[4];
	b[3] = y % 4;
	y -= b[3]
	b[2] = y % 8;
	y -= b[2];
	b[1] = y;

	for (j = 1; j <= 4; j++)
		if (a[j] != 0 && b[j] != 0)
			ans[j] = 1;
		else
			ans[j] = 0;

	return(8*ans[1] + 4*ans[2] + 2*ans[3] + ans[4]);
} 

BEGIN {
	ip=ipaddr
	netm=netmask

	# set up the associative array for mapping hexidecimal numbers
	# to decimal fields.

	hex_to_dec["0"]=0
	hex_to_dec["1"]=1
	hex_to_dec["2"]=2
	hex_to_dec["3"]=3
	hex_to_dec["4"]=4
	hex_to_dec["5"]=5
	hex_to_dec["6"]=6
	hex_to_dec["7"]=7
	hex_to_dec["8"]=8
	hex_to_dec["9"]=9
	hex_to_dec["a"]=10
	hex_to_dec["b"]=11
	hex_to_dec["c"]=12
	hex_to_dec["d"]=13
	hex_to_dec["e"]=14
	hex_to_dec["f"]=15
	hex_to_dec["A"]=10
	hex_to_dec["B"]=11
	hex_to_dec["C"]=12
	hex_to_dec["D"]=13
	hex_to_dec["E"]=14
	hex_to_dec["F"]=15

	# split the netmask into an array of 8 4-bit numbers
	for (i = 1; i <= 8; i++)
		nm[i]=hex_to_dec[substr(netm, i, 1)]

	# split the ipaddr into its four decimal fields
	split(ip, df, ".")

	# now, for each decimal field, split the 8-bit number into its
	# high and low 4-bit fields, and do a bit-wise AND of those
	# fields with the corresponding fields from the netmask.

	for (i = 1; i <= 4; i++) {
		lo=df[i] % 16;
		hi=(df[i] - lo)/16;

		res_hi[i] = bitwise_and(hi, nm[2*i - 1])
		res_lo[i] = bitwise_and(lo, nm[2*i])
	}

	printf("%d.%d.%d.%d",
	    res_hi[1]*16 + res_lo[1],
	    res_hi[2]*16 + res_lo[2],
	    res_hi[3]*16 + res_lo[3],
	    res_hi[4]*16 + res_lo[4]);
}'`

export final_res
}

# ====================== PROBE FUNCTIONS =========================

probe_hostname() {
	[ -n "${SI_HOSTNAME}" ] && return

	SI_HOSTNAME=`uname -n`
	export SI_HOSTNAME
}

probe_hostaddress() {
	[ -n "${SI_HOSTADDRESS}" ] && return

	# take the 1st ipaddress that is not a loopback
	# hopefully the 0th interface
	SI_HOSTADDRESS=`ifconfig -a | \
	while read ifname flags ; do
		read inet ipaddr netmask mask broadcast broadaddr
		if [ "${ifname}" = "lo0:" ]; then
			continue;
		fi
       		echo ${ipaddr}
		break
	done`
	export SI_HOSTADDRESS
}

probe_network() {
	[ -n "${SI_NETWORK}" ] && return

	# take the 1st ipaddress that is not a loopback
	# hopefully the 0th interface
	SI_NETWORK=`ifconfig -a | grep broadcast | awk '{print $2, $4}'`

	mask_ipaddress $SI_NETWORK

	SI_NETWORK=${final_res}

	export SI_NETWORK
}

probe_domainname() {
	[ -n "${SI_DOMAINNAME}" ] && return

	SI_DOMAINNAME=`domainname`
	export SI_DOMAINNAME
}


#
# get_diskinfo
#
# Purpose : to set a lot of environment variables with disk information
# 
# Arguments :
#	$1 = disk device (/dev/dks/c0t2d0) slice is optional (it's removed)
#
#

get_diskinfo() {
	TMP_VTOC=/tmp/vtoc.$$

	# prtvtoc uses raw device.  must convert and add slice.
	g_d_basename=`basename $1`
	g_d_basename=`expr ${g_d_basename} : '\(c[0-9]*.*d[0-9]*\).*'`
	g_d_slice=0
	# work around prtvtoc bug
	while [ ${g_d_slice} -lt 8 ]
	do
		prtvtoc /dev/rdsk/${g_d_basename}s${g_d_slice} > ${TMP_VTOC} \
			2> /dev/null
		if [ $? -eq 0 ]; then
			break
		fi
		g_d_slice=`expr ${g_d_slice} + 1`
	done

	if [ ${g_d_slice} -eq 8 ]; then
		mbytes_p_cyl=0;	export mbytes_p_cyl;
		mbytes_p_disk=0;export mbytes_p_disk;
		bytes_p_sec=0;	export bytes_p_sec;
		track_p_cyl=0;	export track_p_cyl;
		sec_p_cyl=0;	export sec_p_cyl;
		sec_p_track=0;	export sec_p_track;
		cyls=0;		export cyls;
		return
	fi

	eval `awk 'BEGIN { tot = 1 } 
		$3 == "bytes/sector" 	{ bytes_p_sec = $2 } 
		$3 == "sectors/track"	{ sec_p_track = $2 } 
		$3 == "tracks/cylinder"	{ track_p_cyl = $2 } 
		$3 == "sectors/cylinder" { sec_p_cyl  = $2 } 
		$4 == "cylinders"	{ cyls	      = $2 } 
		# round down, just to be sure we do not over estimate
	END { 
		mbytes_p_disk= ((bytes_p_sec * sec_p_track) / 1048576) * \
					track_p_cyl * cyls;

		# be accurate in the mbytes_p_cyl, but integerize
		# mbytes_p_disk after this
		printf("mbytes_p_cyl=%f;export  mbytes_p_cyl;", \
							mbytes_p_disk/cyls);
		mbytes_p_disk=int(mbytes_p_disk);
	     	printf("mbytes_p_disk=%d;export mbytes_p_disk;", mbytes_p_disk);
		printf("bytes_p_sec=%d;  export bytes_p_sec;", 	bytes_p_sec);
		printf("track_p_cyl=%d;  export track_p_cyl;", 	track_p_cyl);
		printf("sec_p_cyl=%d;    export sec_p_cyl;", 	sec_p_cyl);
		printf("sec_p_track=%d;  export sec_p_track;",	sec_p_track);
		printf("cyls=%d;	 export cyls",  	cyls);

	}' ${TMP_VTOC}`

	rm -f ${TMP_VTOC}
}

#
# probe_rootdisk
#
# Purpose: set SI_ROOTDISK via autoamatic policy
#
#	If SI_ROOTDISK cannot be set, an error message is printed and the
#	system is shut down (guarantees to set SI_ROOTDISK)
#
probe_rootdisk() {
	# SI_ROOTDISK is already set; we're done
	if [ -n "${SI_ROOTDISK}" ] ; then
		return 0
	fi

	# check for stub-boot; if /tmp/.preinstall exists, it
	# may have the rootdisk already setup
	if [ -z "${SI_ROOTDISK}" -a -f /tmp/.preinstall ] ; then
		TMP_ROOTDISK=`cat /tmp/.preinstall`
		if [ -b "${TMP_ROOTDISK}" ] ; then
			SI_ROOTDISK=${TMP_ROOTDISK}
		fi
	fi

	# see if there is a device c0t3d0s0
	if [ -z "${SI_ROOTDISK}" -a -b /dev/dsk/c0t3d0s0 ] ; then
		SI_ROOTDISK=c0t3d0s0
	fi

	# nothing else has worked so just pick the first
	# available disk
	if [ -z "${SI_ROOTDISK}" ] ; then
		for i in /dev/dsk/*s0 ; do
			SI_ROOTDISK=`basename ${i}`
			break
		done
	fi

	# if SI_ROOTDISK isn't set by now, there are no disks on
	# the system; things are really hosed; shut down
	if [ -z "${SI_ROOTDISK}" ] ; then
		echo `gettext "ERROR: The rootdisk keyword cannot be set"`
		/sbin/uadmin 2 0
	fi

	export SI_ROOTDISK
	return 0
}

probe_disks() {
	[ -n "${SI_DISKLIST}" ] && [ -n "${SI_DISKSIZES}" ] && \
	[ -n "${SI_NUMDISKS}" ] && return

	echo '0\nquit' | format > /tmp/disks.tmp 2> /dev/null
	tmp_disk_list=`awk '/<.*>/ {print $2}' /tmp/disks.tmp`
	SI_NUMDISKS="0"
	for t_disk in ${tmp_disk_list}
	do  
		SI_NUMDISKS=`expr ${SI_NUMDISKS} + 1`
		if [ "${SI_DISKLIST}X" = "X" ]; then
			SI_DISKLIST="${t_disk}"
			get_diskinfo ${t_disk}
			SI_DISKSIZES=${mbytes_p_disk}
			SI_TOTALDISK=${mbytes_p_disk}
		else
			SI_DISKLIST="${SI_DISKLIST},${t_disk}"
			get_diskinfo ${t_disk}
			SI_DISKSIZES="${SI_DISKSIZES},${mbytes_p_disk}"
			SI_TOTALDISK=`expr ${mbytes_p_disk} + ${SI_TOTALDISK}`
		fi
	done
	export SI_DISKLIST 
	export SI_DISKSIZES
	export SI_NUMDISKS
	export SI_TOTALDISK
}

probe_totaldisk() {
	[ -n "${SI_TOTALDISK}" ] && return

	probe_disks
}

probe_arch() {
	[ -n "${SI_ARCH}" ] && return

	SI_ARCH=`uname -p`
	export SI_ARCH
}

probe_karch() {
	[ -n "${SI_KARCH}" ] && return

	SI_KARCH=`uname -m`
	export SI_KARCH
}

probe_model() {
	[ -n "${SI_MODEL}" ] && return

	SI_MODEL=`prtconf | awk ' {
			if (state == 1 && $0 != "") {
				print $0
				exit
			}
			if (substr($0, 1, 18) == "System Peripherals")
				state=1
		}'`

	# substitute all spaces with "_"
        SI_MODEL=`echo ${SI_MODEL} | sed 's/ /_/g' | sed "s/\'//g"`

	export SI_MODEL
}

probe_memsize() {
	[ -n "${SI_MEMSIZE}" ] && return

	SI_MEMSIZE=`prtconf | awk '/^Memory/ {print $3}'`
	export SI_MEMSIZE
}

# we are always running off the media; /cdrom/.cdtoc is always there
probe_osname() {
	[ -n "${SI_OSNAME}" ] && return

	PRODNAME=`grep PRODNAME /cdrom/.cdtoc | sed 's/PRODNAME=//'`
	PRODVERS=`grep PRODVERS /cdrom/.cdtoc | sed 's/PRODVERS=//'`

	[ "${PRODNAME}" != "Solaris" ] && return

	SI_OSNAME=${PRODNAME}_${PRODVERS}
	export SI_OSNAME
}

# check if the specified disk is installed with an OS
check_installed() {
	mnt_dev=$1
	fsck_dev=`echo ${mnt_dev} | sed 's,/dsk,/rdsk,g'`
	old_mntpnt=`get_mntpnt ${mnt_dev}`
	fsck -m ${fsck_dev} > /dev/null 2>&1
	fsck_status=$?
	if [ $fsck_status -ne 0 ]; then
		if [ $fsck_status -ne 32 ]; then
			return
		fi

		fsck -o p ${fsck_dev} >/dev/null 2>&1
		if [ $? -ne 0 ]; then
			# Unable to fsck ${fsck_dev} - user intervention required
			return
		fi
	fi
	mount ${mnt_dev} /a >/tmp/mount.out 2>&1
	if [ $? -ne 0 ]; then
		return
	fi

	if [ -f /a/etc/vfstab ]; then
		# "preinstalled" disks are not installed
		if [ -h /a/etc/.sysIDtool.state ]; then
			umount /a
			set_mntpnt ${mnt_dev} ${old_mntpnt}
			return
		fi

		VAR_DEV=`awk '{if ($3 == "/var") print $1}' /a/etc/vfstab`
		if [ "${VAR_DEV}" ]; then
			umount /a
			set_mntpnt ${mnt_dev} ${old_mntpnt}

			mnt_dev=${VAR_DEV}
			old_mntpnt=`get_mntpnt ${mnt_dev}`
			mount ${mnt_dev} /a >/tmp/mount.out 2>&1
			if [ $? -ne 0 ]; then
				# have seperate /var, but can't mount
				# just pick a generic version
				SI_INSTALLED=SystemV
				export SI_INSTALLED
				if [ -z "${SI_ROOTDISK}" ]; then
					SI_ROOTDISK=`basename $1`
					export SI_ROOTDISK
				fi
				return
			else
				# set path for vers since /var mounted
				SOFTINFO=/a/sadm/softinfo
				ADMINDIR=/a/sadm/system/admin
			fi
		else
			# set path for vers since no seperate /var
			SOFTINFO=/a/var/sadm/softinfo
			ADMINDIR=/a/var/sadm/system/admin
		fi

		#
		# Look in the new INST_RELEASE location first; if not
		# there, then look in the old location; if not there,
		# then just set a default of "SystemV"
		#
		if [ -d ${ADMINDIR} ]; then
			SI_INST_OS=`grep OS ${ADMINDIR}/INST_RELEASE \
				| cut -d= -f2`
			SI_INST_VER=`grep VERSION ${ADMINDIR}/INST_RELEASE \
				| cut -d= -f2`
			SI_INSTALLED="${SI_INST_OS}_${SI_INST_VER}"
		elif [ -d ${SOFTINFO} ]; then
			SI_INST_OS=`grep OS ${SOFTINFO}/INST_RELEASE \
				| cut -d= -f2`
			SI_INST_VER=`grep VERSION ${SOFTINFO}/INST_RELEASE \
				| cut -d= -f2`
			SI_INSTALLED="${SI_INST_OS}_${SI_INST_VER}"
		else
			# no vers info, just pick a generic version
			SI_INSTALLED=SystemV
		fi
		export SI_INSTALLED

		if [ -z "${SI_ROOTDISK}" ]; then
			SI_ROOTDISK=`basename $1`
			export SI_ROOTDISK
		fi
		umount /a
		set_mntpnt ${mnt_dev} ${old_mntpnt}
		return
	elif [ -f /a/etc/fstab ]; then
		SI_INSTALLED=SunOS4.x
		export SI_INSTALLED
		if [ -z "${SI_ROOTDISK}" ]; then
			SI_ROOTDISK=`basename $1`
			export SI_ROOTDISK
		fi
		umount /a
		set_mntpnt ${mnt_dev} ${old_mntpnt}
		return
	else
		umount /a
		set_mntpnt ${mnt_dev} ${old_mntpnt}
	fi
}

# try and find out if this machine is installed with some OS
probe_installed() {
	[ -n "${SI_INSTALLED}" ] && return

	case $1 in
	"rootdisk")
			# set SI_ROOTDISK
			probe_rootdisk
			check_installed /dev/dsk/${SI_ROOTDISK}
			;;

	"any")
			# if we know the rootdisk, check it first
			if [ -n "${SI_ROOTDISK}" ]; then
				check_installed /dev/dsk/${SI_ROOTDISK}
				if [ "${SI_INSTALLED}" ]; then
					return
				fi
			fi

			# look at all slices on all disks for a root FS
			for i in /dev/dsk/* ; do
				check_installed $i
				if [ "${SI_INSTALLED}" ]; then
					return
				fi
			done
			;;

	*)
			check_installed /dev/dsk/$1
			;;
	esac
}

#======================== COMPARISONS ==================================

# purpose:  exact match with SI_HOSTNAME
#  syntax:  hostname <host_name>
cmp_hostname () {
	probe_hostname

	if [ ${SI_HOSTNAME}"X" = $1"X" ]; then
		return 0
	else
		return 1
	fi
}

# purpose:  exact match with SI_HOSTADDRESS
#  syntax:  hostaddress <host_address>
cmp_hostaddress () {
	probe_hostaddress

	if [ ${SI_HOSTADDRESS}"X" = $1"X" ]; then
		return 0
	else
		return 1
	fi
}

# purpose:  exact match with SI_NETWORK
#  syntax:  network <net_number>
cmp_network () {
	probe_network

	if [ ${SI_NETWORK}"X" = $1"X" ]; then
		return 0
	else
		return 1
	fi
}

# purpose:  exact match with SI_OSNAME
#  syntax:  osname <os_name>
cmp_osname() {
	probe_osname

	if [ ${SI_OSNAME}"X" = $1"X" ]; then
		return 0
	else
		return 1
	fi
}

# purpose:  exact match with SI_DOMAINNAME
#  syntax:  domainname <domain_name>
cmp_domainname () {
	probe_domainname

	if [ ${SI_DOMAINNAME}"X" = $1"X" ]; then
		return 0
	else
		return 1
	fi
}

#  syntax:  disksize <disk_name>|rootdisk <size_range>
#    note:  $2 is a range specified as "x-y"
cmp_disksize () {
	if [ "${1}" = "rootdisk" ]; then
		# set SI_ROOTDISK
		probe_rootdisk
		get_diskinfo ${SI_ROOTDISK}
		SI_ROOTDISKSIZE=${mbytes_p_disk}
		export SI_ROOTDISKSIZE
		disk_size=${SI_ROOTDISKSIZE}
	else
		probe_disks
		dsk_name="${1}"
		echo ${SI_DISKLIST} | grep "${dsk_name}" >/dev/null 2>&1
		if [ $? -ne 0 ]; then 
			# disk is not here, it can't match
			return 1
		fi
		#
		# Determine the position of the disk in SI_DISKLIST so
		# you can find the corresponding size in the SI_DISKSIZES
		# list
		#
		n=0
		for disk in `echo ${SI_DISKLIST} | sed 's/,/ /g'`
		do
			n=`expr ${n} + 1`
			if [ `basename ${disk}` = `basename ${dsk_name}` ]; then
				break
			fi
		done
		i=0
		for size in `echo ${SI_DISKSIZES} | sed 's/,/ /g'`
		do
			i=`expr ${i} + 1`
			if [ ${i} = ${n} ]; then
				disk_size=${size}
				break
			fi
		done
	fi

	disk_range="${2}"
	t_lower=`expr ${disk_range} : '\([0-9]*\)-[0-9]*'`
	t_upper=`expr ${disk_range} : '[0-9]*-\([0-9]*\)'`

	if [ ${disk_size} -ge ${t_lower} -a \
	     ${disk_size} -le ${t_upper} ]; then
		return 0
	else 
		return 1
	fi
}


# purpose:  range match with SI_TOTALDISK
#  syntax:  totaldisk <size_range>
#    note:  $1 is a range specified as "x-y"
cmp_totaldisk () {
	probe_totaldisk

	# check if total disk space known
	if [ ${SI_TOTALDISK}"X" = "X" ]; then
		return 1
	fi
	t_lower=`expr $1 : '\([0-9][0-9]*\)-[0-9][0-9]*'`
	t_upper=`expr $1 : '[0-9][0-9]*-\([0-9][0-9]*\)'`

	if [ -z "${t_lower}" ]; then 
		return 1 
	fi

	if [ ${SI_TOTALDISK} -ge ${t_lower} -a \
	     ${SI_TOTALDISK} -le ${t_upper} ]; then
		return 0
	else 
		return 1
	fi
}

# purpose:  exact match with SI_ARCH
#  syntax:  arch <arch_value>
cmp_arch () {
	probe_arch

	if [ ${SI_ARCH}"X" = $1"X" ]; then
		return 0
	else
		return 1
	fi
}

# purpose:  exact match with SI_KARCH
#  syntax:  karch <karch_value>
cmp_karch () {
	probe_karch

	if [ ${SI_KARCH}"X" = $1"X" ]; then
		return 0
	else
		return 1
	fi
}

# purpose:  exact match with SI_MODEL
#  syntax:  model <model_name>
cmp_model () {
	probe_model

	if [ "${SI_MODEL}X" = "${1}X" ]; then
		return 0
	else
		return 1
	fi
}

# purpose:  range match with SI_MEMSIZE
#  syntax:  memsize <physical_memsize_range>
#    note:  $1 is a range specified as "x-y"
cmp_memsize () {
	probe_memsize

	t_lower=`expr $1 : '\([0-9]*\)-[0-9]*'`
	t_upper=`expr $1 : '[0-9]*-\([0-9]*\)'`

	if [ ${SI_MEMSIZE} -ge ${t_lower} -a \
	     ${SI_MEMSIZE} -le ${t_upper} ]; then
		return 0
	else 
		return 1
	fi
}

# purpose:  match with SI_INSTALLED
#  syntax:  installed <disk_name> <release>|any|upgrade
cmp_installed () {
	probe_installed $1

	if [ ! "${SI_INSTALLED}" ]; then
		return 1
	fi

	if [ "${SI_INSTALLED}" = "none" ]; then
		return 1
	fi

	if [ $2"X" = "anyX" ]; then
		return 0
	fi

	if [ $2"X" = "upgradeX" ]; then
		if [ "${SI_INSTALLED}" = "SystemV" -o \
			"${SI_INSTALLED}" = "Solaris_2.0" ]; then
			return 1
		else
			i=`expr ${SI_INSTALLED} : '\(Solaris\).*'`
			if [ -z "${i}" ]; then
				return 1
			else
				return 0
			fi
		fi
	fi

	# exact match with installed release
	if [ ${SI_INSTALLED}"X" = $2"X" ]; then
		return 0
	else
		return 1
	fi
}

# purpose:  ignores constant, always matches true
#  syntax:  any
cmp_any () {
	return 0
}

#======================== External Functions  ==============================

# source custom comparisons.
# XXX this may go away for initial release
# XXX or maybe just don't expose?
if [ -x ${SI_CONFIG_DIR}/${SI_CUSTOM_PROBES_FILE} ]; then
	gettext "Loading user defined probe and match routines...."; echo
	.  ${SI_CONFIG_DIR}/${SI_CUSTOM_PROBES_FILE}
fi
 
#======================== Internal Functions=========================

#
# convert_args
#
# Purpose : this function takes arguments and see's if they have single
# 		quotes arount them. If they do, it concatenates all
# 		arguments between each set of "'"'s with _'s instead of spaces.
#
# Inputs : any args
#
# Ret Val: $* with all quoted args spaces converted to _
#
convert_args () 
{

	# if no 's then get out
	if echo $* | grep -v \' > /dev/null 2>&1; then
		echo $*
		return
	fi

	# because of all the funkiness of handling "'" s, put it in a file
	if [ ! -f "${CONV_TMP_FILE}" ]; then

		cat << \FOOBAR > "${CONV_TMP_FILE}"
		BEGIN { stderr="cat 1>&2" }
		{
			i = 1
			var=""
			len=length($0)
			while (i <= len) {
				if (substr($0, i, 1) == "'") {
					if (length(var) > 0) {
						print var
						var=""
					}
					i++
					while (substr($0, i, 1) != "'" && i <= len) {
						if (substr($0, i, 1) == " ") {
							var=var "_"
						} else {
							var=var substr($0, i, 1)
						}
						i++
					}
					if ( i > len ) {
						print "Mis-matched \"'\" quotes <" $0 ">" | stderr
						exit 1
					}
				} else {
					if (length(var) > 0) {
						print var
						var=""
					}
					while (substr($0, i, 1) != " " && i <= len) {
						var=var substr($0, i, 1)
						i++
					}
						
				}
				i++
			}
			if (length(var) > 0) print var
		}
FOOBAR
	fi

	echo "$*" | nawk -f ${CONV_TMP_FILE}
}


not_q() {
	n_ret_val=${1}
	case ${n_ret_val} in
	0) [ ${not_flag} -eq 1 ] && n_ret_val=1;;
	1) [ ${not_flag} -eq 1 ] && n_ret_val=0;;
	esac
	not_flag=0
	return ${n_ret_val}
}

#======================== Interpreter  ==============================

# put each rule in a different env variable for easier handling
# and put the total rule count in the ${NUM_RULES} file
#
tmp_rules=0
echo ${tmp_rules} > ${NUM_RULES}
eval `while read first rest ; do 
	case ${first} in 
	\#*)	# comment 
		continue 
		;;
	"")	# blank line
		continue
		;;
	esac

	# take out comments at end of line
	# this does have the side effect of not allowing any '#' to be in
	# the grammer, but that is acceptable. 

	tmp_rules=\`expr ${tmp_rules} + 1\`
	echo ${tmp_rules} > ${NUM_RULES}
	tmp_var=\`echo ${first} ${rest} | sed 's/\#.*$//'\`
	echo  ${RULES_VAR}_${tmp_rules}=\"${tmp_var}\"";"

done < ${CHECK_INPUT}`
	

# now, for each rule that we have to parse, break it up into individual
# match and  mulitple match_key arguments.

tot_rules=`cat ${NUM_RULES}`
rm -f ${NUM_RULES}
rule_num=0
saverc=1

while test ${rule_num} -lt ${tot_rules}
do	

	rule_num=`expr ${rule_num} + 1`
	rule=`eval echo $"${RULES_VAR}_${rule_num}"`

	set -- ${rule}
	# if probe function, then run it 
	if [ ${1}"X" = "probeX" -o ${1}"X" = "setX" ]; then
		probe_name=${2}
		shift; shift
		probe_${probe_name} `convert_args $*`
		continue
	fi

	# not a probe...so it must be a compare.
	# grab the begin, profile, and end scripts/profiles
	eval `echo ${rule} | awk '{printf( "begin=%s; class=%s; finish=%s", \
			   $(NF - 2), $(NF - 1), $NF)}'`
	
	# XXX - debuging
	# echo begin=${begin} class=${class} finish=${finish}
	
	# strip the line of the last three arguments...
	# ("begin", "profile", and "end").. to give only match/match_key
	# section of the rule
	matches=`echo ${rule} | awk '{ for (i = 1; i < NF - 2; i++) 
				     printf("%s ",$i) }'`
	
	# split the matches into separate variables, independant of # of args
	# for easy parsing... ... it might seem gorpy, but it
	# simplifies the handling the arguments and leaves the compare
	# routines free to do what they will with the arguments.
	eval `echo ${matches} | awk -F'&&' ' BEGIN {count = 0}
		{ for (i = 1; i <= NF; i++) 
			if ($i != "") {
			       	count++
				cmd_var = var"_"count
			   	printf("%s=\"%s\";", cmd_var, $i) 
			}
		}
		END {printf("tot_cmps=%s", count)}' var=${CMP_VAR}`
	
	
	saverc=1	# start with no match
	cmp_num=0

	# make all the compare matches.
	while test ${cmp_num} -lt ${tot_cmps}
	do
		not_flag=0
		cmp_num=`expr ${cmp_num} + 1`

		comp=`eval echo $"${CMP_VAR}_${cmp_num}"`

		set -- ${comp}

		if [ "${1}"X = "!"X ]; then
			not_flag=1
			shift
		fi
	
		# check should have made sure there are at least 2 arguments.
		set -- `echo $*`
		key=${1}
		first_arg=${2}
		shift; shift

		next_args=`echo $*`
	
		# we must have a cmp at this point
		cmp_${key} `convert_args ${first_arg} ${next_args}`
		not_q $?
		rc=$?
		if [ $rc -ne 0 ]; then
			saverc=${rc}
			break	# does not match, so keep on parsing.
		fi
		saverc=0
	done	# while cmdfiles

	if [ ${saverc} -eq 0 ]; then
		break;
	fi

done	# while rules

if [ ${saverc} -eq 0 ]; then
	# we have a match...so let's set the correct variables
	SI_BEGIN=${begin}; export SI_BEGIN
	SI_CLASS=${class}; export SI_CLASS
	SI_FINISH=${finish}; export SI_FINISH

	if [ "${SI_BEGIN}" -a X"${SI_BEGIN}" != "X-" ]; then
		echo `gettext "Using begin script: "` ${SI_BEGIN}
	fi
	if [ "${SI_CLASS}" ]; then
		if [ "X${SI_CLASS}" = "X-" ]; then
			# null profile
			unset SI_PROFILE
		elif [ "X${SI_CLASS}" = "X=" ]; then
			# create a temporary profile file
			echo `gettext "Using derived profile: "` ${SI_BEGIN}
			SI_PROFILE="${PROBE_OUTPUT}"
			export SI_PROFILE
		else
			echo `gettext "Using profile: "` ${SI_CLASS}
			SI_PROFILE=${SI_CONFIG_DIR}/${SI_CLASS}
			export SI_PROFILE
		fi
	fi
	if [ "${SI_FINISH}" -a X"${SI_FINISH}" != "X-" ]; then
		echo `gettext "Using finish script: "` ${SI_FINISH}
	fi
fi

if [ -f "${CONV_TMP_FILE}" ]; then
	/usr/bin/rm -f "${CONV_TMP_FILE}"
fi
