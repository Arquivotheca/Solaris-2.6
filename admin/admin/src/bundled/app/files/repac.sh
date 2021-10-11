#!/sbin/sh
#
#       @(#)repac.sh 1.5 94/01/10 SMI
#
# Copyright (c) 1992 by Sun Microsystems, Inc. 
#
# repac.sh - converts packaes to/from special WOS format & handles classes
#	USAGE : sh repac.sh [-t] [-c|-u|-s] [-v] [-d <Base Directory> ]
#		[-x <Exclude List>] [<List of Packages>]
#
#	DESC :	Unless the '-s' option is specified, this utility converts
#		a standard ABI-type package to the simpler and smaller
#		format used for bundled packages. The resulting package
#		installs from the CD more quickly than the package in
#		full directory format. There are many key limitations
#		associated with this format. It is ONLY reliable for
#		bundled packages installing to a BLANK disk. This format
#		is highly undesireable and often non-functional for
#		patches and upgrades since cpio is used to do the install
#		and cpio cannot overwrite an executing file!
#
#	OPTIONS :
#	t	Translate an old style "root.Z" or "reloc.Z" to the ABI
#		compliant format before performing the requested operation.
#		(Note : if you're working with an old WOS package and
#		haven't set this flag, repac will warn you and exit.)
#
#	c	Compress the cpio archives if possible
#
#	u	Uncompress the compressed cpio archives
#
#	s	Standard ABI type package translation. This takes a
#		cpio'd and/or compressed package and makes it a standard
#		ABI compliant package format.
#
#	q	quiet mode
#
#	d [bd]	This is a directory in which all packages present will
#		be acted upon as required by the command line. This is
#		mutually exclusive with the <List of Packages> entry.
#
#	x [el]	Comma separated or quoted, space-separated list of
#		packages to exclude from processing.
#

#
# Some Functions
#

#
# classes - determine if there are 'real' classes to deal with in this package.
#
classes() {	# $1 = Package Directory
	Pkgd="$1"
	Map="${Pkgd}/pkgmap"
	Class=`nawk '
	    $1 != ":" && $2 ~ /[^i]/ { Class[$3]++ }
	    END {
		for (i in Class)
		    if (i != "none")
			print i
	    }
	    ' ${Map}`
	if [ ! -z "${Class}" ]; then
	    set -- ${Class}
	    Class=""
	    while [ ! -z "$1" ]; do
		if [ -f "${Pkgd}/install/i.$1" -o -f "${Pkgd}/install/r.$1" -o \
		     "$1" = "build" -o "$1" = "sed" -o "$1" = "awk" ]
		then
		    Class="${Class} $1"
		fi
		shift
	    done
	fi
	echo ${Class}
	return 0
}
#
# display message if quiet is off
#
msg_opt() {
    if [ ${Quiet} -eq 0 ]; then
	echo $1
    fi
    return 0
}
#
# display indented message if quiet is off
#
msg_ind() {
    if [ ${Quiet} -eq 0 ]; then
	echo "  "$1
    fi
    return 0
}

#
# UnMap - remove the third entry from the pkgmap of an uncompressed package
#
UnMap() {
	cat ${Item}/pkgmap | awk \
	    'BEGIN { \
		done = 0 \
	    } \
	    / / { \
		if (done == 1) { \
		    print; \
		} \
	    } \
	    /:/{ \
		if (done == 0) { \
		    printf(": %s %s\n", $2, $3); \
		done = 1; \
		} \
	    } ' > ${Item}/pkgmap.tmp
	if [ ! -f ${Item}/pkgmap.tmp ]; then
	    echo "Error : cannot write to ${Item} directory"
	    exit 13
	else
	    msg_ind "modifying ${BaseName}/pkgmap"
	    rm ${Item}/pkgmap
	    cp ${Item}/pkgmap.tmp ${Item}/pkgmap
	    rm ${Item}/pkgmap.tmp
	fi
	return 0
}

