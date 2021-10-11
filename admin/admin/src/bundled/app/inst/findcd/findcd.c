#ifndef lint
#ident   "@(#)findcd.c 1.6 94/05/20 SMI"
#endif				/* lint */
/*
 * Copyright (c) 1991 Sun Microsystems, Inc.  All Rights Reserved. Sun
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

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/dkio.h>
#include <sys/vtoc.h>
#include <sys/param.h>

/* Local prototypes */

static int	is_distribution_cd(char *);
static int	slice_name(char *, int);

/* Local Globals */

static char	*directory = "/dev/rdsk";
static char	*bdir = "/dev/dsk";

/* Useful parsing macros */

#define	must_be(s, c)	if (*s++ != c) return (0)
#define	skip_digits(s)	while (isdigit(*s)) s++

/*
 * This program looks for cd drives with a solaris 2.x install cd in it.
 * If found, it returns the (solaris style) name of the BLOCK CD device
 * slice 0. It is up to the invoker (stubboot) to find the correct boot
 * slice.
 *
 * needs libadm - read_vtoc()
 */
main()
{
	DIR		*dir;
	struct dirent	*dp;

	/*
	 * change directory to the device directory for efficient
	 * access
	 */
	if (chdir(directory) == -1) {
		(void) fprintf(stderr, "Cannot set directory to %s\n",
			directory);
		exit(1);
	}

	/* before doing all the work, check for cd at standard name */
	if (is_distribution_cd("c0t6d0s0") == 1) {
		(void) printf("%s/c0t6d0s0\n", bdir);
		exit(0);
	}

	/* Open the device directory */
	if ((dir = opendir(".")) == NULL) {
		(void) fprintf(stderr, "Cannot open %s\n", directory);
		exit(1);
	}

	/* Find all nodes with the standard device naming conventions */
	while ((dp = readdir(dir)) != NULL) {
		if (dp->d_name[0] == '.')
			continue;

		/* we look for slice 0 only - it (may) have the slice map */
		if (slice_name(dp->d_name, 0)) {
			if (is_distribution_cd(dp->d_name) == 1) {
				(void) printf("%s/%s\n", bdir, dp->d_name);
				(void) closedir(dir);
				exit(0);
			}
		}
	}

	(void) closedir(dir);
	exit(1);
}

/*
 * is_distribution_cd()
 *	Check the device 'pathname' to see if it is a CDROM device
 *	with an installation media loaded. The installation media
 *	will have an ASCII label with the string "Install" in it.
 * Parameters:
 *	pathname - device name to search (e.g. c0t6d0s0)
 * Return:
 *	1	- the device IS an installation distribution CD
 *	0	- the device is NOT an installation distribution CD
 */
static int
is_distribution_cd(char *pathname)
{
	struct dk_cinfo	dkcinfo;
	struct vtoc	vtoc;
	struct stat	stbuf;
	struct stat	*st = &stbuf;
	int		fd;

#ifdef DEBUG
	(void) printf("is_distribution_cd: %s\n", pathname);
#endif DEBUG

	/* Attempt to open the disk.  If it fails, skip it.  */
	if ((fd = open(pathname, O_RDONLY | O_NDELAY)) < 0)
		return (0);

	/* Must be a character device */
	if (fstat(fd, st) == -1 || !S_ISCHR(st->st_mode)) {
		(void) close(fd);
		return (0);
	}
	/*
	 * Attempt to read the configuration info on the disk.
	 * If it fails, we assume the disk's not there.
	 */
	if (ioctl(fd, DKIOCINFO, &dkcinfo) < 0) {
		(void) close(fd);
		return (0);
	}

	/* Have we found a CD-ROM? */
	if (dkcinfo.dki_ctype == DKC_CDROM) {
#ifdef DEBUG
		(void) printf("cdrom: %s\n", pathname);
#endif DEBUG

		/* read_vtoc here while we have disk open */
		if (read_vtoc(fd, &vtoc) < 0 || vtoc.v_sanity != VTOC_SANE) {
#ifdef DEBUG
			(void) printf("read vtoc error\n");
#endif DEBUG
			(void) close(fd);
			return (0);
		}

#ifdef DEBUG
		(void) printf("%s\n", vtoc.v_asciilabel);
#endif DEBUG

		if (strstr(vtoc.v_asciilabel, "Install") == NULL &&
			strstr(vtoc.v_asciilabel, "install") == NULL) {
#ifdef DEBUG
			(void) printf("not an install cd\n");
#endif DEBUG
			(void) close(fd);
			return (0);
		}

		/*
		 * We have an "[iI]nstall" CD. No more name conversion,
		 * deal with that in stubboot
		 */
		(void) close(fd);
		return (1);
	}
#ifdef DEBUG
	(void) printf("dkcinfo.dki_ctype != cdrom: ");

	switch (dkcinfo.dki_ctype) {

	case DKC_SCSI_CCS:
		(void) printf("DKC_SCSI_CCS\n");
		break;
	case DKC_SUN_IPI1:
		(void) printf("DKC_SUN_IPI1\n");
		break;
	case DKC_XY450:
		(void) printf("DKC_XY450\n");
		break;
	default:
		(void) printf("other\n");
		break;
	}
#endif DEBUG
	(void) close(fd);
	return (0);
}

/*
 * slice_name()
 *	Return true if a device name matches the conventions for a
 *	disk, with slice 'slice'.
 * Parameters:
 *	name	- pointer to string containing device name to be parsed
 *	slice	-
 * Return:
 *	0	- 'name' is NOT a disk for the slice specified
 *	1	- 'name' IS a disk for the slice specified
 */
static int
slice_name(char *name, int slice)
{
	int	s;

	must_be(name, 'c');
	skip_digits(name);

	if (*name == 't') {
		name++;
		skip_digits(name);
	}

	must_be(name, 'd');
	skip_digits(name);
	must_be(name, 's');

	s = atoi(name);
	skip_digits(name);

	if (s == slice && *name == '\0')
		return (1);
	else
		return (0);
}
