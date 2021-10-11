#!/sbin/sh
#
#       @(#)faspac.sh 1.1 95/01/27 SMI
#
# Copyright (c) 1994 by Sun Microsystems, Inc. 
# All rights reserved.
#
# faspac - converts packages to/from class archive format
#	USAGE : faspac [-a] [-s] [-q] [-d <Base Directory>]
#		[-x <Exclude List>] [<List of Packages>]
#
#	DESC :	Unless the '-s' option is specified, this utility converts
#		a standard ABI-type package to the class archive
#		format used for bundled packages starting with release
#		2.5. The resulting package is smaller and installs from the
#		CD more quickly than the package in full directory
#		format. This utility works only with ABI or or class
#		archive format packages. To convert older compressed
#		formats to/from ABI use the utility repac.sh 
#
#		The resulting package will have an additional directory in
#		the top directory called 'archive.' In this directory will be
#		all of the archives named by class. The install directory
#		will contain the class action scripts necessary to unpack
#		each archive. Absolute paths are not archived.
#
#	OPTIONS :
#	a	Fix attributes (must be root)
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
# Constants
#
TOOBIG=675
USAGE="Usage: ${PROGNAME}  [-a] [-s] [-q] [-d <Base Directory>] [-x <Exclude List>] [<List of Packages>]"
PROGNAME=`/usr/bin/basename $0`
CAS_TEMPLATE=`dirname $0`/i.template
M4_CMD=/usr/ccs/bin/m4
NAWK_CMD=/usr/bin/nawk
WC_CMD=/usr/bin/wc
EGREP_CMD=/usr/bin/egrep
SED_CMD=/usr/bin/sed
CPIO_CMD=/usr/bin/cpio
COMP_CMD=/usr/bin/compress
UNCOMP_CMD=/usr/bin/uncompress

#
# Globals
#
Comp=0
Large=0
Has_x=0

#
# Some Functions
#

#
# non_classes - Set Class to the list of degenerate classes in this package.
#
non_classes() {	# $1 = Package Directory
    Pkgd="$1"
    Map="${Pkgd}/pkgmap"

    # Create a list of all classes
    Class=`$NAWK_CMD '
	$1 != ":" && $2 ~ /[^i]/ { Class[$3]++ }
	END {
	    for (i in Class)
		print i
	}
	' ${Map}`

    # Weed out those that have install class action scripts
    if [ ! -z "${Class}" ]; then
	set -- ${Class}
	Class=""
	while [ ! -z "$1" ]; do
	    if [ -f "${Pkgd}/install/i.$1" -o "$1" = "build" -o \
		"$1" = "sed" -o "$1" = "awk" ]
	    then
		shift
		continue
	    else
		Class="${Class} $1"
	    fi
	    shift
	done
    fi

    # Class is a space separated list of the 'degenerate' classes
    /usr/bin/echo ${Class}
    return 0
}

