#!/bin/sh
#
#	@(#)parse_dynamic_clustertoc.sh	1.2	94/08/12 SMI"
#
# parse dynamic clustertoc
#	invoke from the end of <cd>/sbin/sysconfig BEFORE running suninstall
#

# PKGS is where the installable pkgs live
# ASSUME: running off CDROM, only one version of Solaris packages per CD
#PKGS=`echo /cdrom/Solaris*`
prod_line=`cat /cdrom/.cdtoc |grep PRODDIR | grep Solaris`
pkg_dir=`expr ${prod_line} : 'PRODDIR=\(.*\)'`
PKGS=/cdrom/${pkg_dir}

# TMPDIR is the writable place to put the parsed clustertoc
TMPDIR=/tmp/clustertocs/locale/C

# DEFAULT is the default .clustertoc
DEFAULT="${PKGS}/locale/C/.clustertoc.default"

# DYNAMIC is the dynamic .clustertoc
DYNAMIC="${PKGS}/locale/C/.clustertoc.dynamic"

# TESTDIR is where to find the tests that are programs
#TESTDIR="${PKGS}/locale/C"
TESTDIR="/usr/sbin/install.d/dynamic_test"

#
# process_it() - processes a SUNW_CSRMBRIFF line
# INPUT: $1 is the line
# OUTPUT: stdout, if test matches then SUNW_CSRMEMBER entry, else null
#	stderr: if unknown test, print a warning.
#
process_it () {

	thetest=`expr "$1" : 'SUNW_CSRMBRIFF=(\(.*\)[ ].*'`
	thearg=`expr "$1" : 'SUNW_CSRMBRIFF=(.*[ ][ ]*\(.*\)).*'`
	thething=`expr "$1" : 'SUNW_CSRMBRIFF=(.*)\(.*\)'`

	# if the test is a builtin, do it then return
	case ${thetest} in
	platform)
		# a builtin - platform
		if [ "${SI_MODEL}" = "${thearg}" ]; then
			echo "SUNW_CSRMEMBER=${thething}"
		fi
		return
		;;
	# locale and other builtins should go here
	esac

	# if we got here, must try an external program
	if [ ! -x "${TESTDIR}/${thetest}" ]; then
		echo "parse dynamic clustertoc: \"${thetest}\" not found or not executable" >> /dev/stderr
		return
	fi
	${TESTDIR}/${thetest} "${thearg}" 1>> /dev/stderr 2>&1
	if [ $? -eq 0 ]; then
		echo "SUNW_CSRMEMBER=${thething}"
	fi
}

# get the platform specific identifier
SI_MODEL=`prtconf | awk ' {
		if (state == 1 && $0 != "") {
			print $0
			exit
		}
		if (substr($0, 1, 18) == "System Peripherals")
			state=1
	}'`

# substitute all spaces and tabs with "_"
SI_MODEL=`echo ${SI_MODEL} | sed 's/ /_/g' | sed 's/	/_/g' | sed "s/\'//g"`

# if we cannot get the dynamic clustertoc, then punt
if [ ! -f "${DYNAMIC}" ]; then
	# use the .default if possible
	if [ -f ${DEFAULT} ]; then
		mkdir -p ${TMPDIR}
		ln -s "${DEFAULT}" "${TMPDIR}/.clustertoc"
	fi
	exit 0
fi
		
mkdir -p ${TMPDIR}
rm -f ${TMPDIR}/.clustertoc

while read aline ; do
	# if anything other than "SUNW_CSRMBRIFF", pass thru
	case "${aline}" in
	SUNW_CSRMBRIFF=*)
		# process it, assume it to be syntactically correct
		output=`process_it "${aline}"`
		if [ "${output}" ]; then
			echo "${output}" >> "${TMPDIR}/.clustertoc"
		fi
		;;
	*)
		echo "${aline}" >> "${TMPDIR}/.clustertoc"
	esac
done < ${DYNAMIC}

exit 0
