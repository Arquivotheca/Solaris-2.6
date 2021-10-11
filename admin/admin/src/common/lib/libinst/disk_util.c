#ifndef lint
#pragma ident   "@(#)disk_util.c 1.64 95/09/08 SMI"
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
 * This module contains miscellaneous utility functions
 */
#include "disk_lib.h"

#include <sys/mntent.h>
#include <sys/mnttab.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>

/* Local Constants and Macros */

#define	_SC_PHYS_MB_DEFAULT	0x1000000    /* sixteen MB */

#define	skip_digits(s)		if (!isdigit(*(s))) \
					return (-1); \
				while (isdigit(*(s))) \
					(s)++;
#define	must_be(s, c)   	if (*s++ != c) return (0)

int		_library_trace_level = 0;

/*
 * Local structures
 */
typedef struct access_entry {
	char			*device;
	short			slices[16];	/* set to max slice size */
	struct access_entry	*next;
} Access_t;

/* Public Function Prototypes */

u_int		blocks2size(Disk_t *, u_int, int);
u_int		size2blocks(Disk_t *, u_int);
Units_t		set_units(Units_t);
Units_t		get_units(void);
Disk_t *	next_disk(Disk_t *);
Disk_t *	first_disk(void);
Disk_t *	find_bootdisk(void);
Disk_t *	find_disk(char *);
int 		simplify_disk_name(char *, char *);
int		umount_slash_a(void);
char *		make_slice_name(char *, int);
char *		make_block_device(char *, int);
char *		make_char_device(char *, int);
int		axtoi(char *);
int		is_disk_name(char *);
int		is_slice_name(char *);
int		is_ipaddr(char *);
int		is_hostname(char *);
int		is_allnums(char *);
int		is_numeric(char *);
int		is_hex_numeric(char *);
int		map_in_file(const char *, char **);
int		slice_access(char *, int);
char *		library_error_msg(int);
int		get_trace_level(void);
int		set_trace_level(int);

/* Library Function Prototypes */

int		_system_fs_ancestor(char *);
int 		_whole_disk_name(char *, char *);
void    	_sort_disks(void);
void		_set_first_disk(Disk_t *);
void		_set_next_disk(Disk_t *, Disk_t *);
int		_map_to_effective_dev(char *, char *);
int		_disk_is_scsi(Disk_t *);
void		_sort_fdisk_input(Disk_t *);
int		_calc_memsize(void);

/* Local Function Prototypes */

static int	_shift_part_entries(Disk_t *, int);
static void	_swap_part_entries(Disk_t *, int, int);
static int	_usable_part_hole(Disk_t *, int, int, int);
static char *	_find_abs_path(char *);
static int	_map_node_to_devlink(char *, char *);
static int	_map_old_device_to_new(char *, char *);

/* Local Statics */

static Units_t	display_units = D_BLOCK;
static Disk_t	*disks = NULL;
static char blkdevdir[] = "/dev/dsk/";
static char rawdevdir[] = "/dev/rdsk/";

/* External Function Definitions */
extern FILE	*popen(const char *, const char *);

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */
/*
 * blocks2size()
 *	Convert the number of 512 byte blocks to the current unit
 *	(see get_units()/set_units()). The returned value is rounded
 *	to the nearest cylinder boundary, and up or down to the nearest
 *	target unit (see parameters). The physical geometry for
 *	the disk must exist because it is used in the rounding
 *	calculations.
 * Parameters:
 *	dp	- non-NULL pointer to disk structure with valid sdisk
 *		  geometry pointer
 *		  (state: okay)
 *	size 	- number of 512 byte blocks
 *	round	- Boolean indicating if the value should be rounded up
 *		  to the nearest unit (e.g. MB):
 *			ROUNDDOWN - truncate the unit value
 *			ROUNDUP   - round up the unit value
 *
 *		  NOTE: This does not affect cylinder rounding which is
 *			mandatory
 * Return:
 *	0	- 'dp' is NULL, or disk state is not okay
 *	# >= 0	- converted size
 * Status:
 *	public
 */
u_int
blocks2size(Disk_t *dp, u_int size, int round)
{
	if (dp == NULL || disk_not_okay(dp))
		return (0);

	switch (get_units()) {
	    case D_MBYTE:
		if (round == ROUNDDOWN)
			size = blocks_to_mb_trunc(dp, size);
		else
			size = blocks_to_mb(dp, size);
		break;

	    case D_KBYTE:
		if (round == ROUNDDOWN)
			size = blocks_to_kb_trunc(dp, size);
		else
			size = blocks_to_kb(dp, size);
		break;

	    case D_BLOCK:
		size = blocks_to_blocks(dp, size);
		break;

	    case D_CYLS:
		size = blocks_to_cyls(dp, size);
		break;

	    default:	/* no action taken */
		break;
	}

	return (size);
}

/*
 * size2blocks()
 *	Convert the size in the current units (see
 *	set_units()/get_units()) to 512 byte blocks, rounded to the
 *	nearest cylinder boundary. The physical geometry for
 *	the disk must exist because it is used in the rounding
 *	calculations.
 * Parameters:
 *	dp	- non-NULL pointer to disk structure
 *		  (state: okay)
 *	size	- size of current unit to be converted to blocks
 * Returns:
 *	0	- 'dp' is NULL, the disk state is not "okay", or the
 *		  converted size is '0'
 *	# >= 0	- 'size' converted to sectors
 * Status:
 *	public
 */
u_int
size2blocks(Disk_t *dp, u_int size)
{
	if (dp == NULL || disk_not_okay(dp))
		return (0);

	switch (get_units()) {

	case D_MBYTE:
		size = mb_to_blocks(dp, size);
		break;

	case D_KBYTE:
		size = kb_to_blocks(dp, size);
		break;

	case D_BLOCK:
		size = blocks_to_blocks(dp, size);
		break;

	case D_CYLS:
		size = cyls_to_blocks(dp, size);
		break;
	}
	return (size);
}

/*
 * next_disk()
 *	Return a pointer to the next disk in the disk list.
 * Parameters:
 *	Disk_t *	- pointer to current disk structure
 * Return:
 *	NULL	- 'dp' is NULL or dp->next is NULL
 *	Disk_t *	- pointer to next disk in chain
 * Status:
 *	public
 */
Disk_t *
next_disk(Disk_t *dp)
{
	if (dp == NULL)
		return (dp);

	return (dp->next);
}

/*
 * first_disk()
 *	Return a pointer to the first disk in the disk list.
 * Parameters:
 *	none
 * Return:
 *	NULL  - no disks defined on system
 *	Disk_t *  - pointer to head of (physical) disk chain
 * Status:
 *	public
 */
