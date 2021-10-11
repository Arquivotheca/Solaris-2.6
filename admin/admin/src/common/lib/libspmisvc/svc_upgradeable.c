#ifndef lint
#pragma ident "@(#)svc_upgradeable.c 1.18 96/09/26 SMI"
#endif

/*
 * Copyright (c) 1995-1996 by Sun Microsystems, Inc. All rights reserved.
 */
/*
 * Module:	svc_upgradeable.c
 * Group:	libspmisvc
 * Description:	This module contains functions which are used to
 *		assess disks which are in an upgradeable condition.
 */

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/fstyp.h>
#include <sys/fsid.h>
#include <sys/mnttab.h>
#include <sys/mntent.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/fs/ufs_fs.h>
#include "spmisvc_lib.h"
#include "spmistore_api.h"
#include "spmisoft_lib.h"
#include "spmicommon_api.h"

#ifndef	MODULE_TEST
/* public prototypes */

void		SliceFindUpgradeable(StringList **, StringList **);

/* private prototypes */

static char *	UfsCheckSeparateVar(void);
static int	UfsIsUpgradeable(char *, char *, char *);
static int	InstanceIsUpgradeable(char *);
static int	FsCheckUpgradeability(char *);
static int	MediaIsUpgradeable(char *);
#endif

/* ---------------------- Test Interface ----------------------- */
#ifdef	MODULE_TEST
main(int argc, char **argv, char **env)
{
	Disk_t *	list = NULL;
	Disk_t *	dp;
	StringList *	slices = NULL;
	StringList *	releases = NULL;
	StringList *	sp;
	int		n;
	char *		file = NULL;
	char *		rootmount = "/a";

	while ((n = getopt(argc, argv, "x:udL")) != -1) {
		switch (n) {
		case 'd':
			(void) SetSimulation(SIM_SYSDISK, 1);
			file = strdup(optarg);
			(void) printf("Using %s as an input file\n", file);
			break;
		case 'x':
			(void) set_trace_level(atoi(optarg));
			break;
		case 'L':
			rootmount = "/";
			break;
		default:
			(void) fprintf(stderr,
		"Usage: %s [-x <level>] [-u] [-L] [-d <disk file>]\n",
				argv[0]);
			exit(1);
		}
	}

	(void) set_rootdir(rootmount);
	/* initialize the disk list only for non-direct runs */
	if (!streq(rootmount, "/")) {
		n = DiskobjInitList(file);
		(void) printf("Disks found - %d\n", n);
	}

	SliceFindUpgradeable(&slices, &releases);
	if (slices != NULL) {
		(void) printf("Upgradeable slices:\n");
		WALK_LIST(sp, slices)
			(void) printf("\t%s\n", sp->string_ptr);

		(void) printf("Upgradeable releases:\n");
		WALK_LIST(sp, releases)
			(void) printf("\t%s\n", sp->string_ptr);

		(void) StringListFree(slices);
		(void) StringListFree(releases);
	} else {
		(void) printf("No upgradeable slices.\n");
	}

	exit(0);
}
#else
/* ---------------------- public functions ----------------------- */

/*
 * Function:	SliceFindUpgradeable
 * Description: This function operates either directly or indirectly:
 *
 *		INDIRECT
 *		--------
 *		Users must have called DiskobjInitList() to create the primary
 *		disk object list before calling this function. This assumes
 *		the primary disks are not currently mounted (i.e. indirect
 *		installation). Search all valid slices on all disks in the
 *		primary disk object list with legal sdisk configs for those
 *		containing a "/" file system in a condition suitable for
 *		upgrading to the current Solaris release. For each slice which
 *		is deemed upgradeable to the current release, add an entry
 *		in the StringList linked list, the head of which is returned
 *		by the call.
 *
 * 		NOTE:	The get_rootdir() directory is used during processing;
 *			any file systems mounted on that directory will be
 *			automatically unmounted.
 *
 *		DIRECT
 *		------
 *		This function is assumed to be running on a live system
 *		(upgrade dry-run) and all file systems are assumed to be
 *		mounted. In this case, get_rootdir() will return "/".
 *		No mounting or unmounting of file systems should occur
 *		when operating in this mode.
 *
 * Scope:	public
 * Parameters:	slices		[RO, *RW, **RW] (StringList **)
 *			Address of a StringList pointer used to retrieve
 *			the head of a linked list of upgradeable slices.
 *			If NULL, no slice data is to be returned.
 *		releases	[RO, *RW, **RW] (StringList **)
 *			Address of a StringList pointer used to retrieve
 *			the head of a linked list of releases positionaly
 *			corresponding to the slice in the slices list.
 *			If NULL, no release data is to be returned.
 * Return:	 none
 */
