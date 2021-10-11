#ifndef lint
#pragma ident "@(#)pfgsolarcust.c 1.43 96/07/08 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	pfgsolarcust.c
 * Group:	installtool
 * Description:
 */

#include "pf.h"
#include "pfg.h"

#include "pfgSolarCust_ui.h"

typedef enum {
	SolarButton = 0,
	DOSHugeButton = 1,
	UnusedButton = 2,
	OtherOSButton = 3
} TypeMenuButtonId;

static void solarcustOkCB(Widget w, XtPointer, XtPointer);
static void solarcustCancelCB(Widget, XtPointer, XtPointer);
static void createPartRow(Disk_t *);
static void solarcustTypeCB(Widget, XtPointer, XtPointer);
static void sizePartLosingFocus(Widget, XtPointer, XtPointer);
static void sizePartActivateCB(Widget, XtPointer, XtPointer);
static void sizePartVerifyCB(Widget, XtPointer, XtPointer);
static TypeMenuButtonId getCurrentType(Disk_t *, int);
static void setMenuSensitivity(Disk_t *);
static void setSolarSensitivity(Disk_t *);
static void calculateTotals(Disk_t *);
static int sizePartChanged(Widget, Disk_t *);
static int setMaxSize(Disk_t * diskPtr, int part, Widget sizeField);
static int pfgSetAttribute(Widget, int, int, TypeMenuButtonId);
static void dismissPartTypeOptionMenus(void);
static void set_fdisk_start_cylinders(void);
static void align_column_labels(void);

/*
 * contains the option menus asscociated with the partition type for each
 * partition
 */
static Widget OptionMenu[FD_NUMPART];
/* contains Widget id's of each of the solaris option buttons */
static Widget SolarWidget[FD_NUMPART];
static Widget solarcust_dialog;

static Widget sizes[FD_NUMPART];
static Widget cylinders[FD_NUMPART];

static Disk_t *DiskPtr;

static WidgetList widget_list;
static WidgetList fdisk_list[FD_NUMPART];

void
pfgCreateSolarCust(Widget parent, Disk_t *diskPtr)
{
	int err;

	set_units(D_MBYTE);
	DiskPtr = diskPtr;
	err = select_disk(diskPtr, NULL);
	if (err != D_OK) {
		pfgDiskError(parent, NULL, err);
		return;
	}
	solarcust_dialog = tu_solarcust_dialog_widget("solarcust_dialog",
		parent, &widget_list);

	XmAddWMProtocolCallback(pfgShell(solarcust_dialog), pfgWMDeleteAtom,
		(XtCallbackProc) pfgExit, NULL);

	XtVaSetValues(pfgShell(solarcust_dialog),
		XmNtitle, TITLE_CUSTOMSOLARIS,
		XmNdeleteResponse, XmDO_NOTHING,
		NULL);
	xm_SetNoResize(pfgTopLevel, solarcust_dialog);

	createPartRow(diskPtr);

	pfgSetWidgetString(widget_list, "panelhelpText", MSG_CUSTOMSOLARIS);

	pfgSetWidgetString(widget_list, "allocatedLabel", PFG_SC_ALLOC);
	pfgSetWidgetString(widget_list, "freeLabel", PFG_SC_FREE);
	pfgSetWidgetString(widget_list, "capacityLabel", PFG_SC_CAPACITY);

	/*
	 * set the rounding error values to blank unless the rounding error
	 * is greater than zero, setting these is done in calculateTotals()
	 */
	pfgSetWidgetString(widget_list, "roundingLabel", " ");
	pfgSetWidgetString(widget_list, "roundingValue", " ");

	pfgSetWidgetString(widget_list, "okButton", PFG_OKAY);
	pfgSetWidgetString(widget_list, "cancelButton", PFG_CANCEL);
	pfgSetWidgetString(widget_list, "helpButton", PFG_HELP);

	pfgSetWidgetString(widget_list, "fdiskPartitionName", PFG_SC_PARTNAME);
	pfgSetWidgetString(widget_list, "fdiskPartitionSize", PFG_SC_PARTSIZE);
	pfgSetWidgetString(widget_list, "fdiskCylinder", PFG_SC_PARTCYL);

	XtAddCallback(pfgGetNamedWidget(widget_list, "okButton"),
		XmNactivateCallback, solarcustOkCB, diskPtr);
	XtAddCallback(pfgGetNamedWidget(widget_list, "cancelButton"),
		XmNactivateCallback, solarcustCancelCB, diskPtr);

	/* calculate disk totals for free, used and capacity space */
	calculateTotals(diskPtr);

	align_column_labels();

	XtManageChild(solarcust_dialog);

	(void) XmProcessTraversal(pfgGetNamedWidget(widget_list, "okButton"),
		XmTRAVERSE_CURRENT);
}


