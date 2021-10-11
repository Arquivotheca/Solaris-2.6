#ifndef lint
#pragma ident "@(#)svc_updateconfig.c 1.25 96/08/08 SMI"
#endif

/*
 * Copyright (c) 1995-1996 by Sun Microsystems, Inc. All rights reserved.
 */
/*
 * Module:	svc_updateconfig.c
 * Group:	libspmisvc
 * Description: Routines to update the configuration of file on
 *		an installed system.
 */

#include <ctype.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <device_info.h>
#include <sys/mman.h>
#include <sys/mntent.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "spmisvc_lib.h"
#include "svc_strings.h"
#include "spmicommon_api.h"
#include "spmisoft_api.h"

/* constants */

#define	TMPVFSTAB		"/tmp/vfstab"
#define	TMPVFSTABUNSELECT	"/tmp/vfstab.unselected"

/* internal prototypes */

int	_setup_bootblock(void);
int	_setup_devices(void);
int	_setup_etc_hosts(Dfs *);
int	_setup_i386_bootrc(Disk_t *, int);
int	_setup_i386_bootenv(Disk_t *, int);
int	_setup_install_log(void);
int	_setup_tmp_root(TransList **);
int	_setup_vfstab(OpType, Vfsent **);
int	_setup_vfstab_unselect(void);
int	SystemConfigProm(void);

/* private prototypes */

static char * 	get_bootpath(char *, int);

/* globals */

static char	cmd[MAXNAMELEN];

/* ---------------------- internal functions ----------------------- */

/*
 * Function:	_setup_bootblock
 * Description:	Install boot blocks on boot disk.
 * Scope:	internal
 * Parameters:	none
 * Return:	ERROR	- boot block installation failed
 *		NOERR	- boot blocks installed successfully
 */
int
_setup_bootblock(void)
{
	Mntpnt_t info;
	Disk_t	*bdp;
	char	*rootdir = get_rootdir();
	char	*bootblk_path;
	char	*pboot_path;
	char	*vof_path;

	if (get_machinetype() == MT_CCLIENT)
		return (NOERR);

	write_status(LOGSCR, LEVEL0, MSG0_BOOT_INFO_INSTALL);

	/* there should only be one "/" in the disk object list */
	if (find_mnt_pnt(NULL, NULL, ROOT, &info, CFG_CURRENT) == 0) {
		write_notice(ERRMSG, MSG0_ROOT_UNSELECTED);
		return (ERROR);
	}

	bdp = info.dp;
	write_status(LOGSCR, LEVEL1|LISTITEM,
		MSG1_BOOT_BLOCKS_INSTALL,
		IsIsa("sparc") ?
			make_slice_name(disk_name(bdp), info.slice) :
			disk_name(bdp));


	/*
	 * if you are not running in execution simulation, find the new
	 * bootblocks which were just installed on the system and install
	 * them on the boot disk
	 */
	if (!GetSimulation(SIM_EXECUTE) && !GetSimulation(SIM_SYSDISK)) {
		if ((bootblk_path = gen_bootblk_path(rootdir)) == NULL &&
				(DIRECT_INSTALL || (bootblk_path =
					gen_bootblk_path("/")) == NULL)) {
			write_notice(ERRMSG, MSG0_BOOT_BLOCK_NOTEXIST);
			return (ERROR);
		}

		if (streq(get_default_inst(), "i386")) {
			/*
			 * the pboot file path name is a required argument for
			 * i386 installboot calls; if you can't find it, you're
			 * in trouble at this point
			 */
			if ((pboot_path = gen_pboot_path(rootdir)) == NULL &&
					(DIRECT_INSTALL || (pboot_path =
						gen_pboot_path("/")) == NULL)) {
				write_notice(ERRMSG, MSG0_PBOOT_NOTEXIST);
				return (ERROR);
			}

			(void) sprintf(cmd,
				"/usr/sbin/installboot %s %s %s",
				pboot_path,
				bootblk_path,
				make_char_device(disk_name(bdp), ALL_SLICE));
		} else if (streq(get_default_inst(), "ppc")) {
			/*
			 * only pass in an openfirmware file argument if one
			 * exists; it is perfectly legal for the system not to
			 * have one (i.e. openfirmware is implemented in
			 * hardware on this system); it is up to prepatory
			 * software to ensure that the file is present on
			 * systems which require it to be installed
			 */
			if ((vof_path = gen_openfirmware_path(rootdir))
					    == NULL &&
					(DIRECT_INSTALL || (vof_path =
					    gen_openfirmware_path("/"))
					    == NULL)) {
				(void) sprintf(cmd,
					"/usr/sbin/installboot %s %s",
					bootblk_path,
					make_char_device(disk_name(bdp),
					    ALL_SLICE));
			} else {
				(void) sprintf(cmd,
					"/usr/sbin/installboot -f %s %s %s",
					vof_path,
					bootblk_path,
					make_char_device(disk_name(bdp),
					    ALL_SLICE));
			}
		} else {
			(void) sprintf(cmd,
				"/usr/sbin/installboot %s %s",
				bootblk_path,
				make_char_device(disk_name(bdp), info.slice));
		}

		if (system(cmd) != 0) {
			write_notice(ERRMSG, MSG0_INSTALLBOOT_FAILED);
			return (ERROR);
		}
	}

	/*
	 * configure the /etc/bootrc file (i386 platforms only)
	 */
	if (_setup_i386_bootenv(bdp, info.slice) != NOERR)
		return (ERROR);

	return (NOERR);
}

