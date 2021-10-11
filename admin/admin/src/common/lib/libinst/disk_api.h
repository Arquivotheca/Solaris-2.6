#ifndef	lint
#pragma ident "@(#)disk_api.h 1.130 95/09/08"
#endif
/*
 * Copyright (c) 1991-1995 Sun Microsystems, Inc.  All Rights Reserved.
 * Sun considers its source code as an unpublished, proprietary
 * trade secret, and it is available only under strict license
 * provisions.  This copyright notice is placed here only to protect
 * Sun in the event the source is deemed a published work.  Dissassembly,
 * decompilation, or other means of reducing the object code to human
 * readable form is prohibited by the license agreement under which
 * this code is provided to the user or company in possession of this
 * copy.
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 52.227-7013 and in similar clauses in the FAR and
 * NASA FAR Supplement.
 */

#ifndef	_DISK_API_H
#define	_DISK_API_H

#include <sys/fs/ufs_fs.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/vtoc.h>
#include <sys/dkio.h>
#include <libintl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>

#include "sw_api.h"

/*
 * <sys/dktp/fdisk.h> SHOULD BE IN THE 494 MERGED SOURCE TREE. THE FOLLOWING
 * FIELDS WERE HAND INCLUDED DURING DEVELOPMENT AND SHOULD BE REMOVED FOR FCS
 */
#ifdef __i386
#include <sys/dktp/fdisk.h>
#else
#define	UNUSED		100
#define	SUNIXOS		130
#define	DOSOS12		1	/* DOS partition, 12-bit FAT */
#define	PCIXOS		2	/* PC/IX partition */
#define	DOSDATA		86	/* DOS data partition */
#define	DOSOS16		4	/* DOS partition, 16-bit FAT */
#define	EXTDOS		5	/* EXT-DOS partition */
#define	DOSHUGE		6	/* Huge DOS partition  > 32MB */
#define	OTHEROS		98	/* part. type for appl. (DB?) needs raw partition */
				/* ID was 0 but conflicted with DOS 3.3 fdisk    */
#define	UNIXOS		99	/* UNIX V.x partition */

#define	MAXDOS		65535L	/* max size (sectors) for DOS partition */
#define	FD_NUMPART	4
#define	NOTACTIVE	0	/* not active constant */
#define	ACTIVE		128	/* active constant */
#endif

/* number of slice partitions in the disk label */
#ifndef	NUMPARTS	/* ifndef used so that compile line can override */
#define	NUMPARTS	NDKMAP
#endif

/* sizing rounding specifiers */
#define	ROUNDDOWN	0	/* round down to the nearest unit */
#define	ROUNDUP		1	/* round up to the nearest unit */

/* minimum file system size (10MB) in sectors */
#define	MINFSSIZE	20480

/* preservation state specifiers */
#define	PRES_NO		 0	/* do not preserve the state */
#define	PRES_NULL	-1	/* NULL value */
#define	PRES_YES	 1	/* preserve the state */

/* ignore state specifiers */
#define	IGNORE_NO	 0	/* clear the ignore state */
#define	IGNORE_YES	 1	/* set the ignore state */

/*
 * Default system mount data structure
 */
typedef struct {
	char	name[16];	/* filesystem name */
	int	status;		/* 0, 1, 2 */
	int	slice;		/* default slice (-1 if don't care) */
	int	allowed;	/* flag specifying if allowed to be indep */
	int	size;		/* number of sectors required for software */
	int	expansion;	/* expansion sector count for hosting fs */
} Defmnt_t;

/*
 * Geometry structure containing information about either the physical or
 * virtual drive. One is associated with the Disk_t structure, and one for
 * each F-disk partition (if F-disk labelling is being used). Note that the
 * 'dcyl' and 'tcyl' values are used for slices and new partitions, whereas
 * 'dsect' and 'tsect' are only used for existing (DOS generated) F-disk
 * partitions
 */
typedef struct {
	int	firstcyl;	/* last legal data cylinder on the drive */
	int	lcyl;		/* total # of accessible cylinders on drive */
	int	dcyl;		/* total # of data cylinders on drive */
	int	tcyl;		/* total # of cylinders on drive */
	int	onecyl;		/* # of sectors per cylinder (physical) */
	int	hbacyl;		/* # of sectors per cylinder (HBA) */
	int	rsect;		/* relative offset sector for the drive */
	int	nsect;		/* # of sectors per track (physical) */
	int	lsect;		/* total # of accessible sectors on the drive */
	int	dsect;		/* total # of data sectors on the drive */
	int	tsect;		/* total # of sectors on the drive */
	int	nhead;		/* total numbers tracks per cylinder (physical) */
} Geom_t;

/*
 * Slice structure (current model assumes no meta-disk device support
 * and file systems are 1:1 matched to slices)
 */
typedef struct {
	u_char	state;			/* slice state */
	int	start;			/* starting cylinder # */
	int	size;			/* # of sectors in the slice */
	char	mntpnt[MAXMNTLEN];	/* pathname for slice mount point */
	char	*mntopts;		/* file system mount options */
} Slice_t;

/*
 * F-Disk partition structure (one per partition entry)
 */
typedef struct {
	u_char	state;		/* partition state */
	int	id;		/* partition type id */
	int	origpart;	/* original actual partition number */
	int	active;		/* 0 (inactive) or 128 (active)	*/
	Geom_t	geom;		/* geometry structure */
} Part_t;

/*
 * F-disk data structure continain all partition specific data
 */
typedef struct {
	u_char	state;			/* F-disk state	*/
	Part_t	part[FD_NUMPART];	/* F-disk partitions */
} Fdisk_t;

/*
 * Solaris disk (S-disk) data structure containing all label related data
 */
typedef struct {
	u_char	state;		/* Solaris disk state */
	Geom_t	*geom;		/* Solaris disk geometry ref ptr */
	Slice_t slice[16];	/* Solaris slices */
} Sdisk_t;

