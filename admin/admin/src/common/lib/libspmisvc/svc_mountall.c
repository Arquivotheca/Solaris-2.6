#ifndef lint
#pragma ident "@(#)svc_mountall.c 1.13 96/10/10 SMI"
#endif
/*
 * Copyright (c) 1995 Sun Microsystems, Inc.
 * All Rights Reserved.
 */

#include "spmistore_api.h"
#include "spmicommon_api.h"
#include "spmisoft_lib.h"
#include "spmisvc_lib.h"

#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <libintl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <ustat.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/mntent.h>
#include <sys/wait.h>

#include "svc_templates.h"

/* Local Statics and Constants */

#define	MOUNT_DEV	0
#define	SWAP_DEV	1

struct mountentry {
	struct mountentry *next;
	int	op_type;	/* MOUNT_DEV, SWAP_DEV */
	int	errcode;
	char	*mntdev;
	char	*emnt;
	char	*mntpnt;
	char	*fstype;
	char	*options;
};

static struct mountentry *retry_list = NULL;

struct stringlist {
	struct stringlist *next;
	int   command_type;	/* MOUNT_DEV, SWAP_DEV */
	char   stringptr[2];
};

/* Local Globals */

static struct stringlist *umount_head = NULL;
static struct stringlist *umount_script_head = NULL;
static struct stringlist *unswap_head = NULL;

#define	NO_RETRY	0
#define	DO_RETRIES	1

static char *	rootmntdev;
static char *	rootrawdev;
static char *	rootrawdevs2;
static char	err_mount_dev[MAXPATHLEN];

/* internal prototypes */

int		gen_mount_script(FILE *, int);
void		gen_umount_script(FILE *);
int		umount_root(void);
void		gen_installboot(FILE *);
char *		get_failed_mntdev(void);

/* private prototypes */

static void	save_for_umount(char *, struct stringlist **, int);
static int	add_swap_dev(char *);
static int	mount_filesys(char *, char *, char *, char *, char *, int);
static void	free_retry_list(void);
static void	free_mountentry(struct mountentry *);
static void	save_for_swap_retry(char *, char *);
static void	save_for_mnt_retry(char *, char *, char *, char *);

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * Function:	mount_and_add_swap()
 * Description: Takes a slice name, which is the slice to be
 *		upgraded. Nothing is mounted when this funciton is called.
 *		First, mount the root. Then find the
 *		/etc/vfstab. Mount everything in the vfstab.
 * Parameters:
 *	diskname	-
 * Return:
 * Status:
 *	public
 */
int
mount_and_add_swap(char *diskname)
{
	char	pathbuf[MAXPATHLEN];
	int	status;
	char	mnt[50], fsckd[50];

	if (GetSimulation(SIM_SYSSOFT)) {
		if (profile_upgrade)
			(void) printf(dgettext("SUNW_INSTALL_LIBSVC",
			    "Can't mount if simulating disks"));
		return (ERR_MOUNT_FAIL);
	}
	free_retry_list();

	err_mount_dev[0] = 0;
	(void) sprintf(mnt, "/dev/dsk/%s", diskname);
	(void) sprintf(fsckd, "/dev/rdsk/%s", diskname);

	rootmntdev = xstrdup(mnt);
	rootrawdev = xstrdup(fsckd);
	rootrawdevs2 = xstrdup(fsckd);
	rootrawdevs2[strlen(rootrawdevs2) - 1] = '2';

	if (*get_rootdir() == '\0') {
		(void) strcpy(pathbuf, "/etc/vfstab");
	} else {
		(void) strcpy(pathbuf, get_rootdir());
		(void) strcat(pathbuf, "/etc/vfstab");
	}

	if ((status = mount_filesys(mnt, fsckd, "/", "ufs", "-", NO_RETRY))
	    != 0)
		return (status);

	if ((status = mount_and_add_swap_from_vfstab(pathbuf))) {
		return (status);
	}
	return (0);
}

/*
 * Function:	mount_and_add_swap_from_vfstab()
 * Description: Takes the path to a vfstab and mounts all ufs file systems
 *		and swaps.
 * Parameters:
 *	vfstab_path	-
 * Return:
 * Status:
 *	public
 */
