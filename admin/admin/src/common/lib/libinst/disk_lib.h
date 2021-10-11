#ifndef	lint
#pragma ident "@(#)disk_lib.h 1.67 95/02/17"
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
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 * Government is subject to restrictions as set forth in subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 52.227-7013 and in similar clauses in the FAR and
 * NASA FAR Supplement.
 */
#ifndef	_DISK_LIB_H
#define	_DISK_LIB_H

#include "disk_api.h"
#include "disk_strings.h"

#define	NUM_CLIENTS	5	/* dflt # of client for server install */
#define	NUMALTCYL	2	/* default alt cyl (dkg_acyl) size in cyl */

/* new tag field type for S495 - will be part of <sys/vtoc.h> */

#ifndef	V_CACHE
#define	V_CACHE	0x0a
#endif

/* number of valid entries in the defmnts[] array */

extern int	numdefmnt;
#define	NUMDEFMNT	11

/* slice specifiers used in configuraiton processes */

#define	WILD_SLICE	-1	/* any slice is acceptable */
#define	SPEC_SLICE	 0	/* only the specified slice is acceptable */

/* String processing macro */

#define	TEXT_DOMAIN	"SUNW_INSTALL_LIB"
#define	ILIBSTR(x)	dgettext(TEXT_DOMAIN, x)

extern char		*_defmplist[];
extern int		_diskfile_load;

/* ******************************************************************** */
/*		EXTERNAL REFERENCES FOR LIBRARY FUNCTIONS		*/
/* ******************************************************************** */

/* disk.c */

extern int	_restore_sdisk_orig(Disk_t *);
extern int	_restore_fdisk_orig(Disk_t *);
extern int	_restore_sdisk_commit(Disk_t *);
extern int	_restore_fdisk_commit(Disk_t *);
extern void	_init_commit_orig(Disk_t *);
extern Disk_t *	_init_bootdisk(void);

/* disk_prom.c */

extern char *	_eeprom_default(void);

/* disk_sdisk.c */

extern int	_reset_sdisk(Disk_t *);
extern int	_reset_slice(Disk_t *, int);
extern int	_null_sdisk(Disk_t *);
extern int	_pres_ok(Disk_t *, int);
extern int	_set_slice_start(Disk_t *, int, int);
extern int	_set_slice_size(Disk_t *, int, int);

/* disk_util.c */

extern int	_system_fs_ancestor(char *);
extern int	_whole_disk_name(char *, char *);
extern void	_sort_disks(void);
extern void	_set_first_disk(Disk_t *);
extern void	_set_next_disk(Disk_t *, Disk_t *);
extern int	_map_to_effective_dev(char *, char *);
extern int	_disk_is_scsi(Disk_t *);
extern void	_sort_fdisk_input(Disk_t *);
extern int	_calc_memsize(void);

/* disk_fdisk.c */

extern int	_reset_fdisk(Disk_t *);
extern int	_set_part_size(Disk_t *, int, int);
extern int	_set_part_start(Disk_t *, int, int);

/* disk_dflt.c */

extern int	_setup_sdisk_default(Disk_t *);
extern int	_setup_fdisk_default(Disk_t *);

/* disk_find.c */

extern Disk_t *	_alloc_disk(char *);
extern void	_dealloc_disk(Disk_t *);
extern void	_add_disk_to_list(Disk_t *);
extern void	_lock_unusable_slices(Disk_t *);
extern void	_mark_overlap_slices(Disk_t *);

/* macros for manipulating parts of the physical disk structure	*/