/*
 * Disk drive data structure - one disk structure per physical disk device
 */
typedef struct disk {
	char	name[16];	/* short name for disk device (c0t0d0) */

	u_short	state;		/* disk state variable */
	u_short ctype;		/* disk controller type (from DKIOCINFO) */
	char	cname[16];	/* disk controller name (DK_DEVLEN == 16) */
	Geom_t	geom;		/* physical disk geometry data */

	Fdisk_t	fdisk;		/* current f-disk information */
	Sdisk_t	sdisk;		/* current slice information */

	Fdisk_t	c_fdisk;	/* last committed copy of fdisk table */
	Sdisk_t c_sdisk;	/* last committed copy of Solaris label	*/

	Fdisk_t	o_fdisk;	/* original state of f-disk table */
	Sdisk_t	o_sdisk;	/* original state of Solaris label */

	struct disk *next;	/* pointer to next disk in chain */
} Disk_t;

#define	NULLDISK	((Disk_t *)0)

/* struct for passing params in find_mnt_pnt() */

typedef struct mnt_pnt {
	Disk_t	*dp;		/* pointer to disk structure with mnt pnt */
	int	slice;		/* slice index */
} Mntpnt_t;

/* Swap resource structures */

struct swap_res {
	int		*explicit;	/* explicit swap requested */
	struct swap_obj	*next;		/* list of swap objects */
};

/* Swap object structure */

struct swap_obj {
	u_char		type;		/* type of swap object */
	char		*name;		/* name of swap object */
	struct swap_obj	*next;		/* next swap object in list */
};

/*
 * Linked list structure for string names
 */
typedef struct namelist {
	char		*name;
	struct namelist	*next;
} Namelist;

/*
 * Linked list structure for storage objects 
 */
typedef struct storage {
	char		*dev;
	char		*name;
	char		*size;
	char		*mntopts;
	int		preserve;
	struct storage	*next;
} Storage;

/*
 * Linked list structure for software modules
 */
typedef struct software_unit {
	char			*name;
	int			delta;
	struct software_unit	*next;
} Sw_unit;

/* Fdisk structure constants */

#define	FD_SIZE_DELETE		 0
#define	FD_SIZE_ALL		-1
#define	FD_SIZE_MAXFREE		-2

/*
 * Fdisk keyword data structure.
 *	flags	- 32 bit flag field
 *	part	- 0 (any) or 1-4 (explicit)
 * 	id	- partition type
 *	size	-  0 - unused
 *		  -1 - all
 *		  -2 - maxfree
 */
typedef struct fdisk {
	char		*disk;		/* cX[tX]dX */
	u_long		flags;		/* see #defines above */
	int		part;		/* partition # (1-4) */
	int		id;		/* partition ID */
	int		size;		/* explicit partition size */
	struct fdisk	*next;
} Fdisk;

/*
 * Linked list structure for client services
 */
typedef struct {
	int		num_clients;	/* # diskless clients */
	int		client_root;	/* explicit size of per-client root (MB) */
	int		client_swap;	/* explicit size of per-client swap (MB) */
	Namelist	*karchs;	/* list of supported client archs */
} Services;

/*
 * swap resource data structure 
 */
typedef struct {
	int	total;		/* user specified required swap */
} Swap;

/*
 * Master disk work profile structure
 */
typedef struct {
	int		partitioning;
	Storage		*filesys;
	Fdisk		*fdisk;
	Namelist	*use;
	Namelist	*dontuse;
} Disk;

/*
 * Software configuration data structure
 */
typedef struct software {
	char 		*meta;
	Sw_unit		*cluster;
	Sw_unit		*pkg;
	Namelist	*lang;
	Module 		*prod;
} Software;

/*
 * Command line pfinstall parameters
 */
typedef struct param {
	MachineType	sys_type;	/* system type */
	char		*pro_file;	/* profile file name */
	char		*media;		/* media specified */
	char		*disk_file;	/* disk file */
	char		*root_disk;	/* explicit root disk specifier */
	int		noreboot;	/* noreboot state flag */
} Param;

/*
 * Data structure representing profile specification
 */
typedef struct profile {
	Param		param;		/* execution parameters */
	Software	software;	/* software structure */
	Swap		swap;		/* swap resources */
	Disk		disk;		/* disk specification structure */
	Remote_FS	*remote;	/* remote file systems */
	Services	services;	/* required services */
} Profile;

/*
 * Miscellaneous macros and constants
 */
#define	DEFAULT_FS_FREE		15	/* default UFS free space */

#define	DISKPARTITIONING(x)	(x)->disk.partitioning
#define	DISKFILESYS(x)		(x)->disk.filesys
#define	DISKFDISK(x)		(x)->disk.fdisk
#define	DISKUSE(x)		(x)->disk.use
#define	DISKDONTUSE(x)		(x)->disk.dontuse
#define	SYSTYPE(x)		(x)->param.sys_type
#define	PROFILE(x)		(x)->param.pro_file
#define	DISKFILE(x)		(x)->param.disk_file
#define	ROOTDISK(x)		(x)->param.root_disk
#define	MEDIANAME(x)		(x)->param.media
#define	NOREBOOT(x)		(x)->param.noreboot
#define	REMOTEFS(x)		(x)->remote
#define	SERVICES(x)		(x)->services
#define	CLIENTROOT(x)		SERVICES((x)).client_root
#define	CLIENTSWAP(x)		SERVICES((x)).client_swap
#define	CLIENTCNT(x)		SERVICES((x)).num_clients
#define	CLIENTPLATFORM(x)	SERVICES((x)).karchs
#define	SOFTWARE(x)		(x)->software
#define	TOTALSWAP(x)		(x)->swap.total
#define	LOCALES(x)		SOFTWARE((x)).lang
#define	CLUSTERS(x)		SOFTWARE((x)).cluster
#define	SWPACKAGES(x)		SOFTWARE((x)).pkg
#define	SWPRODUCT(x)		SOFTWARE((x)).prod
#define	METACLUSTER(x)		SOFTWARE((x)).meta

