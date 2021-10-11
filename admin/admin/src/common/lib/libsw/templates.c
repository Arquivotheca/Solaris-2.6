#ifndef lint
#ident   "@(#)templates.c 1.85 95/11/22 SMI"
#endif

/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 */
char *script_start[] = {
"#!/bin/sh",
"logprogress() {",
"	echo \"LASTCMD=$1\\nTIMESTAMP=$timestamp\" > $restart",
"	sync",
"	cp $restart $restartbkup",
"	sync",
"}",
"recover_file() {",
"	if [ -f $1.bk ] ; then",
"		cp $1.bk $1",
"		rm $1.bk",
"	fi",
"}",
"log_file_diff() {",
"	original_target=$3",
"	new_target=$4",
"	case $3 in",
"	f|e|v)",
"		actual_type=`gettext 'regular file'`",
"	;;",
"	d|x)",
"		actual_type=`gettext 'directory'`",
"	;;",
"	p)",
"		actual_type=`gettext 'named pipe'`",
"	;;",
"	c)",
"		actual_type=`gettext 'character special device'`",
"	;;",
"	b)",
"		actual_type=`gettext 'block special device'`",
"	;;",
"	s)",
"		actual_type=`gettext 'symbolic link'`",
"	;;",
"	esac",
"	case $4 in",
"	f|e|v)",
"		exp_type=`gettext 'regular file'`",
"	;;",
"	d|x)",
"		exp_type=`gettext 'directory'`",
"	;;",
"	p)",
"		exp_type=`gettext 'named pipe'`",
"	;;",
"	c)",
"		exp_type=`gettext 'character special device'`",
"	;;",
"	b)",
"		exp_type=`gettext 'block special device'`",
"	;;",
"	s)",
"		exp_type=`gettext 'symbolic link'`",
"	;;",
"	esac",
"	case $1 in",
"	DIFF_TYPE)",
"	eval echo `gettext SUNW_INSTALL_SWLIB '$2: file type was changed from $exp_type to $actual_type.'` >> $coalesce",
"	;;",
"	DIFF_MISSING)",
"	eval echo `gettext SUNW_INSTALL_SWLIB '$2: had been deleted and has now been restored.'` >> $coalesce",
"	;;",
"	DIFF_SLINK_TARGET)",
"	eval echo `gettext SUNW_INSTALL_SWLIB '$2: target of symbolic link was changed from $original_target to $new_target.'` >> $coalesce",
"	;;",
"	DIFF_HLINK_TARGET)",
"	eval echo `gettext SUNW_INSTALL_SWLIB '$2: target of hard link was changed from $original_target.'` >> $coalesce",
"	;;",
"	esac",
"	cleanup_needed=1",
"}",
"rename_file() {",
"	(cd $rbase; cd ./$1",
"	if [ -r ./$2:$3 ] ; then ",
"		mv ./$2:$3 ./$2:$3~",
"		arg1=\"$1$2:$3\"",
"		arg2=\"$1$2:$3~\"",
"	fi",
"	mv ./$2 ./$2:$3 )",
"	arg1=\"$1$2\"",
"	arg2=\"$1$2:$3\"",
"	eval echo `gettext SUNW_INSTALL_SWLIB '$arg1: existing file renamed to $arg2'` >> $coalesce",
"	cleanup_needed=1",
"}",
"do_pkgadd() {",
"	eval echo `gettext SUNW_INSTALL_SWLIB 'Doing pkgadd of $2 to $1.'`",
"	rm -f /tmp/CLEANUP",
"	(if [ \"$1\" = \"/\" ] ; then",
"	   if [ \"${rbase}\" = \"/\" ] ; then",
"	      pkgadd -S -d $3 -a /tmp/admin.$4.$$ -n $2",
"	   else",
"	      pkgadd -S -R ${rbase} -d $3 -a /tmp/admin.$4.$$ -n $2",
"	   fi",
"	else",
"	   pkgadd -S -R ${base}$1 -d $3 -a /tmp/admin.$4.$$ -n $2 ",
"	fi",
"	errcode=$?",
"	if [ $errcode != 0 ] ; then",
"		echo `gettext SUNW_INSTALL_SWLIB 'pkgadd return code = '` $errcode",
"		echo $2 >> $failedpkgs",
"	fi) 2>&1 | sed -f /tmp/filter.$$",
"	if [ -f /tmp/CLEANUP ] ; then",
"		cat /tmp/CLEANUP | {",
"			while read key arg1 arg2",
"			do",
"				case $key in",
"				EXISTING_FILE_RENAMED:)",
"				eval echo `gettext SUNW_INSTALL_SWLIB '$arg1: existing file renamed to $arg2'` ;;",
"				EXISTING_FILE_PRESERVED:)",
"				eval echo `gettext SUNW_INSTALL_SWLIB '$arg1: existing file preserved, the new version was installed as $arg2'` ;; ",
"				SYSADMIN_NOT_14)",
"				echo",
"				eval echo `gettext SUNW_INSTALL_SWLIB '$arg1:  Caution!'`",
"				gettext 'Group \"sysadmin\" is already defined in the above file.  However, the group\\nid for \"sysadmin\" is defined as something other than 14.  This is a hole in\\nsecurity, because admintool\\(1M\\) grants all users in group \"sysadmin\" the\\nright to run privileged admintool operations on remote hosts.  You must set\\ngroup \"sysadmin\" equal to group id 14 in the above file \\(the appropriate\\nentry of \"sysadmin::14:\" has been appended to the end of the above file\\nautomatically for you.\\)'",
"				echo \"\\n\" ;;",
"				GROUP14_IN_USE)",
"				echo",
"				eval echo `gettext SUNW_INSTALL_SWLIB '$arg1:  Caution!'`",
"				gettext 'Group id 14 is already defined in the above file.  However, the group name\\nfor group id 14 is defined as something other than \"sysadmin\".  This is a\\nhole in security, because admintool\\(1M\\) grants all users in group id 14 the\\nright to run privileged admintool operations on remote hosts.  You must set\\ngroup id 14 equal to group \"sysadmin\" in the above file \\(the appropriate entry\\nof \"sysadmin::14:\" has been appended to the end of the above file\\nautomatically for you.\\) You will also have to change all files on your\\nsystem which are currently owned by group id 14 to the new group id.'",
"				echo \"\\n\" ;;",
"				*)",
"				echo $key $arg1 $arg2 ;;",
"				esac",
"			done",
"		} >> $coalesce",
"		cleanup_needed=1",
"		rm -f /tmp/CLEANUP",
"	fi",
"}",
"do_removef() {",
"	if [ \"$1\" = \"/\" ] ; then",
"	   cat /tmp/rmlist.$$ | xargs removef -R ${rbase} $2 | sort -r | \\",
"		while read path",
"		do",
"			if [ \"$path\" != \"\" ] ; then",
"				if [ -d \"$path\" ] ; then",
"					rmdir $path >/dev/null 2>&1",
"				else",
"					rm -f $path",
"				fi",
"			fi",
"		done",
"	else",
"	   cat /tmp/rmlist.$$ | xargs removef -R ${base}$1 $2 | sort -r | \\",
"		while read path",
"		do",
"			if [ \"$path\" != \"\" ] ; then",
"				if [ -d \"$path\" ] ; then",
"					rmdir $path >/dev/null 2>&1",
"				else",
"					rm -f $path",
"				fi",
"			fi",
"		done",
"	fi",
"	if [ \"$1\" = \"/\" ] ; then",
"	   removef -R ${rbase} -f $2 >/dev/null",
"	else",
"	   removef -R ${base}$1 -f $2 >/dev/null",
"	fi",
"	errcode=$?",
"	if [ $errcode != 0 ] ; then",
"		echo `gettext SUNW_INSTALL_SWLIB 'removef return code = '` $errcode",
"	fi",
"}",
"add_rdot_files() {"
"	(if [ -d $1/var/sadm/pkg/$2/install ] ; then",
"	    cd $1/var/sadm/pkg/$2/install",
"	    for f in i.* ; do",
"		if [ \"$f\" = 'i.*' ] ; then",
"		    break;",
"		fi",
"		class=`expr $f : 'i\\.\\(.*\\)'`",
"		if [ ! -f r.$class ] ; then",
"		    echo \"exit 0\" > r.$class",
"		fi",
"	    done",
"	fi)",
"}",
"do_pkgrm() {",
"	if [ \"$1\" = \"/\" ] ; then",
"		rmbase=${rbase}",
"	else",
"		rmbase=${base}$1",
"	fi",
"	add_rdot_files $rmbase $2",
"	eval echo `gettext SUNW_INSTALL_SWLIB 'Removing package $2:'`",
"	pkgrm -R ${rmbase} -a /tmp/admin.dflt.$$ -n $2",
"	errcode=$?",
"	if [ $errcode != 0 ] ; then",
"		echo `gettext SUNW_INSTALL_SWLIB 'pkgrm return code = '` $errcode",
"	fi",
"}",
"do_pkgrm_f() {",
"	if [ \"$1\" = \"/\" ] ; then",
"		rmbase=${rbase}",
"	else",
"		rmbase=${base}$1",
"	fi",
"	add_rdot_files $rmbase $2",
"	pkgrm -R ${rmbase} -a /tmp/admin.dflt.$$ -F -n $2",
"	errcode=$?",
"	if [ $errcode != 0 ] ; then",
"		echo `gettext SUNW_INSTALL_SWLIB 'pkgrm return code = '` $errcode",
"	fi",
"}",
"do_install_inetboot() {",
"  bootdir=$1",
"  karch=$2",
"  svcprod=$3",
"  svcver=$4",
"  (cd ${base}/${bootdir}",
"  if [ -f inetboot.${karch}.${svcprod}.${svcver} -a \\",
"	! -h inetboot.${karch}.${svcprod}.${svcver} ] ; then",
"     old=inetboot.${karch}.${svcprod}.${svcver}",
"  elif [ -f inetboot.${karch}.${svcprod}_${svcver} -a \\",
"	! -h inetboot.${karch}.${svcprod}_${svcver} ] ; then",
"     old=inetboot.${karch}.${svcprod}_${svcver}",
"  else",
"     old=",
"  fi",
"  inetcopy=",
"  if [ \"$old\" != \"\" ] ; then",
"	for i in inetboot.* ; do",
"	    if [ \"$i\" = \"inetboot.*\" ] ; then",
"		break",
"	    fi",
"	    if [ ! -h $i ] ; then",
"		continue",
"	    fi",
"	    link=`/bin/ls -l $i | sed 's/^.*-> \\(.*\\)/\\1/'`",
"	    link2=`expr $link : '\\.\\/\\(.*\\)'`",
"	    if [ \"$link2\" != \"\" ] ; then",
"		link=$link2",
"	    fi",
"	    if [ \"$link\" = \"$old\" ] ; then",
"		rm -f $i",
"		if [ \"$inetcopy\" != \"\" ] ; then",
"		    ln -s $inetcopy $i",
"		else",
"		    mv $old $i",
"		    inetcopy=$i",
"		fi",
"	    fi",
"	done",
"   fi",
"   rm -f inetboot.${karch}.${svcprod}[._]${svcver}",
"   for i in inetboot.*.${svcprod}_${svcver} ; do",
"	    if cmp -s $i $inetboot ; then",
"		ln -s $i inetboot.${karch}.${svcprod}_${svcver}",
"		break",
"	    fi",
"   done",
"   if [ ! -h inetboot.${karch}.${svcprod}_${svcver} ] ; then",
"       cp $inetboot inetboot.${karch}.${svcprod}_${svcver}",
"   fi",
"   if [ \"${bootdir}\" = \"rplboot\" ] ; then",
"	cp $gluedir/gluecode.com .",
"	cp $gluedir/smc.com .",
"   fi",
"   )",
"}",
"mount_filesys() {",
"/usr/sbin/fsck -m -F $1 $4 >/dev/null 2>&1",
"case $? in",
"  0)	/sbin/mount -F $1 $2 ${base}$3",
"	if [ $? -ne 0 ]; then",
"		eval echo `gettext SUNW_INSTALL_SWLIB 'Unable to mount ${base}$4'` $? ",
"		exit 2",
"	fi",
"	;;",
"  32)	mount | grep \" $2 \"  >/dev/null 2>&1 ",
"	if [ $? -ne 0 ]; then",
"		eval echo `gettext SUNW_INSTALL_SWLIB 'The ${base}$3 file system $4 is being checked.'`",
"		/usr/sbin/fsck -F $1 -o p $4",
"		case $? in",
"		  0|40)	;;",
"		  *)	eval echo `gettext SUNW_INSTALL_SWLIB 'Unable to fsck ${base}$3'` $?",
"			exit 2 ;;",
"		esac",
"		/sbin/mount -F $1 $2 ${base}$3",
"		if [ $? -ne 0 ]; then",
"			eval echo `gettext SUNW_INSTALL_SWLIB 'Unable to mount ${base}$3'` $?",
"			exit 2",
"		fi",
"	else",
"		mount | grep \" $2 \" | while read real_mntpt junk real_device junk",
"		do",
"		if [ \"$real_mntpt\" != \"$3\" ]; then",
"			eval echo `gettext SUNW_INSTALL_SWLIB 'Unable to mount ${base}$3'` $?",
"			eval echo `gettext SUNW_INSTALL_SWLIB '$2 mounted at ${base}$real_mntpt'` $?",
"			exit 2",
"		fi",
"		done",
"	fi",
"	;;",
"  33)  ;;",
"  *)	eval echo `gettext SUNW_INSTALL_SWLIB 'Unrecognized error from fsck of ${base}$3'` $?",
"	exit 2 ;;",
"esac",
"}",
"remove_patch() {",
"ls -L ${base}$1/var/sadm/patch >/dev/null 2>&1",
"if [ $? = 0 ] ; then",
"  eval echo `gettext SUNW_INSTALL_SWLIB 'Removing patch $2 from the system.'`",
"  rm -fr ${base}$1/var/sadm/patch/$2",
"else",
"  if [ -h ${base}$1/var/sadm/patch ] ; then",
"	link=`ls -l ${base}$1/var/sadm/patch | sed -n 's,.* \\(/.*\\),\\1,p'`",
"	if [ \"$link\" != \"\" ] ; then",
"		mv ${base}$1/var/sadm/patch ${base}$1/var/sadm/patch.save.$$",
"		rm -fr ${base}$1/var/sadm/patch",
"		ln -s ${base}$1/${link} ${base}$1/var/sadm/patch",
"		ls -L ${base}$1/var/sadm/patch >/dev/null 2>&1",
"		if [ $? = 0 ] ; then",
"			eval echo `gettext SUNW_INSTALL_SWLIB 'Removing patch $2 from the system.'`",
"			rm -fr ${base}$1/var/sadm/patch/$2",
"		fi",
"		rm ${base}$1/var/sadm/patch",
"	fi",
"		mv ${base}$1/var/sadm/patch.save.$$ ${base}$1/var/sadm/patch",
"  fi",
"fi",
"}",
"# Execution start",
"base=$1",
"rbase=$1",
"if [ \"$base\" = \"/\" ] ; then",
"	base=\"\"",
"fi",
"timestamp=@TIMESTAMP@",
"restart=${base}@RESTART_PATH@",
"restartbkup=${base}@RESTART_PATH@.bkup",
"coalesce=${base}@CLEANUP_PATH@",
"failedpkgs=${base}@UPGRADE_FAILED_PKGS@",
"cleanup_needed=0",
"EXIT_CODE=0",
"resumecnt=0",
"savestamp=",
"restarting=no",
"TEXTDOMAIN=SUNW_INSTALL_SWLIB",
"export TEXTDOMAIN",
"if [ $# -eq 2 -a \"$2\" -eq \"restart\" ] ; then",
"	if [ -f $restart ] ; then",
"	    resumecnt=`grep LASTCMD $restart | sed 's/^.*=//'`",
"	    savestamp=`grep TIMESTAMP $restart | sed 's/^.*=//'`",
"	    restarting=yes",
"	fi",
"	if [ \"$savestamp\" != \"$timestamp\" ] ; then",
"	    if [ -f $restartbkup ] ; then",
"		resumecnt=`grep LASTCMD $restartbkup | sed 's/^.*=//'`",
"		savestamp=`grep TIMESTAMP $restartbkup | sed 's/^.*=//'`",
"		restarting=yes",
"	    fi",
"	fi",
"	if [ \"$savestamp\" != \"$timestamp\" ] ; then",
"		gettext SUNW_INSTALL_SWLIB 'Cannot restart upgrade.  No valid upgrade log.'; echo",
"	    exit 2",
"	fi",
"fi",
"if [ $restarting = yes ] ; then",
"	gettext SUNW_INSTALL_SWLIB 'Restarting upgrade:'; echo",
"else",
"	gettext SUNW_INSTALL_SWLIB 'Starting upgrade:'; echo",
"fi",
"file /usr/sbin/pkgadd >/dev/null 2>&1",
"if [ $? != 0 ] ; then",
"	gettext SUNW_INSTALL_SWLIB 'Fatal error:  cannot access the command /usr/sbin/pkgadd.'; echo",
"	exit 2",
"fi",
"if [ ! -d /var/spool/mqueue ]; then",
"	mkdir /var/spool/mqueue",
"fi",
"cat > /tmp/filter.$$ << EOF",
"/^WARNING: .*<no longer .*>/d",
"/^WARNING: quick verify of <.*>; wrong mod time./d",
"EOF",
""
};