void
SliceFindUpgradeable(StringList **slices, StringList **releases)
{
	Disk_t *	dp;
	char *		slice;
	char		release[256] = "";
	int		s;

	/* initialize return values */
	if (slices != NULL)
		*slices = NULL;

	if (releases != NULL)
		*releases = NULL;

	/* always return TRUE for disk and execution simulations */
	if (GetSimulation(SIM_EXECUTE) || GetSimulation(SIM_SYSDISK)) {
		if (releases != NULL)
			(void) StringListAdd(releases, "");

		if (slices != NULL)
			(void) StringListAdd(slices, "");
	} else if (DIRECT_INSTALL && FsCheckUpgradeability(release) == 0) {
		/*
		 * if this is a direct install and there is an upgradeable
		 * release, return the release and the slice corresponding
		 * to the currently mounted '/'
		 */
		if (releases != NULL)
			(void) StringListAdd(releases, release);

		if (slices != NULL) {
			struct mnttab	mnt;
			struct mnttab	mpref;
			FILE *		mntp;
			char *		cp;

			if ((mntp = fopen(MNTTAB, "r")) != NULL) {
				mpref.mnt_mountp = strdup("/");
				mpref.mnt_special = NULL;
				mpref.mnt_fstype = NULL;
				mpref.mnt_mntopts = NULL;
				mpref.mnt_time = NULL;
				if (getmntany(mntp, &mnt, &mpref) == 0 &&
						(cp = strrchr(mnt.mnt_special,
							'/')) != NULL) {
					(void) StringListAdd(slices, ++cp);
				}

				(void) fclose(mntp);
			}
		}
	} else {
		/*
		 * process all disks with legal sdisk configurations looking
		 * for at least one slice on the disk which contains a "/"
		 * file system which is in an upgradeable condition.
		 */
		WALK_DISK_LIST(dp) {
			if (!disk_okay(dp) || sdisk_geom_null(dp) ||
					!sdisk_legal(dp))
				continue;

			/*
			 * walk all slices looking for an unlocked slice which
			 * has "/" as it's existing mount point. Check to see
			 * if the slice is upgradeable, and if it is, add it
			 * to the list.
			 */
			WALK_SLICES(s) {
				release[0] = '\0';
				/* check slice viability and upgradeability */
				slice = make_slice_name(disk_name(dp), s);
				if (!slice_locked(dp, s) &&
					    streq(orig_slice_mntpnt(dp, s),
						ROOT) &&
					    UfsIsUpgradeable(slice, NULL,
						release)) {
					if (slices != NULL)
						(void) StringListAdd(slices,
						    slice);

					if (releases != NULL)
						(void) StringListAdd(releases,
						    release);
				}
			}
		}
	}
}

/* ---------------------- private functions ----------------------- */

/*
 * Function:	UfsCheckSeparateVar
 * Description: Check the /etc/vfstab file under the current get_rootdir() and
 *		see if it contains a separate active "/var" file system mount
 *		entry. Return the block special device if there is one.
 * Scope:	private
 * Parameters:	none
 * Return:	NULL	- no separate /var entry found
 *		!NULL	- pointer to string containing block special
 *			  device for the "/var" entry
 */
static char *
UfsCheckSeparateVar(void)
{
	static char	emnt[64] = "";
	char		buf[256];
	FILE *		fp;
	char *		pdev;
	char *		pfs;

	(void) sprintf(buf, "%s%s", get_rootdir(), VFSTAB);
	if ((fp = fopen(buf, "r")) != NULL) {
		while (fgets(buf, 255, fp) != NULL) {
			if (((pdev = strtok(buf, " \t")) != NULL) &&
				    is_pathname(pdev) &&
				    (strtok(NULL, " \t") != NULL) &&
				    ((pfs = strtok(NULL, " \t")) != NULL) &&
				    streq(pfs, VAR)) {
				(void) _map_to_effective_dev(pdev, emnt);
				break;
			}
		}

		(void) fclose(fp);
	}

	if (emnt[0] == '\0')
		return (NULL);

	return (emnt);
}

