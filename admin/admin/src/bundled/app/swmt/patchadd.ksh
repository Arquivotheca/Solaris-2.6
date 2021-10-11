#!/bin/ksh -h

# @(#) patchadd.ksh 1.4 96/10/15 SMI
#
# Exit Codes:
#		0	No error
#		1	Usage error
#		2	Attempt to apply a patch that's already been applied
#		3	Effective UID is not root
#		4	Attempt to save original files failed
#		5	pkgadd failed
#		6	Patch is obsoleted
#		7	Invalid package directory
#		8	Attempting to patch a package that is not installed
#		9	Cannot access /usr/sbin/pkgadd (client problem)
#		10	Package validation errors
#		11	Error adding patch to root template
#		12	Patch script terminated due to signal
#		13	Symbolic link included in patch
#		14	NOT USED
#		15	The prepatch script had a return code other than 0.
#		16	The postpatch script had a return code other than 0.
#		17	Mismatch of the -d option between a previous patch
#			install and the current one.
#		18	Not enough space in the file systems that are targets
#			of the patch.
#		19	$SOFTINFO/INST_RELEASE file not found
#		20	A direct instance patch was required but not found
#		21	The required patches have not been installed on the manager
#		22	A progressive instance patch was required but not found
#		23	A restricted patch is already applied to the package
#		24	An incompatible patch is applied
#		25	A required patch is not applied
#		26	The user specified backout data can't be found
#		27	The relative directory supplied can't be found
#		28	A pkginfo file is corrupt or missing
#		29	Bad patch ID format
#		30	Dryrun failure(s)
#		31	Path given for -C option is invalid
#		32	Must be running Solaris 2.6 or greater
#		33	Bad formatted patch file or patch file not found
#

# Set the path for use with these scripts.
PATH=/usr/sadm/bin:/usr/sbin:/usr/bin:$PATH
export PATH

umask 022

# Global Files
RECOVERDIR=/var/sadm/.patchRec 

# Needed utilities
DF=/usr/sbin/df
MV=/usr/bin/mv
RM=/usr/bin/rm
SED=/usr/bin/sed
AWK=/usr/bin/awk
NAWK=/usr/bin/nawk
GREP=/usr/bin/grep
EGREP=/usr/bin/egrep
LS=/usr/bin/ls
CP=/usr/bin/cp
WC=/usr/bin/wc
FIND=/usr/bin/find
MD=/usr/bin/mkdir
TOUCH=/usr/bin/touch
DIFF=/usr/bin/diff
TAIL=/usr/bin/tail
MOUNT=/sbin/mount
UMOUNT=/sbin/umount
UNAME=/usr/bin/uname

multiPtchInstall="no"
multiPtchList=
multiPtchStatus=
lastPtchInList=
patchdir=
olddir=
validate="yes"
saveold="yes"
netImage="none"
dryrunDir="none"

ROOTDIR="/"
PATCHDB="/var/sadm/patch"
PKGDB="/var/sadm/pkg"
NEW_SOFTINFO="/var/sadm/system/admin/INST_RELEASE"
OLD_SOFTINFO="/var/sadm/softinfo/INST_RELEASE"
MGRSOFTINFO="none"
TRGSOFTINFO="none"
PKGDBARG=""
PatchIdFormat='^[A-Z]*[0-9][0-9][0-9][0-9][0-9][0-9]-[0-9][0-9]$'

# response file keywords
PATCH_UNCONDITIONAL="false"
PATCH_PROGRESSIVE="true"
PATCH_NO_UNDO="false"
PATCH_BUILD_DIR="none"
PATCH_UNDO_ARCHIVE="none"
INTERRUPTION="no"
DRYRUN="no"

typeset -i Root_Kbytes_Needed=0
typeset -i Kbytes_Required=0
typeset -i Opt_Kbytes_Needed=0
typeset -i Openwin_Kbytes_Needed=0
typeset -i Usr_Kbytes_Needed=0
typeset -i Client_Kbytes_Needed=0
typeset -i Var_Kbytes_Needed=0
typeset -i ReqArrCount=0
typeset -i ReqdPatchCnt=0
typeset -i Something_Installed=0
typeset -i interactive=0

# List of required commands
REQD_CMDS="/usr/sbin/removef /usr/sbin/installf /usr/sbin/pkgadd /usr/bin/grep \		   /usr/bin/find /usr/bin/pkgparam /usr/bin/pkginfo"

# Description:
#   Set the pid specific globals for use with multiple patch installation.
# Parameters:
#   none
#
function set_globals
{

	EXISTFILES=/tmp/existfiles.$$
	PATCHFILES=/tmp/patchfiles.$$
	PKGCOFILE=/tmp/pkgchk.out.$$
	VALERRFILE=/tmp/valerr.$$
	VALWARNFILE=/tmp/valwarn.$$
	ADMINTFILE=/tmp/admin.tmp.$$
	ADMINFILE=/tmp/admin.$$
	LOGFILE=/tmp/pkgaddlog.$$
	TMP_ARCHIVE=/tmp/TmpArchive.$$
	TMP_FILELIST=/tmp/FileList.$$
	TMP_LIB_DIR=/tmp/TmpLibDir.$$
	INSTPATCHES_FILE=/tmp/MyShowrevFile.$$
	PARAMS_FILE=/tmp/ParamsFile.$$
	RESPONSE_FILE=/tmp/response.$$
	TEMP_REMOTE=/tmp/temp_remote.$$

	Obsoletes=
	Incompat=
	Requires=
	ObsoletePast=""
	UninstReqs=
	InstIncompat=
	Product=
	MgrProduct=
	
	OpenwinFS=
	OptFS=
	UsrFS=
	VarFS=
	ClientFS=
	pkglist=
	newpkglist=

	client="no"
	is_a_root_pkg="no"
	is_an_instpatches="no"
	ret=
	curdir=
	PatchNum=
	PatchBase=
	PatchVers=
	PatchMethod=
	PatchType=
	printpatches="no"
	ThisPatchFnd="no"
	ObsoletedBy="none"
	ReqdOSPatch="none"
	rootlist=
	isapplied="no"
	libs_are_moved="no"
	useRecFiles="no"

	Root_Kbytes_Needed=0
	Kbytes_Required=0
	Opt_Kbytes_Needed=0
	Openwin_Kbytes_Needed=0
	Usr_Kbytes_Needed=0
	Client_Kbytes_Needed=0
	Var_Kbytes_Needed=0
	ReqArrCount=0
	ReqdPatchCnt=0
	Something_Installed=0
	pkgErrGlob=
}

# Description:
#   Usage message
# Parameters:
#   none
#
function print_usage
{
cat << EOF

Usage: patchadd [-u] [-d] [-B <backout_dir>]
                    [-S <service> | -R <client_root_path>]
                    [-C <net_install_image>] <patch_loc>

       patchadd [-u] [-d] [-B <backout_dir>]
                    [-S <service> | -R <client_root_path>] 
                    [-C <net_install_image>]
                    -M <patch_directory> <patchid>... | <patchlist>

       patchadd [-S <service> | -R <client_root_path>] 
                    [-C <net_install_image>] -p
EOF

# This line is part of the usage msg
}

#
# Description:
# 	Quit patchadd and clean up any remaining temporary files.
#
# Parameters:
#	$1 - exitcode
#	$2 - Quit the entire installation 

function patch_quit {	# exit code
	if [[ "$lastPtchInList" != "$PatchNum" && $2 = "no" ]]
	then
		/usr/bin/gettext "WARNING: Skipping patch $PatchNum \n\n"
		return
	elif [[ $1 -ne 0 ]]
	then
		/usr/bin/gettext "\nInstallpatch is terminating.\n"
	fi

	if [[ "$netImage" != "none"  && -d "$ROOTDIR/mnt/root" ]]
	then
		restore_net_image
	fi

	remove_files

	exit $1
}

#
# Description:
#	Remove the patch recovery directory if installation
#	was successful and any other files needing to be removed.
#
# Globals Used:
#	RECOVERDIR
#	Something_Installed	

function remove_files {
    $RM -f $INSTPATCHES_FILE
	$RM -f /tmp/*.$$.1
	$RM -f /tmp/archive.cpio*
	$RM -fr /tmp/*.$$
    $RM -fr /tmp/installa*

	if (( Something_Installed == 1 ))
	then
		$RM -fr $RECOVERDIR
	fi

	if [[ $libs_are_moved = "yes" ]]
	then
		remove_libraries
	fi
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
#	If a prepatch executable exists in the $1 directory, execute it.
#	If the return code is 0, continue. Otherwise, exit with code 15.
#
# Parameters:
#	$1	- patch directory.
# Globals Set:
#	none
#
function execute_prepatch
{
	typeset -i retcode=0
	if [[ -x "$1/prepatch" ]]
	then
		/usr/bin/gettext "Executing prepatch script...\n"
		$1/prepatch
		retcode=$?
		if (( retcode != 0 ))
		then
			/usr/bin/gettext "The prepatch script exited with return code $retcode.\n"
			patch_quit 15 "no"
			return 0
		fi
	fi
	return 1
}

#
# Description:
#	If a postpatch executable exists in the $1 directory, execute it.
#	If the return code is 0, continue. Otherwise, if this is not
#	a re-installation of the patch, execute the 
#	backoutpatch script and exit with a return code 16.
#	If this is a re-installation, don't backout the patch. Instead,
#	send a message to the user.
#
# Parameters:
#	$1	- patch database directory
#	$2	- patch number
#	$3	- patch directory.
# Globals Set:
#	none
#
function execute_postpatch
{
	typeset -i retcode=0
	if [[ -x "$3/postpatch" ]]
	then
		/usr/bin/gettext "Executing postpatch script...\n"
		$3/postpatch
		retcode=$?
		if (( retcode != 0 ))
		then
			/usr/bin/gettext "The postpatch script exited with return code $retcode.\n"
			if [[ "$isapplied" = "no" ]]
			then
				$CP $1/$2/log /tmp/log.$2
				/usr/bin/gettext "Backing out patch:\n"
				cd $3
				if [[ "$ROOTDIR" != "/" ]]
				then
					$patchdir/backoutpatch $PKGDBARG $2
				else
					$patchdir/backoutpatch $2
				fi
				/usr/bin/gettext "See /tmp/log.$2 for more details.\n"
			else
				/usr/bin/gettext "Not backing out patch because this is a re-installation.\nThe system may be in an unstable state!\nSee $1/$2/log for more details.\n"  |tee -a $1/$2/log
			fi
			patch_quit 16 "no"
			return 0
		fi
	fi
	return 1
}

# Description:
#       Check to see if the pkginfo command reports an error
#       before using it to evaluate the installed pkgs.
#
# Parameters:
#       none
#
# Globals used
#       PKGDB
#
function chk_pkginfo_cmd
{
	if [[ "$netImage" != "none" ]]
	then
		return
	fi
	prevDir=$(pwd)
	cd $PKGDB
	for pkg in *
    do
		pkginfo -R $ROOTDIR $pkg > /dev/null 2>&1
		if [[ $? -ne 0 ]]
		then
			/usr/bin/gettext "The pkginfo file for package: $pkg is either corrupt or missing.\nA fsck of the file system or re-installing $pkg\nis recommended before installing any patches!\n"
			patch_quit 28 "yes"
		fi
	done
	cd $prevDir
}

# Description:
#	Give a list of applied patches similar in format to the showrev -p
#	command. Had to write my own because the showrev command won't take
#	a -R option.
#
# Parameters:
#	$1	- package database directory
#
# Locals used
#	arg
#	tmpStr
#
# Globals used
#	PatchNum
#	force
#
# Globals Set:
#	ObsoletedBy
#	ThisPatchFnd
#	PatchedPkgs
#	ReqdOSPatchFnd	The patch that this OS release requires was found
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
	typeset -i TestCount=0
	typeset -i ArrayCount=0
	typeset -i PatchFound=0
	typeset -i req_count=0
	typeset -i sr_count=0

	set -A PkgArrElem
	set -A ObsArrElem
	set -A ReqsArrElem
	set -A IncsArrElem
	set -A PatchArrElem
	set -A ShowRevPkg
	set -A ShowRevPatch
	set -A ShowRevObs

	if [[ "$is_an_instpatches" = "yes" ]]
	then
		cat $INSTPATCHES_FILE
		return
	fi

	olddir=$(pwd)
	#
	# First get the old-style patches and obsoletions
	#
	if [[ -d $1 && -d $PATCHDB && $netImage != "product" ]]
	then
		cd $1
		patches=""

		# This gets old and new style patches
		patches=$($GREP -l SUNW_PATCHID ./*/pkginfo 2>/dev/null | \
		    xargs $SED -n 's/^SUNW_PATCHID=//p' | sort -u)

		if [[ "$patches" != "" ]]
		then
			for apatch in $patches
			do
				outstr="Patch: $apatch Obsoletes: "

				# Scan all the installed packages for this
				# patch number and return the effected
				# package instances
				patchvers=$($GREP -l "SUNW_PATCHID=$apatch" \
				    ./*/pkginfo 2>/dev/null | \
				    $SED 's,^./\(.*\)/pkginfo$,\1,' )

				# If there's a PATCH_INFO entry then this
				# is really a direct instance patch
				for package in $patchvers
				do
					break;
				done

				$($GREP -b "PATCH_INFO_$apatch" $package/pkginfo 1>/dev/null 2>&1)
				if [[ $? -eq 0 ]]
				then
					continue
				fi

				PatchFound=1

				# Get the obsoletes list
				obsoletes_printed="n"
				for vers in $patchvers
				do
					if [[ "$obsoletes_printed" = "n" ]]
					then
						outstr="$outstr$($SED -n 's/SUNW_OBSOLETES=//p' \
						    ./$vers/pkginfo) Packages: "
						outstr="$outstr$vers $($SED -n 's/VERSION=//p' \
						    ./$vers/pkginfo)"
						obsoletes_printed="y"
					else
						outstr="$outstr, $vers $($SED -n 's/VERSION=//p' \
						    ./$vers/pkginfo)"
					fi
				done

				# The current patch is a direct instance patch
				if [[ $apatch = "$PatchNum" ]]
				then
					ThisPatchFnd="yes"
				fi

				if [[ "$printpatches" = "yes" ]]
				then
					echo $outstr
				else
					echo $outstr | tee -a $INSTPATCHES_FILE
					is_an_instpatches="yes"
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
	if [[ "$netImage" = "product" ]]
	then
		arg="-d"
	else
		arg="-R"
	fi

	pkginfo $arg  $ROOTDIR | $NAWK ' { print $2; } ' | while read pkg
	do
		DIPatches=$(pkgparam $arg $ROOTDIR $pkg PATCHLIST)
		for patch in $DIPatches
		do
			get_base_code $patch
			patch_base=$cur_base_code

			get_vers_no $patch $patch_base
			patch_vers=$cur_vers_no

			PatchFound=1;

            # Get the obsoletes from each installed package

			tmpStr=""
			tmpStr=$(pkgparam $arg $ROOTDIR $pkg PATCH_INFO_$patch)
			obsoletes=$(echo $tmpStr | $GREP Obsoletes: | $NAWK ' \
			  { print substr($0, match($0, "Obsoletes:")+11) } ' | \
			  $SED 's/Requires:.*//g')

			# Get the requires from each installed package

			reqs=$(echo $tmpStr | $GREP Requires: | $NAWK ' \
			  { print substr($0, match($0, "Requires:")+10) } ' | \
			  $SED 's/Incompatibles:.*//g')

			# Get the incompatibles from each installed package

			incs=$(echo $tmpStr | $GREP Requires: | $NAWK ' \
			  { print substr($0, match($0, "Incompatibles:")+15) } ')

            if [[ -n "$obsoletes" ]]
			then
				for obs in $obsoletes;
				do
					PatchArrElem[$ArrayCount]=$patch;
					ObsArrElem[$ArrayCount]=$obs;
					PkgArrElem[$ArrayCount]=$pkg;
					ArrayCount=ArrayCount+1;
				done
			else
				PatchArrElem[$ArrayCount]=$patch;
				ObsArrElem[$ArrayCount]="";
				PkgArrElem[$ArrayCount]=$pkg;
				ArrayCount=ArrayCount+1;
			fi

			if [[ -n "$reqs" ]]
			then
				for req in $reqs;
				do
					PatchArrElem[$ArrayCount]=$patch;
					ReqsArrElem[$ArrayCount]=$req;
					PkgArrElem[$ArrayCount]=$pkg;
					ArrayCount=ArrayCount+1;
				done
			else
				PatchArrElem[$ArrayCount]=$patch;
				ReqsArrElem[$ArrayCount]="";
				PkgArrElem[$ArrayCount]=$pkg;
				ArrayCount=ArrayCount+1;
			fi
               
			if [[ -n "$incs" ]]
			then
				for inc in $incs;
				do
					PatchArrElem[$ArrayCount]=$patch;
					IncsArrElem[$ArrayCount]=$inc;
					PkgArrElem[$ArrayCount]=$pkg;
					ArrayCount=ArrayCount+1;
				done
			else
				PatchArrElem[$ArrayCount]=$patch;
				IncsArrElem[$ArrayCount]="";
				PkgArrElem[$ArrayCount]=$pkg;
				ArrayCount=ArrayCount+1;
			fi

			# Check for already installed
			if [[ "$patch" = "$PatchNum" ]]
			then
				ThisPatchFnd="yes"
			fi

			if [[ $printpatches != "yes" ]]
			then
				# Check for incompatible patches
				for incompat in $Incompat
				do
					get_base_code $incompat

					if [[ "$patch_base" = "$cur_base_code" ]]
					then
						get_vers_no $incompat $cur_base_code
						if [[ $patch_vers -ge cur_vers_no ]]
						then
							InstIncompat=$patch
						fi
					fi
				done

				# Check for required patches
				if [[ $ReqArrCount -gt 0 && $validate = "yes" ]]
				then
					req_count=0;

					for required in $Requires; do
						get_base_code $required

						if [[ "$patch_base" = "$cur_base_code" ]]
						then
							get_vers_no $required $cur_base_code

							if [[ $patch_vers -ge $cur_vers_no ]]
							then
								ReqArrElem[$req_count]="yes"
							fi
						fi
						req_count=req_count+1
					done
				fi

				for obs_entry in $obsoletes
				do
					get_base_code $obs_entry

					if [[ "$cur_base_code" = "$PatchBase" ]]
					then
						get_vers_no $obs_entry $cur_base_code
						if [[ $cur_vers_no -ge $PatchVers ]]
						then
							ObsoletedBy=$patch
						else
							ObsoletePast=$PatchBase
							ObsoletedBy=$patch
						fi
					fi
				done
			fi
		done
	done

	req_count=0;
	for req in $Requires; do
		if [[ ${ReqArrElem[$req_count]} = "no" ]]
		then
			ReqdPatchCnt=ReqdPatchCnt+1
			UninstReqs="$UninstReqs $req"
		fi
		req_count=req_count+1;
	done

	if [[ $validate = "no" ]]
	then
		ReqdPatchCnt=0
	fi

	while [[ $TestCount -lt $ArrayCount ]]
	do
		typeset -i TempCount=TestCount+1

		# Scan all entries matching the current one
		PatchArrEntry=${PatchArrElem[$TestCount]}	# Current one
		ObsArrEntry=${ObsArrElem[$TestCount]}
		PkgArrEntry=${PkgArrElem[$TestCount]}
		ReqsArrEntry=${ReqsArrElem[$TestCount]}
		IncsArrEntry=${IncsArrElem[$TestCount]}

		if [[ "$PatchArrEntry" = "used" ]]
		then
			TestCount=TestCount+1
			continue
		fi

		while [[ $TempCount -lt $ArrayCount ]]
		do
			typeset -i dont_use;
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

				if [[ $dont_use = 0 ]]
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

				if [[ $dont_use = 0 ]]
				then
					ObsArrEntry="$ObsArrEntry ${ObsArrElem[$TempCount]}"
				fi

				dont_use=0;
 
				for inc in $IncsArrEntry
				do
					if [[ $inc = ${IncsArrElem[$TempCount]} ]]
					then
						dont_use=1;
						break;
					fi
				done
                   
				if [[ $dont_use = 0 ]]
				then
					IncsArrEntry="$IncsArrEntry ${IncsArrElem[$TempCount]}"
				fi

				dont_use=0;
 
				for req in $ReqsArrEntry
				do
					if [[ $req = ${ReqsArrElem[$TempCount]} ]]
					then
						dont_use=1;
						break;
					fi
				done
                  
				if [[ $dont_use = 0 ]]
				then
					ReqsArrEntry="$ReqsArrEntry ${ReqsArrElem[$TempCount]}"
				fi

			fi
			TempCount=TempCount+1
		done

		if [[ $PatchArrEntry = "$PatchNum" ]]; then
			export PatchedPkgs="$PkgArrEntry"
		fi

		# Now make it comma separated lists
		PkgArrEntry=$(echo $PkgArrEntry | $SED s/\ /,\ /g)
		ObsArrEntry=$(echo $ObsArrEntry | $SED s/\ /,\ /g)
		ReqsArrEntry=$(echo $ReqsArrEntry | $SED s/\ /,\ /g)
		IncsArrEntry=$(echo $IncsArrEntry | $SED s/\ /,\ /g)

		outstr="Patch: $PatchArrEntry Obsoletes: $ObsArrEntry \
		  Requires: $ReqsArrEntry Incompatibles: $IncsArrEntry \
		  Packages: $PkgArrEntry"

		if [[ "$printpatches" = "yes" ]]
		then
			echo $outstr
		else
			echo $outstr | tee -a $INSTPATCHES_FILE
			is_an_instpatches="yes"
		fi

		TestCount=TestCount+1
	done
	if [[ $PatchFound = 0 && $printpatches = "yes" ]]
	then
		print " No patches installed."
	fi

	cd $olddir;
}


