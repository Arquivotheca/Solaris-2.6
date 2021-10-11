#!/sbin/sh
#
#       @(#)sysconfig.sh 1.70 96/07/16 SMI
#
# Copyright (c) 1992-1996 Sun Microsystems, Inc.  All Rights Reserved. Sun
# considers its source code as an unpublished, proprietary trade secret, and
# it is available only under strict license provisions.  This copyright
# notice is placed here only to protect Sun in the event the source is
# deemed a published work.  Dissassembly, decompilation, or other means of
# reducing the object code to human readable form is prohibited by the
# license agreement under which this code is provided to the user or company
# in possession of this copy.
#
# RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the Government
# is subject to restrictions as set forth in subparagraph (c)(1)(ii) of the
# Rights in Technical Data and Computer Software clause at DFARS 52.227-7013
# and in similar clauses in the FAR and NASA FAR Supplement.
#
# NOTE: At this point on non-Intel systems, the locale has
#	not yet been configured
#
# NOTE:	This script may be called from outside the window
#	system (boot with "nowin" or kdmconfig/prtconf 
#	configuration failed) or within the window system
#
# NOTE:	This script is run from the install media, and is also
#	distributed as part of the Solaris product. We are
#	using the presence of /usr/sbin/installtool as the
#	indicator as to which image we are booted under. Is
#	this reasonable?
#
INSTALLBOOT=/tmp/.install_boot
PREINSTALL=/tmp/.preinstall
LOCKFILE=/tmp/.suninstall
PLATFORM=`/sbin/uname -p`

# create a lock file if one does not already exist; this is to prevent
# the suninstall script from being invoked more than once
#
si_single_lock() {
	if [ ! -f ${LOCKFILE} ]; then
		touch ${LOCKFILE}
	fi
}

# remove the suninstall lock if it exists.
#
si_single_unlock() {
	rm -f ${LOCKFILE}
}

#
# Search for the sysid configuration file
# on the bootparams.
#
bootparams_sysid_config()
{
	MOUNT_CONFIG_DIR=/tmp/sysid_config.$$
	mkdir ${MOUNT_CONFIG_DIR}

	#
    # Check for a 'sysid_config' bootparams entry
	#
	set -- `/sbin/bpgetfile -retries 1 sysid_config`
	if [ $1"X" != "X" ]; then
		mount -o ro -F nfs $2:$3  \
			${MOUNT_CONFIG_DIR} >/dev/null 2>&1
					 
		if [ $? -eq 0 ]; then
			if [ -f ${MOUNT_CONFIG_DIR}/sysidcfg ]; then
				cp ${MOUNT_CONFIG_DIR}/sysidcfg /etc
				echo "Using sysid configuration file ${2}:${3}/sysidcfg"
				return 0
			fi
			umount ${MOUNT_CONFIG_DIR}
		fi
	fi
	rmdir ${MOUNT_CONFIG_DIR}

	return 1
}

#
# Search for the sysid configuration file
# on the local floppy drive.  Try to mount
# PCFS first and then UFS
#
floppy_sysid_config()
{
	MOUNT_CONFIG_DIR=/tmp/sysid_config.$$

    # Check to see if there is a floppy in the drive (silently)
    #
    /usr/bin/eject -q floppy >/dev/null 2>&1

    if [ $? -eq 0 ]; then
        # Make the mount point directory used in searching
        # for the profile
        #
        if [ ! -d ${MOUNT_CONFIG_DIR} ]; then
            mkdir ${MOUNT_CONFIG_DIR} 2>/dev/null
        fi

        # Try to mount the floppy first as PCFS, then as UFS
        #
        mount -o ro -F pcfs /dev/diskette ${MOUNT_CONFIG_DIR} \
                            >/dev/null 2>&1
        status=$?
        if [ ${status} -ne 0 ]; then
            mount -o ro -F ufs  /dev/diskette ${MOUNT_CONFIG_DIR} \
                            >/dev/null 2>&1
            status=$?
        fi

	if [ ${status} -eq 0 ]; then
		if [ -f ${MOUNT_CONFIG_DIR}/sysidcfg ]; then
			cp ${MOUNT_CONFIG_DIR}/sysidcfg /etc
			echo "Using sysid configuration file from local floppy"
			return 0
		fi
		umount ${MOUNT_CONFIG_DIR}
	fi

	rmdir ${MOUNT_CONFIG_DIR}
	fi

	return 1
}


#################################
#################################
# Main Script Body
#

echo "The system is coming up.  Please wait."

# lock the startup process silently
#
si_single_lock