/*
 * Function:	UfsIsUpgradeable
 * Description: Mount a slice which represents a UFS "/" file system, and check
 *		to see if it has a "/var" directory configured for upgrading.
 *		The slice is mounted on get_rootdir(). If "/var" is a separate
 *		file system, it is mounted before the verification is performed.
 *
 *		If the slice device specified is a simple slice name (e.g.
 *		c0t3d0s3), the block special device used for mounting is
 *		assumed to exist in /dev/dsk. Otherwise, the user supplied
 *		fully qualified path name is used.
 *
 *		NOTE:	Any file system mounted on get_rootdir() at the time
 *			this function is invoked will automatically be unmounted
 * Scope:	private
 * Parameters:	bdevice	[RO, *RO]
 *			Slice device name for which the block device will be
 *			used for the validation. The device may either be
 *			specified in relative (e.g. c0t3d0s4) or absolute (e.g.
 *			/dev/dsk/c0t3d0s4) form.
 *		cdevice	[RO, *RO] (optional)
 *			Absolute path name for the character device to be used
 *			for restoring the last-mounted-on name for the file
 *			system. This is only specified if the block device is
 *			specified as an absolute path name; otherwise, the value
 *			should be passed as NULL.
 *		release	[RO, *RW] (char *)
 *			Pointer to a character buffer of size 32 used to
 *			retrieve the name of the release associated with the
 *			upgradeable slice. NULL if this information is not
 *			requested. Set to "" if the information is requested
 *			but is not available.
 * Return:	1	- the file system is upgradeable
 *		0	- the file system is not upgradeable
 */
static int
UfsIsUpgradeable(char *bdevice, char *cdevice, char *release)
{
	char *		vardev = NULL;
	char 		var[MAXPATHLEN] = "";
	int		okay = 0;

	/* validate parameters */
	if (is_slice_name(bdevice)) {
		if (cdevice != NULL)
			return (0);
	} else if (is_pathname(bdevice)) {
		if (!is_pathname(cdevice))
			return (0);
	} else {
		return (-1);
	}

	if (get_trace_level() > 5)
		write_status(LOGSCR, LEVEL0,
			"Checking upgradeability for %s\n", bdevice);

	/* always return TRUE for disk and execution simulations */
	if (GetSimulation(SIM_EXECUTE) || GetSimulation(SIM_SYSDISK))
		return (1);

	/* make sure the assembly mount point is cleared */
	if (DirUmountAll(get_rootdir()) < 0)
		return (0);

	/* try to mount the root file system on the get_rootdir() directory */
	if (UfsMount(bdevice, get_rootdir(), "-r") < 0)
		return (0);

	/* if there is a separate /var file system, mount it */
	if ((vardev = UfsCheckSeparateVar()) != NULL) {
		/* make sure there is a /var directory for mounting */
		(void) sprintf(var, "%s%s", get_rootdir(), VAR);
		if (UfsMount(vardev, var, "-r") < 0) {
			(void) UfsUmount(bdevice, ROOT, cdevice);
			return (0);
		}
	}

	if (FsCheckUpgradeability(release) == 0)
		okay = 1;

	/* unmount /var if it is a separate file system */
	if (vardev != NULL)
		(void) UfsUmount(vardev, NULL, NULL);

	/* unmount "/" */
	(void) UfsUmount(bdevice, ROOT, cdevice);
	return (okay);
}

/*
 * Function:	InstanceIsUpgradeable
 * Description:	Check to see if there is an INST_RELEASE file relative to
 *		get_rootdir(), and if there is, check to see if the version is
 *		considered acceptable for upgrading. Requirements for
 *		upgradeability are:
 *
 *		(1) SPARC	- all versions after 2.0
 *		(2) Intel	- all versions 2.4 and later, except
 *				  for 2.4 and 2.5 system with a revision 100
 *				  (Core)
 *		(3) PowerPC	- all versions
 * Scope:	private
 * Parameters:	release	[RO, *RW] (char *)
 *			Pointer to a 32 character buffer used to retrieve the
 *			name of the release instance. NULL if this information
 *			is not requested. Set to "" if this information is
 *			requested, but is not available.
 * Return:	1	- the current release is upgradeable
 *		0	- the current release is not upgradeable
 */