int
mount_and_add_swap_from_vfstab(char *vfstab_path)
{
	struct stat	statbuf;
	FILE	*fp;
	char	buf[BUFSIZ + 1];
	char	*mntdev = NULL;
	char	*fsckdev, *mntpnt;
	char	*fstype, *fsckpass, *automnt, *mntopts;
	char	pathbuf[MAXPATHLEN];
	int	status;
	char	*cp, *str1;
	char	emnt[MAXPATHLEN], efsckd[50];
	char	options[MAXPATHLEN];
	int	all_have_failed;
	struct mountentry	*mntp, **mntpp;

	free_retry_list();

	if ((fp = fopen(vfstab_path, "r")) == (FILE *) NULL) {
		if (profile_upgrade)
			(void) printf(dgettext("SUNW_INSTALL_LIBSVC",
			    "Unable to open %s\n"), vfstab_path);
		(void) umount_root();
		return (ERR_OPENING_VFSTAB);
	}

	while (fgets(buf, BUFSIZ, fp)) {
		/* skip over leading white space */
		for (cp = buf; *cp != '\0' && isspace(*cp); cp++)
			;

		/* ignore comments, newlines and empty lines */
		if (*cp == '#' || *cp == '\n' || *cp == '\0')
			continue;

		cp[strlen(cp) - 1] = '\0';

		if (mntdev)
			free(mntdev);
		mntdev = (char *)xstrdup(cp);
		fsckdev = mntpnt = fstype = fsckpass = NULL;
		automnt = mntopts = NULL;
		for (cp = mntdev; *cp != '\0'; cp++) {
			if (isspace(*cp)) {
				*cp = '\0';
				for (cp++; isspace(*cp); cp++)
					;
				if (!fsckdev)
					fsckdev = cp;
				else if (!mntpnt)
					mntpnt = cp;
				else if (!fstype)
					fstype = cp;
				else if (!fsckpass)
					fsckpass = cp;
				else if (!automnt)
					automnt = cp;
				else if (!mntopts)
					mntopts = cp;
			}
		}

		if (mntdev == NULL || fsckdev == NULL || mntpnt == NULL ||
		    fstype == NULL || fsckpass == NULL || automnt == NULL ||
		    mntopts == NULL) {
			(void) fclose(fp);
			free(mntdev);
			if (profile_upgrade)
				(void) printf(dgettext("SUNW_INSTALL_LIBSVC",
				    "Error parsing vfstab\n"));
			return (ERR_MOUNT_FAIL);
		}

		/* if swap device, add it */
		if (strcmp(fstype, "swap") == 0) {
			(void) strcpy(err_mount_dev, mntdev);
			if ((status =
			    _map_to_effective_dev(mntdev, emnt)) != 0) {
				if (status != 2) {
					if (profile_upgrade)
						(void) printf(
							dgettext("SUNW_INSTALL_LIBSVC",
							    "Can't access device %s\n"),
							    mntdev);
					(void) fclose(fp);
					free(mntdev);
					return (ERR_MOUNT_FAIL);
				} else {
					if (*get_rootdir() == '\0')
						(void) strcpy(emnt, mntdev);
					else {
						(void) strcpy(emnt, get_rootdir());
						(void) strcat(emnt, mntdev);
					}
					status = stat(emnt, &statbuf);
					/*
					 *  If swap file isn't present,
					 *  it may be because the file
					 *  containing it hasn't been
					 *  mounted yet.  Save it for
					 *  later retry.
					 */
					if (status != 0)  {
						save_for_swap_retry(emnt,
						    mntdev);
						continue;
					} else if (!S_ISREG(statbuf.st_mode)) {
						if (profile_upgrade)
							(void) printf(dgettext("SUNW_INSTALL_LIBSVC", "Can't access device %s\n"), mntdev);
						(void) fclose(fp);
						free(mntdev);
						return (ERR_MOUNT_FAIL);
					}
				}
			}
			if ((status = add_swap_dev(emnt)) != 0) {
				(void) fclose(fp);
				free(mntdev);
				return (status);
			}
			err_mount_dev[0] = '\0';
			continue;
		}
		/* skip root device. it has already been mounted */
		if (strcmp(mntpnt, "/") == 0)
			continue;

		/* skip read-only devices */
		if (strcmp(mntopts, "-") != 0) {
			(void) strcpy(options, mntopts);
			str1 = options;
			while ((cp = strtok(str1, ",")) != NULL) {
				if (strcmp(cp, "ro") == 0)
					break;
				str1 = NULL;
			}
			if (cp != NULL)   /* ro appears in opt list */
				continue;
		}

		/* skip non-auto-mounted devices */
		if (strcmp(automnt, "yes") != 0 &&
				strcmp(mntpnt, "/usr") != 0 &&
				strcmp(mntpnt, "/usr/kvm") != 0 &&
				strcmp(mntpnt, "/var") != 0)
			continue;

		/*  mount ufs and s5 file systems */
		if (strcmp(fstype, "ufs") == 0 ||
		    strcmp(fstype, "s5") == 0) {
			if (_map_to_effective_dev(mntdev, emnt) != 0) {
				(void) strcpy(err_mount_dev, mntdev);
				if (profile_upgrade)
					(void) printf(dgettext("SUNW_INSTALL_LIBSVC",
					"Can't access device %s\n"), mntdev);
				(void) fclose(fp);
				free(mntdev);
				return (ERR_MOUNT_FAIL);
			}
			if (_map_to_effective_dev(fsckdev, efsckd) != 0) {
				(void) strcpy(err_mount_dev, fsckdev);
				if (profile_upgrade)
					(void) printf(dgettext("SUNW_INSTALL_LIBSVC",
					"Can't access device %s\n"), fsckdev);
				(void) fclose(fp);
				free(mntdev);
				return (ERR_MOUNT_FAIL);
			}
			if ((status = mount_filesys(emnt, efsckd, mntpnt,
			    fstype, mntopts, DO_RETRIES)) != 0) {
				(void) fclose(fp);
				free(mntdev);
				return (status);
			}
		}
	}
	if (mntdev)
		free(mntdev);
	mntdev = fsckdev = mntpnt = fstype = fsckpass = NULL;
	automnt = mntopts = NULL;

	(void) fclose(fp);

	/*
	 *  Process retry list.  Continue to process it until all operations
	 *  on list have been tried and have failed.
	 */

	all_have_failed = 0;
	while (retry_list && !all_have_failed) {
		all_have_failed = 1;  /* assume failure */
		mntpp = &retry_list;
		while (*mntpp != (struct mountentry *)NULL) {
			mntp = *mntpp;
			if (mntp->op_type == SWAP_DEV) {
				(void) strcpy(err_mount_dev, mntp->mntdev);
				status = stat(mntp->emnt, &statbuf);
				if (status == 0) {
					if (!S_ISREG(statbuf.st_mode)) {
						if (profile_upgrade)
							(void) printf(dgettext("SUNW_INSTALL_LIBSVC", "Can't access device %s\n"), mntp->mntdev);
						return (ERR_MOUNT_FAIL);
					}
					if ((status =
					    add_swap_dev(mntp->emnt)) != 0) {
						return (status);
					}
					err_mount_dev[0] = '\0';
					all_have_failed = 0;

					/* unlink and free retry entry */
					*mntpp = mntp->next;
					free_mountentry(mntp);
					mntp = NULL;
				} else
					mntpp = &(mntp->next);
			} else {   /* it's a mount request */
				(void) strcpy(err_mount_dev, mntp->mntdev);
				(void) sprintf(pathbuf,
				    "/sbin/mount -F %s %s %s %s >/dev/null 2>&1\n",
				    mntp->fstype, mntp->options, mntp->mntdev,
				    mntp->mntpnt);
				if ((status = system(pathbuf)) == 0) {
					err_mount_dev[0] = 0;
					save_for_umount(mntp->mntdev,
					    &umount_head, MOUNT_DEV);
					all_have_failed = 0;

					/* unlink and retry entry */
					*mntpp = mntp->next;
					free_mountentry(mntp);
					mntp = NULL;
				} else {
					mntp->errcode = WEXITSTATUS(status);
					mntpp = &(mntp->next);
				}
			}
		}
	}

	if (retry_list) {
		mntp = retry_list;
		(void) strcpy(err_mount_dev, mntp->mntdev);
		if (mntp->op_type == SWAP_DEV) {
			if (profile_upgrade && !GetSimulation(SIM_EXECUTE))
				(void) printf(dgettext("SUNW_INSTALL_LIBSVC",
				    "Can't access device %s\n"), mntp->mntdev);
			return (ERR_MOUNT_FAIL);
		} else {
			if (profile_upgrade && !GetSimulation(SIM_EXECUTE))
				(void) printf(dgettext("SUNW_INSTALL_LIBSVC",
					"Failure mounting %s, error = %d\n"),
					mntp->mntpnt, mntp->errcode);
			return (ERR_MOUNT_FAIL);
		}
	}

	return (0);
}

