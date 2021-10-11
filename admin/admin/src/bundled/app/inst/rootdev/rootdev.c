/*
 *		"@(#)rootdev.c 1.8 95/12/06"
 *
 * Copyright (c) 1994 by Sun Microsystems, Inc.
 *
 * Find a slice on a disk with a rootfs on it.
 * Rootfs is determined to be any fs last mounted on "/"
 *
 * This uses the disk library routines to find all disks and find the
 * mountpoint name.
 */

#include "spmistore_api.h"

static int rootdev_debug = 0;

/*
 * This stub is needed by the install library. It is never used.
 */
void
cleanup_and_exit()
{
	exit(-2);
}

/*
 * This code is lifted from the init_install view library.
 *
 * get_first_disk(void)
 *
 * Provides interface to the routines which probe the system for disks.
 *
 *	If the disk list head ptr is NULL, call the disk library
 *	   routine which probes for disks and initializes the list.
 *  returns pointer to head of disk list
 */

static Disk_t *
get_first_disk(void)
{
	if (first_disk() == NULL) {
		if (rootdev_debug)
			(void) DiskobjInitList("disks");
		else
			(void) DiskobjInitList(NULL);
	}

	return (first_disk());
}

/*
 * This routine dumps the device name of the given slice in the given
 * Disk_t entry.
 */
static void
exit_dump_device(Disk_t *dc, int slice)
{
	char	buf[128];

	(void) sprintf(buf, "/dev/dsk/%ss%d\n", disk_name(dc), slice);
	(void) printf("%s", buf);
	exit(0);
}

/*
 * Main() program.
 *   Construct a disk chain by calling get_first_disk(). This accesses
 *   the disk library as described above. Next, circulate through all
 *   possible slices, and find the first one last mounted on "/". If none
 *   are found, then it is presumed none exist.
 *
 *   Return 0 if one is found, -1 if none are found.
 */
main()
{
Disk_t *dc = get_first_disk();
int x;
char *fsname;

	for (dc = first_disk(); dc; dc = next_disk(dc))
		for (x = 0; x < NUMPARTS; x++) {
			if (slice_mntpnt_is_fs(dc, x) &&
			    (strcmp(slice_mntpnt(dc, x), "/") == 0))
				exit_dump_device(dc, x);
		}
	return (-1);
}


/*
 * The following functions are dummy placeholders required
 * by the libraries for callback progress displays
 */
void
progress_init()
{
}

void
progress_done()
{
}

void
progress_cleanup()
{
}

/*ARGSUSED*/
void
interactive_pkgadd(int *result)
{
}

/*ARGSUSED*/
void
interactive_pkgrm(int *result)
{
}

/*ARGSUSED0*/
int
start_pkgadd(char *pkgdir)
{
        return (1);
}

/*ARGSUSED0*/
int
end_pkgadd(char *pkgdir)
{
        return (1);
}
