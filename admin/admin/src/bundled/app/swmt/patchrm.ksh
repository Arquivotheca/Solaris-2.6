#!/bin/ksh -h
# @(#) patchrm.ksh 1.3 96/10/15 SMI
#
# Exit Codes:
#		0	No error
#		1	Usage error
#		2	Attempt to backout a patch that hasn't been applied
#		3	Effective UID is not root
#		4	No saved files to restore
#		5	pkgrm failed
#		6	Attempt to back out an obsoleted patch
#		7	Attempt to restore CPIO archived files failed
#		8	Invalid patch id format
#		9	Prebackout script failed
#		10	Postbackout script failed
#		11	Suspended due to administrative defaults
#		12	Backoutpatch could not locate the backout data
#       13	The relative directory supplied can't be found
#       14	Installpatch has been interrupted, re-invoke installpatch 
#       15	This patch is required by a patch already installed, can't back it out
#

# Set up the path to use with this script.

PATH=/usr/sadm/bin:/usr/sbin:/usr/bin:$PATH
export PATH

umask 007

# Global Files

TMPSOFT=/tmp/soft.$$
ADMINFILE=/tmp/admin.$$
LOGFILE=/tmp/backoutlog.$$
RESPONSE_FILE=/tmp/response.$$
TEMP_REMOTE=/tmp/temp_remote.$$
RECOVERDIR=/var/sadm/.patchRec

force=no
pkginstlist=
pkglist=
ret=
curdir=
diPatch="no"
ObsoletedBy="none"
ThisPatchFnd="no"
PatchedPkgs=""
InstPkgs=""
RebootRqd="no"
netImage="none"

ROOTDIR="/"
PATCHDB="/var/sadm/patch"
PATCH_UNDO_ARCHIVE="none"
OBS_PATCH_UNDO_ARCHIVE="none"
TEMP_PATCH_UNDO_ARCHIVE="none"
PKGDB="/var/sadm/pkg"
SOFTINFO="/var/sadm/softinfo"
NEW_SOFTINFO="/var/sadm/system/admin/INST_RELEASE"
OLD_SOFTINFO="/var/sadm/softinfo/INST_RELEASE"
MGRSOFTINFO="none"
TRGSOFTINFO="none"
CONTENTS="/var/sadm/install/contents"
TMP_LIB_DIR="/tmp/TmpLibDir.$$"
PKGDBARG=""
PATCH_PID=""
DASHB_SUPPLIED="no"

# Needed utilities
DF=/usr/sbin/df
RM=/usr/bin/rm
MV=/usr/bin/mv
SED=/usr/bin/sed
AWK=/usr/bin/awk
NAWK=/usr/bin/nawk
EGREP=/usr/bin/egrep
GREP=/usr/bin/grep
CP=/usr/bin/cp
FIND=/usr/bin/find
UNAME=/usr/bin/uname

PatchIdFormat='^[A-Z]*[0-9][0-9][0-9][0-9][0-9][0-9]-[0-9][0-9]$'

#
# Description:
#	Execute the prebackout script if it is executable. Fail if the
#	return code is not 0.
#
# Parameters:
#	$1	- package database directory
#	$2	- patch number
# Globals Set:
#	none
function execute_prebackout
{
	typeset -i retcode=0
	if [ -x $1/$2/prebackout ]
	then
		/usr/bin/gettext "Executing prebackout script...\n"
		$1/$2/prebackout
		retcode=$?
		if (( retcode != 0 ))
		then
			/usr/bin/gettext "prebackout script exited with return code $retcode.\n"
			/usr/bin/gettext "Backoutpatch is exiting.\n\n"
			exit 9
		fi
	fi
}

#
# Description:
#   Check to see if installpatch was interrupted, prompt
#	usr to reinvoke installpatch. 
#
# Globals Set:
#   RECOVERDIR

function check_file_recovery
{
	if [[ -f "$RECOVERDIR/.$PatchNum" ]]
	then
		/usr/bin/gettext "The installation of patch $PatchNum was interrupted.\nInstallpatch needs to be re-invoked to ensure proper installation of the patch.\n"
		patch_quit 14
	fi
}

#
# Description:
#	Execute the postbackout script if it is executable. Fail if the
#	return code is not 0.
#
# Parameters:
#	$1	- package database directory
#	$2	- patch number
# Globals Set:
#	none
function execute_postbackout
{
	typeset -i retcode=0
	if [ -x $1/$2/postbackout ]
	then
		/usr/bin/gettext "Executing postbackout script...\n"
		$1/$2/postbackout
		retcode=$?
		if (( retcode != 0 ))
		then
			/usr/bin/gettext "postbackout script exited with return code $retcode.\nBackoutpatch exiting.\n\n"
			exit 10
		fi
	fi
}

# Quit patchrm and clean up any remaining temporary files.
function patch_quit {   # exit code
        if [[ $1 -ne 0 ]]
        then
                /usr/bin/gettext "\nBackoutpatch is terminating.\n"
        fi

        exit $1
}

#
# Description:
#	Return the base code of the provided patch. The base code
#	returned will include the version prefix token (usu "-").
#
# Parameters Used:
#	$1	- patch number
#
function get_base_code {
	ret_value=${1:%[0-9]}
	last_value=$1

	while [[ $ret_value != $last_value ]]
	do
		last_value=$ret_value
		ret_value=${last_value%[0-9]}
	done

	cur_base_code=${ret_value%?}
}