/*
 * mount_filesys()
 *
 * Parameters:
 *	mntdev	-
 *	fsckdev	-
 *	mntpnt	-
 *	fstype	-
 *	mntopts	-
 * Return:
 *
 * Status:
 *	public
 */
static int
mount_filesys(char * mntdev, char * fsckdev, char * mntpnt,
	char * fstype, char * mntopts, int retry)
{
	char			options[MAXPATHLEN];
	char			fsckoptions[30];
	char			cmd[MAXPATHLEN];
	char			basemount[MAXPATHLEN];
	int			status;
	int			cmdstatus;

	(void) strcpy(err_mount_dev, mntdev);

	if (strcmp(mntopts, "-") == 0)
		options[0] = '\0';
	else {
		(void) strcpy(options, "-o ");
		(void) strcat(options, mntopts);
	}

	if (*get_rootdir() == '\0') {
		(void) strcpy(basemount, mntpnt);
	} else {
		(void) strcpy(basemount, get_rootdir());
		if (strcmp(mntpnt, "/") != 0)
			(void) strcat(basemount, mntpnt);
	}

	/*
	 * fsck -m checks to see if file system
	 * needs checking.
	 *
	 * if return code = 0, disk is OK, can be mounted.
	 * if return code = 32, disk is dirty, must be fsck'd
	 * if return code = 33, disk is already mounted
	 *
	 * If the file system to be mounted is the true root,
	 * don't bother to do the fsck -m (since the results are
	 * unpredictable).  We know it must be mounted, so set
	 * the cmdstatus to 33.  This will drop us into the code
	 * that verifies that the EXPECTED file system is mounted
	 * as root.
	 */
	if (strcmp(basemount, "/") == 0) {
		cmdstatus = 33;
	} else {
		(void) sprintf(cmd, "/usr/sbin/fsck -m -F %s %s >/dev/null 2>&1\n",
			fstype, fsckdev);
		status = system(cmd);
		cmdstatus = WEXITSTATUS(status);
	}
	if (cmdstatus == 0) {
		(void) sprintf(cmd, "/sbin/mount -F %s %s %s %s >/dev/null 2>&1\n",
			fstype, options, mntdev, basemount);
		if ((status = system(cmd)) != 0) {
			if (retry == NO_RETRY) {
				if (profile_upgrade)
					(void) printf(dgettext("SUNW_INSTALL_LIBSVC",
					    "Failure mounting %s, error = %d\n"),
					    basemount, WEXITSTATUS(status));
				return (ERR_MOUNT_FAIL);
			} else {
				save_for_mnt_retry(basemount, fstype, options,
				    mntdev);
				err_mount_dev[0] = 0;
				return (0);
			}
		}
	} else if (cmdstatus == 32 || cmdstatus == 33) {
		dev_t	mntpt_dev;	/* device ID for mount point */
		dev_t	mntdev_dev;	/* dev ID for device */
		struct stat statbuf;

		/*
		 * This may mean the file system is already
		 * mounted. this needs to be checked.
		 */
		/* Get device ID for mount point */
		if (stat(basemount, &statbuf) != 0) {
			if (profile_upgrade)
				(void) printf(dgettext("SUNW_INSTALL_LIBSVC",
				    "Mount failure, cannot stat %s\n"),
				    basemount);
			return (ERR_MOUNT_FAIL);
		}
		mntpt_dev = statbuf.st_dev;

		/* Get device ID for mount device */
		if (stat(mntdev, &statbuf) != 0) {
			if (profile_upgrade)
				(void) printf(dgettext("SUNW_INSTALL_LIBSVC",
				    "Mount failure, cannot stat %s\n"),
				    mntdev);
			return (ERR_MOUNT_FAIL);
		}
		mntdev_dev = statbuf.st_rdev;

		if (mntpt_dev == mntdev_dev) {
			/*
			 * If the two devices are the same that means that
			 * the device is mounted and mounted correctly.
			 */
			return (0);
		} else {
			/*
			 * If these two devices are different that means
			 * that the file system is not mounted or mounted
			 * incorrectly. We need to check to see if the
			 * mount device is alreay mounted. If so that is an
			 * error and we'll returnt that fact.
			 */
			struct	ustat	ustatbuf;

			if (ustat(mntdev_dev, &ustatbuf) == 0) {
				/*
				 * ustat returns 0 if the device is mounted,
				 * which means that the device is not
				 * mounted were we wnat it.
				 */
				if (profile_upgrade)
					(void) printf(dgettext("SUNW_INSTALL_LIBSVC",
					    "%s not mounted at %s, \n"),
					    mntdev, basemount);
				return (ERR_MOUNT_FAIL);
			}

			if (strcmp(fstype, "ufs") == 0)
				(void) strcpy(fsckoptions, "-o p");
			else if (strcmp(fstype, "s5") == 0)
				(void) strcpy(fsckoptions,
					"-y -t /var/tmp/tmp$$ -D");
			else
				(void) strcpy(fsckoptions, "-y");

			if (profile_upgrade)
				(void) printf(dgettext("SUNW_INSTALL_LIBSVC",
				    "The %s file system (%s) is being checked.\n"),
				    mntpnt, fstype);
			(void) sprintf(cmd,
			    "/usr/sbin/fsck -F %s %s %s >/dev/null 2>&1\n",
			    fstype, fsckoptions, fsckdev);
			status = system(cmd);
			cmdstatus = WEXITSTATUS(status);
			if (cmdstatus != 0 && cmdstatus != 40) {
				if (profile_upgrade) {
					/* CSTYLE */
					(void) printf(
						dgettext("SUNW_INSTALL_LIBSVC",
						    "ERROR: unable to repair the %s file system.\n"),
						    mntpnt);
					/* CSTYLE */
					(void) printf(
						dgettext("SUNW_INSTALL_LIBSVC",
						    "Run fsck manually (fsck -F %s %s).\n"),
						    fstype, fsckdev);
				}
				return (ERR_MUST_MANUAL_FSCK);
			}
		}
		(void) sprintf(cmd,
		    "/sbin/mount -F %s %s %s %s >/dev/null 2>&1\n",
		    fstype, options, mntdev, basemount);
		if ((status = system(cmd)) != 0) {
			if (retry == NO_RETRY) {
				if (profile_upgrade)
					(void) printf(
						dgettext("SUNW_INSTALL_LIBSVC",
						    "Failure mounting %s, error = %d\n"),
					    basemount, WEXITSTATUS(status));
				return (ERR_MOUNT_FAIL);
			} else {
				save_for_mnt_retry(basemount, fstype, options,
				    mntdev);
				err_mount_dev[0] = 0;
				return (0);
			}
		}
	} else if (cmdstatus == 34) {
		if (profile_upgrade)
			(void) printf(dgettext("SUNW_INSTALL_LIBSVC",
			    "Mount failure, cannot stat  %s'\n"), mntdev);
		return (ERR_MOUNT_FAIL);
	} else {
		if (profile_upgrade)
			(void) printf(
				dgettext("SUNW_INSTALL_LIBSVC",
				    "Unrecognized failure %d from 'fsck -m -F %s %s'\n"),
				    cmdstatus, fstype, fsckdev);
		return (ERR_FSCK_FAILURE);
	}
	err_mount_dev[0] = 0;
	save_for_umount(mntdev, &umount_head, MOUNT_DEV);
	return (0);
}