#
# Description:
#	Validate the patch directory, and parse out the patch number and
#	patch revision from the first pkginfo file found in the patch
#	packages.
# Parameters:
#	$1	- patch directory
# Globals Set:
#	PatchNum
#	PatchBase
#	PatchVers
function activate_patch
{
	cd $1
	for i in */pkginfo
	do
		#
		# Find the patch number in one of the pkginfo files. If there is 
		# no pkginfo file having a SUNW_PATCHID=xxxxxx entry, send an 
		# error to the user and exit.
		#
		tmp=$($GREP PATCHID $i)
		PatchNum=$(pkgparam -f $i ${tmp:%=*})
		if [[ "$multiPtchInstall" = "no" ]]
		then
			lastPtchInList=$PatchNum
		fi
		break;
	done

	if [[ "$PatchNum" = "" ]]
	then
		/usr/bin/gettext "$1 packages are not proper patch packages.\nSee Instructions for applying the patch in the README file.\n"
		patch_quit 7 "no"
		return 0
	else
		#
		# Get the patch base code (the number up to the version prefix) 
		# and the patch revision number (the number after the version prefix).
		#
		get_base_code $PatchNum
		PatchBase=$cur_base_code
		get_vers_no $PatchNum $cur_base_code
		PatchVers=$cur_vers_no
	fi
	return 1
}


# Description:
#   Check to see if there are any files leftover from a previous
#	installation. Set the unconditional flag if there are.
# Parameters:
#   none
# Globals Used:
#   PatchNum
 
function check_file_recovery_dir
{
	if [[ -f $RECOVERDIR/.$PatchNum && "$PatchMethod" = "direct" ]]
	then
		PATCH_UNCONDITIONAL="true"
	fi

}

# Description:
#	Build the admin file for later use by non-interactive pkgadd
# Parameters:
#	none
# Globals Used:
#	ADMINTFILE

function build_admin_file
{
	if [[ "$PatchMethod" = "direct" && -f /var/sadm/install/admin/patch ]]
	then
		ADMINTFILE="patch"
	else
		cat > $ADMINTFILE << EOF
mail=
instance=unique
partial=nocheck
runlevel=nocheck
idepend=nocheck
rdepend=nocheck
space=quit
setuid=nocheck
conflict=nocheck
action=nocheck
EOF
	fi
}

# Description:
#	create a response file if it is necessary
# Parameters:
#	$1	patch type
#	$2	patch method
function build_response_file
{
	if [[ "$1" != "piPatch" ]]
	then
		if [[ "$2" = "progressive" ]]
		then
			cat > $RESPONSE_FILE << EOF
PATCH_PROGRESSIVE=$PATCH_PROGRESSIVE
EOF
		else
			cat > $RESPONSE_FILE << EOF
PATCH_PROGRESSIVE=$PATCH_PROGRESSIVE
PATCH_UNCONDITIONAL=$PATCH_UNCONDITIONAL
PATCH_NO_UNDO=$PATCH_NO_UNDO
PATCH_BUILD_DIR=$PATCH_BUILD_DIR
PATCH_UNDO_ARCHIVE=$PATCH_UNDO_ARCHIVE
INTERRUPTION=$INTERRUPTION
EOF

		fi
	fi
}

# Description:
#	See if there is any work to be done. If none of the packages to
#	which the patch applies are installed and there is no spooling work
#	to do for the client root templates, then you're done.
#	NEW:
#	If SUNWcar, SUNWcsd or SUNWcsr is included in the patch,
#	but the package is not on the list to be patched, then print an
#	error message and die. At least one instance of these packages
#	should be patched if included in the patch.
# Parameters:
#	$1	- client status
#	$2	- were any of the packages root packages?
# Globals Used:
#	pkglist
#	rootlist
#	patchdir

function check_for_action
{
	if [[ "$pkglist" = "" && "$rootlist" = "" ]]
	then
		#
		# In the first case, the system is not a client, however, 
		# there are still no packages to patch. This will only 
		# occur if the packages in question have not been installed 
		# on the system.
		#
		if [[ $1 = "no" || $2 = "yes" ]]
		then
 			/usr/bin/gettext "None of the packages included in patch $PatchNum\nare installed on this system.\n"
			patch_quit 8 "no"
			return 0
		else
			#
			# In the second case, the system is a client system. 
			# There are two types of packages for client systems: 
			# root packages (those packages installed on the client 
			# machines) and packages installed only on the server. 
			# Installpatch will exit if the machine is a client, and 
			# there are no root packages to be patched.
			#
			/usr/bin/gettext "This patch is not applicable to client systems.\n"
			patch_quit 0 "no"
			return 0
		fi
	fi
	return 1
}

# Description:
#	Check to see if the patch has already been applied
# Parameters:
#	$1	- patch database directory
#	$2	- patch number
# Globals Set:
#	isapplied will be set to "yes" if this is a re-application of a patch. This
#	will not necessarily cause a bail out if there are packages that should be
#	installed that were not installed the first time the patch was applied.
#
function check_if_applied
{
	if [[ "$PatchMethod" = "direct" ]]
	then
		if [[ "$ThisPatchFnd" = "yes" && $PATCH_UNCONDITIONAL != "true" ]]
		then
			isapplied="yes"
		else
			$RM -fr $1/$2
		fi
	else
		if eval_inst_patches $PKGDB | $GREP -s "^Patch:[ 	]*$2" > /dev/null 2>&1
		then
			isapplied="yes"
		else
			$RM -fr $1/$2
		fi
	fi
}

# Description:
#   Print space error message
#
function space_error_msg
{
    /usr/bin/gettext "Not enough space in $1 to apply patch. $1 has $2 Kbytes available\n$1 needs $3 Kbytes free.\n"

}

