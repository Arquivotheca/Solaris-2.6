#ifndef lint
#pragma ident "@(#)spmistore_api.h 1.14 96/06/29 SMI"
#endif

/*
 * Copyright (c) 1995-1996 by Sun Microsystems, Inc. All rights reserved.
 */
/*
 * Module:	spmistore_api.h
 * Group:	libspmistore
 * Description:
 */

#ifndef _SPMISTORE_API_H
#define	_SPMISTORE_API_H

#include <stdarg.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/vtoc.h>

/* constants */

/*
 * should be in common <sys/dktp/fdisk.h>
 */
#if defined(__i386) || defined(__ppc)
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
#define	OTHEROS		98	/* type for appl. (DB?) needs raw partition */
				/* ID was 0 but conflicted w/ DOS 3.3 fdisk */
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

/* VTOC tag-defined slice mount point names */
#define	ALTSECTOR	"alts"
#define	CACHE		"/.cache"
#define	OVERLAP		"overlap"
#define	ROOT		"/"
#define	SWAP		"swap"

/* geometry size specifiers */
#define	GEOM_ORIG	-1	/* existing (size and start) */
#define	GEOM_REST	-2	/* free (size only) */
#define	GEOM_COMMIT	-3	/* committed value (size and start) */
#define	GEOM_IGNORE	-4	/* ignore this parameter */

/* general values for common size constraint specifiers */
#define	VAL_UNSPECIFIED	-1	/* no specified size */
#define	VAL_FREE	-2	/* largest contiguous available space */
#define	VAL_ALL		-3	/* all accessible space */

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
#define	D_BOOTFIXED	-15  /* explicit boot object values conflict */
#define	D_NOTSELECT	-16  /* disk not selected */
#define	D_LOCKED	-17  /* slice state locked; can't change */
#define	D_GEOMCHNG	-18  /* disk geometry changed */
#define	D_NOGEOM	-19  /* no disk geometry defined */
#define	D_NOFIT		-20  /* slice/partition doesn't fit in disk segment */
#define	D_NOSOLARIS	-21  /* no Solaris partition configured on the disk */
#define	D_BADORDER	-22  /* slices/partitions not in a legal ordering */
#define	D_OUTOFREACH	-23  /* boot critical component accessed by BIOS) */
			    /* cannot be reached by the BIOS code */
#define	D_SMALLSWAP	-25  /* less swap configured than required */
#define	D_ALIGNED	-26  /* the starting cyl/size were aligned at load */
#define	D_FAILED	-27  /* generic failure */

/* check disk warning return codes */
#define	D_UNUSED	  1 /* there is unused space on the slice */
#define	D_BOOTCONFIG	  2 /* "/" is not on the default boot disk */
#define	D_PROMRECONFIG	  3 /* the prom will be reconfigured */
#define	D_PROMMISCONFIG	  4 /* the prom will need to be reconfigured */

/* system slice index specifiers */
#define	ALL_SLICE	2	/* all user accessible space */
#define	BOOT_SLICE	8	/* fdisk boot block slice */
#define	ALT_SLICE    	9	/* fdisk alternate sector slice */
#define	LAST_STDSLICE	7	/* last user accessible slice */

/*	miscellaneous macros	*/

#define	valid_fdisk_part(p)		((p) >= 1 && (p) <= FD_NUMPART)
#define	invalid_fdisk_part(p)		((p) < 1 || (p) > FD_NUMPART)
#define	valid_sdisk_slice(s)		((s) >= 0 && (s) < numparts)
#define	invalid_sdisk_slice(s)		((s) < 0 || (s) >= numparts)

/* list walking macros used to simplify loop logic */
#define	WALK_DISK_LIST(x)	for ((x) = first_disk(); \
					(x); (x) = next_disk((x)))
#define	WALK_PARTITIONS(x)	for ((x) = 1; (x) <= FD_NUMPART; (x)++)
#define	WALK_PARTITIONS_REAL(x)	for ((x) = 0; (x) < FD_NUMPART; (x)++)
#define	WALK_SLICES(x)		for ((x) = 0; (x) < numparts; (x)++)
#define	WALK_SLICES_STD(x)	for ((x) = 0; (x) <= LAST_STDSLICE; (x)++)

