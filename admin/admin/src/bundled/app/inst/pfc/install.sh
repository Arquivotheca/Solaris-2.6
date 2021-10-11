#!/bin/sh

###########################################################################
#
# installtool/ttinstall C shell script.
#
# Intended for use by solutions centers, customer service types
# so they can simulate installtool/ttinstall easily without knowing
# all the installtool/ttinstall options, etc...
#
###########################################################################

###########################################################################
###########################################################################
#
# User Settable Defaults...
# These could be set by the user to their most common defaults.
#

# Do you want to run the Motif or the Curses version?
# If you want motif (which is analagous to specifying -M), then
# set this to 'installtool'.
# If you want curses (which is analagous to specifying -C), then
# set this to 'ttinstall'.
# Leaving it blank is good for developers who are in pfc/pfg src
# directories.
#install=installtool
#install=ttinstall
install=""

# cdrom image root directory (-c option)
imagedir=""

# disk vtocfile (-d option)
vtocfile=""

# memory size (e.g. 16, 32, 64, 96, etc.)
# Leaving it blank will allow the application to use the native
# memory size or a previously set SYS_MEMSIZE.
mem_size=""

# system type: sparc|prep|i386
# This is the type of system you want to simulate.
# Leaving it blank will allow the application to use the native
# architecture or a previously set SYS_INST.
system_type=""

# path to installtool or ttinstall
# Not necessary if they'll be in your path already.
install_dir=""

# root of /usr/snadm/lib for LD_LIBRARY_PATH
# Probably only used by developers linking to non-system standard
# libraries.
if [ ! $?ROOT ] ; then
	ROOT=""
fi

# run the program under the debugger?
debugger=0

###########################################################################
###########################################################################


###########################################################################
#
# The guts...  General user's should not touch below here...
#
###########################################################################
# just for debugging the script
debug=0
if [ "$debug" = 1 ] ; then
	printf "install=$install\n"
	printf "vtocfile=$vtocfile\n"
	printf "mem_size=$mem_size\n"
	printf "system_type=$system_type\n"
	printf "install_dir=$install_dir\n"
	printf "ROOT = $ROOT\n"
fi

program=install.sh
usage="Usage: $program\
\n\t-M | -C\t\t\t(i.e. Motif or Curses)\
\n\t-c <cdrom path>\
\n\t-d <disk config>\
\n\t-h\t\t\t(print usage summary)\
\n\t[-m <memory size>]\
\n\t[-s sparc|prep|i386]"

arch=`uname -p`
while getopts MCDc:d:s:m:h\? opt
do
	case $opt in
	M)
		# motif gui version
		install=installtool
		;;
	C)
		# curses version
		install=ttinstall
		;;
	c)
		# root directory of cdrom disk image
		imagedir=$OPTARG
		;;
	d)
		# vtoc file (prtvtoc output file)
		vtocfile=$OPTARG
		;;
	m)
		# memory size
		mem_size=$OPTARG
		;;
	s)
		# system type ($SYS_INST)
		system_type=$OPTARG
		;;
	h | \?)
		printf "$usage\n"
		exit 2
		;;
	D)
		# run it in the debuuger
		debugger=1
	esac
done

#
# Error check user input
#
if [ "$install" = "" ] ; then
	# if no install path set, then assume we're a developer
	# and figure out which one we want to run based on what
	# directory we're in and what architecture machine we're 
	# currently on.
	cwd=`pwd`
	base=`basename $cwd`
	if [ "$base" = "pfc" ] ; then
		ADMINHELPHOME=./HelpFiles
		export ADMINHELPHOME
		install=$arch/ttinstall;
	elif [ "$base" = "pfg" ] ; then
		ADMINHELPHOME=./adminhelp
		export ADMINHELPHOME
		install=$arch/installtool;
	else
		printf "Error: Please specify -M or -C (Motif or Curses).\n"
		printf "$usage\n"
		exit 2;
	fi
fi

# system type
case $system_type in
	sparc|prep|i386)
		# system type on cmmand line takes precedence
		# over current setting of SYS_INST variable
		SYS_INST=$system_type
		export SYS_INST
		;;
	"")
		# if system type was not set -
		# if SYS_INST is set - use it
		# otherwise, use the current architecture.
		SYS_INST=${SYS_INST:-$arch}
		export SYS_INST
		;;
	*)
		printf "Error: Unknown system type \"%s\".\n" $system_type
		printf "$usage\n"
		exit 2
		;;
esac

# prep/ppc tags are a little screwy...
SYS_INST_CD_DIR=$SYS_INST
if [ "$SYS_INST_CD_DIR" = "prep" ] ; then
	SYS_INST_CD_DIR=ppc
fi

#
# For install developer use only...
# You can set the values of CD and DISKS here if you want...
# Note that SYS_INST is available for use now...
#

# CD directory 
CD=../test_cd

# disk file for developers
DISKS=""
if [ "$SYS_INST" = "sparc" ] ; then
	DISKS=../pf/tests/d02
elif [ "$SYS_INST" = "i386" ] ; then
	DISKS=../pf/tests/disklist.i386
elif [ "$SYS_INST" = "prep" ] ; then
	DISKS=../pf/tests/disklist.ppc
fi

if [ "$imagedir" = "" ] ; then
	# if imagedir is not set, then try a developer setting...
	if [ "$CD" = "" ] ; then
		printf "Error: Please specify a cdrom disk image directory.\n"
		printf "$usage\n"
		exit 2;
	else
		imagedir=$CD;
	fi
fi
if [ ! -d $imagedir ] ; then
	printf "Error: Cannot find cdrom disk image directory ($imagedir).\n"
	printf "$usage\n"
	exit 2;
fi

if [ "$vtocfile" = "" ] ; then
	# if vtocfile is not set, then try a developer setting...
	if [ "$DISKS" = "" ] ; then
		printf "Error: Please specify a vtoc file.\n"
		printf "$usage\n"
		exit 2;
	else
		vtocfile=$DISKS;
	fi
fi
if [ ! -f $vtocfile ] ; then
	printf "Error: Cannot find vtoc file \"$vtocfile\".\n"
	printf "$usage\n"
	exit 2;
fi

# hopefully they're smart enough to enter a number - I'm not checking that...
if [ "$mem_size" != "" ] ; then
	SYS_MEMSIZE=$mem_size
	export SYS_MEMSIZE
fi

#
# Build the command and run it
#
if [ $install = ttinstall ] ; then
	LD_LIBRARY_PATH=$ROOT/usr/snadm/lib
else
	LD_LIBRARY_PATH=$ROOT/usr/snadm/lib:/usr/openwin/lib
	DISPLAY=${DISPLAY:-`hostname`:0}
fi

if [ "$install_dir" = "" ] ; then
	install_command="$install"
else
	install_command="$install_dir/$install"
fi

if [ "$debug" = 1 ] ; then
	printf "DISPLAY=$DISPLAY\n"
	printf "SYS_MEMSIZE=$SYS_MEMSIZE\n"
	printf "SYS_INST=$SYS_INST\n"
fi
sim_opts="-v -x 1"
full_command="$install_command -c $imagedir -d $vtocfile $sim_opts"

if [ "$debugger" = 0 ] ; then
	printf "Running:\n$full_command\n"
	$full_command
else
	printf "When in the debugger, type: "
	printf "\n\trun -c $imagedir -d $vtocfile $sim_opts \n"
	debugger $install_command
fi

exit 0