/*
 * Function:	_setup_devices
 * Description:	Configure the /dev and /devices directory by copying over from
 *		the running system /dev and /devices directory. Install the
 *		/reconfigure file so that an automatic "boot -r" will occur.
 *		Note that a minimum set of devices must be present in the
 *		/dev and /devices tree in order for the system to even boot
 *		to the level of reconfiguration.
 * Scope:	internal
 * Parameters:	none
 * Return:	NOERR	- setup successful
 *		ERROR	- setup failed
 */
int
_setup_devices(void)
{
	char	path[MAXNAMELEN] = "";
	int	fd;

	/* only set up devices for indirect installs */
	if (DIRECT_INSTALL)
		return (NOERR);

	write_status(LOGSCR, LEVEL0, MSG0_DEVICES_CUSTOMIZE);
	write_status(LOGSCR, LEVEL1|LISTITEM,
		MSG0_DEVICES_LOGICAL);

	if (!GetSimulation(SIM_EXECUTE)) {
		(void) sprintf(path, "%s/dev", get_rootdir());

		if (access("/dev", X_OK) != 0) {
			write_notice(ERRMSG,
				MSG1_DIR_ACCESS_FAILED,
				"/dev");
			return (ERROR);
		}

		(void) sprintf(cmd, "cd /dev; find . -depth -print |  \
			cpio -pudm %s >/dev/null 2>&1", path);

		if (system(cmd) != 0) {
			write_notice(ERRMSG,
				MSG1_DEV_INSTALL_FAILED,
				"/dev");
			return (ERROR);
		}
	}

	write_status(LOGSCR, LEVEL1|LISTITEM, MSG0_DEVICES_PHYSICAL);

	if (!GetSimulation(SIM_EXECUTE)) {
		(void) sprintf(path, "%s/devices", get_rootdir());

		if (access("/devices", X_OK) != 0) {
			write_notice(ERRMSG,
				MSG1_DIR_ACCESS_FAILED,
				"/devices");
			return (ERROR);
		}

		(void) sprintf(cmd, "cd /devices; find . -depth -print | \
			cpio -pudm %s >/dev/null 2>&1", path);

		if (system(cmd) != 0) {
			write_notice(ERRMSG,
				MSG1_DEV_INSTALL_FAILED,
				"/devices");
			return (ERROR);
		}

		(void) sprintf(path, "%s/reconfigure", get_rootdir());

		if ((fd = creat(path, 0444)) < 0)
			write_notice(WARNMSG, MSG0_REBOOT_MESSAGE);
		else
			(void) close(fd);
	}

	return (NOERR);
}

/*
 * Function:	_setup_etc_hosts
 * Description:	Create the system 'etc/hosts' file using the remote file systems
 *		specified by the user during installation configuration.
 * Scope:	internal
 * Parameters:	cfs	- pointer to remote file system list
 * Return:	NOERR	- /etc/hosts file created successfully
 *		ERROR	- attempt to create /etc/hosts file failed
 */
