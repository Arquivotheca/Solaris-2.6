#!/sbin/sh
#
#       @(#)suninstall.sh 1.106 96/10/14 SMI
#
# Copyright (c) 1992-1996 Sun Microsystems, Inc.  All Rights Reserved.

# Local Variables
#
LOCKFILE=/tmp/.suninstall
JSLOCKFILE=/tmp/.jumpstart
LOGDIR=/a/var/sadm/system/logs
RBLOGDIR=/var/sadm/system/logs
OLDLOGDIR=/a/var/sadm
RELLOGDIR=system/logs
INSTALLBOOT=/tmp/.install_boot
PREINSTALL=/tmp/.preinstall
CD_CONFIG_DIR=/cdrom/genie
SMI_INSTALL=/tmp/.smi_boot
BEGIN=begin
BEGINLOG=begin.log
FINISH=finish
FINISHLOG=finish.log

MY_VERSION=2	# used to match version in JumpStart rules file
ESTATUS=0	# exit status

# Global Variables
#
SI_CONFIG_DIR=/tmp/install_config
SI_CONFIG_PROG=rules.ok
SI_CONFIG_FILE=${SI_CONFIG_DIR}/${SI_CONFIG_PROG}
SI_SYS_STATE=/a/etc/.sysIDtool.state
SI_INSTALL_APP=interactive
export SI_CONFIG_DIR SI_CONFIG_PROG SI_SYS_STATE SI_CONFIG_FILE SI_INSTALL_APP

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

# remove the suninstall lock and execute the /sbin/sh
# NOTE:	If the program being executed is not the shell, but is
#	instead one of the install applications should we really
#	be removing this lock at this point?
#
become_shell() {
	si_single_unlock
	exec /sbin/sh
}

# This shell function and the function suninstall_date_time() found
# in .../src/bundled/app/files/suninstall.sh are the same.  If a change
# is made to one, the same change has to be made to the other.
#
# $1 ::= filename to use to determine date/time string
# $2 ::= the directory and filename to check while
#        establishing a unique dated file name
#
suninstall_date_time()
{
	old_name=$1 
	mon=`ls -l $old_name | awk ' { printf("%s", $6) } ' `
	day=`ls -l $old_name | awk ' { printf("%s", $7) } ' `
	year=`ls -l $old_name | awk ' { printf("%s", $8) } ' `
	mon_day="${mon}${day}"

	if echo $year | grep ':' >/dev/null 2>&1
	then
	    case $mon in
	        Jan) mon_ndx=1 ;;
	        Feb) mon_ndx=2 ;;
	        Mar) mon_ndx=3 ;;
	        Apr) mon_ndx=4 ;;
	        May) mon_ndx=5 ;;
	        Jun) mon_ndx=6 ;;
	        Jul) mon_ndx=7 ;;
	        Aug) mon_ndx=8 ;;
	        Sep) mon_ndx=9 ;;
	        Oct) mon_ndx=10 ;;
	        Nov) mon_ndx=11 ;;
	        Dec) mon_ndx=12 ;;
	    esac

	    current_mon_ndx=`date "+%m"`
	    year=`date "+%Y"`
	    if [ "$mon_ndx" -gt "$current_mon_ndx" ] ; then
	    	year=`expr $year - 1`
	    fi
	fi

	new_name="${2}_${mon_day}_${year}"
	ndx="0"
	ndx_str=
	while [ -f ${new_name}${ndx_str} ] ; do
		ndx=`expr $ndx + 1`
		ndx_str="_$ndx"
	done
	echo "${mon_day}_${year}${ndx_str}"
}

# Move the given log (either begin.log or finish.log) into the system 
# log directory.
move_log()
{
    LogName=$1
    #
    # remove any, and all, symbolic links from $LogName
    #
    if [ -h ${OLDLOGDIR}/${LogName} ]
    then
        rm -f ${OLDLOGDIR}/${LogName}
    fi

    if [ -h ${LOGDIR}/${LogName} ]
    then
        rm -f ${LOGDIR}/${LogName}
    fi

    #
    # move $LogName from old directory to new directory with 
    # dated log name.
    #
    if [ -f ${OLDLOGDIR}/${LogName} ]
    then
        date_time=`suninstall_date_time ${OLDLOGDIR}/${LogName} ${LOGDIR}/${LogName}`
        mv ${OLDLOGDIR}/${LogName} ${LOGDIR}/${LogName}_${date_time}
    fi

    #
    # rename $LogName, in new directory, to dated log name
    #
    if [ -f ${LOGDIR}/${LogName} ]
    then
        date_time=`suninstall_date_time ${LOGDIR}/${LogName} ${LOGDIR}/$LogName`
        mv ${LOGDIR}/${LogName} ${LOGDIR}/${LogName}_${date_time}
    fi

    #
    # If $LogName exists in /tmp, then move it to the new directory,
    # a dated log name, and set up appropriate symbolic links.
    #
    if [ -f /tmp/${LogName} ]
    then
        date_time=`suninstall_date_time /tmp/${LogName} ${LOGDIR}/${LogName}`
        mv /tmp/${LogName} ${LOGDIR}/${LogName}_${date_time}
        ln -s ${LogName}_${date_time} ${LOGDIR}/${LogName}
        chmod 644 ${LOGDIR}/${LogName}_${date_time}
        echo `gettext "The ${2} script log \'${LogName}\'"`
        echo `gettext "\tis located in ${RBLOGDIR} after reboot."`
        echo
    fi
}

