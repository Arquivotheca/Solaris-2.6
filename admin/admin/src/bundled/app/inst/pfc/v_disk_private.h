#ifndef lint
#pragma ident "@(#)v_disk_private.h 1.3 95/11/06 SMI"
#endif

/*
 * Copyright (c) 1993-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	v_disk_private.h
 * Group:	ttinstall
 * Description:
 */

#ifndef _V_DISK_PRIVATE_H
#define	_V_DISK_PRIVATE_H

/*
 * data and structures/typedef's that are shared between v_sdisk.c and
 * v_fdisk.c.
 *
 * This include file is private to the v_?disk modules and should not be used
 * outside of them
 */

/*
 * disk structure, internal to the v_disk code.
 */
typedef struct {
	Disk_t *info;		/* pointer to disk lib's disk struct */
	V_DiskStatus_t status;	/* v_disk code 'status' indicator.   */
} V_Disk_t;


extern V_Disk_t *_disks;	/* array of disks, internal to v_disk code. */

extern Disk_t *_current_disk;
extern int _current_disk_index;
extern int _num_disks;

extern char _disk_err_buf[];

#endif	/* _V_DISK_PRIVATE_H */