/* disk state field values */
#define	DF_BADCTRL	0x0001	/* controller access failed		*/
#define	DF_UNKNOWN	0x0002	/* unknown controller type		*/
#define	DF_NOPGEOM	0x0004	/* unknown physical geometry information */
#define	DF_CANTFORMAT	0x0008	/* drive cannot be fmttd - hardware failure */
#define	DF_FDISKREQ	0x0080	/* fdisk exposure to the user required */
#define	DF_FORMAT	0x0200	/* format S-disk specified for drive */
#define	DF_SELECTED	0x0400	/* drive selected for processing */
#define	DF_FDISKEXISTS	0x1000	/* fdisk physically supported for this disk */
#define	DF_INIT		0x2000	/* disk structure initialization completed */
#define	DF_UNUSABLE	(DF_BADCTRL|DF_UNKNOWN|DF_CANTFORMAT)
#define	DF_NOTOKAY	(DF_BADCTRL|DF_UNKNOWN|DF_NOPGEOM)
#define	DF_INITIAL 	(DF_NOTOKAY)

/* sdisk state field values */
#define	SF_ILLEGAL	0x01	/* illegal sdisk configuration */
#define	SF_NOSLABEL	0x02	/* Solaris label missing */

/* fdisk state field values */
#define	FF_NOFLABEL	0x01	/* fdisk label missing */

/* slice state flags values */
#define	SLF_PRESERVED	0x01	/* preserve the slice data */
#define	SLF_LOCKED	0x02	/* lock the attributes of the slice */
#define	SLF_STUCK	0x04	/* starting cylinder explicitly set */
#define	SLF_IGNORED	0x08	/* ignore this slice during install and treat */
				/* it as "preserved" during configuration */
#define	SLF_REALIGNED	0x10	/* slice start (and size) was adjusted to be */
				/* cylinder aligned at load - can't preserve */
#define	SLF_EXPLICIT	0x20	/* slice size set explicitly */

/* fdisk partition state flag values */
#define	PF_PRESERVED	0x01	/* preserve the partition configuration */
#define	PF_STUCK	0x02	/* starting cylinder explicitly set */

/*
 * Data Structures
 */

/*
 * specify unit of size
 */
typedef enum {
	D_MBYTE = 0,
	D_KBYTE = 1,
	D_BLOCK = 2,
	D_CYLS  = 3
} Units_t;

/*
 * specify disk configuration state
 */
typedef enum {
	CFG_CURRENT = 0,	/* THIS MUST BE '0' INDEX */
	CFG_COMMIT  = 1,	/* THIS MUST BE '1' INDEX */
	CFG_EXIST   = 2		/* THIS MUST BE '2' INDEX */
} Label_t;

/*
 * specify the layout configuration requested
 */
typedef enum {
	LAYOUT_UNDEFINED = 0,
	LAYOUT_RESET = 1,
	LAYOUT_DEFAULT = 2,
	LAYOUT_COMMIT = 3,
	LAYOUT_EXIST = 4
} Layout_t;

/*
 * Slice attribute used to identify specific attributes of a
 * given slice in get/set calls
 */
typedef enum {
	SLICEOBJ_UNDEFINED = 0,		/* RESTRICTED */
	SLICEOBJ_USE,			/* slice use (name) */
	SLICEOBJ_INSTANCE,		/* instance of slice use; 0 - default,
					 * -1 - unspecified */
	SLICEOBJ_START,			/* slice starting cylinder */
	SLICEOBJ_SIZE,			/* slice size in sectors */
	SLICEOBJ_MOUNTOPTS,		/* mount options for file systems */
	SLICEOBJ_EXPLICIT,		/* flag indicating explicit size */
	SLICEOBJ_STUCK,			/* flag indicating fixed start cyl */
	SLICEOBJ_IGNORED,		/* flag indicating slice ignored */
	SLICEOBJ_LOCKED,		/* RESTRICTED: read-only - flag
					 * indicating restricted use */
	SLICEOBJ_PRESERVED,		/* flag indicating preserved */
	SLICEOBJ_REALIGNED		/* RESTRICTED: read-only - flag
					 * indicating starting cylinder altered
					 * to be cylinder aligned */
} SliceobjAttr_t;

