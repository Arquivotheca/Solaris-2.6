# i.template 
#
# This m4 shell script template uncompresses and installs package
# files archived using 'compress' and 'cpio'. The appropriate file
# is found in the 'archive' directory of the package under the class
# name. Only relocatable ($BASEDIR-relative) files in classes without
# pre-established install class action scripts are archived. All
# absolute paths are stored in root.
#
# @(#)i.template 1.2 95/02/13 SMI
# 
# Copyright (c) 1994, by Sun Microsystems, Inc.
# All rights reserved.
#
# i.template - m4 template for use in creating archive class action scripts.
#
# stdin carries the source directory as the first entry followed by the
# paths of the files to be installed as indicated in the pkgmap. Since
# all operations take place from the declared base directory, both relative
# and absolute paths will install correctly. There are two methods and
# since speed is of the essence, we skip straight to the right one :
#
#	If it's an initial install
#		do a full cpio for the archive
#	else
#		make a file list, and do a selective cpio
#
# Packages containing required executables are handled specially.
#
# The build is based upon three variables :
#	comp : the archive is compressed
#	large : the archive has more than Maxlist entries
#	has_x : the archive may replace executables required by this script
#
# The build procedure is essentially
#
#	m4 -D_thisclass=$class [-Dcomp=1] [-DLarge=1] [-Dhas_x=1] \
#	    i.template | sed s/@/\$/g | nawk '$1 != "#" { print}' > \
#	    i.$class
#
# This produces a script with the correct use of argument variables
# and no full line comments.
#
# -- julian.taylor@central (1994-06-22) --
#
# FIRST THE KEY DEFINES
changequote("|, |")dnl
define("|m4_RETURN|", "|
|")dnl
define("|Extract_Cpio|", "|if [ $is_a_filelist -eq 1 ]; then
    if [ $list_empty -eq 0 ]; then
	$CPIO_cmd -idukm -I $Reloc_Arch -E $FILELIST
	if [ $? -ne 0 ]; then
	    echo "cpio of $Reloc_Arch failed with error $?."
	    exit 1
	fi
    fi
else
    $CPIO_cmd -idukm -I $Reloc_Arch
fi
|")dnl
define("|Extract_Comp|", "|if [ $is_a_filelist -eq 1 ]; then
    if [ $list_empty -eq 0 ]; then
	$ZCAT_cmd $Reloc_Arch | $CPIO_cmd -idukm -E $FILELIST
	if [ $? -ne 0 ]; then
	    echo "cpio of $Reloc_Arch failed with error $?."
	    exit 1
	fi
    fi
else
    $ZCAT_cmd $Reloc_Arch | $CPIO_cmd -idukm
fi
|")dnl
define("|Extract_CpioP|", "|if [ $is_a_filelist -eq 1 ]; then
	if [ $list_empty -eq 0 ]; then
		LD_PRELOAD="$Ld_Preload" $CPIO_cmd -idukm -I $Reloc_Arch -E $FILELIST
		if [ $? -ne 0 ]; then
			echo "cpio of $Reloc_Arch failed with error $?."
			exit 1
		fi
	fi
else
	LD_PRELOAD="$Ld_Preload" $CPIO_cmd -idukm -I $Reloc_Arch
fi
|")dnl
define("|Extract_CompP|", "|if [ $is_a_filelist -eq 1 ]; then
	if [ $list_empty -eq 0 ]; then
		LD_PRELOAD="$Ld_Preload" $ZCAT_cmd $Reloc_Arch | LD_PRELOAD="$Ld_Preload" $CPIO_cmd -idukm -E $FILELIST
		if [ $? -ne 0 ]; then
			echo "cpio of $Reloc_Arch failed with error $?."
			exit 1
		fi
	fi
else
	LD_PRELOAD="$Ld_Preload" $ZCAT_cmd $Reloc_Arch | LD_PRELOAD="$Ld_Preload" $CPIO_cmd -idukm
fi
|")dnl
define("|Make_Temp|", "|if [ ${PKG_INSTALL_ROOT:-/} = "/" ]; then
			local_install=1
			if [ ! -d $Tmp_xpath ]; then
				Tmp_Creat=1
				mkdir $Tmp_xpath
				if [ $? -ne 0 ]; then
					echo "ERROR : $NAME cannot create $Tmp_xpath."
					exit 1
				fi
			fi
		fi
|")dnl
define("|Maxlist|", "|MAXLIST=550
count=0
|")dnl
define("|Spcl_Test|", "|if [ -x ${path:-NULL} ]; then
				spclcase $path
				if [ $? -eq 1 ]; then
					break
				fi
			fi
|")dnl
define("|Count_Decision|", "|count=`expr $count + 1`
			if [ $count -gt $MAXLIST ]; then
				is_a_filelist=0
				break
			fi
|")dnl
define("|Spcl_Funcs|", "|Spcl_lib=0
Spcl_exec=0
Movelist=""
Ld_Preload=""
Ld1=usr/lib/ld.so.1
Ld=usr/lib/ld.so
Libintl=usr/lib/libintl.so.1
Libmalloc=usr/lib/libmapmalloc.so.1
Libc=usr/lib/libc.so.1	
Libw=usr/lib/libw.so.1
Libdl=usr/lib/libdl.so.1
Cpio=usr/bin/cpio
Rm=usr/bin/rm
Ln=usr/bin/ln
Mv=usr/bin/mv
Nawk=usr/bin/nawk
Zcat=usr/bin/zcat
Tmp_Creat=0

rm_cpio=0
rm_ln=0
rm_zcat=0
rm_nawk=0
rm_rm=0
rm_mv=0
no_select=0
#
# Functions
#
# Test a path to see if it represents a dynamic library or executable that
# we use in this script. If it is, deal with the special case.
#
spclcase() {	# @1 is the pathname to special case
	if [ $local_install -eq 1 ]; then
		case @1 in
			$Ld)		no_select=1;;
			$Ld1)		no_select=1;;
			$Libintl)	Spcl_lib=1; file=libintl.so.1;;
			$Libmalloc)	Spcl_lib=1; file=libmapmalloc.so.1;;
			$Libc)		Spcl_lib=1; file=libc.so.1;;
			$Libw)		Spcl_lib=1; file=libw.so.1;;
			$Libdl)		Spcl_lib=1; file=libdl.so.1;;
			$Cpio)		rm_cpio=1; Spcl_exec=1;;
			$Ln)		rm_ln=1; Spcl_exec=1;;
			$Zcat)		rm_zcat=1; Spcl_exec=1;;
			$Nawk)		rm_nawk=1; Spcl_exec=1;;
			$Rm)		rm_rm=1; Spcl_exec=1;;
			$Mv)		rm_mv=1; Spcl_exec=1;;
		esac
		if [ $no_select -eq 1 ]; then
			is_a_filelist=0
			list_empty=1
			LD_PRELOAD="$Ld_Preload" $RM_cmd $FILELIST
			if [ $Rm_alt_sav -eq 1 ]; then
				LD_PRELOAD="$Ld_Preload" $RM_cmd -r $PKGSAV
				Rm_alt_sav=0
			fi
			exec_clean 1
			return 1
		elif [ $Spcl_lib -eq 1 ]; then
			if [ $Spcl_init -eq 0 ]; then
				Org_LD_LIBRARY_PATH=${LD_LIBRARY_PATH}
				LD_LIBRARY_PATH="$Org_LD_LIBRARY_PATH $Tmp_xpath"
				export LD_LIBRARY_PATH
				Spcl_init=1
			fi
			Ld_Preload="$Ld_Preload $Tmp_xpath/$file"
			Movelist="@1 $file $Movelist"
			LD_PRELOAD="$Ld_Preload" $MV_cmd @1 $Tmp_xpath
			LD_PRELOAD="$Ld_Preload" $LN_cmd -s ../..$Tmp_xpath/$file @1
			Spcl_lib=0
		elif [ $Spcl_exec -eq 1 ]; then
			$MV_cmd @1 $Tmp_xpath
			if [ $rm_cpio -eq 1 ]; then
				$LN_cmd -s ../..$Tmp_xpath/cpio @1
				CPIO_cmd="$Tmp_xpath/cpio"
				Movelist="@1 cpio $Movelist"
				rm_cpio=0
			elif [ $rm_ln -eq 1 ]; then
				$Tmp_xpath/ln -s ../..$Tmp_xpath/ln @1
				LN_cmd="$Tmp_xpath/ln"
				Movelist="@1 ln $Movelist"
				rm_ln=0
			elif [ $rm_nawk -eq 1 ]; then
				$LN_cmd -s ../..$Tmp_xpath/nawk @1
				NAWK_cmd="$Tmp_xpath/nawk"
				Movelist="@1 nawk $Movelist"
				rm_nawk=0
			elif [ $rm_zcat -eq 1 ]; then
				$LN_cmd -s ../..$Tmp_xpath/zcat @1
				ZCAT_cmd="$Tmp_xpath/zcat"
				Movelist="@1 zcat $Movelist"
				rm_zcat=0
			elif [ $rm_rm -eq 1 ]; then
				$LN_cmd -s ../..$Tmp_xpath/rm @1
				RM_cmd="$Tmp_xpath/rm"
				Movelist="$Movelist @1 rm"
				rm_rm=0
			elif [ $rm_mv -eq 1 ]; then
				$LN_cmd -s ../..$Tmp_xpath/mv @1
				MV_cmd="$Tmp_xpath/mv"
				Movelist="$Movelist @1 mv"
				rm_mv=0
			fi
			Spcl_exec=0
		fi
	fi
	return 0
}

