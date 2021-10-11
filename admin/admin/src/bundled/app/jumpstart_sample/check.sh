#!/bin/sh
#
#       @(#)check.sh 1.44 96/08/21 SMI
#
# Copyright (c) 1995-1996 by Sun Microsystems, Inc. All Rights Reserved.

#	This shell script is for checking the validity of rules,
#	begin, profiles, finish, and probes.
#
# NOTE:	cannot I18N this script since it runs on 4.x & 5.x systems

PATH=/usr/ucb:/usr/sbin/install.d:${PATH}
export PATH

myname=`basename $0`	# check script name for usage message

DEBUG=0
CONV_TMP_FILE=/tmp/conv_awk.script.$$ 
PROF_TMP_FILE=/tmp/.profile_names.$$
CMP_VAR=compare_$$
STD_RULES=rules
CUSTOM_PROBES=custom_probes

# error codes
E_PROBE_NOT_FOUND=1
E_CMP_NOT_FOUND=2
E_CLASS_NOT_FOUND=3
E_FINISH_NOT_FOUND=4
E_LACK_ARGS=5
E_BEGIN_NOT_FOUND=6
E_NO_BEGIN_TO_DERIVE=7
E_EXCESS_ARGS=8
E_DUP_INST_TYPE=9
E_INV_INST_TYPE=10
E_DUP_SYS_TYPE=11
E_INV_SYS_TYPE=12
E_DUP_PART_TYPE=13
E_INV_PART_TYPE=14
E_INV_OPTION=15
E_NO_INST_TYPE=16
E_CMP_LACK_ARGS=17
E_BAD_DEVICE_SPEC=18
E_AUTO_EXISTING=19
E_MOUNT_FULL_PATH=20
E_BAD_SIZE=21
E_INVALID_NUMBER=22
E_INVALID_KEY=23
E_INITIAL_KEY=24
E_UPGRADE_KEY=25
E_BAD_TYPE=26
E_INV_SERVER_ADDR=27
E_DATALESS_EOL=28
E_DUP_ROOT_DEVICE=29
E_INST_TYPE_FIRST=30
E_DUP_BOOT_DEVICE=31
E_INV_PROM_UPDATE=32

# fatal_error()
# Purpose:	print appropriate error according to ${1}
# Input:	${1}	- error code
#
fatal_error()
{
	echo ""
	if [ "${profile_ln}" -a "${profile_ln}" -ne 0 ]; then
		line_number=${profile_ln}
		read_file="${profile_file}"
	else
		line_number=${ln}
		read_file="${in_file}"
	fi

	echo "Error in file \"${read_file}\", line ${line_number}"
	echo "	${line}"
	echo -n "ERROR: "

	case "${1}" in
	${E_EXCESS_ARGS})
		echo "Too many arguments for this keyword"
		;;
	${E_LACK_ARGS})
		echo "Insufficient arguments for this keyword"
		;;
	${E_CMP_LACK_ARGS})
		echo "Insufficient arguments for match key: ${cmp_err_rulename}"
		;;
	${E_PROBE_NOT_FOUND})
		echo "Probe function \"probe_${probe_name}\" undefined"
		;;
	${E_CMP_NOT_FOUND})
		echo "Match key: ${key} undefined"
		;;
	${E_CLASS_NOT_FOUND})
		echo "Profile missing: ${profile}"
		;;
	${E_FINISH_NOT_FOUND})
		echo "Finish script missing: ${finish}"
		;;
	${E_BEGIN_NOT_FOUND})
		echo "Begin script missing: ${begin}"
		;;
	${E_NO_BEGIN_TO_DERIVE})
		echo "Missing begin script for derived profile"
		;;
	${E_DUP_INST_TYPE})
		echo "Duplicate install type specified"
		;;
	${E_INST_TYPE_FIRST})
		echo "First specified keyword must be \"install_type\""
		;;
	${E_DUP_ROOT_DEVICE})
		echo "Duplicate root_device specified"
		;;
	${E_DUP_BOOT_DEVICE})
		echo "Duplicate boot_device specified"
		;;
	${E_INV_BACKUP_IDENT})
		echo "Invalid backup media identifier specified"
		;;
	${E_INV_LAYOUT_CONSTRAINT})
		echo "Invalid layout constraint specified"
		;;
	${E_INV_INST_TYPE})
		echo "Invalid install type specified"
		;;
	${E_NO_INST_TYPE})
		echo "No install type specified"
		;;
	${E_INV_SERVER_ADDR})
		echo "Invalid server address specified"
		;;
	${E_DUP_SYS_TYPE})
		echo "Duplicate system type specified"
		;;
	${E_INV_SYS_TYPE})
		echo "Invalid system type specified"
		;;
	${E_DUP_PART_TYPE})
		echo "Duplicate partitioning specified"
		;;
	${E_INV_PART_TYPE})
		echo "Invalid partitioning specified"
		;;
	${E_INV_OPTION})
		echo "Third argument must be 'add' or 'delete'"
		;;
	${E_BAD_SIZE})
 	 	echo "Size \"${size}\" is invalid" 
		;;
	${E_MOUNT_FULL_PATH})
		echo "Mount point \"${mount}\" must begin with a '/'"
		;;
	${E_AUTO_EXISTING})
		echo -n "A \"preserved\" filesystem cannot be \"auto\" sized"
		echo    ", use \"existing\""
		;;
	${E_BAD_DEVICE_SPEC})
		echo "Device \"${device_name}\" is badly specified"
		;;
	${E_INVALID_NUMBER})
		echo "The argument is not a number"
		;;
	${E_INVALID_KEY})
		echo "Invalid keyword"
		;;
	${E_INITIAL_KEY})