#define	ci_streq(a, b)		(strcasecmp(a, b) == 0)
#define	ci_strneq(a, b)		(strcasecmp(a, b) != 0)
#define	streq(a, b)		(strcmp(a, b) == 0)
#define	strneq(a, b)		(strcmp(a, b) != 0)

/* Globals */

extern char	err_text[];
extern int	numparts;

/* Error return codes */

#define	D_OK		  0  /* no error */
#define	D_NODISK	 -1  /* invalid disk structure pointer */
#define	D_BADARG	 -2  /* incorrect argument to func */
#define	D_NOSPACE	 -3  /* not enough space on disk */
#define	D_DUPMNT	 -4  /* duplicate mount point */
#define	D_IGNORED	 -5  /* action failed because slice marked "ignored" */
#define	D_CHANGED	 -6  /* can't preserve a slice that has been changed */
#define	D_CANTPRES	 -7  /* this mount point can't be preserved */
#define	D_PRESERVED	 -8  /* can't update a preserved slice */
#define	D_BADDISK	 -9  /* can't update this disk */
#define	D_OFF		-10  /* slice extends off disk */
#define	D_ZERO		-11  /* size is less then 1 cylinder */
#define	D_OVER		-12  /* overlapping slices/partitions */
#define	D_ILLEGAL	-13  /* S-Disk is in an illegal configuration */
#define	D_ALTSLICE	-14  /* cannot modify the alternate sector slice */
#define	D_BOOTFIXED	-15  /* UNUSED */
#define	D_NOTSELECT	-16  /* action req'd not done; disk state not selected */
#define	D_LOCKED	-17  /* slice preserve state locked; can't change */
#define	D_GEOMCHNG	-18  /* disk geometry changed */
#define	D_NOGEOM	-19  /* no disk geometry defined */
#define	D_NOFIT		-20  /* slice/partition doesn't fit in disk segment */
#define	D_NOSOLARIS	-21  /* no Solaris partition configured on the disk */
#define	D_BADORDER	-22  /* slices/partitions not in a legal ordering */
#define	D_OUTOFREACH	-23  /* Solaris part (or a component accessed by BIOS) */
				/* cannot be reached by the BIOS code */
#define	D_SMALLSWAP	-25  /* there is less swap configured than what is req */
#define	D_ALIGNED	-26  /* the starting cyl/size were aligned at load */
#define	D_FAILED	-27  /* generic failure */

/* check disk warning return codes */

#define	D_UNUSED	  1 /* there is unused space on the slice */
#define	D_BOOTCONFIG	  2 /* "/" is not on the default boot disk */

/* set_slice_geom() source specifiers */

#define	GEOM_ORIG	-1	/* existing (size and start) */
#define	GEOM_REST	-2	/* free (size only) */
#define	GEOM_COMMIT	-3	/* committed value (size and start) */
#define	GEOM_IGNORE	-4	/* ignore this parameter */

/* disk display units */

typedef enum {
	D_MBYTE = 0,
	D_KBYTE = 1,
	D_BLOCK = 2,
	D_CYLS = 3
} Units_t;

/* label source */

typedef enum {
	CFG_NONE = 0,
	CFG_DEFAULT = 1,
	CFG_EXIST = 2,
	CFG_COMMIT = 3,
	CFG_CURRENT = 4
} Label_t;

/* error message linked list message storage format */
typedef struct errmsg {
	int		code;
	char		*msg;
	struct errmsg	*next;
} Errmsg_t;

/* values for disk state field */

#define	DF_BADCTRL	0x0001	/* controller access failed		*/
#define	DF_UNKNOWN	0x0002	/* unknown controller type		*/
#define	DF_NOPGEOM	0x0004	/* unknown physical geometry information */
#define	DF_CANTFORMAT	0x0008	/* drive cannot be fmttd - hardware failure */
#define	DF_FDISKREQ	0x0080	/* fdisk exposure to the user required */
#define	DF_BOOTDRIVE	0x0100	/* drive explicitly tagged to house '/'	*/
#define	DF_FORMAT	0x0200	/* format S-disk specified for drive 	*/
#define	DF_SELECTED	0x0400	/* drive selected for processing 	*/
#define	DF_FDISKEXISTS	0x1000	/* fdisk physically supported for this disk */
#define	DF_INIT		0x2000	/* disk structure initialization completed */

#define	DF_UNUSABLE	(DF_BADCTRL|DF_UNKNOWN|DF_CANTFORMAT)
#define	DF_NOTOKAY	(DF_BADCTRL|DF_UNKNOWN|DF_NOPGEOM|DF_CANTFORMAT)
#define	DF_INITIAL 	(DF_NOTOKAY)

/* values for s-disk state field */

#define	SF_S2MOD	0x01	/* slice 2 was modified	*/
#define	SF_ILLEGAL	0x02	/* illegal S-disk configuration */
#define	SF_NOSLABEL	0x04	/* Solaris label missing */

/* values for f-disk state field */

#define	FF_NOFLABEL	0x01	/* F-disk label missing			*/

/* Slice state flags */

#define	SLF_PRESERVE	0x01	/* preserve the slice data */
#define	SLF_LOCK	0x02	/* lock the attributes of the slice */
#define	SLF_STUCK	0x04	/* starting cylinder explicitly set */
#define	SLF_IGNORE	0x08	/* ignore this slice during install and treat */
				/* it as "preserved" during configuration */
#define	SLF_ALIGNED	0x10	/* slice start (and size) was adjusted to be */
				/* cylinder aligned at load - can't preserve */
#define	SLF_EXPLICIT	0x20	/* slice size set explicitly */

/* Partition state flags */

#define	PF_PRESERVE	0x01	/* preserve the partition configuration */
#define	PF_STUCK	0x02	/* starting cylinder explicitly set */