#
# Description:
#	Return the version number of the provided patch.
#
# Parameters Used:
#	$1	- patch number
#	$2	- base code
#
function get_vers_no {
	cur_vers_no=${1:#$2?}
}

#
# Description:
#	Give a list of applied patches similar in format to the showrev -p
#	command. Had to write my own because the showrev command won't take
#	a -R option.
#
# Parameters:
#	$1	- package database directory
#
# Globals used
#	PatchNum
#
# Globals Set:
#	diPatch
#	ObsoletedBy
#	ThisPatchFnd
#	PatchedPkgs
#
# Revision History
#	1995-08-01	Added PATCH_OBSOLETES and expanded the tests for
#			direct instance patches since all necessary
#			is reviewed at this time, this function also
#			tests for obsolescence, dependencies and
#			incompatibilities.
#
function eval_inst_patches
{
	typeset -i PatchFound=0
	typeset -i ArrayCount=0

	set -A PkgArrElem
	set -A PatchArrElem
	set -A ObsArrElem

	olddir=$(pwd)

	#
	# First get the old-style patches and obsoletions
	#
	if [ -d $1 -a -d $PATCHDB ]
	then
		cd $1
		patches=
		patches=$(grep -l SUNW_PATCHID ./*/pkginfo | \
		    xargs $SED -n 's/^SUNW_PATCHID=//p' | sort -u)

		if [ "$patches" != "" ]
		then
			for apatch in $patches
			do
				outstr="Patch: $apatch Obsoletes: "

				# Scan all the installed packages for this
				# patch number and return the effected
				# package instances
				patchvers=$(grep -l "SUNW_PATCHID=$apatch" \
				    ./*/pkginfo | $SED 's,^./\(.*\)/pkginfo$,\1,' )

				# If there's a PATCH_INFO entry then this
				# is really a direct instance patch
				for package in $patchvers
				do
					break;
				done

				$(grep "PATCH_INFO_$apatch" $package/pkginfo 1>/dev/null 2>&1)
				if [[ $? -eq 0 ]]
				then
					continue
				fi

				PatchFound=1

				obsoletes_printed="n"
				for vers in $patchvers
				do
					if [ "$obsoletes_printed" = "n" ]
					then
						outstr="$outstr$($SED -n \
						    's/SUNW_OBSOLETES=//p' \
						    ./$vers/pkginfo) Packages: "
						outstr="$outstr$vers $($SED -n \
						    's/VERSION=//p' \
						    ./$vers/pkginfo)"
						obsoletes_printed="y"
					else
						outstr="$outstr, $vers $($SED \
						    -n 's/VERSION=//p' \
						    ./$vers/pkginfo)"
					fi
				done

				# The current patch is a progressive
				# instance patch
				if [[ $apatch = "$PatchNum" ]]
				then
					diPatch="no"
					ThisPatchFnd="yes"
				fi
			done
		fi
	fi

	#
	# Now get the direct instance patches
	#
	# DIPatches is a non-repeating list of all patches applied
	# to the system.
	#
	typeset -i TempCount=0

	InstPkgs=$(pkginfo -R $ROOTDIR | $NAWK ' { print $2; } ')

	for package in $InstPkgs
	do
		DIPatches=$(pkgparam -R $ROOTDIR $package PATCHLIST)
		for patch in $DIPatches
		do
			Obsoletes=$(pkgparam -R $ROOTDIR $package PATCH_INFO_$patch | \
			  $NAWK ' { print substr($0, match($0, "Obsoletes:")+11) } ' | \
			  $SED 's/Requires:.*//g')

			if [ -n "$Obsoletes" ]
			then
				PatchArrElem[$ArrayCount]=$patch;
				ObsArrElem[$ArrayCount]="$Obsoletes";
				PkgArrElem[$ArrayCount]=$package;
			else
				PatchArrElem[$ArrayCount]=$patch;
				PkgArrElem[$ArrayCount]=$package;
				ObsArrElem[$ArrayCount]="";
			fi
			ArrayCount=ArrayCount+1;

			# The current patch is a direct instance patch
			if [[ "$patch" = "$PatchNum" ]]
			then
				diPatch="yes"
				ThisPatchFnd="yes"
			fi

			# Is this patch obsoleted according to a pkginfo
			# file
			for obs_entry in "$Obsoletes"
			do
				if [[ "$obs_entry" = "$PatchNum" ]]
				then
					ObsoletedBy=$patch
				fi
			done
		done
	done

	typeset -i TestCount=0
	while [[ $TestCount -lt $ArrayCount ]]
	do
		typeset -i TempCount=TestCount+1

		# Scan all entries matching the current one
		PatchArrEntry=${PatchArrElem[$TestCount]}	# Current one
		ObsArrEntry=${ObsArrElem[$TestCount]}
		PkgArrEntry=${PkgArrElem[$TestCount]}

		if [[ "$PatchArrEntry" = "used" ]]
		then
			TestCount=TestCount+1
			continue
		fi

		while [[ $TempCount -lt $ArrayCount ]]
		do
			#
			# If this is another line describing this patch
			#
			if [[ ${PatchArrElem[$TempCount]} = $PatchArrEntry ]]
			then
				dont_use=0;

				PatchArrElem[$TempCount]="used"
				for pkg in $PkgArrEntry
				do
					if [[ $pkg = ${PkgArrElem[$TempCount]} ]]
					then
						dont_use=1;
						break;
					fi

				done

				if (( dont_use == 0 ))
				then
					PkgArrEntry="$PkgArrEntry ${PkgArrElem[$TempCount]}"
				fi

				dont_use=0;

				for obs in $ObsArrEntry
				do
					if [[ $obs = ${ObsArrElem[$TempCount]} ]]
					then
						dont_use=1;
						break;
					fi

				done

				if (( dont_use == 0 ))
				then
					ObsArrEntry="$ObsArrEntry ${ObsArrElem[$TempCount]}"
				fi
			fi
			TempCount=TempCount+1
		done

		if [[ $PatchArrEntry = "$PatchNum" ]]
		then
			export PatchedPkgs="$PkgArrEntry"
			export PatchNum="$PatchNum"
			export ROOTDIR="$ROOTDIR"
		fi

		# Now make it comma separated lists
		PkgArrEntry=$(echo $PkgArrEntry | $SED s/\ /,\ /g)
		ObsArrEntry=$(echo $ObsArrEntry | $SED s/\ /,\ /g)

		TestCount=TestCount+1
	done

	cd $olddir
}

# Description:
#	Print out the usage message to the screen
# Parameters:
#	none

function print_usage
{
cat<<EOF

   Usage: patchrm [-f] [-B backout_dir] [-R <client_root_path> | -S <service>]
            [-C <net_install_image>] <patchid>

EOF

# This line gets inserted after Usage message.
# [-C <net_install_image>]
}

# Description:
#	Patch obsolecense message, printed if the patch being backed
#	out was superceded by other patches 
# Parameters:
#	$1	- patch ID
#	$2	- patch revision number
#
function print_obsolete_msg
{
	outstr="This patch was obsoleted by patch $1"
	if [[ "$2" = "none" ]]
	then
		outstr="$outstr."
	else
		outstr="$outstr-$2."
	fi
	/usr/bin/gettext "$outstr\n\nPatches must be backed out in the reverse order in\nwhich they were installed.\n\nBackoutpatch exiting.\n\n"
}

# Description:
#       Find the appropriate softinfo files for the manager and the target.
# Parameters:
#       $1      ROOT of target filesystem
# Globals set:
#       TRGSOFTINFO
#       MGRSOFTINFO
# Globals used:
#       OLD_SOFTINFO
#       NEW_SOFTINFO
function find_softinfos
{
	if [[ "$netImage" = "boot" ]]
	then
		return
	fi

	if [[ -f $NEW_SOFTINFO ]]
	then
		MGRSOFTINFO=$NEW_SOFTINFO
	elif [[ -f $OLD_SOFTINFO ]]
	then
		MGRSOFTINFO=$OLD_SOFTINFO
	fi

	if [[ "$1" = "/" || "$1" = "" ]]
	then
		TRGSOFTINFO=MGRSOFTINFO
	elif [[ -f $1$NEW_SOFTINFO ]]
	then
		TRGSOFTINFO=$1$NEW_SOFTINFO
	elif [[ -f $1$OLD_SOFTINFO ]]
	then
		TRGSOFTINFO=$1$OLD_SOFTINFO
	fi
}

# Description:
#	Parse the arguments and set all affected global variables
# Parameters:
#	Arguments to patchrm
# Globals Set:
#	force
#	PatchNum
#	ROOTDIR
#	PATCHDB
#	PKGDB
#	PKGDBARG
#	CONTENTS
# Globals used:
#	Mgrprodver
#	MGRSOFTINO
#	TRGSOFTINFO
#

function parse_args
{
	# Inserted for readability reasons
	echo ""
	service_specified="n"
	rootdir_specified="n"
	origdir=$(pwd)
	while [[ "$1" != "" ]]
	do
		case $1 in
		-f)	force="yes"
			shift;;
		-B)	shift
			if [[ -d $1 ]]
			then
				determine_directory $1
                                if [[ $ret = 0 ]]
                                then
                                        PATCH_UNDO_ARCHIVE=$1
                                else
                                        PATCH_UNDO_ARCHIVE=$curdir
                                fi
				DASHB_SUPPLIED="yes"
				TEMP_PATCH_UNDO_ARCHIVE=$PATCH_UNDO_ARCHIVE
			else
                                /usr/bin/gettext "Specified backout directory $1 cannot be found.\n"
				exit 1
			fi
			shift;;
                -V) echo "@(#) patchrm.ksh 1.3 96/10/15"
			exit 0
			shift;;
		-S)	shift
			if [[ "$service_specified" != "n" ]]
			then
				/usr/bin/gettext "Only one service may be defined.\n"
				print_usage
				exit 1
			elif [[ "$rootdir_specified" != "n" ]]
			then
				/usr/bin/gettext "The -S and -R options are mutually exclusive.\n"
				print_usage
				exit 1
			fi
                        find_softinfos /export/$1

                        get_OS_version "$TRGSOFTINFO" "$MGRSOFTINFO" "$1"

                        if [ "$1" != "$Mgrprodver" ]
                        then
                                if [ -d "/export/$1$PKGDB" ]
                                then
                                        ROOTDIR=/export/$1
                                        PATCHDB=$ROOTDIR$PATCHDB
                                        PKGDB=$ROOTDIR$PKGDB
                                	SOFTINFO=$ROOTDIR$SOFTINFO
                                        PKGDBARG="-R $ROOTDIR"
                                	CONTENTS=$ROOTDIR$CONTENTS
                                        service_specified="y"
                                else
                                        /usr/bin/gettext "The $1 service cannot be found on this system.\n"
                                        print_usage
                                        patch_quit 1
                                fi
                        fi
			shift;;
		-R)	shift
			if [[ "$rootdir_specified" != "n" ]]
			then
				/usr/bin/gettext "Only one client may be defined.\n"
				print_usage
				exit 1
			elif [[ "$service_specified" != "n" ]]
			then
				/usr/bin/gettext "The -S and -R options are mutually exclusive.\n"
				print_usage
				exit 1
			fi
			if [[ -d "$1" ]]
			then
				determine_directory $1
			if [[ $ret = 0 ]]
			then
				ROOTDIR=$1
			else
				ROOTDIR=$curdir
			fi
				PATCHDB=$ROOTDIR/var/sadm/patch
				PKGDB=$ROOTDIR/var/sadm/pkg
				SOFTINFO=$ROOTDIR$SOFTINFO
				PKGDBARG="-R $ROOTDIR"
				CONTENTS=$ROOTDIR$CONTENTS
				rootdir_specified="y"
			else
				/usr/bin/gettext "The $1 directory cannot be found on this system.\n"
				print_usage
				exit 1
			fi
			shift;;
		-C) shift
			if [[ "$service_specified" = "y" || "$rootdir_specified" = "y" ]]
			then 
				/usr/bin/gettext "The -S, -R and -C arguments are mutually exclusive.\n"
				print_usage
				patch_quit 1
			fi 
			if [ ! -d "$1" ]
			then 
				/usr/bin/gettext "The path to the net install image $1 cannot be found on this disk.\n"
				print_usage
				patch_quit 1
			else 
				determine_directory $1
				if [[ $ret = 0 ]]
				then
					ROOTDIR=$1
				else
					ROOTDIR=$curdir
				fi
				PATCHDB=$ROOTDIR$PATCHDB
				PKGDB=$ROOTDIR$PKGDB
				PKGDBARG="-R $ROOTDIR"
				netImage="boot"
			fi
			shift;;

		-*)	print_usage
			exit 1;;
		 *)	break;;
		esac
	done
	PatchNum=$1
	#
	# If there is no patch number specified, exit with an error.
	#
	if [[ "$PatchNum" = "" ]]
	then
		/usr/bin/gettext "No patch number was specified.\n"
		print_usage
		exit 1
	fi
}