#
# Clean up the libraries and executables that were moved.
#
exec_clean() {	# @1 =1 means be quiet
	if [ ! -z "${Movelist}" ]; then
		echo $Movelist | $NAWK_cmd '
			{ split (@0, line)
			for (n=1; n <= NF; n++) {
				print line[n]
			}
		}' | while read path; do
			read file
			if [ -h $path ]; then
				# then put the original back
				if [ @1 -eq 0 ]; then
					echo "WARNING : $path not found in archive."
				fi
				LD_PRELOAD="$Ld_Preload" $MV_cmd $Tmp_xpath/$file $path
			else
				# remove the temporary copy
				LD_PRELOAD="$Ld_Preload" $RM_cmd $Tmp_xpath/$file
			fi
		done
		for path in $Movelist; do
			if [ -x $path ]; then
				case $path in
					$Cpio)	CPIO_cmd="$CPIO_xpath/cpio";;
					$Ln)	LN_cmd="$LN_xpath/ln";;
					$Zcat)	ZCAT_cmd="$ZCAT_xpath/zcat";;
					$Nawk)	NAWK_cmd="$NAWK_xpath/nawk";;
					$Rm)	RM_cmd="$RM_xpath/rm";;
					$Mv)	MV_cmd="$MV_xpath/mv";;
				esac
			fi
		done
		Movelist=""

		if [ $Tmp_Creat -eq 1 ]; then
			LD_PRELOAD="$Ld_Preload" $RM_cmd -r $Tmp_xpath
			Tmp_Creat=0
		fi
	fi
}
|")dnl
define("|Clean_Up|", "|if [ -f $FILELIST ]; then
	$RM_cmd $FILELIST
fi

if [ $Rm_alt_sav -eq 1 ]; then
	$RM_cmd -r $PKGSAV
fi
|")dnl
define("|Clean_UpP|", "|if [ -f $FILELIST ]; then
	LD_PRELOAD="$Ld_Preload" $RM_cmd $FILELIST
fi

if [ $Rm_alt_sav -eq 1 ]; then
	LD_PRELOAD="$Ld_Preload" $RM_cmd -r $PKGSAV
	Rm_alt_sav=0
fi

exec_clean 0

if [ $Tmp_Creat -eq 1 ]; then
	LD_PRELOAD="$Ld_Preload" $RM_cmd -r $Tmp_xpath
fi

if [ $Spcl_init -eq 1 ]; then
	LD_LIBRARY_PATH=$Org_LD_LIBRARY_PATH
	export LD_LIBRARY_PATH
	Spcl_init=0
fi
|")dnl
define("|Std_Loop|", "|if [ ${PKG_INIT_INSTALL:-null} = null ]; then
	if [ $local_install -eq 1 ]; then
		is_a_filelist=1
		while	read path
		do
			_count_decision
			echo $path >> $FILELIST
			list_empty=0
			_spcl_test
		done
	else
		is_a_filelist=1
		while	read path
		do
			_count_decision
			echo $path >> $FILELIST
			list_empty=0
		done
	fi
fi
|")dnl
define("|Abr_Loop|", "|if [ ${PKG_INIT_INSTALL:-null} = null ]; then
	is_a_filelist=1
	while	read path
	do
		_count_decision
		echo $path >> $FILELIST
		list_empty=0
	done
fi
|")dnl
define("|_test_loop|", "|ifdef("|has_x|", "|Std_Loop|", "|Abr_Loop|")|")dnl
define("|_extract_func|", "|ifdef("|comp|", "|ifdef("|has_x|", "|Extract_CompP|", "|Extract_Comp|")|", "|ifdef("|has_x|", "|Extract_CpioP|", "|Extract_Cpio|")|")|")dnl
define("|_maxlist|", "|ifdef("|large|", "|Maxlist|", "|m4_RETURN|")|")dnl
define("|_count_decision|", "|ifdef("|large|", "|Count_Decision|")|")dnl
define("|_spcl_test|", "|ifdef("|has_x|", "|Spcl_Test|", "||")|")dnl
define("|_spcl_funcs|", "|ifdef("|has_x|", "|Spcl_Funcs|", "||")|")dnl
define("|_clean_up|", "|ifdef("|has_x|", "|Clean_UpP|", "|Clean_Up|")|")dnl
define("|_make_temp|", "|ifdef("|has_x|", "|Make_Temp|", "||")|")dnl

# AND NOW THE BODY OF THE SCRIPT
NAME="i._thisclass"
FILELIST=${PKGSAV:?undefined}/filelist
BD=${BASEDIR:-/}
_maxlist
is_an_archive=0
is_a_filelist=0
list_empty=1
local_install=0
Spcl_init=0
Rm_alt_sav=0
Tmp_xpath=/usr/tmp$$dir

# Set up the default paths
MV_xpath=/usr/bin
MV_cmd=$MV_xpath/mv
CPIO_xpath=/usr/bin
CPIO_cmd=$CPIO_xpath/cpio
ZCAT_xpath=/usr/bin
ZCAT_cmd=$ZCAT_xpath/zcat
LN_xpath=/usr/bin
LN_cmd=$LN_xpath/ln
NAWK_xpath=/usr/bin
NAWK_cmd=$NAWK_xpath/nawk
RM_xpath=/usr/bin
RM_cmd=$RM_xpath/rm

# If there we are going to replace an executable that we need,
# then we need to squirrel it away to a temporary place.
_spcl_funcs

#
# Figure out what kind of package this is
#
eval_pkg() {
	read path	# get the package source directory

	# get the package source directory
	if [ ${path:-NULL} != NULL ]; then
		PKGSRC=${path:?undefined}

		# If it is a local install and we have essential
		# executables to deal with, we need to create a
		# temporary directory on the /usr file system.
		_make_temp

		if [ -r $PKGSRC/archive/_thisclass -o -r $PKGSRC/archive/_thisclass.Z ]; then
			is_an_archive=1
		fi
	else
		exit 0	# empty pipe, we're done
	fi

}

#
# main
#

eval_pkg

if [ $is_an_archive -eq 0 ]; then
	echo "ERROR : $NAME cannot find archived files in $PKGSRC/archive."
	exit 1
fi

Reloc_Arch=$PKGSRC/archive/_thisclass

if [ ! -d $PKGSAV ]; then
	echo "WARNING : $NAME cannot find save directory $PKGSAV."
	PKGSAV=$Tmp_xpath/$PKG.sav

	if [ ! -d $PKGSAV ]; then
		/usr/bin/mkdir $PKGSAV
	fi

	if [ $? -eq 0 ]; then
		echo "  Using alternate save directory" $PKGSAV
		FILELIST=$PKGSAV/filelist
		Rm_alt_sav=1
	else
		echo "ERROR : cannot create alternate save directory" $PKGSAV
		exit 1
	fi
fi

if [ -f $FILELIST ]; then
	rm $FILELIST
fi

cd $BD

# If this is not an initial install then clear out potentially executing
# files and libraries for cpio and create an extraction list if necessary
_test_loop

# Now extract the data from the archive(s)
_extract_func

_clean_up

exit 0