int
_setup_etc_hosts(Dfs *cfs)
{
	FILE 		*fp;
	Dfs 		*p1, *p2;
	int		match;
	struct hostent	*ent;
	char		*cp = NULL;

	write_status(LOGSCR, LEVEL1|LISTITEM, MSG0_HOST_ADDRESS);

	if (GetSimulation(SIM_EXECUTE))
		return (NOERR);

	if ((fp = fopen("/etc/hosts", "a")) == NULL) {
		write_notice(ERRMSG,
			MSG_OPEN_FAILED,
			"/etc/hosts");
		return (ERROR);
	}

	for (p1 = cfs; p1; p1 = p1->c_next) {
		for (match = 0, p2 = cfs; p2 != p1; p2 = p2->c_next) {
			if (strcmp(p1->c_hostname, p2->c_hostname) == 0) {
				match = 1;
				break;
			}
		}
		if (match)
			continue;

		if (strstr(p1->c_mnt_pt, USR) || p1->c_ip_addr[0]) {
			if (p1->c_ip_addr[0] == '\0' &&
					(ent = gethostbyname(
						p1->c_hostname)) != NULL)
			    cp = inet_ntoa(*((struct in_addr *) ent->h_addr));
			if (p1->c_ip_addr[0] != '\0') {
				(void) fprintf(fp,
					"%s\t%s\n", p1->c_ip_addr,
					p1->c_hostname);
			} else if (cp) {
				(void) fprintf(fp,
					"%s\t%s\n", cp, p1->c_hostname);
			}
		}
	}

	(void) fclose(fp);
	return (NOERR);
}



/*
 * Function:	_setup_i386_bootenv
 * Description:	/platform/i86pc/boot/solaris/bootenv.rc file is used by
 *		the initial boot loader to determine the location of
 *		solaris, and to hold other configuration variables.
 * Scope:	internal
 * Parameters:	bdp	valid pointer to boot disk object
 *		slice	slice index for "/" slice
 * Return:	NOERR	all work relating to bootenv completed successfully
 *		ERROR	required work in configuring bootrc failed
 */
int
_setup_i386_bootenv(Disk_t *bdp, int slice)
{
	char	efile[MAXPATHLEN];
	char	tfile[MAXPATHLEN];
	char	edit[MAXNAMELEN];
	char *	lp;
	FILE *	fp;

	/* if this is not an i386 system there is no work to do */
	if (!IsIsa("i386"))
		return (NOERR);

	/*
	 * THE PATH SHOULD BE WRITTEN WITH A STANDARD SOFTWARE
	 * LIBRARY FUNCTION gen_bootenv_path(), not hardcoded
	 * here
	 */
	(void) sprintf(efile,
		"%s/platform/i86pc/boot/solaris/bootenv.rc",
		get_rootdir());

	/*
	 * if we can't find bootenv.rc that we then look for the bootrc
	 */
	if (GetSimulation(SIM_EXECUTE) || access(efile, R_OK) != 0)
		return (_setup_i386_bootrc(bdp, slice));

	/*
	 * we know there's a bootenv.rc file at this point; strip
	 * any "setprop bootpath" lines from the existing file
	 */
	write_status(LOGSCR, LEVEL1|LISTITEM, MSG0_BOOTENV_INSTALL);

	(void) sprintf(tfile, "%s-", efile);
	(void) unlink(tfile);
	(void) sprintf(edit,
		"/usr/bin/sed -e '/^setprop bootpath/d' < %s > %s",
		efile, tfile);
	(void) system(edit);

	/*
	 * find the string that needs to be substituted, and if one
	 * is defined, add the "setprop bootpath" line to the file
	 */
	if ((lp = get_bootpath(disk_name(bdp), slice)) != NULL) {
		/*
		 * append on the new (correct) entry, and replace the
		 * current bootenv.rc file with the temporary edited
		 * copy
		 */
		if (access(tfile, R_OK) != 0 ||
				(fp = fopen(tfile, "a")) == NULL) {
			(void) unlink(tfile);
			return (ERROR);
		}
		(void) fprintf(fp, "setprop bootpath %s\n", lp);
		(void) fclose(fp);
		if (_copy_file(efile, tfile) != NOERR) {
			(void) unlink(tfile);
			return (ERROR);
		}
	}

	(void) unlink(tfile);
	return (NOERR);
}

/*
 * NOTE: This function is no longer needed after Solaris 2.6
 *
 * Function:	_setup_i386_bootrc
 * Description:	/etc/bootrc file is a critical file used during i386 booting.
 *		The file is delivered in a system critical packages and
 *		is installed along with all other files. The file as delivered,
 *		however, requires some configuration before it can be used in a
 *		reboot (similar to /etc/vfstab editing). The line which
 *		needs to be edited is "setprop boot-path <path>", where <path>
 *		must be replaced with the OpenProm-like bootpath of the root
 *		file system slice. The line requiring replacement is in the
 *		middle of the data file, a temporary copy of the file is made
 *		and data is made in /<root path>/etc/bootrc-, and the data is
 *		copied back to the original copy with the appropriate string
 *		substitution, and the temp file is then removed. This logic
 *		preserves the original modes and permissions of the /etc/bootrc
 *		file.
 * Scope:	internal
 * Parameters:	bdp	- valid pointer to boot disk object
 *		slice	- slice index for "/" slice
 * Return:	NOERR	- all work relating to bootrc completed successfully
 *		ERROR	- required work in configuring bootrc failed
 */