#define	disk_geom(d)		((d)->geom)
#define	disk_geom_addr(d)	(&(d)->geom)
#define	disk_sdisk(d)		((d)->sdisk)
#define	disk_fdisk(d)		((d)->fdisk)
#define	disk_fdisk_addr(d)	(&(d)->fdisk)
#define	disk_sdisk_addr(d)	(&(d)->sdisk)
#define	disk_ofdisk_addr(d)	(&(d)->o_fdisk)
#define	disk_osdisk_addr(d)	(&(d)->o_sdisk)
#define	disk_cfdisk_addr(d)	(&(d)->c_fdisk)
#define	disk_csdisk_addr(d)	(&(d)->c_sdisk)
#define	disk_state(d)		((d)->state)
#define	disk_state_set(d, b)	((d)->state |= (u_short)(b))
#define	disk_state_unset(d, b)	((d)->state &= ~(u_short)(b))
#define	disk_state_clear(d, b)	((d)->state = (u_short)0)
#define	disk_bootdrive_on(d)	((d)->state |= DF_BOOTDRIVE)
#define	disk_bootdrive_off(d)	((d)->state &= ~DF_BOOTDRIVE)
#define	disk_select_on(d)	((d)->state |= DF_SELECTED)
#define	disk_select_off(d)	((d)->state &= ~DF_SELECTED)
#define	disk_initialized_on(d)	((d)->state |= DF_INIT)
#define	disk_initialized_off(d)	((d)->state &= ~DF_INIT)
#define	disk_ctype_set(d, c)	((d)->ctype = (u_short)(c))
#define	disk_ctype_clear(d)	((d)->ctype = (u_short)0)
#define	disk_cname_set(d, n)	((void) strcpy((d)->cname, (n)))
#define	disk_cname_clear(d)	((d)->cname[0] = '\0')

/* macros for manipulating components of the current S-disk fields */

#define	sdisk_geom(d)		((d)->sdisk.geom)
#define	sdisk_geom_set(d, b)	((d)->sdisk.geom = (Geom_t *)(b))
#define	sdisk_geom_clear(d)	((d)->sdisk.geom = (Geom_t *)0)
#define	sdisk_state(d)		((d)->sdisk.state)
#define	sdisk_state_set(d, b)	((d)->sdisk.state |= (u_char)(b))
#define	sdisk_state_unset(d, b)	((d)->sdisk.state &= ~(u_char)(b))
#define	sdisk_state_clear(d)	((d)->sdisk.state = (u_char)0)
#define	sdisk_set_touch2(d) 	((d)->sdisk.state |= SF_S2MOD)
#define	sdisk_unset_touch2(d) 	((d)->sdisk.state &= ~SF_S2MOD)
#define	sdisk_set_illegal(d)	((d)->sdisk.state |= SF_ILLEGAL)
#define	sdisk_unset_illegal(d)	((d)->sdisk.state &= ~SF_ILLEGAL)

/* macros for accessing components of the original S-disk fields */

#define	orig_sdisk(d)			((d)->o_sdisk)
#define	orig_sdisk_geom(d)		((d)->o_sdisk.geom)
#define	orig_sdisk_geom_set(d, b)	((d)->o_sdisk.geom = (Geom_t *)(b))

#define	orig_sdisk_geom_null(d)		((d)->o_sdisk.geom == (Geom_t *)0)
#define	orig_sdisk_geom_dcyl(d)		((d)->o_sdisk.geom->dcyl)
#define	orig_sdisk_geom_tcyl(d)		((d)->o_sdisk.geom->tcyl)
#define	orig_sdisk_geom_firstcyl(d)	((d)->o_sdisk.geom->firstcyl)
#define	orig_sdisk_geom_lcyl(d)		((d)->o_sdisk.geom->lcyl)
#define	orig_sdisk_geom_onecyl(d)	((d)->o_sdisk.geom->onecyl)
#define	orig_sdisk_geom_rsect(d)	((d)->o_sdisk.geom->rsect)