Disk_t *
first_disk(void)
{
	return (disks);
}

/*
 * set_units()
 *	Set the current (default) size unit. The initial unit
 *	size is D_BLOCK. Return the old unit.
 * Parameters:
 *	u	 - new unit (D_MBYTE, D_KBYTE, D_CYLS, D_BLOCK)
 * Return:
 *	Units_t	 - old unit (D_MBYTE, D_KBYTE, D_CYLS, D_BLOCK)
 * Status:
 *	public
 */
Units_t
set_units(Units_t u)
{
	Units_t		s;

	s = display_units;
	display_units = u;
	return (s);
}

/*
 * get_units()
 *	Get the current size unit.
 * Parameters:
 *	none
 * Return:
 *	Units_t	 - unit (D_MBYTE, D_KBYTE, D_CYLS, D_BLOCK)
 * Status:
 *	public
 */
Units_t
get_units(void)
{
	return (display_units);
}

/*
 * find_bootdisk()
 *	Search the disk list for the first drive with the boot
 *	disk flag set and return a pointer to the disk structure
 *	if found (there should only be one drive in this state).
 * Parameters:
 *	none
 * Return:
 *	NULL - no disk has the boot flag set
 * 	Disk_t * - pointer to disk with bootdisk flag set
 * Status:
 *	public
 */
Disk_t *
find_bootdisk(void)
{
	Disk_t	*dp;

	WALK_DISK_LIST(dp) {
		if (disk_bootdrive(dp))
			return (dp);
	}

	return (NULL);
}

/*
 * find_disk()
 *	Search the disk list for a disk which has the same base name
 *	as 'dev' (e.g. c0t0d0 retrieved fro c0t0d0s2 or
 *	/dev/rdsk/c0t0d0s0). Return a pointer to the disk structure,
 *	or NULL of none exists.
 * Parameters:
 *	dev	- special file name for a device (e.g. c0t0d0s3 of
 *		  c0t0d0p0)
 * Return:
 *	NULL  - no such device in path; 'dev' is a NULL pointer,
 *		    or 'dev' is a NULL string
 *	!NULL - pointer to disk structure in array
 * Status:
 *	public
 * Note:
 *	This routine assumes that drive names are unique to devices.
 */
Disk_t *
find_disk(char *dev)
{
	char	name[16];
	Disk_t	*dp;

	if (dev == NULL || *dev == '\0')
		return (NULL);

	if (simplify_disk_name(name, dev) == 0) {
		WALK_DISK_LIST(dp) {
			if (strcmp(disk_name(dp), name) == 0)
				return (dp);
		}
	}

	return (NULL);
}

/*
 * simplify_disk_name()
 *	Convert a conventional disk name into the internal canonical
 *	form. Remove the trailing index reference. The return status
 *	reflects whether or not the 'src' name is valid.
 *
 *			src			 dst
 *		---------------------------------------
 *		[/dev/rdsk/]c0t0d0s0	->	c0t0d0
 *		[/dev/rdsk/]c0t0d0p0	->	c0t0d0
 *		[/dev/rdsk/]c0d0s0	->	c0d0
 *		[/dev/rdsk/]c0d0p0	->	c0d0
 *
 * Parameters:
 *	dst	- used to retrieve cannonical form of drive name
 *		  ("" if not valid)
 *	src	- name of drive to be processed (see table above)
 * Return:
 *	 0	- valid disk name
 *	-1	- invalid disk name
 * Status:
 *	public
 */
int
simplify_disk_name(char *dst, char *src)
{
	char	name[128];
	char	*cp;

	*dst = '\0';

	/* look for the controller specifier */
	if ((cp = strrchr(src, 'c')) == NULL)
		return (-1);
	cp = strcpy(name, cp);
	cp++;
	skip_digits(cp);		/* must be at least 1 digit */

	/* the target specifier is optional */
	if (*cp == 't') {
		++cp;
		skip_digits(cp);	/* must be at least 1 digit */
	}

	/* look for the drive specifier */
	if (*cp++ != 'd')
		return (-1);
	skip_digits(cp);		/* must be at least 1 digit */

	/* look for the slice/partition specifier */
	if (*cp && *cp != 'p' && *cp != 's')
		return (-1);

	*cp = '\0';
	(void) strcpy(dst, name);
	return (0);
}

/*
 * umount_slash_a()
 *	Unmount /a* (may be leftover mounts from a previously
 *	aborted upgrade or install attempts. This routine assumes
 *	that all mounted file systms are logged in the /etc/mnttab
 *	file. File systems are unmounted in the reverse order in
 *	which they appear in /etc/mnttab. It is assumed there are
 *	no more than 10 /a* file systems.
 *
 *	NOTE:	this routine will only process those file
 *		systems which are recorded in /etc/mnttab
 * Parameters:
 *	none
 * Return:
 *	0	- successfull
 *	-1	- unmount failed; see errno for reason
 * Status:
 *	public
 */
int
umount_slash_a(void)
{
	struct mnttab	ment;
	char		mounts[10][MAXNAMELEN];
	char		cmd[MAXNAMELEN];
	FILE	*fp;
	int	j, i;

	/*CONSTCOND*/
	if ((fp = fopen("/etc/mnttab", "r")) != NULL) {
		j = 0;
		while (getmntent(fp, &ment) == 0) {
			if (strncmp(ment.mnt_mountp, "/a/", 3) == 0 ||
					strcmp(ment.mnt_mountp, "/a") == 0)
				(void) strcpy(mounts[j++], ment.mnt_mountp);
		}

		(void) fclose(fp);
		if (j > 0) {
			for (i = j - 1; i >= 0; i--) {
				(void) sprintf(cmd,
					"/sbin/umount %s", mounts[i]);
				if (system(cmd) != 0)
					return (-1);
			}
		}
	}
	return (0);
}

/*
 * make_slice_name()
 * 	Assemble a complete slice device name (e.g. c0t0d0s0)
 * Parameters:
 *	disk	- pointer to disk name (e.g. c0t0d0)
 *	slice	- slice number
 * Return:
 *	char *	- pointer to local buffer containing assembled name
 * Status:
 *	public
 */
char *
make_slice_name(char *disk, int slice)
{
	static char	buf[16];

	buf[0] = '\0';
	if ((int)  strlen(disk) < 15)
		(void) sprintf(buf, "%ss%d", disk, slice);
	return (buf);
}

/*
 * make_block_device()
 * 	Assemble a complete block device name (e.g. /dev/dsk/c0t0d0s0)
 * Parameters:
 *	disk	- pointer to disk name (e.g. c0t0d0)
 *	slice	- slice number
 * Return:
 *	char *	- pointer to local buffer containing assembled name
 * Status:
 *	public
 */