echo "This keyword can only be used with an install_type of initial_install"
		;;
	${E_UPGRADE_KEY})
	echo "This keyword can only be used with an install_type of upgrade"
		;;
	${E_BAD_TYPE})
		echo "Type \"${type}\" is invalid"
		;;
	${E_INV_PROM_UPDATE})
		echo "Invalid prom update value specified"
		;;
	esac

	echo 
	exit 1
}

# warning_msg()
# Purpose:	print warning message
# Input:	${1}	- warning code
#
warning_msg()
{
	echo ""	

	if [ "${profile_ln}" -a "${profile_ln}" -ne 0 ]; then
		line_number=${profile_ln}
		read_file="${profile_file}"
	else
		line_number=${ln}
		read_file="${in_file}"
	fi

	echo "Warning in file \"${read_file}\", line ${line_number}"
	echo "	${line}"
	echo -n "WARNING: "

	case "${1}" in
	${E_EXCESS_ARGS})
		echo "Too many arguments for this keyword"
		;;
	${E_DATALESS_EOL})
	    echo "SunSoft plans to remove support for the dataless client system"
	    echo -n "	 "
	    echo "type after Solaris 2.5. You can use this system type now, but in"
	    echo -n "	 "
	    echo "future releases you will need to select a different option."
		;;
	esac

	echo ""
}

# get_functions()
# Purpose:	find all the function headers within the file ${1}
#
get_functions() 
{
	while read first rest ; do 
		case ${first} in 
		\#*)	# comment 
			continue 
			;;
		"")	# blank line
			continue
			;;
		esac
	
		# take out comments at end of line, and white space before
		# comment.  this does have the side effect of not allowing
		# any '#' to be in the grammar, but that is acceptable.
		tmp_rest=`echo ${rest} | sed 's/[	 ]*\#.*$//'`
		if [ -z "${tmp_rest}" ]; then	
			set -- ""
			shift
		else	
			set -- ${first} ${tmp_rest}
		fi

		line="${first} ${tmp_rest}"

		if echo ${line}|grep '[a-zA-Z_][a-zA-Z0-9_]*[ 	]*()' >/dev/null
		then
		func=`expr "$line" : '\([a-zA-Z_][a-zA-Z0-9_]*\)[ 	]*()'`
			echo ${func}
			if [ ${DEBUG} -eq 1 ]; then
				echo ${func} > /dev/tty
			fi
		fi

	done < ${1}
}

# create_derived_file()
# Purpose:	create the derived file which will have comments and
#		blank lines stripped
# Input:	${1} - input file name
#	   	${2} - output file name
#
create_derived_file() 
{
	awk -F\# '{print $1}' ${1} | awk 'NF != 0 {print}' \
		| sed 's/^[	]*//g' > ${2}

}

# append_checksum()
# Purpose:	append a checksum to the end of the file
# Inputs:	${1} - input file name
# Returns:	0 - okay
#		1 - error
#
append_checksum()
{
	in_file="${1}"
	chksum=`/bin/sum ${in_file} | /bin/awk '{print $1}'`
	echo "# version=2 checksum=${chksum}" >> ${in_file}
	if [ $? -ne 0 ]; then
		return 1
	fi
	return 0
}

