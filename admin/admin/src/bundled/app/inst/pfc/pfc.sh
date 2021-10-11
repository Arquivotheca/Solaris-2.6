#!/bin/csh

setenv ADMINHELPHOME ./HelpFiles
setenv LD_LIBRARY_PATH $ROOT/usr/snadm/lib
setenv ARCH `uname -p`

setenv DISKS ../pf/tests/d02
setenv CD ../test_cd

set opts = "-d $DISKS -c $CD -v -x 1"
echo $ARCH/ttinstall $opts
$ARCH/ttinstall $opts