int
_setup_i386_bootrc(Disk_t *bdp, int slice)
{
	MFILE *	mp;
	char	linkbuf[MAXNAMELEN];
	char	efile[MAXPATHLEN];
	char	tfile[MAXPATHLEN];
	char	*bp;
	char	*sp;
	char	*cp;
	int	fd = -1;
	int	len;
	int	size;

	/* if this is not an i386 system there is no work to do */
	if (!IsIsa("i386"))
		return (NOERR);

	write_status(LOGSCR, LEVEL1|LISTITEM, MSG0_BOOTRC_INSTALL);

	/*
	 * none of the subsequent actions should be executed if
	 * running in debug (simulation) mode
	 */
	if (GetSimulation(SIM_EXECUTE))
		return (NOERR);

	/*
	 * copy the existing file to a temporary file and mmap in
	 * the temporary file
	 */
	(void) sprintf(efile, "%s/etc/bootrc", get_rootdir());
	(void) sprintf(tfile, "%s-", efile);
	if (_copy_file(tfile, efile) != NOERR)
		return (ERROR);

	if ((mp = mopen(tfile)) == NULL) {
		write_notice(ERRMSG, MSG_OPEN_FAILED, tfile);
		return (ERROR);
	}

	/* reopen the existing file for overwrite */
	if ((fd = open(efile, O_WRONLY|O_TRUNC)) < 0) {
		write_notice(ERRMSG, MSG_OPEN_FAILED, efile);
		mclose(mp);
		return (ERROR);
	}

	if ((len = readlink(make_block_device(disk_name(bdp), slice),
			linkbuf, MAXNAMELEN)) < 0) {
		write_notice(ERRMSG,
			MSG1_READLINK_FAILED,
			make_block_device(disk_name(bdp), slice));
		(void) close(fd);
		mclose(mp);
		(void) unlink(tfile);
		return (ERROR);
	}

	linkbuf[len] = '\0';
	cp = strstr(linkbuf, "devices/") + 7;
	(void) sprintf(cmd, "setprop boot-path %s\n", cp);

	/*
	 * find the boot-path property path and assemble the newly
	 * formatted setprop bootpath string; write out the new file
	 * with the setprop replacement, copy the remainder of the
	 * original file, and unlink the temporary file
	 */
	bp = (char *) mp->m_base;
	size = (int) mp->m_size;
	if ((sp = strstr(bp, "#setprop boot-path")) == NULL &&
			(sp = strstr(bp, "# setprop boot-path")) == NULL &&
			(sp = strstr(bp, "setprop boot-path")) == NULL) {
		(void) close(fd);
		mclose(mp);
		(void) unlink(tfile);
		/* NOTE: there should be an error message here */
		return (ERROR);
	}

	if (write(fd, bp, sp - bp) != (sp - bp) ||
			write(fd, cmd, strlen(cmd)) != strlen(cmd) ||
			(sp = strchr(sp, '\n')) == NULL ||
			++sp == NULL ||	/* just to force increment */
			write(fd, sp, size - (int)(sp - bp)) !=
				(size - (int)(sp - bp))) {
		(void) close(fd);
		mclose(mp);
		(void) unlink(tfile);
		/* NOTE: there should be an error message here */
		return (ERROR);
	}

	(void) close(fd);
	mclose(mp);
	(void) unlink(tfile);
	return (NOERR);
}

/*
 * Function:	_setup_install_log
 * Description: Copy the install_log file to the installed system and
 *		setup the symbolic link from var/sadm/install_data for
 *		backwards compatibiliy requirements as stated in
 *		PSARC/1994/331.
 *
 *		NOTE:	the symbolic link may be removed as of 2.6
 *
 * Scope:	internal
 * Parameters:	none
 * Return:	NOERR	- log installed successfully
 *		ERROR	- log installation failed
 */