# convert_args
# Purpose:	this function takes arguments and see's if they have single
# 		quotes arount them. If they do, it concatenates all
# 		arguments between each set of "'"'s with _'s instead of spaces.
# Inputs:	any args
#
convert_args()
{
	# if no 's then get out
	if echo $* | grep -v \' > /dev/null 2>&1
	then
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

# check_arg_cnt()
# Purpose:	XXX hardcoded to check specific rules that have more
#		then 1 parameter
#
check_arg_cnt()
{
	if [ "${1}" = "disksize" -a ${2} -ne 3 ]; then
		cmp_err_rulename=${1}
		fatal_error ${E_CMP_LACK_ARGS}
	elif [ "${1}" = "installed" -a ${2} -ne 3 ]; then
		cmp_err_rulename=${1}
		fatal_error ${E_CMP_LACK_ARGS}
	fi
}

# check_part_name()
# Purpose:	subroutine to check if string is a valid canonical
#		fdisk partition name
# Input:	${1}	- string to be validated
#
check_part_name()
{
	cnt=`expr "${1}" : 'c[0-9][0-9]*t[0-9][0-9]*d[0-9][0-9]*p[1-4]$'`
	if [ ${cnt} -eq 0 ]; then
		cnt=`expr "${1}" : 'c[0-9][0-9]*d[0-9][0-9]*p[1-4]$'`
		if [ ${cnt} -eq 0 ]; then
			return 0
		fi
	fi

	return 1
}

# check_slice_name()
# Purpose:	subroutine to check if string is a valid canonical
#		slice name
# Input:	${1}	- string to be validated
#
check_slice_name()
{
	cnt=`expr "${1}" : 'c[0-9][0-9]*t[0-9][0-9]*d[0-9][0-9]*s[0-7]$'`
	if [ ${cnt} -eq 0 ]; then
		cnt=`expr "${1}" : 'c[0-9][0-9]*d[0-9][0-9]*s[0-7]$'`
		if [ ${cnt} -eq 0 ]; then
			return 0
		fi
	fi

	return 1
}

# is_slice_name()
# Purpose:	subroutine determining if string is a valid canonical
#		slice name
# Input:	${1}	- string to be validated
#
is_slice_name()
{
	check_slice_name ${1}
	if [ $? -eq 0 ]; then
		device_name=${1}
		fatal_error ${E_BAD_DEVICE_SPEC}
	fi
}

# is_devname()
# Purpose:	subroutine determining if string is a valid
#		canonical disk name
# Input:	${1}	- string to be validated
#
is_dev_name()
{
	cnt=`expr "${1}" : 'c[0-9][0-9]*t[0-9][0-9]*d[0-9][0-9]*$'`
	if [ ${cnt} -eq 0 ]; then
		cnt=`expr "${1}" : 'c[0-9][0-9]*d[0-9][0-9]*$'`
		if [ ${cnt} -eq 0 ]; then
			device_name=${1}
			fatal_error ${E_BAD_DEVICE_SPEC}
		fi
	fi
}

# is_num()
# Purpose:	Boolean function determining if the string is
#		comprised entirely of numeric characters
# Input:	${1}	- string to be validated
#
is_num()
{
	cnt=`expr "${1}" : '[0-9][0-9]*$'`
	if [ ${cnt} -eq 0 ]; then
		return 0
	fi
	return 1
}

# check_boot_device()
# Purpose:	check syntax of boot_device keyword in profile
# Input:	boot_device <boot device> <prom update>
#				${1}	       ${2}
#
check_boot_device()
{
	# check <boot device> syntax
	# valid values:		c#[t#]d#s#
	#			c#[t#]d#p#
	#			existing
	#			any
	check_slice_name ${1}
	slice=$?
	check_part_name ${1}
	part=$?
	if [ "$slice" -eq 0 -a "$part" -eq 0 -a \
			${1} != "existing" -a ${1} != "any" ]; then
		device_name=${1}
		fatal_error ${E_BAD_DEVICE_SPEC}
	fi

	# check <prom update> syntax
	# valid values:		update
	#			preserve
	if [ "${2}" != "update" -a "${2}" != "preserve" ]; then
		device_name=${2}
		fatal_error  ${E_INV_PROM_UPDATE}
	fi
}

# check_fdisk()
# Purpose:	check syntax of fdisk keyword in profile
# Input:	${1}	- <diskname>
#		${2}	- <type>
#		${3}	- <size>
#
check_fdisk()
{
	# check <diskname> syntax
	# valid values: 	rootdisk
	#			all
	#			cx[ty]dz
	disk=`echo ${1} | tr '[A-z] '[a-z]'`    # lowercase it
	if [ "$disk"X != "rootdisk"X -a "$disk"X != "all"X ]; then
		is_dev_name ${1}
	fi

	# check <type> syntax
	# valid values:		solaris
	#			dosprimary
	#			###
	type=`echo ${2} | tr '[A-Z]' '[a-z]'`	# lowercase it
	case "${type}" in
	    solaris | dosprimary)
		;; 	
	    *)  is_num ${type}
		if [ $? -eq 0 ]; then
			cnt=`expr "${type}" : '0x[0-9a-f][0-9a-f]*'`
			if [ ${cnt} -eq 0 ]; then
				fatal_error ${E_BAD_TYPE}
			fi
		fi
		;;
	esac

	# check <size> syntax
	# valid values:		all
	#			delete
	#			maxfree
	#			###
	size=`echo ${3} | tr '[A-Z]' '[a-z]'`	# lowercase it
	case "${size}" in
	    all | delete | maxfree)
		;; 	
	    *)	is_num ${size}
		if [ $? -eq 0 ]; then
			fatal_error ${E_BAD_SIZE}
		fi
		;;
	esac
}

