#!/sbin/sh
#
#       @(#)startup.sh 1.63 96/04/01 SMI
#
# Copyright (c) 1992-1995 Sun Microsystems, Inc.  All Rights Reserved. Sun
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
# NOTE:	run as the runlevel 234 process; invoked from the inittab; invokes
#	sysconfig with all necessary environment setup
#

SHELL=/sbin/sh
TEXTDOMAIN=SUNW_INSTALL_SCRIPTS
PATH=/sbin:/usr/sbin/install.d:${PATH}

PLATFORM=`/sbin/uname -p`

# make home dir to a writeable place
HOME=/tmp/root
export SHELL TEXTDOMAIN PATH PLATFORM HOME

# Local Variables
RUN_WIN=1
RUN_KDM=0

#
#
#

cd ${HOME}

#
# If the run level is changed after invocation, shell should
# be run
#
if [ -f /tmp/.sh ]; then
	exec ${SHELL}
else
	touch /tmp/.sh
fi

# Determine if we are running on a multi-byte image.  If so, save the
# locale.
ENTRY=`ls /usr/lib/locale/*/LC_MESSAGES/openwin-defaultfont 2>/dev/null`
if [ -n "${ENTRY}" ]; then
	COUNT=`echo ${ENTRY} | wc -w`
	if [ ${COUNT} -eq 1 ]; then
		MB_LOC=`expr "$ENTRY" : '.*locale\/\(.*\)\/LC.*'`
		echo $MB_LOC >/tmp/.mb_locale
	fi
fi

##########
# Make sure all configuration necessary is completed in order
# to run the window system

if [ "${PLATFORM}" = "i386" ]; then
	#
	# For i386, we know what the terminal type will be. Avoid asking the
	# question inside of sysidnet
	#
	TERM=AT386; export TERM
	RUN_KDM=1
else
	#
	# Configure the frame buffer links
	#
	fbdev=`ls /devices\`/usr/sbin/prtconf -F 2>&1\`* 2>/dev/null`

	if [ $? -ne 0 ]; then
		RUN_WIN=0
	else
		set `echo ${fbdev}`
		rm -f /dev/fb
		ln -s $1 /dev/fb

		# XXX temporary
		# NOTE: Why is this temporary and what is its purpose?
		#
		cd /dev/fbs
		for nm in *
		do
			if [ ! -h /dev/$nm ]; then
				ln -s /dev/fbs/$nm /dev/$nm
			fi
		done

		# The code below for handling leo framebuffer configuration
		# should be generalized into a callout which can be
		# used for any framebuffer

		echo ${fbdev} | grep "SUNW\,leo" >/dev/null 2>&1
		if [ $? -eq 0 ]; then
			if [ -x /etc/rc2.d/S91leoconfig ]; then
				/etc/rc2.d/S91leoconfig
			fi
		fi

		echo ${fbdev} | grep "PFU\,aga" >/dev/null 2>&1
		if [ $? -eq 0 ]; then
			 if [ -x /etc/rc2.d/S91agaconfig ]; then
					 /etc/rc2.d/S91agaconfig start
			 fi
		fi


		cd ${HOME}

		if [ "${PLATFORM}" = "ppc" ]; then
			RUN_KDM=1
		fi
	fi
fi

#
# non-SPARC systems require the keyboard/mouse/display hardware be configured
# for the sake of creating the OWconfig file.
# To do this, we try to set the correct locale (the Intel and PPC console
# display's are 8-bit clean so this may work.
# Run kdmconfig to configure the OWconfig file.
#
if [ "${RUN_KDM}" -eq 1 ]; then
	#
	# If we're doing a jumpstart, see if there is a terminal type available
	# to us from the net.  We do a similar piece of logic in the sysconfig
	# script to catch the case of sparc systems (which don't run
	# kdmconfig).
	# Note: This is a private interface and may change at any time.
	#
	if [ -z "${TERM}" ]; then
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
	# Initialize the locale information in /tmp/root/etc/default/init,
	# and set the LANG variable accordingly. From this point on all
	# messages and executable should be running internationalized.
	# Check if we're on a multi-byte image.  If so, we don't want to
	# do any locale work now.
	#
	if [ -f /tmp/.mb_locale ]; then
		NO_MULTI=-m
	fi

	/usr/sbin/sysidnet -l -y ${NO_MULTI}

	LANG=`grep LANG /tmp/root/etc/default/init 2>/dev/null | \
		    sed -e '/^#/d' -e 's/.*=//'`
	if [ -n "${LANG}" ]; then
		export LANG
	else
		LC_MESSAGES=`grep LANG /tmp/root/etc/default/init 2>/dev/null \
			| sed -e '/^#/d' -e 's/.*=//'`
		if [ -n "${LC_MESSAGES}" ]; then
			export LC_MESSAGES
		fi
	fi

	#
	# This routine calls kdmconfig, which creates OWconfig
	#
	/usr/sbin/sysidconfig

	#
	# If OWconfig file was not created by sysidconfig, make sure
	# you don't bring up the window system 
	#
	if [ ! -f /tmp/root/etc/openwin/server/etc/OWconfig ]; then
		RUN_WIN=0
	fi
