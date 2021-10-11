#ifndef lint
#ident   "@(#)get_mntpnt.c 1.1 92/11/19 SMI"
#endif	/* lint */
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
#include <sys/types.h>
#include <sys/fstyp.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/fsid.h>
#include <string.h>
#include <sys/fs/ufs_fs.h>

static int get_fs_name(char *);

main(argc, argv)
int argc;
char **argv;
{
	if (argc != 2)
		exit(1);

	exit(get_fs_name(argv[1]));
}

/* get the superblock from a slice & save the last mounted fs name
 * return 1 if there is a usable fs, 0 if any failures
 */
static int
get_fs_name(char *pathname)
{
	int	fd;
	struct fs fs;

	/* Attempt to open the disk.  If it fails, skip it.  */
	if ((fd = open(pathname, O_RDONLY)) < 0)
		return (1);

	if (lseek(fd, SBOFF, SEEK_SET) == -1) {
		(void) close(fd);
		return (1);
	}

	if (read(fd, &fs, sizeof(fs)) == -1) {
		(void) close(fd);
		return (1);
	}
	(void) close(fd);

	if (fs.fs_magic != FS_MAGIC)
		return (1);

	(void) printf("%s\n", fs.fs_fsmnt);
	return (0);
}