/* Local file systems */

#define	ROOT		"/"
#define	USR		"/usr"
#define	USRPLATFORM	"/usr/platform"
#define	OPT		"/opt"
#define	VAR		"/var"
#define	EXPORT		"/export"
#define	USROWN		"/usr/openwin"
#define	EXPORTROOT	"/export/root"
#define	EXPORTEXEC	"/export/exec"
#define	EXPORTSWAP	"/export/swap"
#define	EXPORTHOME	"/export/home"
#define	HOME		"/home"
#define	SWAP		"swap"
#define	OVERLAP		"overlap"
#define	ALTSECTOR	"alts"
#define	CACHE		"/.cache"

/* Default slice numbers	*/

#define	ROOT_SLICE	0
#define	SWAP_SLICE	1
#define	ALL_SLICE	2	/* entire drive */
#define	BOOT_SLICE	8	/* slice used to access Intel boot blocks */
#define	ALT_SLICE    	9	/* standard Intel alternate sectdor slice */
#define	LAST_STDSLICE	7

/* default size status field values */

#define	DFLT_IGNORE	0
#define	DFLT_SELECT	1
#define	DFLT_DONTCARE	2

/* default size roll-up specifiers */

#define	DONTROLLUP	0
#define	ROLLUP		1

/*
 * Rounding must always be 'up' or errors will occur when
 * trying to fit software in slices which appear to be large
 * enough
 */

/* conversion macros rounding up to nearest unit without cylinder rounding */

#define	sectors_to_mb(s)	(((s) + 2047) / 2048)
#define	sectors_to_kb(s)	(((s) + 1) / 2)
#define	bytes_to_sectors(b) 	(((b) + 511) / 512)
#define	kb_to_mb(k)		(((k) + 1023) / 1024)

/* conversion macros truncating to nearest unit without cylinder rounding */

#define	sectors_to_mb_trunc(s)    ((s) / 2048)
#define	sectors_to_kb_trunc(s)    ((s) / 2)
#define	bytes_to_sectors_trunc(b) ((b) / 512)
#define	kb_to_mb_trunc(k)	  ((k) / 1024)

/*
 * conversion macros without cylinder rounding which are not impacted by
 * unit mismatching
 */
#define	sectors_to_bytes(s)  	((s) * 512)
#define	kb_to_sectors(k)	((k) * 2)
#define	mb_to_kb(m)		((m) * 1024)
#define	mb_to_sectors(m)	((m) * 2048)

/*
 * conversion macros rounding up to the nearest cylinder and not impacted
 * by unit mismatching
 */
#define	one_cyl(d)		(disk_geom_onecyl((d)))
#define	blocks_to_cyls(d, b) 	(((b) + (one_cyl((d)) - 1)) / one_cyl((d)))
#define	blocks_to_blocks(d, b) 	(blocks_to_cyls((d), (b)) * one_cyl((d)))
#define	kb_to_blocks(d, k)    	(blocks_to_cyls((d), (k) * 2) * one_cyl((d)))
#define	mb_to_blocks(d, m) 	(blocks_to_cyls((d), (m) * 2048) * one_cyl((d)))
#define	cyls_to_blocks(d, c)   	((c) * one_cyl((d)))
#define	blocks_to_bytes(d, b) 	(blocks_to_blocks((d), (b)) * 512)

/* conversion macros rounding up to the nearest cylinder and unit */

#define	blocks_to_kb(d, b) 	((blocks_to_blocks((d), (b)) + 1) / 2)
#define	blocks_to_mb(d, b) 	((blocks_to_blocks((d), (b)) + 2047) / 2048)
#define	bytes_to_blocks(d, b) 	(blocks_to_cyls(d, ((b) + 511) / 512) * one_cyl((d)))

/* conversion macros rounding up to the nearest cylinder and truncating the unit */

#define	blocks_to_kb_trunc(d, b) 	(blocks_to_blocks((d), (b)) / 2)
#define	blocks_to_mb_trunc(d, b) 	(blocks_to_blocks((d), (b)) / 2048)
#define	bytes_to_blocks_trunc(d, b) 	(blocks_to_cyls(d, ((b)) / 512) * one_cyl((d)))

/*
 * The following macros should be used in install applications for size testing
 * purposes
 */
#define	usable_sdisk_cyls(d)	((d)->sdisk.geom == NULL ? 0 : \
					(sdisk_geom_dcyl((d)) - \
					(numparts < ALT_SLICE ? 0 : \
					blocks_to_cyls((d), slice_size((d), \
					ALT_SLICE)))))
#define	usable_sdisk_blks(d)	((d)->sdisk.geom == NULL ? 0 : \
					(sdisk_geom_dsect((d)) - \
					(numparts < ALT_SLICE ? 0 : \
					slice_size((d), ALT_SLICE))))

#define	usable_disk_cyls(d)	(disk_geom_dcyl((d)))
#define	usable_disk_blks(d)	(disk_geom_dsect((d)))

#define	total_sdisk_cyls(d)	((d)->sdisk.geom == NULL ? 0 : \
					sdisk_geom_tcyl((d)))
#define	total_sdisk_blks(d)	((d)->sdisk.geom == NULL ? 0 : \
					sdisk_geom_tsect((d)))
/*
 * Total number of cyls/sects (equivalent to the dkg_pcyl value)
 */
#define	total_disk_cyls(d)	(disk_geom_tcyl((d)))
#define	total_disk_blks(d)	(disk_geom_tsect((d)))

/*
 * Total number of accessible cyls/sects (equivalent to the dkg_ncyl value)
 */
#define	accessible_sdisk_cyls(d) ((d)->sdisk.geom == NULL ? 0 : \
					sdisk_geom_lcyl((d)))
#define	accessible_sdisk_blks(d) ((d)->sdisk.geom == NULL ? 0 : \
					sdisk_geom_lsect((d)))
/*
 * Miscellaneous macros
 */
