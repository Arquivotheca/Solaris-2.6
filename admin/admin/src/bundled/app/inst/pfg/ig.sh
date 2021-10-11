#!/bin/csh

setenv ADMINHELPHOME ./adminhelp

if (! $?DISPLAY) then
	setenv DISPLAY `hostname`:0
endif

setenv ARCH `uname -p`

setenv SYS_INST i386

setenv CD ../test_cd
setenv CD /net/install-1/install/297/${SYS_INST}/working

setenv LD_LIBRARY_PATH $ROOT/usr/snadm/lib:/usr/openwin/lib

if ($SYS_INST == "sparc") then
	setenv DISKS ../pf/tests/d02
else if ($SYS_INST == "prep") then
	setenv DISKS ../pf/tests/disklist.ppc
else if ($SYS_INST == "i386") then
	setenv DISKS ../pf/tests/disklist.i386
endif

set opts = "-d $DISKS -c $CD -v -x 1"
echo $ARCH/installtool $opts
$ARCH/installtool $opts