static int
InstanceIsUpgradeable(char *release)
{
	char	line[32];
	FILE *	fp;
	int	upgradeable = 0;
	int	minor;

	if (release != NULL)
		release[0] = '\0';

	if ((fp = fopen(INST_RELEASE_read_path(""), "r")) != NULL) {
		if (fgets(line, sizeof (line), fp) != NULL &&
				strneq(line, "OS=Solaris", 10) &&
				fgets(line, sizeof (line), fp) != NULL &&
				strneq(line, "VERSION=", 8) &&
				isdigit(line[10])) {
			/* clear out the newline */
			line[strlen(line) - 1] = '\0';

			/* don't allow downgrades (system release > media) */
			if (!MediaIsUpgradeable(&line[8])) {
				(void) fclose(fp);
				return (0);
			}

			/* return the release version if requested */
			if (release != NULL)
				(void) strcpy(release, &line[8]);

			/* extract the minor version number */
			minor = atoi(&line[10]);
			if (IsIsa("sparc")) {
				if (minor > 0)
					upgradeable = 1;
			} else if (IsIsa("ppc")) {
				upgradeable = 1;
			} else if (IsIsa("i386")) {
				/*
				 * all Intel releases > 2.3 are upgradeable
				 * except for those with REV=100 (Solaris Base)
				 */
				if (minor > 3) {
					if (fgets(line, sizeof (line),
					    fp) == NULL ||
					    !strneq(line, "REV=", 4) ||
					    !isdigit(line[5]) ||
					    atoi(&line[4]) != 100)
						upgradeable = 1;
				}
			}
		}

		(void) fclose(fp);
	}

	/* clear release value if not upgradeable */
	if ((upgradeable == 0) && (release != NULL))
		release[0] = '\0';

	return (upgradeable);
}

/*
 * Function:	FsCheckUpgradeability
 * Description:	Check to see that the clustertoc is readable and the
 *		instance is considered to be upgradeable.
 * Scope:	private
 * Parameters:	release	[RO, *RW] (char *)
 *			Pointer to a character buffer of size 32 used to
 *			retrieve the name of the release associated with the
 *			upgradeable slice. NULL if this information is not
 *			requested. Set to "" if the information is requested
 * Return:	 0	Upgradeability checks passed
 *		-1	Upgradeability checks failed
 */
static int
FsCheckUpgradeability(char *release)
{
	/* check the upgradeability criteria */
	if (access(clustertoc_read_path(""), F_OK) == 0 &&
			access(CLUSTER_read_path(""), F_OK) == 0 &&
			InstanceIsUpgradeable(release) &&
			UsrpackagesExist()) {
		return (0);
	}

	return (-1);
}

/*
 * Function:	MediaIsUpgradeable
 * Description:	Boolean function used to assess if the current release
 *		of the system is considered upgradeable in the context
 *		of current system release.
 * Scope:	private
 * Parameters:	sysver [RO, *RO] (char *)
 *			Non-NULL string containing the release of the current
 *			machine (e.g. "2.5").
 * Return:	0	The media is not upgradeable from the current
 *			system configuration.
 *		1	The media is upgradeble.
 */
static int
MediaIsUpgradeable(char *sysver)
{
	Module *    mod;
	char *	    mediaver = NULL;
	int	    status;
	char	    sysprod_ver[256], mediaprod_ver[256];

	/*
	 * validate parameters - if the system version is not specified,
	 * then we assume no upgradeability because we don't have
	 * enough information to assess upgradeability (error
	 * conservatively)
	 */
	if (sysver == NULL)
		return (0);

	for (mod = get_media_head(); mod != NULL; mod = mod->next) {
		if (mod->info.media->med_type != INSTALLED_SVC &&
			    mod->info.media->med_type != INSTALLED &&
			    mod->sub->type == PRODUCT &&
			    streq(mod->sub->info.prod->p_name, "Solaris"))
			mediaver = mod->sub->info.prod->p_version;
	}

	/*
	 * compare media versions (if available media) for a constraint
	 */
	if (mediaver != NULL) {
		(void) strcpy(sysprod_ver, "Solaris_");
		(void) strcat(sysprod_ver, sysver);
		(void) strcpy(mediaprod_ver, "Solaris_");
		(void) strcat(mediaprod_ver, mediaver);
		status = prod_vcmp(sysprod_ver, mediaprod_ver);
		if (status == V_GREATER_THEN || status == V_NOT_UPGRADEABLE)
			return (0);
	}

	return (1);
}
#endif /* MODULE_TEST */