#define	orig_sdisk_state(d)		((d)->o_sdisk.state)
#define	orig_sdisk_state_set(d, b)	((d)->o_sdisk.state |= (u_char)(b))
#define	orig_sdisk_state_unset(d, b)	((d)->o_sdisk.state &= ~(u_char)(b))
#define	orig_sdisk_state_clear(d)	((d)->o_sdisk.state = (u_char)0)
#define	orig_sdisk_touch2(d)   		((d)->o_sdisk.state & SF_S2MOD)
#define	orig_sdisk_set_touch2(d) 	((d)->o_sdisk.state |= SF_S2MOD)
#define	orig_sdisk_unset_touch2(d) 	((d)->o_sdisk.state &= ~SF_S2MOD)
#define	orig_sdisk_legal(d)		(!((d)->o_sdisk.state & SF_ILLEGAL))
#define	orig_sdisk_not_legal(d) 	((d)->o_sdisk.state & SF_ILLEGAL)

/* macros for accessing components of the committed S-disk fields */

#define	comm_sdisk(d)			((d)->c_sdisk)
#define	comm_sdisk_geom(d)		((d)->c_sdisk.geom)
#define	comm_sdisk_geom_set(d, b)	((d)->c_sdisk.geom = (Geom_t *)(b))

#define	comm_sdisk_geom_null(d)		((d)->c_sdisk.geom == (Geom_t *)0)
#define	comm_sdisk_geom_dcyl(d)		((d)->c_sdisk.geom->dcyl)
#define	comm_sdisk_geom_tcyl(d)		((d)->c_sdisk.geom->tcyl)
#define	comm_sdisk_geom_firstcyl(d)	((d)->c_sdisk.geom->firstcyl)
#define	comm_sdisk_geom_lcyl(d)		((d)->c_sdisk.geom->lcyl)
#define	comm_sdisk_geom_onecyl(d)	((d)->c_sdisk.geom->onecyl)
#define	comm_sdisk_geom_rsect(d)	((d)->c_sdisk.geom->rsect)

#define	comm_sdisk_state(d)		((d)->c_sdisk.state)
#define	comm_sdisk_state_set(d, b)	((d)->c_sdisk.state |= (u_char)(b))
#define	comm_sdisk_state_unset(d, b)	((d)->c_sdisk.state &= ~(u_char)(b))
#define	comm_sdisk_state_clear(d)	((d)->c_sdisk.state = (u_char)0)
#define	comm_sdisk_touch2(d)   		((d)->c_sdisk.state & SF_S2MOD)
#define	comm_sdisk_set_touch2(d) 	((d)->c_sdisk.state |= SF_S2MOD)
#define	comm_sdisk_unset_touch2(d) 	((d)->c_sdisk.state &= ~SF_S2MOD)
#define	comm_sdisk_legal(d)		(!((d)->c_sdisk.state & SF_ILLEGAL))
#define	comm_sdisk_not_legal(d)		((d)->c_sdisk.state & SF_ILLEGAL)

/* macros for manipulating components of current S-disk slice structures */

#define	slice_state(d, s)	   ((d)->sdisk.slice[(s)].state)
#define	slice_addr(d, s)	   (&(d)->sdisk.slice[(s)])
#define	slice_state_set(d, s, b)   ((d)->sdisk.slice[(s)].state |= (u_char)(b))
#define	slice_state_unset(d, s, b) ((d)->sdisk.slice[(s)].state &= ~(u_char)(b))
#define	slice_state_clear(d, s)	   ((d)->sdisk.slice[(s)].state = (u_char)0)
#define	slice_preserve_on(d, s)	   ((d)->sdisk.slice[(s)].state |= SLF_PRESERVE)
#define	slice_preserve_off(d, s)   ((d)->sdisk.slice[(s)].state &= ~SLF_PRESERVE)
#define	slice_ignore_on(d, s)	   ((d)->sdisk.slice[(s)].state |= SLF_IGNORE)
#define	slice_ignore_off(d, s)	   ((d)->sdisk.slice[(s)].state &= ~SLF_IGNORE)
#define	slice_aligned_on(d, s)	   ((d)->sdisk.slice[(s)].state |= SLF_ALIGNED)
#define	slice_lock_on(d, s)	   ((d)->sdisk.slice[(s)].state |= SLF_LOCK)
#define	slice_lock_off(d, s)	   ((d)->sdisk.slice[(s)].state &= ~SLF_LOCK)
#define	slice_is_overlap(d, s)	   (strcmp(slice_mntpnt((d), (s)), OVERLAP) == 0)
#define	slice_isnt_overlap(d, s)   (strcmp(slice_mntpnt((d), (s)), OVERLAP) != 0)
#define	slice_is_swap(d, s)	   (strcmp(slice_mntpnt((d), (s)), SWAP) == 0)
#define	slice_isnt_swap(d, s)	   (strcmp(slice_mntpnt((d), (s)), SWAP) != 0)
#define	slice_is_alts(d, s)	   (strcmp(slice_mntpnt((d), (s)), ALTSECTOR) == 0)