#################################
#################################
# Main Script Body
#

# check for the suninstall lock and exit if the lock is present
#
if [ -f ${LOCKFILE} ]; then
	echo `gettext "Solaris installation program is already running."`
	exit 1
else
	echo `gettext "Starting Solaris installation program..."`
	si_single_lock
fi

trap "si_single_unlock" 1 2 15	# ignore traps

# set the terminal type if not already set
#
if [ -z "${TERM}" ]; then
	TERM=`tail -1 /etc/.sysIDtool.state`
	export TERM
fi

# make sure "/" is exported with root access
# NOTE: this relies on useradd have mode 500 and returning error 2 for bad args
# NOTE:	there should be a better test here
#
/usr/sbin/useradd >/dev/null 2>&1
if [ $? -eq 1 ]; then
	gettext "Error: Unable to install system."
	echo
	gettext "       The Solaris installation image is not exported with root access."
	echo
	become_shell
fi

#
# Check for SMI Install configuration directories.  If it exists
# and running factory jumpstart or booting with the smi install
# boot parameter start up the SMI Install browser interface.
#
#if  [ -d ${CD_CONFIG_DIR} ]; then
#	if [ -f ${PREINSTALL} -o -f ${SMI_INSTALL} ]; then
if [ -d ${CD_CONFIG_DIR} -a -f ${SMI_INSTALL} ]; then
	/cdrom/genie/bin/smi_install.sh
	#
	# Check the return code
	# if it is non-zero then delete
	# the jumpstart directory so that
	# a normal factory or custom jumpstart
	# will take place
	if [ $? -ne 0 ]; then
		rm -rf ${SI_CONFIG_DIR}	
	fi
fi

# 
# mount the appropriate profile directory and search for the for profile
# for JumpStart and custom JumpStart boots; make sure the use hasn't tried
# to restart JumpStart install
#
if [ -f ${PREINSTALL} -o -f ${INSTALLBOOT} ]; then
	if [ -f ${JSLOCKFILE} ]; then
		gettext "You must reboot the system to restart a JumpStart install."
		echo
		si_single_unlock
		exit 1
	else
		touch ${JSLOCKFILE}
		/usr/sbin/install.d/profind
	fi
fi

# process rules.ok file, validate the version, make sure the version is valid
# for this version of suninstall.sh; if so, invoke chkprobe to process the
# rules.ok file looking for a matching entry
#
if [ -f ${SI_CONFIG_FILE} ]; then
	ver_line=`egrep "# version=" ${SI_CONFIG_FILE}`
	if [ $? -ne 0 ]; then
		gettext "Error: Could not find rules file "
		echo "(${SI_CONFIG_FILE})"
	else
		VERSION=`expr "${ver_line}" : '.*version=\(.*\) .*'`
		if [ ${VERSION} -gt ${MY_VERSION} ]; then
			gettext "Error: File version is greater than expected: "
			echo "(${SI_CONFIG_FILE})"
			echo
		else
			echo `gettext "Checking rules.ok file..."`
			si_single_unlock
			. /usr/sbin/install.d/chkprobe
			si_single_lock
		fi
	fi
fi

# set the application type for JumpStart and custom JumpStart boots;
# custom JumpStart boots require a profile or else the interactive
# install will run
#
if [ -f ${INSTALLBOOT} ]; then
	# make sure that SI_CLASS is defined; remember it could be
	# '=' for derived classes, so use the 'X' test method
	#
	if [ X"${SI_CLASS}" != "X" ]; then
		SI_INSTALL_APP=jumpstart
	else
		gettext "Warning: Could not find matching rule in "
		echo ${SI_CONFIG_PROG}
		gettext "Press the return key for an interactive Solaris install program..."
		read input
	fi
fi