#define	sdisk_is_usable(d)	(((d) != NULLDISK) && \
					disk_selected((d)) && \
					sdisk_geom_not_null((d)))
#define	sdisk_not_usable(d)	(sdisk_is_usable((d)) == 0)

#define	is_pathname(x)		((x) != NULL && *(x) == '/')

/* ******************************************************************** */
/*									*/
/*			DISK STRUCTURE ACCESS MACROS			*/
/*									*/
/*   WARNING*WARNING*WARNING*WARNING*WARNING*WARNING*WARNING*WARNING    */
/*									*/
/*	  The following macros are to be used as RH values ONLY		*/
/*									*/
/* ******************************************************************** */

/* 	macros for accessing parts of the physical disk structure	*/

#define	disk_name(d)			((d)->name)
#define	disk_okay(d)			(!((d)->state & DF_NOTOKAY))
#define	disk_not_okay(d)		((d)->state & DF_NOTOKAY)
#define	disk_bootdrive(d)		((d)->state & DF_BOOTDRIVE)
#define	disk_not_bootdrive(d)		(!((d)->state & DF_BOOTDRIVE))
#define	disk_selected(d)		((d)->state & DF_SELECTED)
#define	disk_not_selected(d)		(!((d)->state & DF_SELECTED))
#define	disk_unusable(d)		((d)->state & DF_UNUSABLE)
#define	disk_format_disk(d)		((d)->state & DF_FORMAT)
#define	disk_not_format_disk(d)		(!((d)->state & DF_FORMAT))
#define	disk_no_pgeom(d)		((d)->state & DF_NOPGEOM)
#define	disk_bad_controller(d)		((d)->state & DF_BADCTRL)
#define	disk_unk_controller(d)		((d)->state & DF_UNKNOWN)
#define	disk_cant_format(d)		((d)->state & DF_CANTFORMAT)
#define	disk_fdisk_req(d)		((d)->state & DF_FDISKREQ)
#define	disk_no_fdisk_req(d)		(!((d)->state & DF_FDISKREQ))
#define	disk_fdisk_exists(d)		((d)->state & DF_FDISKEXISTS)
#define	disk_no_fdisk_exists(d)		(!((d)->state & DF_FDISKEXISTS))
#define	disk_initialized(d)		((d)->state & DF_INIT)
#define	disk_ctype(d)			((d)->ctype)
#define	disk_cname(d)			((d)->cname)

#define	disk_geom_dcyl(d)		((d)->geom.dcyl)
#define	disk_geom_tcyl(d)		((d)->geom.tcyl)
#define	disk_geom_firstcyl(d)		((d)->geom.firstcyl)
#define	disk_geom_lcyl(d)		((d)->geom.lcyl)
#define	disk_geom_onecyl(d)		((d)->geom.onecyl)
#define	disk_geom_hbacyl(d)		((d)->geom.hbacyl)
#define	disk_geom_nsect(d)		((d)->geom.nsect)
#define	disk_geom_nhead(d)		((d)->geom.nhead)
#define	disk_geom_dsect(d)		((d)->geom.dsect)
#define	disk_geom_tsect(d)		((d)->geom.tsect)
#define	disk_geom_rsect(d)		((d)->geom.rsect)
#define	disk_geom_lsect(d)		((d)->geom.lsect)

/*	macros for accessing components of the current S-disk struct		*/

#define	sdisk_touch2(d)   		((d)->sdisk.state & SF_S2MOD)
#define	sdisk_not_touch2(d)   		(!((d)->sdisk.state & SF_S2MOD))
#define	sdisk_legal(d)			(!((d)->sdisk.state & SF_ILLEGAL))
#define	sdisk_not_legal(d)		((d)->sdisk.state & SF_ILLEGAL)
#define	sdisk_no_slabel(d)		((d)->sdisk.state & SF_NOSLABEL)

#define	sdisk_geom_null(d)		((d)->sdisk.geom == (Geom_t *)0)
#define	sdisk_geom_not_null(d)		((d)->sdisk.geom != (Geom_t *)0)
#define	sdisk_geom_dcyl(d)		((d)->sdisk.geom->dcyl)
#define	sdisk_geom_tcyl(d)		((d)->sdisk.geom->tcyl)
#define	sdisk_geom_firstcyl(d)		((d)->sdisk.geom->firstcyl)
#define	sdisk_geom_lcyl(d)		((d)->sdisk.geom->lcyl)
#define	sdisk_geom_onecyl(d)		((d)->sdisk.geom->onecyl)
#define	sdisk_geom_hbacyl(d)		((d)->sdisk.geom->hbacyl)
#define	sdisk_geom_rsect(d)		((d)->sdisk.geom->rsect)
#define	sdisk_geom_lsect(d)		((d)->sdisk.geom->lsect)
#define	sdisk_geom_nsect(d)		((d)->sdisk.geom->nsect)
#define	sdisk_geom_tsect(d)		((d)->sdisk.geom->tsect)
#define	sdisk_geom_dsect(d)		((d)->sdisk.geom->dsect)

/*	macros for access F-disk structure components	*/

#define	fdisk_no_flabel(d)		((d)->fdisk.state & FF_NOFLABEL)

/* 	macros for accessing current S-disk slice components */