void
createPartRow(Disk_t *diskPtr)
{
	int	i, j;
	Widget	fdisk_entry;
	Widget	subMenu, sizeField, unusedButton, dosButton, otherButton;
	char	start_cylinder[10];
	char	partChar[8];
	char	size[10];
	int	initial;
	Dimension	option_width;
	Dimension	max_width;

	/* initialize (clear) the option menu upon entry */
	for (i = 0; i < FD_NUMPART; i++)
		OptionMenu[i] = NULL;

	for (i = 0; i < FD_NUMPART; i++) {
		fdisk_entry = tu_fdisk_entry_widget("fdisk_entry",
			pfgGetNamedWidget(widget_list, "fdiskRowColumn"),
			&fdisk_list[i]);

		/* determine which type is currently selected */
		initial = (int) getCurrentType(diskPtr, i + 1);

		/*
		 * this algorithm assumes there are only 4 partitions if
		 * there are more then the vertical positioning will need to
		 * change
		 */
		(void) sprintf(partChar, "%d", i + 1);

		OptionMenu[i] =
			pfgGetNamedWidget(fdisk_list[i], "fdiskOptionMenu");

		if (i == 0) {
			XtVaGetValues(OptionMenu[i],
				XmNwidth, &option_width,
				NULL);
			max_width = option_width;
		}

		if (i > 0) {
			XtVaGetValues(OptionMenu[i],
				XmNwidth, &option_width,
				NULL);
			if (max_width <= option_width) max_width = option_width;
		}

		for (j = 0; j < FD_NUMPART; j++) {
			if (OptionMenu[j] != NULL) {
				XtVaSetValues(OptionMenu[j],
					XmNwidth, max_width,
					NULL);
			}
		}

		pfgSetWidgetString(fdisk_list[i], "fdiskOptionMenu", partChar);

		SolarWidget[i] =
			pfgGetNamedWidget(fdisk_list[i], "solarButton");
		dosButton = pfgGetNamedWidget(fdisk_list[i], "dosButton");
		unusedButton = pfgGetNamedWidget(fdisk_list[i], "unusedButton");
		otherButton = pfgGetNamedWidget(fdisk_list[i], "otherButton");

		pfgSetWidgetString(fdisk_list[i],
			"solarButton", PFG_SC_SOLARIS);
		XtAddCallback(SolarWidget[i], XmNactivateCallback,
			solarcustTypeCB, SolarButton);
		pfgSetWidgetString(fdisk_list[i], "dosButton", PFG_SC_DOS);
		XtAddCallback(dosButton, XmNactivateCallback,
			solarcustTypeCB, DOSHugeButton);
		pfgSetWidgetString(fdisk_list[i],
			"unusedButton", PFG_SC_UNUSED);
		XtAddCallback(unusedButton, XmNactivateCallback,
			solarcustTypeCB, UnusedButton);

		switch (initial) {
			case SolarButton:
				XtVaSetValues(OptionMenu[i],
					XmNmenuHistory, SolarWidget[i],
					NULL);
				XtUnmanageChild(otherButton);
				otherButton = NULL;
				break;
			case DOSHugeButton:
				XtVaSetValues(OptionMenu[i],
					XmNmenuHistory, dosButton,
					NULL);
				XtUnmanageChild(otherButton);
				otherButton = NULL;
				break;
			case UnusedButton:
				XtVaSetValues(OptionMenu[i],
					XmNmenuHistory, unusedButton,
					NULL);
				XtUnmanageChild(otherButton);
				otherButton = NULL;
				break;
			case OtherOSButton:
				pfgSetWidgetString(
					fdisk_list[i], "otherButton",
					part_id(diskPtr, i + 1) == EXTDOS ?
					PFG_SC_EDOS : PFG_SC_OTHER);
				XtVaSetValues(OptionMenu[i],
					XmNmenuHistory, otherButton,
					NULL);
				XtAddCallback(otherButton, XmNactivateCallback,
					solarcustTypeCB, OtherOSButton);
				break;
		}

		XtVaGetValues(OptionMenu[i],
			XmNsubMenuId, &subMenu,
			NULL);
		XtVaSetValues(subMenu,
			XmNuserData, i + 1,
			NULL);

		set_units(D_MBYTE);

		sizeField = pfgGetNamedWidget(fdisk_list[i], "fdiskSizeText");
		sizes[i] = sizeField;
		(void) sprintf(size, "%d", blocks2size(diskPtr,
			part_size(diskPtr, i + 1), ROUNDDOWN));
		XtVaSetValues(sizeField,
			XmNuserData, i + 1,
			NULL);

		XmTextFieldSetString(sizeField, size);

		if (unusedButton != NULL) {
			XtVaSetValues(unusedButton,
				XmNuserData, sizeField,
				NULL);
		}
		if (dosButton != NULL) {
			XtVaSetValues(dosButton,
				XmNuserData, sizeField,
				NULL);
		}
		if (otherButton != NULL) {
			XtVaSetValues(otherButton,
				XmNuserData, sizeField,
				NULL);
		}
		if (SolarWidget[i] != NULL) {
			XtVaSetValues(SolarWidget[i],
				XmNuserData, sizeField,
				NULL);
		}
		XtAddCallback(sizeField, XmNmodifyVerifyCallback,
			sizePartVerifyCB, diskPtr);
		XtAddCallback(sizeField, XmNlosingFocusCallback,
			sizePartLosingFocus, diskPtr);
		XtAddCallback(sizeField, XmNactivateCallback,
			sizePartActivateCB, diskPtr);

		/*
		 * set the starting cylinder value, note that HBA geometry is
		 * used to determine the starting cylinder, note that i + 1
		 * is the partition number
		 */
		(void) sprintf(start_cylinder, "%d",
			((part_startsect(diskPtr, i + 1) +
			disk_geom_hbacyl(diskPtr) - 1) /
			disk_geom_hbacyl(diskPtr)));
		pfgSetWidgetString(fdisk_list[i], "fdiskStartCylinder",
							start_cylinder);
		cylinders[i] = pfgGetNamedWidget(fdisk_list[i],
						"fdiskStartCylinder");

		XtManageChild(fdisk_entry);
	}

	setSolarSensitivity(diskPtr);
	setMenuSensitivity(diskPtr);
}