#define	slice_addr(d, s)	   (&(d)->sdisk.slice[(s)])
#define	slice_start_set(d, s, b)   ((d)->sdisk.slice[(s)].start = (int)(b))
#define	slice_size_set(d, s, b)	   ((d)->sdisk.slice[(s)].size = (int)(b))
#define	slice_size_clear(d, s)	   ((d)->sdisk.slice[(s)].size = (int)0)
#define	slice_mntpnt_set(d, s, f)  ((void) strcpy((d)->sdisk.slice[(s)].mntpnt, (f)))
#define	slice_mntpnt_clear(d, s)   ((d)->sdisk.slice[(s)].mntpnt[0] = '\0')
#define	slice_mntopts_set(d, s, m) ((d)->sdisk.slice[(s)].mntopts = (char *)(m))
#define	slice_mntopts_clear(d, s)  free((d)->sdisk.slice[(s)].mntopts); \
					(d)->sdisk.slice[(s)].mntopts = NULL

/* macros for manipulating components of the original S-disk slice structures */

#define	orig_slice_state(d, s)		((d)->o_sdisk.slice[(s)].state)
#define	orig_slice_addr(d, s)		(&(d)->o_sdisk.slice[(s)])
#define	orig_slice_state_set(d, s, b)	((d)->o_sdisk.slice[(s)].state |= (u_char)(b))
#define	orig_slice_state_unset(d, s, b)	((d)->o_sdisk.slice[(s)].state &= ~(u_char)(b))
#define	orig_slice_state_clear(d, s)	((d)->o_sdisk.slice[(s)].state = (u_char)0)
#define	orig_slice_ignored(d, s)	((d)->o_sdisk.slice[(s)].state & SLF_IGNORE)
#define	orig_slice_not_ignored(d, s)	(!((d)->o_sdisk.slice[(s)].state & SLF_IGNORE))
#define	orig_slice_start_set(d, s, b)	((d)->o_sdisk.slice[(s)].start = (int)(b))
#define	orig_slice_size_set(d, s, b)	((d)->o_sdisk.slice[(s)].size = (int)(b))
#define	orig_slice_size_clear(d, s)	((d)->o_sdisk.slice[(s)].size = (int)0)
#define	orig_slice_mntpnt_set(d, s, f)	((void) strcpy((d)->o_sdisk.slice[(s)].mntpnt, (f)))
#define	orig_slice_mntpnt_clear(d, s)	((d)->o_sdisk.slice[(s)].mntpnt[0] = '\0')
#define	orig_slice_mntopts_set(d, s, m)	((d)->o_sdisk.slice[(s)].mntopts = (char *)(m))
#define	orig_slice_mntopts_clear(d, s)	free((d)->o_sdisk.slice[(s)].mntopts); \
					(d)->o_sdisk.slice[(s)].mntopts = NULL

/* macros for manipulating components of the committed S-disk slice structures */