char *
make_block_device(char *disk, int slice)
{
	static char	buf[32];

	buf[0] = '\0';
	(void) sprintf(buf, "/dev/dsk/%ss%d", disk, slice);
	return (buf);
}

/*
 * make_char_device()
 * 	Assemble a complete character device name (e.g. /dev/rdsk/c0t0d0s0)
 * Parameters:
 *	disk	- pointer to disk name (e.g. c0t0d0)
 *	slice	- slice number
 * Return:
 *	char *	- pointer to local buffer containing assembled name
 * Status:
 *	public
 */
char *
make_char_device(char *disk, int slice)
{
	static char	buf[32];

	buf[0] = '\0';
	(void) sprintf(buf, "/dev/rdsk/%ss%d", disk, slice);
	return (buf);
}

/*
 * is_disk_name()
 *      Check to see if 'dev' syntactically represents
 *      a cannonical disk name (e.g. c0t0d0).
 * Parameters:
 *      dev     - string containing device name to be validated
 * Return:
 *      1       - string is a valid disk name
 *      0       - string is not a valid disk name
 */
int
is_disk_name(char *dev)
{
	if (dev) {
		must_be(dev, 'c');
		skip_digits(dev);
		if (*dev == 't') {
			dev++;
			skip_digits(dev);
		}
		must_be(dev, 'd');
		skip_digits(dev);
	}

	if (dev != NULL && *dev == '\0')
		return (1);

	return (0);
}

/*
 * is_slice_name()
 *      Check to see if 'dev' syntactically represents
 *      a cannonical slice device name (e.g. c0t0d0s3).
 * Parameters:
 *      dev     - string containing device name to be validated
 * Return:
 *      1       - string is a valid slice name
 *      0       - string is not a valid slice name
 */
int
is_slice_name(char *dev)
{
	if (dev) {
		must_be(dev, 'c');
		skip_digits(dev);
		if (*dev == 't') {
			dev++;
			skip_digits(dev);
		}
		must_be(dev, 'd');
		skip_digits(dev);
		must_be(dev, 's');
		skip_digits(dev);
	}

	if (dev != NULL && *dev == '\0')
		return (1);

	return (0);
}

/*
 * is_ipaddr()
 *      Verify that the argument is a valid Internet address
 * Parameters:
 *      addr    - non-NULL pointer to string containing textual
 *		form of an Internet address
 * Return:
 *      0       - invalid address
 *      1       - valid address
 */
int
is_ipaddr(char *addr)
{
	int num;
	char *p;

	if ((p = strchr(addr, '.')) == NULL)
		return (0);
	*p = '\0';
	num = atoi(addr);
	if (num < 0 || num > 255 || is_allnums(addr) == 0)
		return (0);
	*p = '.';
	addr = p + 1;

	if ((p = strchr(addr, '.')) == NULL)
		return (0);
	*p = '\0';
	num = atoi(addr);
	if (num < 0 || num > 255 || is_allnums(addr) == 0)
		return (0);
	*p = '.';
	addr = p + 1;

	if ((p = strchr(addr, '.')) == NULL)
		return (0);
	*p = '\0';
	num = atoi(addr);
	if (num < 0 || num > 255 || is_allnums(addr) == 0)
		return (0);
	*p = '.';
	addr = p + 1;

	num = atoi(addr);
	if (num < 0 || num > 255 || is_allnums(addr) == 0)
		return (0);

	return (1);
}

/*
 * is_hostname()
 *	Check to see that a string passed in meets the RFC 952/1123
 *	hostname criteria.
 * Parameters:
 *	name	- string containing hostname to be validated
 * Return:
 *      0       - invalid address
 *      1       - valid address
 * Status:
 *	public
 */
int
is_hostname(char *name)
{
	char	*seg;
	char	*cp;
	int	length;
	char	buf[MAXNAMELEN] = "";

	/* validate parameter */
	if (name == NULL)
		return (0);

	(void) strcpy(buf, name);
	if ((seg = strchr(buf, '.')) != NULL) {
		*seg++ = '\0';
		/* recurse with next segment */
		if (is_hostname(seg) == 0)
			return (0);
	}

	/*
	 * length must be 2 to 63 characters (255 desireable, but not
	 * required by RFC 1123)
	 */
	length = (int) strlen(buf);
	if (length < 2 || length > 63)
		return (0);

	/* first character must be alphabetic or numeric */
	if (isalnum((int) buf[0]) == 0)
			return (0);

	/* last character must be alphabetic or numeric */
	if (isalnum((int) buf[length - 1]) == 0)
		return (0);

	/* names must be comprised of alphnumeric or '-' */
	for (cp = buf; *cp; cp++) {
		if (isalnum((int)*cp) == 0 && *cp != '-')
			return (0);
	}

	return (1);
}

/*
 * is_allnums()
 *      Check a character string and ensure that it represents
 *      a decimal number sequence.
 * Parameters:
 *      str     - string to be validated
 * Return:
 *      0       - string is not a pure numeric string
 *      1       - string is a a pure numeric string
 */
int
is_allnums(char *str)
{
	if (str == NULL || *str == '\0')
		return (0);

	if (str && *str) {
		for (; *str; str++) {
			if (isdigit(*str) == 0)
				return (0);
		}
	}

	return (1);
}

/*
 * is_hex_numeric()
 *      Check a character string and ensure that it represents
 *      a hex number sequence.
 * Parameters:
 *      str     - string to be validated
 * Return:
 *      0       - string is not a pure hex string
 *      1       - string is a a pure hex string
 */
int
is_hex_numeric(char *str)
{
	if (str == NULL || *str == '\0')
		return (0);

	if (strlen(str) > 2U && *str++ == '0' && strchr("Xx", *str)) {
		for (++str; *str; str++) {
			if (!isxdigit(*str))
				return (0);
		}
		return (1);
	}

	return (0);
}

/*
 * is_numeric()
 *      Check a character string and ensure that it represents
 *      either a hexidecimal or decimal number.
 * Parameters:
 *      str     - string to be validated
 * Return:
 *      0       - string is not a hex/dec number
 *      1       - string is a hex/dec number
 */
int
is_numeric(char *str)
{
	if (str && *str) {
		if (strlen(str) > 2U &&
				str[0] == '0' && strchr("Xx", str[1])) {
			str += 2;
			while (*str) {
				if (!isxdigit(*str))
					return (0);
				else
					str++;
			}
			return (1);
		} else {
			while (*str) {
				if (!isdigit(*str))
					return (0);
				else
					str++;
			}
			return (1);
		}
	}
	return (0);
}