/*
 * This function sets the solaris button for the option menus to the
 * appropriate sensitivity.  Only one partition can have a type of solaris
 * at any given time.
 */
void
setSolarSensitivity(Disk_t *diskPtr)
{
	int i, part;

	if ((part = get_solaris_part(diskPtr, CFG_CURRENT)) != 0) {
		for (i = 0; i < FD_NUMPART; i++) {
			if ((i + 1) != part) {
				if (SolarWidget[i] != NULL) {
					XtSetSensitive(SolarWidget[i], False);
				}
			} else {
				if (SolarWidget[i] != NULL) {
					XtSetSensitive(SolarWidget[i], True);
				}
			}
		}
	} else {
		for (i = 0; i < FD_NUMPART; i++) {
			if (SolarWidget[i] != NULL) {
				XtSetSensitive(SolarWidget[i], True);
			}
		}
	}
}


void
setMenuSensitivity(Disk_t *diskPtr)
{
	int i, size;

	for (i = 0; i < FD_NUMPART; i++) {
		if (!(size = max_size_part_hole(diskPtr, i + 1))) {
			XtSetSensitive(OptionMenu[i], False);
		} else {
			XtSetSensitive(OptionMenu[i], True);
		}
		if (debug)
			(void) printf("max_size_part_hole size for %d = %d\n",
				i + 1, size);
	}
}


