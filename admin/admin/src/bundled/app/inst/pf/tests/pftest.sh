#!/bin/sh
#
#ident @(#)pftest.sh	1.19	96/04/24
#

SYS_INST=`uname -p`
PFINSTALL=../`uname -p`/pfinstall
SIMEXECUTE=
PROFILE=./test_profile
LEVEL=0
DEBUG=0
#DEBUGGER=/master/cbe/297/sparc.tools/opt/SUNWspro/bin/debugger
DEBUGGER=debugger

while [ -n "$1" ]; do
    case $1 in
	"-b") shift; SYS_BOOTDEVICE=$1; export SYS_BOOTDEVICE; shift
	    ;;
	"-c") shift; CDIMAGE=$1; shift
	    ;;
	"-p") shift; PROFILE=$1; shift
	    ;;
	"-d") shift; DISKLIST=$1; shift
	    ;;
	"-b") shift; DEBUG=1;
	    ;;
	"-D") shift; SIMEXECUTE="-D";
	    ;;
	"-s") shift; SYS_SWAPSIZE=$1; export SYS_SWAPSIZE; shift
	    ;;
	"-x") shift; LEVEL=$1; shift
	    ;;
	"-m") shift; SYS_MEMSIZE=$1; export SYS_MEMSIZE; shift
	    ;;
	"-a") shift; SYS_INST=$1; export SYS_INST; shift
	    ;;
           *) echo "Usage: pftest [-c <CD path>]"
	      echo "              [-p <profile name>]"
	      echo "              [-m <memsize in MB>]"
	      echo "              [-d <disk file name>]"
	      echo "              [-a <architecture>]"
	      echo "              [-x <level>]"
	      echo "              [-D]"
	      echo "              [-b]"
	      exit 1
	    ;; 
    esac
done

# if there isn't an explicit cdimage, set the image based on the
# instruction set being simulated
if [ -z "$CDIMAGE" ]; then
	CDIMAGE=/net/labserv/install/297/$SYS_INST
fi

# if there isn't an explicit disklist, use the one that goes with
# the instruction set being simulated
if [ -z "$DISKLIST" ]; then
	DISKLIST=./disklist.$SYS_INST
fi

LD_LIBRARY_PATH=/master/archive/cbe/297/sparc/usr/lib:/master/archive/cbe/297/sparc/usr/openwin/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH

if [ $DEBUG = 0 ]; then
	echo "$PFINSTALL -x $LEVEL -d $DISKLIST -c $CDIMAGE $SIMEXECUTE $PROFILE 2>&1"
	$PFINSTALL -x $LEVEL -d $DISKLIST -c $CDIMAGE $SIMEXECUTE $PROFILE 2>&1 
else
	echo debugging $PFINSTALL -d $DISKLIST -c $CDIMAGE $SIMEXECUTE $PROFILE
	$DEBUGGER -c "stop in main; run -x $LEVEL -d $DISKLIST -c $CDIMAGE $SIMEXECUTE $PROFILE" $PFINSTALL
fi