/*
 * axtoi()
 *      Convert a hexidecimal, octal, or decimal string to integer
 *	form.
 * Parameters:
 *      str	- string to convert
 * Return:
 *      # >= 0  - decimal conversion of a hexidecimal string
 */
int
axtoi(char *str)
{
	int	retval = 0;

	if (str && *str) {
		if (strlen(str) > 2U &&
				str[0] == '0' && strchr("Xx", str[1])) {
			str += 2;
			if (sscanf(str, "%x", (u_int *) &retval) != 1)
				return (0);
		} else if (strlen(str) > 1U && str[0] == '0') {
			str++;
			if (sscanf(str, "%o", (u_int *) &retval) != 1)
				return (0);
		} else {
			if (sscanf(str, "%d", &retval) != 1)
				return (0);
		}

		return (retval);
	}

	return (0);
}

/*
 * map_in_file()
 *	mmaps in a file into a buffer *buf.  returns the size of
 *      the mmapped buffer.
 * Parameters:
 *	file	- pointer to a string containing the path name to the
 *		  file being mapped
 *	buf	- address of a character pointer which will hold the
 *		  mmapped addressing
 * Return:
 *	-1	- mmap failed
 *	#>=0	- size of file mmapped in
 * Status:
 *	public
 */
int
map_in_file(const char *file, char **buf)
{
	struct stat	st;
	int		fd;

	if (buf == NULL)
		return (-1);

	if ((fd = open(file, O_RDONLY)) < 0)
		return (-1);

	if (fstat(fd, &st) < 0) {
		(void) close(fd);
		return (-1);
	}

	if (st.st_size == 0) {
		(void) close(fd);
		*buf = NULL;
		return (0);
	}

	if ((*buf = mmap((caddr_t)0, (size_t)st.st_size, PROT_READ,
			(MAP_PRIVATE | MAP_NORESERVE),
			fd, (off_t)0)) == MAP_FAILED) {
		(void) close(fd);
		return (-1);
	}

	(void) close(fd);
	return (st.st_size);
}

/*
 * get_trace_level()
 *	Get the current level of library tracing. This is used for
 *	internal tracing of application and library activity
 *	independent of the current debug status.
 * Parameters:
 *	none
 * Return:
 *	# >= 0	- current trace level
 * Status:
 *	public
 */
int
get_trace_level(void)
{
	return (_library_trace_level);
}

/*
 * set_trace_level()
 *	Set the current level of library tracing. This is used for
 *	internal tracing of application and library activity
 *	independent of the current debug status.
 * Parameters:
 *	set	 - trace level:
 *		   0	 - off
 *		   # > 0 - trace level (the higher the more verbose)
 * Return:
 *	# >= 0	 - new trace level
 *	D_BADARG - invalid argument
 * Status:
 *	public
 */
int
set_trace_level(int set)
{
	if (set < 0)
		return (D_BADARG);

	_library_trace_level = set;

	return (_library_trace_level);
}

/*
 * slice_access()
 *	Keep a record of all slices which have been accessed
 *	explicitly. Return code indicates if the slice is already
 *	in the list of specified slices. The list of known slices
 *	is not kept in sorting order since the average chain will
 *	not have more than three or four entries.
 * Parameters:
 *	device	- string specifying device name (e.g. c0t0d0s2)
 *	alloc	- specifies if the entry should be added to the
 *		  list of access slices
 *		  (Valid values:  0 - do not add   1 - add)
 * Return:
 *	0	- the specified slice has not already been used
 *	1	- the specified slice has already been used
 */
int
slice_access(char *device, int alloc)
{
	static Access_t	*Access_list = NULL;
	int		retval = 0;
	Access_t	*ap;
	char		buf[32];
	char		*cp;
	int		si;

	(void) strcpy(buf, device);
	cp = strrchr(buf, 's');
	*cp++ = '\0';
	si = atoi(cp);

	WALK_LIST(ap, Access_list) {
		if (streq(buf, ap->device)) {
			if (ap->slices[si] == 1) {
				retval = 1;
				break;
			}
		}
	}

	if (alloc == 1) {
		if (ap == NULL) {
			/*
			 * allocate and initialize new structure and add
			 * it to the list of known slices
			 */
			if ((ap = (Access_t *)xcalloc(
					sizeof (Access_t))) == NULL)
				return (0);

			ap->device = xstrdup(buf);
			ap->next = Access_list;
			Access_list = ap;
		}

		ap->slices[si] = 1;
	}

	return (retval);
}

/*
 * library_error_msg()
 *	Assemble an error message based on an install library function return
 *	code. The disk name which failed is prepended to the front of the
 *	message. No newline is appended.
 * Parameters:
 *	status	- return status code from library function call
 * Return:
 *	char *	- pointer to local static buffer containing the assembled
 *		  message
 * Status:
 *	public
 */
char *
library_error_msg(int status)
{
	static char	buf[BUFSIZ * 2];

	switch (status) {
	    case D_ALIGNED:
		    (void) sprintf(buf, MSG0_ALIGNED);
		    break;
	    case D_ALTSLICE:
		    (void) sprintf(buf, MSG0_ALTSLICE);
		    break;
	    case D_BADARG:
		    (void) sprintf(buf, MSG0_BADARGS);
		    break;
	    case D_BADDISK:
		    (void) sprintf(buf, MSG0_BADDISK);
		    break;
	    case D_BADORDER:
		    (void) sprintf(buf, MSG0_BADORDER);
		    break;
	    case D_BOOTCONFIG:
		    (void) sprintf(buf, MSG0_BOOTCONFIG);
		    break;
	    case D_BOOTFIXED:
		    (void) sprintf(buf, MSG0_BOOTFIXED);
		    break;
	    case D_CANTPRES:
		    (void) sprintf(buf, MSG0_CANTPRES);
		    break;
	    case D_CHANGED:
		    (void) sprintf(buf, MSG0_CHANGED);
		    break;
	    case D_DUPMNT:
		    (void) sprintf(buf, MSG0_DUPMNT);
		    break;
	    case D_GEOMCHNG:
		    (void) sprintf(buf, MSG0_GEOMCHNG);
		    break;
	    case D_IGNORED:
		    (void) sprintf(buf, MSG0_IGNORED);
		    break;
	    case D_ILLEGAL:
		    (void) sprintf(buf, MSG0_ILLEGAL);
		    break;
	    case D_LOCKED:
		    (void) sprintf(buf, MSG0_LOCKED);
		    break;
	    case D_NODISK:
		    (void) sprintf(buf, MSG0_DISK_INVALID);
		    break;
	    case D_NOFIT:
		    (void) sprintf(buf, MSG0_NOFIT);
		    break;
	    case D_NOGEOM:
		    (void) sprintf(buf, MSG0_NOGEOM);
		    break;
	    case D_NOSOLARIS:
		    (void) sprintf(buf, MSG0_NOSOLARIS);
		    break;
	    case D_NOSPACE:
		    (void) sprintf(buf, MSG0_NOSPACE);
		    break;
	    case D_NOTSELECT:
		    (void) sprintf(buf, MSG0_NOTSELECT);
		    break;
	    case D_OFF:
		    (void) sprintf(buf, MSG0_OFF);
		    break;
	    case D_OK:
		    (void) sprintf(buf, MSG0_OK);
		    break;
	    case D_OUTOFREACH:
		    (void) sprintf(buf, MSG0_OUTOFREACH);
		    break;
	    case D_OVER:
		    (void) sprintf(buf, MSG0_OVER);
		    break;
	    case D_PRESERVED:
		    (void) sprintf(buf, MSG0_PRESERVED);
		    break;
	    case D_ZERO:
		    (void) sprintf(buf, MSG0_ZERO);
		    break;
	    case D_FAILED:
		    (void) sprintf(buf, MSG0_FAILED);
		    break;
	    default:
		    (void) sprintf(buf, MSG1_STD_UNKNOWN_ERROR, status);
		    break;
	}

	return (buf);
}