/*
 * Function:	gen_mount_script
 * Description:
 * Scope:	public
 * Parameters:	script_fp
 *		do_root
 * Return:	ERR_OPEN_VFSTAB
 *		ERR_MOUNT_FAIL
 *		0		    no errors
 */
/*ARGSUSED0*/
int
gen_mount_script(FILE *script_fp, int do_root)
{
	FILE	*fp;
	char	buf[BUFSIZ + 1];
	char	*mntdev = NULL;
	char	*fsckdev, *mntpnt;
	char	*fstype, *fsckpass, *automnt, *mntopts;
	char	vfstabpath[MAXPATHLEN];
	char	*cp;
	char	options[MAXPATHLEN];
	char	fsckoptions[30];
	char	emnt[50], efsckd[50];

	if (*get_rootdir() == '\0') {
		(void) strcpy(vfstabpath, "/etc/vfstab");
	} else {
		(void) strcpy(vfstabpath, get_rootdir());
		(void) strcat(vfstabpath, "/etc/vfstab");
	}

	if ((fp = fopen(vfstabpath, "r")) == (FILE *) NULL) {
		(void) printf(dgettext("SUNW_INSTALL_LIBSVC",
				"Unable to open /a/etc/vfstab\n"));
		return (ERR_OPEN_VFSTAB);
	}

	while (fgets(buf, BUFSIZ, fp)) {
		/* skip over leading white space */
		for (cp = buf; *cp != '\0' && isspace(*cp); cp++)
			;

		/* ignore comments, newlines and empty lines */
		if (*cp == '#' || *cp == '\n' || *cp == '\0')
			continue;

		cp[strlen(cp) - 1] = '\0';

		if (mntdev)
			free(mntdev);
		mntdev = (char *)xstrdup(cp);
		fsckdev = mntpnt = fstype = fsckpass = NULL;
		automnt = mntopts = NULL;
		for (cp = mntdev; *cp != '\0'; cp++) {
			if (isspace(*cp)) {
				*cp = '\0';
				for (cp++; isspace(*cp); cp++)
					;
				if (!fsckdev)
					fsckdev = cp;
				else if (!mntpnt)
					mntpnt = cp;
				else if (!fstype)
					fstype = cp;
				else if (!fsckpass)
					fsckpass = cp;
				else if (!automnt)
					automnt = cp;
				else if (!mntopts)
					mntopts = cp;
			}
		}

		/* if swap device, add it */
		if (strcmp(fstype, "swap") == 0) {
			if (_map_to_effective_dev(mntdev, emnt) != 0) {
				if (profile_upgrade)
					(void) printf(dgettext(
						"SUNW_INSTALL_LIBSVC",
						    "Can't access device %s\n"),
					    mntdev);
				free(mntdev);
				(void) fclose(fp);
				return (ERR_MOUNT_FAIL);
			}
/*
			scriptwrite(script_fp, LEVEL0|CONTINUE, add_swap_cmd,
			    "MNTDEV", emnt, NULL);
*/
			save_for_umount(emnt, &umount_script_head, SWAP_DEV);
			continue;
		}

		/*  if root, only mount if do_root is set */
		if (strcmp(mntpnt, "/") == 0 && !do_root)
			continue;

		/*  mount ufs and s5 file systems */
		if (strcmp(fstype, "ufs") == 0 ||
		    strcmp(fstype, "s5") == 0) {
			if (_map_to_effective_dev(mntdev, emnt) != 0) {
				(void) strcpy(err_mount_dev, mntdev);
				if (profile_upgrade)
					(void) printf(dgettext(
						"SUNW_INSTALL_LIBSVC",
						    "Can't access device %s\n"),
					    mntdev);
				free(mntdev);
				(void) fclose(fp);
				return (ERR_MOUNT_FAIL);
			}
			if (_map_to_effective_dev(fsckdev, efsckd) != 0) {
				(void) strcpy(err_mount_dev, fsckdev);
				if (profile_upgrade)
					(void) printf(dgettext(
						"SUNW_INSTALL_LIBSVC",
						    "Can't access device %s\n"),
					    fsckdev);
				free(mntdev);
				(void) fclose(fp);
				return (ERR_MOUNT_FAIL);
			}
			if (strcmp(mntopts, "-") == 0) {
				(void) strcpy(options, "-o ");
				(void) strcat(options, mntopts);
			} else
				options[0] = '\0';

			if (strcmp(fstype, "ufs") == 0)
				(void) strcpy(fsckoptions, "-o p");
			else if (strcmp(fstype, "s5") == 0)
				(void) strcpy(fsckoptions,
					"-y -t /var/tmp/tmp$$ -D");
			else
				(void) strcpy(fsckoptions, "-y");

/*
			scriptwrite(script_fp, LEVEL0|CONTINUE, mount_fs_cmd,
			    "FSTYPE", fstype, "MNTDEV", emnt,
			    "MNTPNT", mntpnt, "FSCKDEV", efsckd, NULL);
*/
			save_for_umount(mntdev, &umount_script_head, MOUNT_DEV);
		}
	}
	if (mntdev)
		free(mntdev);
	(void) fclose(fp);
	return (0);
}

