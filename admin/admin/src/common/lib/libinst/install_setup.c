#ifndef lint
#pragma ident "@(#)install_setup.c 1.117 95/06/20"
#endif
/*
 * Copyright (c) 1991-1995 Sun Microsystems, Inc.  All Rights Reserved. Sun
 * considers its source code as an unpublished, proprietary trade secret, and
 * it is available only under strict license provisions.  This copyright
 * notice is placed here only to protect Sun in the event the source is
 * deemed a published work.  Dissassembly, decompilation, or other means of
 * reducing the object code to human readable form is prohibited by the
 * license agreement under which this code is provided to the user or company
 * in possession of this copy.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the Government
 * is subject to restrictions as set forth in subparagraph (c)(1)(ii) of the
 * Rights in Technical Data and Computer Software clause at DFARS 52.227-7013
 * and in similar clauses in the FAR and NASA FAR Supplement.
 */
/*
 * MODULE PURPOSE:	Setup the customerize system files for the installed
 *			platform
 */
#include "disk_lib.h"
#include "ibe_lib.h"

#include <sys/vfstab.h>
#include <sys/mntent.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/systeminfo.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>

/* Local Statics and Constants */

static char	cmd[MAXNAMELEN];

/* Public Function Prototypes */

/* Library Function Prototypes */

int		_setup_vfstab(Vfsent *);
int		_setup_vfstab_unselect(void);
int		_setup_etc_hosts(Dfs *);
int		_setup_devices(void);
int		_setup_bootblock(void);
int		_setup_inetdconf(void);
int		_setup_hostid(void);
int		_setup_tmp_root(TransList **);
int		_setup_software(Module *, TransList **);
int		_setup_install_log(void);
int		_setup_disks(Disk_t *, Vfsent *);

/* Local Function Prototypes */

static int	_newfs_disks(Vfsent *);
static int	_label_disks(Disk_t *);
static int	_format_disk(Disk_t *);
static int	_label_sdisk(Disk_t *);
static int	_label_fdisk(Disk_t *);
static int 	_create_ufs(Disk_t *, int, int);
static int 	_check_ufs(Disk_t *, int, int);
static int	_load_alt_slice(Disk_t *);
static void	_setup_admin_file(Admin_file *);
static void	_setup_pkg_params(PkgFlags *);
static int	_merge_mount_list(Vfsent **);
static int	_setup_software_results(Module *);
static int	_setup_transferlist(TransList **);
static int	_setup_i386_bootrc(Disk_t *, int);

/* ******************************************************************** */
/*			LIBRARY SUPPORT FUNCTIONS			*/
/* ******************************************************************** */
/*
 * _setup_disks()
 *	Update the fdisk and solaris labels on all selected drives,
 *	start swapping to all swap devices (not swap files), create
 *	or check UFS file systems on all devices which are specified.
 * Parameters:
 *	dlist	- pointer to disk list
 *	vlist	- pointer to mount list
 * Return:
 *	NOERR	- successful updating of disk state
 *	ERROR	- update failed
 * Status:
 *	semi-private (internal library use only)
 */
int
_setup_disks(Disk_t *dlist, Vfsent *vlist)
{
	/*
	 * update the fdisk and Solaris VTOC labels on all selected drives
	 */
	if (_label_disks(dlist) == 0) {
		write_notice(ERRMSG, MSG0_DISK_LABEL_FAILED);
		return (ERROR);
	}

	/*
	 * start swapping to all swap slices as soon as possible
	 * to relieve virtual memory constraints
	 */
	_swap_add(dlist);

	/*
	 * create and check file systems on selected disks
	 * according to the disk list specifications
	 */
	if (_newfs_disks(vlist) != 0) {
		write_notice(ERRMSG, MSG0_DISK_NEWFS_FAILED);
		return (ERROR);
	}

	return (NOERR);
}

/*
 * _setup_hostid()
 *	Set the hostid on any system supporting the i386 model of
 *	hostids.
 * Parameters:
 *	none
 * Return:
 *	NOERR	- set successful
 *	ERROR	- set failed
 * Status:
 *	semi-private (internal library use only)
 */
int
_setup_hostid(void)
{
	char	buf[32] = "";
	char	orig[64] = "";
	char	path[MAXPATHLEN] = "";

	/* cache client hostids are set by hostmanager */
	if (get_machinetype() == MT_CCLIENT)
		return (NOERR);

	/* take no action when running dry-run */
	if (get_install_debug() > 0)
		return (NOERR);

	(void) sprintf(orig, "/tmp/root%s", IDKEY);
	(void) sprintf(path, "%s%s", get_rootdir(), IDKEY);

	/* only set if the original was not saved */
	if (access(orig, F_OK) < 0 &&
			access(path, F_OK) == 0 &&
			(sysinfo(SI_HW_SERIAL, buf, 32) < 0 ||
				buf[0] == '0')) {
		if (setser(path) < 0)
			return (ERROR);
	}

	return (NOERR);
}

