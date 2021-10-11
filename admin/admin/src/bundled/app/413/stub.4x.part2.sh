#!/sbin/sh
#
# @(#)stub.4x.part2.sh 1.5 92/07/27 SMI
# Copyright 1992 Sun Microsystems Inc.
#
# (catastrophic upgrade to 5.X for 4.1.3 systems)
# .stub.4x.part2.sh - reboot the appropriate media (network or CDROM),
# which had previously been found by re-preinstall.svr4.sh.
# This piece lives on the install media, and is exec'd by
# re-preinstall.svr4.sh, with the following set:
#       STUB - path of stub dir on the root disk
#       KVMDIR - path of the install root to use as the stub base
#       BOOTBLK - path to the bootblock off the media

myname=stub.4x.part2
if [ ! "${STUB}" -o ! "${KVMDIR}" -o ! "${BOOTBLK}" ]; then
	echo "${myname}: STUB undefined, bailing out"
	exit 1
fi

MACH=`mach`
if [ ! "${MACH}" ]; then
	echo "${myname}: cannot find MACH, bailing out"
	exit 1
fi
#
# NOTE: this version will only work with a root that is large enough
# to hold the stub (previously checked with .stub.4x.check).
#

echo ""
echo "loading stub, this will take up to 4 minutes..."

ROOTDISK=`df / | ( read junk ; read disk junk; echo $disk )`
# we want to reboot off _exactly_ the root disk (if we can)
if [ -x /usr/kvm/unixname2bootname ]; then
	REBOOT=`/usr/kvm/unixname2bootname ${ROOTDISK}`
else
	REBOOT=""	# sorry, poor old sun4
fi

# unmount all but root, (kvm), /usr and the media
mount | while read mount on path type fstype stuff ; do
	case $path in
	/|/usr|"${STUB}/mnt")
		# nothing, leave alone
		continue
		;;
	*)	umount ${path}
		;;
	esac
done

# we cannot move "/usr" to a hidden place because ld.so can only come
# from /usr/lib hardcoded path.  So, we play some games to use /usr
# until the very last moment
 
# grab the media "/usr" link now, while we have the tools to do so
USRLNK2=`cd "${KVMDIR}/cdrom" ; echo export/exec/${MACH}.* | \
	( read first junk; echo $first )`
USRLNK="cdrom/${USRLNK2}"

# clean up the root partition of everything that isn't needed
#  things that show, but NOT /tmp, /usr and /dev
RM1=`/usr/bin/ls -ad /* | egrep -v "^/tmp|^/usr|^/dev"`
/usr/bin/rm -rf ${RM1}
/usr/bin/rm -rf /tmp/*

#  things that are hidden, but NOT "/." or "/.." or the stub
RM2=`/usr/bin/ls -ad /.[a-zA-Z0-9]* | /usr/bin/grep -v "\</.stub\>"`
/usr/bin/rm -rf ${RM2}

# install the new bootblock, before we trash /dev/zero
/usr/bin/dd if=${ROOTDISK} of=/tmp/label$$ bs=1b count=1 2>/dev/null
/usr/bin/cat /tmp/label$$ $BOOTBLK | \
	/usr/bin/dd of=$ROOTDISK bs=1b count=16 conv=sync 2>/dev/null
/usr/bin/rm -f /tmp/label$$
echo "" > /AUTOINSTALL		# mark as "stub installed"
/usr/bin/sync

# ok, now snarf the 5.X stub off the media and reboot it
# because we want the cpio list to be fairly static, the
#  goodies hidden down in a version dependant path are manually copied
/usr/bin/mkdir -p /${USRLNK}
/usr/bin/mkdir /${USRLNK}/bin
/usr/bin/cp ${KVMDIR}/${USRLNK}/bin/ls /${USRLNK}/bin
/usr/bin/cp ${KVMDIR}/${USRLNK}/bin/expr /${USRLNK}/bin
/usr/bin/cp ${KVMDIR}/${USRLNK}/bin/grep /${USRLNK}/bin
/usr/bin/mkdir /${USRLNK}/sbin
/usr/bin/cp ${KVMDIR}/${USRLNK}/bin/find /${USRLNK}/bin
/usr/bin/cp ${KVMDIR}/${USRLNK}/sbin/drvconfig /${USRLNK}/sbin
/usr/bin/cp ${KVMDIR}/${USRLNK}/sbin/modload /${USRLNK}/sbin
/usr/bin/cp ${KVMDIR}/${USRLNK}/sbin/devlinks /${USRLNK}/sbin
/usr/bin/cp ${KVMDIR}/${USRLNK}/sbin/disks /${USRLNK}/sbin
/usr/bin/mkdir /${USRLNK}/lib
/usr/bin/cp ${KVMDIR}/${USRLNK}/lib/ld.so.* /${USRLNK}/lib
/usr/bin/ln /${USRLNK}/lib/ld.so.* /${USRLNK}/lib/ld.so
/usr/bin/sync

# these two are for the copy of ufs fsck, needed to preen disk after boot
/usr/bin/cp ${KVMDIR}/etc/lib/libadm.so* /${USRLNK}/lib
/usr/bin/cp ${KVMDIR}/etc/lib/libdl.so* /${USRLNK}/lib
/usr/bin/mkdir /etc
/usr/bin/cp ${KVMDIR}/../../../../${USRLNK2}/lib/fs/ufs/fsck /etc/fsck
/usr/bin/sync

# add the correct rcS 
/usr/bin/mkdir /sbin
/usr/bin/cp ${KVMDIR}/sbin/rcS.stub /sbin/rcS
/usr/bin/sync

# now cpio over the rest of the stuff (in the list)
cd ${KVMDIR}
/usr/bin/cpio -pdum / < ${KVMDIR}/.stub.4x.cpio
# /dev/zero and many other things are trash - only static things after here
${STUB}/xxx sync -
${STUB}/xxx sync -

${STUB}/xxx umount /usr
${STUB}/xxx rmdir /usr
${STUB}/ln -s ${USRLNK} /usr
${STUB}/xxx sync -
${STUB}/xxx sync -
${STUB}/xxx sync -
${STUB}/xxx sync -
${STUB}/xxx sync -
${STUB}/xxx reboot "${REBOOT}"