int
_setup_install_log(void)
{
	char	path[64] = "";
	char	newpath[64] = "";
	char	oldpath[64] = "";

	(void) sprintf(oldpath, "%s%s/install_log",
		get_rootdir(), OLD_DATA_DIRECTORY);
	(void) sprintf(newpath, "%s%s/install_log",
		get_rootdir(), SYS_LOGS_DIRECTORY);
	(void) sprintf(path, "%s/install_log",
		SYS_LOGS_RELATIVE);

	write_status(SCR, LEVEL0, MSG0_INSTALL_LOG_LOCATION);
	if (INDIRECT_INSTALL) {
		write_status(SCR, LEVEL1|LISTITEM,
			MSG1_INSTALL_LOG_BEFORE, oldpath);
	}

	write_status(SCR, LEVEL1|LISTITEM, MSG1_INSTALL_LOG_AFTER,
	    newpath+strlen(get_rootdir()));

	if (!GetSimulation(SIM_EXECUTE)) {
		/*
		 * This file is created on upgrades also, so...
		 * Move an existing logfile to a new location in dated form
		 */
		rm_link_mv_file(newpath, newpath);

		if (access(TMPLOGFILE, F_OK) == 0) {
			if (_copy_file(newpath, TMPLOGFILE) == ERROR ||
					symlink(path, oldpath) < 0)
				return (ERROR);
			(void) chmod(newpath, 0644);
		}
	}

	return (NOERR);
}

/*
 * Function:	_setup_tmp_root
 * Description:	Copy files from the transfer list (parameter transL), which are
 *		located in /tmp/root, to the indirect install base (only
 *		applies	to indirect installs).
 * Scope:	internal
 * Parameters:	transL	a pointer to the list of files being transfered.
 * Return:	NOERR	Either this is an indirect installation and nothing
 *			was done. Or all of the applicable files were copied
 *			from /tmp/root to /a.
 *		ERROR	Some error occured, the transfer list was corrupted, a
 *			file could not be copied, or the attributes could not
 *			be set.
 */
int
_setup_tmp_root(TransList **transL)
{
	TransList	*trans = *transL;
	char		tmpFile[MAXPATHLEN];	/* /tmp/root file name	*/
	char		aFile[MAXPATHLEN];	/* name of /a file	*/
	struct stat	Stat_buf;
	int		error = 0,
			i;

	/* only set up /tmp/root for indirect installs */
	if (DIRECT_INSTALL || GetSimulation(SIM_EXECUTE))
		return (NOERR);

	/* Make sure the 1st element of array is not corrupted */
	if ((trans[0].found <= 0) || (trans[0].file != NULL)) {
		write_notice(ERRMSG, MSG0_TRANS_CORRUPT);
		return (ERROR);
	}

	/* Step through the transfer array looking for items to process */
	for (i = 1; i <= trans[0].found; i++) {
		(void) sprintf(aFile, "%s%s",
			get_rootdir(), trans[i].file);
		(void) sprintf(tmpFile, "/tmp/root%s", trans[i].file);
		/*
		 * If the file in question is not present in tmp/root
		 * then skip it. (this happens when the file is not in
		 * /tmp/root before the installation.
		 */
		if (stat(tmpFile, &Stat_buf) >= 0) {
			/* If the /tmp/root file is present ... */
			/* remove the /a file (since it is a symlink) */
			(void) unlink(aFile);

			/* Is this file really a directory? */
			if ((Stat_buf.st_mode & S_IFDIR) == S_IFDIR) {
				/*
				 * Since this is a directory, we know that
				 * is have been created outside of a
				 * normal pkgadd. Thus it just needs tobe
				 * created in /a and given the correct
				 * attributes.
				 */
				/* Make a direcotry in /a */
				if (mkdir(aFile, Stat_buf.st_mode) < 0)
					error = 1;
				/* Change its ownership to be the way it */
				/* was created */
				if (chown(aFile, Stat_buf.st_uid,
					Stat_buf.st_gid) < 0) {
					write_notice(WARNMSG,
						MSG1_TRANS_ATTRIB_FAILED,
						aFile);
					error = 1;
				}
			} else { /* not a directory, but a file */
				if (_copy_file(aFile, tmpFile) == ERROR) {
					error = 1;
				} else if (trans[i].found != 0) {
					/* Set the various attributes of */
					/* the /a file */
					if ((chmod(aFile, trans[i].mode) <
						0) || (chown(aFile,
							trans[i].uid,
							trans[i].gid) <
							0)) {
						write_notice(WARNMSG,
						    MSG1_TRANS_ATTRIB_FAILED,
						    aFile);
						error = 1;
					}
				}
			}
		}
		/* free up the space taken by the file and package name */
		if (trans[i].file != NULL)
			free(trans[i].file);
		if (trans[i].package != NULL)
			free(trans[i].package);
	}

	/* Give back the borrowed memory */
	free(trans);

	if (error)
		return (ERROR);
	else
		return (NOERR);
}