char *init_swm_coalesce[] = {
"if [ $restarting = no ] ; then",
"	rm -f $coalesce",
"	rm -f ${base}/var/sadm/install_data/upgrade_cleanup",
"	rm -f $failedpkgs",
"	> $coalesce",
"	chmod 644 $coalesce",
"fi",
""
};

char *init_coalesce[] = {
"if [ $restarting = no ] ; then",
"	rm -f $coalesce",
"	rm -f $failedpkgs",
"	touch $coalesce",
"	chmod 644 $coalesce",
"(gettext SUNW_INSTALL_SWLIB 'This file contains a list of files on the upgraded system that may need\\nto be manually modified after the upgrade.  Typically, the files in this\\nlist are files that were modified since their original installation.'",
"echo \"\\n\"",
"gettext SUNW_INSTALL_SWLIB 'The following text explains the entries in this file:'",
"echo '\\n'",
"gettext SUNW_INSTALL_SWLIB 'ENTRY:'; echo",
"gettext SUNW_INSTALL_SWLIB '<file1>: existing file renamed to <file2>'",
"echo '\\n'",
"gettext SUNW_INSTALL_SWLIB 'EXPLANATION:'; echo",
"gettext SUNW_INSTALL_SWLIB 'The file with the name <file> was present on the system at the time of\\nthe upgrade.  It had been modified since its original installation,\\nso the upgrade program renamed it to <file2>.'",
"echo '\\n'",
"gettext SUNW_INSTALL_SWLIB 'RECOMMENDED ACTION:'; echo",
"gettext SUNW_INSTALL_SWLIB 'The user should examine the contents of the renamed file to determine\\nwhether the modifications made to the file should be made to the\\nnewly installed version of the file, which will have name <file1>\\nafter the upgrade completes.'",
"echo '\\n\\n'",
"gettext SUNW_INSTALL_SWLIB 'ENTRY'; echo",
"gettext SUNW_INSTALL_SWLIB '<file1>: existing file preserved, the new version was installed as <file2>'",
"echo '\\n'",
"gettext SUNW_INSTALL_SWLIB 'EXPLANATION'; echo",
"gettext SUNW_INSTALL_SWLIB 'The file with the name <file1> has been preserved.  The new version of\\nthe file was installed as <file2>.'",
"echo '\\n'",
"gettext SUNW_INSTALL_SWLIB 'RECOMMENDED ACTION:'; echo",
"gettext SUNW_INSTALL_SWLIB 'The user should examine <file2> to determine whether changes made to\\n<file2> should be incorporated into the preserved version of the file.'",
"echo '\\n\\n'",
"gettext SUNW_INSTALL_SWLIB 'ENTRY'; echo",
"gettext SUNW_INSTALL_SWLIB '<file>: had been deleted and has now been restored'",
"echo '\\n'",
"gettext SUNW_INSTALL_SWLIB 'EXPLANATION'; echo",
"gettext SUNW_INSTALL_SWLIB 'The file with the name <file1> had been deleted from the system since\\nits original installation.  The upgrade has installed the new version\\nof the file.'",
"echo '\\n'",
"gettext SUNW_INSTALL_SWLIB 'RECOMMENDED ACTION:'; echo",
"gettext SUNW_INSTALL_SWLIB 'The user should determine whether the new version of the file should be\\ndeleted also.'",
"echo '\\n\\n'",
"gettext SUNW_INSTALL_SWLIB 'ENTRY'; echo",
"gettext SUNW_INSTALL_SWLIB '<file>: file type was changed from <type1> to <type2>'",
"echo '\\n'",
"gettext SUNW_INSTALL_SWLIB 'EXPLANATION'; echo",
"gettext SUNW_INSTALL_SWLIB 'At its original installation, the file with the name <file> was of type\\n<type1>.  Later, it was replaced by a file of type <type2>.  For example,\\na symbolic link may have been replaced by a regular file.  In most cases,\\nthe upgrade will restore the file to its original type.'",
"echo '\\n'",
"gettext SUNW_INSTALL_SWLIB 'RECOMMENDED ACTION:'; echo",
"gettext SUNW_INSTALL_SWLIB 'The user should determine whether the new version of the file should also\\nbe replaced by a file of type <type2>.'",
"echo '\\n\\n'",
"gettext SUNW_INSTALL_SWLIB 'ENTRY'; echo",
"gettext SUNW_INSTALL_SWLIB '<file>: target of symbolic link was changed from <target1> to <target2>'",
"echo '\\n'",
"gettext SUNW_INSTALL_SWLIB 'EXPLANATION'; echo",
"gettext SUNW_INSTALL_SWLIB 'At the time of its original installation, <file> was a symbolic link\\nto <target1>.  It was changed to be a symbolic link to <target2>.\\nThe upgrade would have changed the link to point to its original\\ntarget.'",
"echo '\\n'",
"gettext SUNW_INSTALL_SWLIB 'RECOMMENDED ACTION:'; echo",
"gettext SUNW_INSTALL_SWLIB 'The user should determine whether the symbolic link should be changed\\nto point to the target to which it pointed before the upgrade.'",
"echo '\\n\\n'",
"gettext SUNW_INSTALL_SWLIB 'ENTRY'; echo",
"gettext SUNW_INSTALL_SWLIB '<file1>: target of hard link was changed from <file2>'",
"echo '\\n'",
"gettext SUNW_INSTALL_SWLIB 'EXPLANATION'; echo",
"gettext SUNW_INSTALL_SWLIB 'When originally installed, <file1> was a hard link to <file2>.  At the\\ntime of upgrade, it was no longer a hard link to <file2>.  The upgrade\\nwould have restored the original link.'",
"echo '\\n'",
"gettext SUNW_INSTALL_SWLIB 'RECOMMENDED ACTION:'; echo",
"gettext SUNW_INSTALL_SWLIB 'The user should determine whether the restored link should be changed\\nto what it was before the upgrade.'",
"echo '\\n\\n'",
"gettext SUNW_INSTALL_SWLIB 'Before the system is rebooted, all files listed below should be\\nreferenced relative to /a.'",
"echo '\\n'",
"echo '---------------------------------------------------------------------------'",
") >> $coalesce",
"fi",
""
};