fi

#
# Remove un-needed /dev symlinks, since each one uses a page in tmpfs.
#
rm -f /tmp/dev/pts/4? /tmp/dev/pts/3? /tmp/dev/pts/2? /tmp/dev/pts/1?
rm -f /tmp/dev/win1?? /tmp/dev/win[1-9]? /tmp/dev/win[5-9]
rm -f /tmp/dev/ptyp[5-9] /tmp/dev/ptyp[a-f]
rm -f /tmp/dev/ttyp[5-9] /tmp/dev/ttyp[a-f]
rm -f /tmp/dev/ptyq* /tmp/dev/ptyr* /tmp/dev/ttyq* /tmp/dev/ttyr*

MEMSIZE=`/sbin/mem`
echo "startup available memory: ${MEMSIZE}" \
	>> /tmp/root/var/sadm/system/logs/sysidtool.log
if [ "${MEMSIZE}" -lt 6000 ]; then 
	if [ ! -f /tmp/.nowin ]; then
	        touch /tmp/.nowin 
		echo "startup: insufficient memory for window system" \
			>> /tmp/root/var/sadm/system/logs/sysidtool.log
		echo "Warning: Insufficient memory to start window system."
	fi
fi

# Start up the window system unless the display device cannot be
# determined or the install process with explicitly booted with
# the "-nowin" option.
#
# The window system is necessary to display Internationalized messages.
#
# NOTE:	At this point localization may be set
#
if [ "${RUN_WIN}" -eq 0 -o -f /tmp/.nowin ]; then
	. /sbin/sysconfig
else
	# if the hostname is not yet configured we need a dummy entry
	# in /etc/hosts so the window system will work
	#
	name=`uname -n`
	if [ -z "${name}" ]; then
		uname -S localhost
	fi

	# Set up the customized install user's account directory (/tmp/root);
	# copy the menu file to a writeable area in case localization
	# updates are required by sysconfig; copy the customized openwin-init,
	# Xdefaults, openwin-defaultfont (optional), and Xinitrc files to
	# ${HOME}
	#
	cp -p /usr/lib/locale/C/LC_MESSAGES/install-openwin-menu.gui \
		${HOME}/.openwin-menu 2>/dev/console

	if [ -f /tmp/.mb_locale ]; then
		cp -p \
	/usr/lib/locale/`cat /tmp/.mb_locale`/LC_MESSAGES/openwin-defaultfont \
			${HOME}/.openwin-defaultfont 2>/dev/console
		chmod 755 ${HOME}/.openwin-defaultfont
	fi

	cp -p /usr/sbin/install.d/openwin-init \
		${HOME}/.openwin-init 2>/dev/console
	chmod 755 ${HOME}/.openwin-init
	cp -p /usr/sbin/install.d/Xdefaults \
		${HOME}/.Xdefaults 2>/dev/console
	cp -p /usr/sbin/install.d/Xinitrc \
		${HOME}/.xinitrc 2>/dev/console
	chmod 755 ${HOME}/.xinitrc

	echo "Starting OpenWindows..."

	# Start the window system without authentication
	#
	/usr/openwin/bin/openwin -noauth

	# If openwin fails for some reason, start sysconfig without the window
	# system as if the user had booted with the -nowin option
	#
	if [ $? -ne 0 ]; then
		touch /tmp/.nowin
		. /sbin/sysconfig
	fi
fi