/*
 * Boot object enumerated types representing each piece of boot object
 * data - used for setting and getting values
 */
typedef enum {
	BOOTOBJ_UNDEFINED	= 0,	/* RESTRICTED: placeholder */
	BOOTOBJ_DISK		= 1,	/* disk containing boot device */
	BOOTOBJ_DISK_EXPLICIT	= 2,	/* flag indicating disk is required */
	BOOTOBJ_DEVICE		= 3,	/* slice/partition containing boot
					 * device */
	BOOTOBJ_DEVICE_EXPLICIT = 4,	/* flag indicating device is reauired */
	BOOTOBJ_DEVICE_TYPE	= 5,	/* slice/partition device specifier */
	BOOTOBJ_PROM_UPDATE	= 6,	/* flag indicating if firmware should
					 * be automatically updated */
	BOOTOBJ_PROM_UPDATEABLE	= 7	/* RESTRICTED: read-only - flag
					 * indicating if firmware is capable
					 * of being automatically updated */
} BootobjAttrType;

/*
 * error message linked list message storage format
 */
typedef struct errmsg {
	int		code;
	char		*msg;
	struct errmsg	*next;
} Errmsg_t;

/*
 * Geometry structure containing information about either the physical or
 * virtual drive. One is associated with the Disk_t structure, and one for
 * each F-disk partition (if F-disk labelling is being used). Note that the
 * 'dcyl' and 'tcyl' values are used for slices and new partitions, whereas
 * 'dsect' and 'tsect' are only used for existing (DOS generated) F-disk
 * partitions
 */
typedef struct {
	int	firstcyl;	/* last legal data cylinder on the disk */
	int	lcyl;		/* # of accessible cylinders on disk */
	int	dcyl;		/* # of data cylinders on disk */
	int	tcyl;		/* # of cylinders on disk */
	int	onecyl;		/* # of sectors per cylinder (physical) */
	int	hbacyl;		/* # of sectors per cylinder (HBA) */
	int	rsect;		/* relative offset sector for the disk */
	int	nsect;		/* # of sectors per track (physical) */
	int	lsect;		/* # of accessible sectors on the disk */
	int	dsect;		/* # of data sectors on the disk */
	int	tsect;		/* # of sectors on the disk */
	int	nhead;		/* # of tracks per cylinder (physical) */
} Geom_t;

/*
 * Slice structure (current model assumes no meta-disk device support
 * and file systems are 1:1 matched to slices)
 */
typedef struct {
	char	use[MAXNAMELEN];	/* slice utilization identifier */
	int	instance;		/* use instance */
	u_char	state;			/* slice state to hold flags */
	int	start;			/* starting cylinder # */
	int	size;			/* # of sectors in the slice */
	char	mntopts[MAXNAMELEN];	/* file system mount options */
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
	Geom_t	geom;		/* physical disk geometry data */
	Fdisk_t	fdisk[3];	/* fdisk information */
	Sdisk_t	sdisk[3];	/* VTOC information */
	struct disk *next;	/* pointer to next disk in chain */
} Disk_t;

/*
 * used to pass find_mnt_pnt() parameters
 */
typedef struct mnt_pnt {
	Disk_t	*dp;	/* disk pointer */
	int	slice;	/* slice index */
} Mntpnt_t;

typedef Mntpnt_t	SliceKey;

/*
 * disk structure access macros: the following macros are to be used
 * as RH values ONLY
 */

/* physical disk data */
#define	disk_name(d)			((d)->name)
#define	disk_okay(d)			(!((d)->state & DF_NOTOKAY))
#define	disk_not_okay(d)		((d)->state & DF_NOTOKAY)
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

/*
 * configuration explicit slice data access
 */
#define	Sdiskobj_Config(l, d)		((d)->sdisk[(l)])
#define	Sdiskobj_Addr(l, d)		(&((d)->sdisk[(l)]))
#define	Sdiskobj_State(l, d)		(Sdiskobj_Config((l), (d)).state)
#define	Sdiskobj_Geom(l, d)		(Sdiskobj_Config((l), (d)).geom)
#define Sdiskobj_Flag(l, d,  b)		(Sdiskobj_State((l), (d)) & (u_char)(b))
#define SdiskobjIsIllegal(l, d)		(Sdiskobj_Flag((l), (d), SF_ILLEGAL))