# check_local_filesys()
# Purpose:	check syntax of local filesys keyword in profile
# Input:
# filesys <device> <size> [ <mount> [ <fsoptions> ] ]
#	    ${1}    ${2}  [  ${3}		    ]
#
#				    [ preserve [ <mntopts> ]
#					 ${4}	  ${5} 
#
#				    [ <mntopts> ]
#					 ${4}
#
check_local_filesys()
{
	# check <device> syntax
	# valid values:	"any",
	#		"rootdisk.s"#
	# 		<xxx>:/<yyy>
	#		cx[ty]dzs#
	disk=`echo ${1} | tr '[A-Z]' '[a-z]'`  # lowercase it 
	if [ "$disk"X != "anyX" ]; then 
		cnt=`expr $disk : 'rootdisk[.]s[0-7]'`
		if [ ${cnt} -eq 0 ]; then 
			# must be in c0txdys0 form
			is_slice_name $disk
		fi
	fi

	# check <size> syntax
	# valid values:		auto
	#			all
	#			existing
	#			free
	#			###:###
	#			###
	size=`echo ${2} | tr '[A-Z]' '[a-z]'`	# lowercase it
	case "${size}" in
	    auto | all | existing | free)
		;; 	

	    *)	# we have a size, so check it out some
		# check x:y or just a num
		dash=`expr ${size} : '[0-9]*\(:\)[0-9]*'`
		t_start=`expr ${size} : '\([0-9]*\):[0-9]*'`
		if [ ! "${t_start}" ]; then
			# if the start cyl is not there, must be a num
			is_num ${size}
			if [ $? -eq 0 ]; then
				fatal_error ${E_BAD_SIZE}
			fi
		fi
		;;
	esac

	# check <mount> syntax
	# valid values:		swap
	#			unnamed
	#			overlap
	#			ignore
	#			<pathname>
	mount=`echo ${3} | tr '[A-Z]' '[a-z]'`	# lowercase it
	case "${mount}" in
	    swap | unnamed | overlap | ignore)
		;;
	    *)	if [ -n "${3}" ]; then
			# expr would take "/" as being a division,
			# so special case this 
			first_let=`expr "${3}"X : '\(.\)[.]*'`
			if [ "${first_let}" != "/" ]; then
				fatal_error ${E_MOUNT_FULL_PATH}
			fi
		fi
		;;
	esac
}

# check_remote_filesys()
# Purpose:	check syntax of remote filesys keyword in profile
# Inputs:	$* are all the arguments on the remote filesys line
#
# 	filesys <remote> <ip_addr>|"-" [ <mount> ] [ <mntopts> ]
#		 ${1}       ${2}       [   ${3}  ] [   ${4}    ]
#
check_remote_filesys()
{
	# check <ip_addr> syntax
	# valid values:		-
	#			#.#.#.#
	if [ "${2}"X != "-"X ]; then
		cnt=`expr ${2} : '[0-9][0-9]*[.][0-9][0-9]*[.][0-9][0-9]*[.][0-9][0-9]*'`
		if [ ${cnt} -eq 0 ]; then 
			fatal_error ${E_INV_SERVER_ADDR}
		fi

	fi

	# check <mount> syntax
	# valid values:		<pathname>
	mount=`echo ${3} | tr '[A-Z]' '[a-z]'`	# lowercase it
	first_let=`expr "${3}"X : '\(.\)[.]*'`
	if [ "${first_let}" != "/" ]; then
		fatal_error ${E_MOUNT_FULL_PATH}
	fi
}

# check_type()
# Purpose:	make sure the keyword is appropriate for the install_type
#		specified (fatal exit with error message if not)
# Input:	${1}	- 'I' or 'U' specifiying if the command is install
#			  or upgrade specific
check_type()
{
	if [ "${INSTALL}"X = "INITIAL_INSTALLX" ]; then
		if [ "${1}" = "U" ]; then
			fatal_error ${E_UPGRADE_KEY}
		fi
	elif [ "${INSTALL}"X = "UPGRADEX" ]; then
		if [ "${1}" = "I" ]; then
			fatal_error ${E_INITIAL_KEY}
		fi
	fi
}