/*
 * gen_umount_script()
 *
 * Parameters:
 *	fp	-
 * Return:
 *	none
 * Status:
 *	public
 */
void
gen_umount_script(FILE * fp)
{
	struct stringlist *p, *op;

	p = umount_script_head;
	while (p) {
		if (p->command_type == MOUNT_DEV)
			scriptwrite(fp, LEVEL1, umount_cmd,
			    "MNTDEV", p->stringptr, NULL);
		else
			scriptwrite(fp, LEVEL1, del_swap_cmd,
			    "MNTDEV", p->stringptr, NULL);
		op = p;
		p = p->next;
		free(op);
	}
	umount_script_head = NULL;
}

/*
 * umount_and_delete_swap()
 * Parameters:
 *	none
 * Return:
 * Status:
 *	public
 */
int
umount_and_delete_swap(void)
{
	int	status;

	if ((status = umount_all()) != 0)
		return (status);
	return (unswap_all());
}

/*
 * umount_all()
 * Parameters:
 *	none
 * Return:
 * Status:
 *	public
 */
int
umount_all(void)
{
	struct stringlist	*p, *op;
	char			cmd[MAXPATHLEN];

	p = umount_head;
	while (p) {
		if (p->command_type == MOUNT_DEV) {
			(void) sprintf(cmd,
				"/sbin/umount %s >/dev/null 2>&1\n",
				p->stringptr);
			(void) system(cmd);
		}
		op = p;
		p = p->next;
		free(op);
	}
	umount_head = NULL;
	return (0);
}