char *add_swap_cmd[] = {
"/usr/sbin/swap -l | /bin/grep @MNTDEV@ >/dev/null 2>&1",
"if [ $? != 0 ] ; then",
"	/usr/sbin/swap -a @MNTDEV@",
"	if [ $? != 0 ]; then",
"		echo `gettext SUNW_INSTALL_SWLIB 'add swap failure'` $?",
"		exit 2",
"	fi",
"fi",
""
};

char *del_swap_cmd[] = {
"/usr/sbin/swap -d @MNTDEV@",
"if [ $? != 0 ]; then",
"	echo `gettext SUNW_INSTALL_SWLIB 'delete swap failure'` $?",
"fi",
""
};

char *mount_fs_cmd[] = {
"mount_filesys @FSTYPE@ @MNTDEV@ @MNTPNT@ @FSCKDEV@",
""
};

char *build_admin_file[] = {
"cat > /tmp/admin.@NAME@.$$ << EOF",
"mail=",
"instance=@INSTANCE@",
"partial=nocheck",
"runlevel=nocheck",
"idepend=nocheck",
"rdepend=nocheck",
"space=nocheck",
"setuid=nocheck",
"conflict=nocheck",
"action=nocheck",
"basedir=@BASEDIR@",
"EOF",
""
};

char *touch_upgrade[] = {
"touch @BASEDIR@/.UPGRADE",
""
};

