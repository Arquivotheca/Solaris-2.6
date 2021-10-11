#ifndef lint
#pragma ident "@(#)store_boot.c 1.22 96/09/27 SMI"
#endif

/*
 * Copyright (c) 1995-1996 by Sun Microsystems, Inc. All rights reserved.
 */
/*
 * Module:	store_boot.c
 * Group:	libspmistore
 * Description: This module contains functions which get (and
 *		eventually set) data about the default firmware
 *		specified disk device.
 */

#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <libgen.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/dkio.h>
#include <sys/fcntl.h>
#include <sys/openpromio.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/vtoc.h>
#include <device_info.h>
#include "spmistore_lib.h"
#include "spmicommon_api.h"

/* private functions */

static int 		_valid_boot_disk(const char *);
static char *		_ddi_get_bootdev(void);

/* ---------------------- public functions ----------------------- */

/*
 * Function:	DiskobjFindBoot
 * Description:	Search the disk object list and find the disk with the disk
 *		name matching the disk name in the boot object for the
 *		state specified. This routine will not evaluate the
 *		availability or selection status of the associated disk
 *		when returning the disk object pointer.
 * Scope:	public
 * Parameters:	state	[RO] (Label_t)
 *			Identify boot object state from which to retrieve
 *			configuration information. Valid values are:
 *			    CFG_CURRENT		current boot state info
 *			    CFG_COMMIT		committed boot state info
 *			    CFG_EXIST		existing boot state info
 *		diskp	[RO, *RW] (Disk_t **)
 *			Address of disk object pointer used to retrieve pointer
 *			to the boot disk. Set to NULL if no disk object is
 *			found in the disk object list correponding to the boot
 *			object disk value in the specified state, of if the
 *			boot object disk name is undefined.
 * Return:	D_OK	  disk object successfully found
 * 		D_BADARG  invalid argument
 *		D_FAILED  disk object not found
 */
int
DiskobjFindBoot(Label_t state, Disk_t **diskp)
{
	char	    disk[32] = "";
	Disk_t *    dp;

	/* validate parameters */
	if (state != CFG_CURRENT && state != CFG_COMMIT && state != CFG_EXIST)
		return (D_BADARG);

	if (diskp == NULL)
		return (D_BADARG);

	(void) BootobjGetAttribute(state, BOOTOBJ_DISK, disk, NULL);

	WALK_DISK_LIST(dp) {
		if (streq(disk_name(dp), disk))
			break;
	}

	if (dp == NULL) {
		*diskp = NULL;
		return (D_FAILED);
	} else {
		*diskp = dp;
		return (D_OK);
	}
}

/* ---------------------- internal functions ----------------------- */

/*
 * Function:	BootDefault
 * Description:	Retrieve the default boot disk and device.
 * Scope:	internal
 * Parameters:	diskp	[RO, *RW, **RW] (char **)
 *			Address of character pointer used to retrieve boot
 *			disk name.
 *		devp	[RO, *RW] (int *)
 *			Address of integer used to retrieve device index.
 * Return:	none
 */
void
BootDefault(char **diskp, int *devp)
{
	static char	disk[32];
	Mntpnt_t	info;
	Disk_t *  	dp;
	char *	  	dev = NULL;
	char *	  	cp;
	int	  	pid;

	if (diskp != NULL)
		*diskp = NULL;

	if (devp != NULL)
		*devp = NULL;

	if (diskp == NULL || devp == NULL)
		return;

	if (first_disk() == NULL)
		return;

	/* look for the ENV variable first */
	if ((dev = getenv("SYS_BOOTDEVICE")) == NULL) {
		dev = _ddi_get_bootdev();
	}

	/*
	 * For simulations which haven't been resolved:
	 * (1) SPARC, use the slice with the first "/" file system found in
	 *	the disk list.
	 * (2) Intel, the Solaris partition containing the first "/" file
	 *	system found in the disk list.
	 * (3) PPC, the DOS partition on the disk containing the first "/"
	 *	file system found in the disk list.
	 */
	if (dev == NULL && GetSimulation(SIM_SYSDISK)) {
		WALK_DISK_LIST(dp) {
			if (find_mnt_pnt(dp, NULL, ROOT, &info, CFG_EXIST))
				break;
		}

		if (dp != NULL) {
			if (IsIsa("sparc")) {
				dev = make_slice_name(disk_name(dp),
					info.slice);
			} else if (IsIsa("i386")) {
				pid = get_solaris_part(dp, CFG_EXIST);
				dev = make_device_name(disk_name(dp), pid);
			} else if (IsIsa("ppc")) {
				WALK_PARTITIONS(pid) {
					if (part_id(dp, pid) == DOSOS12 ||
						    part_id(dp, pid) ==
							DOSOS16) {
						dev = make_device_name(
							disk_name(dp), pid);
						break;
					}
				}
			}
		}
	}

	disk[0] = '\0';
	if (dev == NULL) {
		*diskp = NULL;
		*devp = -1;
	} else if (is_disk_name(dev)) {
		(void) strcpy(disk, dev);
		*diskp = &disk[0];
		*devp = -1;
	} else if (IsIsa("sparc") && is_slice_name(dev) &&
			(cp = strrchr(dev, 's')) != NULL) {
		*cp = NULL;
		(void) strcpy(disk, dev);
		*diskp = &disk[0];
		*devp = atoi(++cp);
	} else if ((IsIsa("ppc") || IsIsa("i386")) && is_part_name(dev) &&
			(cp = strrchr(dev, 'p')) != NULL) {
		*cp = NULL;
		(void) strcpy(disk, dev);
		*diskp = &disk[0];
		*devp = atoi(++cp);
		/*
		 * if the returned device was p0, the firmware did not
		 * have an explicit partition configured and is relying
		 * on the current configuration of the fdisk table instead
		 */
		if (*devp == 0)
			*devp = -1;
	} else {
		*diskp = NULL;
		*devp = -1;
	}
}