/*
 * unswap_all
 * Parameters:
 *	none
 * Return:
 * Status:
 *	public
 */
int
unswap_all(void)
{
	struct stringlist	*p, *op;
	char			cmd[MAXPATHLEN];
	int			status;

	p = unswap_head;
	while (p) {
		if (p->command_type == SWAP_DEV) {
			(void) sprintf(cmd, "/usr/sbin/swap -d %s",
			    p->stringptr);
			if ((status = system(cmd)) != 0) {
				(void) printf(dgettext("SUNW_INSTALL_LIBSVC",
					"Error freeing swap, error = %x"),
					WEXITSTATUS(status));
				return (ERR_DELETE_SWAP);
			}
		}
		op = p;
		p = p->next;
		free(op);
	}
	unswap_head = NULL;
	return (0);
}

void
set_profile_upgrade(void)
{
	profile_upgrade = 1;
}

/* ******************************************************************** */
/*			LIBRARY SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * umount_root()
 * Parameters:
 *	none
 * Return:
 * Status:
 *	semi-private (internal library use only)
 */
int
umount_root(void)
{
	char	cmd[MAXPATHLEN];
	int	status;

	(void) sprintf(cmd, "/sbin/umount %s", rootmntdev);
	if ((status = system(cmd)) != 0) {
		(void) printf(dgettext("SUNW_INSTALL_LIBSVC",
		    "Error from umount, error = %x"),
		    WEXITSTATUS(status));
		return (ERR_UMOUNT_FAIL);
	}
	return (0);
}