#define	slice_preserved(d, s)	  ((d)->sdisk.slice[(s)].state & SLF_PRESERVE)
#define	slice_not_preserved(d, s) (!((d)->sdisk.slice[(s)].state & SLF_PRESERVE))
#define	slice_locked(d, s)	  ((d)->sdisk.slice[(s)].state & SLF_LOCK)
#define	slice_not_locked(d, s)	  (!((d)->sdisk.slice[(s)].state & SLF_LOCK))
#define	slice_stuck(d, s)	  ((d)->sdisk.slice[(s)].state & SLF_STUCK)
#define	slice_not_stuck(d, s)	  (!((d)->sdisk.slice[(s)].state & SLF_STUCK))
#define	slice_stuck_on(d, s)	  ((d)->sdisk.slice[(s)].state |= SLF_STUCK)
#define	slice_stuck_off(d, s)	  ((d)->sdisk.slice[(s)].state &= ~SLF_STUCK)
#define	slice_ignored(d, s)	  ((d)->sdisk.slice[(s)].state & SLF_IGNORE)
#define	slice_not_ignored(d, s)	  (!((d)->sdisk.slice[(s)].state & SLF_IGNORE))
#define	slice_aligned(d, s)	  ((d)->sdisk.slice[(s)].state & SLF_ALIGNED)
#define	slice_not_aligned(d, s)	  (!((d)->sdisk.slice[(s)].state & SLF_ALIGNED))
#define	slice_explicit_on(d, s)	  ((d)->sdisk.slice[(s)].state |= SLF_EXPLICIT)
#define	slice_explicit_off(d, s)  ((d)->sdisk.slice[(s)].state &= ~SLF_EXPLICIT)
#define	slice_explicit(d, s)	  ((d)->sdisk.slice[(s)].state & SLF_EXPLICIT)

#define	slice_start(d, s)	  ((d)->sdisk.slice[(s)].start)
#define	slice_size(d, s)	  ((d)->sdisk.slice[(s)].size)
#define	slice_mntpnt(d, s)	  ((d)->sdisk.slice[(s)].mntpnt)
#define	slice_mntpnt_exists(d, s) ((d)->sdisk.slice[(s)].mntpnt[0] != '\0')
#define	slice_mntpnt_not_exists(d, s)	(!((d)->sdisk.slice[(s)].mntpnt[0] != '\0'))
#define	slice_mntpnt_is_fs(d, s)  ((d)->sdisk.slice[(s)].mntpnt[0] == '/')
#define	slice_mntpnt_isnt_fs(d, s)	((d)->sdisk.slice[(s)].mntpnt[0] != '/')
#define	slice_mntopts(d, s)	  ((d)->sdisk.slice[(s)].mntopts)

/* 	macros for accessing components of the original S-disk slice structs */

#define	orig_slice_preserved(d, s)	((d)->o_sdisk.slice[(s)].state & SLF_PRESERVE)
#define	orig_slice_not_preserved(d, s)	(!((d)->o_sdisk.slice[(s)].state & SLF_PRESERVE))
#define	orig_slice_locked(d, s)		((d)->o_sdisk.slice[(s)].state & SLF_LOCK)
#define	orig_slice_not_locked(d, s)	(!((d)->o_sdisk.slice[(s)].state & SLF_LOCK))
#define	orig_slice_stuck(d, s)		((d)->o_sdisk.slice[(s)].state & SLF_STUCK)
#define	orig_slice_not_stuck(d, s)	(!((d)->o_sdisk.slice[(s)].state & SLF_STUCK))
#define	orig_slice_start(d, s)		((d)->o_sdisk.slice[(s)].start)
#define	orig_slice_size(d, s)		((d)->o_sdisk.slice[(s)].size)
#define	orig_slice_mntpnt(d, s)		((d)->o_sdisk.slice[(s)].mntpnt)
#define	orig_slice_mntpnt_exists(d, s)	((d)->o_sdisk.slice[(s)].mntpnt[0] != '\0')
#define	orig_slice_mntopts(d, s)	((d)->o_sdisk.slice[(s)].mntopts)
#define	orig_slice_aligned(d, s)	((d)->o_sdisk.slice[(s)].state & SLF_ALIGNED)
#define	orig_slice_not_aligned(d, s)	(!((d)->o_sdisk.slice[(s)].state & SLF_ALIGNED))

/* macros for accessing components of the committed S-disk slice structs */

#define	comm_slice_preserved(d, s)	((d)->c_sdisk.slice[(s)].state & SLF_PRESERVE)
#define	comm_slice_not_preserved(d, s)	(!((d)->c_sdisk.slice[(s)].state & SLF_PRESERVE))
#define	comm_slice_locked(d, s)		((d)->c_sdisk.slice[(s)].state & SLF_LOCK)
#define	comm_slice_not_locked(d, s)	(!((d)->c_sdisk.slice[(s)].state & SLF_LOCK))
#define	comm_slice_stuck(d, s)		((d)->c_sdisk.slice[(s)].state & SLF_STUCK)
#define	comm_slice_not_stuck(d, s)	(!((d)->c_sdisk.slice[(s)].state & SLF_STUCK))
#define	comm_slice_start(d, s)		((d)->c_sdisk.slice[(s)].start)
#define	comm_slice_size(d, s)		((d)->c_sdisk.slice[(s)].size)
#define	comm_slice_mntpnt(d, s)		((d)->c_sdisk.slice[(s)].mntpnt)
#define	comm_slice_mntpnt_exists(d, s)	((d)->c_sdisk.slice[(s)].mntpnt[0] != '\0')
#define	comm_slice_mntopts(d, s)	((d)->c_sdisk.slice[(s)].mntopts)
#define	comm_slice_aligned(d, s)	((d)->c_sdisk.slice[(s)].state & SLF_ALIGNED)
#define	comm_slice_not_aligned(d, s)	(!((d)->c_sdisk.slice[(s)].state & SLF_ALIGNED))

/*	macros for accessing current F-Disk Partition data */

#define	part_id(d, p)		 ((d)->fdisk.part[(p) - 1].id)
#define	part_active(d, p)	 ((d)->fdisk.part[(p) - 1].active)
#define	part_is_active(d, p)	 ((d)->fdisk.part[(p) - 1].active == ACTIVE)
#define	part_not_active(d, p)	 ((d)->fdisk.part[(p) - 1].active == NOTACTIVE)
#define	part_preserved(d, p)	 ((d)->fdisk.part[(p) - 1].state & PF_PRESERVE)
#define	part_orig_partnum(d, p)	 ((d)->fdisk.part[(p) - 1].origpart)