#
# DoMap - add third entry to the pkgmap of a newly compressed package
#
DoMap() { # $1 = compressed size
	cat ${Item}/pkgmap | awk \
	    'BEGIN { \
		done = 0 \
	    } \
	    / / { \
		if (done == 1) { \
		    print; \
		} \
	    } \
	    /:/{ \
		if (done == 0) { \
		    printf(": %s %s %s\n", $2, $3, s); \
		    done = 1; \
		} \
	    } ' s=$1 > ${Item}/pkgmap.tmp
	if [ ! -f ${Item}/pkgmap.tmp ]; then
		echo "Error : cannot write to ${Item} directory"
		exit 13
	else
		msg_ind "modifying ${BaseName}/pkgmap"
		rm ${Item}/pkgmap
		cp ${Item}/pkgmap.tmp ${Item}/pkgmap
		rm ${Item}/pkgmap.tmp
	fi
	return 0
}

#
# excluded - check a list of excluded directories (ExclLst)
#
excluded() {
    echo $2 | grep -w $1 > /dev/null 2>&1 && return 1
    return 0
}
#
# main
#
PROGNAM=`basename $0`
USAGE="Usage: ${PROGNAME} [-t] [-c|-u] [-q] [-v] [-d <Base Directory> ] [-x <Exclude List>] [<List of Packages>]"
trap "rm -f /usr/tmp/rp$$*;exit" 0 1 2 3 15
BaseDir=""
ExclLst=""
Quiet=0
Compress=0
UnCompress=0
Translate=0
MkStandard=0
if type getopts | grep 'not found' > /dev/null
then
	eval set -- "`getopt scd:tx:uq "$@"`"
	if [ $? != 0 ]; then
		echo $USAGE
		exit 2
	fi
	for i in $*
	do
		case $i in
		-c)	Compress=1; shift;;
		-d)	BaseDir=$1; shift 2;;
		-t)	Translate=1; shift;;
		-s)	MkStandard=1; shift;;
		-x)	ExclLst=$1; shift 2;;
		-u)	UnCompress=1; shift;;
		-q)	Quiet=1; shift;;
		--)	shift; break;;
		esac
	done
else
	while getopts scd:tx:uq i
	do
		case $i in 
		c)	Compress=1;;
		d)	BaseDir=${OPTARG};;
		t)	Translate=1;;
		s)	MkStandard=1;;
		x)	ExclLst=${OPTARG};;
		u)	UnCompress=1;;
		q)	Quiet=1;;
		\?)	echo $USAGE
			exit 2;;
		esac
	done
	shift `expr ${OPTIND} - 1`
fi

#
# Only runs on Solaris!
#
if [ ! -f /etc/vfstab ]; then
    echo "${PROGNAME}: This script may only be executed under Solaris!"
    echo $USAGE
    exit 1
fi