char *do_pkgadd[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"	do_pkgadd @ROOT@ @PKG@ @SPOOL@ @ADMIN@",
"	logprogress @SEQ@",
"fi",
""
};

char *print_copyright[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"	echo",
"	gettext SUNW_INSTALL_SWLIB 'Installing new packages:'; echo; echo",
"	sleep 3",
"	cat @SPOOL@/@PKG@/install/copyright",
"	logprogress @SEQ@",
"fi",
""
};

char *print_rmpkg_msg[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"	gettext SUNW_INSTALL_SWLIB 'Removing obsolete packages and saving modified files:'; echo; echo",
"	logprogress @SEQ@",
"fi",
""
};

char *do_pkgrm[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"	do_pkgrm @ROOT@ @PKG@",
"	logprogress @SEQ@",
"fi",
""
};

char *do_pkgrm_f[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"	do_pkgrm_f @ROOT@ @PKG@",
"	logprogress @SEQ@",
"fi",
""
};

char *rename_file[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"	rename_file @DIR@ @FILE@ @VER@",
"	logprogress @SEQ@",
"fi",
""
};

char *do_rm_fr[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"	echo rm -fr ${base}@DIR@",
"	rm -fr ${base}@DIR@",
"	logprogress @SEQ@",
"fi",
""
};

char *remove_template[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"	rmtemplate=@DIR@",
"	eval echo `gettext SUNW_INSTALL_SWLIB 'Removing $rmtemplate:'`",
"	rm -fr ${base}@DIR@",
"	logprogress @SEQ@",
"fi",
""
};

char *move_template[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"	mv ${base}/@OLDDIR@ ${base}/export/root/templates/@NEWSVC@/@PKG@_@VER@_@ARCH@",
"	logprogress @SEQ@",
"fi",
""
};

char *do_rm[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"	echo rm -f ${base}@ROOT@@FILE@",
"	rm -f ${base}@ROOT@@FILE@",
"	logprogress @SEQ@",
"fi",
""
};

char *do_removef[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"	do_removef @ROOT@ @PKG@",
"	logprogress @SEQ@",
"fi",
""
};

char *spool_pkg[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"	pkg=@PKG@",
"	eval echo `gettext SUNW_INSTALL_SWLIB 'Transferring $pkg package'`",
"	mkdir ${base}@SPOOLDIR@",
"	(cd @MEDIA@; tar cf - @PKG@ | (cd ${base}@SPOOLDIR@; tar xf -) )",
"	logprogress @SEQ@",
"fi",
""
};

char *echo_INST_RELEASE[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"   if [ ! -h ${base}/@ROOT@/var/sadm/softinfo/INST_RELEASE ]; then",
"	rm -f ${base}/@ROOT@/var/sadm/softinfo/INST_RELEASE",
"   fi",
"   echo \"OS=@OS@\" > ${base}/@ROOT@@INST_REL_PATH@",
"   echo \"VERSION=@VERSION@\" >> ${base}/@ROOT@@INST_REL_PATH@",
"   echo \"REV=@REVISION@\" >> ${base}/@ROOT@@INST_REL_PATH@",
"   chmod 644 ${base}/@ROOT@@INST_REL_PATH@",
"	logprogress @SEQ@",
"fi",
""
};

char *echo_softinfo[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"    rm -f ${base}@SERVICE_PATH@@OS@_2.*",
"    rm -f ${base}/var/sadm/softinfo/@OS@_2.*",
"    echo \"OS=@OS@\" > ${base}@SERVICE_PATH@@OS@_@VERSION@",
"    echo \"VERSION=@VERSION@\" >> ${base}@SERVICE_PATH@@OS@_@VERSION@",
"    echo \"REV=@REVISION@\" >> ${base}@SERVICE_PATH@@OS@_@VERSION@",
"    chmod 644 ${base}@SERVICE_PATH@@OS@_@VERSION@",
"    logprogress @SEQ@",
"fi",
""
};

char *touch_reconfig[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"	touch ${base}/reconfigure",
"	logprogress @SEQ@",
"fi",
""
};

char *rm_tmp[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"	rm /tmp/*.$$",
"	logprogress @SEQ@",
"fi",
""
};

char *umount_cmd[] = {
"	umount @MNTDEV@",
""
};