/*
 * _setup_vfstab_unselect()
 * 	Scan all unselected disk for any slices with mountpoints
 *	beginning with '/' and assemble a vfstab entry in
 *	<bdir>/var/sadm/system/data/vfstab.unselected for the
 *	convenience of the system administrator.
 * Parameters:
 *	none
 * Return Value:
 *	NOERR	- the vfstab.unselected file was either unnecessary
 *		  or was created successfully
 *	ERROR	- vfstab.unselected file should have been created,
 *		  but was not
 * Status:
 *	semi-private (internal library use only)
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

		if (get_install_debug() == 0) {
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
			write_status(get_install_debug() > 0 ? SCR : LOG,
				LEVEL1|LISTITEM|CONTINUE,
				"%s\t%s\t%s\t%s\t%s\t%s\t%s",
				ent->vfs_special ? ent->vfs_special : "-", \
				ent->vfs_fsckdev ? ent->vfs_fsckdev : "-", \
				ent->vfs_mountp ? ent->vfs_mountp : "-", \
				ent->vfs_fstype ? ent->vfs_fstype : "-", \
				ent->vfs_fsckpass ? ent->vfs_fsckpass : "-", \
				ent->vfs_automnt ? ent->vfs_automnt : "-", \
				ent->vfs_mntopts ? ent->vfs_mntopts : "-");
			if (get_install_debug() == 0)
				(void) putvfsent(fp, tmp->entry);
		}

		if (get_install_debug() == 0) {
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
 * _setup_vfstab()
 *	Create the  <bdir>/etc/vfstab file.
 * 	This function sets up the /etc/vfstab.  In order to have it copied
 *	to the correct file system at the end, it is made and then put in
 *	/tmp/root/etc, to be copied over when the real filesystem would be.
 * Parameters:
 *	vent	- pointer to mount list to be used to create vfstab
 * Return Value :
 *	NOERR	- successful
 * 	ERROR	- error occurred
 * Status:
 *	semi-private (internal library use only)
 */