#define	comm_slice_state(d, s)		((d)->c_sdisk.slice[(s)].state)
#define	comm_slice_addr(d, s)		(&(d)->c_sdisk.slice[(s)])
#define	comm_slice_state_set(d, s, b)	((d)->c_sdisk.slice[(s)].state |= (u_char)(b))
#define	comm_slice_state_unset(d, s, b)	((d)->c_sdisk.slice[(s)].state &= ~(u_char)(b))
#define	comm_slice_state_clear(d, s)	((d)->c_sdisk.slice[(s)].state = (u_char)0)
#define	comm_slice_start_set(d, s, b)	((d)->c_sdisk.slice[(s)].start = (int)(b))
#define	comm_slice_ignored(d, s)	((d)->c_sdisk.slice[(s)].state & SLF_IGNORE)
#define	comm_slice_not_ignored(d, s)	(!((d)->c_sdisk.slice[(s)].state & SLF_IGNORE))
#define	comm_slice_size_set(d, s, b)	((d)->c_sdisk.slice[(s)].size = (int)(b))
#define	comm_slice_size_clear(d, s)	((d)->c_sdisk.slice[(s)].size = (int)0)
#define	comm_slice_mntpnt_set(d, s, f)	((void) strcpy((d)->c_sdisk.slice[(s)].mntpnt, (f)))
#define	comm_slice_mntpnt_clear(d, s)	((d)->c_sdisk.slice[(s)].mntpnt[0] = '\0')
#define	comm_slice_mntopts_set(d, s, m)	((d)->c_sdisk.slice[(s)].mntopts = (char *)(m))
#define	comm_slice_mntopts_clear(d, s)	free((d)->c_sdisk.slice[(s)].mntopts); (d)->c_sdisk.slice[(s)].mntopts = NULL

/*  macros for manipulating current F-disk struct data 	*/

#define	fdisk_part_addr(d, p)		(&((d)->fdisk.part[(p) - 1]))
#define	fdisk_state(d)			((d)->fdisk.state)
#define	fdisk_state_set(d, b)		((d)->fdisk.state |= (u_char)(b))
#define	fdisk_state_unset(d, b)		((d)->fdisk.state &= ~(u_char)(b))
#define	fdisk_state_clear(d)		((d)->fdisk.state = (u_char)0)

/*  macros for manipulating original F-disk struct data	*/

#define	orig_fdisk(d)			((d)->o_fdisk)
#define	orig_fdisk_part_addr(d, p)	(&((d)->o_fdisk.part[(p) - 1]))
#define	orig_fdisk_state(d)		((d)->o_fdisk.state)
#define	orig_fdisk_state_set(d, b)	((d)->o_fdisk.state |= (u_char)(b))
#define	orig_fdisk_state_unset(d, b)	((d)->o_fdisk.state &= ~(u_char)(b))
#define	orig_fdisk_state_clear(d, b)	((d)->o_fdisk.state = (u_char)0)

/*  macros for manipulating committed F-disk struct data  */

#define	comm_fdisk(d)			((d)->c_fdisk)
#define	comm_fdisk_part_addr(d, p)	(&((d)->c_fdisk.part[(p) - 1]))
#define	comm_fdisk_state(d)		((d)->c_fdisk.state)
#define	comm_fdisk_state_set(d, b)	((d)->c_fdisk.state |= (u_char)(b))
#define	comm_fdisk_state_unset(d, b)	((d)->c_fdisk.state &= ~(u_char)(b))
#define	comm_fdisk_state_clear(d, b)	((d)->c_fdisk.state = (u_char)0)

/*  macros for accessing/manipulating current F-disk partition data  */