/* ---------------------- private functions ----------------------- */

/*
 * Function:	_valid_boot_disk
 * Description:	Determine whether the input parameter is a valid disk.
 *		The following must be true in order to be a valid disk:
 *		- it is of the form:
 *			/dev/dsk/c[0-9][t[0-9]]d[0-9]{s[0-9]]|p[0-3]
 *		- it is openable (the device exists)
 *		- it is not a CD
 *
 * Scope:	private
 * Parameters:	boot_device [RO, *RO] (char *)
 * Return:	0	- parameter is not a valid boot disk
 *		1	- parameter is a valid boot disk
 */
static int
_valid_boot_disk(const char *boot_device)
{
	char 		*dev_name;
	char 		*dev_path;
	int  		ret_val;
	struct dk_cinfo	dkc;
	char		buf[MAXNAMELEN];
	char		dev_buf[MAXNAMELEN];
	int		n;
	int		fd;

	if (!boot_device || *boot_device == '\0')
		return (0);

	ret_val = 0;
	(void) strcpy(dev_buf, boot_device);
	dev_name = basename(dev_buf);
	dev_path = dirname(dev_buf);

	/* the device must be in /dev/dsk and be in the correct format */
	if (streq(dev_path, "/dev/dsk") &&
		(is_slice_name(dev_name) || is_part_name(dev_name))) {

		/*
		 * the ioctl() used to check to see if
		 * the device is a cdrom must be run on
		 * the raw device
		 */
		(void) sprintf(buf,
			"/dev/rdsk/%s", dev_name);
		if ((fd = open(buf, O_RDONLY)) >= 0) {
			n = ioctl(fd, DKIOCINFO, &dkc);
			(void) close(fd);
			if (n == 0 && dkc.dki_ctype !=
					DKC_CDROM) {
				ret_val = 1;
			}
		}
	}

	return (ret_val);
}

/*
 * Function:	_ddi_get_bootdev
 * Description:	Retrieve the disk boot device information using the DDI
 *		interfaces for accessing the PROM configuration variable.
 * Scope:	private
 * Parameters:	none
 * Return:	NULL	- unable to access a boot device
 *		char *	- pointer to local buffer with disk boot device
 */
/* 
 * To get around a problem with dbx and libthread, define NODEVINFO
 * to 'comment out' code references to functions in libdevinfo,
 * which is threaded.
 */
static char *
_ddi_get_bootdev(void)
{
	static char bootdev_str[MAXPATHLEN];
	struct boot_dev **boot_devices;
	struct boot_dev **boot_devices_orig;
	char *int_boot_dev_str;
	char **trans_list;
	int  dev_found;

	/* if this is dryrun then return NULL */
	if (GetSimulation(SIM_SYSDISK))
		return (NULL);

	dev_found = 0;
	/* Retrieve the list of boot device values */
#ifndef NODEVINFO
	if (devfs_bootdev_get_list("/", &boot_devices) == SUCCESS) {
		boot_devices_orig = boot_devices;
		/*
		 * For each boot device entry a list of resolvable
		 * /dev device translations are returned - scan
		 * the lists for the first viable candidate
		 */
		while (*boot_devices && !dev_found) {
			trans_list = (*boot_devices)->bootdev_trans;
			while (*trans_list && !dev_found) {
				int_boot_dev_str = *trans_list;
				if (_valid_boot_disk(int_boot_dev_str)) {
					(void) strcpy(bootdev_str,
						int_boot_dev_str);
					dev_found = 1;
				}
				trans_list++;
			}
			boot_devices++;
		}
		/* free the space allocated by devfs_bootdev_get_list */
		devfs_bootdev_free_list(boot_devices_orig);
	}
#endif

	if (dev_found) {
		return (basename(bootdev_str));
	} else {
		return (NULL);
	}
}