if [ $# -eq 0 -a -z "${BaseDir}" ]; then
    echo $USAGE
    exit 1
fi
if [ ${Compress} -eq 1 -a ${UnCompress} -eq 1 ]; then
    echo "-c and -u flags are mutually exclusive."
    echo $USAGE
    exit 1
fi
if [ ${Translate} -eq 1 -a ${Compress} -eq 1 -o \
     ${Translate} -eq 1 -a ${UnCompress} -eq 1 ]; then
    echo "-s flag is incompatible with -c or -u flag."
    echo $USAGE
    exit 1
fi
if [ ! -z "${BaseDir}" ]; then
    ToCheck=`echo ${BaseDir}/*`
else
    ToCheck=$@
fi

for Item in ${ToCheck}; do
    PkgIsCpio=0
    PkgIsComp=0
    PkgIsStandard=0
    PkgIsOld=0
    PkgNoComp=0

    BaseName=`basename ${Item}`
    excluded $BaseName "$ExclLst"
    if [ $? -eq 0 ]; then
	#
	# Do some preprocessing to simplify things later.
	#
	if [ ! -d ${Item} ]; then
	    echo "No such package as ${Item}"
	    continue
	fi
	Map="${Item}/pkgmap"
	Info="${Item}/pkginfo"
	if [ ! -f ${Map} -o ! -f ${Info} ]; then
	    echo "Package ${Item} is incomplete"
	    continue
	fi
	if [ -f ${Item}/reloc.cpio -o -f ${Item}/root.cpio ]; then
	    PkgIsCpio=1
	elif [ -f ${Item}/reloc.cpio.Z -o -f ${Item}/root.cpio.Z ]; then
	    PkgIsComp=1
	elif [ -f ${Item}/reloc.Z -o -f ${Item}/root.Z -o \
	  -f ${Item}/reloc -o -f ${Item}/root ]; then
	    PkgIsOld=1

	#
	# If it's not cpio'd & not compressed & not old & it has the
	# appropriate directories, it's ABI.
	#
	elif [ -d ${Item}/reloc -o -d ${Item}/root ]; then
	    PkgIsStandard=1
	else
	    msg_ind "Conversion is inappropriate for ${Item}"
	    continue
	fi

	msg_opt "Processing: ${BaseName}"

	if [ ${PkgIsOld} -eq 1 ]; then
	    if [ ${Translate} -eq 1 ]; then
		svd=`pwd`
		cd ${Item}
		if [ -f root -o -f root.Z ]; then
		    msg_ind "Translating old ${BaseName}/root \c"
		    if [ -f root.Z ]; then
			uncompress root
		    fi
		    mv root root.cpio
		    mkdir root
		    cd root
		    cpio -idum < ../root.cpio
		    if [ $? -eq 0 ]; then
		    	cd ..
		    	rm -f root.cpio
		    	msg_ind "done."
		    else
			msg_ind "cpio of root.cpio failed with error $?."
			cd ${svd}
			continue	# don't do anything else with this
		    fi
		fi
		if [ -f reloc -o -f reloc.Z ]; then
		    msg_ind "Translating old ${BaseName}/reloc \c"
		    if [ -f reloc.Z ]; then
			uncompress reloc
		    fi
		    mv reloc reloc.cpio
		    mkdir reloc
		    cd reloc
		    cpio -idum < ../reloc.cpio
		    if [ $? -eq 0 ]; then
		    	cd ..
		    	rm -f reloc.cpio
		    	msg_ind "done."
		    else
			msg_ind "cpio of reloc.cpio failed with error $?."
			cd ${svd}
			continue	# don't do anything else with this
		    fi
		fi
		cd ${svd}

		# Now it's standard
		PkgIsOld=0
		PkgIsStandard=1
	    else
		echo "Package ${Basename} is old style. Use '-t'."
		continue
	    fi
	fi

	#
	# This converts a cpio'd or compressed package to a standard
	# package
	#
	if [ ${MkStandard} -eq 1 ]; then
	    if [ ${PkgIsStandard} -eq 0 ]; then
		svd=`pwd`  # remember where we are (cpio requires us to move)
		cd ${Item}
		if [ -f root.cpio -o -f root.cpio.Z ]; then
		    msg_ind "Converting ${BaseName}/root \c"
		    if [ -f root.cpio.Z ]; then
			uncompress root.cpio
		    fi
		    if [ ! -d root ]; then
			mkdir root
		    fi
		    cd root
		    cpio -idum < ../root.cpio
		    if [ $? -eq 0 ]; then
		    	cd ..
		    	rm -f root.cpio
		    	msg_ind "done."
		    else
			msg_ind "cpio of root.cpio failed with error $?."
			cd ${svd}
			continue
		    fi
		fi
		if [ -f reloc.cpio -o -f reloc.cpio.Z ]; then
		    msg_ind "Converting ${BaseName}/reloc \c"
		    if [ -f reloc.cpio.Z ]; then
			uncompress reloc.cpio
		    fi
		    if [ ! -d reloc ]; then
			mkdir reloc
		    fi
		    cd reloc
		    cpio -idum < ../reloc.cpio
		    if [ $? -eq 0 ]; then
		    	cd ..
		    	rm -f reloc.cpio
		    	msg_ind "done."
		    else
			msg_ind "cpio of reloc.cpio failed with error $?."
			cd ${svd}
			continue
		    fi
		fi
		cd ${svd}	# Now back in place

		# Modify the pkgmap if package was compressed
		if [ $PkgIsComp -eq 1 ]; then
		    UnMap
		fi

		#
		# This isn't necessary, but may save problems later
		#
		PkgIsStandard=1
		PkgIsCpio=0
		PkgIsComp=0
	    else
		msg_ind "package is already ABI standard."
	    fi
	    continue	# Done with this package
	fi

	#
	# Here we are going to cpio a package.
	#
	# This goes through the pkgmap, selecting entities in the class
	# 'none' to archive. Entities in other classes must remain in
	# directory format since they must be directly accessable by
	# potential Class Action Scripts.
	#
	if [ ${PkgIsStandard} -eq 1 -a ${UnCompress} -eq 0 ]; then
	    Class=`classes ${Item}`
	    FilePref=/usr/tmp/rp$$.${BaseName}
	    /bin/rm -f /usr/tmp/rp$$.${BaseName}*
	    nawk '
		NR == 1 {			# Set up
		    if (Class != "") {
			NClass=split(Class,Classes)
			for (i=1; i <= NClass; i++)
			    AClass[Classes[i]] = 1
		    }
		}
		$2 == "e" {			# Edittable
		    print $4 | ("sort > " FilePref ".N")
		    next
		}
		$2 ~ /[fv]/ {			# Files
		    if ($3 in AClass || $4 ~ /\$/) {
			print $4 | ("sort > " FilePref ".N")
		    } else {
			if ($4 ~ /^\//) {
			    print "." $4 | ("sort >" FilePref ".root.A")
			} else {
			    print $4 | ("sort >" FilePref ".reloc.A")
			}
			if (match($4, /.*\//) && RLENGTH > 1)
			    DirLst[substr($4,RSTART,(RLENGTH-1))]++
		    }
		    next
		}
		END { 			# Sort the list of directories
		    for (i in DirLst) {
			FinalDirLst[i]++
			k=i
			while (match(k, /.*\//) && RLENGTH > 1) {
			    FinalDirLst[substr(k,RSTART,(RLENGTH-1))]++
			    k=substr(k,RSTART,(RLENGTH-1))
			}
		    }
		    for (i in FinalDirLst)
			if (i ~ /^\//) {
			    print "." i | ("sort -r >" FilePref ".root.RD")
			} else {
			    print i | ("sort -r >" FilePref ".reloc.RD")
			}
		} ' Class="${Class}" FilePref="${FilePref}" ${Map}
	    #
	    # Now we have lists of items we need to act upon.
	    #
	    Ar_root=${FilePref}.root.A		# root entities safe to archive
	    Dr_root=${FilePref}.root.RD		# root entities for directory
	    Ar_reloc=${FilePref}.reloc.A	# reloc entities safe to archive
	    Dr_reloc=${FilePref}.reloc.RD	# reloc entities for directory
	    #
	    # Create the root archive
	    #
	    if [ -f ${Ar_root} ]; then
		if [ -d ${Item}/root ]; then
		    svd=`pwd`
		    cd ${Item}/root
		    Archive=../root.cpio
		    if [ ! -f ${Archive} -a ! -f ${Archive}.Z ]; then
			msg_ind "Creating ${Archive} \c"
			cpio -oc -O ${Archive} < ${Ar_root}
			if [ $? -eq 0 ]; then
			    chmod 644 ${Archive}
			    msg_ind "Removing archived files \c"
			    cat ${Ar_root} | xargs -l5 rm -f
			    cat ${Dr_root} | xargs -l5 rmdir -s -p
			    msg_ind "done($?)."
			else
			    msg_ind "cpio of ${Archive} failed with error $?."
			    if [ -f ${Archive} ]; then
				rm -f $Archive}
			    fi
			fi
		    fi
		    cd ${svd}
		    rmdir -s -p ${Item}/root	# remove empties
		fi
	    fi
	    rm -f ${Ar_root}
	    rm -f ${Dr_root}
	    #
	    # Now we create the reloc archive
	    #
	    if [ -f ${Ar_reloc} ]; then
		if [ -d ${Item}/reloc ]; then
		    svd=`pwd`
		    cd ${Item}/reloc
		    Archive=../reloc.cpio
		    if [ ! -f ${Archive} -a ! -f ${Archive}.Z ]; then
			msg_ind "Creating ${Archive} \c"
			cpio -oc -O ${Archive} < ${Ar_reloc}
			if [ $? -eq 0 ]; then
			    chmod 644 ${Archive}
			    msg_ind "Removing archived files \c"
			    cat ${Ar_reloc} | xargs -l5 rm -f
			    cat ${Dr_reloc} | xargs -l5 rmdir -s -p
			    msg_ind "done($?)."
			else
			    msg_ind "cpio of ${Archive} failed with error $?."
			    if [ -f ${Archive} ]; then
				rm -f $Archive}
			    fi
			fi
		    fi
		    cd ${svd}
		    rmdir -s -p ${Item}/reloc
		fi
	    fi
	    rm -f ${Ar_reloc}
	    rm -f ${Dr_reloc}
	    rm -f ${FilePref}.N

	    if [ -f ${Item}/root.cpio -o -f ${Item}/reloc.cpio ]; then
		PkgIsStandard=0
		PkgIsCpio=1
	    else
		msg_ind "No items appropriate for archive."
	    fi
	fi
	#
	# If Package is cpio'd and we're compressing then compress
	#
	if [ ${PkgIsCpio} -eq 1 -a ${Compress} -eq 1 ]; then
	    if [ -f ${Item}/root.cpio ]; then
	    	msg_ind "Compressing ${BaseName}/root.cpio \c"
	    	compress ${Item}/root.cpio
	    	msg_ind "done($?)."
	    fi
	    if [ -f ${Item}/reloc.cpio ]; then
		msg_ind "Compressing ${BaseName}/reloc.cpio \c"
		compress ${Item}/reloc.cpio
		msg_ind "done($?)."
	    fi

	    if [ -f ${Item}/root.cpio.Z -o -f ${Item}/reloc.cpio.Z ]; then
	    	PkgIsCpio=0
	    	PkgIsComp=1
	    else
		msg_ind "Archives did not compress."
	    fi
	#
	# Otherwise, if Package is compressed and we're uncompressing
	# then uncompress
	#
	elif [ ${PkgIsComp} -eq 1 -a ${UnCompress} -eq 1 ]; then
	    if [ -f ${Item}/root.cpio.Z ]; then
	    	msg_ind "UnCompressing ${BaseName}/root.cpio.Z \c"
	    	uncompress ${Item}/root.cpio.Z
	    	msg_ind "done($?)."
	    fi
	    if [ -f ${Item}/reloc.cpio.Z ]; then
		msg_ind "UnCompressing ${BaseName}/reloc.cpio.Z \c"
		uncompress ${Item}/reloc.cpio.Z
		msg_ind "done($?)."
	    fi
	    PkgIsComp=0
	    PkgIsCpio=1

	#
	# If the user wanted to compress but there were no archives,
	# say so.
	#
	elif [ ${UnCompress} -eq 1 ]; then
	    msg_ind "This package is not compressed."
	    PkgNoComp=1
	fi

	#
	# If the directories have been compressed, a third entry is added
	# to the first line of the pkgmap to tell pkgtrans how big
	# the total compressed package actually is. The format of
	# the first line in pkgmap will be
	#	: number_of_parts maximum_part_size compressed_size
	# this is adding compressed_size. -- JST
	#
	if [ ${PkgIsComp} -eq 1 -a ${Compress} -eq 1  ]; then
	    size=`du -s ${Item} | nawk '{printf($1)}'`
	    DoMap $size
	fi

	#
	# If we're uncompressing, we need to undo the stuff we did
	# to pkgmap when we compressed it. See above
	# "if [ ${Compressed..." -- JST
	#
	if [ ${PkgNoComp} -eq 0 -a ${PkgIsComp} -eq 0 -a ${UnCompress} -eq 1 ]; then
	    UnMap
	fi
    else
	msg_opt "Excluding package ${Item}."
    fi
done