#
# install_script - insert the correct class action script into the
# install directory
#
install_script () {	# $1 is package directory $2 is class
    msg_ind "Constructing class action script."

    if [ ! -d $1/install ]; then
	/usr/bin/mkdir $1/install
    fi

    targ=$1/install/i.$2

    Cmd="-D_thisclass=$2"

    if [ $Comp -gt 0 ]; then
	Cmd="$Cmd -Dcomp=1"
    fi

    if [ $Large -gt 0 ]; then
	Cmd="$Cmd -Dlarge=1"
    fi

    if [ $Has_x -gt 0 ]; then
	Cmd="$Cmd -Dhas_x=1"
    fi

    $M4_CMD $Cmd $CAS_TEMPLATE | $SED_CMD s/@/\\$/g | $NAWK_CMD '$1 != "#" { print}' > $targ
}
#
# DoInfo - update the pkginfo file
#
DoInfo () {	# $1 = pkginfo $2 = pkgmap $3 = class list
    if [ -z ${3} ]; then
	exit 0
    fi

    msg_ind "Modifying ${PkgName}/pkginfo."

    /usr/bin/cat ${1} | $NAWK_CMD '
	BEGIN {
	    src_const = 0
	    dst_const = 0
	    cas_const = 0
	}
	/PKG_SRC_NOVERIFY/ {
	    if (src_const == 0) {
		src_line = $0
		src_const = 1
	    } else {
		src_line = sprintf("%s %s", src_line, substr($0, index($0,"=")+1))
	    }
	    next
	}
	/PKG_DST_QKVERIFY/ {
	    if (dst_const == 0) {
		dst_line = $0
		dst_const = 1
	    } else {
		dst_line = sprintf("%s %s", dst_line, substr($0, index($0,"=")+1))
	    }
	    next
	}
	/PKG_CAS_PASSRELATIVE/ {
	    if (cas_const == 0) {
		cas_line = $0
		cas_const = 1
	    } else {
		cas_line = sprintf("%s %s", cas_line, substr($0, index($0,"=")+1))
	    }
	    next
	}
	{ print }
	END {
	    if (src_const == 0) {
		printf("PKG_SRC_NOVERIFY=%s\n", classes)
	    } else {
		printf("%s %s\n", src_line, classes)
	    }
	    if (dst_const == 0) {
		printf("PKG_DST_QKVERIFY=%s\n", classes)
	    } else {
		printf("%s %s\n", dst_line, classes)
	    }
	    if (cas_const == 0) {
		printf("PKG_CAS_PASSRELATIVE=%s\n", classes)
	    } else {
		printf("%s %s\n", cas_line, classes)
	    }
	    printf("#FASPACD=%s\n", classes)
	} ' classes="$3" > ${1}.tmp

    /usr/bin/mv ${1}.tmp ${1}

    # Now update the pkgmap
    Isum=`/usr/bin/sum $1 | $NAWK_CMD '{ print $1 }'`
    Isize=`$WC_CMD -c $1 | $NAWK_CMD '{ print $1 }'`

    /usr/bin/cat $2 | $NAWK_CMD '
	/i pkginfo/ {
	    printf("%d %s %s %d %d %d\n", $1, $2, $3, size, sum, $6)
	    next
	}
	{ print }
	' sum=$Isum size=$Isize > ${2}.tmp

    /usr/bin/mv ${2}.tmp ${2}
}
#
# UnInfo - remove the verify qualifiers from pkginfo
#
UnInfo () {	# $1 = pkginfo file $2 = pkgmap $3 = class list
    if [ -z ${3} ]; then
	exit 0
    fi

    msg_ind "Modifying ${PkgName}/pkginfo."

    /usr/bin/cat ${1} | $NAWK_CMD '
	{ split(classes, class) }
	/PKG_SRC_NOVERIFY/ {
	    doprint=0
	    line = substr($0, index($0,"=")+1)
	    fldcount=split(line, list)
	    for(fld=1; fld<=fldcount; fld++) {
		for(cls=1; class[cls]; cls++) {
		    if(class[cls] == list[fld]) {
			list[fld] = "-1"
		    }
		}
	    }
	    output="PKG_SRC_NOVERIFY="
	    for(fld=1; fld<=fldcount; fld++) {
		if(list[fld] != "-1" && list[fld] ) {
		    if (doprint==0) {
			output = sprintf("%s%s", output, list[fld])
		    } else {
			output = sprintf("%s %s", output, list[fld])
		    }
		    doprint=1
		}
	    }
	    if (doprint==1) {
		printf("%s\n", output)
	    }
	    next
	}
	/PKG_DST_QKVERIFY/ {
	    doprint=0
	    line = substr($0, index($0,"=")+1)
	    fldcount=split(line, list)
	    for(fld=1; fld<=fldcount; fld++) {
		for(cls=1; class[cls]; cls++) {
		    if(class[cls] == list[fld]) {
			list[fld] = "-1"
		    }
		}
	    }
	    output="PKG_DST_QKVERIFY="
	    for(fld=1; fld<=fldcount; fld++) {
		if(list[fld] != "-1") {
		    if (doprint==0) {
			output = sprintf("%s%s", output, list[fld])
		    } else {
			output = sprintf("%s %s", output, list[fld])
		    }
		    doprint=1
		}
	    }
	    if (doprint==1) {
		printf("%s\n", output)
	    }
	    next
	}
	/PKG_CAS_PASSRELATIVE/ {
	    doprint=0
	    line = substr($0, index($0,"=")+1)
	    fldcount=split(line, list)
	    for(fld=1; fld<=fldcount; fld++) {
		for(cls=1; class[cls]; cls++) {
		    if(class[cls] == list[fld]) {
			list[fld] = "-1"
		    }
		}
	    }
	    output="PKG_CAS_PASSRELATIVE="
	    for(fld=1; fld<=fldcount; fld++) {
		if(list[fld] != "-1") {
		    if (doprint==0) {
			output = sprintf("%s%s", output, list[fld])
		    } else {
			output = sprintf("%s %s", output, list[fld])
		    }
		    doprint=1
		}
	    }
	    if (doprint==1) {
		printf("%s\n", output)
	    }
	    next
	}
	/#FASPACD=/ {
	    next
	}
	{ print }
	 ' classes="$3" > ${1}.tmp

    /usr/bin/mv ${1}.tmp ${1}

    # Now update the pkgmap
    Isum=`/usr/bin/sum $1 | $NAWK_CMD '{ print $1 }'`
    Isize=`$WC_CMD -c $1 | $NAWK_CMD '{ print $1 }'`

    /usr/bin/cat $2 | $NAWK_CMD '
	/i pkginfo/ {
	    printf("%d %s %s %d %d %d\n", $1, $2, $3, size, sum, $6);
	    next;
	}
	{ print }
	' sum=$Isum size=$Isize > ${2}.tmp

    /usr/bin/mv ${2}.tmp ${2}
}
#
# display message if quiet is off
#
msg_opt() {
    if [ ${Quiet} -eq 0 ]; then
	/usr/bin/echo $1
    fi
    return 0
}
#
# display indented message if quiet is off
#
msg_ind() {
    if [ ${Quiet} -eq 0 ]; then
	/usr/bin/echo "  "$1
    fi
    return 0
}
#
# DoMap - add third entry to the pkgmap of a possibly compressed package
# if it is required.
#
DoMap() {
    size=`/usr/bin/du -s ${PkgPath} | $NAWK_CMD '{printf($1)}'`

    if [ -f ${PkgPath}/pkgmap.tmp ]; then
	usr/bin/rm ${PkgPath}/pkgmap.tmp
    fi

    /usr/bin/cat ${PkgPath}/pkgmap | $NAWK_CMD '
	$1 ~ /[:]/ {	# size line
	    if (NF == 3) {	# if this package was uncompressed
		if( s < $3-4 ) {	# & now compression is significant
		    printf(": %s %s %s\n", $2, $3, s)	# add 3rd entry
		} else {	# no change
		    exit 0
		}
	    } else {	# it was compressed in the first place
		if( s < $4-4 || s > $4+4 ) { # & diff is significant
		    if( s < $3-20) {	# & its still compressed
			printf(": %s %s %s\n", $2, $3, s) # new 3rd ent
		    } else {
			printf(": %s %s\n", $2, $3)	# remove 3rd entry
		    }
		} else {	# no change
		    exit 0
		}
	    }
	    next
	}
	    { print } ' s=$size > ${PkgPath}/pkgmap.tmp
    if [ -s ${PkgPath}/pkgmap.tmp ]; then
	msg_ind "Modifying ${PkgName}/pkgmap."
	/usr/bin/rm ${PkgPath}/pkgmap
	/usr/bin/mv ${PkgPath}/pkgmap.tmp ${PkgPath}/pkgmap
    else
	/usr/bin/rm  ${PkgPath}/pkgmap.tmp
    fi
    return 0
}