/* ARGSUSED */
void
solarcustOkCB(Widget w, XtPointer diskPtr, XtPointer callD)
{
	int i, err;
	/* LINTED [pointer cast] */
	Disk_t *dp = (Disk_t *) diskPtr;
	char buf[500];
	int  part_num, total, correct;

	err = validate_fdisk(dp);
	if (err != D_OK && err != D_OUTOFREACH) {
		pfgDiskError(solarcust_dialog, "validate_fdisk", err);
		return;
	}

	if (err == D_OUTOFREACH) {
		part_num = get_solaris_part(dp, CFG_CURRENT);
		if (part_num != 0) {
			total = slice_size(dp, BOOT_SLICE) +
				slice_size(dp, ALT_SLICE) +
				get_default_fs_size(ROOT, NULL, DONTROLLUP);
			correct = part_startsect(dp, part_num) -
				(1023 * disk_geom_hbacyl(dp)) + total;
			correct = (correct + 2047) / 2048;
			(void) sprintf(buf, PFG_ER_OUTREACH, correct);
			pfAppWarn(pfErOUTREACH, buf);
		}
	}

	err = commit_disk_config(dp);
	if (err != D_OK) {
		pfgDiskError(solarcust_dialog, NULL, err);
		return;
	} else {
		/* if no solaris partition remove from use disk list */
		if (get_solaris_part(dp, CFG_CURRENT) == 0) {
			/* move disk from use list */
			deselect_disk(dp, NULL);
			moveDisk(dp, False);
		} else {
			moveDisk(dp, True);
			moveDisk(dp, False);
		}

		pfgUnbusy(pfgShell(XtParent(pfgShell(w))));

		XtUnmanageChild(solarcust_dialog);
		XtDestroyWidget(solarcust_dialog);
	}

	for (i = 0; i < FD_NUMPART; i++)
		free(fdisk_list[i]);
	free(widget_list);
}


/* ARGSUSED */
void
solarcustCancelCB(Widget w, XtPointer diskPtr, XtPointer callD)
{
	int i, err;

	/* LINTED [pointer cast] */
	err = restore_disk((Disk_t *) diskPtr, CFG_COMMIT);
	if (err != D_OK) {
		pfgDiskError(solarcust_dialog, NULL, err);
	} else {

		pfgUnbusy(pfgShell(XtParent(pfgShell(w))));

		XtUnmanageChild(solarcust_dialog);
		XtDestroyWidget(solarcust_dialog);
	}

	for (i = 0; i < FD_NUMPART; i++)
		free(fdisk_list[i]);
	free(widget_list);
}


/* ARGSUSED */
void
sizePartVerifyCB(Widget w, XtPointer diskPtr, XtPointer callD)
{
	int part, i;

	XmTextVerifyCallbackStruct *cbs =
		/* LINTED [pointer cast] */
		(XmTextVerifyCallbackStruct *) callD;

	XtVaGetValues(w,
		XmNuserData, &part,
		NULL);


	if (part) {
		if (XtIsSensitive(OptionMenu[part - 1]) == False ||
			/* LINTED [pointer cast] */
			getCurrentType((Disk_t *) diskPtr, part) ==
			OtherOSButton) {
			cbs->doit = False;
		} else {
			for (i = 0; i < cbs->text->length; i++) {
				if (!isdigit(cbs->text->ptr[i])) {
					cbs->doit = False;
					break;
				}
			}
		}
	}
}

/* ARGSUSED */
void
sizePartLosingFocus(Widget w, XtPointer diskPtr, XtPointer callD)
{

	/* LINTED [pointer cast] */
	(void) sizePartChanged(w, (Disk_t *) diskPtr);
}

/* ARGSUSED */
void
sizePartActivateCB(Widget w, XtPointer diskPtr, XtPointer callD)
{

	XtRemoveCallback(w, XmNlosingFocusCallback, sizePartLosingFocus,
		diskPtr);

	/* LINTED [pointer cast] */
	if (sizePartChanged(w, (Disk_t *) diskPtr) == D_OK) {
		XmProcessTraversal(w, XmTRAVERSE_DOWN);

	}

	XtAddCallback(w, XmNlosingFocusCallback, sizePartLosingFocus, diskPtr);
}

