#ifndef lint
#pragma ident   "@(#)disk_load.c 1.82 95/01/23 SMI"
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
 * MODULE PURPOSE:	This module is responsible for building the disk list
 *			from data files
 */
#include "disk_lib.h"

/* Locally Defined Globals */

int	_diskfile_load = 0;	/* set to '1' if disk configuration came from */
/* Macros */

#define	BUFSIZE 	120

/* Public Function Prototypes */

int		load_disk_list(char *);

/* Library Function Prototypes */

/* Local Function Prototypes */

static void	parse_buffer(char *, char **, char **,
			char **, char **, char **, char **,
			char **, char **, char **, char **);
static char *	_read_line(char *, int, FILE *);
static void	_unload_disks(void);
static int 	_load_fdisk_info(char *, int, FILE *, Disk_t *);
static void 	_load_sdisk_info(char *, int, FILE *, Disk_t *);
static int 	_load_physical_geom(char *, int, FILE *, Disk_t *);
static int	_read_fdisk_header(char *, int, FILE *, Disk_t *);
static int	_scan_for_disk(char *, int, FILE *, char *, char *);

/* ******************************************************************** */
/*			PUBLIC SUPPORT FUNCTIONS			*/
/* ******************************************************************** */
/*
 * load_disk_list()
 *	Used to simulate a system disk configuration. Reads the disk
 *	configuration from 'file' and loads it into the linked list of
 *	disk structures (allocated by this routine). Returns the number
 *	of disks in the global disk list (future accesses to head ofdisk
 *	list are done with first_cyl()). If the input file has any
 *	syntax errors, this routine will stop immediately and the disk
 *	chain will be disassembled.
 *
 *	NOTE:	On fdisk systems, the UNIX slice table is loaded before the
 *		fdisk partition table. There are a number of points in the
 *		loading code which rely on this dependency. The ordering should
 *		not be changed without extensive review.
 *
 *	NOTE:	This routine should be used exclusively from the
 *		build_disk_list() routine.
 * Parameters:
 *	file	- pathname of file containing disk configurations.
 *		  This file must be in prtvtoc output format.
 * Return:
 *	# >= 0	- number of legally loaded disks in disk list
 * Status:
 *	public
 */
int
load_disk_list(char * file)
{
	Disk_t	*dp;
	char	realbuf[BUFSIZE];
	char	*buf = &realbuf[0];
	FILE	*fp;
	int	count;
	int	complete;
	char	disk[MAXNAMELEN];
	char	c;
	int	status;

	if (first_disk() != NULL ||
			(fp = fopen(file, "r")) == (FILE *)NULL)
		return (0);

	_diskfile_load++;

	buf[0] = '\0';
	count = 0;

	while (_scan_for_disk(buf, BUFSIZE, fp, disk, &c)) {
		complete = 0;

		if (find_disk(disk) != NULL)
			break;

		if ((dp = _alloc_disk(disk)) == NULL)
			break;

		/*
		 * update fdisk related globals and disk flags
		 */
		if (c == 'p') {
			/*
			 * all fdisk supporting systems have the DF_FDISKEXISTS
			 * bit set and have 16 slices
			 */
			numparts = 16;
			disk_state_set(dp, DF_FDISKEXISTS);

			/* fdisk exposure is only allowed for i386 */
			if (streq(get_default_inst(), "i386"))
				disk_state_set(dp, DF_FDISKREQ);
		}

		if ((status = _load_physical_geom(buf, BUFSIZE, fp, dp)) < 0) {
			_dealloc_disk(dp);
			continue;
		} else if (status == 1) {
			disk_initialized_on(dp);
			_init_commit_orig(dp);
			_add_disk_to_list(dp);
			count++;
			complete = 1;
			continue;
		}

		_load_sdisk_info(buf, BUFSIZE, fp, dp);

		if ((status = _load_fdisk_info(buf, BUFSIZE, fp, dp)) < 0) {
			_dealloc_disk(dp);
			continue;
		}

		/*
		 * copy the current F-Disk and S-Disk configuration into
		 * the committed and original stores, and add the drive to
		 * the list of drives on the system
		 */
		disk_initialized_on(dp);
		_init_commit_orig(dp);
		_add_disk_to_list(dp);
		count++;
		complete = 1;
	}

	(void) fclose(fp);

	if (complete) {
		/*
		 * mark the boot disk and recommit (existing must be
		 * defined at this point
		 */
		if ((dp = _init_bootdisk()) != NULL)
			_init_commit_orig(dp);

		return (count);
	}

	_unload_disks();
	return (0);
}