# Description:
#   Check space needed against space available
#
# Parameters:
#	None
#
# Globals Used:
#   ROOTDIR
#	VarFS
#	ClientFS
#	OptFS
#	UsrFS
#	OpenwinFS
#	Root_Kbytes_Needed
#	Var_Kbytes_Needed
#	Opt_Kbytes_Needed
#	Usr_Kbytes_Needed
#	Client_Kbytes_Needed
#	Openwin_Kbytes_Needed
#
# Globals Set:
#   None
#
function check_fs_space
{
	typeset -i Client_Available=0
	typeset -i Opt_Available=0
	typeset -i Openwin_Available=0
	typeset -i Root_Available=0
	typeset -i Usr_Available=0
	typeset -i Var_Available=0
	typeset -i exit_status=0

	if [[ "$DRYRUN" = "no" ]]
	then
		/usr/bin/gettext "Verifying sufficient filesystem capacity (exhaustive method) ...\n"
	else
		/usr/bin/gettext "Verifying sufficient filesystem capacity (dry run method) ...\n"
		return 1
	fi

	#
	# Bear in mind that df -b gives the total kbytes available to
	# the super-user. That means we have to be conservative since
	# there's no pad.
	#
	Tmp_Available=$($DF -b $ROOTDIR | $SED -e '1d')
	Root_Available=${Tmp_Available:#* }

	#
	# The root file system must have at least 1Mb of free
	# space or there will be problems after rebooting
	#
	Root_Available=Root_Available-1000

	if (( Root_Kbytes_Needed > Root_Available ))
	then
		space_error_msg $ROOTDIR $Root_Available $Root_Kbytes_Needed
		exit_status=18
	fi

	if [[ -n "$ClientFS" ]]
	then
		Tmp_Available=$($DF -b $ROOTDIR | $SED -e '1d')
		Client_Available=${Tmp_Available:#* }

		if (( Client_Kbytes_Needed > Client_Available ))
		then
			space_error_msg $ROOTDIR $Client_Available $Client_Kbytes_Needed
			exit_status=18
		fi
	fi

	if [[ -n "$UsrFS" ]]
	then
		Tmp_Available=$($DF -b $ROOTDIR/usr | $SED -e '1d')
		Usr_Available=${Tmp_Available:#* }

		if (( Usr_Kbytes_Needed > Usr_Available ))
		then
			space_error_msg $ROOTDIR/usr $Usr_Available $Usr_Kbytes_Needed
			exit_status=18
		fi
	fi

	if [[ -n "$OptFS" ]]
	then
		Tmp_Available=$($DF -b $ROOTDIR/opt | $SED -e '1d')
		Opt_Available=${Tmp_Available:#* }
		if (( Opt_Kbytes_Needed > Opt_Available ))
		then
			space_error_msg $ROOTDIR/opt $Opt_Available $Opt_Kbytes_Needed
			exit_status=18
		fi
	fi

	if [[ -n "$VarFS" ]]
	then
		Tmp_Available=$($DF -b $ROOTDIR/var | $SED -e '1d')
		Var_Available=${Tmp_Available:#* }
		if (( Var_Kbytes_Needed > Var_Available ))
		then
			space_error_msg $ROOTDIR/var $Var_Available $Var_Kbytes_Needed
			exit_status=18
		fi
	fi

	if [[ -n "$OpenwinFS" ]]
	then
		Tmp_Available=$($DF -b $ROOTDIR/usr/openwin | $SED -e '1d')
		Openwin_Available=${Tmp_Available:#* }
		if (( Openwin_Kbytes_Needed > Openwin_Available ))
		then
			space_error_msg $ROOTDIR/usr/openwin $Openwin_Available $Openwin_Kbytes_Needed
			exit_status=18
		fi
	fi

	if (( exit_status != 0 ))
	then
		patch_quit $exit_status "no"
		return 0
	fi
	return 1
}

# Description:
#	Compute the file system space requirements for /, /var, /opt,
#   /usr, and /usr/openwin to determine if there is enough free space
#   in which to place the patch.
#
# Parameters:
#	None
#
# Globals Used:
#
# Globals Set:
#
function compute_fs_space_requirements
{
	typeset -i size=0

    if [[ "$DRYRUN" = "yes" ]]
    then
		return
	fi 	

	if [[ "$ROOTDIR" != "/" ]]
	then
		ClientFS=$($DF -a $ROOTDIR 2>/dev/null)
	else
		VarFS=$($DF -a /var 2>/dev/null | $GREP var)
		OptFS=$($DF -a /opt 2>/dev/null | $GREP opt)
		UsrFS=$($DF -a /usr 2>/dev/null | $GREP usr)
		OpenwinFS=$($DF -a /usr/openwin 2>/dev/null | $GREP openwin)
	fi

	if [[ -n "$ClientFS" ]]
	then
		# This assume that the -R argument is one file system
		Client_Kbytes_Needed=Usr_Kbytes_Needed+Openwin_Kbytes_Needed+Root_Kbytes_Needed+Var_Kbytes_Needed
		Client_Kbytes_Needed=Client_Kbytes_Needed/1000
	fi

	if [[ -z "$OpenwinFS" ]]
	then
		Usr_Kbytes_Needed=Usr_Kbytes_Needed+Openwin_Kbytes_Needed
		Openwin_Kbytes_Needed=0
	else
		Openwin_Kbytes_Needed=Openwin_Kbytes_Needed/1000
	fi

	if [[ -z "$UsrFS" ]]
	then
		Root_Kbytes_Needed=Root_Kbytes_Needed+Usr_Kbytes_Needed
		Usr_Kbytes_Needed=0
	else
		Usr_Kbytes_Needed=Usr_Kbytes_Needed/1000
	fi

	if [[ -z "$OptFS" ]]
	then
		Root_Kbytes_Needed=Root_Kbytes_Needed+Opt_Kbytes_Needed
		Opt_Kbytes_Needed=0
	else
		Opt_Kbytes_Needed=Opt_Kbytes_Needed/1000
	fi

	Var_Kbytes_Needed=Var_Kbytes_Needed+Kbytes_Required
	if [[ -z "$VarFS" ]]
	then
		Root_Kbytes_Needed=Root_Kbytes_Needed+Var_Kbytes_Needed
		Var_Kbytes_Needed=0
	else
		Var_Kbytes_Needed=Var_Kbytes_Needed/1000
	fi

	Root_Kbytes_Needed=Root_Kbytes_Needed/1000
}

# Description:
#	Generate a list of packages to be installed. Remove from the previously
#	generated $pkglist any packages that have already been patched. This
#	procedure is called only for a patch re-installation.
# Parameters:
#	$1	- package database directory
#	$2	- patch database directory
#	$3	- patch number
# Globals Used:
#	pkglist
# Globals Set:
#	pkglist
function gen_uninstalled_pkgs
{
	pkg=
	for i in $pkglist
	do
		if eval_inst_patches $1 | $GREP "^Patch:[ 	]*$3" | \
			$GREP -s $i > /dev/null 2>&1 ; then
			continue
		else
			pkg="$pkg $i"
		fi
	done
	if [[ "$pkg" = "" ]]
	then
		/usr/bin/gettext "Patch $3 has already been applied.\nSee README file for instructions.\n"
		patch_quit 2 "no"
		return 0
	else
		/usr/bin/gettext "Re-installing patch $3...\n"
		/usr/bin/gettext "\nRe-installing Patch.\n" >> $2/$3/log
	fi
	pkglist="$pkg"
	return 1
}

# Description:
#	Check to see if the patch is obsoleted by an earlier patch
# Parameters:
#	none
# Globals used:
#	PKGDB
#	PatchBase
#	PatchVers
# Globals set:
#	isapplied

function check_if_obsolete
{
	if [[ "$PatchMethod" = "direct" ]]
	then
		if [[ "$ObsoletedBy" = "none" ]]
		then
			return 1
		else 
			print_obsolete_msg "$ObsoletedBy"
			patch_quit 6 "no"
			return 0
		fi
	else
		#
		# Search for patches that specifically obsolete the current 
		# patch.  Ignore if the PatchBase of the obsoletor is the same 
		# as the obsoletee.
		#
		if eval_inst_patches $PKGDB | $GREP -v "Patch: $PatchBase" | \
			$GREP -s "Obsoletes:.*$PatchBase.*Packages:" > /dev/null 2>&1
		then
			print_obsolete_msg "$ObsoletedBy"
			eval_inst_patches $PKGDB | $GREP -v "Patch: $PatchBase" | \
				$GREP "Obsoletes:.*$PatchBase.*Packages:"
			patch_quit 6 "no"
			return 0
		fi
	fi

	currentdir=$(pwd)
	#
	# Now search for patches with the same patch base, but a greater
	# than rev. If an equal to rev, set the isapplied global to "yes"
	#
	oldRevs=
	cd $PKGDB
	oldRevs=$($GREP "SUNW_PATCHID=$PatchBase" ./*/pkginfo 2>/dev/null | \
	         $SED 's/^.*-\([0-9][0-9]\).*$/\1/' | sort -u)
	if [[ "$oldRevs" != "" ]]
	then
		oldRevs=$(echo $oldRevs | sort -u)
		for ii in $oldRevs X
		do
			if [[ "$ii" = "X" ]]
			then
				break;
			fi
			if [[ "$ii" = "$PatchVers" ]]
			then
				isapplied="yes"
				continue
			elif [[ "$ii" -gt "$PatchVers" ]]
			then
				print_obsolete_msg "$PatchBase-$ii"
				patch_quit 6 "no"
				return 0
			fi
		done
	fi

	cd $currentdir
	return 1
}

# Description:
#	Determine if the patch contains any symbolic links. If so, die with
#	an error and a message to the user. I assume the patch will be tested
#	at least once in-house before getting to a non-sun user, so an
#	external user should NEVER see a symbolic link message.
# Parameters:
#	None
# Globals Set:
#	None.
# Globals Used:
#	patchdir
#
function check_for_symbolic_link
{
	$RM -f /tmp/symlink.$$ > /dev/null 2>&1
	olddir=$(pwd)
	cd $patchdir
	for ii in * X
	do
		if [[ "$ii" = X ]]
		then
			break
		fi
		if [[ ! -d "$ii" ]]
		then
			continue
		fi
		#
		# Comment out ignoring symbolic links for packages with no current
		# instance. New packages will not be added using patchadd.
		#
		# $GREP -s "VERSION=.*PATCH=" $1/$2/$ii/pkginfo
		# if [[ $? != 0 ]]; then
		# 	continue
		# fi
		symlinks=
		symlinks=$($SED -n '/^[^ 	]*[ 	]*s[ 	]/p' $1/$2/$ii/pkgmap)
		if [[ "$symlinks" != "" ]]; then
			/usr/bin/gettext "Symbolic link in package $ii.\n" >> /tmp/symlink.$$
		fi
	done
	if [[ -s /tmp/symlink.$$ ]]
	then
		cat /tmp/symlink.$$
		/usr/bin/gettext "Symbolic links cannot be part of a patch.\n"
		patch_quit 13 "no"
		return 0
	fi
	cd $olddir
	return 1
}

# Description:
#	Find package instance of originally-installed package. Extract the
#	PKGID, ARCH, and VERSION by scanning the pkginfo files of each patch
#	package. Check to see if the packages that are being patched were 
#	actually installed on the system in the first place.
# Parameters:
#	$1	- package database directory
#	$2	- patch directory
# Globals Set:
#	pkglist
#	is_a_root_pkg
# Globals Used:
#	pkglist

function check_pkgs_installed
{
	i=
	j=
	pkginst=
	finalpkglist=
	minver=
	Pkgpatchver=
	Pkgarch=
	Pkginst=
	Pkgabbrev=
	Pkgver=
	Pkgtype=
	OrigPkgver=

	# Search the installed pkginfo files for matches with the list 
	# of packages to be patched.  The package names are listed in 
	# global pkglist.  These names correspond to the package database 
	# subdirectory names.
	#
	for i in $pkglist	# for each package in the patch
	do
		#
		# Get the package abbreviation, architecture, version
		# and target filesystem.
		#
		Pkginst=$(basename $i)
		Pkgabbrev=$(pkgparam -f $i/pkginfo PKG)
		Pkgarch=$(pkgparam -f $i/pkginfo ARCH)
		Pkgpatchver=$(pkgparam -f $i/pkginfo VERSION)
		Pkgtype=$(pkgparam -f $i/pkginfo SUNW_PKGTYPE)

		if [[ "$Pkgtype" = "root" && "$service_specified" = "y" ]]
		then
			is_a_root_pkg="yes"
			continue
		elif [[ "$Pkgtype" = "" ]]
		then
			Pkgtype="opt"
		fi

		#
		# First the easy test, see if there's a package by
		# that name installed.
		#
		if [ ! -d "$1/$Pkgabbrev" ] && [ ! -d $1/$Pkgabbrev.* ]
		then
			/usr/bin/gettext "Package not patched:\n" >> $LOGFILE
			/usr/bin/gettext "PKG=$Pkgabbrev\n" >> $LOGFILE
			/usr/bin/gettext "Original package not installed.\n" >> $LOGFILE

			continue
		fi

		#
		# At this point, there's a package of that name
		# installed. So now we have to look for the right
		# architecture and version. This is pretty easy for a
		# direct instance patch. For the progressive instance
		# patch, there's a lot of munging around with the various
		# installed versions.

		if [[ "$netImage" = "product" ]]
		then
			arg="-d"
		else
			arg="-R"
		fi
		if [[ "$PatchMethod" = "direct" ]]
		then
			if [[ $ROOTDIR = "/" ]]
			then
				pkginst=$(pkginfo -a $Pkgarch -v $Pkgpatchver $Pkgabbrev.\* 2>/dev/null | $NAWK ' { print $2 } ')
			else
				pkginst=$(pkginfo $arg $ROOTDIR -a $Pkgarch -v $Pkgpatchver $Pkgabbrev.\* 2>/dev/null | $NAWK ' { print $2 } ')
			fi

			if [[ -n $pkginst ]] 
			then
				finalpkglist="$finalpkglist $i,$pkginst"
			else
				/usr/bin/gettext "Package not patched:\n" >> $LOGFILE
				/usr/bin/gettext "PKG=$Pkginst\n" >> $LOGFILE
				/usr/bin/gettext "Original package not installed.\n" >> $LOGFILE

				continue
			fi

		else
			#
			# Get the package version number.
			#
			Pkgver=$($SED -n \
			   -e 's/^[ 	]*VERSION[ 	]*=[ 	]*\([^ 	]*\)\.[0-9][0-9]*[ 	]*$/\1/p' \
			   -e 's/^[ 	]*VERSION[ 	]*=[ 	]*\([^ 	]*\),PATCH=.*$/\1/p' $i/pkginfo )
			minver=$(expr $Pkgver : '\(.*\)\.0$')
			while [ "$minver" != "" ]
			do
			        Pkgver=$minver
			        minver=$(expr $Pkgver : '\(.*\)\.0$')
			done

			for j in $1/$Pkgabbrev $1/$Pkgabbrev.* X
			do
				if [[ "$j" = "X" ]]
				then
					break
				fi
				if [[ ! -d $j ]]
				then
					continue;
				fi
				OrigPkgver=$($SED -n 's/^VERSION=\(.*\)$/\1/p' $j/pkginfo)
				minver=$(expr $OrigPkgver : '\(.*\)\.0$')
				while [[ "$minver" != "" ]]
				do
					OrigPkgver=$minver
					minver=$(expr $OrigPkgver : '\(.*\)\.0$')
				done
			    if $GREP -s "^PKG=$Pkgabbrev$" $j/pkginfo >/dev/null 2>&1 \
				  && $GREP -s "^ARCH=$Pkgarch$" $j/pkginfo >/dev/null 2>&1 \
				  && [ "$OrigPkgver" = "$Pkgver" ] ;
				then
					pkginst=$(basename $j)
					finalpkglist="$finalpkglist $i,$pkginst"
					break;
				else
					/usr/bin/gettext "Package not patched:\n" >> $LOGFILE
					/usr/bin/gettext "PKG=$Pkgabbrev\n" >> $LOGFILE
					/usr/bin/gettext "ARCH=$Pkgarch\n" >> $LOGFILE
					/usr/bin/gettext "VERSION=$OrigPkgver\n" >> $LOGFILE
					tmp=""
					tmp=$($GREP "^ARCH=$Pkgarch$" $j/pkginfo 2>/dev/null)
					if [[ "$tmp" = "" ]]
					then
						/usr/bin/gettext "Architecture mismatch.\n" >> $LOGFILE
					fi
					if  [[ "$OrigPkgver" != "$Pkgver" ]]
					then
						/usr/bin/gettext "Version mismatch.\n" >> $LOGFILE
					fi
					echo "" >> $LOGFILE
				fi

			done
		fi
	done

	pkglist=$finalpkglist
}

# Description:
#	If validation is being done, and pkgchk reported ERRORs, bail out.
#	If no validation is being done, keep a list of files that failed
#	validation. If this patch needs to be backed out, don't do an installf
#	on these files. Any files that failed validation before the patch was
#	applied should still fail validation after the patch is backed out.
#	This will be the .validation.errors file in the patch directory.
# Parameters:
#	$1	- validation status [ "yes" or "no" ]
# Globals Used:
#	PKGCOFILE
#	VALERRFILE

function check_validation
{
	if [[ "$1" = "yes" && -s $PKGCOFILE ]]
	then
		if $GREP -s ERROR $PKGCOFILE >/dev/null 2>&1
		then
			/usr/bin/gettext "The following validation error was found:\n"
			cat $PKGCOFILE
			/usr/bin/gettext "\nSee the README file for instructions regarding patch validation errors.\n"
			patch_quit 10 "no"
			return 0
		fi
			
	fi
	if [[ -s $VALWARNFILE ]]
	then
		$CP $VALWARNFILE $VALERRFILE
	fi
	return 1
}

# Description:
#	Create the remote file associated with the backout data
# Parameters:
#       $1      - patch database directory
#       $2      - patch number
# Globals Used:
#	PATCH_UNDO_ARCHIVE

function create_remote_file
{
	cat > $1/$2/save/remote << EOF
# Backout data stored remotely
TYPE=filesystem
FIND_AT=$PATCH_UNDO_ARCHIVE/$2/archive.cpio
STATE=N/A
EOF
}

# Description:
# 	Create a spooling area in the sadm/patch/<patchID> tree for files
# 	which are being replaced by the patch. Store the validation error
# 	file with it.
# Parameters:
#	$1	- patch database directory
#	$2	- patch number
# Globals Used:
#	VALERRFILE

function create_archive_area
{
	if [[ "$PATCH_UNDO_ARCHIVE" != "none" ]]; then
		/usr/bin/gettext "Creating patch archive area...\n"
		$MD -p -m 750 $PATCH_UNDO_ARCHIVE/$2
		chown -h -f -R root $PATCH_UNDO_ARCHIVE/$2
		chgrp -h -f -R sys $PATCH_UNDO_ARCHIVE/$2

		$MD -p -m 750 $1/$2/save
		chown -h -f -R root $1/$2
		chgrp -h -f -R sys $1/$2
		create_remote_file $1 $2
	elif [[ ! -d $1/$2/save ]]
	then
		/usr/bin/gettext "Creating patch archive area...\n"
		$MD -p -m 750 $1/$2/save
		chown -h -f -R root $1/$2
		chgrp -h -f -R sys $1/$2
	fi
	if [ -s $VALERRFILE ]
	then
		$CP $VALERRFILE $1/$2/.validation.errors
	fi
}

# Description:
#	Scan the patch package maps for a list of affected files.
# Parameters:
#	$1	- package database directory
#	$2	- package relocation argument
#
# Locals Used
#	arg
#
# Globals Used:
#	PKGCOFILE
#	PATCHFILES
#	pkglist
#
function gen_install_filelist
{
	if [[ "$DRYRUN" = "yes" ]]
	then
		return
	fi

	pkgfiles=/tmp/pkgfiles.$$
	resfiles=/tmp/resolvedfiles.$$
	macrofiles=/tmp/pkgmacros.$$
	pkginst=
	pkginfofile=
	patchpkg=
	basedir=
	i=
	$RM -f $PATCHFILES
	/usr/bin/gettext "Generating list of files to be patched...\n"
	for i in $pkglist
	do
		patchpkg=`expr $i : '\(.*\),.*'`
		pkginst=`expr $i : '.*,\(.*\)'`
		if [[ $pkginst = "" ]]
		then
			continue
		fi

		pkginfofile="$1/$pkginst/pkginfo"
		pkgmapfile="$1/$pkginst/pkgmap"

		# Get the BASEDIR
		basedir=$(pkgparam -f $pkginfofile BASEDIR)

		#
		# Parse out the pkgmap files to get the file names.
		# First, get rid of all checksum info. Then get rid
		# of all info file entries. Replace all BASEDIR values
		# with emptiness (BASEDIR will be prepended).  Delete
		# all entries that are the BASEDIR without a file
		# (directory entries).  Get the file name. If it's a
		# symbolic link, keep the link, don't follow it to the
		# file.
		#
		$SED -e '/^:/d' \
		    -e '/^[^ ][^ ]* i/d' \
		    -e 's, \$BASEDIR/, ,' \
		    -e '/ \$BASEDIR /d' \
		    -e 's/^[^ ]* . \([^ ]*\) \([^ ]*\).*$/\2 \1/' \
		    -e 's/=.*//' $patchpkg/pkgmap > $pkgfiles
		#
		# Resolve any macros in the list of files before determining if
		# the file is relocatable.
		#
		if [[ -s $pkgfiles ]]
		then
			# resolve any macros in the list of files
			(	# different shell
			$RM -f $macrofiles $resfiles

			# Extract every macro that may be meaningful
			# and throw quotes around all of the values
			# assigned.
			$NAWK -F= '
				$1 ~ /PATCHLIST/	{ next; }
				$1 ~ /OBSOLETES/	{ next; }
				$1 ~ /ACTIVE_PATCH/	{ next; }
				$1 ~ /PATCH_INFO/	{ next; }
				$1 ~ /UPDATE/		{ next; }
				$1 ~ /SCRIPTS_DIR/	{ next; }
				$1 ~ /PATCH_NO_UNDO/	{ next; }
				$1 ~ /INSTDATE/		{ next; }
				$1 ~ /PKGINST/		{ next; }
				$1 ~ /OAMBASE/		{ next; }
				$1 ~ /PATH/		{ next; }
				{
					printf("%s=\"%s\"\n", $1, $2);
				} ' $pkginfofile > $macrofiles

			. $macrofiles
			cat $pkgfiles |
			while read i
			do
				eval /usr/bin/echo $i >> $resfiles
			done
			)	# back to original shell

			#
			# Prepend the basedir to the file name if the file is
			# relocatable, then add it to the pkgfile list. 
			#
			$MV $resfiles $pkgfiles
			cat $pkgfiles | parse_sizes $patchpkg

 			$SED -e "s,^\([^/]\),$basedir/&," \
 			    -e 's,\/\/,\/,g' $pkgfiles > $resfiles

			#
			# If there are some files to patch in the package, see if 
			# they have validation errors. Ignore any validation errors 
			# for files having class action scripts. The remaining 
			# validation errors will be put in a validation error file.
			#
			if [[ -s $resfiles && "$PatchType" != "diPatch" ]]
			then
				cat $resfiles |
				while read j
				do
					jfile=$(echo $j | $SED 's/^\([^ ]*\).*/\1/')
					class=$(echo $j | $SED 's/^[^ ]* \(.*\)/\1/')
					badfile=
					badfile=$(pkgchk $2 -p $jfile\
						$patchpkg 2>&1 | \
						$GREP "^ERROR:" | \
						$SED -n 's/^ERROR:[ 	]*//p')
					if [ "$badfile" != "" ]
					then
						if [ "$class" != "" -a "$class" != "preserve" -a ! -f $patchdir/$patchpkg/install/i.$class ]
						then
							pkgchk $2 -p $jfile\
							$patchpkg >> $PKGCOFILE 2>&1
						fi
						echo $jfile >> $VALWARNFILE
					fi
				done
			fi
		 	$SED 's/^\([^ ]*\).*/\1/' $resfiles >> $PATCHFILES
		fi
	done
}

# Description: 
# 	Set flag in case of power outage.
# Parameters: 
# 
# Globals Used:
#	RECOVERDIR
#	INTERUPTION
#	PatchNum
# 
function file_recovery
{ 
	if [[ "$PatchMethod" = "direct" ]]
	then
		if [[ -d "$RECOVERDIR" ]]
		then
			INTERRUPTION="yes"
		else
			$MD $RECOVERDIR 
			$TOUCH $RECOVERDIR/.$PatchNum
			#echo "no" > $RECOVERDIR/.$PatchNum
			sync
		fi
	fi
} 
 
# Description:
#   Used in the file system space calculation.  Determine where each
#   identified file will be placed, and add its size to the correct
#   running total.
# Parameters:
#	$1	- patch package name
# Globals Used:
#	Openwin_Kbytes_Needed
#	Usr_Kbytes_Needed
#	Client_Kbytes_Needed
#	Opt_Kbytes_Needed
#	Var_Kbytes_Needed
#	Root_Kbytes_Needed
#
function parse_sizes
{
	typeset -i size=0
	while read Filename junk
	do
		$GREP " $Filename " $1/pkgmap  |
		while read part ftype f3 f4 f5 f6 f7 f8 Junk
		do
			case $ftype in
				f|e|v)
					pathname=$f4
					size=$f8
					case $pathname in
						usr\/openwin\/*|\/usr\/openwin\/*|openwin\/*|\/openwin\/*)
							Openwin_Kbytes_Needed=Openwin_Kbytes_Needed+size ;;
						usr\/*|\/usr\/*)
							Usr_Kbytes_Needed=Usr_Kbytes_Needed+size ;;
						var\/*|\/var\/*)
							Var_Kbytes_Needed=Var_Kbytes_Needed+size ;;
						opt\/*|\/opt\/*)
							Opt_Kbytes_Needed=Opt_Kbytes_Needed+size ;;
						*)
							Root_Kbytes_Needed=Root_Kbytes_Needed+size ;;
					esac
					;;
	   			i)	
					size=$f4
					Var_Kbytes_Needed=Var_Kbytes_Needed+size
					;;
				d|l|s|p|b|c|x)
					pathname=$f4
					case $pathname in
						usr\/openwin\/*|\/usr\/openwin\/*|openwin\/*|\/openwin\/*)
							Openwin_Kbytes_Needed=Openwin_Kbytes_Needed+512 ;;
						usr\/*|\/usr\/*)
							Usr_Kbytes_Needed=Usr_Kbytes_Needed+512 ;;
						var\/*|\/var\/*)
							Var_Kbytes_Needed=Var_Kbytes_Needed+512 ;;
						opt\/*|\/opt\/*)
							Opt_Kbytes_Needed=Opt_Kbytes_Needed+512 ;;
						*)
							Root_Kbytes_Needed=Root_Kbytes_Needed+512 ;;
					esac
					;;
				*)
					;;
			esac
		done
	done
}

# Description:
#	Generate a list of files which are "to be patched." Determine their
#	total size in bytes to figure out the space requirements of backing
#	them up.
# Parameters:
#	none
# Globals Used:
#	PATCHFILES
#	EXISTFILES
#
function gen_patch_filelist
{
	typeset -i tmp_total=0
	typeset -i kbytes_total=0
	typeset -i kb=0
	size=

	if [[ "$DRYRUN" = "yes" ]]
	then
		return
	fi

	if [[ -s $PATCHFILES ]]
	then
		cat $PATCHFILES |
		while read j
		do
			if $LS -d $ROOTDIR$j >/dev/null 2>&1
			then
				echo "."$j >> $EXISTFILES
				size=$($LS -Ldl $ROOTDIR$j)
				size=$(echo $size | $NAWK ' { print $5 } ')
#				size=$($WC -c $ROOTDIR$j)
#				size=$(echo $size | $SED 's/\ .*//')
				if [ "$size" != "" ]
				then
					tmp_total=tmp_total+$size
				fi
				if (( tmp_total >= 1024 ))
				then
					kb=tmp_total/1024
					tmp_total=tmp_total-kb*1024
					kbytes_total=kbytes_total+kb
				fi
#break
			fi
		done;
		if (( tmp_total > 0 ))
		then
			kbytes_total=kbytes_total+1
		fi
		Kbytes_Required=kbytes_total
	else
		$RM -f $EXISTFILES
	fi
}

# Description:
# 	Assemble a list of the patch package IDs contained in the patch
#	(at least one directory with a pkginfo file must exist due to checks
#	in activate_patch)
# Parameters:
#	none
# Globals Set:
#	pkglist
#
function gen_patchpkg_list
{
	pkg=
	for i in */pkginfo X
	do
		if [ "$i" = "X" ]
		then
			break
		fi

		pkg=`expr $i : '\(.*\)/pkginfo'`
		pkglist="$pkglist $pkg"
	done
}

# Description:
#	Get the product version <name>_<version> of local Solaris installation
# Parameters:
#	$1	target host softinfo directory path
#	$2	managing host softinfo directory path
#	$3	root of the target host
# Globals Set:
#	prodver
#
function get_OS_version 
{
	# If this a patch to a net install image we don't care about 
	# the managing and target host we know it will be a 2.6 or 
	# beyond OS.
	if [[ "$netImage" = "boot" ]]
	then
        MgrProduct="Solaris"
        MgrOSVers=$(uname -r | $SED -n -e 's/5\./2\./p' -e 's/4\./1\./p')
		if [[ "$MgrOSVers" != "2.6" ]]
		then
			/usr/bin/gettext "This system must be running Solaris 2.6\nin order to install a patch to a Net Install Image."
			patch_quit 1 "yes"
		fi
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

	if [[ $3 = "/" ]]	# If there's not a client
	then
		Product=$MgrProduct
		TrgOSVers=$MgrOSVers
		prodver=$Mgrprodver

	# OK, there is a client
	elif [[ "$1" = "none" ]]	# but no softinfo file
	then
		/usr/bin/gettext "patchadd is unable to find the INST_RELEASE file for the target filesystem.\nThis file must be present for patchadd to function correctly.\n"
		patch_quit 11 "yes"
	else
		Product=$($SED -n 's/^OS=\(.*\)/\1/p' $1)
		TrgOSVers=$($SED -n 's/^VERSION=\(.*\)/\1/p' $1)
		prodver=$Product"_"$TrgOSVers
	fi
}

# Description:
# 	Actually install patch packages which apply to the system
# Parameters:
#	$1	- patch database directory
#	$2	- patch number
#	$3	- patch directory
#	$4	- package add relocation argument
#	$5	- package database directory
# Globals Used:
#	ADMINTFILE
#	ADMINFILE
#	pkglist
#
function install_patch_pkgs
{
	typeset -i pkgadderr=0
	typeset -i real_pkgadderr_2=0

	i=
	ij=
	pkginst=
	pkginfofile=
	patchpkg=
	basedir=
	pkgDispList=""
	#
	#	Write out the contents of the logfile if there were any
	#	messages. Do this now, because the $1/$2 directory may not
	#	exist before this point.
	#
	if [ -f $LOGFILE ]
	then
		cat $LOGFILE >> $1/$2/log
		$RM -f $LOGFILE
	fi

	move_libraries

	/usr/bin/gettext "Installing patch packages...\n"
	for ij in $pkglist
	do
		i=`expr $ij : '\(.*\),.*'`
		pkginst=`expr $ij : '.*,\(.*\)'`

		pkginfofile="$5/$pkginst/pkginfo"

		basedir=$($GREP '^BASEDIR' $pkginfofile | $SED -e 's@.*=\ *@@' -e 's@/a/@/@' -e 's@/a$@/@')
		if [ ! -d $1/$2/$i ]
		then
			$MD -m 750 $1/$2/$i
		fi
		$CP $i/pkgmap $1/$2/$i/pkgmap
		$CP $i/pkginfo $1/$2/$i/pkginfo
 		$CP $ADMINTFILE $ADMINFILE
		echo basedir=$basedir >>$ADMINFILE
	
		/usr/bin/gettext "\nDoing pkgadd of $i package:\n"

		if [[ $PatchType = "caPatch" ]]
		then
			$CP $RESPONSE_FILE $RESPONSE_FILE.1
			pkgadd $4 -S -a $ADMINFILE -r $RESPONSE_FILE.1 -n -d $3 $i >$LOGFILE 2>&1
		else
			pkgadd $4 -S -a $ADMINFILE -n -d $3 $i >$LOGFILE 2>&1
		fi
		pkgadderr=$?
		exit_code=$pkgadderr

		if [[ $PatchType = "caPatch" ]]
		then
			$RM -f $RESPONSE_FILE.1
		fi

		real_pkgadderr_2=0
		if (( pkgadderr = 2 ))
		then
			if $GREP '^ERROR' $LOGFILE >/dev/null 2>&1
			then
				real_pkgadderr_2=1
			fi
		fi

		# reboot after installation of all packages
		if (( pkgadderr == 10  || pkgadderr == 20 ))
		then
			/usr/bin/gettext "Reboot after patchadd has installed the patch.\n"
		fi

		cat $LOGFILE >> $1/$2/log
		cat $LOGFILE | $GREP -v "^$"
		$RM -f $LOGFILE
		if (( pkgadderr != 0 && real_pkgadderr_2 != 0 && \
		      pkgadderr != 10 && pkgadderr != 20 ))
		then
			/usr/bin/gettext "Pkgadd of $i package failed with error code $pkgadderr.\n" |tee -a $1/$2/log
			if [ "$isapplied" = "no" ]
			then
				/usr/bin/gettext "See /tmp/log.$2 for reason for failure.\n"
				$CP $1/$2/log /tmp/log.$2
				/usr/bin/gettext "Backing out patch:\n"
				cd $3
				if [ "$ROOTDIR" != "/" ]
				then
					$patchdir/backoutpatch $PKGDBARG $2
				else
					$patchdir/backoutpatch $2
				fi
			else
				/usr/bin/gettext "See $1/$2/log for reason for failure.\nWill not backout patch...patch re-installation.\nWarning: The system may be in an unstable state!\n"
			fi
			patch_quit 5 "no"
			return 0
		fi

		pkgDispList="$pkgDispList $i"

	done
	remove_libraries
	return 1
}

# Description:
#	Make internal variables available to child processes
#   of patchadd.  This is done by writing them to a
#   file and by exporting them.
# Parameters:
#	none
# Environment Variables Set:
#	none
#
function make_params_available
{
	echo "saveold=$saveold" > $PARAMS_FILE
	echo "validate=$validate" >> $PARAMS_FILE
	echo "patchdir=$patchdir" >> $PARAMS_FILE
	echo "patchnum=$PatchNum" >> $PARAMS_FILE
	echo "patchbase=$PatchBase" >> $PARAMS_FILE
	echo "patchrev=$PatchVers" >> $PARAMS_FILE
	echo "ROOTDIR=$ROOTDIR" >> $PARAMS_FILE
	echo "PATCHDB=$PATCHDB" >> $PARAMS_FILE
	echo "PKGDB=$PKGDB" >> $PARAMS_FILE
	echo "PKGDBARG=$PKGDBARG" >> $PARAMS_FILE
	echo "PATCHMETHOD=PatchMethod" >> $PARAMS_FILE
	echo "UNINST_REQS=\"$UninstReqs\"" >> $PARAMS_FILE
	echo "PATCH_UNDO_ARCHIVE=$PATCH_UNDO_ARCHIVE" >> $PARAMS_FILE
	echo "PATCH_BUILD_DIR=$PATCH_BUILD_DIR" >> $PARAMS_FILE
	echo "INTERRUPTION=$INTERRUPTION" >> $PARAMS_FILE

	export saveold validate patchdir PatchNum PatchBase PatchVers
	export PARAMS_FILE ROOTDIR PATCHDB PKGDB PKGDBARG
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
	typeset -i Rev=0

	Rev=$(echo $TrgOSVers | $SED -e 's/[0-9]\.//' -e 's/_.*$//')
	if (( Rev >= 5 ))
	then
		if [ ! -d $TMP_LIB_DIR ]
		then
			$MD -p -m755 $TMP_LIB_DIR
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
		libs_are_moved="yes"
	fi
}


# Description:
#	Find the appropriate softinfo files for the manager and the target.
# Parameters:
#	$1	ROOT of target filesystem
# Globals set:
#	TRGSOFTINFO
#	MGRSOFTINFO
# Globals used:
#	OLD_SOFTINFO
#	NEW_SOFTINFO
function find_softinfos
{
	if [[ "$netImage" = "boot" || "$netImage" = "product" ]]
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
# 	Parse the arguments and set all affected global variables
# Parameters:
#	Argument list passed into patchadd 
# Globals Set:
#	validate
#	saveold
#	force
#	printpatches
#	patchdir
#	ROOTDIR
#	PATCHDB
#	PKGDB
#	PKGDBARG
# Globals Used:
#	Mgrprodver
#	MGRSOFTINO
#	TRGSOFTINFO
#	PKGDB
#	PATCHDB
#
function parse_args
{
	# Inserted for readability reasons
	echo ""
	service_specified="n"
	rootdir_specified="n"
	orig_dir=$(pwd)
	while [ "$1" != "" ]
	do
		case $1 in
		-i) interactive=1; shift ;;
		-u) validate="no"; PATCH_UNCONDITIONAL="true"; shift;;
		-d)	saveold="no"; PATCH_NO_UNDO="true";
			if [[ "$PATCH_UNDO_ARCHIVE" != "none" ]]
			then
				/usr/bin/gettext "The -d option and the -B option are mutually exclusive.\n"
				patch_quit 1 "yes"
			fi	
			shift ;;
		-B)	shift
			if [[ "$PATCH_NO_UNDO" = "true" ]]
			then
				/usr/bin/gettext "The -d option and the -B option are mutually exclusive.\n"
				patch_quit 1 "yes"
			fi
			if [[ -d $1 ]]
			then
				determine_directory $1
				if [[ $ret = 0 ]]
				then
					PATCH_BUILD_DIR=$1
					PATCH_UNDO_ARCHIVE=$1
				else
					PATCH_BUILD_DIR=$curdir
					PATCH_UNDO_ARCHIVE=$curdir
				fi
			else
				/usr/bin/gettext "Specified backout directory $1 cannot be found.\n"
				patch_quit 26 "yes"
			fi
			shift;;
		-p)	printpatches="yes"; shift;;
		-S)	shift
			if [ "$rootdir_specified" = "y" ]
			then
				/usr/bin/gettext "The -S and -R arguments are mutually exclusive.\n"
				print_usage
				patch_quit 1 "yes"
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
					PKGDBARG="-R $ROOTDIR"
				else
					/usr/bin/gettext "The $1 service cannot be found on this system.\n"
					print_usage
					patch_quit 1 "yes"
				fi
			fi
                        service_specified="y"
			shift;;
		-V) echo "@(#) patchadd.ksh 1.4 96/10/15"
			patch_quit 0 "yes";;

		-R)	shift
			if [ "$service_specified" = "y" ]
			then
				/usr/bin/gettext "The -S and -R arguments are mutually exclusive.\n"
				print_usage
				patch_quit 1 "yes"
			fi
			if [ ! -d "$1" ]
			then
				/usr/bin/gettext "The Package Install Root directory $1 cannot be found on this system.\n"
				print_usage
				patch_quit 1 "yes"
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
				rootdir_specified="y"
			fi
			shift;;
        -M) shift
			if [ ! -d "$1" ]
			then
				/usr/bin/gettext "The patch directory $1 cannot be found on this system.\n"
				print_usage
				patch_quit 1 "yes"
			else
				determine_directory $1
                if [[ $ret = 0 ]]
                then
                    multiPtchDir=$1
                else
                    multiPtchDir=$curdir
                fi
				multiPtchInstall="yes"
			fi
			shift;;
        -C) shift
            if [[ "$service_specified" = "y" || "$rootdir_specified" = "y" ]]
            then
                /usr/bin/gettext "The -S, -R and -C arguments are mutually exclusive.\n"
                print_usage
                patch_quit 1 "yes"
            fi 
            if [ ! -d "$1" ]
            then
                /usr/bin/gettext "The path to the net install image $1 cannot be found on this disk.\n"
                print_usage
                patch_quit 1 "yes"
            else
				determine_Product_Boot $1
                determine_directory $1
                if [[ $ret = 0 ]]
                then
                    ROOTDIR=$1
                else
                    ROOTDIR=$curdir
                fi
                PATCHDB=$ROOTDIR$PATCHDB
                PKGDB=$ROOTDIR$PKGDB
                PKGDBARG="-C $ROOTDIR"
				PatchMethod="direct"
            fi
            shift;;

		-*)	print_usage; patch_quit 1 "yes";;
		 *)	if [[ "$multiPtchInstall" = "yes" ]]
			then
				if [[ -d "$multiPtchDir/$1" ]]
				then
					multiPtchList="$multiPtchList $1"
					lastPtchInList=$1
				elif [[ -f "$multiPtchDir/$1" ]]
				then
					process_multi_patch_file "$multiPtchDir/$1"
					multiPtchList=$($NAWK ' { print $1 } ' $multiPtchDir/$1)
					lastPtchInList=$($TAIL -1 $multiPtchDir/$1)
				else
					/usr/bin/gettext "The file $1 cannot be found in $multiPtchDir.\n"
					patch_quit 33 "yes"
				fi
			else
				break
			fi
			shift;;
		esac
	done
	if [[ "$printpatches" = "yes" ]]
	then
		eval_inst_patches $PKGDB
		exit 0
	fi
	if [[ "$1" = "" && "$multiPtchInstall" = "no" ]]
	then
		/usr/bin/gettext "No patch directory specified.\n"
		print_usage
		patch_quit 1 "yes"
	fi
	if [[ ! -d "$1" && "$multiPtchInstall" = "no" ]]
	then
		/usr/bin/gettext "Patch directory $1 does not exist.\n"
		print_usage
		patch_quit 1 "yes"
	fi
	if [[ "$multiPtchInstall" = "no" ]]
	then
		determine_directory $1
		if [[ $ret = 0 ]]
		then
			patchdir=$1
		else
			patchdir=$curdir
		fi
		multiPtchList=$(basename $patchdir)
	fi
}