int
_setup_vfstab(Vfsent *vent)
{
	char    	buf[128] = "";
	Vfsent  	*vp;
	FILE    	*infp;
	FILE    	*outfp;
	struct vfstab   *ent;
	u_char  	status = (get_install_debug() > 0 ? SCR : LOG);
	char		vfile[64] = "";
	char		*v;

	write_status(LOGSCR, LEVEL1|LISTITEM, MSG0_MOUNT_POINTS);

	/*
	 * merge mount list entries from the existing /etc/vfstab file
	 * with the new new mount list
	 */
	if (_merge_mount_list(&vent) == ERROR)
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
	WALK_LIST(vp, vent) {
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
	if (get_install_debug() == 0) {
		if (_copy_file(buf, TMPVFSTAB) == ERROR) {
			write_notice(ERRMSG, MSG0_VFSTAB_INSTALL_FAILED);
			return (ERROR);
		}
	}

	return (NOERR);
}

/*
 * _setup_bootblock()
 * 	Install boot blocks on boot disk.
 * Parameters:
 *	none
 * Return:
 *	ERROR	- boot block installation failed
 *	NOERR	- boot blocks installed successfully
 * Status:
 *	semi-private (internal library use only)
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

	if ((bdp = find_bootdisk()) == NULL || disk_not_selected(bdp)) {
		write_notice(ERRMSG, MSG0_BOOT_DISK_UNSELECT);
		return (ERROR);
	}

	if (find_mnt_pnt(bdp, NULL, ROOT, &info, CFG_CURRENT) == 0) {
		write_notice(ERRMSG, MSG0_ROOT_UNSELECTED);
		return (ERROR);
	}

	write_status(LOGSCR, LEVEL1|LISTITEM,
		MSG1_BOOT_BLOCKS_INSTALL,
		disk_name(bdp));

	/*
	 * if you are not running in dry-run mode, find the new bootblocks
	 * which were just installed on the system and install them on
	 * the boot disk
	 */
	if (get_install_debug() == 0) {
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
	if (_setup_i386_bootrc(bdp, info.slice) != NOERR)
		return (ERROR);

	return (NOERR);
}

/*
 * _setup_etc_hosts()
 *	Create the system 'etc/hosts' file using the remote file systems
 *	specified by the user during installation configuration.
 * Parameters:
 *	cfs	- pointer to remote file system list
 * Return:
 *	NOERR	- /etc/hosts file created successfully
 *	ERROR	- attempt to create /etc/hosts file failed
 * Status:
 *	semi-private (internal library use only)
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

	if (get_install_debug() > 0)
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
				cp = inet_ntoa(*((struct in_addr *)
					/*LINTED [alignment ok]*/
					ent->h_addr));
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
 * _setup_devices()
 *	Configure the /dev and /devices directory by copying over from
 *	the running system /dev and /devices directory. Install the
 *	/reconfigure file so that an automatic "boot -r" will occur.
 *	Note that a minimum set of devices must be present in the
 *	/dev and /devices tree in order for the system to even boot
 *	to the level of reconfiguration.
 * Parameters:
 *	none
 * Return:
 *	NOERR	- setup successful
 *	ERROR	- setup failed
 * Stauts:
 *	semi-private (internal library use only)
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

	if (get_install_debug() == 0) {
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

	if (get_install_debug() == 0) {
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
 * _setup_inetdconf()
 *	Make necessary customization modifications to inetd.conf
 *	based on the installation parameters:
 *
 *			CURRENTLY A NO-OP
 *
 *	NOTE:	it is assumed that the /tmp/root/etc/inet directory
 *		already exists since the hosts file has to reside
 *		there and must have been set up to boot the install
 *		environment
 *
 * Parameters:
 *	none
 * Return:
 *	NOERR	- modifications successful
 * Status:
 *	semi-private (internal library use only)
 */
int
_setup_inetdconf(void)
{
	return (NOERR);
}

/*
 * _setup_tmp_root()
 *	Copy files from the transfer list (parameter transL), which are
 *	located in /tmp/root, to the indirect install base (only
 *	applies to indirect installs).
 * Parameters:
 *	transL	- a pointer to the list of files being transfered.
 * Return:
 *	NOERR	- Either this is an indirect installation and nothing was
 *		  done. Or all of the applicable files were copied from
 *		  /tmp/root to /a.
 *	ERROR	- Some error occured, the transfer list was corrupted, a
 *		  file could not be copied, or the attributes could not be
 *		  set.
 * Status:
 *	semi-private (internal library use only)
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
	if (DIRECT_INSTALL || get_install_debug() > 0)
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
 * _setup_software()
 * Parameters:
 *	prod	- pointer to product structure
 *	trans	- A pointer to the list of files being transfered from
 *		  /tmp/root to the indirect install location.
 * Return:
 *	NOERR	- success
 *	ERROR	- error occurred
 * Status:
 *	semi-private (internal library routine)
 */
int
_setup_software(Module *prod, TransList **trans)
{
	Admin_file	admin;
	PkgFlags	pkg_parms;

	if (get_machinetype() == MT_CCLIENT)
		return (NOERR);

	/* Read in the transferlist of files */
	if (_setup_transferlist(trans) == ERROR) {
		write_notice(ERRMSG, MSG0_TRANS_SETUP_FAILED);
		return (ERROR);
	}

	_setup_admin_file(&admin);
	_setup_pkg_params(&pkg_parms);

	/* print the solaris installation introduction  message */
	write_status(LOGSCR, LEVEL0, MSG0_SOLARIS_INSTALL_BEGIN);

	/* install software packages */
	if (_install_prod(prod, &pkg_parms,
			&admin, trans) == ERROR)
		return (ERROR);

	/* print out the results of the installation */
	_print_results(prod);

	/*
	 * install the software related files on installed system
	 * for future upgrade
	 */
	if (_setup_software_results(prod) != NOERR) {
		write_notice(ERRMSG, MSG0_ADMIN_INSTALL_FAILED);
		return (ERROR);
	}

	return (NOERR);
}

/*
 * _setup_install_log()
 *	Copy the install_log file to the installed system and
 *	setup the symbolic link from var/sadm/install_data for
 *	backwards compatibiliy requirements as stated in
 *	PSARC/1994/331.
 *
 *	NOTE:	the symbolic link may be removed as of 2.6
 * Parameters:
 *	none
 * Return:
 *	NOERR	- log installed successfully
 *	ERROR	- log installation failed
 * Status:
 *	semi-private (internal library use only)
 */
int
_setup_install_log(void)
{
	char	path[64] = "";
	char	newpath[64] = "";
	char	oldpath[64] = "";

	write_status(SCR, LEVEL0, MSG0_INSTALL_LOG_LOCATION);
	/*
	 * post the existing location only for indirect installs
	 */
	if (INDIRECT_INSTALL) {
		write_status(SCR, LEVEL1|LISTITEM,
			MSG1_INSTALL_LOG_BEFORE,
			"/tmp");
	}

	write_status(SCR, LEVEL1|LISTITEM,
		MSG1_INSTALL_LOG_AFTER,
		SYS_LOGS_DIRECTORY);

	(void) sprintf(oldpath, "%s%s/install_log",
		get_rootdir(), OLD_DATA_DIRECTORY);
	(void) sprintf(newpath, "%s%s/install_log",
		get_rootdir(), SYS_LOGS_DIRECTORY);
	(void) sprintf(path, "%s/install_log",
		SYS_LOGS_RELATIVE);

	if (get_install_debug() == 0) {
		if (_copy_file(newpath, TMPLOGFILE) == ERROR ||
				symlink(path, oldpath) < 0)
			return (ERROR);
		(void) chmod(newpath, 0644);
	}

	return (NOERR);
}

/* ******************************************************************** */
/*			LOCAL SUPPORT FUNCTIONS				*/
/* ******************************************************************** */
/*
 * _setup_admin_file()
 *	Initialize the fields of an existing admin structure
 * Parameters:
 *	admin	- non-NULL pointer to the Admin structure
 *		  to be initialized
 * Return:
 *	none
 * Status:
 *	private
 */
static void
_setup_admin_file(Admin_file *admin)
{
	static char 	nocheck[] = "nocheck";
	static char 	unique[] = "unique";
	static char 	quit[] = "quit";
	static char 	blank[] = " ";

	if (admin != NULL) {
		admin->mail = blank;
		admin->instance = unique;
		admin->partial = nocheck;
		admin->runlevel = nocheck;
		admin->idepend = nocheck;
		admin->rdepend = quit;
		admin->space = nocheck;
		admin->setuid = nocheck;
		admin->action = nocheck;
		admin->conflict = nocheck;
		admin->basedir = blank;
	}
}

/*
 * _setup_pkg_params()
 *	Initialize the package params structure to be used
 *	during pkgadd calls.
 * Parameters:
 *	params	- non-NULL pointer to the PkgFlags structure to be
 *		  initialized
 * Return:
 *	none
 * Status:
 *	private
 */
static void
_setup_pkg_params(PkgFlags *params)
{
	if (params != NULL) {
		params->silent = 1;
		params->checksum = 1;
		params->notinteractive = 1;
		params->accelerated = 1;
		params->spool = NULL;
		params->admin_file = (char *)admin_file(NULL);
		params->basedir = get_rootdir();
	}
}

/*
 * _merge_mount_list()
 *	Merge vfstab entries from the existing vfstab with the
 *	explicitly specified mount entries. Exclude all existing
 *	entries which:
 *
 *	(1)  have duplicate special device entries with existing
 *	     entries
 *	(2)  have duplicate mount point names
 *	(3)  are swap files
 *
 * Parameters:
 *	vlist	- address of pointer to head of mount list
 * Return:
 *      NOERR	- merge successful
 *	ERROR	- merge failed
 * Status:
 *      private
 */
static int
_merge_mount_list(Vfsent **vlist)
{
	struct vfstab   *vp;
	struct vfstab   vfstab;
	Disk_t		*dp;
	FILE		*fp;
	int		status = NOERR;
	char		vfile[32] = "";
	char		*v;

	if (vlist == NULL)
		return (ERROR);

	/*
	 * open the appropriate vfstab file for reading
	 */
	if (((v = getenv("SYS_VFSTAB")) != NULL) && *v)
		(void) strcpy(vfile, v);
	else
		(void) sprintf(vfile, "%s/etc/vfstab", get_rootdir());

	if (get_install_debug() > 0) {
		write_status(LOGSCR, LEVEL1|LISTITEM,
			MSG1_VFSTAB_ORIG_OPEN,
			vfile);
	}

	if ((fp = fopen(vfile, "r")) == NULL) {
		/*
		 * failure to open the original vfstab file is only an
		 * error if you are running live; otherwise it is just
		 * a warning
		 */
		if (get_install_debug() == 0) {
			write_notice(ERRMSG,
				MSG_OPEN_FAILED,
				vfile);
			return (ERROR);
		}

		write_notice(WARNMSG,
			MSG_OPEN_FAILED,
			vfile);
	} else {
		/*
		 * scan through the existing vfstab file and add
		 * in any entries which are not swap, or on selected
		 * disks
		 */
		for (vfsnull(&vfstab); getvfsent(fp, &vfstab) == 0;
				vfsnull(&vfstab)) {
			/*
			 * strip out swap files
			 */
			if (strcmp(vfstab.vfs_fstype, MNTTYPE_SWAP) == 0 &&
					vfstab.vfs_special[0] == '/')
				continue;

			/*
			 * strip out existing entries on selected disks
			 */
			if ((dp = find_disk(vfstab.vfs_special)) != NULL &&
					disk_selected(dp))
				continue;

			if ((vp = (struct vfstab *)
				    xcalloc(sizeof (struct vfstab))) == NULL) {
				status = ERROR;
				break;
			}

			vp->vfs_special = xstrdup(vfstab.vfs_special);
			vp->vfs_fsckdev = xstrdup(vfstab.vfs_fsckdev);
			vp->vfs_mountp = xstrdup(vfstab.vfs_mountp);
			vp->vfs_fstype = xstrdup(vfstab.vfs_fstype);
			vp->vfs_fsckpass = xstrdup(vfstab.vfs_fsckpass);
			vp->vfs_automnt = xstrdup(vfstab.vfs_automnt);
			vp->vfs_mntopts = xstrdup(vfstab.vfs_mntopts);

			/* merge the new entry into the mount list */
			if (_merge_mount_entry(vp, vlist) == ERROR) {
				status = ERROR;
				_vfstab_free_entry(vp);
				break;
			}
		}

		(void) fclose(fp);
	}

	return (status);
}

/*
 * _setup_software_results()
 * 	Copy the .clustertoc to the installed system and create
 *	the CLUSTER software administration files.
 * Parameters:
 *	prod	- pointer to product structure
 * Return:
 *	NOERR	- results file set up successfully
 *	ERROR	- results file failed to set up
 * Status:
 *	private
 */
static int
_setup_software_results(Module *prod)
{
	char	path[64] = "";
	FILE	*fp;
	Module  *mp;

	if (get_install_debug() > 0)
		return (NOERR);

	/*
	 * Copy the .clustertoc file.
	 */
	(void) sprintf(path, "%s%s/.clustertoc",
		get_rootdir(), SYS_ADMIN_DIRECTORY);
	if (_copy_file(path, get_clustertoc_path(NULL)) != NOERR)
		return (ERROR);

	/*
	 * Create the .platform file.
	 */
	if (write_platform_file(get_rootdir(), prod) != SUCCESS)
		return (ERROR);

	WALK_LIST(mp, get_current_metacluster()) {
		if (mp->info.mod->m_status == SELECTED ||
				mp->info.mod->m_status == REQUIRED)
			break;
	}

	if (mp == NULL)
		return (ERROR);

	/*
	 * Create the CLUSTER file based on the current metacluster
	 */
	(void) sprintf(path, "%s%s/CLUSTER",
		get_rootdir(), SYS_ADMIN_DIRECTORY);
	if ((fp = fopen(path, "a")) != NULL) {
		(void) fprintf(fp, "CLUSTER=%s\n", mp->info.mod->m_pkgid);
		(void) fclose(fp);
		return (NOERR);
	}

	return (ERROR);
}

/*
 * _setup_transferlist()
 *	Initialize the transfer list with the files to be transfered to
 *	the indirect installation directory after the initial
 *	installation. The data structures are initialized with data from
 *	the /tmp/.transfer_list file.
 * Parameters:
 *	transL	- a pointer to the TransList structure list to be
 *		  initialized.
 * Return:
 *	NOERROR - setup of transfer list succeeded
 *	ERROR - setup of transfer list failed. Reasons: could not open
 *		file, couldn't read file, couldn't malloc space, or
 *		transfer-file list corrupted.
 * Status:
 *	private
 */
static int
_setup_transferlist(TransList **transL)
{
	FILE		*TransFile;	/* transferlist file pointer	*/
	int		i, allocCount;	/* Simple counter		*/
	TransList 	*FileRecord;	/* tmp trans file item		*/
	char		file[MAXPATHLEN], /* individual transfer files	*/
			package[32];	/* String for the  package name	*/

	/* only setup the transfer list file for indirect installs */
	if (DIRECT_INSTALL || get_install_debug() > 0)
		return (NOERR);

	if ((TransFile = fopen(TRANS_LIST, "r")) == NULL) {
		write_notice(ERRMSG,
			MSG_OPEN_FAILED,
			TRANS_LIST);
		return (ERROR);
	}

	/*
	 * Allocate the array for files and packages
	 * I get 50 entries a time (malloc 50 then realloc 50 more)
	 */
	if ((FileRecord = (TransList *) xcalloc(
			sizeof (TransList) * 50)) == NULL)
		return (ERROR);

	/* initialize the array counter and allocation count */
	i = 1;
	allocCount = 1;

	while (fscanf(TransFile, "%s %s\n", file, package) != EOF) {
		/* Verify that the read was good and the file and package */
		/* are of the correct length. */
		if ((file == NULL) || (package == NULL) ||
		    (strlen(file) > (size_t) MAXPATHLEN) ||
		    (strlen(package) > (size_t) 32)) {
			write_notice(WARNMSG,
				MSG_READ_FAILED,
				TRANS_LIST);
			return (ERROR);
		}

		/* See if we have to reallocate space */
		if ((i / 50) > allocCount) {
			if ((FileRecord = (TransList *) xrealloc(FileRecord,
					sizeof (TransList) *
					(50 * ++allocCount))) == NULL) {
				return (ERROR);
			}
		}

		/* Initialize the record for this file */
		FileRecord[i].file = (char *)xstrdup(file);
		FileRecord[i].package = (char *)xstrdup(package);
		FileRecord[i].found = 0;

		/* increment counter */
		i++;
	}
	/* Store the size of the array in the found filed of the 1st entry */
	FileRecord[0].found = --i;

	/* Just for safety NULL out the package and file */
	FileRecord[0].file = NULL;
	FileRecord[0].package = NULL;

	*transL = FileRecord;

	return (NOERR);
}

/*
 * _label_disks()
 *	Write necessary labels out to all "selected" disks in the disk
 *	list which are in an "okay" state. This includes F-disk and
 *	S-disk labels. Low level format any drives flagged for processing.
 * Parameters:
 *	dp	- pointer to the head of the disk list (NULL If
 *		  the standard disk chain is to be used)
 * Return:
 *	0	- one of the drives failed in writing the label
 *	1	- success writing the label on all drives
 * Status:
 *	private
 */
static int
_label_disks(Disk_t *dp)
{
	if (dp == NULL)
		dp = first_disk();

	for (; dp; dp = next_disk(dp)) {
		if (disk_not_selected(dp) || disk_not_okay(dp))
			continue;

		write_status(LOGSCR, LEVEL0,
			MSG1_DISK_SETUP,
			disk_name(dp));

		/* format disk (if necessary) */
		if (_format_disk(dp) < 0)
			return (0);

		/* update F-disk label (if necessary) */
		if (_label_fdisk(dp) < 0)
			return (0);

		/* update S-disk label (if necessary) */
		if (_label_sdisk(dp) < 0)
			return (0);
	}

	return (1);
}

/*
 * _newfs_disks()
 *	Process the entire disk list. Newfs all UFS file systems with valid
 *	UFS mount points (and non-zero size) on all selected disks which are
 *	not "preserved", "locked", or "ignored". Run fsck for all UFS file
 *	systems which are marked "preserved".
 * Parameters:
 *	vlist	- pointer to list of mount points
 * Return:
 *	 0	- labelling completed successfully
 *	-1	- NEWFS of a default file system failed
 *	 1	- FSCK of default file system failed
 * Status:
 *	private
 */
static int
_newfs_disks(Vfsent *vlist)
{
	Disk_t	*dp;
	int	i;
	int	wait;
	Vfsent	*vp;

	write_status(LOGSCR, LEVEL0, MSG0_CREATE_CHECK_UFS);

	WALK_DISK_LIST(dp) {
		if (sdisk_not_legal(dp) ||
				disk_not_okay(dp) ||
				disk_not_selected(dp))
			continue;

		if (disk_fdisk_exists(dp) && sdisk_geom_null(dp))
			continue;

		WALK_SLICES(i) {
			if (slice_locked(dp, i) ||
					slice_ignored(dp, i) ||
					slice_mntpnt_isnt_fs(dp, i) ||
					slice_size(dp, i) == 0)
				continue;

			WALK_LIST(vp, vlist) {
				if (vp->entry != NULL &&
					    vp->entry->vfs_mountp != NULL &&
					    strcmp(vp->entry->vfs_mountp,
						slice_mntpnt(dp, i)) == 0) {
					wait = _mount_synchronous_fs(
						vp->entry, vlist);
					break;
				}
			}

			/* make sure the wait variable is set */
			if (vp == NULL)
				continue;

			if (slice_preserved(dp, i)) {
				if (_check_ufs(dp, i, wait) == 0)
					continue;

				/*
				 * bail out on FSCK only if it was a
				 * default mount filesystem
				 */
				if (get_dfltmnt_ent((Defmnt_t *)0,
						slice_mntpnt(dp, i)) == D_OK)
					return (1);
			} else if (_create_ufs(dp, i, wait) != 0)
				return (-1);
		}
	}

	return (0);
}

/*
 * _label_sdisk()
 *	If the disk requires a Solaris label, create one using the
 *	current F-disk configuration in the disk structure. Load
 *	the alternate sector slice if one is required.
 *
 *	ALGORITHM:
 *	(1) If the S-disk geometry pointer is NULL, there is no work
 *	    to do (the disk was never validated)
 *	(2) If an F-disk is required, but there is no Solaris partition
 *	    there is no work to do
 *	(3) Load the VTOC from the disk (or setup a dummy if there isn't
 *	    one or if install debug is turned on).
 *	(4) Update that VTOC structure from the current S-disk structure
 *	(5) Write out the VTOC structure (slam the controller on Intel)
 *	(6) If there is an alternate sector slice required, load it
 *	    with the addbadsec(1M)
 *
 *	NOTE:	The VTOC is only read and written, and the alternate
 *		sector slice loaded when the install debug flag is turned off
 *
 *	WARNING:
 *		In 494 there is rumored to be another mechanism for
 *		loading the alternate sector slice, but there is no
 *		information on this at this time
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 * Return:
 *	 0	- success, 'dp' is NULL, or the S-disk geometry pointer
 *		  is NULL
 *	-1	- failure
 * Status:
 *	private
 */
static int
_label_sdisk(Disk_t *dp)
{
	struct vtoc	vtoc;
	char		device[126];
	int		write_alts = 0;
	int		fd;
	int		i;

	if (dp == NULL)
		return (0);

	if (sdisk_geom_null(dp))
		return (0);

	write_status(LOGSCR, LEVEL1|LISTITEM, MSG0_VTOC_CREATE);

	(void) sprintf(device, "/dev/rdsk/%ss2", disk_name(dp));

	if (get_install_debug() == 0) {
		if ((fd = open(device, O_RDWR | O_NDELAY)) == -1) {
			write_notice(ERRMSG, MSG0_SLICE2_ACCESS_FAILED);
			return (-1);
		}
	}

	if (get_install_debug() > 0 || read_vtoc(fd, &vtoc) < 0) {
		(void) memset(&vtoc, 0, sizeof (struct vtoc));
		vtoc.v_sectorsz = 512;
		vtoc.v_nparts = (ushort) numparts;
		vtoc.v_sanity = VTOC_SANE;
		vtoc.v_version = V_VERSION;	/* required for Intel */
	}

	WALK_SLICES(i) {
		if (slice_ignored(dp, i))
			continue;

		vtoc.v_part[i].p_start = cyls_to_blocks(dp, slice_start(dp, i));
		vtoc.v_part[i].p_size = slice_size(dp, i);
		vtoc.v_part[i].p_flag = 0;

		if (i == BOOT_SLICE) {
			vtoc.v_part[i].p_tag = V_BOOT;
			vtoc.v_part[i].p_flag = V_UNMNT;
		} else if (strcmp(slice_mntpnt(dp, i), ROOT) == 0) {
			vtoc.v_part[i].p_tag = V_ROOT;
		} else if (strcmp(slice_mntpnt(dp, i), SWAP) == 0) {
			vtoc.v_part[i].p_tag = V_SWAP;
			vtoc.v_part[i].p_flag = V_UNMNT;
		} else if (i == ALT_SLICE && slice_size(dp, i) > 0) {
			vtoc.v_part[i].p_tag = V_ALTSCTR;
			vtoc.v_part[i].p_flag = V_UNMNT;
			write_alts = 1;
		} else if (strncmp(slice_mntpnt(dp, i), USR, 4) == 0) {
			vtoc.v_part[i].p_tag = V_USR;
		} else if (strcmp(slice_mntpnt(dp, i), VAR) == 0) {
			vtoc.v_part[i].p_tag = V_VAR;
		} else if (strncmp(slice_mntpnt(dp, i), HOME, 5) == 0) {
			vtoc.v_part[i].p_tag = V_HOME;
		} else if (strncmp(slice_mntpnt(dp, i), EXPORTHOME, 12) == 0) {
			vtoc.v_part[i].p_tag = V_HOME;
		} else if (strncmp(slice_mntpnt(dp, i), CACHE, 12) == 0) {
			vtoc.v_part[i].p_tag = V_CACHE;
		} else if (slice_is_overlap(dp, i)) {
			vtoc.v_part[i].p_tag = V_BACKUP;
			if (slice_locked(dp, i))
				vtoc.v_part[i].p_flag = V_UNMNT;
		} else if (slice_mntpnt_exists(dp, i)) {
			/* assigned, but not a system FS */
			if (slice_not_preserved(dp, i))
				vtoc.v_part[i].p_tag = V_UNASSIGNED;
		} else {
			/* no mount point and not whole disk */
			if (slice_not_preserved(dp, i)) {
				vtoc.v_part[i].p_tag = V_UNASSIGNED;
				vtoc.v_part[i].p_flag = V_UNASSIGNED;
			}
		}

		if (get_install_debug() > 0) {
			write_status(SCR, LEVEL1|LISTITEM|CONTINUE,
				MSG4_SLICE_VTOC_ENTRY,
				i,
				slice_mntpnt(dp, i),
				vtoc.v_part[i].p_tag,
				vtoc.v_part[i].p_flag);
		}
	}

	if (get_install_debug() > 0)
		return (0);

	/* write out the VTOC (and label) */
	if (write_vtoc(fd, &vtoc) < 0) {
		write_notice(ERRMSG, MSG0_VTOC_CREATE_FAILED);
		(void) close(fd);
		return (-1);
	}

	(void) close(fd);

	/* load alternate sector slice if one exists */

	if (write_alts == 1) {
		if (_load_alt_slice(dp) < 0)
			return (-1);
	}

	return (0);
}

/*
 * _label_fdisk()
 *	Write out an F-disk partition table on a specified drive based
 *	on the current F-disk configuration. This will also install the
 *	mboot file since the partition table is embedded within mboot.
 *
 *	ALGORITHM:
 *	(1) Assemble an	fdisk(1M) input file based on the current fdisk
 *	    configuration in the specified disk structure
 *	(2) Call the fdisk command to install the partition table and
 *	    the mboot file
 *
 *	NOTE: 	this routine does not actually write the partition
 *		table to the disk if the disk debug flag is set.
 *	NOTE:	partitions are written out in the order in which they
 *		were originally loaded, which is the same order in which
 *		they should be displayed to users. This is to prevent
 *		the install library from scrambling partition tables
 *		with the original sort
 * Parameters:
 *	dp	- non-NULL pointer to disk structure
 * Return:
 *	 0	- attempt to write F-disk table succeeded, or no
 *		  partition table required on drive
 *	-1	- attempt to write F-disk label failed
 * Status:
 *	private
 */
static int
_label_fdisk(Disk_t *dp)
{
	u_char  logval;
	FILE    *fp;
	char    *fname;
	int	p;
	int	c;

	if (dp == NULL)
		return (-1);

	if (disk_no_fdisk_exists(dp))
		return (0);

	/*
	 * only report fdisk status information on systems which expose the
	 * fdisk interface, or if tracing is enabled
	 */
	if (disk_fdisk_req(dp) || get_trace_level() > 0) {
		write_status(LOGSCR, LEVEL1|LISTITEM, MSG0_FDISK_CREATE);
		write_status(LOG, LEVEL0, MSG1_FDISK_TABLE, disk_name(dp));

		/*
		 * store a copy of the fdisk input file in the install log or
		 * display to screen depending on debug status
		 */
		logval = (get_install_debug() == 1 ? SCR : LOG);

		WALK_PARTITIONS(c) {
			WALK_PARTITIONS(p) {
				if (part_orig_partnum(dp, p) == c) {
					write_status(logval,
						LEVEL1|LISTITEM|CONTINUE,
						MSG4_FDISK_ENTRY,
						part_id(dp, p),
						part_active(dp, p),
						part_geom_rsect(dp, p),
						part_geom_tsect(dp, p));
					break;
				}
			}
		}

		write_status(logval, LEVEL0, "");
	}

	/* ignore the rest of this code if we are running in dry-run */
	if (get_install_debug() == 1)
		return (0);

	/* create a temporary file for use with the fdisk(1M) command */
	if ((fname = tmpnam(NULL)) == NULL) {
		write_notice(ERRMSG, MSG0_FDISK_INPUT_FAILED);
		return (-1);
	}

	/*
	 * write out the fdisk(1M) input file into the temporary file,
	 * preserving the original partition ordering
	 */
	if ((fp = fopen(fname, "w")) == NULL) {
		write_notice(ERRMSG, MSG0_FDISK_OPEN_FAILED);
		return (-1);
	}

	WALK_PARTITIONS(c) {
		WALK_PARTITIONS(p) {
			if (part_orig_partnum(dp, p) == c) {
				(void) fprintf(fp,
					"%d %d 0 0 0 0 0 0 %d %d\n",
					part_id(dp, p),
					part_active(dp, p),
					part_geom_rsect(dp, p),
					part_geom_tsect(dp, p));
				break;
			}
		}
	}

	(void) fclose(fp);

	/* call fdisk(1M) to put down the fdisk label */
	(void) sprintf(cmd,
		"/sbin/fdisk -n -F %s /dev/rdsk/%sp0 >/dev/null 2>&1",
		fname, disk_name(dp));

	if (system(cmd) != 0) {
		write_notice(ERRMSG, MSG0_FDISK_CREATE_FAILED);
		(void) unlink(fname);
		return (-1);
	}

	(void) unlink(fname);

	return (0);
}

/*
 * _format_disk()
 *	Low level format an entire physical disk.
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 * Return:
 *	D_OK	  - disk analysis completed successfully
 *	D_NODISK  - invalid disk structure pointer
 *	D_BADDISK - disk not in a state to be analyzed
 *	D_BADARG  - could not create tmpfile for format data input
 * Status:
 *	private
 */
static int
_format_disk(Disk_t *dp)
{
	static char	tmpfile[64] = "";
	FILE		*fp;

	if (dp == NULL)
		return (D_NODISK);

	if (disk_not_format_disk(dp))
		return (D_OK);

	if (disk_not_okay(dp))
		return (D_BADDISK);

	write_status(LOGSCR, LEVEL1|LISTITEM,
		MSG0_DISK_FORMAT);

	if (get_install_debug())
		return (D_OK);

	if (tmpfile[0] == '\0') {
		(void) tmpnam(tmpfile);
		if ((fp = fopen(tmpfile, "w")) == NULL) {
			tmpfile[0] = '\0';
			return (D_BADARG);
		}

		(void) fprintf(fp, "f\n");
		(void) fprintf(fp, "quit\n");
		(void) fclose(fp);
	}

	if (disk_fdisk_exists(dp)) {
		(void) sprintf(cmd,
		"/usr/sbin/format -s -d /dev/rdsk/%sp0 -f %s >/dev/null 2>&1",
			disk_name(dp), tmpfile);
	} else {
		(void) sprintf(cmd,
		"/usr/sbin/format -s -d /dev/rdsk/%ss2 -f %s >/dev/null 2>&1",
			disk_name(dp), tmpfile);
	}

	if (system(cmd) != 0) {
		write_notice(ERRMSG,
			MSG1_DISK_FORMAT_FAILED, disk_name(dp));
		return (D_BADDISK);
	}

	return (D_OK);
}

/*
 * _create_ufs()
 *	Create a UFS file system on a specific disk slice. Return
 *	in error only if the mount point name is a default mount list
 *	file system and the newfs fails. Default mount file systems are
 *	created sequentially; all other file systems are created in
 *	parallel in the background. NEWFS failures are only reported for
 *	default mount point file systems.
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 *	slice	- valid slice index number
 *	wait	- 0/1 whether the process should wait or be backgrounded
 * Return:
 *	0	- successful, or 'dp' is NULL, of 'slice' is invalid
 *	#!=0	- error code returned from newfs command
 * Status:
 *	private
 */
static int
_create_ufs(Disk_t *dp, int slice, int wait)
{
	int	status = 0;

	if (dp == NULL || invalid_sdisk_slice(slice))
		return (0);

	write_status(LOGSCR, LEVEL1|LISTITEM,
		MSG3_SLICE_CREATE,
		slice_mntpnt(dp, slice),
		disk_name(dp),
		slice);

	if (get_trace_level() > 0) {
		write_status(SCR, LEVEL2|LISTITEM,
			wait == 1 ?
			MSG0_PROCESS_FOREGROUND :
			MSG0_PROCESS_BACKGROUND);
	}

	(void) sprintf(cmd,
		"%s /usr/sbin/newfs /dev/rdsk/%ss%d </dev/null >>%s 2>&1 %s",
		(wait == 1 ? "nice -19" : ""),
		disk_name(dp),
		slice,
		(wait == 1 ? "/tmp/install_log" : "/dev/null"),
		(wait == 1 ? "" : "&"));

	if (get_install_debug() == 0) {
		status = system(cmd);
		status >>= 8;
		if (wait == 1 && status != 0) {
			write_notice(ERRMSG,
				MSG3_SLICE_CREATE_FAILED,
				slice_mntpnt(dp, slice),
				disk_name(dp),
				slice);
		}
	}

	return (status);
}

/*
 * _check_ufs()
 *	Run fsck on the file system for a specific disk slice. Return
 *	in error only if the mount point name is a default mount list
 *	file system. Default mount file systems are checked sequentially.
 *	All other filesystems are checked in parallel in the background.
 *	FSCK failures are only reported for default mount point file
 *	systems.
 *
 *	NOTE: 	this routine does not actually execute an FSCK if the
 *		disk debug flag is set.
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 *	slice	- slice index
 *	wait	- 0/1 whether the process should wait or be backgrounded
 * Return:
 *	0	- successful, or 'dp' is NULL, of 'slice' is invalid
 *	# != 0	- exit code returned from fsck command
 * Status:
 *	private
 */
static int
_check_ufs(Disk_t *dp, int slice, int wait)
{
	int	status = 0;

	if (dp == NULL || invalid_sdisk_slice(slice))
		return (0);

	write_status(LOGSCR, LEVEL1|LISTITEM,
		MSG3_SLICE_CHECK,
		slice_mntpnt(dp, slice),
		disk_name(dp),
		slice);

	if (get_trace_level() > 0) {
		write_status(SCR, LEVEL2|LISTITEM,
			wait == 1 ?
			MSG0_PROCESS_FOREGROUND :
			MSG0_PROCESS_BACKGROUND);
	}

	if (get_install_debug() > 0)
		return (0);

	(void) sprintf(cmd,
		"%s /usr/sbin/fsck -y -o p -o w /dev/rdsk/%ss%d \
			</dev/null >> %s 2>&1 %s",
		(wait == 1 ? "nice -19" : ""),
		disk_name(dp),
		slice,
		(wait == 1 ? "/tmp/install_log" : "/dev/null"),
		(wait == 1 ? "" : "&"));

	status = system(cmd);
	status >>= 8;

	if (wait == 1 && status != 0) {
		write_notice(ERRMSG,
			MSG3_SLICE_CHECK_FAILED,
			slice_mntpnt(dp, slice),
			disk_name(dp),
			slice);
		return (status);
	}

	return (0);
}

/*
 * _load_alt_slice()
 *	If there is a non-zero alternate sector slice, make sure
 *	initialize the alternate sector table. Do nothing if the
 *	slice is already loaded (initialized) or is zero sized.
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 * Return:
 *	 0	- load successful
 *	-1	- load failed
 * Status:
 *	private
 */
static int
_load_alt_slice(Disk_t *dp)
{
	int	status;

	write_status(LOGSCR, LEVEL1|LISTITEM, MSG0_ALT_SECTOR_SLICE);

	/*
	 * initialize the alternate sector slice with default
	 * data, or do nothing if it's already initialized
	 */
	(void) sprintf(cmd,
		"/usr/bin/addbadsec -f /dev/null /dev/rdsk/%sp0 \
			>/dev/null 2>&1",
		disk_name(dp));

	status = system(cmd);

	if (status != 0) {
		write_notice(ERRMSG, MSG0_ALT_SECTOR_SLICE_FAILED);
		return (-1);
	}

	return (0);
}

/*
 * _setup_i386_bootrc()
 *	The /etc/bootrc file is a critical file used during i386 booting.
 *	The file is delivered in one of the system critical packages and
 *	is installed along with all other files. The file as delivered,
 *	however, requires some configuration before it can be used in a
 *	reboot (similar to /etc/vfstab requiring editing). The line which
 *	needs to be edited is "setprop boot-path <path>", where <path>
 *	needs to get replaced with the OpenProm-like bootpath of the root
 *	file system slice. Since the line requiring replacement is in the
 *	middle of the data file, a temporary copy of the file is made
 *	and data is made in /<root path>/etc/bootrc-, and the data is
 *	copied back to the original copy with the appropriate string
 *	substitution, and the temporary file is then removed. This logic
 *	preserves the original modes and permissions of the /etc/bootrc
 *	file.
 * Parameters:
 *	bdp	- valid pointer to boot disk object
 *	slice	- slice index for "/" slice
 * Return:
 *	NOERR	- all work relating to bootrc completed successfully
 *	ERROR	- required work in configuring bootrc failed
 * Static:
 *	private
 */
static int
_setup_i386_bootrc(Disk_t *bdp, int slice)
{
	char	linkbuf[MAXNAMELEN];
	char	efile[MAXPATHLEN];
	char	tfile[MAXPATHLEN];
	char	*bp;
	char	*sp;
	char	*cp;
	int	size;
	int	fd = -1;
	int	len;

	/* if this is not an i386 system there is no work to do */
	if (strneq(get_default_inst(), "i386"))
		return (NOERR);

	write_status(LOGSCR, LEVEL1|LISTITEM, MSG0_BOOTRC_INSTALL);

	/*
	 * none of the subsequent actions should be executed if
	 * running in debug (simulation) mode
	 */
	if (get_install_debug() > 0)
		return (NOERR);

	/*
	 * copy the existing file to a temporary file and mmap in
	 * the temporary file
	 */
	(void) sprintf(efile, "%s/etc/bootrc", get_rootdir());
	(void) sprintf(tfile, "%s-", efile);
	if (_copy_file(tfile, efile) != NOERR)
		return (ERROR);

	if ((size = map_in_file(tfile, &bp)) <= 0) {
		write_notice(ERRMSG, MSG_OPEN_FAILED, tfile);
		return (ERROR);
	}

	/* reopen the existing file for overwrite */
	if ((fd = open(efile, O_WRONLY|O_TRUNC)) < 0) {
		write_notice(ERRMSG, MSG_OPEN_FAILED, efile);
		(void) munmap(bp, size);
		return (ERROR);
	}

	if ((len = readlink(make_block_device(disk_name(bdp), slice),
			linkbuf, MAXNAMELEN)) < 0) {
		write_notice(ERRMSG,
			MSG1_READLINK_FAILED,
			make_block_device(disk_name(bdp), slice));
		(void) close(fd);
		(void) munmap(bp, size);
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
	if ((sp = strstr(bp, "#setprop boot-path")) == NULL &&
			(sp = strstr(bp, "# setprop boot-path")) == NULL &&
			(sp = strstr(bp, "setprop boot-path")) == NULL) {
		(void) close(fd);
		(void) munmap(bp, size);
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
		(void) munmap(bp, size);
		(void) unlink(tfile);
		/* NOTE: there should be an error message here */
		return (ERROR);
	}

	(void) close(fd);
	(void) munmap(bp, size);
	(void) unlink(tfile);
	return (NOERR);
}