#
# if we're doing a jumpstart, and the terminal type is not already set,
# and we're not in the window system, see if there is a terminal type
# available to us from the net.
# There is similar logic in the startup script to handle the case of
# running sysidnet in the "locale" mode.
# Note: This is a private interface and may change at any time.
#
if [ -z "${TERM}" -a -z "${DISPLAY}" ]; then
	if [ -f ${PREINSTALL} -o -f ${INSTALLBOOT} ]; then
		set -- `/sbin/bpgetfile term`
		if [ $# -eq 3 ]; then
			if [ ! -z "$3" ]; then
				TERM="$3"
			fi
		elif [ $# -eq 2 ]; then
			if [ ! -z "$2" ]; then
				TERM="$2"
			fi
		fi
		if [ ! -z "${TERM}" ]; then
			echo "TERM set to \"${TERM}\"."
			export TERM
		fi
	fi
fi

#
# Search for the sysidtool configuration
# in the following order
#	floppy (PCFS)
#	floppy (UFS)
#	network (bootparams)
#
#echo "Searching for sysid configuration file..."
floppy_sysid_config
if [ $? -ne 0 ]; then
	bootparams_sysid_config
#	if [ $? -ne 0 ]; then
#		echo "No sysid configuration file found"
#	fi
fi

if [ -z "$MEM_THRESHOLD" ]; then
        MEM_THRESHOLD=4750 
fi

if [ -n "${DISPLAY}" ]; then
	if [ -f /tmp/.mb_locale ]; then
		MEMSIZE=`/sbin/mem`
	        echo "sysconfig available memory: ${MEMSIZE} ${MEM_THRESHOLD}" \
			>> /tmp/root/var/sadm/system/logs/sysidtool.log
		if [ "${MEMSIZE}" -gt ${MEM_THRESHOLD} ]; then
			WM_LOC=`cat /tmp/.mb_locale`
			xrdb -merge -nocpp - <<-EOF
			OpenWindows.BasicLocale:        $WM_LOC
			OpenWindows.DisplayLang:        $WM_LOC
			OpenWindows.Numeric:            $WM_LOC
			EOF
		else
echo "Warning: Insufficient memory for localized install; using English."
			SYSID_MULTI=-m
			LANG=C; export LANG
		fi
	fi
fi

#
#*****	S30sysid.net
# Configure the network interface. Invoke sysidnet, which gets hostname
# & if on net, IP address
#
# invoke sysidnet, which gets hostname & if on net, IP address
#

/usr/sbin/sysidnet -y ${SYSID_MULTI}

#
# Source in all LANG, LC_*, and TZ settings and export them
# Make sure LANG doesn't already have a value (it is initially C).
#
LANG=""
. /etc/default/init
export TZ
export LANG LC_COLLATE LC_CTYPE LC_MESSAGES LC_MONETARY LC_NUMERIC LC_TIME

#
# If LANG was set, make sure to explicitly set all LC_* values
#
if [ "${LANG}" ]; then
	LC_COLLATE=${LANG}
	LC_CTYPE=${LANG}
	LC_MESSAGES=${LANG}
	LC_MONETARY=${LANG}
	LC_NUMERIC=${LANG}
	LC_TIME=${LANG}
fi

# If we are running in the window system with a non-default message
# locale, update the X resources, the openwin workspace menu, and
# the console title bar for the locale
#
if [ -n "${DISPLAY}" -a "${LC_MESSAGES}" -a "${LC_MESSAGES}" != "C" ]; then
	LANG_DIR="/usr/lib/locale/${LC_MESSAGES}/LC_MESSAGES"
	if [ -f ${LANG_DIR}/install-openwin-menu.gui ]; then
		cp ${LANG_DIR}/install-openwin-menu.gui \
			/tmp/root/.openwin-menu 2>/dev/console
	fi
	xrdb -merge -nocpp - <<-EOF
	OpenWindows.BasicLocale:	${LC_MESSAGES}
	OpenWindows.DisplayLang:	${LC_MESSAGES}
	OpenWindows.Numeric:		${LC_NUMERIC}
	EOF

	# Localize the title bar
	# NOTE:	current escape sequence is for cmdtool
	#
	TITLE=`gettext SUNW_INSTALL_SCRIPTS 'Solaris Install Console'`
	echo "]l${TITLE}\\" > /dev/console
fi

#*****	S71rpc
#
# Run the pertinent startup activities for the rpc facility:
#	1) sysidnis
#	2) keyserv
#
echo "Starting remote procedure call (RPC) services:\c"
#gettext " rpcbind"
#/usr/sbin/rpcbind > /dev/console 2>&1

echo " sysidnis\c"
/usr/sbin/sysidnis -y

# NOTE:	/var/nis/$hostname will never exist at this point
# 
if [ -f /var/nis/NIS_COLD_START ]; then
	echo " keyserv\c"
	/usr/sbin/keyserv > /dev/console 2>&1
fi

echo " done."

#*****	S71sysid.sys
#
# invoke sysidsys, which completes configuration of various system attributes.
#

/usr/sbin/sysidsys

#***** S72inetsvc
# 
# This is third phase of TCP/IP startup/configuration.  This script
# runs after the NIS/NIS+ startup script.  We run things here that may
# depend on NIS/NIS+ maps.
#

# 
# Re-set the netmask and broadcast addr for all IP interfaces.  This
# ifconfig is run here, after NIS has been started, so that "netmask
# +" will find the netmask if it lives in a NIS map.
#

/usr/sbin/ifconfig -au netmask + broadcast + > /dev/null

#
# Add a static route for multicast packets out our default interface.
# The default interface is the interface that corrsponds to the node name.
#
#echo "Setting default interface for multicast: \c"
#/usr/sbin/route add "224.0.0.0" "`uname -n`" 0

#
# Run inetd in "standalone" mode (-s flag) so that it doesn't have
# to submit to the will of SAF.
#
#/usr/sbin/inetd -s

#*****  S73nfs.client

#
# Start processes required for client NFS
#
# NOTE:	Do we want to pull this code since it's commented out?
#

#/usr/lib/nfs/statd > /dev/console 2>&1
#/usr/lib/nfs/lockd > /dev/console 2>&1

# Start the automounter.
# Use the NIS auto.master and/or /etc/auto.master if present
# If there are no master files it quietly exits.
#/usr/lib/nfs/automount > /dev/console 2>&1

# exiting the sysconfig state so remove the lock file
si_single_unlock

#
# create the clustertoc to be used by the installation process
#
if [ -x /usr/sbin/install.d/parse_dynamic_clustertoc ]; then
	/usr/sbin/install.d/parse_dynamic_clustertoc	
fi

exec /sbin/suninstall