#define	part_startsect(d, p)	 ((d)->fdisk.part[(p) - 1].geom.rsect)
#define	part_startcyl(d, p)	 ((d)->fdisk.part[(p) - 1].geom.rsect / one_cyl((d)))
#define	part_size(d, p)		 ((d)->fdisk.part[(p) - 1].geom.tsect)
#define	part_geom_dcyl(d, p)	 ((d)->fdisk.part[(p) - 1].geom.dcyl)
#define	part_geom_tcyl(d, p)	 ((d)->fdisk.part[(p) - 1].geom.tcyl)
#define	part_geom_firstcyl(d, p) ((d)->fdisk.part[(p) - 1].geom.firstcyl)
#define	part_geom_lcyl(d, p)	 ((d)->fdisk.part[(p) - 1].geom.lcyl)
#define	part_geom_onecyl(d, p)	 ((d)->fdisk.part[(p) - 1].geom.onecyl)
#define	part_geom_hbacyl(d, p)	 ((d)->fdisk.part[(p) - 1].geom.hbacyl)
#define	part_geom_rsect(d, p)	 ((d)->fdisk.part[(p) - 1].geom.rsect)
#define	part_geom_lsect(d, p)	 ((d)->fdisk.part[(p) - 1].geom.lsect)
#define	part_geom_nsect(d, p)	 ((d)->fdisk.part[(p) - 1].geom.nsect)
#define	part_geom_nhead(d, p)	 ((d)->fdisk.part[(p) - 1].geom.nhead)
#define	part_geom_tsect(d, p)	 ((d)->fdisk.part[(p) - 1].geom.tsect)
#define	part_geom_dsect(d, p)	 ((d)->fdisk.part[(p) - 1].geom.dsect)

#define	part_stuck(d, p)	 ((d)->fdisk.part[(p) - 1].state & PF_STUCK)
#define	part_not_stuck(d, p)	 (!((d)->fdisk.part[(p) - 1].state & PF_STUCK))
#define	part_stuck_on(d, p)	 ((d)->fdisk.part[(p) - 1].state |= PF_STUCK)
#define	part_stuck_off(d, p)	 ((d)->fdisk.part[(p) - 1].state &= ~PF_STUCK)

/*	macros for accessing components of the committed F-Disk Partition data */

#define	comm_part_id(d, p)		((d)->c_fdisk.part[(p) - 1].id)
#define	comm_part_active(d, p)	 	((d)->c_fdisk.part[(p) - 1].active)
#define	comm_part_is_active(d, p)	((d)->c_fdisk.part[(p) - 1].active == ACTIVE)
#define	comm_part_not_active(d, p)	((d)->c_fdisk.part[(p) - 1].active == NOTACTIVE)
#define	comm_part_preserved(d, p)	((d)->c_fdisk.part[(p) - 1].state & PF_PRESERVE)

/*	macros for accessing components of the original F-Disk Partition data */

#define	orig_part_id(d, p)		((d)->o_fdisk.part[(p) - 1].id)
#define	orig_part_active(d, p)	 	((d)->o_fdisk.part[(p) - 1].active)
#define	orig_part_is_active(d, p)	((d)->o_fdisk.part[(p) - 1].active == ACTIVE)
#define	orig_part_not_active(d, p)	((d)->o_fdisk.part[(p) - 1].active == NOTACTIVE)
#define	orig_part_preserved(d, p)	((d)->o_fdisk.part[(p) - 1].state & PF_PRESERVE)

/* Debug print macros */

#ifdef	DEBUG
#define	DPRINT0(X)			(void) fprintf(stderr, X)
#define	DPRINT1(X, Y)			(void) fprintf(stderr, X, Y)
#define	DPRINT2(X, Y, Z)		(void) fprintf(stderr, X, Y, Z)
#define	DPRINT3(X, Y, Z, A)		(void) fprintf(stderr, X, Y, Z, A)
#define	DPRINT4(X, Y, Z, A, B)		(void) fprintf(stderr, X, Y, Z, A, B)
#define	DPRINT5(X, Y, Z, A, B, C)	(void) fprintf(stderr, X, Y, Z, A, B, C)
#else
#define	DPRINT0(X)
#define	DPRINT1(X, Y)
#define	DPRINT2(X, Y, Z)
#define	DPRINT3(X, Y, Z, A)
#define	DPRINT4(X, Y, Z, A, B)
#define	DPRINT5(X, Y, Z, A, B, C)
#endif /* DEBUG */

/*	miscellaneous macros	*/

#define	valid_fdisk_part(p)		((p) >= 1 && (p) <= FD_NUMPART)
#define	invalid_fdisk_part(p)		((p) < 1 || (p) > FD_NUMPART)
#define	valid_sdisk_slice(s)		((s) >= 0 && (s) < numparts)
#define	invalid_sdisk_slice(s)		((s) < 0 || (s) >= numparts)
#define	WALK_DISK_LIST(x)		for ((x) = first_disk(); \
						(x); (x) = next_disk((x)))
#define	WALK_LIST(x, y)			for ((x) = (y); (x) != NULL; (x) = (x)->next)
#define	WALK_LIST_INDIRECT(x, y)	for ((x) = &(y); *(x) != NULL; \
						(x) = &((*x)->next))
#define	WALK_PARTITIONS(x)		for ((x) = 1; (x) <= FD_NUMPART; (x)++)
#define	WALK_PARTITIONS_REAL(x)		for ((x) = 0; (x) < FD_NUMPART; (x)++)
#define	WALK_SLICES(x)			for ((x) = 0; (x) < numparts; (x)++)
#define	WALK_SLICES_STD(x)		for ((x) = 0; (x) <= LAST_STDSLICE; (x)++)

/* **************************************************************************** */
/*			EXTERNAL REFERENCES FOR APPLICATIONS 			*/
/* **************************************************************************** */