/* ******************************************************************** */
/*			LIBRARY SUPPORT FUNCTIONS			*/
/* ******************************************************************** */
/*
 * _system_fs_ancestor()
 *	Determine if a directory name is a child of the system directory
 *	namespace which is used during install. System directories include:
 *
 *			/dev 	/usr	/etc	/sbin
 *			/bin 	/var	/opt	/devices
 *			/lib 	/export	/a	/tmp
 *			/kernel /.cache
 *
 * Parameters:
 *	fs	- non-NULL file system name to be analyzed
 * Return:
 *	0	- 'fs' is not a system directory child
 *	1	- 'fs' is a system directory child
 * Status:
 *	semi-private (internal library use only)
 */
int
_system_fs_ancestor(char *fs)
{
	int	i;
	int	n;
	static char	*_sysfs[] = {
			"/a",
			"/.cache",
			"/bin",
			"/dev",
			"/devices",
			"/etc",
			"/export",
			"/kernel",
			"/lib",
			"/opt",
			"/sbin",
			"/tmp",
			"/usr",
			"/var",
			"/platform",
			NULL };

	if (strcmp(fs, "/")  == 0)
		return (1);

	for (i = 0; _sysfs[i]; i++) {
		if (strcmp(fs, _sysfs[i]) == 0)
			return (1);

		n = strlen(_sysfs[i]);
		if (strncmp(fs, _sysfs[i], n) == 0 &&
				fs[n] == '/' &&
				strncmp(fs, "/export/home", 12) != 0)
			return (1);
	}

	return (0);
}

/*
 * _disk_is_scsi()
 *	Determine if a disk controller type is or is not SCSI. The
 *	controller type should have been set at disk initialization
 *	time. Controller types identified as SCSI as of S494 are:
 *		DKC_SCSI_CCS
 *		DKC_CDROM
 *		DKC_MD21
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 * Return:
 *	0	- disk is not a known SCSI type
 * 	1	- disk is a known SCSI type
 * Status:
 *	semi-private (internal library use only)
 */
int
_disk_is_scsi(Disk_t *dp)
{
	if (dp == NULL)
		return (0);

	switch (dp->ctype) {

	case  DKC_SCSI_CCS:
	case  DKC_CDROM:
	case  DKC_MD21:
		return (1);

	default:
		return (0);
	}
}

/*
 * _sort_disks()
 *	Search the disk chain to see if a (there should only be one)
 *	drive is marked as D_BOOTDRIVE and, if so, move it to
 *	the head of the disk list.
 * Parameters:
 *	none
 * Return:
 *	none
 * Status:
 *	semi-private (internal library use only)
 */
void
_sort_disks(void)
{
	Disk_t	*dp, *prev;

	/* Search for the first (only) D_BOOTDRIVE disk and move it up front */
	prev = NULL;
	WALK_DISK_LIST(dp) {
		if (disk_bootdrive(dp)) {
			if (prev) {
				_set_next_disk(prev, next_disk(dp));
				_set_next_disk(dp, first_disk());
				_set_first_disk(dp);
			}

			return;
		}

		prev = dp;
	}
}

/*
 * _set_first_disk()
 *	Set the first physical disk pointer to 'dp'
 * Parameters:
 *	dp	- disk structure pointer to use in intialization
 * Return:
 *	none
 * Status:
 *	semi-private (internal library use only)
 */
void
_set_first_disk(Disk_t *dp)
{
	disks = dp;
}

/*
 * _set_next_disk()
 *	Set the "next" disk pointer field in 'dp' to 'nextdp'.
 * Parameters:
 *	dp	- pointer to disk structure for which the next pointer
 *		  is to be augmented
 *	nextdp	- pointer to disk structure to be referenced as "next"
 * Return:
 *	none
 * Status:
 *	semi-private (internal library use only)
 */
void
_set_next_disk(Disk_t *dp, struct disk *nextdp)
{
	if (dp != NULL)
		dp->next = nextdp;
}

/*
 * _whole_disk_name()
 *	Determine if the disk device name (or pathname) represented
 *	by 'src' has a special file name of cX[tX]dXs2 or cX[tX]dXp0,
 *	which usually represents the backup slice for the whole drive.
 *	If 'src' is a whole pathname, put the cannonical form in 'dst'
 *	and returen 0, otherwise, return "" in 'dst' and return -1.
 * Parameters:
 *	dst	- modified string in drive canonical form ("" if not valid)
 *	src	- string name of disk
 * Return:
 *	 0	- 'src' does represent a whole disk name and 'dst'
 *		  is set to the cannonical form
 *	-1	- 'src' does not represent a whole disk name and
 *		  'dst' is set to ""
 * Status:
 *	semi-private (internal library use only)
 */
int
_whole_disk_name(char *dst, char *src)
{
	if (simplify_disk_name(dst, src) == 0 &&
			(strcmp(&src[strlen(src) - 2], "s2") == 0 ||
			strcmp(&src[strlen(src) - 2], "p0") == 0))
		return (0);

	*dst = '\0';
	return (-1);
}