#
# excluded - check a list of excluded directories (ExclLst)
#
excluded() {
    /usr/bin/echo $2 | $EGREP_CMD -w $1 > /dev/null 2>&1 && return 1
    return 0
}

#
# Confirm that all key executables are available
#
exec_avail() {
if [ $MkStandard -ne 1 -a ! -x $M4_CMD ]; then
    	/usr/bin/echo "Cannot find required executable: $M4_CMD."
    	exit 1
fi

if [ ! -x $NAWK_CMD ]; then
    	/usr/bin/echo "Cannot find required executable: $NAWK_CMD."
    	exit 1
fi

if [ ! -x $WC_CMD ]; then
    	/usr/bin/echo "Cannot find required executable: $WC_CMD."
    	exit 1
fi

if [ ! -x $SED_CMD ]; then
    	/usr/bin/echo "Cannot find required executable: $SED_CMD."
    	exit 1
fi

if [ ! -x $EGREP_CMD ]; then
    	/usr/bin/echo "Cannot find required executable: $EGREP_CMD."
    	exit 1
fi

if [ ! -x $CPIO_CMD ]; then
    	/usr/bin/echo "Cannot find required executable: $CPIO_CMD."
    	exit 1
fi

if [ ! -x $COMP_CMD ]; then
    	/usr/bin/echo "Cannot find required executable: $COMP_CMD."
    	exit 1
fi

if [ ! -x $UNCOMP_CMD ]; then
    	/usr/bin/echo "Cannot find required executable: $UNCOMP_CMD."
    	exit 1
fi

return 0
}