# Description:
# 	Derive the full path name from a (possibly) relative path name.
# Parameters:
#       $1      - command line argument
#
# Globals Used:
#	ret
#       curdir

function determine_directory
{
	$(valpath -a $1)
	ret=$?
	if [[ $ret != 0 ]]
	then
		cd $1 3>/dev/null
		if [[ $? = 0 ]]
		then
			curdir=$(pwd)
			cd $orig_dir
		else
			/usr/bin/gettext "Can not determine relative directory.\n"
			patch_quit 13
		fi
	else
		return
	fi
}

# Description:
#	Make sure the effective UID is '0'
# Parameters:
#	none
function validate_uid
{
	typeset -i uid
	uid=$(id | $SED 's/uid=\([0-9]*\)(.*/\1/')
	if (( uid != 0 ))
	then
		/usr/bin/gettext "You must be root to execute this script.\n"
		exit 3
	fi
}

# Description:
#       Get the product version <name>_<version> of local Solaris installation
# Parameters:
#       $1      target host softinfo directory path
#       $2      managing host softinfo directory path
#       $3      root of the target host
# Globals Set:
#       prodver
#
function get_OS_version
{
	# If this a patch to a net install image we don't care about
	# the managing and target host we know it will be a 2.6 or
	# beyond OS.
	if [[ "$netImage" = "boot" ]]
	then
		MgrProduct="Solaris"
		MgrOSVers="2.6"
		Mgrprodver=$MgrProduct"_"$MgrOSVers
		TrgOSVers=$MgrOSVers
		Product=$MgrProduct
		prodver=$Mgrprodver
		return
	fi

	if [[ "$2" != "none" ]]
	then
		MgrProduct=$($SED -n 's/^OS=\(.*\)/\1/p' $2)
		MgrOSVers=$($SED -n 's/^VERSION=\(.*\)/\1/p' $2)
		Mgrprodver=$MgrProduct"_"$MgrOSVers
	else
		MgrProduct="Solaris"
		MgrOSVers=$(uname -r | $SED -n -e 's/5\./2\./p' -e 's/4\./1\./p')
		Mgrprodver=$MgrProduct"_"$MgrOSVers
	fi

	if [[ $3 = "/" ]]       # If there's not a client
	then
		Product=$MgrProduct
		TrgOSVers=$MgrOSVers
		prodver=$Mgrprodver

	# OK, there is a client
	elif [[ "$1" = "none" ]]        # but no softinfo file
	then
		/usr/bin/gettext "patchrm is unable to find the INST_RELEASE file for the target\nfilesystem.  This file must be present for patchrm to function correctly.\n"
		patch_quit 11
	else
		Product=$($SED -n 's/^OS=\(.*\)/\1/p' $1)
		TrgOSVers=$($SED -n 's/^VERSION=\(.*\)/\1/p' $1)
		prodver=$Product"_"$TrgOSVers
	fi
}