void
gen_installboot(FILE * script_fp)
{
	if (rootrawdev == NULL)
		return;
	if (strcmp(get_default_inst(), "ppc") == 0)
		scriptwrite(script_fp, LEVEL0, copy_ppc_vof, NULL);
	scriptwrite(script_fp, LEVEL0, gen_installboot_cmd,
	    "RAWROOT", rootrawdev, "RAWROOTS2", rootrawdevs2, NULL);
}

char *
get_failed_mntdev(void)
{
	return (err_mount_dev);
}

/* ******************************************************************** */
/*			INTERNAL SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * save_for_umount()
 * Parameters:
 *	mntdev	-
 *	head	-
 *	type	-
 * Return:
 *	none
 * Status:
 *	private
 */
static void
save_for_umount(char * mntdev, struct stringlist ** head, int type)
{
	struct stringlist *p;

	p = (struct stringlist *) xmalloc((size_t) sizeof (struct stringlist) +
				strlen(mntdev));
	(void) strcpy(p->stringptr, mntdev);
	p->command_type = type;
	p->next = *head;
	*head = p;
}

static int
add_swap_dev(char *mntdev)
{

	char	cmd[MAXPATHLEN];
	int	status;

	(void) sprintf(cmd,
	    "(/usr/sbin/swap -l 2>&1) | /bin/grep %s >/dev/null 2>&1", mntdev);
	if ((status = system(cmd)) != 0) {
		/* swap not already added */
		(void) sprintf(cmd, "/usr/sbin/swap -a %s", mntdev);
		if ((status = system(cmd)) != 0) {
			if (profile_upgrade)
				(void) printf(dgettext("SUNW_INSTALL_LIBSVC",
				    "Error adding swap, error = %x\n"),
				    WEXITSTATUS(status));
			return (ERR_ADD_SWAP);
		}
	}
	save_for_umount(mntdev, &unswap_head, SWAP_DEV);
	return (0);
}