# Description:
#   Determine if this patch is being applied to the net install
#   images boot or product area.
# Parameters:
#   $1 - The file containing the patches to install.
# Locals Used:
#   file_errors

function process_multi_patch_file
{
	file_errors=

	file_errors=$($NAWK ' { print $2 } ' $1)
	if [[ -n $file_errors ]]
	then
		/usr/bin/gettext "Only one patch per line is allowed in the file\n$1.\n"
		patch_quit 33 "yes"
	fi
}

# Description:
#	Determine if this patch is being applied to the net install
#	images boot or product area. 
# Parameters:
#   $1 - command line argument
# Locals Used:
#	result	
# Globals Used:
#	netImage

function determine_Product_Boot
{
	result=$(basename $1)
	if [[ "$result" = "Boot" ]]
	then
		if [[ -d $1/.tmp_proto ]]
		then
			netImage="boot"
		else
			/usr/bin/gettext "Although this appears to be a valid Net Install Image,\nit doesn't have the neccessary directories."
			patch_quit 31 "yes"
		fi
	elif [[ "$result" = "Product" ]]
	then
		netImage="product"
	else
		/usr/bin/gettext "Argument to the -C command line option is not a valid net install image path."
		patch_quit 31 "yes"
	fi
}

# Description:
# 	Derive the full path name from a (possibly) relative path name.
# Parameters:
#	$1      - command line argument
#
# Globals Used:
#	olddir
#	patchdir  
#	ret

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
			patch_quit 27 "yes"
		fi
	else 
		return
	fi
}