/*
 * _map_to_effective_dev()
 *	Used during installation and upgrade to retrieve the local
 *	'/dev/<r>dsk' name which points to the same physical device
 *	(i.e. /devices/...) as 'dev' does in the <bdir> client device namespace.
 * Parameters:
 *	dev	- [IN]  device name (e.g. /dev/rdsk/c0t0d0s3)
 *	edevbuf	- [OUT] pathname of device on booted OS
 *			(e.g. /devices/....).  The space allocated for this
 *			buffer is allocated by calling routine.
 *
 * Return:
 *	0	- search completed; edevbuf has whatever value was found
 *	1	- failure; error while scanning links in local /dev directory
 *	2	- failure; cannot read the link /<bdir>/<dev>
 * Status:
 *	private
 * Algorithm:
 *	Check to see if '/dev' is a character or block device. Then
 *	translate /<bdir>/<dev> to its symbolic link value, and scan the
 *	local /dev/<r>dsk directory for a device that has the same
 *	symbolic link destination (e.g. /device/...). If not found,
 *	leave 'edevbuf' NULL, otherwise, copy the name of the local
 *	device (e.g. /dev/<r>dsk/...) into 'edevbuf'.
 */
int
_map_to_effective_dev(char *dev, char *edevbuf)
{
	static char deviceslnk[] = "../devices/";
	static char devlnk[] = "../dev/";
	char	linkbuf[MAXNAMELEN];
	char	mapped_name[MAXNAMELEN];
	char	ldev[MAXNAMELEN];
	char	*abs_path;
	int	len;

	edevbuf[0] = '\0';

	(void) sprintf(ldev, "%s%s", get_rootdir(), dev);
	if ((len = readlink(ldev, linkbuf, MAXNAMELEN)) == -1)
		return (2);
	linkbuf[len] = '\0';
	/*
	 * We now have the link (this could be to dev/ or ../devices. We
	 * now must make sure that we correctly map the BSD style devices.
	 */
	if (strncmp(dev, blkdevdir, strlen(blkdevdir)) != 0 &&
	    strncmp(dev, rawdevdir, strlen(rawdevdir)) != 0) {
		/* this must be a BSD style device */
		if (strncmp(linkbuf, deviceslnk, strlen(deviceslnk)) == 0) {
			/*
			 * A link to ../devices/, to be compatible with SVR4
			 * devices this link must be ../../devices
			 */
			(void) sprintf(linkbuf, "../%s", linkbuf);
		} else {
			if (strncmp(linkbuf, devlnk, strlen(devlnk)) == 0) {
				char	*tmpStr;
				/*
				 * This is a link to ../dev, we can just
				 * strip off the ../dev and use the logic
				 * below to get the linkbuf.
				 */
				if ((tmpStr = strstr(linkbuf, devlnk)) ==
				    NULL)
					return (1);
				/* Step past the ../dev/ */
				tmpStr = tmpStr + strlen(devlnk);
				/* copy the new path into linkbuf */
				(void) strcpy(linkbuf,tmpStr);
			}
			
			/*
			 * Here we have a link to dev/, we now need to map
			 * this to /a/dev/ and then read that link.
			 */
			sprintf(ldev, "%s/dev/%s", get_rootdir(), linkbuf);
			if ((len = readlink(ldev, linkbuf, MAXNAMELEN)) == -1)
				return (2);
			linkbuf[len] = '\0';
		}
	}

	/*
	 * Find the point in the linkbuf where the absolute pathname
	 * of the node begins (that is, skip over the "..[/..]*" part)
	 * and save the length of the leading relative part of the pathname.
	 */

	abs_path = _find_abs_path(linkbuf);
	len = abs_path - linkbuf;

	/*
	 * Now that we have the devices path to the device we need to
	 * search for that entry on the boot environment. (This is the
	 * effective device.
	 */

	if (access(abs_path, F_OK) == 0) {
		if (_map_node_to_devlink(linkbuf, edevbuf) == 0)
			return (0);
	}

	/*
	 *  The device node in linkbuf either doesn't exist in the
	 *  current effective root file system, or it exists but no
	 *  link to it exists in /dev/dsk or /dev/rdsk.  The device
	 *  may have a new name in the new release.  Attempt to map
	 *  the old name to a new name.
	 */

	/* copy the leading relative part of the link to the mapped name buf */
	strncpy(mapped_name, linkbuf, len);

	/*
	 *  Now fill the rest of the mapped_name buffer with the mapping
	 *  of the absolute part of the name (if possible).
	 */
	if (_map_old_device_to_new(abs_path, mapped_name + len) == 0)
		return (_map_node_to_devlink(mapped_name, edevbuf));
	else
		return (1);

}

/*
 * _sort_fdisk_input()
 *	Make sure that blocks of unused space in the F-disk partition
 *	table have an UNUSED partition relative to their location
 *	on the disk. For example, if there is unused space at the
 *	beginning of the drive, make sure partition 1 is UNUSED so
 *	that install can allocate it.
 *
 *	ALGORITHM:
 *		Unused partitions are assigned to the largest holes
 *		to ensure maximum use of the drive.
 *
 *		(1) determine how many unused partitions there are.
 *		    If none or all are unused, no work to do.
 *		(2) for each used partition which doesn't have an
 *		    UNUSED partition preceding it, check to see if
 *		    it qualifies as "biggest" of remaining holes,
 *		    and if so, shift it down in the partition table
 *		    and create an UNUSED partition in front of it.
 *
 *	NOTE:	This sorting routine relies on the fact that
 *		Solaris fdisk(1M) displays partitions in
 *		ascending order based on their relative
 *		physical position on the disk (i.e. physical
 *		order == real order). For this reason, the
 *		input file is always sorted before processing
 *		for unused space.
 * Parameters:
 *	dp	- non-NULL disk structure pointer with loaded F-disk
 *		  partition information
 * Return:
 *	none
 * Status:
 *	semi-private (internal library use only)
 */
void
_sort_fdisk_input(Disk_t *dp)
{
	int	part;
	int	start;
	int	count;
	int	size;
	int	next;

	start = one_cyl(dp);
	count = 0;

	/* sort the table (if necessary) */
	WALK_PARTITIONS(part) {
		if (part_id(dp, part) == UNUSED)
			continue;

		for (next = part + 1; next <= FD_NUMPART; next++) {
			if (part_id(dp, next) == UNUSED)
				continue;

			if (part_startsect(dp, next) < part_startsect(dp, part))
				_swap_part_entries(dp, part, next);
		}
	}

	/* find how many unused partitions exist */
	WALK_PARTITIONS(part) {
		if (part_id(dp, part) == UNUSED)
			count++;
	}

	/* no work to do if no free partitions or all free partitions */
	if (count == 0 || count == FD_NUMPART)
		return;

	WALK_PARTITIONS(part) {
		if (count <= 0)
			break;

		if (part_id(dp, part) == UNUSED)
			continue;

		if (part > 1 && part_id(dp, part - 1) == UNUSED) {
			start = part_startsect(dp, part) +
					part_size(dp, part);
			continue;
		}

		size = part_startsect(dp, part) - start;

		/*
		 * if there is a hole before "part", try to shift
		 * part down in the partition table - may have to
		 * scoot others down too. If this fails, you're
		 * done, otherwise, decrement the UNUSED partition
		 * counter and skip over the next partition since
		 * you know you just shifted the current one to that
		 * position
		 */
		if (_usable_part_hole(dp, part, count, size)) {
			if (_shift_part_entries(dp, part) < 0)
				return;

			--count;
			++part;
		}

		/* advance the hole start to the end of this partition */
		start = part_startsect(dp, part) + part_size(dp, part);
	}
}