/*
 * Function:	_setup_vfstab
 * Description:	Create the  <bdir>/etc/vfstab file.
 *		This function sets up the /etc/vfstab. In order to have it
 *		copied to the correct file system at the end, it is made and
 *		then put in /tmp/root/etc, to be copied over when the real
 *		filesystem would be.
 * Scope:	internal
 * Parameters:	vent	- pointer to mount list to be used to create vfstab
 * Return:	NOERR	- successful
 * 		ERROR	- error occurred
 */
int
_setup_vfstab(OpType Operation, Vfsent **vent)
{
	char    	buf[128] = "";
	Vfsent  	*vp;
	FILE    	*infp;
	FILE    	*outfp;
	struct vfstab   *ent;
	u_char  	status = (GetSimulation(SIM_EXECUTE) ? SCR : LOG);
	char		vfile[64] = "";
	char		*v;

	write_status(LOGSCR, LEVEL1|LISTITEM, MSG0_MOUNT_POINTS);

	/*
	 * merge mount list entries from the existing /etc/vfstab file
	 * with the new new mount list
	 */
	if (_merge_mount_list(Operation, vent) == ERROR)
		return (ERROR);

	/*
	 * open the appropriate vfstab file for reading
	 */
	if (((v = getenv("SYS_VFSTAB")) != NULL) && *v)
		(void) strcpy(vfile, v);
	else
		(void) sprintf(vfile, "%s/etc/vfstab", get_rootdir());

	/*
	 * make sure there isn't a residual vfstab file sitting
	 * around, and open the temporary vfstab file for writing
	 */
	(void) unlink(TMPVFSTAB);
	if ((outfp = fopen(TMPVFSTAB, "a")) == NULL) {
		write_notice(ERRMSG,
			MSG1_FILE_ACCESS_FAILED,
			TMPVFSTAB);
		return (ERROR);
	}

	/*
	 * transfer all comment lines directly from the source vfstab
	 * file, and write out the vfstab entries from the merged
	 * mount list
	 */
	if ((infp = fopen(vfile, "r")) != NULL) {
		while (fgets(buf, 128, infp) != NULL && buf[0] == '#')
			(void) fprintf(outfp, buf);

		(void) fclose(infp);
	}

	/*
	 * load the entries from the mount list into the vfstab file
	 */
	WALK_LIST(vp, *vent) {
		ent = vp->entry;
		write_status(status, LEVEL1|LISTITEM|CONTINUE,
			"%s\t%s\t%s\t%s\t%s\t%s\t%s",
			ent->vfs_special ? ent->vfs_special : "-", \
			ent->vfs_fsckdev ? ent->vfs_fsckdev : "-", \
			ent->vfs_mountp ? ent->vfs_mountp : "-", \
			ent->vfs_fstype ? ent->vfs_fstype : "-", \
			ent->vfs_fsckpass ? ent->vfs_fsckpass : "-", \
			ent->vfs_automnt ? ent->vfs_automnt : "-", \
			ent->vfs_mntopts ? ent->vfs_mntopts : "-");
			(void) putvfsent(outfp, ent);
	}

	(void) fclose(outfp);
	(void) sprintf(buf, "%s/%s",
		INDIRECT_INSTALL ? "/tmp/root" : "", VFSTAB);

	/*
	 * only do the actual installation of the temporary file if this
	 * is a live run
	 */
	if (!GetSimulation(SIM_EXECUTE)) {
		if (_copy_file(buf, TMPVFSTAB) == ERROR) {
			write_notice(ERRMSG, MSG0_VFSTAB_INSTALL_FAILED);
			return (ERROR);
		}
	}

	return (NOERR);
}

/*
 * Function:	_setup_vfstab_unselect
 * Description: Scan all unselected disk for any slices with mountpoints
 *		beginning with '/' and assemble a vfstab entry in
 *		<bdir>/var/sadm/system/data/vfstab.unselected for the
 *		convenience of the system administrator.
 * Scope:	internal
 * Parameters:	none
 * Return:	NOERR	the vfstab.unselected file was either unnecessary
 *			or was created successfully
 *		ERROR	vfstab.unselected file should have been created,
 *			but was not
 */