#define	Sliceobj_Config(l, d, s)	(Sdiskobj_Config((l), (d)).slice[(s)])
#define	Sliceobj_Addr(l, d, s)		(&(Sdiskobj_Config((l), (d)).slice[(s)]))
#define Sliceobj_State(l, d, s)		(Sliceobj_Config((l), (d), (s)).state)
#define Sliceobj_Use(l, d, s)		(Sliceobj_Config((l), (d), (s)).use)
#define Sliceobj_Instance(l, d, s)	(Sliceobj_Config((l), (d), (s)).instance)
#define Sliceobj_Size(l, d, s)		(Sliceobj_Config((l), (d), (s)).size)
#define Sliceobj_Start(l, d, s)		(Sliceobj_Config((l), (d), (s)).start)
#define Sliceobj_Mountopts(l, d, s)	(Sliceobj_Config((l), (d), (s)).mntopts)
#define Sliceobj_Flag(l, d, s, b)	(Sliceobj_State((l), \
                                                (d), (s)) & (u_char)(b))
#define SliceobjSetBit(l, d, s, b)	(Sliceobj_State((l), \
                                                (d), (s)) |= (u_char)(b))
#define SliceobjClearBit(l, d, s, b)	(Sliceobj_State((l), \
						(d), (s)) &= ~(u_char)(b))
#define SliceobjIsExplicit(l, d, s)	(Sliceobj_Flag((l), (d), (s), SLF_EXPLICIT))
#define SliceobjIsRealigned(l, d, s)	(Sliceobj_Flag((l), (d), (s), SLF_REALIGNED))
#define SliceobjIsStuck(l, d, s)	(Sliceobj_Flag((l), (d), (s), SLF_STUCK))
#define SliceobjIsLocked(l, d, s)	(Sliceobj_Flag((l), (d), (s), SLF_LOCKED))
#define SliceobjIsPreserved(l, d, s)	(Sliceobj_Flag((l), (d), (s), SLF_PRESERVED))
#define SliceobjIsIgnored(l, d, s)	(Sliceobj_Flag((l), (d), (s), SLF_IGNORED))

/*
 * CFG_CURRENT sdisk data
 */
#define	sdisk_state(d)			(Sdiskobj_State(CFG_CURRENT, (d)))
#define	sdisk_geom(d)			(Sdiskobj_Geom(CFG_CURRENT, (d)))
#define	sdisk_geom_dcyl(d)		(sdisk_geom((d))->dcyl)
#define	sdisk_geom_tcyl(d)		(sdisk_geom((d))->tcyl)
#define	sdisk_geom_firstcyl(d)		(sdisk_geom((d))->firstcyl)
#define	sdisk_geom_lcyl(d)		(sdisk_geom((d))->lcyl)
#define	sdisk_geom_onecyl(d)		(sdisk_geom((d))->onecyl)
#define	sdisk_geom_hbacyl(d)		(sdisk_geom((d))->hbacyl)
#define	sdisk_geom_rsect(d)		(sdisk_geom((d))->rsect)
#define	sdisk_geom_lsect(d)		(sdisk_geom((d))->lsect)
#define	sdisk_geom_nsect(d)		(sdisk_geom((d))->nsect)
#define	sdisk_geom_tsect(d)		(sdisk_geom((d))->tsect)
#define	sdisk_geom_dsect(d)		(sdisk_geom((d))->dsect)
#define	sdisk_legal(d)			(!(Sdiskobj_Flag(CFG_CURRENT, (d), SF_ILLEGAL)))
#define	sdisk_no_slabel(d)		(Sdiskobj_Flag(CFG_CURRENT, (d), SF_NOSLABEL))
#define	sdisk_geom_null(d)		(sdisk_geom((d)) == (Geom_t *)0)
#define	sdisk_geom_not_null(d)		(!(sdisk_geom_null(d)))