if [ -f ${PREINSTALL} ]; then
	# make sure that SI_CLASS is defined; remember it could be
	# '=' for derived classes, so use the 'X' test method
	#
	if [ X"${SI_CLASS}" != "X" ]; then
		SI_INSTALL_APP=jumpstart
	fi
fi

# if running in the window system, determine the screen dimensions
#
if [ -n "${DISPLAY}" ]; then
	export SCREENWIDTH SCREENHEIGHT
	eval `xwininfo -root | \
		sed -n -e 's/Height: /SCREENHEIGHT=/p' \
		-e 's/Width: /SCREENWIDTH=/p'`
fi

# if running the window system and using the JumpStart application,
# expand the hosting cmdtool so the JumpStart data can be more easily
# read
#
if [ -n "${DISPLAY}" -a "${SI_INSTALL_APP}" = "jumpstart" ]; then
	if [ ${SCREENHEIGHT} -gt 480 ]; then
		echo "[8;35;t"
	else
		echo "[4;${SCREENHEIGHT};t"
	fi
fi

# process jumpstart "begin" script if defined
#
if [ -n "${SI_BEGIN}" -a X"${SI_BEGIN}" != "X-" ]; then
	if [ ! -f "${SI_CONFIG_DIR}/${SI_BEGIN}" ]; then
		echo `gettext "Warning: Could not find begin script "` \
			" (${SI_BEGIN})" | tee -a /tmp/${BEGINLOG}
	else
		echo `gettext "Executing begin script \"${SI_BEGIN}\"..."`
		echo
		si_single_unlock
		/sbin/sh ${SI_CONFIG_DIR}/${SI_BEGIN} 2>&1 | \
			tee -a /tmp/${BEGINLOG}
		si_single_lock
		echo
		gettext "Begin script ${SI_BEGIN} execution completed."
		echo
	fi
fi

# process a jumpstart installation
#
if [ "${SI_INSTALL_APP}" = "jumpstart" ]; then
	if [ ! -f "${SI_PROFILE}" ]; then
		# no profile file; prompt the user to continue with
		# an interactive installation.
		#
		gettext "Error: Could not find the profile "
		echo "(${SI_PROFILE})"
		gettext "Press the return key for an interactive Solaris install..."
		read input
		SI_INSTALL_APP=interactive
	else
		pfinstall ${SI_PROFILE}
		ESTATUS=$?
	fi
fi

# process interactive installations
#
if [ "${SI_INSTALL_APP}" = "interactive" ]; then
	# select the helpfile directory based on the status of the
	# L10N environment variables
	#
	if [ -n "${LC_MESSAGES}" ]; then
		HELPPATH=/usr/openwin/lib/locale/${LC_MESSAGES}/help
	elif [ -n "${LANG}" ]; then
		HELPPATH=/usr/openwin/lib/locale/${LANG}/help
	else
		HELPPATH=/usr/openwin/lib/locale/C/help
	fi

	# execute appropriate interactive install app based on the availability
	# of the window system
	#
	if [ -n "${DISPLAY}" ]; then
		# set the path for installtool help text messages
		ADMINHELPHOME=${HELPPATH}/installtool.help
		export ADMINHELPHOME
		/usr/sbin/installtool
		ESTATUS=$?
	else
		# set the path for installtool help text messages
		ADMINHELPHOME=${HELPPATH}/install.help
		export ADMINHELPHOME
		/usr/sbin/ttinstall
		ESTATUS=$?
	fi
fi

# if the installation application executed exited in error, invoke a
# Bourne shell
#
if [ ${ESTATUS} -eq 2 ]; then
	become_shell
fi

# process jumpstart "finish" script if defined
#
if [ -n "${SI_FINISH}" -a X"${SI_FINISH}" != "X-" ]; then
	if [ ! -f "${SI_CONFIG_DIR}/${SI_FINISH}" ]; then
		echo `gettext "Warning: Could not find finish script "` \
			"(${SI_FINISH})" | tee -a /tmp/${FINISHLOG}
	else
		echo `gettext "Executing finish script \"${SI_FINISH}\"..."`
		echo
		si_single_unlock
		/sbin/sh ${SI_CONFIG_DIR}/${SI_FINISH} 2>&1 | tee -a /tmp/${FINISHLOG}
		si_single_lock
		echo
		gettext "Finish script ${SI_FINISH} execution completed."
		echo
	fi
fi

# spacing for final messages
#
echo

# Move the begin log into the system log directory
#
move_log $BEGINLOG $BEGIN

# Move the finish log into the system log directory
#
move_log $FINISHLOG $FINISH

# exiting the install routine so remove the lock file
si_single_unlock

# reboot only if the install application exited with a '0' status
#
if [ ${ESTATUS} -eq 0 ]; then
	reboot
else
	become_shell
fi
