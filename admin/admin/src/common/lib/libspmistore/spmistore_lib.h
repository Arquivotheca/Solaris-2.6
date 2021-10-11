#ifndef lint
#pragma ident "@(#)spmistore_lib.h 1.14 96/07/17 SMI"
#endif

/*
 * Copyright (c) 1995-1996 by Sun Microsystems, Inc. All rights reserved.
 */
/*
 * Module:	spmistore_lib.h
 * Group:	libspmistore
 * Description:
 */

#ifndef _SPMISTORE_LIB_H
#define	_SPMISTORE_LIB_H

#include "spmistore_api.h"

/* constants */

#define	NUMALTCYL	2	/* default alt cyl (dkg_acyl) size in cyl */

/*
 * disk structure access macros (restricted for RH use)
 */
/*
 * physical disk data
 */
#define	disk_geom(d)		((d)->geom)
#define	disk_geom_addr(d)	(&(d)->geom)
#define	disk_state(d)		((d)->state)
#define	disk_state_test(d, b)	((d)->state & (u_short)(b))
#define	disk_state_set(d, b)	((d)->state |= (u_short)(b))
#define	disk_state_unset(d, b)	((d)->state &= ~(u_short)(b))
#define	disk_state_clear(d, b)	((d)->state = (u_short)0)
#define	disk_select_on(d)	((d)->state |= DF_SELECTED)
#define	disk_select_off(d)	((d)->state &= ~DF_SELECTED)
#define	disk_initialized_on(d)	((d)->state |= DF_INIT)
#define	disk_initialized_off(d)	((d)->state &= ~DF_INIT)
#define	disk_ctype_set(d, c)	((d)->ctype = (u_short)(c))
#define	disk_ctype_clear(d)	((d)->ctype = (u_short)0)
#define	disk_cname_set(d, n)	((void) strcpy((d)->cname, (n)))

/*
 * configuration explicit slice data access
 */
#define SdiskobjSetBit(l, d, b)		(Sdiskobj_State((l), (d)) |= (u_char)(b))
#define SdiskobjClearBit(l, d, b)	(Sdiskobj_State((l), (d)) &= ~(u_char)(b))

/*
 * configuration explicit fdisk data access
 */
#define	Fdiskobj_Addr(l, d)		(&((d)->fdisk[(l)]))
#define FdiskobjSetBit(l, d, b)		(Fdiskobj_State((l), (d)) |= (u_char)(b))
#define FdiskobjClearBit(l, d, b)	(Fdiskobj_State((l), (d)) &= ~(u_char)(b))

/*
 * CFG_CURRENT fdisk partition data
 */
#define	fdisk_part_addr(d, p)		(Partobj_Addr(CFG_CURRENT, (d), (p)))
#define	part_state_clear(d, p)		(part_state((d), (p)) = (u_char)0)
#define	part_preserve_on(d, p)		(part_state((d), (p)) |= PF_PRESERVED)
#define	part_preserve_off(d, p)		(part_state((d), (p)) &= ~PF_PRESERVED)
#define	part_size_set(d, p, c)		(part_geom((d), (p)).tcyl = (int)(c))
#define	part_size_clear(d, p)		(part_geom((d), (p)).tcyl = (int)0)
#define	part_start_set(d, p, b)		(part_geom((d), (p)).rsect = (int)(b))

/*
 * CFG_EXIST fdisk partition data
 */
#define	orig_part_startsect(d, p)	(Partobj_Geom(CFG_EXIST, (d), (p)).rsect)
#define	orig_part_size(d, p)		(Partobj_Geom(CFG_EXIST, (d), (p)).tsect)
#define	orig_part_startcyl(d, p)	((orig_part_startsect((d), (p)) + \
					    (one_cyl((d)) / 2)) / one_cyl((d)))

/* function prototypes */

#ifdef __cplusplus
extern "C" {
#endif

/* store_boot.c */
void		BootDefault(char **, int *);

/* store_bootobj.c */
int		BootobjSetAttributePriv(Label_t, ...);
int		BootobjInit(void);

/* store_fdisk.c */
void		FdiskobjReset(Disk_t *);

/* store_disk.c */
int		_disk_is_scsi(const Disk_t *);
int		DiskobjSave(Label_t, Disk_t *);
int		FdiskobjRestore(Label_t, Disk_t *);
int		SdiskobjRestore(Label_t, Disk_t *);
void		DiskobjAddToList(Disk_t *);
void		DiskobjDestroy(Disk_t *);

/* store_sdisk.c */
int		_reset_sdisk(Disk_t *);
int		SliceobjSetAttributePriv(Disk_t *, const int, ...);
int		SliceobjGetAttributePriv(const Label_t, const Disk_t *,
			const int, ...);
int		SliceobjIsAllocated(const Label_t, const Disk_t *, const int);
int		SliceobjCountUse(const Label_t, const Disk_t *, const char *);

#ifdef __cplusplus
}
#endif

#endif	/* _SPMISTORE_LIB_H */