/*
 * CFG_CURRENT slice data
 */
#define	slice_state(d, s)	    (Sliceobj_State(CFG_CURRENT, (d), (s)))
#define	slice_start(d, s)	    (Sliceobj_Start(CFG_CURRENT, (d), (s)))
#define	slice_size(d, s)	    (Sliceobj_Size(CFG_CURRENT, (d), (s)))
#define	slice_use(d, s)		    (Sliceobj_Use(CFG_CURRENT, (d), (s)))
#define	slice_instance(d, s)	    (Sliceobj_Instance(CFG_CURRENT, (d), (s)))
#define	slice_mntopts(d, s)	    (Sliceobj_Mountopts(CFG_CURRENT, (d), (s)))
#define	slice_preserved(d, s)	    (SliceobjIsPreserved(CFG_CURRENT, (d), (s)))
#define	slice_not_preserved(d, s)   (!(SliceobjIsPreserved(CFG_CURRENT, (d), (s))))
#define	slice_locked(d, s)	    (SliceobjIsLocked(CFG_CURRENT, (d), (s)))
#define	slice_stuck(d, s)	    (SliceobjIsStuck(CFG_CURRENT, (d), (s)))
#define	slice_stuck_on(d, s)	    (SliceobjSetBit(CFG_CURRENT, (d), (s), SLF_STUCK))
#define	slice_stuck_off(d, s)	    (SliceobjClearBit(CFG_CURRENT, (d), (s), \
						SLF_STUCK))
#define	slice_ignored(d, s)	    (SliceobjIsIgnored(CFG_CURRENT, (d), (s)))
#define	slice_aligned(d, s)	    (SliceobjIsRealigned(CFG_CURRENT, (d), (s)))
#define	slice_explicit(d, s)	    (SliceobjIsExplicit(CFG_CURRENT, (d), (s)))
#define	slice_explicit_on(d, s)	    (SliceobjSetBit(CFG_CURRENT, (d), (s), \
						SLF_EXPLICIT))
#define	slice_mntpnt(d, s)	    (Sliceobj_Use(CFG_CURRENT, (d), (s)))
#define	slice_mntpnt_exists(d, s)   (Sliceobj_Use(CFG_CURRENT, (d), (s))[0] != '\0')
#define	slice_mntpnt_not_exists(d, s)	(!slice_mntpnt_exists((d), (s)))
#define	slice_mntpnt_is_fs(d, s)    (Sliceobj_Use(CFG_CURRENT, (d), (s))[0] == '/')
#define	slice_mntpnt_isnt_fs(d, s)  (!slice_mntpnt_is_fs((d), (s)))
#define	slice_is_overlap(d, s)	    (streq(Sliceobj_Use(CFG_CURRENT, (d), (s)), \
						OVERLAP))
#define	slice_is_swap(d, s)	    (streq(Sliceobj_Use(CFG_CURRENT, (d), (s)), \
						SWAP))
/*
 * generic fdisk data macros
 */
#define	Fdiskobj_Config(l, d)		((d)->fdisk[(l)])
#define	Fdiskobj_State(l, d)		(Fdiskobj_Config((l), (d)).state)
#define Fdiskobj_Flag(l, d, b)		(Fdiskobj_State((l), (d)) & (u_char)(b))

/*
 * CFG_CURRENT fdisk data
 */
#define	fdisk_state(d)			(Fdiskobj_State(CFG_CURRENT, (d)))
#define	fdisk_no_flabel(d)		(fdisk_state((d)) & FF_NOFLABEL)

/*
 * generic partition data
 */