char *rm_service[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"	rm -fr ${base}/export/@PRODVER@",
"	rm -fr ${base}/export/exec/@PRODVER@_*",
"	rm -fr ${base}/export/exec/kvm/@PRODVER@_*",
"	rm -fr ${base}/export/share/@PRODVER@",
"	rm -fr ${base}/export/root/templates/@PRODVER@",
"	rm -f ${base}/var/sadm/system/admin/services/@PRODVER@",
"	rm -f ${base}/var/sadm/softinfo/@PRODVER@",
"	rmdir ${base}/export/root/templates > /dev/null 2>&1",
"	rmdir ${base}/export/root > /dev/null 2>&1",
"	rmdir ${base}/export/share> /dev/null 2>&1",
"	rmdir ${base}/export/exec/kvm > /dev/null 2>&1",
"	rmdir ${base}/export/exec > /dev/null 2>&1",
"	logprogress @SEQ@",
"fi",
""
};

char *mv_whole_svc[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"	(cd ${base}/export",
"	rm -f ./@OLD@/usr_*",
"	rm -f ./@OLD@/usr.kvm_*",
"	if [ -d ./@OLD@ ] ; then",
"		mv ./@OLD@ ./@NEW@",
"	fi",
"	if [ -h ./share/@OLD@ -o -d ./share/@OLD@ ] ; then",
"		mv ./share/@OLD@ ./share/@NEW@",
"	fi",
"	if [ ! -d ./root ] ; then",
"		mkdir ./root",
"	fi",
"	if [ ! -d ./root/templates ] ; then",
"		mkdir ./root/templates",
"	fi",
"	if [ ! -d ./root/templates/@NEW@ ] ; then",
"		mkdir ./root/templates/@NEW@",
"	fi",
"	)",
"	logprogress @SEQ@",
"fi",
""
};

char *mv_isa_svc[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"	(cd ${base}/export",
"	if [ -d ./exec/@OLD@_@PARCH@.all ] ; then",
"		mv ./exec/@OLD@_@PARCH@.all ./exec/@NEW@_@PARCH@.all",
"	fi",
"	rm -fr ./@NEW@/usr_@PARCH@.all",
"	ln -s ../exec/@NEW@_@PARCH@.all ./@NEW@/usr_@PARCH@.all",
"	)",
"	logprogress @SEQ@",
"fi",
""
};

char *add_varsadm_usr[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"	(cd ${base}/export",
"	mkdir ./@SVC@",
"	mkdir ./@SVC@/var",
"	mkdir ./@SVC@/var/sadm",
"	if [ \"postKBI\" = \"@POST_KBI@\" ]; then",
"		mkdir ./@SVC@/var/sadm/system",
"		mkdir ./@SVC@/var/sadm/system/admin",
"		mkdir ./@SVC@/var/sadm/system/admin/services",
"		mkdir ./@SVC@/var/sadm/system/logs",
"		mkdir ./@SVC@/var/sadm/system/data",
"	else",
"		mkdir ./@SVC@/var/sadm/softinfo",
"	fi",
"	mkdir ./@SVC@/var/sadm/install_data",
"	if [ ! -d ./exec ] ; then",
"		mkdir ./exec",
"	fi",
"	if [ ! -d ./share ] ; then",
"		mkdir ./share",
"	fi",
"	if [ ! -d ./root ] ; then",
"		mkdir ./root",
"	fi",
"	if [ ! -d ./root/templates ] ; then",
"		mkdir ./root/templates",
"	fi",
"	if [ ! -d ./root/templates/@SVC@ ] ; then",
"		mkdir ./root/templates/@SVC@",
"	fi",
"	)",
"	logprogress @SEQ@",
"fi",
""
};

char *link_varsadm_usr[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"	(cd ${base}/export",
"	mkdir ./@SVC@",
"	mkdir ./@SVC@/var",
"	rm -fr ./@SVC@/var/sadm",
"	ln -s /var/sadm ./@SVC@/var/sadm",
"	if [ ! -d ./exec ] ; then",
"		mkdir ./exec",
"	fi",
"	if [ ! -d ./share ] ; then",
"		mkdir ./share",
"	fi",
"	if [ ! -d ./root ] ; then",
"		mkdir ./root",
"	fi",
"	if [ ! -d ./root/templates ] ; then",
"		mkdir ./root/templates",
"	fi",
"	if [ ! -d ./root/templates/@SVC@ ] ; then",
"		mkdir ./root/templates/@SVC@",
"	fi",
"	)",
"	logprogress @SEQ@",
"fi",
""
};

char *rm_kvm_svc[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"     rm -fr ${base}/export/exec/kvm/@OLD@_@ARCH@",
"     rmdir ${base}/export/exec/kvm > /dev/null 2>&1",
"     logprogress @SEQ@",
"fi",
""
};

char *mv_kvm_svc[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"	(cd ${base}/export",
"	if [ -d ./exec/kvm/@OLD@_@ARCH@ ] ; then",
"		mv ./exec/kvm/@OLD@_@ARCH@ ./exec/kvm/@NEW@_@ARCH@",
"	fi",
"	rm -fr ./@NEW@/usr.kvm_@ARCH@",
"	ln -s ../exec/kvm/@NEW@_@ARCH@ ./@NEW@/usr.kvm_@ARCH@",
"	)",
"	logprogress @SEQ@",
"fi",
""
};

char *add_kvm_svc[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"	(cd ${base}/export",
"	if [ ! -d ./exec/kvm ] ; then",
"		mkdir ./exec/kvm",
"	fi",
"	if [ ! -d ./exec/kvm/@NEW@_@ARCH@ ] ; then",
"		mkdir ./exec/kvm/@NEW@_@ARCH@",
"	fi",
"	rm -fr ./@NEW@/usr.kvm_@ARCH@",
"	ln -s ../exec/kvm/@NEW@_@ARCH@ ./@NEW@/usr.kvm_@ARCH@",
"	)",
"	logprogress @SEQ@",
"fi",
""
};

char *link_kvm_svc[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"	(cd ${base}/export",
"	if [ ! -d ./exec/kvm ] ; then",
"		mkdir ./exec/kvm",
"	fi",
"	if [ ! -d ./exec/kvm/@NEW@_@ARCH@ ] ; then",
"		mkdir ./exec/kvm/@NEW@_@ARCH@",
"	fi",
"	if [ ! -d ./exec/kvm/@NEW@_@ARCH@/usr ] ; then",
"		mkdir ./exec/kvm/@NEW@_@ARCH@/usr",
"	fi",
"	rm -fr ./exec/kvm/@NEW@_@ARCH@/usr/kvm",
"	ln -s /usr/kvm ./exec/kvm/@NEW@_@ARCH@/usr/kvm",
"	rm -fr ./@NEW@/usr.kvm_@ARCH@",
"	ln -s ../exec/kvm/@NEW@_@ARCH@ ./@NEW@/usr.kvm_@ARCH@",
"	)",
"	logprogress @SEQ@",
"fi",
""
};

