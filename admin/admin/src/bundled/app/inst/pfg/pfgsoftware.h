#ifndef lint
#pragma ident "@(#)pfgsoftware.h 1.12 95/12/20 SMI"
#endif

/*
 * Copyright (c) 1994-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	pfgsoftware.h
 * Group:	installtool
 * Description:
 */

#ifndef	_PFGSOFTWARE_H
#define	_PFGSOFTWARE_H

#define	SIZE_LENGTH 20
#define	MOUNTP_LENGTH 20 /* string length of space->mount */
#define	INDENT 20
#define	SELECT_OFFSET 10
typedef struct {
	Widget select;		/* selection button widget, contains pointer */
				/* to cluster/package module */
	ModStatus orig_status;	/* original status of module */
	int level;		/* indention level of module */
	Widget size;
} SelectionList;

typedef struct {
	Pixmap select;
	Pixmap unselect;
	Pixmap partial;
	Pixmap required;
} SelectPixmaps;

extern void CreateList(Widget parent_rc, Module *module, int level,
	Widget info);
extern void CreateEntry(Widget parent, Widget info, int level,
	Module *module, Widget *expand);
extern Module *get_sub_meta_all(Module * module);
extern void SetSelection(Widget select, XtPointer selectValue,
	XtPointer *call_data);

extern void ExpandCluster(Widget button, XtPointer client_data, XtPointer
	call_data);
extern void update_selection(Widget select, ModStatus selectStatus);
extern void pfgupdate_software();
extern void pfgselection(Module *module, ModStatus status);
extern void initializeList();
extern void showDependencies();
extern void CreateSelectPixmap(Widget button, Pixmap *unselectPixmap,
	Pixmap *selectPixmap, Pixmap *partialPixmap,
	Pixmap *requiredPixmap);

#define	MAX_SPACE_FS 15 	/* maximum number of file systems in space */
				/* calculations */
#define	PF_ADD 1		/* add value for software packages */
#define	PF_REMOVE 0		/* remove value for software packages */
#define	SUCCESS 0

#endif	/* _PFGSOFTWARE_H */