/* ******************************************************************** */
/*			LOCAL SUPPORT FUNCTIONS				*/
/* ******************************************************************** */
/*
 * _load_sdisk_info()
 *	Load the information for slices from the prtvtoc output. If there
 *	are no slice entries then the disk is considered to have no S-disk
 *	label.
 * Parameters:
 *	buf	- buffer to used for input file scanning
 *	n	- size of 'buf'
 *	fp	- open file pointer for input file
 *	dp	- non-NULL pointer to disk structure being initialized
 * Return:
 *	none
 * Status:
 *	private
 */
static void
_load_sdisk_info(char *buf, int n, FILE *fp, Disk_t *dp)
{
	char	*f1, *f2, *f4, *f5, *f7, *d;
	int	count = 0;
	int	i;
	int	over;
	int	size;
	char	sbuf[BUFSIZE];

	/*
	 * null the disk rather than reset, because you don't know whether the
	 * fdisk is required at this point or not
	 */
	(void) _null_sdisk(dp);

	while (_read_line(buf, n, fp) != NULL) {
		sbuf[0] = '\0';
		(void) strcpy(sbuf, buf);

		parse_buffer(sbuf, &f1, &f2, &d, &f4, &f5, &d, &f7, &d, &d, &d);

		if (isdigit(*f1) == 0 || isdigit(*f2) == 0 ||
				isdigit(*f4) == 0 || isdigit(*f5) == 0)
			break;

		i = atoi(f1);

		if (invalid_sdisk_slice(i))
			continue;

		/*
		 * since we are immediately rounding the starting cylinder up
		 * to the nearest cylinder boundary, we may need to adjust the
		 * size accordingly, and mark the slice as SLF_ALIGNED so that
		 * it cannot be preserved. If the size is not cylinder aligned,
		 * an adjustment is also made and the slice marked.
		 */
		size = atoi(f5);

		slice_start_set(dp, i, blocks_to_cyls(dp, atoi(f4)));

		over = atoi(f4) % one_cyl(dp);

		if (over > 0) {
			slice_aligned_on(dp, i);
			size -= (one_cyl(dp) - over);
		}

		slice_size_set(dp, i, blocks_to_blocks(dp, size));

		if (slice_size(dp, i) != 0) {
			if (f7 != NULL) {
				(void) set_slice_mnt(dp, i, f7, (char *)NULL);
			} else {
				switch (atoi(f2)) {
				    case V_ROOT:
					(void) set_slice_mnt(dp, i, ROOT, NULL);
					break;
				    case V_SWAP:
					(void) set_slice_mnt(dp, i, SWAP, NULL);
					break;
				    case V_BACKUP:
					(void) set_slice_mnt(dp, i,
						OVERLAP, NULL);
					break;
				    case V_CACHE:
					/*
					 * NOTE: the assumption that V_CACHE ==
					 * /.cache is true as of Solaris 2.5,
					 * but may not always be true
					 */
					(void) set_slice_mnt(dp, i,
						CACHE, NULL);
					break;
				    default:
					break;
				}
			}
		}
		count++;
	}

	_mark_overlap_slices(dp);	/* mark overlap slices */

	if (count == 0)
		sdisk_state_set(dp, SF_NOSLABEL);
}

/*
 * _unload_disks()
 *	Disassemble the disk list and null out the head pointer.
 * Parameters:
 *	none
 * Return:
 *	none
 * Status:
 *	private
 */
static void
_unload_disks(void)
{
	Disk_t	*dp;
	int	i;

	WALK_DISK_LIST(dp) {
		WALK_SLICES(i)
			slice_mntopts_clear(dp, i);

		free(dp);
	}

	_set_first_disk(NULL);
}

/*
 * parse_buffer()
 *	Parse out value from string passed in.
 * Parameters:
 *	buf	-
 *	s1	-
 *	s2	-
 *	s3	-
 *	s4	-
 *	s5	-
 * Return:
 *	none
 * Status:
 *	private
 */