char *move_files_in_contents[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
" if [ -f ${base}/@ROOT@/var/sadm/install/sav.contents ] ; then",
"   mv ${base}/@ROOT@/var/sadm/install/sav.contents ${base}/@ROOT@/var/sadm/install/contents",
" fi",
" cat ${base}/@ROOT@/var/sadm/install/contents | \\",
" sed \\",
" -e 's,^/export/exec/kvm/@OLD@_,/export/exec/kvm/@NEW@_,' \\",
" -e 's,^/export/exec/@OLD@_,/export/exec/@NEW@_,' \\",
" -e '/^#/d'    | sort > ${base}/@ROOT@/var/sadm/install/sav.contents",
" mv $base/@ROOT@/var/sadm/install/sav.contents $base/@ROOT@/var/sadm/install/contents",
" (cd ${base}/@ROOT@/var/sadm/pkg",
" for i in *; do",
"   if [ \"$i\" = \"*\" ] ; then",
"	break",
"   fi",
"   if [ -f $i/pkginfo.upgsav ] ; then",
"      mv $i/pkginfo.upgsav $i/pkginfo",
"   fi",
"   grep '^BASEDIR=/export/exec/' $i/pkginfo >/dev/null 2>&1",
"   if [ $? = 0 ] ; then",
"      sed -e 's,^BASEDIR=/export/exec/@OLD@_,BASEDIR=/export/exec/@NEW@_,' \\",
"         -e 's,^BASEDIR=/export/exec/kvm/@OLD@_,BASEDIR=/export/exec/kvm/@NEW@_,'\\",
"        -e 's,^CLIENT_BASEDIR=/export/exec/@OLD@_,CLIENT_BASEDIR=/export/exec/@NEW@_,' \\",
"	 -e 's,^CLIENT_BASEDIR=/export/exec/kvm/@OLD@_,CLIENT_BASEDIR=/export/exec/kvm/@NEW@_,' $i/pkginfo > \\",
"      $i/pkginfo.upgsav",
"      mv $i/pkginfo.upgsav $i/pkginfo",
"   fi",
" done)",
"	logprogress @SEQ@",
"fi",
""
};

char *start_softinfo[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"rm -f ${base}@SERVICE_PATH@@OS@_@VER@",
"touch ${base}@SERVICE_PATH@@OS@_@VER@",
"chmod 644 ${base}@SERVICE_PATH@@OS@_@VER@",
"cat >> ${base}@SERVICE_PATH@@OS@_@VER@ << EOF",
"FORMAT_VERSION=2",
"OS=@OS@",
"VERSION=@VER@",
"REV=@REVISION@",
""
};

char *usr_softinfo[] = {
"USR_PATH=@ARCH@.all:/export/exec/@OS@_@VER@_@ARCH@.all/usr",
""
};

char *kvm_softinfo[] = {
"KVM_PATH=@ARCH@:/export/exec/kvm/@OS@_@VER@_@ARCH@/usr/kvm",
""
};

char *root_softinfo[] = {
"SPOOLED_ROOT=@ARCH@:@PATH@",
"ROOT=@ARCH@:@PKG@,@SIZE@,@VERSION@",
""
};

char *platgrp_softinfo[] = {
"PLATFORM_GROUP=@ISA@:@PLATGRP@",
""
};

char *platmember_softinfo[] = {
"PLATFORM_MEMBER=@PLAT@",
""
};

char *locale_softinfo[] = {
"LOCALE=@ARCH@:@LOC@",
""
};

char *end_softinfo[] = {
"EOF",
"	logprogress @SEQ@",
"fi",
""
};

char *rm_softinfo[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"	rm -f ${base}/var/sadm/softinfo/@OLD@",
"	rm -f ${base}@SERVICE_PATH@@OLD@",
"	logprogress @SEQ@",
"fi",
""
};

char *start_platform[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"rm -f ${base}@ROOT@/var/sadm/system/admin/.platform",
"touch ${base}@ROOT@/var/sadm/system/admin/.platform",
"chmod 644 ${base}@ROOT@/var/sadm/system/admin/.platform",
"cat >> ${base}@ROOT@/var/sadm/system/admin/.platform << EOF",
""
};

char *generic[] = {
"@LINE@",
""
};

char *rm_svc_dfstab[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"    if [ -f ${base}/etc/dfs/dfstab ] ; then",
"	cat ${base}/etc/dfs/dfstab | \\",
"	   sed -e '/export\\/exec\\/@NAME@_[^_\\.][^_\\.]*\\.all\\/usr$/d' \\",
"	       -e '/export\\/exec\\/kvm\\/@NAME@_[^_\\.][^_\\.]*\\.[^\\/]*\\/usr\\/kvm/d' \\",
"	       -e '/export\\/share\\/@NAME@[ 	]*$/d' > /tmp/tmp.$$",
"	   mv /tmp/tmp.$$ ${base}/etc/dfs/dfstab",
"    fi",
"    logprogress @SEQ@",
"fi",
""
};

char *add_usr_svc_dfstab[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"    echo share -F nfs -o ro /export/exec/@NAME@_@ISA@.all/usr >> ${base}/etc/dfs/dfstab",
"    logprogress @SEQ@",
"fi",
""
};

char *sed_dfstab_usr[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"    if [ -f ${base}/etc/dfs/dfstab ] ; then",
"	cat ${base}/etc/dfs/dfstab | \\",
"	sed -e 's,/@OLD@_@ISA@.all/usr,/@NEW@_@ISA@.all/usr,' > /tmp/tmp.$$",
"	mv /tmp/tmp.$$ ${base}/etc/dfs/dfstab",
"    fi",
"    logprogress @SEQ@",
"fi",
""
};

char *add_kvm_svc_dfstab[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"    echo share -F nfs -o ro /export/exec/kvm/@NAME@_@ARCH@/usr/kvm >> \\",
"        ${base}/etc/dfs/dfstab",
"    logprogress @SEQ@",
"fi",
""
};

char *share_usr_svc_dfstab[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"    share -F nfs -o ro /export/exec/@NAME@_@PARCH@.all/usr > /dev/null 2>&1",
"    logprogress @SEQ@",
"fi",
""
};

char *share_kvm_svc_dfstab[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"    share -F nfs -o ro /export/exec/kvm/@NAME@_@ARCH@/usr/kvm > /dev/null 2>&1",
"    logprogress @SEQ@",
"fi",
""
};

/*
 * DVB 5/14/93
 * Make both types of directories, since we are not yet sure what the arch
 * will be. This doesn't really hurt anything.
 */
char *init_inetboot_dir[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"       if [ ! -d ${base}/tftpboot ] ; then",
"		mkdir ${base}/tftpboot",
"		ln -s . ${base}/tftpboot/tftpboot",
"	fi",
"       if [ ! -d ${base}/rplboot ] ; then",
"		mkdir ${base}/rplboot",
"	fi",
"    logprogress @SEQ@",
"fi",
""
};

char *rm_inetboot[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"	rm -f ${base}/tftpboot/inetboot.*.@SVCPROD@[._]@SVCVER@",
"	rm -f ${base}/rplboot/inetboot.*.@SVCPROD@[._]@SVCVER@",
"    logprogress @SEQ@",
"fi",
""
};

char *cp_shared_inetboot[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"   inetboot=${base}/usr/platform/@KARCH@/lib/fs/nfs/inetboot",
"   gluedir=${base}/usr/platform/@KARCH@/lib/fs/nfs",
"   if [ ! -f $inetboot ] ; then",
"	inetboot=${base}/usr/lib/fs/nfs/inetboot",
"	gluedir=${base}/usr/lib/fs/nfs/drv.@KARCH@",
"   fi",
"   logprogress @SEQ@",
"fi",
""
};