/*
 * _calc_memsize()
 *	Return system memory size in 512 byte blocks. If sysconf()
 *	does not have a value for the number of pages on the system,
 *	use the localled defined constant _SC_PHYS_MB_DEFAULT (16 MB).
 * Parameters:
 *	none
 * Return:
 *	# > 0	- memory size in 512 byte blocks
 * Status
 *	semi-private (internal library use only)
 */
int
_calc_memsize(void)
{
	long	pages, size;
	char	*tmem;
	u_int	byte_calc;

	/* first check if an overrride memsize is set in memory */

	if ((tmem = getenv("SYS_MEMSIZE")) != NULL)
		byte_calc = atoi(tmem) * 0x100000;
	else {
		size = PAGESIZE;			/* bytes per page  */
		pages = sysconf(_SC_PHYS_PAGES);	/* number of pages */

		/* required test i386 S1093 doesn't support _SC_PHYS_PAGES */
		if (pages <= 0)
			byte_calc = _SC_PHYS_MB_DEFAULT;
		else
			byte_calc = pages * size;
	}

	return (bytes_to_sectors(byte_calc));
}

/* ******************************************************************** */
/*			  LOCAL SUPPORT FUNCTIONS			*/
/* ******************************************************************** */

/*
 * _usable_part_hole()
 *	Determine if a given partition on the disk is preceded by unused
 *	space, and if so, if that space is larger than gaps which occur
 *	later on the drive such that this partition is warranted being
 *	assigned one of the UNUSED partitions.
 *
 *	ASSUMPTIONS:
 *		- partitions MUST be ordered in physically ascending order
 *		  at this point
 *	NOTE:
 *		- we ignore partitions of size < one_cyl(), since that
 *		  space will not be usable in the Solaris domain
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 *	part	- non-UNUSED partition number for which you
 *		  want to find if there's a preceding hole
 *	count	- number of UNUSED partitions on the disk
 *		  (# >= 0)
 *	size	- size of hole preceding the current partition
 *		  (could be positive, 0, or even negative), but
 *		  must bet at least one_cyl() to be considered
 *		  usable
 * Return:
 *	0	- there is no preceding hole, or the hole that
 *		  exists is not big enough relative to other
 *		  subsequent holes to warrant being allocated
 *		  an UNUSED partition
 *	1	- the preceding hole is large enough to warrant
 *		  being assigned an UNUSED partition
 * Status:
 *	private
 */
static int
_usable_part_hole(Disk_t *dp, int part, int count, int size)
{
	int	start;
	int	hole;
	int	holecnt;
	int	bigger;
	int	i;

	holecnt = 0;
	bigger = 0;

	/*
	 * the hole is unusable if it is less than one cylinder,
	 * or the # of available UNUSED partitions is unusable
	 */
	if (size < one_cyl(dp) || count <= 0)
		return (0);

	/* start searching for holes after the current partition */
	start = part_startsect(dp, part) + part_size(dp, part);

	for (i = part + 1; i <= FD_NUMPART; i++) {
		if (part_id(dp, i) == UNUSED)
			continue;

		hole = part_startsect(dp, i) - start;

		if (hole > one_cyl(dp)) {
			holecnt++;
			if (size >= hole)
				bigger++;
		}

		start = part_startsect(dp, i) + part_size(dp, i);
	}
	/* check the end of the drive in case there is a hole there */
	hole = disk_geom_lcyl(dp) - start;

	if (hole > one_cyl(dp)) {
		holecnt++;
		if (size >= hole)
			bigger++;
	}

	/*
	 * if the given hole is bigger than "N" other subsequent holes
	 * and "N" is the number of subsequent holes for which there
	 * are no UNUSED partitions available, then this hole should
	 * get an UNUSED partition
	 */
	if (bigger > (holecnt - count))
		return (1);

	return (0);
}

/*
 * _shift_part_entries()
 *	Shift the specified F-disk partition entry to the next slot.
 *	If the next slot is not UNUSED, then shift that entry, and
 *	so forth, until you hit the end of the table, or you successfully
 *	shift.
 *
 *	ALGORITHM:
 *		- recursive
 *		- terminating condition: part == last valid partition
 * Parameters:
 *	dp	- non-NULL disk structure pointer
 *	part	- valid partition index to be shifted down in the table
 * Return:
 *	 0	- shift successful
 *	-1	- no more partitions to shift to; shift failed
 * Status:
 *	private
 */
static int
_shift_part_entries(Disk_t *dp, int part)
{
	Part_t		tmp;

	/* terminating condition */
	if (part == FD_NUMPART)
		return (-1);

	if (part_id(dp, part + 1) != UNUSED) {
		if (_shift_part_entries(dp, part + 1) < 0)
			return (-1);
	}

	/* swap the UNUSED entry with the used entry */
	(void) memcpy(&tmp, fdisk_part_addr(dp, part + 1), sizeof (Part_t));
	(void) memcpy(fdisk_part_addr(dp, part + 1),
			fdisk_part_addr(dp, part), sizeof (Part_t));
	(void) memcpy(fdisk_part_addr(dp, part), &tmp, sizeof (Part_t));

	return (0);
}

/*
 * _swap_part_entries()
 *	Swap two partition entries in an Fdisk_t data structure
 * Parameters:
 *	dp	- non-NULL pointer to disk structure
 *	next	- valid logical partition index to switch
 *	part	- valid logical partition index to switch
 * Return:
 *	none
 * Status:
 *	private
 */
static void
_swap_part_entries(Disk_t *dp, int part, int next)
{
	Part_t	tmp;

	/* swapping a partition with itself; we're done */
	if (part == next)
		return;

	(void) memcpy(&tmp, fdisk_part_addr(dp, part), sizeof (Part_t));
	(void) memcpy(fdisk_part_addr(dp, part),
			fdisk_part_addr(dp, next), sizeof (Part_t));
	(void) memcpy(fdisk_part_addr(dp, next), &tmp, sizeof (Part_t));
}