int
_setup_vfstab_unselect(void)
{
	FILE		*fp = stderr;
	Disk_t		*dp;
	int		i;
	int		count;
	struct vfstab	*vfsp;
	Vfsent		*vp = NULL;
	Vfsent		*tmp;
	struct vfstab	*ent;
	char		buf[64] = "";

	/*
	 * scan through all unselected drives; merge all mount
	 * points found on those drives into the unselected drive
	 * mount linked list; only slices with file systems are
	 * considered for this list
	 */
	count = 0;
	WALK_DISK_LIST(dp) {
		if (disk_selected(dp) || disk_not_okay(dp))
			continue;

		WALK_SLICES(i) {
			if ((vfsp = (struct vfstab *)xcalloc(
					sizeof (struct vfstab))) == NULL)
				return (ERROR);

			vfsnull(vfsp);
			if (orig_slice_mntpnt(dp, i)[0] != '/' ||
					orig_slice_locked(dp, i) ||
					orig_slice_size(dp, i) == 0)
				continue;

			count++;
			vfsp->vfs_special = xstrdup(
					make_block_device(disk_name(dp), i));
			vfsp->vfs_fsckdev = xstrdup(
					make_char_device(disk_name(dp), i));
			vfsp->vfs_mountp = xstrdup(orig_slice_mntpnt(dp, i));
			vfsp->vfs_fstype = xstrdup(MNTTYPE_UFS);
			(void) _merge_mount_entry(vfsp, &vp);
		}
	}

	/*
	 * if there was at least one mount point entry on an unselected
	 * drive, create the vfstab.unselected file and install it on
	 * the target system
	 */
	if (count > 0) {
		write_status(LOGSCR, LEVEL1|LISTITEM,
			MSG0_VFSTAB_UNSELECTED);

		if (!GetSimulation(SIM_EXECUTE)) {
			(void) unlink(TMPVFSTABUNSELECT);
			if ((fp = fopen(TMPVFSTABUNSELECT, "a")) == NULL)
				return (ERROR);

			(void) fprintf(fp, VFSTAB_COMMENT_LINE1);
			(void) fprintf(fp, VFSTAB_COMMENT_LINE2);
			(void) fprintf(fp, VFSTAB_COMMENT_LINE3);
			(void) fprintf(fp, VFSTAB_COMMENT_LINE4);
		}

		WALK_LIST(tmp, vp) {
			ent = tmp->entry;
			write_status(GetSimulation(SIM_EXECUTE) ? SCR : LOG,
				LEVEL1|LISTITEM|CONTINUE,
				"%s\t%s\t%s\t%s\t%s\t%s\t%s",
				ent->vfs_special ? ent->vfs_special : "-", \
				ent->vfs_fsckdev ? ent->vfs_fsckdev : "-", \
				ent->vfs_mountp ? ent->vfs_mountp : "-", \
				ent->vfs_fstype ? ent->vfs_fstype : "-", \
				ent->vfs_fsckpass ? ent->vfs_fsckpass : "-", \
				ent->vfs_automnt ? ent->vfs_automnt : "-", \
				ent->vfs_mntopts ? ent->vfs_mntopts : "-");
			if (!GetSimulation(SIM_EXECUTE))
				(void) putvfsent(fp, tmp->entry);
		}

		if (!GetSimulation(SIM_EXECUTE)) {
			(void) fclose(fp);
			(void) sprintf(buf, "%s%s/vfstab.unselected",
				get_rootdir(), SYS_DATA_DIRECTORY);
			if (_copy_file(buf, TMPVFSTABUNSELECT) == ERROR)
				return (ERROR);
		}
	}

	return (NOERR);
}

/*
 * Function:	SystemConfigProm
 * Description:	If the existing boot device differs from the current boot
 *		device, and the system supports prom modification, and the user
 *		has authorized prom modification, then update the prom
 *		configuration by prepending the current boot device to the boot
 *		device list, using the new DDI supplied interfaces.
 * Scope:	internal
 * Parameters:	none
 * Return:	NOERR	Prom updated successfully, or no prom modification
 *			required.
 *		ERROR	Prom update required, authorized, and possible, but
 *			attempt to update failed.
 */
/* 
 * To get around a problem with dbx and libthread, define NODEVINFO
 * to 'comment out' code references to functions in libdevinfo,
 * which is threaded.
 */