static void
save_for_swap_retry(char *emnt, char *mntdev)
{
	struct mountentry	*m, *p;

	m = (struct mountentry *) xcalloc((size_t) sizeof (struct mountentry));
	m->op_type = SWAP_DEV;
	m->mntdev = xstrdup(mntdev);
	m->emnt = xstrdup(emnt);
	m->next = NULL;

	/* queue it to the retry list */
	if (retry_list == NULL)
		retry_list = m;
	else {
		p = retry_list;
		while (p->next != NULL)
			p = p->next;
		p->next = m;
	}
}

static void
save_for_mnt_retry(char *basemount, char *fstype, char *options, char *mntdev)
{
	struct mountentry	*m, *p;

	m = (struct mountentry *) xcalloc((size_t) sizeof (struct mountentry));
	m->op_type = MOUNT_DEV;
	m->mntpnt = xstrdup(basemount);
	m->mntdev = xstrdup(mntdev);
	m->fstype = xstrdup(fstype);
	m->options = xstrdup(options);
	m->next = NULL;

	/* queue it to the retry list */
	if (retry_list == NULL)
		retry_list = m;
	else {
		p = retry_list;
		while (p->next != NULL)
			p = p->next;
		p->next = m;
	}
}

static void
free_retry_list()
{
	struct mountentry *mntp, *next;

	mntp = retry_list;
	retry_list = NULL;
	while (mntp != NULL) {
		next = mntp->next;
		free_mountentry(mntp);
		mntp = next;
	}
}

static void
free_mountentry(struct mountentry *mntp)
{
	if (mntp->mntdev)
		free(mntp->mntdev);
	if (mntp->emnt)
		free(mntp->emnt);
	if (mntp->mntpnt)
		free(mntp->mntpnt);
	if (mntp->fstype)
		free(mntp->fstype);
	if (mntp->options)
		free(mntp->options);
	free(mntp);
}