/*
 * _find_abs_path()
 *	Find the absolute part of a relative pathname (that is, find
 *	the part that starts after the "..[/..]*".  If no "." or ".."
 *	pathname segments exist at the beginning of the path, just
 *	return the beginning of the input string.  Don't modify the
 *	input string.  Just return a pointer to the character in the
 *	input string where the absolute part begins.
 * Parameters:
 *	path	- pointer to the pathname whose absolute portion is
 *		  to be found.
 * Return:
 *	pointer to the absolute part of the pathname.
 * Status:
 *	private
 */
static char *
_find_abs_path(char *path)
{
	enum	parse_state {
		AFTER_SLASH,
		AFTER_FIRST_DOT,
		AFTER_SECOND_DOT
	} state;
	char	*cp;
	char	*last;

	for (cp = path, last = path, state = AFTER_SLASH; *cp; cp++) {
		switch (*cp) {
		    case '.':
			if (state == AFTER_SLASH)
				state = AFTER_FIRST_DOT;
			else if (state == AFTER_FIRST_DOT)
				state = AFTER_SECOND_DOT;
			else if (state == AFTER_SECOND_DOT)
				return (last);
			break;

		    case '/':
			if (state == AFTER_SLASH)
				last = cp;
			else if (state == AFTER_FIRST_DOT ||
					state == AFTER_SECOND_DOT) {
				last = cp;
				state = AFTER_SLASH;
			}
			break;

		    default:
			return (last);
		}
	}
}

/*
 * _map_node_to_devlink()
 *	Search the /dev/dsk or /dev/rdsk directory for a device link
 *	to the device node identified by linkbuf.  Copy the absolute
 *	pathname of that device link to the buffer pointed to by
 *	edevbuf.
 * Parameters:
 *	linkbuf	- [IN]  device node path
 *	edevbuf	- [OUT] pathname which is a symlink to the device node
 *		identified by linkbuf.
 *
 * Return:
 *	0	- search completed; edevbuf has whatever value was found
 *	1	- failure; error while scanning links in local /dev directory
 *	2	- failure; cannot read the link /<bdir>/<dev>
 * Status:
 *	private
 */
static int
_map_node_to_devlink(char *linkbuf, char *edevbuf)
{
	struct dirent	*dp;
	char	elink[MAXNAMELEN];
	DIR	*dirp;
	char	*dirname;
	int	len;

	if (strstr(linkbuf, ",raw") != NULL)
		/* This is a character device, search /dev/rdsk */
		dirname = rawdevdir;
	else
		/* This is a block device, search /dev/dsk */
		dirname = blkdevdir;
	
	if ((dirp = opendir(dirname)) == NULL)
		return (0);

	while ((dp = readdir(dirp)) != (struct dirent *)0) {
		if (strcmp(dp->d_name, ".") == 0 ||
				strcmp(dp->d_name, "..") == 0)
			continue;

		(void) sprintf(edevbuf, "%s%s", dirname, dp->d_name);

		if ((len = readlink(edevbuf, elink, MAXNAMELEN)) == -1) {
			edevbuf[0] = '\0';
			(void) closedir(dirp);
			return (1);
		}

		elink[len] = '\0';

		if (strcmp(linkbuf, elink) == 0) {
			(void) closedir(dirp);
			return (0);
		}
	}
	edevbuf[0] = '\0';
	(void) closedir(dirp);
	return (1);
}

/*
 * _map_old_device_to_new()
 *	Uses the /tmp/physdevmap.nawk.* files (if any) to map the
 *	input device name to the new name for the same device.  If
 *	the name can be mapped, copy the mapped name into newdev.
 *	Otherwise, just copy olddev to newdev.
 * Parameters:
 *	olddev	- [IN]  device name to be mapped (may have leading "../"*)
 *	newdev	- [OUT] new, equivalent name for same device.
 * Return:
 *	none
 * Status:
 *	private
 * Algorithm:
 *	If this is the first call to this routine, use the
 *	/tmp/physdevmap.nawk.* files to build a mapping array.
 *	Once the mapping array is built, use it to map olddev
 *	to the new device name.
 */
static int
_map_old_device_to_new(char *olddev, char *newdev)
{
	static int nawk_script_known_not_to_exist = 0;
	static char nawkfile[] = "physdevmap.nawk.";
	static char sh_env_value[] = "SHELL=/sbin/sh";
	char	cmd[MAXPATHLEN];
	DIR	*dirp;
	FILE	*pipe_fp;
	int	nawk_script_found;
	struct dirent	*dp;
	char	*envp;
	char	*shell_save = NULL;

	if (nawk_script_known_not_to_exist)
		return (1);

	if ((dirp = opendir("/tmp")) == NULL) {
		nawk_script_known_not_to_exist = 1;
		return (1);
	}

	nawk_script_found = 0;
	
	/*
	 * Temporarily set the value of the SHELL environment variable
	 * to "/sbin/sh" to ensure that the Bourne shell will
	 * interpret the commands passed to popen.  THen set it back to
	 * whatever it was before after doing all the popens.
	 */

	if ((envp = getenv("SHELL")) != NULL) {
		shell_save = xmalloc(strlen(envp) + 6 + 1);
		strcpy(shell_save, "SHELL=");
		strcat(shell_save, envp);
		(void) putenv(sh_env_value);
	}

	while ((dp = readdir(dirp)) != (struct dirent *)0) {
		if (strcmp(dp->d_name, ".") == 0 ||
		    strcmp(dp->d_name, "..") == 0)
			continue;

		if (strncmp(nawkfile, dp->d_name, strlen(nawkfile)) != 0)
			continue;

		nawk_script_found = 1;

		/*
		 *  This is a nawk script for mapping old device names to
		 *  new.  Now use it to try to map olddev to a new name.
		 */

		(void) sprintf(cmd, "/usr/bin/echo \"%s\" | "
			"/usr/bin/nawk -f /tmp/%s -v 'rootdir=\"%s\"' "
			"2>/dev/null", olddev, dp->d_name,
			streq(get_rootdir(), "") ? "/" : get_rootdir());

		if ((pipe_fp = popen(cmd, "r")) == NULL)
			continue;

		if (fgets(newdev, MAXPATHLEN, pipe_fp) != NULL) {
			/* remove the trailing new-line */
			newdev[strlen(newdev) - 1] = '\0';
			(void) pclose(pipe_fp);
			(void) closedir(dirp);
			if (shell_save != NULL)
				(void) putenv(shell_save);
			return (0);
		}
		pclose(pipe_fp);
	}
	(void) closedir(dirp);

	if (shell_save != NULL)
		(void) putenv(shell_save);

	if (nawk_script_found == 0)
		nawk_script_known_not_to_exist = 1;
	return (1);
}