int
SystemConfigProm(void)
{
	int 	vip;
	int 	auth;
	char	disk[32];
	int	dev_specifier;
	char	dev_type;
	char	buf[MAXNAMELEN];
	int	retcode;

	/* see if the system is capable of being updated */
	if (BootobjGetAttribute(CFG_CURRENT,
			BOOTOBJ_PROM_UPDATEABLE,	&vip,
			BOOTOBJ_PROM_UPDATE,		&auth,
			NULL) != D_OK || vip == 0 || auth == 0)
		return (NOERR);

	/* compare old and new boot disk and device values */
	if (BootobjCompare(CFG_CURRENT, CFG_EXIST, 1) != D_OK) {
		if (vip == 1 && auth == 1) {
			write_status(LOGSCR, LEVEL1|LISTITEM,
				MSG0_BOOT_FIRMWARE_UPDATE);
			if (!GetSimulation(SIM_EXECUTE) &&
					!GetSimulation(SIM_SYSDISK)) {

				if (BootobjGetAttribute(CFG_CURRENT,
						BOOTOBJ_DISK, disk,
						BOOTOBJ_DEVICE, &dev_specifier,
						BOOTOBJ_DEVICE_TYPE, &dev_type,
						NULL) != D_OK ||
						dev_specifier < 0 ||
						streq(disk, ""))
					return (ERROR);

				/*
				 * create the boot device specification for
				 * the DDI interface routine
				 */
				(void) sprintf(buf, "/dev/dsk/%s%c%d",
				    disk, dev_type, dev_specifier);
#ifndef NODEVINFO
				if ((retcode = devfs_bootdev_set_list(buf,
						0)) != 0) {
					/*
					 * if by prepending we will exceed the
					 * prom limits then attempt to
					 * overwrite boot dev
					 */

					if (retcode != DEVFS_LIMIT ||
						    (retcode == DEVFS_LIMIT &&
						    devfs_bootdev_set_list(buf,
						    BOOTDEV_OVERWRITE) != 0))
						return (ERROR);
				}
#endif
			}
		}
	}

	return (NOERR);
}

/* ---------------------- private functions ----------------------- */

#define	MAXLINE	2048	/* maximum expected line length */
/*
 * Function:	get_bootpath
 * Description: Quick and ugly (very ugly) hack to pull "boot-device" out
 *		of the device tree and add :a to it.  all the ugliness is
 *		because prtconf insists on printing the string in hex. This
 *		routine will be replaced by a library real soon now.
 *
 *	char *get_bootpath(char *disk, int slice)
 *
 *		returns NULL if the bootpath cannot be determined,
 *		otherwise returns the right-hand-side of the line
 *		for the file /platform/i86pc/boot/solaris/bootenv.rc
 *		that reads:
 *
 *			setprop bootpath RHS
 *
 *		NOTE: "bootpath" does not have a dash in it!
 */
static char *
get_bootpath(char *disk, int slice)
{
	static char outline[MAXLINE];
	static char boottok[] = "<boot-device>";
	static char linktok[] = "../../devices";
	char inline[MAXLINE];
	char linkline[MAXLINE];
	FILE *infp;
	char *incp;
	char *outcp;
	char *retval = NULL;

	/* run "prtconf -v" */
	if ((infp = popen("/usr/sbin/prtconf -v", "r")) == NULL)
		return (NULL);

	/*
	 * read output of prtconf -v, looking for line after <boot-device>.
	 * note that we continue to read the output even after we've found
	 * what we're looking for to avoid a "broken pipe" message.
	 */
	while (fgets(inline, MAXLINE, infp) != NULL)
		if ((incp = strchr(inline, '<')) != NULL &&
		    (strncmp(incp, boottok, strlen(boottok)) == 0) &&
		    (fgets(inline, MAXLINE, infp) != NULL) &&
		    (incp = strchr(inline, 'x'))) {
			/*
			 * got it, convert prtconf's hex back into a string
			 */
			incp++;	/* skip the 'x' */
			outcp = outline;
			while (isxdigit(incp[0]) && (isxdigit(incp[1]))) {
				(void) sscanf(incp, "%02x", outcp);
				incp += 2;
				if (*outcp == '\0')
					break;
				outcp++;
			}

			/* tack on the slice name (with sanity check) */
			*outcp++ = ':';
			if ((slice > 0) && (slice < 26))
				*outcp++ = 'a' + slice;
			else
				*outcp++ = 'a';
			*outcp = '\0';
			retval = outline;
			/*
			 * now we have a bootpath the corresponds to the
			 * BIOS primary drive.  if the root device doesn't
			 * appear to be the BIOS primary drive, then it is
			 * better for us to return NULL and let the user
			 * pick the correct boot device from the menu.
			 *
			 */
			(void) sprintf(inline, "/dev/dsk/%ss%d", disk, slice);
			if ((readlink(inline, linkline, MAXLINE) < 0) ||
			    (strlen(linkline) < strlen(linktok)))
				/* ??? just leave well enough alone... */
				;
			else {
				incp = &linkline[strlen(linktok)];
				outcp = outline;
				/*
				 * check the bootpath against /devices up to
				 * the "at" sign...
				 */
				while (*incp && (*incp != '@')) {
					if (*incp != *outcp) {
						retval = NULL;
						break;
					} else {
						incp++;
						outcp++;
					}
				}
			}
		}

	/* never found the line we wanted... */
	(void) pclose(infp);
	return (retval);
}
