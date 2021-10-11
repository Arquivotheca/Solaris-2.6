#ifndef lint
#pragma ident "@(#)pfgdisks.h 1.7 95/11/07 SMI"
#endif

/*
 * Copyright (c) 1994-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	pfgdisks.h
 * Group:	installtool
 * Description:
 */

#ifndef _PFDISKS_H
#define	_PFDISKS_H

extern Widget solarpart_dialog;

extern void updateDiskCells();
extern Widget *createCellArray(int);

typedef enum {
	mountCellType,
	sizeCellType,
	startCellType,
	endCellType
} CellType;

typedef struct diskw_tag {
	struct diskw_tag *next;
	struct disk *d;
	Widget frame, header, total1, total2, total3, total4, cylinder;
	Widget	rounding_widget;
} pfDiskW_t;

/* stucture containing widget id's used to display slice info */
typedef struct {
	Widget mountWidget;
	Widget sizeWidget;
	Widget startWidget;
	Widget endWidget;
} SliceWidgets;

/* structure to temporarily save disk information */
typedef struct {
	int sliceSize;
	int sliceStart;
	char *sliceMount;
	char stuckState;
} TmpDiskStruct;
extern void updateTotals(pfDiskW_t *);

#endif /* _PFDISKS_H */