# Description:
#	Print the patch obsolecensce message
# Parameters:
#	$1	- number of patch which obsoleted this patch
#
function print_obsolete_msg
{
	if [[ $1 = "none" ]]
	then
		/usr/bin/gettext "This patch is obsoleted by the following which has already\nbeen applied to this system.\n"
	else
		/usr/bin/gettext "This patch is obsoleted by patch $1 which has already\nbeen applied to this system.\n"
	fi
}

# Description:
#	Print the list of patch packages which were applied, and those
#	which were not.
# Parameters:
#	none
# Globals Used:
#	pkglist
#
function print_results
{
	i=
	p=
	/usr/bin/gettext "\nPatch packages installed:\n"
	#if [[ -z "$pkglist" || "$exit_code" != 0 ]]
	if [[ -z "$pkgDispList" || "$exit_code" != 0 ]]
	then
		/usr/bin/gettext "		none\n\n"
	else
		#for i in $pkglist
		for i in $pkgDispList
		do
			echo "	${i%,*}"
		done
	fi
	echo ""
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
	libs_are_moved="no"
}

# Description:
#	Check space requirements for the backout
#	data for both direct instance and progressive instance patches.
# Parameters:
#	$1      - database directory (PKGDB or PATCHDB)
#	$2      - patch number
#	$3      - patch directory
#	$4      - save old files [ "yes" or "no" ]
# Environment Variables Set:
#
function check_backout_space
{
	typeset -i kbytes_avail=0
	typeset -i buffer=10

	if [[ "$DRYRUN" = "yes" ]]
	then
		return 1
	elif [[ ! -s $EXISTFILES && $1 = "$PATCHDB" ]]
	then
		$TOUCH $1/$2/.nofilestosave
	elif [[ $4 = "yes" ]]
	then
		if [[ "$PATCH_UNDO_ARCHIVE" != "none" ]]
		then
			backout_dir=$PATCH_UNDO_ARCHIVE
		elif [[ "$netImage" = "boot" ]]
		then
			backout_dir=$ROOTDIR
		else
			backout_dir=$1
		fi	

		# Is there enough space? Use sed to extract the fourth field of
		# df output (can't use awk because it may not be installed).

		kbytes_avail=$($DF -b $backout_dir | tail -1 | \
		$SED 's/^[^   ]*[     ]*\([^  ]*\).*/\1/')

		# To build and compress the backout packages in the archive directory
		# takes about 3x as much space then there really needs to be
		# to save just the archive.
		Kbytes_Required=Kbytes_Required*3+buffer
		if (( Kbytes_Required > kbytes_avail ))
		then
			/usr/bin/gettext "Insufficient space in $backout_dir to save old files.\nSpace required in kilobytes:  $Kbytes_Required\nSpace available in kilobytes:  $kbytes_avail\n"
			if [ "$isapplied" = no ]
			then
				cd $3
				if [ "$ROOTDIR" != "/" ]
				then
					$patchdir/backoutpatch $PKGDBARG $2
				else
					$patchdir/backoutpatch $2
				fi
					$RM -fr $backout_dir/$2
			fi
			patch_quit 4 "no"
			return 0
		fi
	fi
	return 1
}

# Description:
# 	Archive files which will be overwritten by the patch application,
#	if the patch actually affects any existing files.
# Parameters:
#	$1	- patch database directory
#	$2	- patch number
#	$3	- patch directory
#	$4	- save old files [ "yes" or "no" ]
# Globals Used:
#	EXISTFILES
#