# check_profile_file()
# Purpose:	syntax check the profile
# Input:	${1}	- input file name
#
check_profile_file() 
{
	profile_ln=0
	INSTALL=""
	SYSTEM=""
	PRODUCT=""
	PARTITION=""
	META=""
	while read first rest ; do 
	      	profile_ln=`expr ${profile_ln} + 1`
		case ${first} in 
		\#*)	# comment 
			continue 
			;;
		"")	# blank line
			continue
			;;
		esac
	
		# take out comments at end of line, and white space before
		# comment.  this does have the side effect of not allowing
		# any '#' to be in the grammar, but that is acceptable.

		tmp_rest=`echo ${rest} | sed 's/[	 ]*\#.*$//'`
		if [ -z "${tmp_rest}" ]; then	
			set -- ""
			shift
		else	
			set -- ${tmp_rest}
		fi

		# set current line being evaluated for error reporting
		line="${first} ${tmp_rest}"
		keyword=`echo ${first} | tr '[a-z]' '[A-Z]'`  # uppercase it

		# process the keyword
		case ${keyword} in
		    BACKUP_MEDIA)
				# validate argument count
				if [ $# -gt 2 ]; then
					fatal_error ${E_EXCESS_ARGS}
				elif [ $# -lt 2 ]; then
					fatal_error ${E_LACK_ARGS}
				fi
				# uppercase the identifier
				IDENTIFIER=`echo ${1} | tr '[a-z]' '[A-Z]'`
				case ${IDENTIFIER} in
				    LOCAL_TAPE | LOCAL_DISKETTE | \
				    LOCAL_FILESYSTEM | REMOTE_SYSTEM | \
				    REMOTE_FILESYSTEM)
				    	;;
				    *)
				    	fatal_error ${E_INV_BACKUP_IDENT}
					;;
				esac
				check_type "U"
				;;
		    LAYOUT_CONSTRAINT)
				# validate argument count
				if [ $# -lt 2 ]; then
					fatal_error ${E_LACK_ARGS}
				fi
				# uppercase the identifier
				IDENTIFIER=`echo ${2} | tr '[a-z]' '[A-Z]'`
				case ${IDENTIFIER} in
				    MOVABLE | AVAILABLE | COLLAPSE)
					if [ $# -gt 2 ]; then
						fatal_error ${E_EXCESS_ARGS}
					fi
					;;
				    CHANGEABLE)
				    	if [ $# -eq 3 ]; then
					    is_num ${3}
					    if [ $? -eq 0 ]; then
						fatal_error ${E_INVALID_NUMBER}
					    fi
					elif [ $# -gt 3 ]; then
						fatal_error ${E_EXCESS_ARGS}
				    	fi
					;;
				    *)
				    	fatal_error ${E_INV_LAYOUT_CONSTRAINT}
					;;
				esac
				is_slice_name ${1}
				check_type "U"
				;;
		    CLIENT_ARCH)
				# validate argument count
				if [ $# -gt 1 ]; then
					warning_msg ${E_EXCESS_ARGS}
				elif [ $# -lt 1 ]; then
					fatal_error ${E_LACK_ARGS}
				fi
				check_type "I"
				;;
		    CLIENT_SWAP)
				# validate argument count
				if [ $# -gt 1 ]; then
					warning_msg ${E_EXCESS_ARGS}
				elif [ $# -lt 1 ]; then
					fatal_error ${E_LACK_ARGS}
				fi
				is_num ${1}
				if [ $? -eq 0 ]; then
					fatal_error ${E_INVALID_NUMBER}
				fi
				check_type "I"
				;;
		    CLIENT_ROOT)
				# validate argument count
				if [ $# -gt 1 ]; then
					warning_msg ${E_EXCESS_ARGS}
				elif [ $# -lt 1 ]; then
					fatal_error ${E_LACK_ARGS}
				fi
				is_num ${1}
				if [ $? -eq 0 ]; then
					fatal_error ${E_INVALID_NUMBER}
				fi
				check_type "I"
				;;
		    CLUSTER)
				# validate argument count
				if [ $# -gt 2 ]; then
					warning_msg ${E_EXCESS_ARGS}
				elif [ $# -lt 1 ]; then
					fatal_error ${E_LACK_ARGS}
				elif [ $# -eq 2 ]; then
					# uppercase the action field
					ACTION=`echo ${2} | tr '[a-z]' '[A-Z]'`
					if [ "${ACTION}" != "ADD" -a \
				    		    "${ACTION}" != "DELETE" ]; then
						fatal_error ${E_INV_OPTION}
						echo $?
					fi
				fi
				;;
		    DONTUSE)
				# validate argument count
				if [ $# -gt 1 ]; then
					warning_msg ${E_EXCESS_ARGS}
				elif [ $# -lt 1 ]; then
					fatal_error ${E_LACK_ARGS}
				fi
				is_dev_name ${1}
				check_type "I"
				;;
		    FDISK)
				# validate arguments
				if [ $# -gt 3 ]; then
					warning_msg ${E_EXCESS_ARGS}
				elif [ $# -lt 3 ]; then
					fatal_error ${E_LACK_ARGS}
				fi
				check_fdisk $*
				check_type "I"
				;;
		    FILESYS)
				# validate arguments
				if [ $# -lt 1 ]; then
					fatal_error ${E_LACK_ARGS}
				fi
				cnt=`expr ${1} : '..*:/..*'`
				if [ ${cnt} -gt 0 ]; then 
					if [ $# -gt 4 ]; then
						fatal_error ${E_EXCESS_ARGS}
					elif [ $# -lt 3 ]; then
						fatal_error ${E_LACK_ARGS}
					fi
					check_remote_filesys $*
				else
					# validate argument count
					if [ $# -gt 5 ]; then
						fatal_error ${E_EXCESS_ARGS}
					elif [ $# -lt 2 ]; then
						fatal_error ${E_LACK_ARGS}
					fi
					check_local_filesys $*
				fi
				check_type "I"
				;;
		    INSTALL_TYPE)
				# validate argument count
				if [ $# -gt 1 ]; then
					warning_msg ${E_EXCESS_ARGS}
				elif [ $# -lt 1 ]; then
					fatal_error ${E_LACK_ARGS}
				fi
				# check for duplicate specification
				if [ "${INSTALL}X" != "X" ]; then
					fatal_error ${E_DUP_INST_TYPE}
				fi
				# check for valid value
				INSTALL=`echo ${1} | tr '[a-z]' '[A-Z]'`
				if [ "${INSTALL}" != "INITIAL_INSTALL" -a \
					    "${INSTALL}" != "UPGRADE" ]; then
					fatal_error ${E_INV_INST_TYPE}
				fi
				;;
		    LOCALE)
				# validate argument count
				if [ $# -gt 1 ]; then
					warning_msg ${E_EXCESS_ARGS}
				elif [ $# -lt 1 ]; then
					fatal_error ${E_LACK_ARGS}
				fi
				;;
		    NOREBOOT)
				# validate argument count
				if [ $# -gt 0 ]; then
					warning_msg ${E_EXCESS_ARGS}
				fi
				check_type "I"
				;;
		    NUM_CLIENTS)
				# validate argument count
				if [ $# -gt 1 ]; then
					warning_msg ${E_EXCESS_ARGS}
				elif [ $# -lt 1 ]; then
					fatal_error ${E_LACK_ARGS}
				fi
				is_num ${1}
				if [ $? -eq 0 ]; then
					fatal_error ${E_INVALID_NUMBER}
				fi
				check_type "I"
				;;
		    PACKAGE)
				# validate argument count
				if [ $# -gt 2 ]; then
					warning_msg ${E_EXCESS_ARGS}
				elif [ $# -lt 1 ]; then
					fatal_error ${E_LACK_ARGS}
				elif [ $# -eq 2 ]; then
					# uppercase the action field
					ACTION=`echo ${2} | tr '[a-z]' '[A-Z]'`
					if [ "${ACTION}" != "ADD" -a \
				    		    "${ACTION}" != "DELETE" ]; then
						fatal_error ${E_INV_OPTION}
					fi
				fi
				;;
		    PARTITIONING)
				# validate argument count
				if [ $# -gt 1 ]; then
					warning_msg ${E_EXCESS_ARGS}
				elif [ $# -lt 1 ]; then
					fatal_error ${E_LACK_ARGS}
				fi
				# check for valid value
				PARTITION=`echo ${1} | tr '[a-z]' '[A-Z]'`
				if [ "${PARTITION}" != "DEFAULT" -a \
					    "${PARTITION}" != "EXISTING" -a \
					    "${PARTITION}" != "EXPLICIT" ]; then
					fatal_error ${E_INV_PART_TYPE}
				fi
				check_type "I"
				;;
		    SWAP_SIZE)
				# validate argument count
				if [ $# -gt 1 ]; then
					warning_msg ${E_EXCESS_ARGS}
				elif [ $# -lt 1 ]; then
					fatal_error ${E_LACK_ARGS}
				fi
				is_nums ${1}
				if [ $? -eq 0 ]; then
					fatal_error ${E_BAD_SIZE}
				fi	
				check_type "I"
				;;
		    SYSTEM_TYPE)
				# validate argument count
				if [ $# -gt 1 ]; then
					warning_msg ${E_EXCESS_ARGS}
				elif [ $# -lt 1 ]; then
					fatal_error ${E_LACK_ARGS}
				fi
				# check for valid value
				SYSTEM=`echo ${1} | tr '[a-z]' '[A-Z]'`
				if [ "${SYSTEM}" = "DATALESS" ]; then
					warning_msg ${E_DATALESS_EOL};
				elif [ "${SYSTEM}" != "STANDALONE" -a \
					    "${SYSTEM}" != "SERVER" ]; then
					fatal_error ${E_INV_SYS_TYPE}
				fi
				check_type "I"
				;;
		    USEDISK)
				# validate argument count
				if [ $# -gt 1 ]; then
					warning_msg ${E_EXCESS_ARGS}
				elif [ $# -lt 1 ]; then
					fatal_error ${E_LACK_ARGS}
				fi
				is_dev_name ${1}
				check_type "I"
				;;
		ROOT_DEVICE)
				# validate argument count
				if [ $# -gt 1 ]; then
					warning_msg ${E_EXCESS_ARGS}
				elif [ $# -lt 1 ]; then
					fatal_error ${E_LACK_ARGS}
				fi
				# check for duplicate root_device
				if [ "${ROOTDEV}X" != "X" ]; then
					fatal_error ${E_DUP_ROOT_DEVICE}
				fi
				is_slice_name ${1}
				ROOTDEV=${1}
				;;
		BOOT_DEVICE)
				# validate argument count
				if [ $# -gt 2 ]; then
					warning_msg ${E_EXCESS_ARGS}
				elif [ $# -lt 2 ]; then
					fatal_error ${E_LACK_ARGS}
				fi
				# check for duplicate boot_device
				if [ "${BOOTDEV}X" != "X" ]; then
					fatal_error ${E_DUP_BOOT_DEVICE}
				fi
				check_boot_device $*
				check_type "I"
				BOOTDEV=${1}
				;;
		    *)
				# default
				fatal_error ${E_INVALID_KEY}
				;;
		esac
		# check for install_type first
		if [ "${INSTALL}X" = "X" ]; then
			fatal_error ${E_INST_TYPE_FIRST}
		fi
	done 

	# fix the variable line up for any possible warning messages.
	line="profile ${profile}"

	if [ "${INSTALL}X" = "X" ];then
		fatal_error ${E_NO_INST_TYPE}
	fi

	profile_ln=0
}	

# check_rule_file()
# Purpose:	check syntax of input file (rules) and create derived
#		file
# Input:	${1} -	input file name
#
check_rule_file() 
{
	# remove any previously existing profile list file
	echo > ${PROF_TMP_FILE}

	# read each rule one at a time, since we don't have to worry about 
	# being in a subshell of the main shell here.
	in_file="${1}"
	ln=0
	while read first rest; do 
	      	ln=`expr ${ln} + 1`
		case ${first} in 
		\#*)	# comment 
			continue 
			;;
		"")	# blank line
			continue
			;;
		esac
	
		# take out comments at end of line this does have the side
		# effect of not allowing any '#' to be in the grammar, but
		# that is acceptable.
	
		rule=`echo ${first} ${rest} | sed 's/\#.*$//'`

		# now, for each rule that we have to parse, break it up into
		# individual match and mulitple match_key arguments.
		
		set -- ${rule}
		line="${rule}"
		
		# we need at least 2 args on a probe line...
		if [ $# -lt 2 ]; then
			fatal_error ${E_LACK_ARGS}
		fi
	
		# if probe function, then run it 
		if [ ${1}"X" = "probeX" ]; then
			probe_name=${2}

			# find the probe function in ${CHECK_PROBE} or
			# ${CUSTOM_PROBES} we can't force the { to be on the
			# same line, unfortunately 
	
			grep "probe_${probe_name}[ 	]*()" ${CHECK_PROBE} \
								>/dev/null
			rc1=$?
			# if we found it alread, don't look anymore
			if [ $rc1 -ne 0 ]; then
				if [ "${CUSTOM_PROBES}" ]; then 
					grep "probe_${probe_name}[ 	]*()" \
						${CUSTOM_PROBES} >/dev/null 2>&1
					rc2=$?
				else
					rc2=1
				fi
			else
				rc2=0
			fi
			if [ $rc1 -ne 0 -a $rc2 -ne 0 ]; then
				fatal_error ${E_PROBE_NOT_FOUND}
			fi

			continue
		fi
	
		if [ $# -lt 2 ]; then
			fatal_error ${E_LACK_ARGS}
		fi
	
		# not a probe...so it must be a compare.
		# grab the begin, profile, and end scripts/profiles
		eval `echo ${rule} | \
			awk '{printf( "begin=%s; profile=%s; finish=%s", \
				   $(NF - 2), $(NF - 1), $NF)}'`
		
		if [ ${DEBUG} -eq 1 ]; then
			echo begin=${begin} profile=${profile} finish=${finish}
		fi
		
		# strip the line of the last three arguments...
		# ("begin", "profile", and "end").. to give only
		# match/match_key section of the rule
		matches=`echo ${rule} | \
			awk '{ for (i = 1; i < NF - 2; i++) 
					     printf("%s ",$i) }'`
		set -- ${matches}

		if [ $# -lt 2 ]; then
			cmp_err_rulename=${1}
			fatal_error ${E_CMP_LACK_ARGS}
		fi
	
		# split the matches into separate variables,
		# independant of # of args for easy parsing... ...
		# it might seem gorpy, but it simplifies the
		# handling the arguments and leaves the compare
		# routines free to do what they will with the
		# arguments.
		eval `echo ${matches} | awk -F'&&' ' BEGIN {count = 0}
			{ for (i = 1; i <= NF; i++) 
				if ($i != "") {
				       	count++
					cmd_var = var"_"count
				   	printf("%s=\"%s\";",cmd_var,$i) 
				}
			}
			END {printf("tot_cmps=%s", count)}' \
							var=${CMP_VAR}`
		
		
		# because of previous checks, we must have at least 1
		# compare by the time we get here  (good syntax is unknown yet)

		saverc=1	# start with no match
		cmp_num=0
	
		# make all the compare matches.
		# NOTE: this loop is running in a subshell, so make sure
		#	you check the exit status of this subshell (after
		#	the loop completes) to determine the exit status
		#	of the main shell
		while test ${cmp_num} -lt ${tot_cmps} ; do
			not_flag=0
			cmp_num=`expr ${cmp_num} + 1`
			comp=`eval echo $"${CMP_VAR}_${cmp_num}"`
			set -- ${comp}
	
			if [ "${1}"X = "!"X ]; then
				not_flag=1
				shift
			fi
		
			# check should have made sure there are at least 2
			# arguments. 
			set -- `convert_args $*`
			key=${1}
			first_arg=${2}
	
			# basic check for multi-arg rule cnt
			check_arg_cnt ${key} $#

			# run the comparison function; find the probe function
			# in ${CHECK_PROBE} can't force the { to be on the same
			# line, unfortunately
			grep "cmp_${key}[ 	]*()" ${CHECK_PROBE} > /dev/null
			rc1=$?

			# if we found it already, don't look anymore
			if [ $rc1 -ne 0 ]; then
				if [ "${CUSTOM_PROBES}" ]; then 
					grep "cmp_${key}[ 	]*()" \
						${CUSTOM_PROBES} > /dev/null
					rc2=$?
				else
					rc2=1
				fi
			else
				rc2=0
			fi
			if [ $rc1 -ne 0 -a $rc2 -ne 0 ]; then
				fatal_error ${E_CMP_NOT_FOUND}
			fi
		done
	
		# if we get here... then all we have left to check for is
		# the begin, profile, and finish scripts
		if [ "${begin}" != "-" -a ! -f "${begin}" ]; then
			fatal_error ${E_BEGIN_NOT_FOUND}
		fi

		# profile type
		case "${profile}" in
		"-")	# null
			;;
		"=")	# derived, must have begin
			if [ ! -f ${begin} ]; then
				fatal_error ${E_NO_BEGIN_TO_DERIVE}
			fi
			;;
		*)	# a file name
			if [ ! -f ${profile} ]; then
				fatal_error ${E_CLASS_NOT_FOUND}
			fi
			;;
		esac

		if [ "${finish}" != "-" -a ! -f ${finish} ]; then
			fatal_error ${E_FINISH_NOT_FOUND}
		fi

		# check input file using check_input file
		# a "-" means no profile, so don't check it
		if [ "${profile}" != "-" -a "${profile}" != "=" ]; then
			# only check the profile if it hasn't been
			# checked before; look for an exact match
			exist=`egrep -c "^${profile}$" ${PROF_TMP_FILE}`
			if [ ${exist} -eq 0 ]; then
				echo "Validating profile ${profile}..."
				echo ${profile} >> ${PROF_TMP_FILE}
				profile_file="${profile}"
				export profile_file
				check_profile_file < ${profile}
			fi
		fi
	done < ${in_file}

	estatus=$?
	rm -f ${PROF_TMP_FILE}
	if [ ${estatus} -ne 0 ]; then
		exit 1
	fi
}
	
usage()
{
	echo "Usage: ${myname} [-r <rules filename>] [-p <Solaris 2.x CD image path>]"
	exit 1
}
		
###################
# MAIN 
###################

CHECK_PROBE=/usr/sbin/install.d/chkprobe

#
# Parse command line options. All options must be by a seperated by a space
#
while [ "${1}"x != "x" ] ; do
	case ${1} in
	-r)
		# explicit rules file specified 
		if [ ! "${2}" ]; then
			usage
		fi
		RULES=${2}
		if [ ! -f ${RULES} ]; then
			echo "No rules file \"${RULES}\" found"
			exit 1
		fi
		shift 2
		;;
	-p) 
		if [ ! "${2}" ]; then
			usage
		fi
		CDROM=${2}
		i_dir=`echo ${CDROM}/Solaris*/Tools/Boot/usr/sbin/install.d`
		if [ ! -f ${i_dir}/chkprobe ]; then
			echo "ERROR: ${2} is not a valid Solaris 2.x CD image"
			exit 1
		fi
		PATH=${i_dir}:${PATH}
		export PATH
		CHECK_PROBE=${i_dir}/chkprobe
		shift 2
		;;
	*) 
		# -anything else is an unknown argument
	    	usage
		;;
	esac
done

# see if we are trying a different rules file
if [ ! "${RULES}" ]; then
	RULES=${STD_RULES}
fi

# don't define CUSTOM_PROBES, if none exists
if [ ! -f "${CUSTOM_PROBES}" ]; then
	CUSTOM_PROBES=""
fi

if [ ! -x "${CHECK_PROBE}" ]; then
	echo "ERROR: Unable to execute /usr/sbin/chkprobe"
	usage
fi

egrep -s probe_installed ${CHECK_PROBE}
if [ $? -ne 0 ]; then
	echo "ERROR: The path to the Solaris CD image is required"
	echo "Use the -p option."
	usage
fi

# check rule file using check_rule_file
echo "Validating ${RULES}..."
check_rule_file ${RULES}
if [ $? -ne 0 ]; then 
	exit 1
fi

if [ "${RULES}" = "${STD_RULES}" ]; then
	create_derived_file ${RULES} ${RULES}.ok
	append_checksum ${RULES}.ok
	[ $? -ne 0 ] && exit 1

	if [ "${CUSTOM_PROBES}" ]; then
		create_derived_file ${CUSTOM_PROBES} ${CUSTOM_PROBES}.ok
		append_checksum ${CUSTOM_PROBES}.ok
	fi
	[ $? -ne 0 ] && exit 1
else
	# only create a .ok file when checking the standard rules file
	echo "${RULES}.ok file not created"
fi

rm -f /tmp/*.$$		# get rid of tmp files....

echo "The custom JumpStart configuration is ok."
exit 0