static void
parse_buffer(char * buf, char ** s1, char ** s2, char ** s3, char ** s4,
		char ** s5, char ** s6, char ** s7, char ** s8, char ** s9,
		char ** s10)
{
	char	*cp;
	int	f = 2;

	/* consume leading blanks */
	for (cp = buf; *cp != '\0' && (*cp == ' ' || *cp == '\t'); cp++);

	*s1 = cp;
	*s2 = *s3 = *s4 = *s5 = *s6 = *s7 = *s8 = *s9 = *s10 = (char *) NULL;
	for (; *cp != '\0'; cp++) {
		if (*cp == ' ' || *cp == '\t') {
			*cp = '\0';
			for (cp++; *cp == ' ' || *cp == '\t'; cp++);

			switch (f) {

			case 2:	 *s2 = cp;	break;
			case 3:	 *s3 = cp;	break;
			case 4:	 *s4 = cp;	break;
			case 5:	 *s5 = cp;	break;
			case 6:	 *s6 = cp;	break;
			case 7:	 *s7 = cp;	break;
			case 8:	 *s8 = cp;	break;
			case 9:	 *s9 = cp;	break;
			case 10: *s10 = cp;	break;
			}

			f++;
		}
	}
}

/*
 * _read_line()
 *	Read lines from file 'fp'. Strip off leading blanks, and skip
 *	blank lines.
 * Parameters:
 *	buf	- buffer to retrieve data in
 *	n	- number of characters to fetch (buffer size)
 *	fp	- file pointer for file to read
 * Return:
 *	NULL	- EOF
 *	'buf'	- parameter with data filled in buffer
 * Status:
 *	private
 */
static char *
_read_line(char * buf, int n, FILE * fp)
{
	char	*cp;

	do {
		if (fgets(buf, n, fp) == NULL) {
			*buf = '\0';
			return ((char *)NULL);
		}

		for (cp = buf; *cp == ' ' || *cp == '\t'; cp++);
	} while (*cp == '\n');

	buf[strlen(buf) - 1] = '\0';	/* null out newline */
	return (buf);
}

/*
 * _load_fdisk_info()
 *	Load the physical drive geometry and the F-disk partition data from
 *	the input file.
 * Parameters:
 *	buf	- buffer to used for input file scanning
 *	n	- size of 'buf'
 *	fp	- open file pointer for input file
 *	dp	- non-NULL pointer to disk structure being initialized
 * Return:
 *	-1	- load failed; deallocate the drive
 *	 0	- load succeeded
 *	 1	- load failed; no further processing needed for drive
 * Status:
 *	private
 */