static int
sizePartChanged(Widget w, Disk_t * diskPtr)
{
	int part;
	char *string;
	int err = D_OK;
	int value;
	char size[32];

	XtVaGetValues(w,
		XmNuserData, &part,
		NULL);

	string = XmTextFieldGetString(w);

	if (string[0] == '\0') {
		value = 0;
	} else {
		value = atoi(string);
	}


	if (value !=
		blocks2size(diskPtr, part_size(diskPtr, part), ROUNDDOWN)) {
		/*
		 * if size entered for unused partition set the attribute to
		 * solaris if no solaris part. else set to Dos Huge. Set the
		 * option menu to display the new attribute
		 */
		if ((part_size(diskPtr, part) == 0) &&
			(part_id(diskPtr, part) == UNUSED)) {
			if (get_solaris_part(diskPtr, CFG_CURRENT) == 0) {
				err = set_part_attr(DiskPtr, part, SUNIXOS,
					GEOM_IGNORE);
				if (err != D_OK) {
					dismissPartTypeOptionMenus();
					(void) sprintf(size, "%d",
						blocks2size(diskPtr,
						part_size(diskPtr, part),
						ROUNDDOWN));
					XmTextFieldSetString(w, size);
					pfgDiskError(solarcust_dialog,
						NULL, err);
					return (False);
				}
				XtVaSetValues(OptionMenu[part - 1],
					XmNmenuHistory, SolarWidget[part - 1],
					NULL);
			} else {
				if (pfgSetAttribute(w, part, DOSHUGE,
					DOSHugeButton) == False) {
					return (False);
				}
			}

		}
		err = set_part_geom(diskPtr, part, GEOM_IGNORE,
			size2blocks(diskPtr, value));
		if (err == D_PRESERVED) {
			dismissPartTypeOptionMenus();
			if (pfgAppQuery(solarcust_dialog, PFG_FDISKPRES)) {
				(void) set_part_preserve(diskPtr, part,
					PRES_NO);
				err = set_part_geom(diskPtr, part, GEOM_IGNORE,
					size2blocks(diskPtr, value));
			}
		}
		if (err != D_OK) {
			(void) sprintf(size, "%d", blocks2size(diskPtr,
				part_size(diskPtr, part), ROUNDDOWN));
			if (atoi(size) == 0) {
				(void) pfgSetAttribute(w, part, UNUSED,
					UnusedButton);
			}
			XmTextFieldSetString(w, size);
			if (err != D_PRESERVED) {
				dismissPartTypeOptionMenus();
				pfgDiskError(solarcust_dialog, "set_part_geom",
					err);
			}
		} else {
			if (!value) {
				/*
				 * set attribute to unused if size if 0
				 */
				if (pfgSetAttribute(w, part, UNUSED,
					UnusedButton) == False) {
					return (False);
				}
			}
		}
		setMenuSensitivity(diskPtr);
		calculateTotals(diskPtr);
	}
	return (err);
}

/*
 * function to set the partition attribute and to update the
 * option menu to correctly display the new attribute
 */
int
pfgSetAttribute(Widget sizeField, int part, int attrib,
	TypeMenuButtonId buttonNum)
{
	Widget button = NULL;
	char size[32];
	int err;

	err = set_part_attr(DiskPtr, part, attrib, GEOM_IGNORE);
	if (err != D_OK) {
		dismissPartTypeOptionMenus();
		(void) sprintf(size, "%d", blocks2size(DiskPtr,
			part_size(DiskPtr, part), ROUNDDOWN));
		XmTextFieldSetString(sizeField, size);
		pfgDiskError(solarcust_dialog, NULL, err);
		return (False);
	}

	switch (buttonNum) {
		case SolarButton:
			button = pfgGetNamedWidget(fdisk_list[part - 1],
				"solarButton");
			break;
		case DOSHugeButton:
			button = pfgGetNamedWidget(fdisk_list[part - 1],
				"dosButton");
			break;
		case UnusedButton:
			button = pfgGetNamedWidget(fdisk_list[part - 1],
				"unusedButton");
			break;
		case OtherOSButton:
			button = pfgGetNamedWidget(fdisk_list[part - 1],
				"otherButton");
			break;
	}

	XtVaSetValues(OptionMenu[part - 1],
		XmNmenuHistory, button,
		NULL);

	return (True);
}

int
pfgResetAttributeMenu(Disk_t *dp, int part)
{
	int buttonNum;
	Widget button = NULL;

	buttonNum = getCurrentType(dp, part);

	switch (buttonNum) {
		case SolarButton:
			button = pfgGetNamedWidget(fdisk_list[part - 1],
				"solarButton");
			break;
		case DOSHugeButton:
			button = pfgGetNamedWidget(fdisk_list[part - 1],
				"dosButton");
			break;
		case UnusedButton:
			button = pfgGetNamedWidget(fdisk_list[part - 1],
				"unusedButton");
			break;
		case OtherOSButton:
			button = pfgGetNamedWidget(fdisk_list[part - 1],
				"otherButton");
			break;
	}

	XtVaSetValues(OptionMenu[part - 1],
		XmNmenuHistory, button,
		NULL);

	return (True);
}