char *cp_svc_inetboot[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"   inetboot=${base}/export/exec/@SVCPROD@_@SVCVER@_@ARCH@.all/usr/platform/@KARCH@/lib/fs/nfs/inetboot",
"   gluedir=${base}/export/exec/@SVCPROD@_@SVCVER@_@ARCH@.all/usr/platform/@KARCH@/lib/fs/nfs",
"   if [ ! -f $inetboot ] ; then",
"	inetboot=${base}/export/exec/@SVCPROD@_@SVCVER@_@ARCH@.all/usr/lib/fs/nfs/inetboot",
"	gluedir=${base}/export/exec/@SVCPROD@_@SVCVER@_@ARCH@.all/usr/lib/fs/nfs/drv.@KARCH@",
"   fi",
"    logprogress @SEQ@",
"fi",
""
};

char *cp_inetboot[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"  do_install_inetboot @BOOTDIR@ @KARCH@ @SVCPROD@ @SVCVER@",
"  logprogress @SEQ@",
"fi",
""
};

char *sed_vfstab[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"    cat ${base}/@CLIENTROOT@/etc/vfstab | \\",
"    sed -e 's,/@OLD@_@ARCH@.all,/@NEW@_@ARCH@.all,' \\",
"        -e 's,/@OLD@_@ARCH@.@KARCH@,/@NEW@_@ARCH@.@KARCH@,' \\",
"        -e 's,/@OLD@[ 	],/@NEW@	,' > /tmp/tmp.$$",
"    mv /tmp/tmp.$$ ${base}/@CLIENTROOT@/etc/vfstab",
"    logprogress @SEQ@",
"fi",
""
};

char *sed_vfstab_rm_kvm[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"    cat ${base}/@CLIENTROOT@/etc/vfstab | \\",
"    sed -e '/\\/usr\\/kvm/d' > /tmp/tmp.$$",
"    mv /tmp/tmp.$$ ${base}/@CLIENTROOT@/etc/vfstab",
"    logprogress @SEQ@",
"fi",
""
};

char *sed_dataless_vfstab[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"    cat ${base}/etc/vfstab | \\",
"    sed -e 's,/@OLD@_@ARCH@.all,/@NEW@_@ARCH@.all,' \\",
"        -e 's,/@OLD@_@ARCH@.@KARCH@,/@NEW@_@ARCH@.@KARCH@,' \\",
"        -e 's,/@OLD@[ 	],/@NEW@	,' > /tmp/tmp.$$",
"    mv /tmp/tmp.$$ ${base}/etc/vfstab",
"    logprogress @SEQ@",
"fi",
""
};

char *touch_client_reconfigure[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"	touch ${base}/@CLIENTROOT@/reconfigure",
"    logprogress @SEQ@",
"fi",
""
};

char *upgrade_client_inetboot[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
" (cd ${base}/@BOOTDIR@;",
" for i in @SEARCHFILE@ @SEARCHFILE@.*; do",
"	[ ! -h $i ] && continue;",
"	rm -f $i",
"	ln -s inetboot.@KARCH@.@SVCPROD@_@SVCVER@ $i",
" done)",
" logprogress @SEQ@",
"fi",
""
};

char *remove_restart_files[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"	rm -f $restart $restartbkup",
"fi",
""
};

char *exit_ok[] = {
"exit $EXIT_CODE",
""
};

char *remove_coalesce[] = {
"	rm -f $coalesce",
""
};

char *print_cleanup_msg[] = {
"echo",
"gettext SUNW_INSTALL_SWLIB 'The messages printed to the screen by this upgrade have been saved to:'; echo '\\n'",
"echo \"	${base}@UPGRADE_LOG@\\n\"",
"gettext  SUNW_INSTALL_SWLIB 'After this system is rebooted, the upgrade log can be found in the file:'; echo '\\n'",
"echo '	@UPGRADE_LOG@\\n\\n'",
"if [ -f $failedpkgs ]; then",
"	gettext SUWN_INSTALL_SWLIB 'The following packages failed to install correctly'",
"	echo '\\n\\n'",
"	cat $failedpkgs",
"	chmod 444 $failedpkgs",
"	EXIT_CODE=2",
"fi",
"if [ $cleanup_needed = 1 ] ; then",
"	gettext SUNW_INSTALL_SWLIB 'Please examine the file:'; echo '\\n'",
"	echo  \"	${base}@UPGRADE_CLEANUP@\\n\"",
"	gettext SUNW_INSTALL_SWLIB 'It contains a list of actions that may need to be performed to complete\\nthe upgrade.  After this system is rebooted, this file can be found at:'; echo '\\n'",
"	echo  '	@UPGRADE_CLEANUP@\\n'",
"	gettext SUNW_INSTALL_SWLIB 'After performing any necessary cleanup actions, the system should\\nbe rebooted.'; echo",
"	rm -f ${base}/var/sadm/install_data/upgrade_cleanup",
"	ln -s ../system/data/upgrade_cleanup ${base}/var/sadm/install_data",
"else",
"	rm -f $coalesce",
"	gettext SUNW_INSTALL_SWLIB 'Please reboot the system.'; echo",
"fi",
""
};

char *remove_patch[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"	remove_patch @ROOT@ @PATCHID@",
"	logprogress @SEQ@",
"fi",
""
};

char *rm_template_dir[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"	rm -fr ${base}/export/root/templates/@SVC@",
"	logprogress @SEQ@",
"fi",
""
};

char *write_CLUSTER[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"	rm -f ${base}/@ROOT@/var/sadm/install_data/CLUSTER",
"	rm -f ${base}/@ROOT@@CLUSTER_PATH@",
"	echo CLUSTER=@CLUSTER@ > ${base}/@ROOT@@CLUSTER_PATH@",
"	logprogress @SEQ@",
"fi",
""
};

char *write_clustertoc[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"	rm -f ${base}/@ROOT@/var/sadm/install_data/.clustertoc",
"	rm -f ${base}/@ROOT@@CLUSTERTOC_PATH@",
"	cp @TOC@ ${base}/@ROOT@@CLUSTERTOC_PATH@",
"	logprogress @SEQ@",
"fi",
""
};

char *start_rmlist[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"cat > /tmp/rmlist.$$ << EOF",
""
};

char *addto_rmlist[] = {
"@FILE@",
""
};

char *end_rmlist[] = {
"EOF",
"	logprogress @SEQ@",
"fi",
""
};

char *log_file_diff[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"	log_file_diff @ERR@ @FILE@ @ARG3@ @ARG4@",
"	logprogress @SEQ@",
"fi",
""
};

char *copy_ppc_vof[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"	vof=/platform/`uname -i`/openfirmware.x41",
"	if [ -f /tmp/root/$vof ] ; then",
"		rm -f $base/$vof",
"		echo cp /tmp/root/$vof $base/$vof",
"		cp /tmp/root/$vof $base/$vof",
"	fi",
"	logprogress @SEQ@",
"fi",
""
};