#define	Partobj_Config(l, d, p)		(Fdiskobj_Config((l), (d)).part[(p) - 1])
#define	Partobj_Addr(l, d, p)		(&(Partobj_Config((l), (d), (p))))
#define	Partobj_Id(l, d, p)		(Partobj_Config((l), (d), (p)).id)
#define	Partobj_Geom(l, d, p)		(Partobj_Config((l), (d), (p)).geom)
#define	Partobj_GeomAddr(l, d, p)	(&(Partobj_Geom((l), (d), (p))))
#define	Partobj_Active(l, d, p)		(Partobj_Config((l), (d), (p)).active)
#define	Partobj_State(l, d, p)		(Partobj_Config((l), (d), (p)).state)
#define	Partobj_Origpart(l, d, p)	(Partobj_Config((l), (d), (p)).origpart)
#define Partobj_Flag(l, d, p, b)	(Partobj_State((l), (d), (p)) & (u_char)(b))
#define	Partobj_Size(l, d, p)		(Partobj_Geom((l), (d), (p)).tsect)
#define	Partobj_Startsect(l, d, p)	(Partobj_Geom((l), (d), (p)).rsect)
#define	Partobj_Startcyl(l, d, p)	(Partobj_Startsect((l), (d), (p)) / \
						one_cyl((d)))

/*
 * CFG_CURRENT partition data
 */
#define	part_state(d, p)		(Partobj_State(CFG_CURRENT, (d), (p)))
#define	part_geom(d, p)			(Partobj_Geom(CFG_CURRENT, (d), (p)))
#define	part_id(d, p)			(Partobj_Id(CFG_CURRENT, (d), (p)))
#define	part_active(d, p)		(Partobj_Active(CFG_CURRENT, (d), (p)))
#define	part_orig_partnum(d, p)		(Partobj_Origpart(CFG_CURRENT, (d), (p)))
#define	part_is_active(d, p)		(part_active((d), (p)) == ACTIVE)
#define	part_preserved(d, p)		(part_state((d), (p)) & PF_PRESERVED)
#define	part_stuck(d, p)	 	(part_state((d), (p)) & PF_STUCK)
#define	part_stuck_on(d, p)	 	(part_state((d), (p)) |= PF_STUCK)
#define	part_stuck_off(d, p)	 	(part_state((d), (p)) &= ~PF_STUCK)
#define	part_startsect(d, p)		(part_geom((d), (p)).rsect)
#define	part_startcyl(d, p)		(part_geom((d), (p)).rsect / one_cyl((d)))
#define	part_size(d, p)			(part_geom((d), (p)).tsect)
#define	part_geom_dcyl(d, p)		(part_geom((d), (p)).dcyl)
#define	part_geom_tcyl(d, p)		(part_geom((d), (p)).tcyl)
#define	part_geom_firstcyl(d, p) 	(part_geom((d), (p)).firstcyl)
#define	part_geom_lcyl(d, p)	 	(part_geom((d), (p)).lcyl)
#define	part_geom_onecyl(d, p)	 	(part_geom((d), (p)).onecyl)
#define	part_geom_hbacyl(d, p)	 	(part_geom((d), (p)).hbacyl)
#define	part_geom_rsect(d, p)	 	(part_geom((d), (p)).rsect)
#define	part_geom_lsect(d, p)	 	(part_geom((d), (p)).lsect)
#define	part_geom_nsect(d, p)	 	(part_geom((d), (p)).nsect)
#define	part_geom_nhead(d, p)	 	(part_geom((d), (p)).nhead)
#define	part_geom_tsect(d, p)	 	(part_geom((d), (p)).tsect)
#define	part_geom_dsect(d, p)	 	(part_geom((d), (p)).dsect)

/*
 * CFG_COMMIT slice data (OBSOLETE)
 */
#define	comm_slice_start(d, s)		(Sliceobj_Start(CFG_COMMIT, (d), (s)))
#define	comm_slice_size(d, s)	   	(Sliceobj_Size(CFG_COMMIT, (d), (s)))
#define	comm_slice_use(d, s)		(Sliceobj_Use(CFG_COMMIT, (d), (s)))
#define	comm_slice_instance(d, s)	(Sliceobj_Instance(CFG_COMMIT, (d), (s)))
#define	comm_slice_mntopts(d, s)	(Sliceobj_Mountopts(CFG_COMMIT, (d), (s)))
#define	comm_slice_mntpnt(d, s)		(Sliceobj_Use(CFG_COMMIT, (d), (s)))
#define	comm_slice_preserved(d, s)	(Sliceobj_Flag(CFG_COMMIT, (d), (s), \
						SLF_PRESERVED))
/*
 * CFG_EXIST slice data (OBSOLETE)
 */