void
calculateTotals(Disk_t * diskPtr)
{
	int	free, allocated, capacity, rounding_error;
	char	sizeBuf[80], unitString[8], Buf[80];
	int	units;

	/* set label for units being displayed */
	units = get_units();
	switch (units) {
	case D_MBYTE:
		(void) strcpy(unitString, PFG_MBYTES);
		break;
	case D_CYLS:
		(void) strcpy(unitString, PFG_SC_CYLS);
		break;
	default:
		(void) strcpy(unitString, PFG_MBYTES);
		break;
	}

	/* get the amount of free disk space in blocks */
	free = fdisk_space_avail(diskPtr);
	/* get the disk capacity in blocks */
	capacity = usable_disk_blks(diskPtr);
	/* calculate the total allocated disk blocks */
	allocated = capacity - free;

	/*
	 * convert the number of allocated blocks to MB,
	 * then set the widget label
	 */
	allocated = blocks2size(diskPtr, allocated, ROUNDDOWN);
	(void) sprintf(sizeBuf, "%6d %s", allocated, unitString);
	pfgSetWidgetString(widget_list, "allocatedValue", sizeBuf);

	/* convert the number of free blocks to MB, then set the widget label */
	free = blocks2size(diskPtr, free, ROUNDDOWN);
	(void) sprintf(sizeBuf, "%6d %s", free, unitString);
	pfgSetWidgetString(widget_list, "freeValue", sizeBuf);

	/* convert the capacity blocks to MB, then set the widget label */
	capacity = blocks2size(diskPtr, capacity, ROUNDDOWN);
	(void) sprintf(sizeBuf, "%6d %s", capacity, unitString);
	pfgSetWidgetString(widget_list, "capacityValue", sizeBuf);

	/*
	 * there is a potential loss of space due to conversion, rounding,
	 * or truncation, in this case allocated + free != capacity and
	 * we should display the rounding error
	 */

	/* calculate the rounding error */
	rounding_error = (int) (capacity - free - allocated);

	/*
	 * if the rounding error is not 0, then display the value
	 * on the screen
	 */
	if (rounding_error != 0) {
		if (debug)
			(void) printf("there is a rounding error\n");
		(void) sprintf(Buf, "%6d %s", rounding_error, unitString);
		pfgSetWidgetString(widget_list, "roundingValue", Buf);
		pfgSetWidgetString(widget_list, "roundingLabel",
			PFG_SC_RNDERROR);
	}

	/* set the fdisk start cylinders when the sizes have changed */
	set_fdisk_start_cylinders();
}