char *gen_installboot_cmd[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"	(cd $rbase",
"	isa=`uname -p`",
"	uname -i >/dev/null 2>&1",
"	if [ $? = 0 ] ; then",
"	    platform=`uname -i`",
"	    bootblk=$base/usr/platform/$platform/lib/fs/ufs/bootblk",
"	    if [ ! -f $bootblk ] ; then",
"		bootblk=$base/usr/platform/`uname -m`/lib/fs/ufs/bootblk",
"		if [ ! -f $bootblk ] ; then",
"		    bootblk=$base/usr/lib/fs/ufs/bootblk",
"		fi",
"	    fi",
"	    # This next bit is for dataless clients",
"	    if [ ! -f $bootblk ] ; then",
"	    	bootblk=/usr/platform/$platform/lib/fs/ufs/bootblk",
"		if [ ! -f $bootblk ] ; then",
"		    bootblk=/usr/platform/`uname -m`/lib/fs/ufs/bootblk",
"		    if [ ! -f $bootblk ] ; then",
"			bootblk=/usr/lib/fs/ufs/bootblk",
"		    fi",
"		fi",
"	    fi",
"	    if [ \"$isa\" = \"i386\" ] ; then",
"	        pboot=$base/usr/platform/$platform/lib/fs/ufs/pboot",
"	        if [ ! -f $pboot ] ; then",
"		    pboot=$base/usr/platform/`uname -m`/lib/fs/ufs/pboot",
"		    if [ ! -f $pboot ] ; then",
"		        pboot=$base/usr/lib/fs/ufs/pboot",
"		    fi",
"	        fi",
"		# This next bit is for dataless clients",
"	        if [ ! -f $pboot ] ; then",
"		    pboot=/usr/platform/$platform/lib/fs/ufs/pboot",
"		    if [ ! -f $pboot ] ; then",
"			pboot=/usr/platform/`uname -m`/lib/fs/ufs/pboot",
"			if [ ! -f $pboot ] ; then",
"			    pboot=/usr/lib/fs/ufs/pboot",
"			fi",
"		    fi",
"	        fi",
"	    fi",
"	else",
"	    bootblk=$base/usr/lib/fs/ufs/bootblk",
"	    if [ ! -f $bootblk ]; then",
"		bootblk=/usr/lib/fs/ufs/bootblk",
"	    fi",
"	    pboot=$base/usr/lib/fs/ufs/pboot",
"	    if [ ! -f pboot ]; then",
"		pboot=/usr/lib/fs/ufs/pboot",
"	    fi",
"	fi",
"	if [ \"$isa\" = \"sparc\" ] ; then",
"	    echo /usr/sbin/installboot $bootblk @RAWROOT@",
"	    /usr/sbin/installboot $bootblk @RAWROOT@",
"	elif [ \"$isa\" = \"i386\" ] ; then",
"	    echo /usr/sbin/installboot $pboot $bootblk @RAWROOTS2@",
"	    /usr/sbin/installboot $pboot $bootblk @RAWROOTS2@",
"	elif [ \"$isa\" = \"ppc\" ] ; then",
"	  vof=$base/platform/`uname -i`/openfirmware.x41",
"	  if [ -f $vof ] ; then",
"	    echo /usr/sbin/installboot -f $vof $bootblk @RAWROOTS2@",
"	    /usr/sbin/installboot -f $vof $bootblk @RAWROOTS2@",
"	  else",
"	    echo /usr/sbin/installboot $bootblk @RAWROOTS2@",
"	    /usr/sbin/installboot $bootblk @RAWROOTS2@",
"	  fi",
"	else",
"	    echo Unknown instruction set architecture",
"	fi)",
"	logprogress @SEQ@",
"fi",
""
};

char *start_perm_restores[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
""
};

char *chmod_file[] = {
"/bin/ls -d $base@DIR@@FILE@ >/dev/null 2>&1",
"if [ $? = 0 ] ; then chmod @MODE@ $base@DIR@@FILE@; fi",
""
};

char *chown_file[] = {
"/bin/ls -d $base@DIR@@FILE@ >/dev/null 2>&1",
"if [ $? = 0 ] ; then chown @OWNER@ $base@DIR@@FILE@; fi",
""
};

char *chgrp_file[] = {
"/bin/ls -d $base@DIR@@FILE@ >/dev/null 2>&1",
"if [ $? = 0 ] ; then chgrp @GROUP@ $base@DIR@@FILE@; fi",
""
};

char *end_perm_restores[] = {
"	logprogress @SEQ@",
"fi",
""
};

char *link_usr_svc[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"	(cd ${base}/export",
"	if [ ! -d ./exec/@SVC@_@ISA@.all ] ; then",
"		mkdir ./exec/@SVC@_@ISA@.all",
"	fi",
"	rm -fr ./exec/@SVC@_@ISA@.all/usr",
"	rm -fr ./@SVC@/usr_@ISA@.all",
"	ln -s /usr ./exec/@SVC@_@ISA@.all/usr",
"	ln -s ../exec/@SVC@_@ISA@.all ./@SVC@/usr_@ISA@.all",
"	if [ \\( ! -d ./share/@SVC@ \\) -a \\( ! -h ./share/@SVC@ \\) ] ; then",
"		rm -fr ./share/@SVC@",
"		ln -s /export/exec/@SVC@_@ISA@.all/usr/share ./share/@SVC@",
"	fi",
"	)",
"	logprogress @SEQ@",
"fi",
""
};

char *add_usr_svc[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"	(cd ${base}/export",
"	if [ ! -d ./exec/@SVC@_@ISA@.all ] ; then",
"		mkdir ./exec/@SVC@_@ISA@.all",
"	fi",
"	rm -fr ./@SVC@/usr_@ISA@.all",
"	ln -s ../exec/@SVC@_@ISA@.all ./@SVC@/usr_@ISA@.all",
"	if [ \\( ! -d ./share/@SVC@ \\) -a \\( ! -h ./share/@SVC@ \\) ] ; then",
"		rm -fr ./share/@SVC@",
"		ln -s /export/exec/@SVC@_@ISA@.all/usr/share ./share/@SVC@",
"	fi",
"	)",
"	logprogress @SEQ@",
"fi",
""
};

char *mk_varsadm_dirs[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"	mkdir -m 755 -p $base/@CLIENTROOT@/var/sadm/system/admin/services >/dev/null 2>&1",
"	mkdir -m 755 -p $base/@CLIENTROOT@/var/sadm/system/logs >/dev/null 2>&1",
"	mkdir -m 755 -p $base/@CLIENTROOT@/var/sadm/system/data >/dev/null 2>&1",
"	chgrp -R sys $base/@CLIENTROOT@/var/sadm/system",
"	logprogress @SEQ@",
"fi",
""
};

char *mv_varsadm_files[] = {
"if [ @SEQ@ -gt $resumecnt ] ; then",
"	(cd ${base}/@CLIENTROOT@/var/sadm",
"	[ ! -h install_data/install_log ] && mv install_data/install_log system/logs",
"	if [ -f system/logs/install_log ] ; then",
"		rm -f install_data/install_log",
"		ln -s ../system/logs/install_log install_data/install_log",
"	fi",
"	[ -f begin_log ] && mv begin_log system/logs/begin_log.orig",
"	[ -f finish_log ] && mv finish_log system/logs/finish_log.orig",
"	[ -f install_data/upgrade_log ] && rm -f install_data/upgrade_log",
"	[ ! -h install_data/upgrade_log ] && ln -s ../system/logs/upgrade_log install_data",
"	[ ! -h install_data/upgrade_script ] && rm -f install_data/upgrade_script",
"	[ ! -h install_data/upgrade_script ] && ln -s ../system/admin/upgrade_script install_data",
"	[ -f install_data/upgrade_space_required ] && rm -f install_data/upgrade_space_required",
"	[ -f system/data/upgrade_space_required ] && ln -s ../system/data/upgrade_space_required install_data",
"	[ -f install_data/upgrade_cleanup ] && rm -f install_data/upgrade_cleanup",
"	)",
"	logprogress @SEQ@",
"fi",
""
};