# Description:
#	Build the admin script for pkgadd
# Parameters:
#	none
# Globals Used:
#	ADMINFILE
function build_admin
{
	if [[ "$PatchMethod" = "direct" && -f /var/sadm/install/admin/patch ]]
	then
		ADMINFILE=/var/sadm/install/admin/patch
	else
		cat >$ADMINFILE <<EOF
mail=
instance=unique
partial=nocheck
runlevel=nocheck
idepend=quit
rdepend=quit
space=quit
setuid=nocheck
conflict=nocheck
action=nocheck
basedir=default
EOF
fi
}

# Description:
# 	Restore old versions of files
# Parameters:
#	$1	- patch database directory
#	$2	- patch number
#	$3	- package command relocation argument
#	$4	- path name of contents file
#	

function restore_orig_files
{
	olddir=
	file=
	ownerfound=
	srch=
	cfpath=
	instlist=
	filelist=

	if [[ ! -f $1/$2/.nofilestosave ]]
	then
		/usr/bin/gettext "Restoring previous version of files...\n"
		if [[ "$PATCH_UNDO_ARCHIVE" != "none" ]]
		then
			olddir=$PATCH_UNDO_ARCHIVE
		else
			olddir=$(pwd)
			olddir=$olddir/save
		fi
		cd $ROOTDIR
		# Must retain backwards compatibility to restore
		# archives which were not stored as files
		if [[ -f $olddir/archive.cpio ]]
		then 
			filelist=$(cat $olddir/archive.cpio | cpio -it 2>/dev/null)
			cpio -idumv -I $olddir/archive.cpio
		else 
			if [[ -f $olddir/archive.cpio.Z ]]
			then
				filelist=$(zcat $olddir/archive.cpio.Z | \
							cpio -it 2>/dev/null)
				zcat $olddir/archive.cpio.Z | cpio -idumv
			else
				filelist=$($FIND . -print | $SED "s/^.//")
				$FIND  . -print | cpio -pdumv / 
			fi
		fi
		if [[ $? -ne 0 ]]
		then
			/usr/bin/gettext "Restore of old files failed.\nSee README file for instructions.\n"
			$RM -f /tmp/*.$$
			remove_libraries
			exit 7
		fi
		/usr/bin/gettext "Making package database consistent with restored files:\n"
		$RM -f /tmp/fixfile.$$ > /dev/null 2>&1
		for file in $filelist
		do
			if [[ ! -f $file || -h $file ]]
			then
				continue
			fi
			# if file failed validation when the patch was 
			# installed, don't do an installf on it.  It should 
			# continue to fail validation after the patch is 
			# backed out.
			file1=$(expr $file : '\(\/.*\)')
			if [[ "$file1" = "" ]]
			then
				file1="/"$file
			fi
			srch="^$file1\$"
			if [[ -f $1/$2/.validation.errors ]] && \
				grep "$srch" $1/$2/.validation.errors >/dev/null 2>&1
			then 
				continue
			fi

			# The following commands find the file's entry in the
			# contents file, and return the first field of the 
			# entry. If the file is a hard link, the first field 
			# will contain an "=".  This will cause the -f test to 
			# fail and we won't try to installf the file.
			srch="^$file1[ =]"
			cfpath=$(grep "$srch" $CONTENTS | $SED 's/ .*//')
			if [[ "$cfpath" = "" || ! -f "$ROOTDIR$cfpath" ]]
			then
				continue
			fi
			ownerfound=no
			# Parsing pkgchk output is complicated because all text
			# may be localized. Currently the only line in the 
			# output which contains a tab is the line of packages 
			# owning the file, so we search for lines containing a 
			# tab.  This is probably reasonably safe. If any of the
			# text lines end up with tabs due to localization, the 
			# pkginfo check should protect us from calling installf
			# with a bogus package instance argument.
			pkgchk $3 -lp $file1 | grep '	' | \
			while read instlist
			do
				for i in $instlist
				do
					pkginfo $3 $i >/dev/null 2>&1
					if [[ $? -eq 0 ]]
					then
						echo $i $file1 >> /tmp/fixfile.$$
						ownerfound=yes
						break
					fi
				done
				if [[ $ownerfound = "yes" ]]
				then
					break
				fi
			done
		done
		if [[ -s /tmp/fixfile.$$ ]]
		then
			$SED 's/^\([^ ]*\).*/\1/' /tmp/fixfile.$$ | sort -u | \
			while read pkginst
			do
				grep "^${pkginst} " /tmp/fixfile.$$ | \
				$SED 's/^[^ ]* \(.*\)/\1/' | \
				if [[ "$ROOTDIR" != "/" ]]
				then
					installf $PKGDBARG $pkginst -
					installf $PKGDBARG -f $pkginst
				else
					installf $pkginst -
					installf -f $pkginst
				fi
			done
		fi
		cd $olddir
	fi
}