/* ARGSUSED */
void
solarcustTypeCB(Widget w, XtPointer buttonNum, XtPointer callD)
{
	int part, err;
	char value[32];
	Widget sizeField;
	TypeMenuButtonId oldType;

	XtVaGetValues(XtParent(w),
		XmNuserData, &part,
		NULL);

	set_fdisk_start_cylinders();

	if (part) {
		XtVaGetValues(w,
			XmNuserData, (XtPointer) & sizeField,
			NULL);
		oldType = getCurrentType(DiskPtr, part);
		switch ((int) buttonNum) {
		case SolarButton:
			err = set_part_attr(DiskPtr, part, SUNIXOS,
				GEOM_IGNORE);
			if (err == D_PRESERVED) {
				if (pfgAppQuery(solarcust_dialog,
						PFG_FDISKPRES)) {
					set_part_preserve(DiskPtr, part,
						PRES_NO);
					err = set_part_attr(DiskPtr, part,
						SUNIXOS, GEOM_IGNORE);
				} else {
					(void) pfgResetAttributeMenu(DiskPtr,
						part);
					break;
				}
			}
			if (err == D_OK) {
				(void) setMaxSize(DiskPtr, part, sizeField);
			}
			break;
		case DOSHugeButton:
			err = set_part_attr(DiskPtr, part, DOSHUGE,
				GEOM_IGNORE);
			if (err == D_PRESERVED) {
				if (pfgAppQuery(solarcust_dialog,
					PFG_FDISKPRES)) {
					set_part_preserve(DiskPtr, part,
						PRES_NO);
					err = set_part_attr(DiskPtr, part,
						DOSHUGE, GEOM_IGNORE);
				} else {
					(void) pfgResetAttributeMenu(DiskPtr,
						part);
					break;
				}
			}
			if (err == D_OK) {
				(void) setMaxSize(DiskPtr, part, sizeField);
			}
			break;
		case UnusedButton:
			err = set_part_attr(DiskPtr, part, UNUSED,
				GEOM_IGNORE);
			if (err == D_PRESERVED) {
				if (pfgAppQuery(solarcust_dialog,
					PFG_FDISKPRES)) {
					set_part_preserve(DiskPtr, part,
						PRES_NO);
					err = set_part_attr(DiskPtr, part,
						UNUSED, GEOM_IGNORE);
				} else {
					(void) pfgResetAttributeMenu(DiskPtr,
						part);
					break;
				}
			}
			if (err == D_OK || err == D_PRESERVED) {
				calculateTotals(DiskPtr);
				XtVaGetValues(w,
					XmNuserData, (XtPointer) & sizeField,
					NULL);
				if (sizeField != NULL) {
					(void) sprintf(value, "%d",
						sectors_to_mb(part_size(DiskPtr,
						part)));
					XmTextFieldSetString(sizeField, value);
				}
			}
			break;
		}

		set_fdisk_start_cylinders();

		if (err != D_OK && err != D_PRESERVED) {
			pfgDiskError(solarcust_dialog, "set_part_attr", err);
			(void) pfgResetAttributeMenu(DiskPtr, part);
			return;
		}
		calculateTotals(DiskPtr);
		setSolarSensitivity(DiskPtr);
		setMenuSensitivity(DiskPtr);

		/*
		 * if button type is otheros then
		 * unmanage the button, this is necessary
		 * because we don't want the user to be able
		 * to create otheros types.
		 */
		if (oldType == OtherOSButton && err != D_PRESERVED) {
			XtUnmanageChild(pfgGetNamedWidget(fdisk_list[part],
				"otherButton"));
		}
	}
}

int
setMaxSize(Disk_t * diskPtr, int part, Widget sizeField)
{
	int size;
	char string[32];
	int err;

	if (!part_size(DiskPtr, part)) {
		if (sizeField != NULL) {
			if ((size = max_size_part_hole(DiskPtr, part))) {
				err = set_part_geom(DiskPtr,
					part, GEOM_IGNORE, size);
				if (err != D_OK) {
					pfgDiskError(solarcust_dialog,
						NULL, err);
					return (False);
				}
				(void) sprintf(string, "%d",
					blocks2size(diskPtr, size, ROUNDDOWN));
				XmTextFieldSetString(sizeField, string);
			} else {
				pfgWarning(solarcust_dialog, pfErATTR);
				return (False);
			}
		}
		return (True);
	}
	return (False);
}
/*
 * Return the option button number corresponding to the current type set for
 * the fdisk partition.  The button number is used to set the default button
 * for the option menu
 */
TypeMenuButtonId
getCurrentType(Disk_t * diskPtr, int partition)
{
	TypeMenuButtonId button;

	switch (part_id(diskPtr, partition)) {
	case SUNIXOS:
		button = SolarButton;
		break;

	case DOSOS12:		/* (void) strcpy(buf, "DOSOS12") */
	case DOSOS16:		/* (void) strcpy(buf, "DOSOS16"); */
	case DOSHUGE:
		button = DOSHugeButton;
		break;

	case OTHEROS:
		button = OtherOSButton;
		break;

	case UNUSED:
		button = UnusedButton;
		break;

	case EXTDOS:		/* (void) strcpy(buf, "EXTDOS"); */
	case PCIXOS:		/* (void) strcpy(buf, "PCIXOS"); */
	case DOSDATA:		/* (void) strcpy(buf, "DOSDATA"); */
	case UNIXOS:		/* (void) strcpy(buf, "UNIXOS"); */
	case MAXDOS:		/* (void) strcpy(buf, "MAXDOS"); */
	default:
		button = OtherOSButton;
		break;
	}
	return (button);
}