static int
_load_fdisk_info(char *buf, int n, FILE *fp, Disk_t *dp)
{
	char	*id, *active, *rsect, *tsect, *d;
	int	p = 1;
	int	status;
	int	pid;
	int	lcyl;
	int	lsect;
	int	origpart = 1;

	/* routine only required for systems supporting fdisk */
	if (disk_no_fdisk_exists(dp))
		return (0);

	/*
	 * clear the S-disk geometry pointer (reset later if there turns out
	 * to be a Solaris partition) and reset all fdisk partition entries
	 * and status fields
	 */
	sdisk_geom_clear(dp);
	(void) _reset_fdisk(dp);

	/* save the prtvtoc header values */
	lcyl = disk_geom_lcyl(dp);
	lsect = disk_geom_lsect(dp);

	/* read in the geometry header lines */
	if ((status = _read_fdisk_header(buf, n, fp, dp)) != 0)
		return (status);

	/* scan in the partition table; "fdisk -d" always produces 4 lines */
	for (p = 1; _read_line(buf, n, fp) != NULL &&
			valid_fdisk_part(p); p++) {
		/*
		 * set nsect, nhead, and onecyl from the main disk
		 * geometry structure (nsect/nhead/onecyl)
		 */
		part_geom_nsect(dp, p) = disk_geom_nsect(dp);
		part_geom_nhead(dp, p) = disk_geom_nhead(dp);
		part_geom_onecyl(dp, p) = disk_geom_onecyl(dp);
		part_geom_hbacyl(dp, p) = disk_geom_hbacyl(dp);

		parse_buffer(buf, &id, &active, &d, &d, &d, &d,
					&d, &d, &rsect, &tsect);
		if (!isdigit(*id) || !isdigit(*active) ||
				!isdigit(*rsect) || !isdigit(*tsect)) {
			break;
		}

		/*
		 * set the partition type and active state (id/active) and
		 * original parititon number
		 */
		part_id_set(dp, p, atoi(id));
		part_active_set(dp, p, atoi(active));
		part_orig_partnum(dp, p) = origpart++;

		/*
		 * set the relative offset sector and total cyl/sector
		 * (rsect/tsect/tcyl) on used partitions only
		 */
		if (part_id(dp, p) != UNUSED) {
			part_geom_rsect(dp, p) = atoi(rsect);
			part_geom_tsect(dp, p) = atoi(tsect);
			part_geom_tcyl(dp, p) = blocks_to_cyls(dp,
					part_geom_tsect(dp, p));
		}

		/*
		 * set the first data cylinder, last data cylinder/sectors
		 * (firstcyl/lcyl/lsect)
		 */
		if (part_id(dp, p) == SUNIXOS) {
			/*
			 * set the first data cylinder (firstcyl) to offset the
			 * boot cylinder
			 */
			part_geom_firstcyl(dp, p) = 1;
			/*
			 * reset the last data cylinder (lcyl/lsect) to the
			 * values found in the prtvtoc header
			 */
			part_geom_lcyl(dp, p) = lcyl;
			part_geom_lsect(dp, p) = lsect;
		} else {
			/*
			 * non-Solaris partitions start at the beginning of the
			 * partition, and have no alternate cylinders
			 */
			part_geom_firstcyl(dp, p) = 0;
			part_geom_lcyl(dp, p) = part_geom_tcyl(dp, p);
			part_geom_lsect(dp, p) =
				cyls_to_blocks(dp, part_geom_lcyl(dp, p));
		}

		/* set the data sector/cylinder (dcyl/desect) */
		part_geom_dcyl(dp, p) = part_geom_lcyl(dp, p) -
				part_geom_firstcyl(dp, p);
		part_geom_dsect(dp, p) = part_geom_dcyl(dp, p) * one_cyl(dp);

		if (part_geom_dcyl(dp, p) < 0)
			part_geom_dcyl(dp, p) = 0;

		if (part_geom_dsect(dp, p) < 0)
			part_geom_dsect(dp, p) = 0;

		if (part_geom_lcyl(dp, p) < 0)
			part_geom_lcyl(dp, p) = 0;

		if (part_geom_lsect(dp, p) < 0)
			part_geom_lsect(dp, p) = 0;

		if (part_geom_firstcyl(dp, p) > part_geom_tcyl(dp, p))
			part_geom_firstcyl(dp, p) = 0;
	}

	_sort_fdisk_input(dp);

	/*
	 * after sorting the partition table, initialize the sdisk
	 * geometry pointer if there is a Solaris partition. On PowerPC,
	 * if there is no Solaris partition then mark the disk as
	 * having no physical geometry and return it as unusable
	 */
	if ((pid = get_solaris_part(dp, CFG_CURRENT)) > 0)
		sdisk_geom_set(dp, part_geom_addr(dp, pid));
	else if (streq(get_default_inst(), "ppc")) {
		disk_state_set(dp, DF_NOPGEOM);
		return (1);
	}

	return (0);
}

/*
 * _read_fdisk_header()
 *	Load the phyical and (on i386) HBA disk geometry into the disk_geom
 *	structure. This will overwrite what was originally (and erroneously)
 *	loaded from the prtvtoc output (which is really only Solaris partition
 *	data) If successful, this routine will leave the input file pointer
 *	ready to read F-disk partition data (if any exist).
 * Parameters:
 *	buf	- common buffer for reading input from disk file
 *	n	- size of 'buf'
 *	dp	- non-NULL disk structure pointer
 * Return:
 *	-1	- read failed; this is fatal to the loading process
 *	 0	- read succeeded
 *	 1	- read failed, but only for this drive
 * Status:
 *	private
 */