#
# main
#
trap "rm -f /usr/tmp/rp$$*;exit" 0 1 2 3 15

BaseDir=""
DirName=""
ExclLst=""
Quiet=0
Attr_fix=0
Attr_only=0
MkStandard=0
svd=`/usr/bin/pwd`

if type getopts | $EGREP_CMD 'not found' > /dev/null
then
	eval set -- "`getopt sd:x:qa "$@"`"
	if [ $? -ne 0 ]; then
		/usr/bin/echo $USAGE
		exit 2
	fi
	for i in $*
	do
		case $i in
		-a)	Attr_fix=1; shift;;
		-d)	BaseDir=$1; shift 2;;
		-s)	MkStandard=1; shift;;
		-x)	ExclLst=$1; shift 2;;
		-q)	Quiet=1; shift;;
		--)	shift; break;;
		esac
	done
else
	while getopts sd:x:qa i
	do
		case $i in 
		a)	Attr_fix=1;;
		d)	BaseDir=${OPTARG};;
		s)	MkStandard=1;;
		x)	ExclLst=${OPTARG};;
		q)	Quiet=1;;
		\?)	/usr/bin/echo $USAGE
			exit 2;;
		esac
	done
	shift `expr ${OPTIND} - 1`
fi

exec_avail	# make sure we can even run in this environment

#
# Only runs on Solaris!
#
if [ ! -f /etc/vfstab ]; then
    /usr/bin/echo "${PROGNAME}: This script may only be executed under Solaris!"
    /usr/bin/echo $USAGE
    exit 1
fi