function save_overwritten_files
{
	typeset -i exit_code=0
	archive_path=

	if [[ -f $1/$2/.nofilestosave ]]
	then
		return 1
	elif [ "$4" = "yes" ]
	then
		/usr/bin/gettext "Saving a copy of existing files to be patched...\n"

		cd $ROOTDIR

		if [[ "$PATCH_UNDO_ARCHIVE" != "none" ]]
		then
			archive_path=$PATCH_UNDO_ARCHIVE/$2
		else
			archive_path=$1/$2/save
		fi

        if [ "$isapplied" = "no" ]
        then
            cpio -oL -O $archive_path/archive.cpio < $EXISTFILES
            exit_code=$?

		else
			if [ ! -d $TMP_ARCHIVE ]
			then
				$MD $TMP_ARCHIVE
			fi

			cd $TMP_ARCHIVE
			if [ -f $archive_path/archive.cpio.Z ]
			then
				zcat $archive_path/archive.cpio.Z | cpio -idum
			else
				cpio -idum -I $archive_path/archive.cpio
			fi
			$FIND . -print > $TMP_FILELIST

			cd $ROOTDIR
			cpio -oL -O /tmp/archive.cpio < $EXISTFILES >/dev/null 2>&1
			exit_code=$?

			cd $TMP_ARCHIVE
			cpio -oAL -O /tmp/archive.cpio < $TMP_FILELIST >/dev/null 2>&1
			exit_code=exit_code+$?

			cd $ROOTDIR
			$RM -rf $TMP_ARCHIVE/* $TMP_FILELIST
			rmdir $TMP_ARCHIVE
		fi
		if (( exit_code != 0 ))
		then
			/usr/bin/gettext "Save of old files failed.\nSee README file for instructions.\n"
			if [ "$isapplied" = "no" ]
			then
				cd $3
				if [ "$ROOTDIR" != "/" ]
				then
					$patchdir/backoutpatch $PKGDBARG $2
				else
					$patchdir/backoutpatch $2
				fi
				$RM -fr $1/$2
			fi
			patch_quit 4 "no"
			return 0
		fi
		if [ -x /usr/bin/compress ]
		then
			if [ "$isapplied" = "no" ]
			then
				compress $archive_path/archive.cpio
			else
				compress /tmp/archive.cpio
			fi
			if [ $? = 0 ]
			then
				/usr/bin/gettext "	File compression used.\n"
			else
				/usr/bin/gettext "	No file compression used.\n"
			fi
		else
			/usr/bin/gettext "	No file compression used.\n"
		fi
		if [ "$isapplied" = "yes" ]
		then
			$CP /tmp/archive.cpio* $1/$2/save
		fi
		chmod 600 $archive_path/archive.cpio*
		$TOUCH $1/$2/.oldfilessaved
		sync
	fi
	cd $3
	return 1
}

# Description:
#	Finish up the patch
# Parameters:
#	$1	- patch database directory
#	$2	- patch number
#
function set_patch_status
{
	if [[ ! -d $1/$2 ]]
	then
		$MD -m 750 -p $1/$2
	fi
	$MV -f /tmp/ACTION.$PatchNum $1/$2 >/dev/null 2>&1
	$CP -p README.$2 backoutpatch $1/$2 >/dev/null 2>&1
	$CP -p prebackout postbackout $1/$2 > /dev/null 2>&1
}

# Description:
# Parameters:
#	$1	- patch database directory
#	$2	- patch number
#	$3	- patch directory
#
function trap_backoutsaved
{
	/usr/bin/gettext "Interrupt signal detected.\n"
	if [[ "$isapplied" = "no" ]]
	then
		/usr/bin/gettext "Backing out patch:\n"
		cd $3
		if [[ "$ROOTDIR" != "/" ]]
		then
			$patchdir/backoutpatch $PKGDBARG $2
		else
			$patchdir/backoutpatch $2
		fi
		$RM -fr $1/$2
	else
		if [[ "$PATCH_UNDO_ARCHIVE" != "none" ]]
		then
			$CP /tmp/archive.cpio* $PATCH_UNDO_ARCHIVE/$2
		else
			$CP /tmp/archive.cpio* $1/$2/save
		fi
		$RM -f /tmp/archive.cpio*
		/usr/bin/gettext "Installpatch Interrupted.\n" >> $1/$2/log
	fi
	patch_quit 12 "yes"
}

# Description:
# Parameters:
#	$1	- patch directory
#	$2	- patch number
#
function trap_backout
{
	/usr/bin/gettext "Interrupt signal detected.\nBacking out Patch:\n"
	cd $1
	if [[ "$ROOTDIR" != "/" ]]
	then
		$patchdir/backoutpatch $PKGDBARG $2
	else
		$patchdir/backoutpatch $2
	fi
	if [[ "$isapplied" = "yes" ]]
	then
		$RM -f /tmp/archive.cpio*
	fi
	patch_quit 12 "yes"
}

# Description:
# Parameters:
# 	$1	- patch database directory
#	$2	- patch number
#
function trap_notinstalled
{
	/usr/bin/gettext "Interrupt signal detected. Patch not installed.\n"
	$RM -f /tmp/*.$$
	$RM -f $INSTPATCHES_FILE
	if [[ "$isapplied" = "no" ]]
	then
		$RM -fr $1/$2
	else
		/usr/bin/gettext "Install Interrupted.\n" >> $1/$2/log
	fi
	patch_quit 12 "yes"
}

# Description:
#	 Make sure effective UID is '0'
# Parameters:
#	none
#
function validate_uid
{
	typeset -i uid
	uid=$(id | $SED 's/uid=\([0-9]*\)(.*/\1/')
	if (( uid != 0 ))
	then
		/usr/bin/gettext "You must be root to execute this script.\n"
		patch_quit 3 "yes"
	fi
}

# Description:
#	Assume that any system on which the SUNWcsu package is NOT
#	installed is a client. It is a safe bet that this criterion
#	will remain valid through Solaris 2.3. Later releases may require
#	that this test be changed. Make sure pkgadd is executable too.
# Parameters:
#	none
# Globals Set:
#	client
#
function verify_client
{
	pkginfo -q SUNWcsu
	if [[ $? != 0 ]]
	then
		client=yes
		sum /usr/sbin/pkgadd > /dev/null 2>&1
		if [[ $? != 0 ]]
		then
			/usr/bin/gettext "The /usr/sbin/pkgadd command is not executable.\nSee the README file for instructions\nfor making this command executable.\n"
			patch_quit 9 "yes"
		fi
	fi
}

# Description:
#	Get key parameters relating to this patch
# Parameters:
#	none
# Globals Set:
#	Obsoletes	those patches that this one obsoletes
#	Incompat	those patches with which this one is incompatible
#	Requires	those patches that this one requires
#	ReqArrElem[]	an ordered mapping of "yes" or "no" attributes
#			associated with each entry in Requires. If it
#			is "yes", that package has been found on the
#			system. If "no", it has not been found.
#	ReqArrCount	The count of elements in the above array
#
# Locals Used:
#	list
#	tmp
#	tmpInstalled
#
function collect_data
{
	tmp=
	list=
	tmpInstalled=

	cd $patchdir
	for pkg in *
	do
		if [[ -f $pkg/pkginfo ]]
		then
			# Collect the data from a pkginfo file
			tmp=$($GREP OBSOLETES $pkg/pkginfo)
			if [[ $tmp = "" ]]
			then
				Obsoletes=""
			else
				Obsoletes=$(pkgparam -f $pkg/pkginfo ${tmp:%=*})
			fi

			tmp=$($GREP INCOMPAT $pkg/pkginfo)
			if [[ $tmp = "" ]]
			then
				Incompat=""
			else
				Incompat=$(pkgparam -f $pkg/pkginfo ${tmp:%=*})
			fi

			tmp=$($GREP REQUIRES $pkg/pkginfo)
			if [[ $tmp = "" ]]
			then
				Requires=""
			else
				Requires=$(pkgparam -f $pkg/pkginfo ${tmp:%=*})
			fi
			for req in $Requires
			do
				ReqArrElem[$ReqArrCount]="no"
				ReqArrCount=ReqArrCount+1;
			done
			break
		fi
	done
}

# Description:
#   Find previously installed patches that the applying patch is
#	incompatible with.
# Parameters:
#   none
#
# Locals Used:
#	list
#	obsPatch
#	incPat
#	obsVer
#	obsBase
#	incVer
#	incBase
#	ReqdPatchFlg
#	tmpStr
#
function eval_compats
{
	if [[ "$PatchMethod" = "progressive" ]]
	then
		return
	fi

	obsPatch=""
	ReqdPatchFlg="false"
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
				obPat=$(echo $tmpStr | $GREP Obsoletes: | $NAWK ' \
				  { print substr($0, match($0, "Obsoletes:")+11) } ' \
				  | $SED 's/Requires:.*//g')

				reqPat=$(echo $tmpStr | $GREP Requires: | $NAWK ' \
				  { print substr($0, match($0, "Requires:")+10) } ' \
				  | $SED 's/Incompatibles:.*//g')

				incPat=$(echo $tmpStr | $GREP Incompatibles: | $NAWK ' \
				  { print substr($0, match($0, "Incompatibles:")+15) } ')

				if [[ -n "$obPat" ]]
				then
					# Check to see if the earlier determined required
					# patch has been obsoleted by an installed patch.
					# If it hasn't set ReqPatchCnt to 0
					# NOTE: if this routine is beefed up a bit function
					# eval_inst_patch can be bypassed for patch
					# installations.

					for ob in $obPat
					do
						for obReq in $Requires
						do
							get_base_code $ob
							obBase=$cur_base_code
							get_base_code $obReq
							obPatchBase=$cur_base_code
							if [[ "$obBase" = "$obPatchBase" ]]
							then
								get_vers_no $ob $obBase
								obVers=$cur_vers_no
								get_vers_no $obReq $obPatchBase
								obPatchVers=$cur_vers_no
								if [[ "$obVers" -le "$obPatchVers" ]]
								then
									ReqdPatchCnt=0
								fi
							fi
						done

						# check the installing patch to see if
						# it is incompatible with a patch that
						# has been installed and has it obsoleted.

						for inc in $Incompat
						do
							get_base_code $ob
							obBase=$cur_base_code
							get_base_code $inc
							obPatchBase=$cur_base_code
							if [[ "$obBase" = "$obPatchBase" ]]
							then
								get_vers_no $ob $obBase
								obVers=$cur_vers_no
								get_vers_no $inc $obPatchBase
								obPatchVers=$cur_vers_no
								if [[ "$obVers" -le "$obPatchVers" ]]
								then
									InstIncompat=$patch
									check_patch_compatibility
								fi
							fi
						done
					done
				fi

				if [[ -n "$reqPat" ]]
				then
					for req in $reqPat
					do
						get_base_code $req
						reqBase=$cur_base_code
						get_vers_no $req $cur_base_code
						reqVers=$cur_vers_no
						incReqObFlag="req"
						check_INC_REQ_OBS "$reqBase" "$reqVers" "$patch" \
						  "$pkg" "$incReqObFlag"
					done
				fi

				if [[ -n "$incPat" ]]
				then
					for inc in $incPat
					do
						get_base_code $inc
						incBase=$cur_base_code
						get_vers_no $inc $cur_base_code
						incVers=$cur_vers_no
						incReqObFlag="inc"
						check_INC_REQ_OBS "$incBase" "$incVers" "$patch" \
						  "$pkg" "$incReqObFlag"
					done
				fi
			done
		fi
	done
 
	cd $patchdir
}

# Description:
#   Check REQUIRE or INCOMPATs that have been obsoleted
# Parameters:
#   none
# Locals Set:
#   obsByPatch
# Globals Set:
#   none
# Parameters:
#   $1  Base code of either INCOMPAT or REQUIRE patch.
#   $2  Version of either INCOMPAT or REQUIRE patch.
#   $3  The patch associated with the INCOMPAT or REQUIRE requirement.
#   $4  The pkg associated with the patch.
#   $5  A flag determining either INCOMPAT or REQ.
#
function check_INC_REQ_OBS
{
	obsByPatch=""
 
	# Check to see if the incompatible/required patch has
	# been obsoleted.
 
	if [[ -f $4/save/$3/obsoleted_by ]]
	then
		obsByPatch=$(/usr/bin/cat \
		  $4/save/$3/obsoleted_by)
	fi	  
 
	# Check to see if this implicit/requires patch obsoletes
	# the installing patch.
	
	if [[ -z "$obsByPatch" ]]
	then
		# We need to check to see if the obsoleted patch
		# is obsoleted by an already installed patch.
	
		for ob in $4/save/$ObsArrEntry
		do
			if [[ -f $ob/obsoleted_by ]]
			then
				obsPatch=$($NAWK ' { print $1 } ' \
				  $ob/obsoleted_by)
			fi
			if [[ -n "$obsPatch" ]]
			then
				break
			fi
		done   
	fi
	
	if [[ "$PatchBase" = "$1" && "$PatchVers" -ge "$2" ]]
	then
		if [[ -n "$obsByPatch" && "$5" = "inc" ]]
		then
			InstIncompat=$obsByPatch
		elif [[ -n "$obsByPatch" && "$5" = "req" ]]
		then
			ReqdPatchCnt=1
			UninstReqs=$obsByPatch
		elif [[ "$5" = "inc" ]]
		then
			InstIncompat=$3
		elif [[ "$5" = "req" ]]
		then
			ReqdPatchCnt=1
			UninstReqs=$3
		fi
		check_patch_compatibility
	fi   
	
	# Check to see if this patch explicitly obsoletes
	# a patch that is INCOMPAT/REQUIRE.
	
	obsPatch=$($GREP OBSOLETE $patchdir/*/pkginfo | \
	  $NAWK -F= ' { print $2 } ')
	for ob in $obsPatch
	do
		get_base_code $ob
		obsBase=$cur_base_code
		get_vers_no $ob $cur_base_code
		obsVers=$cur_vers_no
 
		if [[ "$obsBase" = "$1" && "$obsVers" -ge "$2" ]]
		then
			if [[ -n "$obsByPatch" && "$5" = "inc" ]]
			then
				InstIncompat=$obsByPatch
			elif [[ -n "$obsByPatch" && "$5" = "req" ]]
			then
				ReqdPatchCnt=1
				UninstReqs=$obsByPatch
			elif [[ "$5" = "inc" ]]
			then
				InstIncompat=$3
			elif [[ "$5" = "req" ]]
			then
				ReqdPatchCnt=1
				UninstReqs=$3
			fi
			check_patch_compatibility
		fi
	done
}

# Description:   
#   Construct the files needed to backout the patch for dryrun mode.
# Parameters:
#   none    
# Globals Set:   
#   none    
# Parameters:
#	$1	Dryrun directory
#
function construct_backout_files {
	$NAWK ' { print $2 } ' $1/dryrun.ipo.asc > $PATCHFILES
}

# Description:
#   Check to see if there will be enough space for the backout pkg(s).
# Parameters:
#   none
# Globals Set:
#   none
# Locals Used:
#	spaceRequired
# Parameters:
#	$1 Dryrun Directory
#
function check_dryrun_backoutSpace {
	typeset -i totalBlocksNeeded=0
	typeset -i totalBytesNeeded=0
	typeset -i dryrunKbytesAvail=0
	typeset -i dryrunBytesAvail=0
	typeset -i used

    spaceNeeded=$($NAWK ' $1 !~ /\/tmp|name|FSUSAGE|\\/ { print $5 } ' \
	  $1/dryrun.fs.asc )

	for used in $spaceNeeded
	do
		totalBlocksNeeded=used+totalBlocksNeeded
	done

	totalBytesNeeded=totalBlocksNeeded*512
	
	if [[ "$PATCH_UNDO_ARCHIVE" != "none" ]]
    then
        backout_dir=$PATCH_UNDO_ARCHIVE
    else
        backout_dir=$PKGDB
    fi

    # Is there enough space? Use sed to extract the fourth field of
    dryrunKbytesAvail=$($DF -b $backout_dir | tail -1 | \
    $SED 's/^[^   ]*[     ]*\([^  ]*\).*/\1/')

	dryrunBytesAvail=dryrunKbytesAvail*1024

    # To build and compress the backout packages in the archive directory
    # takes about 3x as much space then there really needs to be
    # to save just the archive.
    totalBytesNeeded=totalBytesNeeded*3+10

    if (( totalBytesNeeded > dryrunBytesAvail ))
    then
		totalKbytesNeeded=totalBytesNeeded/1024
        /usr/bin/gettext "Insufficient space in $backout_dir to save old files.\nSpace required in kilobytes:  $totalKbytesNeeded\nSpace available in kilobytes:  $dryrunKbytesAvail\n"
        $RM -fr $backout_dir/$2
		patch_quit 4 "no"
		return 0
    fi
	return 1
}

# Description:
#   Evaluate the dry run data
# Parameters:
#   none
# Globals Set:
#   none
# Locals Used:
#   dryrunExit
#   dryrunDir
#
function eval_dryrun {
	dryrunExit=
	dryrunDir=

	dryrunDir="/tmp/$PatchNum.$$"

	dryrunExit=$($NAWK -F= ' $1 ~ /^EXITCODE$/ { print $2 } ' $dryrunDir/dryrun.isum.asc )

	if [[ "$dryrunExit" != "0" ]]
	then   
		eval_dryrun_failures "$dryrunDir"
		dryrunFailure="yes"
	fi

	if [[ "$saveold" = "yes" ]]
	then
		construct_backout_files "$dryrunDir"
		if check_dryrun_backoutSpace "$dryrunDir"
		then
			dryrunFailure="yes"
		fi
	fi
}

# Description:
#   Evaluate the dry run failures.
# Parameters:
#   $1  Dryrun directory
# Globals Set:
#   none
# Locals Used:
#   dryrunFailures
#   failed
#   exitCheck
#
function eval_dryrun_failures  {
 
	failed=

    /usr/bin/gettext "The following errors were reported by pkgadd dryrun...\n\n"
 
    exitCheck=$($NAWK -F= ' $2 ~ /!0/ { print $1 } ' $1/dryrun.isum.asc )
    for code in $exitCheck
    do
        case $code in
            CHECKINSTALLEXITCODE)   /usr/bin/gettext "  The checkinstall script failed.\n" ;;
            REQUESTEXITCODE)        /usr/bin/gettext "  The request script failed.\n" ;;
        esac
    done
 
    dryrunFailures=$($NAWK -F= ' $2 ~ /NOT_OK/ { print $1 } ' $1/dryrun.isum.asc )
    failed="  Installation failed due to"
    for param in $dryrunFailures
    do
        case $param in
            SPACE)      /usr/bin/gettext "$failed lack of space reported by pkgadd dryrun\n" ;;
            PARTIAL)    /usr/bin/gettext "$failed partial install reported by pkgadd dryrun\n" ;;
            RUNLEVEL)   /usr/bin/gettext "$failed incorrect run level\n" ;;
            PKGFILES)   /usr/bin/gettext "$failed bad pkg reported by pkgadd dryrun\n" ;;
            DEPEND)     /usr/bin/gettext "$failed incorrect depend file\n" ;;
            CONFLICT)   /usr/bin/gettext "$failed conflicts reported by pkgadd dryrun\n" ;;
            SETUID)     /usr/bin/gettext "$failed incorrect uid\n" ;;
            PKGDIRS)    /usr/bin/gettext "$failed package directories not found\n" ;;
        esac
    done
 
	patch_quit 30 "no"
}