static int
_read_fdisk_header(char *buf, int n, FILE *fp, Disk_t *dp)
{
	int	i;
	int	d;
	int	hcyl;
	int	hhead;
	int	hsect;
	int	pcyl;
	int	phead;
	int	psect;

	/*
	 * make sure the current line representes the beginning of Fdisk
	 * dry-run input data
	 */
	if (strncmp(buf, "fdisk", 5) != 0)
		return (-1);

	/*
	 * load the physical geometry line (DKIOC_PHYGEOM)
	 */
	if (_read_line(buf, n, fp) == NULL)
		return (-1);

	if (sscanf(buf,
"cylinders[%d] heads[%d] sectors[%d] sector size[%d] blocks[%d] mbytes[%d]",
			&pcyl, &phead, &psect, &d, &d, &d) != 6)
		return (-1);

	/*
	 * the original disk geometry was loaded from the prtvtoc label data;
	 * this data viewed the Solaris partition as the "entire disk". At this
	 * point we know this is not the case, so we need to make the necessary
	 * modifications in the disk geometry to reflect the true "total
	 * cylinders" first cylinder, and data cylinder; remember that the
	 * solaris label was created with DKIOCG_PHYGEOM, so we need to make
	 * sure we pick up our data from the physical geometry line of the
	 * fdisk output; we do not need to change the sectors/cylinder, nhead,
	 * or nsect fields because the basic cylinder geometry has not changed
	 * from DKIOCG_PHYGEOM
	 */
	disk_geom_firstcyl(dp) = 1;
	disk_geom_tcyl(dp) = pcyl;
	disk_geom_tsect(dp) = cyls_to_blocks(dp, pcyl);
	disk_geom_lcyl(dp) = pcyl;
	disk_geom_lsect(dp) = cyls_to_blocks(dp, pcyl);
	disk_geom_dcyl(dp) = (pcyl > 0 ? pcyl - 1 : 0);
	disk_geom_dsect(dp) = cyls_to_blocks(dp, disk_geom_dcyl(dp));

	/*
	 * skip forward 2 lines
	 */
	for (i = 0; i < 2; i++) {
		if (_read_line(buf, n, fp) == NULL)
			return (-1);
	}

	/* parse in the HBA geometry line (DKIOCG_VIRTGEOM) */
	if (sscanf(buf,
"cylinders[%d] heads[%d] sectors[%d] sector size[%d] blocks[%d] mbytes[%d]",
			&hcyl, &hhead, &hsect, &d, &d, &d) != 6)
		return (-1);

	/* update the hbacyl on i386 only */
	if (streq(get_default_inst(), "i386"))
		disk_geom_hbacyl(dp) = hhead * hsect;

	/* skip 2 lines to position input to read in partition entries */
	for (i = 0; i < 2; i++) {
		if (_read_line(buf, n, fp) == NULL)
			return (-1);
	}

	/* look for the fdisk partition table header */
	if (strncmp(buf, "SYSID", 5) != 0)
		return (-1);

	return (0);
}

/*
 * _scan_for_disk()
 *	Scan forward through the the input file for the pattern:
 *
 *		"*" "<disk>" ... "partition map"
 *
 *	This is the pattern produced at the head of prtvtoc output for a given
 *	drive. If found, set 'disk' to the simplified drive name (i.e. c0t0d0)
 *	and return whether the device is p0 (fdisk system) or s2 (non-fdisk
 *	system)
 *
 *	NOTE:	The "partition map" line on fdisk system prtvtoc output differs
 *		from SPARC. There is an additional field after the "<disk>" line
 *		indicating the volume name. For this reason, this searching
 *		algorithm should NOT rely on explicit format, but instead on the
 *		existence of the pattern "partition map".
 *
 *	NOTE:	<disk> is expected to be either c#[t#]d#s2 or c#[t#]d#p0
 * Parameters:
 *	buf	- buffer to used for input file scanning
 *	n	- size of 'buf'
 *	fp	- open file pointer for input file
 *	disk	- array to return simplified disk name in if found (set to
 *		  "" if no disk found)
 *	type	- return a value of '', 'p', or 's' to indicate drive type
 * Return:
 *	0	- didn't find a drive name
 *	1	- found a drive name
 * Status:
 *	private
 */
static int
_scan_for_disk(char *buf, int n, FILE *fp, char *disk, char *type)
{
	char	*f2;
	char	*d;

	do {
		/*
		 * only concerned with lines which start with '*' and contain
		 * the pattern "partition map"
		 */
		if (buf[0] != '*' || strstr(buf, "partition map") == NULL)
			continue;

		parse_buffer(buf, &d, &f2, &d, &d, &d, &d, &d, &d, &d, &d);

		if (simplify_disk_name(disk, f2) == 0) {
			*type = (f2[strlen(f2) - 2] == 'p' ? 'p' : 's');
			return (1);
		}
	} while (_read_line(buf, n, fp) != NULL);

	disk[0] = '\0';
	*type = '\0';
	return (0);
}

/*
 * _load_physical_geom()
 *	Load the physical disk geometry information from the prtvtoc() data
 *	area. For non-i386 systems this is the actual drive data. On i386
 *	systems, this data virtual SOLARIS partition data and will have to be
 *	modified later in the input process. The input file pointer is left at
 *	the head of the prtvtoc() slice table entries.
 * Parameters:
 *	buf	- buffer to used for input file scanning
 *	n	- size of 'buf'
 *	fp	- open file pointer for input file
 *	dp	- non-NULL pointer to disk structure being initialized
 * Return:
 *	-1	- load failed; this is fatal to the loading process
 *	 0	- load succeeded
 *	 1	- load failed, but only for this drive
 * Status:
 *	private
 */