#
# Description:
#	Change directory to location of patch
# Parameters:
#	$1	- patch database directory
#	$2	- patch number
# Globals Set:
#	patchdir
#	PatchBase
#	PatchVers
function activate_patch
{
	eval_inst_patches $PKGDB

	if [[ $ThisPatchFnd = "yes" ]]
	then
		patchdir=$1/$2

		# For direct instance patches, this may not be here
		if [[ -d $patchdir ]]
		then
			cd $patchdir
		fi

		#
		# Get the patch base code (the number up to the version prefix) 
		# and the patch revision number (the number after the version prefix).
		#
		get_base_code $PatchNum
		PatchBase=$cur_base_code
		get_vers_no $PatchNum $cur_base_code
		PatchVers=$cur_vers_no
	else
		/usr/bin/gettext "Patch $2 has not been applied to this system.\n"
 		if [[ -d $1/$2 ]]
		then
 			/usr/bin/gettext "Will remove directory $1/$2\n"
 			$RM -r $1/$2
 		fi

		patch_quit 2
	fi

}

# Description:
#	Find the package instances for this patch
# Parameters:
#	$1	- package database directory
#	$2	- patch number
# Globals Set:
#	pkginstlist

function get_pkg_instances
{
	pkginst=
	j=
	for j in $1/*
	do
		if grep -s "SUNW_PATCHID *= *$2" $j/pkginfo > /dev/null 2>&1
		then
			pkginst=$(basename $j)
			pkginstlist="$pkginstlist $pkginst"
		fi
	done
}

# Description:
# 	Check to see if this patch was obsoleted by another patch.
# Parameters:
#	$1	- patch database directory
#	$2	- patch ID
#	$3	- patch revision

function check_if_obsolete
{
	if [[ "$diPatch" = "yes" ]]
	then
		if [[ "$ObsoletedBy" = "none" ]]
		then
			return
		else
			print_obsolete_msg "$ObsoletedBy" "none"
			exit 6
		fi
	else
		Patchid=
		oldbase=
		oldrev=
		opatchid=
		obase=
		obsoletes=
		i=
		j=
		if [[ -d $1 ]]
		then
			cd $1
			for i in * X
			do
				if [[ $i = X || "$i" = "*" ]]
				then
					break
				elif [[ ! -d $i ]]
				then
					continue
				fi
				cd $i
				for j in */pkginfo X
				do
					if [[ "$j" = "X" || "$j" = "*/pkginfo" ]]
					then
						break
					fi
					Patchid=$($SED -n 's/^[ 	]*SUNW_PATCHID[ 	]*=[ 	]*\([^ 	]*\)[	 ]*$/\1/p' $j)
					if [[ "$Patchid" = "" ]]
					then
						continue
					fi
					oldbase=${Patchid%-*}
					oldrev=${Patchid#*-}
					if [[ $oldbase = $2 && $3 -lt $oldrev ]]
					then
						print_obsolete_msg "$2" "$oldrev"
						exit 6
					fi
					obsoletes=$($SED -n 's/^[	 ]*SUNW_OBSOLETES[ 	]*=[ 	]*\([^ 	]*\)[ 	]*$/\1/p' $j)
					while [ "$obsoletes" != "" ]
					do
						opatchid=$(expr $obsoletes : '\([0-9\-]*\).*')
						obsoletes=$(expr $obsoletes : '[0-9\-]*[ ,]*\(.*\)')
						# patchrevent infinite loop.  If we couldn't
						# find a valid patch id, just quit.
						if [[ "$opatchid" = "" ]]
						then
							break;
						fi
						obase=$(expr $opatchid : '\(.*\)-.*')
						if [[ "$obase" = "" ]]
						then
							# no revision field in opatchid,
							# might be supported someday 
							# (we don't use the revision 
							# field for obsoletion testing)
							obase=$opatchid
						fi
						if [[ $obase = $2 && $2 != $oldbase ]]
						then
							print_obsolete_msg "$Patchid" "none"
							exit 6
						fi
					done
				done
				cd $1
			done
		fi
	fi
}

# Description:
#	Check to see if originally modified files were saved. If not,
#	the patch cannot be backed out.
# Parameters:
#	$1	- patch database directory
#	$2	- patch number

function check_if_saved
{
	if [[ ! -f $1/$2/.oldfilessaved && ! -f $1/$2/.nofilestosave ]]
	then
		/usr/bin/gettext "Patch $2 was installed without backing up the original files.\nIt cannot be backed out.\n"
		exit 4
	fi
}

# Description:
#	Get the list of packages 
# Parameters:
#	$1	- patch database directory
#	$2	- patch number
# Globals Set:
#	pkglist

function get_package_list
{
	pkg=
	i=
	cd $1/$2
	for i in */pkgmap
	do
		pkg=`expr $i : '\(.*\)/pkgmap'`
		pkglist="$pkglist $pkg"
	done
}

# Description:
# Parameters:
#	$1	- patch database directory
#	$2	- patch number
#	$3	- softinfo directory
#	$4	- product version
# Globals Used:
#	TMPSOFT

function cleanup
{
	$RM -f /tmp/*.$$

	if [[ -d $1 ]]
	then
		cd $1
		if [[ -f softinfo_sed ]]
		then
			sed -f softinfo_sed $3/$4 > $TMPSOFT
			$MV $3/$4 $3/sav.$4
			$CP $TMPSOFT $3/$4
		fi
		$RM -fr ./$2/*
		$RM -fr $2

		if [[ "$PATCH_UNDO_ARCHIVE" != "none" ]]
		then
			PATCH_UNDO_ARCHIVE=$(dirname $PATCH_UNDO_ARCHIVE)
			$RM -fr $PATCH_UNDO_ARCHIVE/$2
		fi
	fi

    if [[ "$netImage" = "boot" && -d $ROOTDIR/mnt/root ]]
    then
        restore_net_image
    fi

}

# Description:
#	Remove appropriate patch packages from the system 
#	NOTE: this will not restore the overwritten or removed files, but will
#	      remove any files which were added by the patch.
# Parameters:
#	$1	- patch database directory
#	$2	- patch number
#	$3	- packaging command relocation argument 
# Globals Used:
#	ADMINFILE
#	pkginstlist

function remove_patch_pkgs
{
	pkgrmerr=
	i=
	if [[ "$PATCH_UNDO_ARCHIVE" != "none" ]]
	then
		if [[ ! -d $PATCH_UNDO_ARCHIVE ]]
		then
			/usr/bin/gettext "The backout data has been moved. Please supply\npatchrm with the new location of the archive.\n"
			patch_quit 12
		fi
	fi

	for i in $pkginstlist
	do
		/usr/bin/gettext "\nRemoving patch package for $i:\n"
		pkgrm $3 -a $ADMINFILE -n $i>$LOGFILE 2>&1
		pkgrmerr=$?
		cat $LOGFILE >>$1/$2/log
		cat $LOGFILE | grep -v "^$"
		$RM -f $LOGFILE
		if [[ $pkgrmerr != 0 && $pkgrmerr != 2 && $pkgrmerr != 10 && $pkgrmerr != 20 ]]
		then
			/usr/bin/gettext "pkgrm of $i package failed with return code $pkgrmerr.\nSee $1/$2/log for details.\n"
			$RM -fr /tmp/*.$$
			remove_libraries
			exit 5
		fi
	done
}

# Description:
#	Copy required libraries to TMP_LIB_DIR, set and
#	export LD_PRELOAD.
# Parameters:
#	none
# Environment Variables Set:
#	LD_PRELOAD
#
function move_libraries
{
	typeset -i Rev
    Rev=$(uname -r | $SED -e 's/\..*$//')
    if (( Rev >= 5 ))
    then

        if [[ ! -d $TMP_LIB_DIR ]]
        then
            mkdir -p -m755 $TMP_LIB_DIR
        fi

        LD_PRELOAD=
        for Lib in libc libdl libelf libintl libw libadm
        do
            $CP /usr/lib/${Lib}.so.1 ${TMP_LIB_DIR}/${Lib}.so.1

            chown bin ${TMP_LIB_DIR}/${Lib}.so.1
            chgrp bin ${TMP_LIB_DIR}/${Lib}.so.1
            chmod 755 ${TMP_LIB_DIR}/${Lib}.so.1

            LD_PRELOAD="${LD_PRELOAD} ${TMP_LIB_DIR}/${Lib}.so.1"
        done
        export LD_PRELOAD
    fi
}

# Description:
#	remove the TMP_LIB_DIR directory
# Parameters:
#	none
# Environment Variables Set:
#	LD_PRELOAD
#
function remove_libraries
{
	LD_PRELOAD=
	export LD_PRELOAD
	$RM -rf $TMP_LIB_DIR
}

# Description:
#	unobsolete direct instance patches that this one obsoleted
# Parameters:
#	none
# Environment Variables Used:
#	ROOTDIR
#	InstPkgs
#	PatchNum
#
function di_unobsolete
{
	cd $ROOTDIR
	cd var/sadm/pkg
	for pkg in $InstPkgs; do
		PATCHLIST=$(pkgparam -R $ROOTDIR $pkg PATCHLIST)
		for Patchno in $PATCHLIST; do
			check_remote_file $pkg $Patchno
	        	archive_path=$pkg/save/$Patchno

			if [[ -f $archive_path/obsolete || -f $archive_path/obsolete.Z || -f $archive_path/remote ]]
			then
				if [[ -f $archive_path/obsoleted_by ]]
				then
					egrep -s $PatchNum $archive_path/obsoleted_by
				fi
				if [[ $? -eq 0 ]]
				then
					cat $archive_path/obsoleted_by | $NAWK -v patchno=$PatchNum '
						$0 ~ patchno	{ next; }
						{ print; } ' > $archive_path/obsoleted_by.new

					if [[ -f $archive_path/remote ]]
					then
						restore_remote_state $pkg $Patchno
					fi

					if [[ -s $archive_path/obsoleted_by.new ]]
					then
						$MV $archive_path/obsoleted_by.new $archive_path/obsoleted_by
					else
						$RM -f $archive_path/obsoleted_by.new $archive_path/obsoleted_by
						if [[ -f $archive_path/remote ]]
						then
							continue
						fi

						if [[ -f $archive_path/obsolete ]]
						then
							$MV $archive_path/obsolete $archive_path/undo
						else
							$MV $archive_path/obsolete.Z $archive_path/undo.Z
						fi
					fi
				fi
			fi
		done
	done
}

# Description:
#	Check to see if the patch being backed out has other patches 
#	installed that require it to be there.
# Parameters:
#	none
# Locals Used
#	tmp
#	list
#	requires
#	obsPatch
#	pkg
#
function check_REQUIRE
{
	requires=
	list=
	obsPatch=
	pkg=

	cd $PKGDB
	for pkg in *
	do
		if [[ -f $pkg/pkginfo ]]
		then
			# Collect the data from a pkginfo file
			list=$(pkgparam -R $ROOTDIR $pkg PATCHLIST)
			for patch in $list
			do
                tmpStr=""
                tmpStr=$(pkgparam -R $ROOTDIR $pkg PATCH_INFO_$patch)
                requires=$(echo $tmpStr | $GREP Requires: | $NAWK ' \
                  { print substr($0, match($0, "Requires:")+10) } ' \
                  | $SED 's/Incompatibles:.*//g')

				if [[ -n "$requires" ]]
				then
					for req in $requires
					do
						get_base_code $req
						reqBase=$cur_base_code
						get_vers_no $req $cur_base_code
						reqVers=$cur_vers_no

						# check to see if the required patch has been obsoleted.
						obsPatch=$(/usr/bin/grep $PatchNum  \
						  $pkg/save/$requires/obsoleted_by > /dev/null 2>&1)

						if [[ -z "$obsPatch" ]]
						then
							# We need to check to see if the obsoleted patch 
							# is obsoleted by another.
							for ob in $ObsArrEntry
							do
								obsPatch=$(/usr/bin/grep $PatchNum \
								  $pkg/save/$ob/obsoleted_by > /dev/null 2>&1)
							done
						fi

						# We need to check if the patch that is being
						# backed out has obsoleted a patch that is 
						# required to be installed.

                		obPat=$(pkgparam -R $ROOTDIR $pkg PATCH_INFO_$PatchNum \
						  | $GREP Obsoletes: | $NAWK ' \
						  { print substr($0, match($0, "Obsoletes:")+11) } ' \
						  | $SED 's/Requires:.*//g')

						if [[ -n "$obPat" ]]
						then
							for ob in $obPat
							do
								get_base_code $ob
								obBase=$cur_base_code
								get_vers_no $ob $cur_base_code
								obVers=$cur_vers_no
						
								if [[ "$reqBase" = "$obBase" && \
								  "$obVers" -ge "$reqVers" ]]
								then
									print_require_msg $patch
								fi
							done
						fi
							
						if [[ "$reqBase" = "$PatchBase" && \
						  "$reqVers" -le "$PatchVers" || -n $obsPatch ]]
						then
							print_require_msg $patch
						fi
						if [[ "$reqBase" = "$PatchBase" && \
						  "$reqVers" -le "$PatchVers" || -n $obsPatch ]]
						then
							print_require_msg $patch
						fi
					done
				fi
			done
		fi
	done
}

# Description:
#   Patch requires message.
#   out was superceded by other patches
# Parameters:
#   $1  - required patch ID
#
function print_require_msg
{
	/usr/bin/gettext "Patch $PatchNum is required to be installed by patch $1\nit cannot be backed out until patch $1 is backed out.\n"
	patch_quit 15
}

# Description:
#	Detect if there have been any implicit or explicit obsoletions.
# Parameters:
#	none
# Environment Variable Set:
#
function detect_obs
{
	cd $ROOTDIR
	cd var/sadm/pkg
	if [[ $ThisPatchFnd = "no" ]]
	then
		/usr/bin/gettext "Patch $PatchNum has not been applied to this system.\n"
		exit 2
	fi

	#
	# First scan for the undo and remote files and make sure, none of them have
	# been obsoleted
	#
	for pkg in $PatchedPkgs; do
		if [[ -f $pkg/pkginfo ]]
		then
			if [[ -d $pkg/save/$PatchNum ]]
			then
				if [[ -f $pkg/save/$PatchNum/remote ]]
				then
                   	check_remote_file $pkg $PatchNum
					if [[ ! -f $PATCH_UNDO_ARCHIVE/undo && ! -f $PATCH_UNDO_ARCHIVE/undo.Z ]]
					then
           				/usr/bin/gettext "The backout archive has been moved.\nSupply the -B option to back out the patch.\n"
						patch_quit 12
                     elif [[ -f $pkg/save/$PatchNum/obsoleted_by ]]
                     then
						ObsoletedBy=$(cat $pkg/save/$PatchNum/obsoleted_by)
						print_obsolete_msg "$ObsoletedBy" "none"
						exit 6
					fi
				else
					if [[ -f $pkg/save/$PatchNum/obsolete || -f $pkg/save/$PatchNum/obsolete.Z ]]
					then
						ObsoletedBy=$(cat $pkg/save/$PatchNum/obsoleted_by)
						print_obsolete_msg "$ObsoletedBy" "none"
						exit 6
						
					elif [[ ! -f $pkg/save/$PatchNum/undo && ! -f $pkg/save/$PatchNum/undo.Z ]]
					then
						/usr/bin/gettext "Patch $PatchNum was installed without backing up the original files.\nIt cannot be backed out.\n"
						exit 4 
					fi
				fi
			else
				/usr/bin/gettext "Patch $PatchNum was installed without backing up the originalfiles.\nIt cannot be backed out.\n"
				exit 4 

			fi
		else
			/usr/bin/gettext "Patch $PatchNum was installed without backing up the originalfiles.\nIt cannot be backed out.\n"
			exit 4 
		fi
	done
}

# Description:
#	backout a patch applied using direct instance patching
# Parameters:
#	none
# Environment Variable Set:
#
function di_backout
{
	typeset -i Something_Backedout=0
	typeset -i exit_code=0

	cd $ROOTDIR
	cd var/sadm/pkg

	#
	# With no obsoletions detected, we pkgadd the undo packages.
	#
	for pkg in $PatchedPkgs; do
               	check_remote_file $pkg $PatchNum
		if [[ "$PATCH_UNDO_ARCHIVE" != "none" && -f $PATCH_UNDO_ARCHIVE/undo.Z ]]
		then
                        uncompress $PATCH_UNDO_ARCHIVE/undo.Z 1> $LOGFILE 2>&1
		elif [[ -f $pkg/save/$PatchNum/undo.Z ]]
		then
			uncompress $pkg/save/$PatchNum/undo.Z 1> $LOGFILE 2>&1
		fi

		# Get the prior patch list since the checkinstall script
		# doesn't have permission to make this enquiry.
		OLDLIST=$(pkgparam -R $ROOTDIR $pkg PATCHLIST)

		echo OLDLIST=\'$OLDLIST\' > $RESPONSE_FILE

		if [[ "$PATCH_UNDO_ARCHIVE" != "none" ]]
		then
			pkgadd -n -r $RESPONSE_FILE -R $ROOTDIR -a $ADMINFILE -d $PATCH_UNDO_ARCHIVE/undo all 1>> $LOGFILE 2>&1
		else
			pkgadd -n -r $RESPONSE_FILE -R $ROOTDIR -a $ADMINFILE -d $pkg/save/$PatchNum/undo all 1>> $LOGFILE 2>&1
		fi

		exit_code=$?

		# If it's a suspend (exit code 4), then the
		# message type is the appropriate installpatch
		# exit code and the appropriate message follows.
		# A suspend means, nothing has been installed.
		if (( exit_code == 4 ))	# suspend
		then
			Message=$(egrep PaTcH_MsG $LOGFILE | $SED s/PaTcH_MsG\ //)
			if [[ $Message = "" ]]
			then
				exit_code=5
			else
				Msg_Type=$(echo $Message | $NAWK ' { print $1 } ')
				Message=$(echo $Message | $SED s/$Msg_Type\ //)
				/usr/bin/gettext "$Message\n" >> $LOGFILE
				/usr/bin/gettext "$Message\n"
				exit $Msg_Type
			fi
		fi

		if ((exit_code == 5 ))
		then	# administration
			/usr/bin/gettext "Backout has been halted due to administrative defaults.\n"
			cat $LOGFILE
			exit 11
		elif (( exit_code == 10 || exit_code == 20 ))
		then
			/usr/bin/gettext "NOTE: After backout the target host will need to be rebooted.\n"
			RebootRqd="yes"
		elif (( exit_code != 0 ))
		then
			egrep ERROR $LOGFILE
			exit 7
		else
			Something_Backedout=1
		fi
	done

	if (( Something_Backedout == 1 ))
	then
		di_unobsolete
		if [[ "$PATCH_UNDO_ARCHIVE" != "none" ]]
		then
			PATCH_UNDO_ARCHIVE=$(dirname $PATCH_UNDO_ARCHIVE)
			PATCH_UNDO_ARCHIVE=$(dirname $PATCH_UNDO_ARCHIVE)
			$RM -r $PATCH_UNDO_ARCHIVE/$PatchNum
		fi
	else
		/usr/bin/gettext "Patch number $PatchNum backout packages were not found.\n"
	fi
}

# Description:
#       check to see if the backout data is saved remotely
# Parameters:
#       $1      - package associated with the patch
#       $2      - the patch number
#
# Environment Variable Set:
#
#	PATCH_UNDO_ARCHIVE
#	OBS_PATCH_UNDO_ARCHIVE

function check_remote_file
{
	if [[ "$diPatch" = "yes" ]]
	then
        	if [[ ! -f $PKGDB/$1/save/$2/remote ]]
        	then
			return
		fi

		if [[ "$DASHB_SUPPLIED" = "yes" ]]
		then
			if [[ $2 != "$PatchNum" ]]
			then
                        	OBS_PATCH_UNDO_ARCHIVE=$TEMP_PATCH_UNDO_ARCHIVE/$2/$1			
			else
				PATCH_UNDO_ARCHIVE=$TEMP_PATCH_UNDO_ARCHIVE/$2/$1
			fi
		elif [[ $2 != "$PatchNum" ]]
		then
                        OBS_PATCH_UNDO_ARCHIVE=$(grep "FIND_AT" $PKGDB/$1/save/$2/remote | $AWK -F= '{print $2}')
                        OBS_PATCH_UNDO_ARCHIVE=$(dirname $OBS_PATCH_UNDO_ARCHIVE)
		else
                        PATCH_UNDO_ARCHIVE=$(grep "FIND_AT" $PKGDB/$1/save/$2/remote | $AWK -F= '{print $2}')
                        PATCH_UNDO_ARCHIVE=$(dirname $PATCH_UNDO_ARCHIVE)
		fi
	# progressive instance logic
	else
        	if [[ ! -f $PATCHDB/$2/save/remote ]]
        	then
			return
		fi

		if [[ "$DASHB_SUPPLIED" = "yes" ]]
		then
        		PATCH_UNDO_ARCHIVE=$TEMP_PATCH_UNDO_ARCHIVE/$2/archive.cpio
		else
        		PATCH_UNDO_ARCHIVE=$(grep "FIND_AT" $PATCHDB/$2/save/remote | $AWK -F= '{print $2}')
		fi
		PATCH_UNDO_ARCHIVE=$(dirname $PATCH_UNDO_ARCHIVE)
		
	fi
}
		
# Description:
#       restore the STATE parameter back to the proper
#	state in the remote file
# Parameters:
#       $1      - package associated with the patch
#       $2      - the patch number
#
# Environment Variable Set:
#
function restore_remote_state
{
	$(grep . $PKGDB/$1/save/$2/remote | $SED 's/STATE=.*/STATE=active/' > $TEMP_REMOTE)
	$RM -f $PKGDB/$1/save/$2/remote
	$MV $TEMP_REMOTE $PKGDB/$1/save/$2/remote
	$RM -f $TEMP_REMOTE
}

# Description:
#       Call check_remote_file if the remote file is found
#       for progressive instance patches
# Parameters:
#       $1      - patch database
#       $2      - the patch number
#
# Globals Set:
#	none

function set_archive_path
{
	if [[ ! -f $1/$2/remote ]]
	then
		check_remote_file $1 $2
	fi
}

# Description:
#   Setup the net install boot image to look like an installed system.
# Parameters:
#   none
# Globals Set:
#   none
#
function setup_net_image {

    if [[ "$netImage" != "boot" ]]
    then
        return
    fi

	# Check to see if there was an interruption that left the loop back
	# mounts mounted for Net Install Patching.

	if [[ -d $ROOTDIR/mnt/root ]]
	then
		restore_net_image
	fi

	# The .../Boot/.tmp_proto/root needs to be re-mapped to .../Boot/tmp in order
	# for the boot image to be patched successfully.

    $MOUNT -F lofs $ROOTDIR/tmp $ROOTDIR/mnt
    $MOUNT -F lofs $ROOTDIR/.tmp_proto $ROOTDIR/tmp
    $MOUNT -F lofs $ROOTDIR/mnt/root/var $ROOTDIR/tmp/root/var
	# At this point installpatch thinks the net install image is just like an installed image.
}

# Description:
#	Restore the net image to the way it was before mucking
#	with it in the setup_net_image function.
# Parameters:
#   none
# Globals Set:
#   none
#
function restore_net_image {

    if [[ "$netImage" != "boot" ]]
    then
        return
    fi

    $UMOUNT $ROOTDIR/tmp/root/var
    $UMOUNT $ROOTDIR/tmp
    $UMOUNT $ROOTDIR/mnt
}

#########################################################
#					
# 			Main routine
#				
#########################################################

# -	Parse the argument list and set globals accordingly
# -	Make sure the user is running as 'root'
# -	Get the product version <name>_<version> of the local
#	Solaris installation
# - 	activate the patch

Cmd=$0
CmdArgs=$*

parse_args $*

validate_uid

find_softinfos $ROOTDIR

get_OS_version $TRGSOFTINFO $MGRSOFTINFO $ROOTDIR

check_file_recovery

setup_net_image

/usr/bin/gettext "Checking installed packages and patches...\n\n"

activate_patch "$PATCHDB" "$PatchNum"

if [[ "$diPatch" != "yes" ]]
then
	echo $PatchNum | grep $PatchIdFormat >/dev/null
	if [[ $? -ne 0 ]]
	then
		/usr/bin/gettext "Invalid patch id format: $PatchNum\n"
		exit 8
	fi
fi

#
# Check to see if this patch was obsoleted by another patch
#
if [[ "$force" = "no" || "$diPatch" = "yes" ]]
then
	check_if_obsolete "$PATCHDB" "$PatchBase" "$PatchVers"
fi
execute_prebackout "$PATCHDB" "$PatchNum"

# -	Check to see if original files were actually saved
# -	Generate list of packages to be removed
# -	Find the package instances for this patch
# -	Build admin file for later use by pkgrm
# -	pkgrm patch packages
# -	Restore the original files which were overwritten by the patch
# -	Update the prodver file & cleanup tmp files

build_admin

if [[ "$diPatch" = "yes" ]]
then
	detect_obs
	check_REQUIRE
	di_backout
	$RM -f $RESPONSE_FILE
else
	set_archive_path "$PATCHDB" "$PatchNum" 

	check_if_saved "$PATCHDB" "$PatchNum"

	get_package_list "$PATCHDB" "$PatchNum"

	get_pkg_instances "$PKGDB" "$PatchNum"

	trap 'remove_libraries' HUP INT QUIT TERM
	move_libraries

	remove_patch_pkgs "$PATCHDB" "$PatchNum" "$PKGDBARG"

	restore_orig_files "$PATCHDB" "$PatchNum" "$PKGDBARG" "$CONTENTS"

	remove_libraries

fi

execute_postbackout "$PATCHDB" "$PatchNum"

cleanup "$PATCHDB" "$PatchNum" "$SOFTINFO" "$prodver"

/usr/bin/gettext "Patch $PatchNum has been backed out.\n\n"

exit 0