#define	orig_slice_state(d, s)	  	(Sliceobj_State(CFG_EXIST, (d), (s)))
#define	orig_slice_start(d, s)	  	(Sliceobj_Start(CFG_EXIST, (d), (s)))
#define	orig_slice_size(d, s)	  	(Sliceobj_Size(CFG_EXIST, (d), (s)))
#define	orig_slice_use(d, s)	  	(Sliceobj_Use(CFG_EXIST, (d), (s)))
#define	orig_slice_instance(d, s) 	(Sliceobj_Instance(CFG_EXIST, (d), (s)))
#define	orig_slice_mntopts(d, s)	(Sliceobj_Mountopts(CFG_EXIST, (d), (s)))
#define	orig_slice_locked(d, s)	  	(Sliceobj_Flag(CFG_EXIST, (d), (s), SLF_LOCKED))
#define	orig_slice_aligned(d, s)  	(Sliceobj_Flag(CFG_EXIST, (d), (s), \
						SLF_REALIGNED))
#define	orig_slice_mntpnt(d, s)		(Sliceobj_Use(CFG_EXIST, (d), (s)))

/*
 * CFG_EXIST fdisk partition data
 */
#define	orig_part_id(d, p)	  	(Partobj_Id(CFG_EXIST, (d), (p)))
#define	orig_part_active(d, p)	  	(Partobj_Active(CFG_EXIST, (d), (p)))

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
#define	bytes_to_blocks(d, b) 	(blocks_to_cyls(d, ((b) + 511) / 512) * \
					one_cyl((d)))

/* conversion macros - round up to the nearest cylinder and truncat the unit */

#define	blocks_to_kb_trunc(d, b) 	(blocks_to_blocks((d), (b)) / 2)
#define	blocks_to_mb_trunc(d, b) 	(blocks_to_blocks((d), (b)) / 2048)
#define	bytes_to_blocks_trunc(d, b) 	(blocks_to_cyls(d, ((b)) / 512) * \
						one_cyl((d)))

/*	miscellaneous macros	*/

#define	valid_fdisk_part(p)	((p) >= 1 && (p) <= FD_NUMPART)
#define	invalid_fdisk_part(p)	((p) < 1 || (p) > FD_NUMPART)
#define	valid_sdisk_slice(s)	((s) >= 0 && (s) < numparts)
#define	invalid_sdisk_slice(s)	((s) < 0 || (s) >= numparts)

/*
 * The following macros should be used in install applications for size testing
 * purposes
 */
#define	usable_sdisk_cyls(d)	(sdisk_geom((d)) == NULL ? 0 : \
					(sdisk_geom_dcyl((d)) - \
					(numparts < ALT_SLICE ? 0 : \
					blocks_to_cyls((d), slice_size((d), \
					ALT_SLICE)))))
#define	usable_sdisk_blks(d)	(sdisk_geom((d)) == NULL ? 0 : \
					(sdisk_geom_dsect((d)) - \
					(numparts < ALT_SLICE ? 0 : \
					slice_size((d), ALT_SLICE))))

#define	usable_disk_cyls(d)	(disk_geom_dcyl((d)))
#define	usable_disk_blks(d)	(disk_geom_dsect((d)))
#define	total_sdisk_cyls(d)	(sdisk_geom((d)) == NULL ? 0 : \
					sdisk_geom_tcyl((d)))
#define	total_sdisk_blks(d)	(sdisk_geom((d)) == NULL ? 0 : \
					sdisk_geom_tsect((d)))
/*
 * Total number of cyls/sects (equivalent to the dkg_pcyl value)
 */
#define	total_disk_cyls(d)	(disk_geom_tcyl((d)))
#define	total_disk_blks(d)	(disk_geom_tsect((d)))

/*
 * Total number of accessible cyls/sects (equivalent to the dkg_ncyl value)
 */
#define	accessible_sdisk_cyls(d) (sdisk_geom((d)) == NULL ? 0 : \
					sdisk_geom_lcyl((d)))
#define	accessible_sdisk_blks(d) (sdisk_geom((d)) == NULL ? 0 : \
					sdisk_geom_lsect((d)))
/* miscellaneous macros */