static int
_load_physical_geom(char *buf, int n, FILE *fp, Disk_t *dp)
{
	char	*f2;
	char	*f3;
	char	*d;
	int	i;

	/* set the first cylinder and relative offset sector (firstcyl/rsect) */
	disk_geom_firstcyl(dp) = 0;
	disk_geom_rsect(dp) = 0;

	/* set the bytes/sector field (nsect) */
	for (i = 0; i < 3; i++)
		if (_read_line(buf, n, fp) == NULL || *buf != '*')
			return (-1);

	parse_buffer(buf, &d, &f2, &d, &d, &d, &d, &d, &d, &d, &d);
	if (!isdigit(*f2))
		return (-1);

	disk_geom_nsect(dp) = atoi(f2);

	/* set the number of heads (nhead) */
	for (i = 0; i < 2; i++) {
		if (_read_line(buf, n, fp) == NULL || *buf != '*')
			return (-1);
	}

	parse_buffer(buf, &d, &f2, &d, &d, &d, &d, &d, &d, &d, &d);
	if (!isdigit(*f2))
		return (-1);

	disk_geom_nhead(dp) = atoi(f2);

	/* set the sectors/cyl (onecyl) */
	if (_read_line(buf, n, fp) == NULL || *buf != '*')
		return (-1);

	parse_buffer(buf, &d, &f2, &d, &d, &d, &d, &d, &d, &d, &d);
	if (!isdigit(*f2))
		return (-1);

	disk_geom_onecyl(dp) = atoi(f2);
	disk_geom_hbacyl(dp) = disk_geom_onecyl(dp);

	/* set the total cylinders (tcyl/tsect) */
	if (_read_line(buf, n, fp) == NULL || *buf != '*')
			return (-1);

	parse_buffer(buf, &d, &f2, &d, &d, &d, &d, &d, &d, &d, &d);
	if (!isdigit(*f2))
		return (-1);

	disk_geom_tcyl(dp) = atoi(f2);
	disk_geom_tsect(dp) = cyls_to_blocks(dp, disk_geom_tcyl(dp));

	/* set the accessible cylinders (lcyl/lsect) */
	if (_read_line(buf, n, fp) == NULL || *buf != '*')
			return (-1);

	parse_buffer(buf, &d, &f2, &d, &d, &d, &d, &d, &d, &d, &d);
	if (!isdigit(*f2))
		return (-1);

	disk_geom_lcyl(dp) = atoi(f2);
	disk_geom_lsect(dp) = cyls_to_blocks(dp, disk_geom_lcyl(dp));

	/* set the number of data cylinders (dcyl/dsect) */
	disk_geom_dcyl(dp) = disk_geom_lcyl(dp);
	disk_geom_dsect(dp) = disk_geom_lsect(dp);

	/*
	 * pick up the controller specifier (if it exists). This is a hand
	 * created line which does not exist in normal prtvtoc output, but
	 * which permits testing of SCSI versus non-SCSI drive configurations
	 * for IDE alternate sector testing
	 */
	if (_read_line(buf, n, fp) == NULL || *buf != '*')
		return (-1);

	parse_buffer(buf, &d, &f2, &f3, &d, &d, &d, &d, &d, &d, &d);
	if (f2 && strcmp(f2, "Controller:") == 0 &&
			f3 && strcmp(f3, "IDE") == 0) {
		disk_ctype_set(dp, DKC_DIRECT);
		disk_cname_set(dp, "ide");
	} else {
		disk_ctype_set(dp, DKC_SCSI_CCS);
		disk_cname_set(dp, "scsi");
	}
			/* read rest of lines to the slice table header */
	for (;;) {
		if (_read_line(buf, BUFSIZE, fp) == NULL)
				return (-1);

		parse_buffer(buf, &d, &f2, &d, &d, &d, &d, &d, &d, &d, &d);

		if (f2 && strcmp(f2, "Partition") == 0)
			break;
	}

	if (disk_geom_tcyl(dp) == 0)
		return (1);

	/* set disk to "okay" state; this may later be reversed */
	disk_state_unset(dp, DF_NOTOKAY);
	return (0);
}
