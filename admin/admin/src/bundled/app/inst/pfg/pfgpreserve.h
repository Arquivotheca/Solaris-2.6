#ifndef lint
#pragma ident "@(#)pfgpreserve.h 1.8 95/11/06 SMI"
#endif

/*
 * Copyright (c) 1994-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	pfgpreserve.h
 * Group:	installtool
 * Description:
 */

#ifndef _PFGPRESERVE_H
#define	_PFGPRESERVE_H

typedef struct preserveStruct {
	Widget diskSlice;	/* label containing ctds name of disk slice */
	Widget mountField;	/* text widget containing mount pt name */
	Widget preserveButton;	/* preserve toggle button */
	Widget sizeField;	/* slice size field */
	int slice;		/* disk slice */
	Disk_t *disk;		/* disk */
	struct preserveStruct *next;
} PreserveStruct;

#endif	/* _PFGPRESERVE_H */