if [ $# -eq 0 -a -z "${BaseDir}" ]; then
    /usr/bin/echo $USAGE
    exit 1
fi

if [ $Attr_only -eq 1 -a $Attr_fix -eq 1 ]; then
    /usr/bin/echo "-a and -A are mutually exclusive."
    exit 1
fi

if [ `/usr/ucb/whoami` != root -a $Attr_fix -eq 1 ]; then
    /usr/bin/echo "-a requires root permissions."
    exit 1
fi

if [ ! -z "${BaseDir}" ]; then
    if [ $BaseDir = "." ]; then
	BaseDir=$svd
    else
	/usr/bin/echo $BaseDir | ckpath -a 1>/dev/null 2>/dev/null
	if [ $? -ne 0 ]; then
	    BaseDir=$svd/$BaseDir
	fi
    fi
else
    BaseDir=$svd
fi

ToCheck=$@

if [ -z "${ToCheck}" ]; then
    ToCheck=`/usr/bin/echo $BaseDir/*`
else
    ToCheck=`/usr/bin/echo $BaseDir/$@`
fi

for PkgPath in ${ToCheck}; do
    PkgIsArch=0		# pkg is archived correctly in class archive format
    PkgHasArch=0	# pkg has an archive directory
    PkgIsOld=0		# pkg is old style cpio format
    PkgIsStandard=0	# pkg is ABI format
    PkgIsVeryOld=0	# pkg is mars style ancient
    PkgIsComp=0

    PkgName=`/usr/bin/basename $PkgPath`

    excluded $PkgName "$ExclLst"

    if [ $? -eq 0 ]; then
	#
	# Do some preprocessing to simplify things later.
	#
	if [ ! -d ${PkgPath} ]; then
	    /usr/bin/echo "No such package as ${PkgName}"
	    continue
	fi
	Map="${PkgPath}/pkgmap"
	Info="${PkgPath}/pkginfo"
	if [ ! -f ${Map} -o ! -f ${Info} ]; then
	    /usr/bin/echo "Package ${PkgName} is incomplete"
	    continue
	fi
	if [ -f ${PkgPath}/reloc.cpio -o -f ${PkgPath}/root.cpio ]; then
	    PkgIsComp=0
	    PkgIsOld=1
	elif [ -f ${PkgPath}/reloc.cpio.Z -o -f ${PkgPath}/root.cpio.Z ]; then
	    PkgIsComp=1
	    PkgIsOld=1
	elif [ -f ${PkgPath}/reloc.Z -o -f ${PkgPath}/root.Z -o \
	  -f ${PkgPath}/reloc -o -f ${PkgPath}/root ]; then
	    PkgIsComp=1
	    PkgIsVeryOld=1
	elif [ -d ${PkgPath}/archive ]; then
	    PkgHasArch=1
	    Classes=`$EGREP_CMD "#FASPACD=" $Info | $NAWK_CMD '{print substr($0, index($0, "=")+1)}'`
	    if [ ! -z "${Classes}" ]; then
		PkgIsArch=1
	    fi
	#
	# If it's not cpio'd & not compressed & not old & it has the
	# appropriate directories, it's ABI.
	#
	elif [ -d ${PkgPath}/reloc -o -d ${PkgPath}/root ]; then
	    PkgIsStandard=1
	else
	    msg_ind "Conversion is inappropriate for ${PkgName}."
	    continue
	fi

	msg_opt "Processing: ${PkgName}"

	if [ ${PkgIsVeryOld} -eq 1 ]; then
	    /usr/bin/echo "Format of ${PkgName} is obsolete. It must be converted"
	    /usr/bin/echo "to ABI format as follows."
	    /usr/bin/echo "    repac.sh -ts ${PkgName}"
	    continue
	fi

	if [ ${PkgIsOld} -eq 1 ]; then
	    /usr/bin/echo "Format of ${PkgName} is obsolete. It must be converted"
	    /usr/bin/echo "to ABI format as follows."
	    /usr/bin/echo "    repac.sh -s ${PkgName}"
	    continue
	fi

	#
	# This converts a class archive package to a standard
	# package (ABI)
	#
	if [ ${MkStandard} -eq 1 ]; then
	    if [ ${PkgIsStandard} -eq 0 ]; then
		Inplace=0
		if [ -d ${PkgPath}/archive ]; then
		    for class in $Classes
		    do
			if [ $Inplace -eq 0 ]; then
			    if [ ! -d ${PkgPath}/reloc ]; then
				/usr/bin/mkdir ${PkgPath}/reloc
			    fi
			    Inplace=1
			fi
			cd ${PkgPath}/reloc
			Archive=${PkgPath}/archive/$class
			ArScript=${PkgPath}/install/i.$class
			msg_ind "Converting ${Archive}. \c"
			if [ -f ${Archive}.Z ]; then
			    $UNCOMP_CMD ${Archive}
			fi
			if [ -f $Archive ]; then
			    $CPIO_CMD -idukm -I ${Archive}
			    if [ $? -eq 0 ]; then
				/usr/bin/rm -f ${Archive}
				msg_ind "-- done"
				msg_ind "Removing class action script."
				/usr/bin/rm -f ${ArScript}
			    else
				msg_ind "cpio of ${Archive} failed with error $?."
				continue
			    fi
			fi
		    done
		    cd ${svd}
		fi

		if [ ! -z "${Classes}" ]; then
		    UnInfo ${Info} ${Map} "$Classes"
		fi

		/usr/bin/rmdir -s -p ${PkgPath}/archive

		# Modify the pkgmap if package is compressed differently
		DoMap 
	    else
		msg_ind "Package is already ABI standard."
	    fi
	    continue	# Done with this package
	#
	# Here we are going to do something to an ABI.
	#
	# This goes through the pkgmap, selecting entities from each
	# degenerate class to archive. Entities in scripted classes
	# must remain in directory format since they must be directly
	# accessable by their Class Action Scripts.
	#
	else
	    FnlClass=""
	    Class=`non_classes ${PkgPath}`
	    if [ -z "${Class}" ]; then
		msg_ind "No items appropriate for archive."
		continue
	    fi

	    # FilePref is the list of files and directories NOT to archive.
	    # FilePref.class is the list of files to archive for a
	    # particular class
   	    /usr/bin/rm -f /usr/tmp/rp$$.${PkgName}*
	    FilePref=/usr/tmp/rp$$.${PkgName}

	    # If this is to have correct attributes then set them
	    if [ $Attr_fix -eq 1 ]; then
		msg_ind "Synchronizing file attributes."
		/usr/sbin/pkgchk -f -d ${BaseDir} ${PkgName} 1>/dev/null 2>/dev/null
	    fi

	    # Now scan for files in scriptless classes and list them by class
	    Has_x=`$NAWK_CMD '
		NR == 1 {			# Set up
		    if (Class != "") {
			NClass=split(Class,Classes)
			for (i=1; i <= NClass; i++)
			    AClass[Classes[i]] = 1
		    }
		    has_x=0
		}
		$4 ~ /\\$/ {			# this messes up cpio
		    next
		}
		$2 ~ /[fv]/ {	# Files (no variables, relative, in AClass)

		    if ( $4 !~ /\\$/ && substr($4,1,1) !~ /\// && $3 in AClass ) {
			print $4 |("sort > " FilePref "." $3)
			if ( $4 == "usr/lib/libintl.so.1" || $4 == "usr/lib/libmapmalloc.so.1" || $4 == "usr/lib/libc.so.1" || $4 == "usr/lib/libw.so.1" || $4 == "usr/lib/libdl.so.1" || $4 == "usr/bin/cpio" || $4 == "usr/bin/rm" || $4 == "usr/bin/ln" || $4 == "usr/sbin/static/mv" || $4 == "usr/bin/nawk" || $4 == "usr/bin/zcat" ) {
			    has_x=1
			}
		    }
		    if (match($4, /.*\//) && RLENGTH > 1)
			DirLst[substr($4,RSTART,(RLENGTH-1))]++
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
		    for (i in FinalDirLst) {
			print i | ("sort -r >" FilePref ".RD")
		    }
		    print has_x
		} ' Class="${Class}" FilePref="${FilePref}" ${Map}`

	    #
	    # Now we have lists of items we need to act upon by class.
	    # So we create each archive.
	    #
	    if [ ! -z "${Class}" ]; then
	    	Dr_list=${FilePref}.RD
		if [ -d ${PkgPath}/reloc ]; then
		    cd ${PkgPath}/reloc
		else
		    msg_ind "Cannot find files to archive."
		    exit 1
		fi
	    fi

	    for class in $Class
	    do
		Ar_class=${FilePref}.${class}

		if [ -f ${Ar_class} ]; then
		    Large=`/usr/bin/cat $Ar_class | $WC_CMD -l`
		    if [ $Large -gt $TOOBIG ]; then
			Large=1
		    else
			Large=0
		    fi

		    # Create the archive directory if needed
		    if [ $PkgHasArch -eq 0 ]; then
			/usr/bin/mkdir ${PkgPath}/archive
			PkgHasArch=1
		    fi

		    Archive=${PkgPath}/archive/${class}
		    if [ ! -f ${Archive} -a ! -f ${Archive}.Z ]; then
			msg_ind "Creating ${Archive}. \c"
			$CPIO_CMD -oc -O ${Archive} < ${Ar_class}
			if [ $? -eq 0 ]; then
			    /usr/bin/chmod 644 ${Archive}
			    msg_ind "Removing archived files. \c"
			    /usr/bin/cat ${Ar_class} | xargs -l5 /usr/bin/rm -f
			    msg_ind "-- done"
			    PkgIsStandard=0
			    PkgIsCpio=1
			    PkgIsArch=1
			    msg_ind "Compressing ${Archive}. \c"
			    $COMP_CMD ${Archive}
				
			    if [ -f ${Archive}.Z ]; then
				msg_ind "-- done"
				PkgIsCpio=0
				PkgIsComp=1
				Comp=1
				install_script ${PkgPath} ${class}
			    else
				msg_ind "Compression is inappropriate for ${Archive}."
				Comp=0
				install_script ${PkgPath} ${class}
			    fi
			else
			    msg_ind "cpio of ${Archive} failed with error $?."
			    if [ -f ${Archive} ]; then
				/usr/bin/rm -f ${Archive}
			    fi
			fi
		    fi
		    /usr/bin/rm -f ${Ar_class}
		    FnlClass="${FnlClass} ${class}"
		fi
	    done

	    # Now remove whatever directories are empty
	    if [ -f Dr_List ]; then
		/usr/bin/cat ${Dr_list} | xargs -l5 /usr/bin/rmdir -s -p
		/usr/bin/rm -f ${Dr_list}
	    fi

	    cd ${svd}

	    /usr/bin/rmdir -s -p ${PkgPath}/reloc

	    if [ $PkgIsStandard -eq 1 ]; then
		msg_ind "No items appropriate for archive."
	    elif [ ! -z "${FnlClass}" ]; then
		DoInfo ${Info} ${Map} "$FnlClass"
		DoMap
	    fi
	fi
    else
	msg_opt "Excluding package ${PkgPath}."
    fi
done