# Description:
#	Setup the net install boot image to look like an installed system.
# Parameters:
#	none
# Globals Set:
#	none
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

	# At this point patchadd thinks the net install image is just like an installed image.
}

# Description:
#	Restore the net image to the way it was before mucking 
#	with it in the setup_net_image function.
# Parameters:
#	none
# Globals Set:
#	none
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

# Description:
#	Patch the product Database on the netinstall image.
# Parameters:
#	none
# Globals Set:
#	none
#
function patch_product {

	if [[ "$netImage" != "product" ]]
	then
		return
	fi

	eval_utilities
	activate_patch "$patchdir"
	cd $patchdir

	collect_data

	# If this is the first patch to the product area create the patch repository.
	if [[ ! -d "$PATCHDB" ]]
	then
		$MD -p -m 750 $PATCHDB
	fi

	# Check to see if the pkg to patch is in the product area
	# and all VERSION and ARCH are the right instance.
	eval_inst_patches $PKGDB 
	check_patch_compatibility
	check_if_applied "$PATCHDB" "$PatchNum"
	check_if_obsolete "$PATCHDB"    # see if this package is already obsolete
	if [[ "$isapplied" = "yes" ]]
	then
		eval_applied_patch $PATCHDB $PKGDB $PatchMethod $saveold $PatchNum
	fi
	gen_patchpkg_list
	check_pkgs_installed "$PKGDB" "$patchdir"

	#### Make sure what PKGDBARG is !!!
	gen_install_filelist "$PKGDB" "$PKGDBARG"
	gen_patch_filelist

	check_reloc_dir
	merge_pkgmaps
	
}

# Description:
#	Merge the PATCHLIST with each pkgmap called out by the pkg.
# Parameters:
#	none
# Globals Set:
#	none
#
function merge_pkgmaps {

	for pkg in $pkglist
	do
		echo "test"	
		cd $ROOTDIR/$pkg
		$NAWK ' 
			$1 ~ /[:]/ { 		# size line
				if (NF == 3) {	# if uncompressed
					uncompress="yes" }
				} ' pkgmap
		if [[ "$uncompress" != "yes" ]]
		then
			zcat reloc.cpio.Z | cpio -idum	
		fi
		$DIFF pkgmap $patchDir/$pkg/pkgmap | $NAWK ' $1 ~ /[>]/ {
			if ($3 == "i") { next; }
			if ($3 == "v" || $3 == "e") { next; }
			{ print $2 $3 $4 $5 $6 $7 $8 } } ' > tmpPkgmap
		cat tmpPkgmap |
		while read line 
		do
			$GREP $line pkgmap
			if [[ $? = 0 ]]
			then
				$SED 's/$line/'
			fi
		########
		## Continue with merging the maps
		########				

		done
				
	done
}

# Description:
#	Ceck to see if the reloc directory is compressed, if it is
#	decompress it and poke in the objects being replaced by the patch.
# Parameters:
#	none
# Globals Set:
#	none
#
function check_reloc_dir {

	for pkg in $pkglist
	do
		cd $ROOTDIR/$pkg
		if [[ -d reloc ]]
		then
			cd reloc
		fi
		if [[ -f reloc.cpio.Z ]]
		then
			zcat reloc.cpio.Z | cpio -idum 
		else
			cpio -idum -l reloc.cpio
		fi
		
	done
}

# Description:
#   Check exit code from pkgadd
# Parameters:
#   $1  exitcode from pkgadd
# Globals Set:
#   none 
#
function check_pkgadd_exitcode {
 
    pkgadd_code=$1
 
    # If it's a suspend (exit code 4), then the
    # message type is the appropriate patchadd
    # exit code and the appropriate message follows.
    # A suspend means, nothing has been installed.
    if [[ $pkgadd_code == 4 ]]  # suspend
    then 
        Message=$($EGREP PaTcH_MsG $LOGFILE | $SED s/PaTcH_MsG\ //)
        if [[ $Message = "" ]]
        then
            pkgadd_code=5
        else
            Msg_Type=$(echo $Message | $NAWK ' { print $1 } ')
            Message=$(echo $Message | $SED s/$Msg_Type\ //)
 
            /usr/bin/gettext "$Message/n" >> $LOGFILE
            /usr/bin/gettext "$Message"
			if [[ "$lastPtchInList" != "$PatchNum" ]]
			then
				# If there are more pkgs in the list skip them since this patch
				# will not be installed.
				/usr/bin/gettext "Skipping patch $PatchNum"
        		$patchdir/backoutpatch -R $ROOTDIR $PatchNum
        		Something_Installed=0
				pkg=
				return 1
			else
            	patch_quit $Msg_Type "no"
				return 0
			fi
        fi
    fi
 
    # reboot after installation of all packages
    if [[ $pkgadd_code == 10  || $pkgadd_code == 20 ]]
    then 
        /usr/bin/gettext "Reboot after patchadd has installed the patch.\n"
    fi
 
    if [[ $pkgadd_code == 5 ]]  # administration
    then 
        cat $LOGFILE
		if [[ "$lastPtchInList" != "$PatchNum" ]]
		then
			# If there are more pkgs in the list skip them since this patch
			# will not be installed.
			/usr/bin/gettext "Skipping patch $PatchNum"
        	$patchdir/backoutpatch -R $ROOTDIR $PatchNum
        	Something_Installed=0
			pkg=
			return 1
		else
        	$patchdir/backoutpatch -R $ROOTDIR $PatchNum
        	Something_Installed=0
		fi
    elif [[ $pkgadd_code != 0 ]]
    then
        $EGREP ERROR $LOGFILE
		if [[ "$lastPtchInList" != "$PatchNum" ]]
		then
			# If there are more pkgs in the list skip them since this patch
			# will not be installed.
			/usr/bin/gettext "Skipping patch $PatchNum"
        	$patchdir/backoutpatch -R $ROOTDIR $PatchNum
        	Something_Installed=0
			pkg=
			return 1
		else
        	$patchdir/backoutpatch -R $ROOTDIR $PatchNum
        	Something_Installed=0
        	patch_quit 5 "no"
			return 0
		fi
    else
        Something_Installed=1
    fi
	return 1
}

# Description:
#	Apply a direct instance patch
# Parameters:
#	none
# Globals Set:
#	pkgDispList
#
function apply_diPatch {	
	exit_code=0
	pkgDispList=""

	ReqArrCount=0
	firstTimeThru="yes"
	cd $patchdir
	curdir=$(pwd)

	# strip the installed instance out of the package list
	for pkg in $pkglist
	do
		newpkglist="$newpkglist ${pkg:%,*}"
	done

	pkglist=$newpkglist

	/usr/bin/gettext "Installing patch packages...\n"

	# actually install the packages
	#
	for pkg in $pkglist
	do
		if [[ -f $pkg/pkginfo ]]	# If this is a package
		then
			$CP $RESPONSE_FILE $RESPONSE_FILE.1
			if [[ "$DRYRUN" = "no" ]]
			then   
				pkgadd -S -n -a $ADMINTFILE -r $RESPONSE_FILE.1 \
			      -R $ROOTDIR -d . $pkg 1>$LOGFILE 2>&1
				exit_code=$?
			else
				if [[ "$firstTimeThru" = "yes" ]]
				then
					pkgadd -D /tmp/$PatchNum.$$ -S -n -a $ADMINTFILE \
					  -r $RESPONSE_FILE.1 \
			    	  -R $ROOTDIR -d . $pkg 1>$LOGFILE 2>&1
					exit_code=$?
					if [[ "$exit_code" = "0" ]]
					then
						firstTimeThru="no"	
						eval_dryrun
						if [[ "$dryrunFailures" = "yes" ]]
						then
							return 0
						fi
					else
            			if check_pkgadd_exitcode "$exit_code"
						then
							return 0
						fi
					fi
				fi
                pkgadd -S -n -a $ADMINTFILE -r $RESPONSE_FILE.1 \
                -R $ROOTDIR -d . $pkg 1>$LOGFILE 2>&1
                exit_code=$?
			fi
           	if check_pkgadd_exitcode "$exit_code"
			then
				return 0
			fi
		fi
		$RM -f $RESPONSE_FILE.1

		if [[ "$exit_code" = "0" ]]
		then
			pkgDispList="$pkgDispList $pkg"
		fi

	done
	
	if (( Something_Installed == 1 ))
	then
		cd $ROOTDIR
		cd var/sadm/pkg

		InstPkgs=$(pkginfo -R $ROOTDIR | $NAWK '
		    { printf ("%s ", $2) }
		END { printf("\n") } ')

		#
		# With that done successfully, obsolete explicitly
		# listed prior patches.
		#
		for patch in $Obsoletes
		do
			get_base_code $patch
			patch_base=$cur_base_code

			get_vers_no $patch $patch_base
			patch_vers=$cur_vers_no

			for pkg in $InstPkgs
			do
				#
				# Locate all applicable obsoleted patches
				# by searching for entries with identical
				# base codes and versions greater than or
				# equal to the one specified.
				#
				cd $pkg/save

				patch_list=$($LS -db $patch_base* 2>/dev/null)

				for cur_patch in $patch_list
				do
					get_vers_no $cur_patch $patch_base
					if [[ $cur_vers_no -gt $patch_vers ]]
					then
						/usr/bin/gettext "WARNING: Later version of obsolete patch $patch was found.\nLeaving $cur_patch as is.\n"
						continue;
					fi

					if [[ -f $cur_patch/undo ]]
					then
						$MV $cur_patch/undo $cur_patch/obsolete
						echo $PatchNum >> $cur_patch/obsoleted_by
					elif [[ -f $cur_patch/undo.Z ]]
					then
						$MV $cur_patch/undo.Z $cur_patch/obsolete.Z
						echo $PatchNum >> $cur_patch/obsoleted_by
						elif [[ -f $cur_patch/remote ]]
                        then
                            set_remote_state $pkg $cur_patch
                            check_remote_file $pkg $cur_patch
                            echo $PatchNum >> $cur_patch/obsoleted_by
					elif  [[ -f $cur_patch/obsolete || -f $cur_patch/obsolete.Z ]]
					then
						$GREP $PatchNum $cur_patch/obsoleted_by >/dev/null
						if [[ $? -ne 0 ]]
						then
							echo $PatchNum >> $cur_patch/obsoleted_by
						fi
					fi
				done

				cd $ROOTDIR
				cd var/sadm/pkg
			done
		done
		/usr/bin/gettext "\nPatch number $PatchNum has been successfully installed.\n"
	else
		/usr/bin/gettext "Installation of patch number $PatchNum has been suspended.\n"
        if [ -f $LOGFILE ]
        then
            if [[ ! -d $PATCHDB/$PatchNum ]]
            then
                $MD $PATCHDB/$PatchNum
            fi 
            /usr/bin/gettext "See $PATCHDB/$PatchNum/log for details\n"
            $CP -p $LOGFILE $PATCHDB/$PatchNum/log
            $RM -f $LOGFILE
        fi
    fi
	
	$RM -f $RESPONSE_FILE
	cd $curdir
	return 1
}

# Description:
#	Determine which patch is required for this OS release to work.
# Parameters:
#	$1	Solaris release of the managing host
#	$2	The patch method to use
# Globals Set:
#	ReqdOSPatch	patch number that this OS requires
#	ReqdOSPatchBase	base number of the above patch
#	ReqdOSPatchVers	version number of the above patch
#
function ident_reqd_patch {
	if [[ "$PatchMethod" = "direct" ]]
	then
		case $1 in
			"2.0")
				/usr/bin/gettext "ERROR: Solaris 2.0 is not capable of installing patches\nto a 2.5 or later client.\n";
				patch_quit 21 "yes";;
			"2.1")
				/usr/bin/gettext "ERROR: Solaris 2.1 is not capable of installing patches\nto a 2.5 or later client.\n";
				patch_quit 21 "yes";;
			"2.2")
				ReqdOSPatch="101122-07";;
			"2.3")
				ReqdOSPatch="101331-06";;
			"2.4")
				MgrPlatform=$(uname -p);
				case $MgrPlatform in
					"sparc")
						ReqdOSPatch="102039-04";;
					"i386")
						ReqdOSPatch="102041-04";;
				esac;;
		esac

		if [[ "$ReqdOSPatch" != "none" ]]
		then
			get_base_code $ReqdOSPatch
			ReqdOSPatchBase=$cur_base_code;

			get_vers_no $ReqdOSPatch $cur_base_code
			ReqdOSPatchVers=$cur_vers_no;

			cd /var/sadm/patch
			for apatch in *
			do
				get_base_code $apatch
				if [[ "$ReqdOSPatchBase" = "$cur_base_code" ]]
				then
					get_vers_no $apatch $cur_base_code
					if [[ "$ReqdOSPatchVers" -le "$cur_vers_no" ]]
					then
						ReqdOSPatchFnd="true"
					fi
				fi
			done
		fi
	fi

	cd $patchdir
}

# Description:
#	Evaluate the patch provided and return the patch type.
# Parameters:
#	$1 - patch directory
# Globals Set:
#	PatchType	one of:
#				diPatch		direct instance patch
#				caPatch		cross architecture patch
#				piPatch		progressive instance patch
#
function eval_patch {
	if [[ -f ${1}/.diPatch ]]
	then
		if [[ -d ${1}/old_style_patch ]]
		then
			PatchType="caPatch"
		else
			PatchType="diPatch"
		fi
	else
		PatchType="piPatch"
	fi
}