static void
dismissPartTypeOptionMenus(void)
{
	Widget subMenu;
	int menu;

	/*
	 * unmanage the submenus of all of the partition type option
	 * menus. This is done because this function is called during
	 * a losingFocus event from the partition size textbox. The
	 * server will hang without this.
	 */
	for (menu = 0; menu < FD_NUMPART; menu++) {
		XtVaGetValues(OptionMenu[menu],
			XmNsubMenuId, &subMenu,
			NULL);
		XtUnmanageChild(subMenu);
	}
}


static void
set_fdisk_start_cylinders(void)
{
	int	part_num; /* the fdisk partition number, 1 - 4 */
	int	i;
	char	start_cyl[10];

	for (i = 0; i < FD_NUMPART; i++) {
		/*
		 * the fdisk_list array is zero based from 0 to FD_NUMPART
		 * but the partition numbers are 1 (one) based so we adjust
		 * for the difference here
		 */
		part_num = i + 1;
		(void) sprintf(start_cyl, "%d",
			((part_startsect(DiskPtr, part_num) +
			disk_geom_hbacyl(DiskPtr) - 1) /
			disk_geom_hbacyl(DiskPtr)));
		if (debug)
			(void) printf("start cyl for partition %d is %s\n",
				i, start_cyl);
		pfgSetWidgetString(fdisk_list[i],
				"fdiskStartCylinder", start_cyl);
	}
}

static void
align_column_labels(void)
{
	int	i, j;
	Widget	temp_widget;
	Dimension	widget_width, max_width, temp_width;
	Dimension	margin_width, spacing;
	Widget		submenu_widget;
	Dimension	option_width;

	temp_widget = pfgGetNamedWidget(widget_list, "fdiskPartitionName");
	XtVaGetValues(temp_widget,
		XmNwidth, &widget_width,
		NULL);
	for (i = 0; i < FD_NUMPART; i++) {
		XtVaGetValues(OptionMenu[i],
			XmNsubMenuId, &submenu_widget,
			XmNwidth, &option_width,
			XmNmarginWidth, &margin_width,
			XmNspacing, &spacing,
			NULL);
		XtVaGetValues(submenu_widget,
			XmNwidth, &temp_width,
			NULL);

		margin_width = (Dimension)(margin_width * 2);

		if ((Dimension)widget_width >
			(Dimension)(temp_width + margin_width
					+ spacing + option_width))
			max_width = widget_width;
		else
			max_width = temp_width + margin_width +
					spacing + option_width;

	}

	XtVaSetValues(temp_widget,
		XmNwidth, max_width,
		XmNleftOffset, margin_width + spacing,
		NULL);

	for (j = 0; j < FD_NUMPART; j++) {
		XtVaGetValues(OptionMenu[j],
			XmNsubMenuId, &submenu_widget,
			NULL);
		XtVaSetValues(submenu_widget,
			XmNwidth, max_width,
			NULL);
	}

	temp_widget = pfgGetNamedWidget(widget_list, "fdiskPartitionSize");
	XtVaGetValues(temp_widget,
		XmNwidth, &widget_width,
		NULL);
	for (i = 0; i < FD_NUMPART; i++) {
		XtVaGetValues(sizes[i],
			XmNwidth, &temp_width,
			NULL);

		if (widget_width > temp_width)
			max_width = widget_width;
		else
			max_width = temp_width;

	}

	XtVaSetValues(temp_widget,
		XmNwidth, max_width,
		XmNleftOffset, margin_width + spacing,
		NULL);

	for (j = 0; j < FD_NUMPART; j++) {
		XtVaSetValues(sizes[j],
			XmNwidth, max_width,
			NULL);
	}

	temp_widget = pfgGetNamedWidget(widget_list, "fdiskCylinder");
	XtVaGetValues(temp_widget,
		XmNwidth, &widget_width,
		NULL);
	for (i = 0; i < FD_NUMPART; i++) {
		XtVaGetValues(cylinders[i],
			XmNwidth, &temp_width,
			NULL);

		if (widget_width > temp_width)
			max_width = widget_width;
		else
			max_width = temp_width;

	}

	XtVaSetValues(temp_widget,
		XmNwidth, max_width,
		XmNleftOffset, margin_width + spacing,
		NULL);

	for (j = 0; j < FD_NUMPART; j++) {
		XtVaSetValues(cylinders[j],
			XmNwidth, max_width,
			NULL);
	}

}
