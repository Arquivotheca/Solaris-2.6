#
# @(#)stub.4x.check.sh 1.3 92/06/15 SMI
# Copyright 1992 Sun Microsystems Inc.
#
# .stub.4x.check - shell fragment for checking if the re-preinstall-svr4
#	script embedded in 4.1.3 will be successfull with this media.
#	This is designed to be sourced, not run or exec-ed.
#
# on success: fall thru
# on failure: print a message and invoke the function "cleanup"
#

# this is the size of the stub in KB, as .stub.4x.part2 installs it.
# XXX HARDCODED
STUBSIZE=7000
# DDD echo -n "DDD DEBUG enter STUBSIZE: "
# DDD read STUBSIZE
export STUBSIZE

# check that all the pieces needed to create a stub are there
if [ ! -f ${KVMDIR}/sbin/rcS.stub ] ; then
	echo "${myname}: corrupted media"
	echo "    cannot find ${KVMDIR}/sbin/rcS.stub on the media"
	cleanup
	# =====
fi
if [ ! -f ${KVMDIR}/.stub.4x.cpio ] ; then
	echo "${myname}: corrupted media"
	echo "    cannot find ${KVMDIR}/.stub.4x.cpio on the media"
	cleanup
	# =====
fi

# check that there is enough space to create a stub
if [ ${ROOTSIZE} -lt ${STUBSIZE} ]; then
	echo "${myname}: The stub is too big to fit on the root partition"
	echo "    you will have to upgrade to Solaris 2.X another way"
	cleanup
	# =====
fi