#define	part_state_clear(d, p)	  ((d)->fdisk.part[(p) - 1].state = (u_char)0)
#define	part_preserve_on(d, p)	  ((d)->fdisk.part[(p) - 1].state |= PF_PRESERVE)
#define	part_preserve_off(d, p)	  ((d)->fdisk.part[(p) - 1].state &= ~PF_PRESERVE)
#define	part_active(d, p)	  ((d)->fdisk.part[(p) - 1].active)
#define	part_active_set(d, p, b)  ((d)->fdisk.part[(p) - 1].active = (int)(b))
#define	part_id_set(d, p, t)	  ((d)->fdisk.part[(p) - 1].id = (int)(t))
#define	part_geom(d, p)		  ((d)->fdisk.part[(p) - 1].geom)
#define	part_geom_addr(d, p)	  (&(d)->fdisk.part[(p) - 1].geom)
#define	part_size_set(d, p, c)	  ((d)->fdisk.part[(p) - 1].geom.tcyl = (int)(c))
#define	part_size_clear(d, p)	  ((d)->fdisk.part[(p) - 1].geom.tcyl = (int)0)
#define	part_start_set(d, p, b)	  ((d)->fdisk.part[(p) - 1].geom.rsect = (int)(b))
#define	part_start_clear(d, p)	  ((d)->fdisk.part[(p) - 1].geom.rsect = (int)0)

/* macros for accessing/manipulating original F-disk partition data */

#define	orig_part_size(d, p)		((d)->o_fdisk.part[(p) - 1].geom.tsect)
#define	orig_part_startcyl(d, p)	(((d)->o_fdisk.part[(p) - 1].geom.rsect + (one_cyl((d)) / 2)) / one_cyl((d)))
#define	orig_part_startsect(d, p)	((d)->o_fdisk.part[(p) - 1].geom.rsect)
#define	orig_part_geom(d, p)		((d)->o_fdisk.part[(p) - 1].geom)
#define	orig_part_geom_addr(d, p)	(&(d)->o_fdisk.part[(p) - 1].geom)
#define	orig_part_geom_dcyl(d, p)	((d)->o_fdisk.part[(p) - 1].geom.dcyl)
#define	orig_part_geom_tcyl(d, p)	((d)->o_fdisk.part[(p) - 1].geom.tcyl)
#define	orig_part_geom_firstcyl(d, p)	((d)->o_fdisk.part[(p) - 1].geom.firstcyl)
#define	orig_part_geom_lcyl(d, p)	((d)->o_fdisk.part[(p) - 1].geom.lcyl)
#define	orig_part_geom_onecyl(d, p)	((d)->o_fdisk.part[(p) - 1].geom.onecyl)
#define	orig_part_geom_rsect(d, p)	((d)->o_fdisk.part[(p) - 1].geom.rsect)

/* macros for accessing/manipulating committed F-disk partition data */

#define	comm_part_size(d, p)		((d)->c_fdisk.part[(p) - 1].geom.tsect)
#define	comm_part_startcyl(d, p)	(((d)->c_fdisk.part[(p) - 1].geom.rsect + \
		(one_cyl((d)) / 2)) / one_cyl((d)))
#define	comm_part_startsect(d, p)	((d)->c_fdisk.part[(p) - 1].geom.rsect)
#define	comm_part_geom(d, p)		((d)->c_fdisk.part[(p) - 1].geom)
#define	comm_part_geom_addr(d, p)	(&(d)->c_fdisk.part[(p) - 1].geom)
#define	comm_part_geom_dcyl(d, p)	((d)->c_fdisk.part[(p) - 1].geom.dcyl)
#define	comm_part_geom_tcyl(d, p)	((d)->c_fdisk.part[(p) - 1].geom.tcyl)
#define	comm_part_geom_firstcyl(d, p)	((d)->c_fdisk.part[(p) - 1].geom.firstcyl)
#define	comm_part_geom_lcyl(d, p)	((d)->c_fdisk.part[(p) - 1].geom.lcyl)
#define	comm_part_geom_onecyl(d, p)	((d)->c_fdisk.part[(p) - 1].geom.onecyl)
#define	comm_part_geom_rsect(d, p)	((d)->c_fdisk.part[(p) - 1].geom.rsect)

#endif /* _DISK_LIB_H */