# Description:
#   Evaluate the patch methodology to be used based upon the
#	Solaris version of the manager and target hosts.
# Parameters:
#	$1	Managing host OS version
#	$2	Target host OS version
#	$3	patch type
# Globals Set:
#	PatchMethod	one of:
#				direct		direct instance method
#				progressive	progressive instance method
#	PATCH_PROGRESSIVE
#
function eval_OS_version {
    if [[ "$1" > "2.5" ]]
    then
        if [[ "$2" < "2.5" ]]
        then
            PatchMethod="progressive"
            PATCH_PROGRESSIVE="true"
        else
            if [[ $3 = "diPatch" || $3 = "caPatch" ]]
            then
                PatchMethod="direct"
                PATCH_PROGRESSIVE="false"
				if [[ "$1" > "2.5.1" ]]
				then
					DRYRUN="yes"
				fi
			else
				PatchMethod="progressive"
				PATCH_PROGRESSIVE="true"
            fi
        fi

	elif [[ "$1" < "2.5" ]]
	then
		if [[ "$2" < "2.5" ]]
		then
			PatchMethod="progressive"
			PATCH_PROGRESSIVE="true"
		else
			if [[ $3 = "diPatch" || $3 = "caPatch" ]]
			then
				PatchMethod="direct"
				PATCH_PROGRESSIVE="false"
			else
				PatchMethod="progressive"
				PATCH_PROGRESSIVE="true"
			fi
		fi
	else
		if [[ "$2" < "2.5" ]]
		then
			if [[ $PatchType = "diPatch" ]]
			then
				/usr/bin/gettext "ERROR: This direct instance patch cannot be installed\nonto a Solaris $TrgOSVers host.\n"
				patch_quit 21 "yes"
			else
				PatchMethod="progressive"
				PATCH_PROGRESSIVE="true"
			fi
		else
			if [[ $3 = "diPatch" || $3 = "caPatch" ]]
			then
				PatchMethod="direct"
				PATCH_PROGRESSIVE="false"
				if [[ "$2" > "2.5.1" ]]
				then
					DRYRUN="yes"
				fi
			else
				PatchMethod="progressive"
				PATCH_PROGRESSIVE="true"
			fi
		fi
	fi
}

# Description:
#       Evaluate the applied patch to be sure we aren't going to hose up 
#	any existing backout data.
#
#	If this is a progressive instance patch, here's how it is evaluated:
# prev  | curr | .nofilestosave |   OK to   |  How to verify previous
# save  | save |    exist?      | continue? |    save/no_save state
#-------+------+----------------+-----------+--------------------------
#       |      |      yes       |           | a. empty save directory
# 1 yes | yes  |----------------| continue  +--------------------------
#       |      |       no       |           | b. ! empty save directory
#-------+------+----------------+-----------+--------------------------
#       |      |      yes       | continue  | a. empty save directory
# 2 yes |  no  |----------------+-----------+--------------------------
#       |      |       no       | terminate | b. ! empty save directory
#-------+------+----------------+-----------+--------------------------
# 3  no |  no  |       no       | continue  |    empty save directory
#-------+------+----------------+-----------+--------------------------
# 4  no | yes  |       no       | terminate |    empty save directory
#-------+------+----------------+-----------+--------------------------
#
#	And the direct instance patch is evaluated as follows:
#  prev  | curr |   OK to   |  How to verify previous
#  save  | save | continue? |    save/no_save state
# -------+------+-----------+------------------------------------------
# A.  no | yes  | terminate | ! -d /var/sadm/pkg/<pkg>/save/<patch_id>
# -------+------+-----------+------------------------------------------
# B.  no |  no  | continue  | ! -f /var/sadm/pkg/<pkg>/save/<patch_id>
# -------+------+-----------+------------------------------------------
# C. yes |  --  | continue  |   -f /var/sadm/pkg/<pkg>/save/<patch_id>
# -------+------+-----------+------------------------------------------
#
# Parameters:
#	$1	patch database directory
#	$2	package database directory
#	$3	the patch method
#	$4	the saveold parameter value
#	$5	the patch number
# Globals Set:
#
function eval_applied_patch { 
	if [[ "$3" = "progressive" ]]
	then
		if [[ "${4}" = "no" ]]
		then
			if [ ! -f "${1}/${5}/.nofilestosave" -a \
			    \( -f "${1}/${5}/save/archive.cpio" -o \
			    -f "${1}/${5}/save/archive.cpio.Z" \) ]
	       		then
				# condition #2b - terminate
				/usr/bin/gettext "A previous installation of patch ${5} was invoked which saved\nfiles that were to be patched.\nSince files have already been saved, you must apply this patch\nWITHOUT the -d option.\n"
				patch_quit 17 "no"
				return 0
			elif [ -f "${1}/${5}/.nofilestosave" -a \
			    ! -f "${1}/${5}/save/archive.cpio" -a \
			    ! -f "${1}/${5}/save/archive.cpio.Z" -a \
			    ! -f "${1}/${5}/save/remote" ]
			then
				# condition #2a - $RM .nofilestosave
				$RM ${1}/${5}/.nofilestosave
			fi
		else	# ${4} = "yes"
			if [ ! -f "${1}/${5}/.nofilestosave"    -a \
			    ! -f "${1}/${5}/save/archive.cpio" -a \
			    ! -f "${1}/${5}/save/archive.cpio.Z" -a \
			    ! -f "${1}/${5}/save/remote" ]
			then
				# condition #4 - terminate
				/usr/bin/gettext "A previous installation of patch ${5} was invoked with the -d option.\ni.e. Do not save files that would be patched\nTherefore, this invocation of patchadd\nmust also be run with the -d option.\n"
				patch_quit 17 "no"
				return 0
			fi
		fi
	else	# $3 != "progressive"
		$FIND ${2}/. -name "${5}" -print >/dev/null 2>&1
		prev_save=$?
		if [[ $prev_save != 0 && "${4}" = "yes" ]]
		then
			# condition A.
			/usr/bin/gettext "A previous installation of patch ${5} was invoked\nwith the -d option. i.e. Do not save files that would be patched\nTherefore, this invocation of patchadd\nmust also be run with the -d option.\n"
			patch_quit 17 "no"
			return 0
		fi
	fi
	return 1
}

function eval_utilities {
	for command in $REQD_CMDS; do
		if [[ ! -f $command ]]
		then
			/usr/bin/gettext "ERROR: Cannot find $command which is required for proper execution of patchadd.\n"
			patch_quit 1 "yes"
		fi
	done		
}

# Description:
#       Check the remote file to see if the remotely stored backout data
#	location needs to be changed.
# Parameters:
#       $1      - package associated with the patch
#       $2      - the patch number
#
# Environment Variable Set:
#
function check_remote_file
{
	if [[ "$PatchMethod" = "direct" ]]
        then
                if [[ -f $PKGDB/$1/save/$2/remote && -s $PKGDB/$1/save/$2/remote ]]
                then
                        PATCH_UNDO_ARCHIVE=$($GREP "FIND_AT" $PKGDB/$1/save/$2/remote | $AWK -F= '{print $2}')
                        PATCH_UNDO_ARCHIVE=$(dirname $PATCH_UNDO_ARCHIVE)
                fi
        else
                # Add logic for pi patches
                echo $PATCH_UNDO_ARCHIVE > /dev/null
        fi
}

# Description:
#       Change the STATE parameter to an obsolete state
# Parameters:
#       $1      - package associated with the patch
#       $2      - the patch number
#
# Environment Variable Set:
#	none

function set_remote_state
{
        $($GREP . $PKGDB/$1/save/$2/remote | $SED 's/STATE=.*/STATE=obsolete/' > $TEMP_REMOTE)
        $RM -f $PKGDB/$1/save/$2/remote
        $MV $TEMP_REMOTE $PKGDB/$1/save/$2/remote
        $RM -f $TEMP_REMOTE

}

# Description:
#       Determine if the patch is a progressive instance patch
# Parameters:
#	none
#
# Environment Variable Set:
#	none 

function is_progressive
{
	if [[ "$PatchMethod" = "progressive" ]]
	then
		echo $PatchNum | $GREP $PatchIdFormat >/dev/null
		if [[ $? -ne 0 ]]
		then
			/usr/bin/gettext "Invalid patch id format: $PatchNum.\n"
			patch_quit 29 "no"
			return 0
		fi
		if [[ "$PatchType" = "diPatch" ]]
		then
			/usr/bin/gettext "ERROR:  A progressive instance patch is required\nbut patch number $PatchNum is direct instance only\nand can only be installed onto a host running Solaris 2.5 or later.\n"
			patch_quit 22 "no"
			return 0
		elif [[ "$PatchType" = "caPatch" ]]
		then
			cd old_style_patch
			patchdir=$(pwd)
		fi
	fi
	return 1
}

# Description:
#       Display error messages if the patch being applied conflicts with incompatible,
#	required or obsolete patches.
# Parameters:
#	none
#
# Environment Variable Set:
#       none

function check_patch_compatibility
{

	if [[ "$InstIncompat" != "" ]]
	then
     	/usr/bin/gettext "ERROR: This patch is incompatible with patch $InstIncompat\nwhich has already been applied to the system.\n"
       	patch_quit 24 "no"
		return 0
	fi

	if (( ReqdPatchCnt == 1 ))
	then
       	/usr/bin/gettext "ERROR: This patch requires patch$UninstReqs\nwhich has not been applied to the system.\n"
       	patch_quit 25 "no"
		return 0
	elif (( ReqdPatchCnt > 1 ))
	then
       	/usr/bin/gettext "ERROR: This patch requires the following patches\nwhich have not been applied to the system:\n"
       	echo "    $UninstReqs"
       	patch_quit 25 "no"
		return 0
	fi

	if [[ "$ObsoletePast" != "" ]]
	then
       	/usr/bin/gettext "WARNING: This patch appears to have been produced after the\nbase code $ObsoletePast was obsoleted. This patch will be treated\nas though it were also obsoleted.\n"
	fi

	if [[ "$ReqdOSPatch" != "none" && "$ReqdOSPatchFnd" != "true" ]]
	then
       	/usr/bin/gettext "ERROR: This Solaris $MgrOSVers server requires the following patch before it\ncan apply a patch to a Solaris $TrgOSVers client.\n    patch base number : $ReqdOSPatchBase\n    patch version number : $ReqdOSPatchVers or higher.\n"
       	patch_quit 21 "yes"
	fi
	return 1
} 

############################################
#		Main Routine		   #
############################################

#
# -	Get the product version <name>_<version> of local Solaris
#	installation (sets the prodver global variable)
# -	Parse the argument list and set globals accordingly
# -	Make sure the user is running as 'root'
#

Cmd=$0
CmdArgs=$*

set_globals

parse_args $CmdArgs

validate_uid	# the caller must be "root"

setup_net_image

# determine the OS versions involved
find_softinfos $ROOTDIR
get_OS_version $TRGSOFTINFO $MGRSOFTINFO $ROOTDIR

eval_utilities		# make sure the required utilities are available

for ptch in $multiPtchList
do
	set_globals
	if [[ "$multiPtchInstall" = "yes" ]]
	then
		patchdir="$multiPtchDir/$ptch"
	fi
		
	eval_patch $patchdir	# determine what type of patch it is

	# Establish the patching options based on the versions involved
	eval_OS_version $MgrOSVers $TrgOSVers $PatchType

	# Clear the list of installed patches, if it's there
	$RM -f $INSTPATCHES_FILE

	if activate_patch "$patchdir"
	then
		continue
	fi

	#
	# Change to the patch directory and set globals according to the patchID
	# found in the pkginfo files of the patch packages.
	#
	collect_data

	# Check to see if there are any remains from a previous installation.
	check_file_recovery_dir

	if is_progressive
	then
		continue
	fi

	ident_reqd_patch $MgrOSVers	# determine if this host needs a patch

	# Check the output from the pkginfo command.
	#chk_pkginfo_cmd

	if [[ "$PatchMethod" = "direct" ]]
	then
		/usr/bin/gettext "Checking installed packages and patches...\n"
	fi

	eval_inst_patches $PKGDB > /dev/null 2>&1	# Scan installed patches & analyze.

	check_if_applied "$PATCHDB" "$PatchNum"

	if [[ "$isapplied" = "yes" ]]
	then
		if eval_applied_patch $PATCHDB $PKGDB $PatchMethod $saveold $PatchNum 
		then
			continue
		fi
	fi

	if check_if_obsolete "$PATCHDB"	# see if this package is already obsolete
	then
		continue
	fi

	# Evaluate the compatibility of REQUIRES INCOMPATS and
	# OBSOLETES.
	eval_compats

	if check_patch_compatibility
	then
		continue
	fi

	# For the old-style patch, there were sometimes problems running on a
	# client and also symbolic links were not allowed. We check for those next.
	if [[ "$PatchMethod" = "progressive" ]]
	then
		verify_client
		if check_for_symbolic_link "$patchdir"
		then
			continue
		fi
	fi

	trap 'trap_notinstalled "$PATCHDB" "$PatchNum"' 1 2 3 15	# set the trap

	build_admin_file

	gen_patchpkg_list

	# If this is a re-installation of the patch, remove the already
	# installed packages from the package list. If all packages in 
	# the patch have already been applied, then exit.
	#
	if [[ "$isapplied" = "yes" ]]; then
		if gen_uninstalled_pkgs $PKGDB $PATCHDB $PatchNum
		then
			continue
		fi
	fi

	check_pkgs_installed "$PKGDB" "$patchdir"

	if check_for_action "$client" "$is_a_root_pkg" 
	then
		continue
	fi

	gen_install_filelist "$PKGDB" "$PKGDBARG"

	# Set flag in case of power outage
	file_recovery 		

	# Construct the required response file
	build_response_file $PatchType $PatchMethod

	make_params_available   # export parameters for use by patch scripts

	# If there is a prepatch file in the $patchdir directory,
	# execute it. If the return code is not 0, exit patchadd
	# with an error. Lord knows what this does. We just have to hope
	# it's benign.
	#
	if [[ "$isapplied" = "no" ]]	# if this isn't a reinstallation
	then
		if execute_prepatch "$patchdir"	# do it
		then
			continue
		fi
	fi

	#
	# OK, which patch method are we using?
	#
	if [[ "$PatchMethod" = "direct" ]]
	then
		#
		# This is how we install a direct instance patch
		#
		gen_patch_filelist

		compute_fs_space_requirements 

		if check_fs_space
		then
			continue
		fi

		if check_backout_space "$PKGDB" "$PatchNum" "$patchdir" "$saveold"
		then
			continue
		fi

		if apply_diPatch
		then
			continue
		fi
	else	
		if check_validation "$validate"
		then
			continue
		fi

		gen_patch_filelist

		compute_fs_space_requirements 

		if check_fs_space
		then
			continue
		fi

		create_archive_area "$PATCHDB" "$PatchNum"

		if check_backout_space "$PATCHDB" "$PatchNum" "$patchdir" "$saveold"
		then
			continue
		fi

		trap 'trap_backoutsaved "$PATCHDB" "$PatchNum" "$patchdir"' 1 2 3 15

		# -	Save current versions of files to be patched
		# -	On servers, spool the patch into /export/root/templates for
		#	future clients (CURRENTLY DISABLED)
		# -	Build admin file for later use by pkgadd
		if save_overwritten_files "$PATCHDB" "$PatchNum" "$patchdir" "$saveold"
		then
			continue
		fi

		trap 'trap_backout "$patchdir" "$PatchNum"' 1 2 3 15

		# -	Install the patch packages
		# -	Print results of install
		# -	Save ACTION file if exists, README file and backoutpatch
		#	script
		if install_patch_pkgs "$PATCHDB" "$PatchNum" "$patchdir" \
		  "$PKGDBARG" "$PKGDB"
		then
			continue
		fi
	
	fi

	if execute_postpatch "$PATCHDB" "$PatchNum" "$patchdir"
	then
		continue
	fi

	print_results

	if [[ "$PatchMethod" = "progressive" ]]
	then
		/usr/bin/gettext "\nPatch installation completed.\n"
		/usr/bin/gettext "See $PATCHDB/$PatchNum/log for more details.\n\n"
	fi

	set_patch_status "$PATCHDB" "$PatchNum"

	remove_files
done
patch_quit 0 "yes"