#define	sdisk_is_usable(d)	(((d) != NULL) && \
					disk_selected((d)) && \
					sdisk_geom_not_null((d)))
#define	sdisk_not_usable(d)	(sdisk_is_usable((d)) == 0)

/* globals */

extern int	numparts;	/* number of slices supported */
extern char	err_text[];	/* OBSOLETE */

/* functional prototypes */

#ifdef __cplusplus
extern "C" {
#endif

/* store_bootobj.c */

int		BootobjCommit(void);
int		BootobjRestore(Label_t);
int		BootobjGetAttribute(Label_t, ...);
int		BootobjSetAttribute(Label_t, ...);
int		BootobjCompare(Label_t, Label_t, int);
int		BootobjConflicts(Label_t, char *, int);
int		BootobjIsExplicit(Label_t, BootobjAttrType);

/* store_common.c */
u_int		blocks2size(Disk_t *, u_int, int);
Units_t		get_units(void);
char *		library_error_msg(int);
Units_t		set_units(Units_t);
u_int		size2blocks(Disk_t *, u_int);

/* store_initdisk.c */

int		DiskobjInitList(char *);
Disk_t *	DiskobjCreate(char *);

/* store_fdisk.c */
int		adjust_part_starts(Disk_t *);
int		find_mnt_pnt(Disk_t *, char *, char *, Mntpnt_t *, Label_t);
int		fdisk_space_avail(Disk_t *);
int		get_solaris_part(Disk_t *, Label_t);
int		part_geom_same(Disk_t *, int, Label_t);
int		part_overlaps(Disk_t *, int, int, int, int **);
int		max_size_part_hole(Disk_t *, int);
int		set_part_attr(Disk_t *, int, int, int);
int		set_part_geom(Disk_t *, int, int, int);
int		set_part_preserve(Disk_t *, int, int);

/* store_debug.c */
void		DiskobjPrint(Label_t, Disk_t *);
void		BootobjPrint(void);
void		print_disk(Disk_t *, char *);

/* store_check.c */
int		check_disk(Disk_t *);
int		check_disks(void);
int		check_fdisk(Disk_t *);
int		check_sdisk(Disk_t *);
void		free_error_list(void);
Errmsg_t	*get_error_list(void);
Errmsg_t *	worst_error(void);
int		validate_fdisk(Disk_t *);

/* store_boot.c */
int		DiskobjFindBoot(Label_t, Disk_t **);

/* store_disk.c */
int		commit_disk_config(Disk_t *);
int		commit_disks_config(void);
int		deselect_disk(Disk_t *, char *);
Disk_t *	find_disk(char *);
Disk_t *	first_disk(void);
Disk_t *	next_disk(Disk_t *);
int		restore_disk(Disk_t *, Label_t);
int		select_disk(Disk_t *, char *);

/* store_rwdisklist.c */
int WriteDiskList(void);
int ReadDiskList(Disk_t **HeadDP);

/* store_sdisk.c */
int		adjust_slice_starts(Disk_t *);
int		filesys_preserve_ok(char *);
int		get_slice_autoadjust(void);
int		sdisk_compare(Disk_t *, Label_t);
int		sdisk_geom_same(Disk_t *, Label_t);
int		sdisk_max_hole_size(Disk_t *);
int		sdisk_space_avail(Disk_t *);
int		set_slice_autoadjust(int);
int		set_slice_geom(Disk_t *, int, int, int);
int		set_slice_ignore(Disk_t *, int, int);
int		set_slice_mnt(Disk_t *, int, char *, char *);
int		set_slice_preserve(Disk_t *, int, int);
int		slice_name_ok(char *);
int		slice_overlaps(Disk_t *, int, int, int, int **);
int		slice_preserve_ok(Disk_t *, int);
int		SdiskobjRootSetBoot(Disk_t *dp, int slice);
int		SliceobjSetAttribute(Disk_t *, int, ...);
int		SliceobjGetAttribute(Label_t, Disk_t *, int, ...);
SliceKey *	SliceobjFindUse(Label_t, Disk_t *, char *, int, int);

#ifdef __cplusplus
}
#endif

#endif	/* _SPMISTORE_API_H */