#ifdef __cplusplus
extern "C" {
#endif

/* disk.c */

extern int	commit_disk_config(Disk_t *);
extern int	commit_disks_config(void);
extern int	deselect_disk(Disk_t *, char *);
extern int	select_disk(Disk_t *, char *);
extern int	select_bootdisk(Disk_t *, char *);
extern int	restore_disk(Disk_t *, Label_t);
extern char *	spec_dflt_bootdisk(void);
extern int	duplicate_disk(Disk_t *, Disk_t *);

/* disk_profile.c */

extern int	configure_dfltmnts(Profile *);
extern int	configure_sdisk(Profile *);

/* disk_sdisk.c */

extern int	sdisk_geom_same(Disk_t *, Label_t);
extern int	sdisk_config(Disk_t *, char *, Label_t);
extern int	sdisk_compare(Disk_t *, Label_t);
extern int	set_slice_geom(Disk_t *, int, int, int);
extern int	set_slice_mnt(Disk_t *, int, char *, char *);
extern int	set_slice_preserve(Disk_t *, int, int);
extern int	set_slice_ignore(Disk_t *, int, int);
extern int 	adjust_slice_starts(Disk_t *);
extern int	sdisk_space_avail(Disk_t *);
extern int	sdisk_max_hole_size(Disk_t *);
extern void	sdisk_use_free_space(Disk_t *);
extern int	find_mnt_pnt(Disk_t *, char *, char *, Mntpnt_t *, Label_t);
extern int	slice_preserve_ok(Disk_t *, int);
extern int	filesys_preserve_ok(char *);
extern int	slice_name_ok(char *);
extern int	slice_overlaps(Disk_t *, int, int, int, int **);
extern int	get_slice_autoadjust(void);
extern int	set_slice_autoadjust(int);
extern int	get_mntpnt_size(Disk_t *, char *, char *, Label_t);
extern int	swap_size_allocated(Disk_t *, char *);

/* disk_util.c */

extern u_int	blocks2size(Disk_t *, u_int, int);
extern u_int	size2blocks(Disk_t *, u_int);
extern Units_t	set_units(Units_t);
extern Units_t	get_units(void);
extern Disk_t *	next_disk(Disk_t *);
extern Disk_t *	first_disk(void);
extern Disk_t *	find_bootdisk(void);
extern Disk_t *	find_disk(char *);
extern int 	simplify_disk_name(char *, char *);
extern int	umount_slash_a(void);
extern char *	make_slice_name(char *, int);
extern char *	make_block_device(char *, int);
extern char *	make_char_device(char *, int);
extern int	axtoi(char *);
extern int	is_disk_name(char *);
extern int	is_ipaddr(char *);
extern int	is_hostname(char *);
extern int	is_numeric(char *);
extern int	is_allnums(char *);
extern int	is_hex_numeric(char *);
extern int	is_slice_name(char *);
extern int	map_in_file(const char *, char **);
extern int	slice_access(char *, int);
extern char *	library_error_msg(int);
extern int	get_trace_level(void);
extern int	set_trace_level(int);

/* disk_debug.c */

extern void	print_disk(Disk_t *, char *);
extern void	print_disk_state(Disk_t *, char *);
extern void	print_orig_sdisk(Disk_t *, char *);
extern void	print_orig_fdisk(Disk_t *, char *);
extern void	print_commit_sdisk(Disk_t *, char *);
extern void	print_commit_fdisk(Disk_t *, char *);
extern void	print_vtoc(struct vtoc *);
extern void	print_geom(Geom_t *);
extern void	print_slices(Sdisk_t *);
extern void	print_parts(Fdisk_t *);
extern void	print_sdisk_state(u_char);
extern void	print_fdisk_state(u_char);
extern void	print_dfltmnt_list(char *, Defmnt_t **);

/* disk_filesys.c */

extern Space	**filesys_ok(void);

/* disk_dflt.c */

extern int		get_default_fs_size(char *, Disk_t *, int);
extern int		get_minimum_fs_size(char *, Disk_t *, int);
extern int		sdisk_default_all(void);

/* disk_chk.c */

extern int		validate_disks(void);
extern int		validate_disk(Disk_t *);
extern int		validate_sdisk(Disk_t *);
extern int		validate_fdisk(Disk_t *);
extern void		free_error_list(void);
extern int		check_disks(void);
extern int		check_disk(Disk_t *);
extern int		check_sdisk(Disk_t *);
extern int		check_fdisk(Disk_t *);
extern Errmsg_t		*get_error_list(void);

/* disk_dfltmnt.c */

extern Defmnt_t **	get_dfltmnt_list(Defmnt_t **);
extern int		set_dfltmnt_list(Defmnt_t **);
extern Defmnt_t **	free_dfltmnt_list(Defmnt_t **);
extern int 		get_dfltmnt_ent(Defmnt_t *, char *);
extern int		set_dfltmnt_ent(Defmnt_t *, char *);
extern void 		update_dfltmnt_list(void);
extern int		set_client_space(int, int, int);

/* disk_fdisk.c */

extern int		set_part_geom(Disk_t *, int, int, int);
extern int		set_part_attr(Disk_t *, int, int, int);
extern int		part_geom_same(Disk_t *, int, Label_t);
extern int		adjust_part_starts(Disk_t *);
extern int		fdisk_config(Disk_t *, char *, Label_t);
extern int		part_overlaps(Disk_t *, int, int, int, int **);
extern int		fdisk_space_avail(Disk_t *);
extern int		set_part_preserve(Disk_t *, int, int);
extern int		get_solaris_part(Disk_t *, Label_t);
extern int		max_size_part_hole(Disk_t *, int);

/* disk_upg.c */

extern Disk_t *		upgradeable_disks(void);
extern int		do_mounts_and_swap(Disk_t *);

/* disk_find.c */

extern int 		build_disk_list(void);

/* disk_load.c */

extern int 		load_disk_list(char *);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _DISK_API_H */
