#ifndef lint
#pragma ident "@(#)pfgdisks.c 1.96 96/09/11 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	pfgdisks.c
 * Group:	installtool
 * Description:
 */

#include "pf.h"
#include "pfg.h"
#include "pfgdisks.h"
#include "cyl.bmp"

#include "pfgDisks_ui.h"

#include <X11/keysymdef.h>

static pfDiskW_t *pfgCreateDiskMatrix(Widget, Widget, Disk_t *);
static void createSliceEntry(Widget, Disk_t *, int, Widget);
static void CellChanged(Widget, XEvent *, String *, Cardinal *);
static int pfgNewAltBootQuery(Disk_t *disk);

/* file globals */
static Defmnt_t **MountList;
/* textfield cell currently slaved to the main text field */
static Widget SlavedCell = NULL;
static Widget disks_dialog;

/* array of widget containing disk information */
static Widget *DiskCells;
static int NumCells = 0;
static Widget mainText;

/*
 * prevCell is used to turn the resource "XmNcursorPositionVisible" to
 * False when focus changes to another text field within the disk
 * editor window. when a new text field gets the focus, this resourse
 * is set to false for the previous text field indicated by this
 * variable. This used to be handled in the losingFocusCallback for the
 * text fields, but this caused problems on the server later down the
 * road (see bug 1200782)
 */
static Widget prevCell = NULL;

Widget
pfgCreateDisks(Widget parent)
{
	Widget DiskRC;
	Widget recText, minText;
	XtActionsRec action[2];
	Disk_t *ptr;
	Dimension height, width;
	Position x, y;
	pfDiskW_t *diskW;
	int dispheight;
	WidgetList widget_list;
	char	bootdiskName[32];
	char	part_char;
	int	dev_index;
	char	bootdev_buf[200];

	SlavedCell = NULL;

	/* save default mount list incase user wishes to cancel changes */
	MountList = get_dfltmnt_list(NULL);
	pfgCommitDisks();

	write_debug(GUI_DEBUG_L1, "Entering pfgCreateDisks");
	NumCells = 0;
	DiskCells = createCellArray(2);
	set_units(D_MBYTE);

	/* add actions to handle arrow key events for disk slices */
	action[0].string = "cellChanged";
	action[0].proc = CellChanged;
	XtAppAddActions(pfgAppContext, action, 1);

	disks_dialog = tu_disks_dialog_widget("disks_dialog",
		parent, &widget_list);

	XmAddWMProtocolCallback(pfgShell(disks_dialog), pfgWMDeleteAtom,
		(XtCallbackProc) pfgExit, NULL);

	XtVaSetValues(pfgShell(disks_dialog),
		XmNtitle, TITLE_CUSTDISKS,
		XmNdeleteResponse, XmDO_NOTHING,
		XmNmappedWhenManaged, False, /* makes for clean window manage */
		NULL);

	mainText = pfgGetNamedWidget(widget_list, "mainText");

	pfgSetWidgetString(widget_list, "recommendedLabel", PFG_DK_RECCOM);
	pfgSetWidgetString(widget_list, "minimumLabel", PFG_DK_MINIMUM);

	recText = pfgGetNamedWidget(widget_list, "recommendedValue");

	minText = pfgGetNamedWidget(widget_list, "minimumValue");
	XtVaSetValues(minText,
		XmNuserData, recText,
		NULL);
	XtVaSetValues(mainText,
		XmNuserData, minText,
		NULL);

	DiskRC = pfgGetNamedWidget(widget_list, "disksRowColumn");
	XtAddCallback(mainText, XmNlosingFocusCallback, mainLosingFocus,
		DiskRC);
	XtAddCallback(mainText, XmNactivateCallback, mainActivateCB,
		DiskRC);

	/*
	 * if a boot device is defined display it for the user,
	 * for reference while manually customizing disks
	 */
	(void) BootobjGetAttribute(CFG_CURRENT,
			BOOTOBJ_DISK, bootdiskName,
			BOOTOBJ_DEVICE_TYPE, &part_char,
			BOOTOBJ_DEVICE, &dev_index,
			NULL);

	if (streq(bootdiskName, ""))
		XtUnmanageChild(
			pfgGetNamedWidget(widget_list, "bootdeviceLabel"));
	else if (!streq(bootdiskName, "") && dev_index == -1) {
		(void) sprintf(bootdev_buf,
			PFG_BOOTDISKLABEL, bootdiskName);
		pfgSetWidgetString(widget_list, "bootdeviceLabel", bootdev_buf);
	} else if (!streq(bootdiskName, "") && dev_index != -1) {
		if (IsIsa("ppc") || IsIsa("i386")) {
			(void) sprintf(bootdev_buf,
				PFG_BOOTDISKLABEL, bootdiskName);
		} else {
			(void) sprintf(bootdev_buf, PFG_BOOTDEVICELABEL,
					bootdiskName, part_char, dev_index);
		}
		pfgSetWidgetString(widget_list, "bootdeviceLabel", bootdev_buf);
	}

	/* create disk matrix for each disk */
	for (ptr = first_disk(); ptr; ptr = next_disk(ptr)) {
		if (disk_selected(ptr)) {
			diskW = pfgCreateDiskMatrix(DiskRC, mainText, ptr);
		}
	}

	pfgSetWidgetString(widget_list, "okButton", PFG_OKAY);
	pfgSetWidgetString(widget_list, "cancelButton", PFG_CANCEL);
	pfgSetWidgetString(widget_list, "helpButton", PFG_HELP);

	XtManageChild(disks_dialog);

	XtVaGetValues(DiskRC, XmNheight, &height, NULL);

	XtVaSetValues(pfgGetNamedWidget(widget_list, "disksScrolledWindow"),
		XmNheight, (pfgLowResolution ? height + 31 : height + 36),
		NULL);

	XtVaGetValues(diskW->frame,
		XmNwidth, &width,
		NULL);

	XtVaGetValues(XtParent(disks_dialog),
		XmNheight, &height,
		NULL);

	XtVaSetValues(XtParent(disks_dialog),
		XmNminHeight, height,
		XmNmaxHeight, height,
		XmNwidth, (width * 2 + 36),
		XmNminWidth, (width * 2 + 36),
		NULL);

	XtVaGetValues(pfgShell(parent),
		XmNx, &x,
		XmNy, &y,
		NULL);
	if (!pfgLowResolution) {
		dispheight = DisplayHeight(XtDisplay(disks_dialog),
			DefaultScreen(XtDisplay(disks_dialog)));

		/* account for screen heights of 768 */
		if (dispheight < (int)y + (int)height)
			XtVaSetValues(pfgShell(disks_dialog),
				XmNx, x + 20,
				XmNy, dispheight - height - 30,
				NULL);
		else
			XtVaSetValues(pfgShell(disks_dialog),
				XmNx, x + 20,
				XmNy, y + 20,
				NULL);
	} else {
		XtVaSetValues(pfgShell(disks_dialog),
			XmNx, x,
			XmNy, 0,
			NULL);
	}

	XtMapWidget(pfgShell(disks_dialog));

	XmProcessTraversal(pfgGetNamedWidget(widget_list, "okButton"),
		XmTRAVERSE_CURRENT);

	return (disks_dialog);
}

static pfDiskW_t *
pfgCreateDiskMatrix(Widget parent, Widget mainText, Disk_t * disk)
{
	pfDiskW_t *ret = (pfDiskW_t *) xmalloc(sizeof (pfDiskW_t));
	char buf1[80];
	int i, overhead_value;
	Pixmap pixmap;
	Pixel fg, bg;
	char *overhead;
	Widget disk_entry;
	WidgetList	disk_list;
	char	overhead_unitString[8];
	Units_t	overhead_units;

	write_debug(GUI_DEBUG_L1, "Entering pfgCreateDiskMatrix");

	disk_entry = tu_disks_entry_widget("disk_entry",
		parent, &disk_list);

	ret->d = disk;

	ret->frame = disk_entry;

	XtVaGetValues(parent,
		XmNforeground, &fg,
		XmNbackground, &bg,
		NULL);

	pixmap = XCreatePixmapFromBitmapData(XtDisplay(parent),
		RootWindowOfScreen(XtScreen(parent)),
		cyl_bits, cyl_width, cyl_height,
		fg, bg, DefaultDepthOfScreen(XtScreen(parent)));

	/* unmanaged for early releases */
	ret->cylinder = pfgGetNamedWidget(disk_list, "cylButton");
	XtVaSetValues(ret->cylinder,
		XmNlabelPixmap, pixmap,
		XmNuserData, disk,
		NULL);

	(void) sprintf(buf1, PFG_DK_DSKMSG,
		disk_name(disk), blocks2size(disk, usable_sdisk_blks(disk),
							ROUNDDOWN));
	pfgSetWidgetString(disk_list, "diskLabel", buf1);

	/* lists */

	XtVaSetValues(pfgGetNamedWidget(disk_list, "diskEntryRowColumn"),
		XmNuserData, ret,
		NULL);

	/* create text field for each slice in the disk */
	for (i = 0; i < LAST_STDSLICE + 1; i++) {
		createSliceEntry(
			pfgGetNamedWidget(disk_list, "diskEntryRowColumn"),
			disk, i, mainText);
	}

	/*
	 * get the system overhead value, if there is system overhead on sparc
	 * systems do not show it, only show for non-sparc systems
	 */
	overhead_value = blocks2size(disk, total_sdisk_blks(disk) -
					usable_sdisk_blks(disk), ROUNDDOWN);
	overhead_units = get_units();
	switch (overhead_units) {
	case D_MBYTE:
		(void) strcpy(overhead_unitString, PFG_MBYTES);
		break;
	case D_CYLS:
		(void) strcpy(overhead_unitString, PFG_DK_CYLS);
		break;
	default:
		(void) strcpy(overhead_unitString, PFG_MBYTES);
		break;
	}

	if (disk_fdisk_req(disk) && overhead_value != 0) {
		overhead = xmalloc(11);
		(void) sprintf(overhead, "%6d %s",
			overhead_value, overhead_unitString);
		pfgSetWidgetString(disk_list, "overheadLabel", PFG_OVERHEAD);
		pfgSetWidgetString(disk_list, "overheadValue", overhead);
		free(overhead);
	} else {
		XtUnmanageChild(pfgGetNamedWidget(disk_list, "overheadLabel"));
		XtUnmanageChild(pfgGetNamedWidget(disk_list, "overheadValue"));
	}

	pfgSetWidgetString(disk_list, "allocatedLabel", PFG_DK_ALLOC);
	pfgSetWidgetString(disk_list, "freeLabel", PFG_DK_FREE);
	pfgSetWidgetString(disk_list, "capacityLabel", PFG_DK_CAPACITY);
	/*
	 * set the rounding error lines to blank, we don't want them
	 * to show unless there IS a rounding error
	 */
	ret->rounding_widget = pfgGetNamedWidget(disk_list, "roundingLabel");
	pfgSetWidgetString(disk_list, "roundingLabel", " ");
	pfgSetWidgetString(disk_list, "roundingValue", " ");

	/* 3 totals at bottom */

	/* ALLOCATED */
	ret->total1 = pfgGetNamedWidget(disk_list, "allocatedValue");

	/* FREE */
	ret->total2 = pfgGetNamedWidget(disk_list, "freeValue");

	/* CAPICITY */
	ret->total3 = pfgGetNamedWidget(disk_list, "capacityValue");

	/* ROUNDING ERROR */
	ret->total4 = pfgGetNamedWidget(disk_list, "roundingValue");

	updateTotals(ret);

	ret->next = NULL;

	XtManageChild(disk_entry);

	free(disk_list);

	return (ret);
}

void
createSliceEntry(Widget parent, Disk_t * disk, int slice, Widget mainText)
{
	char tmp[3];
	Widget mountCell, sizeCell;
	char *mountName, sizeString[32];
	Widget slice_entry;
	WidgetList slice_list;

	slice_entry = tu_disk_slice_entry_widget("slice_entry",
		parent, &slice_list);

	/* retrieve name and size of slice from disk lib */
	mountName = strdup(slice_mntpnt(disk, slice));
	if (!slice_size(disk, slice) && !*slice_mntpnt(disk, slice)) {
		sizeString[0] = '\0';
	} else {
		if (!*slice_mntpnt(disk, slice)) {
			mountName = xstrdup(APP_FS_UNNAMED);
		}
		(void) sprintf(sizeString, "%d",
			blocks2size(disk, slice_size(disk, slice), ROUNDDOWN));
	}

	if (slice == BOOT_SLICE) {
		mountName = xstrdup(PFG_DK_BOOTSLICE);
	}

	(void) sprintf(tmp, "%2d", slice);
	pfgSetWidgetString(slice_list, "sliceLabel", tmp);

	DiskCells[NumCells++] = mountCell =
		pfgGetNamedWidget(slice_list, "mountText");
	XtVaSetValues(mountCell,
		XmNuserData, slice,
		XmNmaxLength, MAXNAMELEN,
		NULL);

	XmTextFieldSetString(mountCell, mountName);

	DiskCells[NumCells++] = sizeCell =
		pfgGetNamedWidget(slice_list, "sizeText");
	XtVaSetValues(sizeCell,
		XmNuserData, slice,
		NULL);

	XmTextFieldSetString(sizeCell, sizeString);

	/*
	 * add callbacks to copy contents of cell to main text field when
	 * cell receives focus
	 */
	XtAddCallback(mountCell, XmNfocusCallback, mountFocusCB, mainText);
	XtAddCallback(sizeCell, XmNfocusCallback, sizeFocusCB, mainText);

	/* add callback to duplicate input in cells into main text field */

	XtAddCallback(mountCell, XmNvalueChangedCallback, cellValueChanged,
		mainText);
	XtAddCallback(sizeCell, XmNvalueChangedCallback, cellValueChanged,
		mainText);

	/*
	 * add callbacks to verify valid input if valid insert into main
	 * textt field
	 */

	XtAddCallback(mountCell, XmNmodifyVerifyCallback, mountVerifyCB,
		mainText);

	if (slice == BOOT_SLICE || slice == ALT_SLICE) {
		XtVaSetValues(sizeCell,
			XmNtraversalOn, False,
			XmNeditable, False,
			NULL);
		XtVaSetValues(mountCell,
			XmNtraversalOn, False,
			XmNeditable, False,
			NULL);
	}

	XtManageChild(slice_entry);

	free(slice_list);
}


/*
 * create an array the size of the maximum number of cells need to contain
 * the disk information
 */
Widget *
createCellArray(int numColumns)
{
	Disk_t *ptr;
	int i = 0;

	/* calculate the number of selected disks */
	for (ptr = first_disk(); ptr; ptr = next_disk(ptr)) {
		if (disk_selected(ptr)) {
			i++;
		}
	}
	return ((Widget *) xmalloc((i * numColumns * (NUMPARTS + 1)) *
		sizeof (Widget)));
}

/*
 * update the display for size and mount point name for each slice
 */
void
updateDiskCells(Disk_t * diskPtr)
{
	pfDiskW_t *pfdisk;
	int i, slice;
	char sizeString[32];
	char *string;

	for (i = 0; i < NumCells; i++) {
		XtVaGetValues(XtParent(XtParent(DiskCells[i])),
			XmNuserData, &pfdisk,
			NULL);
		/*
		 * only update the disk that was edited in the cylinder
		 * editor
		 */
		if (pfdisk->d == diskPtr) {
			XtVaGetValues(DiskCells[i],
				XmNuserData, &slice,
				NULL);
			if (strcmp(XtName(DiskCells[i]), "mountText") == 0) {
				string = XmTextFieldGetString(DiskCells[i]);
				if (strcmp(string, slice_mntpnt(pfdisk->d,
						slice)) != 0) {
					XmTextFieldSetString(DiskCells[i],
						slice_mntpnt(pfdisk->d, slice));
				}
			} else {
				string = XmTextFieldGetString(DiskCells[i]);
				if (atoi(string) != blocks2size(pfdisk->d,
					slice_size(pfdisk->d, slice),
					ROUNDDOWN)) {
					(void) sprintf(sizeString, "%d",
						blocks2size(pfdisk->d,
						slice_size(pfdisk->d,
							slice), ROUNDDOWN));
					XmTextFieldSetString(DiskCells[i],
						sizeString);
				}
			}
			if (slice == 0) {
				/* update disk size info only once */
				updateTotals(pfdisk);
			}
		}
	}
}

void
updateTotals(pfDiskW_t * pfdisk)
{
	int		alloc, free, rounding_error, capacity;
	XmString	xms, xms1;
	char		buf1[80], unitString[8];
	Units_t		units;

	if (pfdisk == NULL)
		return;
	units = get_units();
	switch (units) {
	case D_MBYTE:
		(void) strcpy(unitString, PFG_MBYTES);
		break;
	case D_CYLS:
		(void) strcpy(unitString, PFG_DK_CYLS);
		break;
	default:
		(void) strcpy(unitString, PFG_MBYTES);
		break;
	}

	alloc = blocks2size(pfdisk->d, (usable_sdisk_blks(pfdisk->d)) -
		sdisk_space_avail(pfdisk->d), ROUNDDOWN);
	free = blocks2size(pfdisk->d, sdisk_space_avail(pfdisk->d), ROUNDDOWN);
	(void) sprintf(buf1, "%6d %s", alloc, unitString);
	xms = XmStringCreateLocalized(buf1);
	XtVaSetValues(pfdisk->total1, XmNlabelString, xms, NULL);
	XmStringFree(xms);

	/* FREE */
	(void) sprintf(buf1, "%6d %s", free, unitString);
	xms = XmStringCreateLocalized(buf1);
	XtVaSetValues(pfdisk->total2, XmNlabelString, xms, NULL);
	XmStringFree(xms);

	/* CAPACITY */
	capacity = blocks2size(pfdisk->d, usable_sdisk_blks(pfdisk->d),
		ROUNDDOWN);
	(void) sprintf(buf1, "%6d %s",
		(int) blocks2size(pfdisk->d,
			usable_sdisk_blks(pfdisk->d), ROUNDDOWN),
		unitString);
	xms = XmStringCreateLocalized(buf1);
	XtVaSetValues(pfdisk->total3, XmNlabelString, xms, NULL);
	XmStringFree(xms);

	/*
	 * there is a potential loss of space due to conversion, rounding,
	 * or truncation, in this case allocated + free != capacity and
	 * we should display the rounding error
	 */

	/* ROUNDING ERROR */
	rounding_error = (int) (capacity - free - alloc);
	if (rounding_error != 0) {
		xms1 = XmStringCreateLocalized(PFG_DK_RNDERROR);
		XtVaSetValues(pfdisk->rounding_widget,
			XmNlabelString, xms1,
			NULL);
		(void) sprintf(buf1, "%6d %s", rounding_error, unitString);
		xms = XmStringCreateLocalized(buf1);
		XtVaSetValues(pfdisk->total4, XmNlabelString, xms, NULL);
		XmStringFree(xms);
	}

}

/* ARGSUSED */
void
mountFocusCB(Widget w, XtPointer mainText, XtPointer callD)
{
	XmString xmstr;
	Widget recommText, minText;
	char *string;
	XmTextPosition position;
	int slice, total;
	pfDiskW_t *pfdisk;
	char sizeString[80];

	string = XmTextFieldGetString(w);

	if (prevCell)
		XtVaSetValues(prevCell,
			XmNcursorPositionVisible, False,
			NULL);
	XtVaSetValues(w,
		XmNcursorPositionVisible, True,
		NULL);

	XmTextFieldSetString((Widget) mainText, string);
	position = XmTextFieldGetLastPosition(w);
	XmTextFieldSetInsertionPosition(w, position);

	XtVaGetValues(mainText,
		XmNuserData, &minText,
		NULL);
	XtVaGetValues(w,
		XmNuserData, &slice,
		NULL);
	XtVaGetValues(XtParent(XtParent(w)),
		XmNuserData, &pfdisk,
		NULL);

	if (minText) {
		total = blocks2size(pfdisk->d,
			get_minimum_fs_size(slice_mntpnt(pfdisk->d, slice),
			pfdisk->d, ROLLUP), ROUNDUP);
		(void) sprintf(sizeString, "%d", total);
		write_debug(GUI_DEBUG_L1, "minimum size = %s", sizeString);
		xmstr = XmStringCreateLocalized(sizeString);
		XtVaSetValues(minText, XmNlabelString, xmstr, NULL);
		XmStringFree(xmstr);
		XtVaGetValues(minText,
			XmNuserData, &recommText,
			NULL);
		if (recommText) {
			total = blocks2size(pfdisk->d,
				get_default_fs_size(slice_mntpnt(pfdisk->d,
					slice), pfdisk->d, ROLLUP), ROUNDUP);
			(void) sprintf(sizeString, "%d", total);
			write_debug(GUI_DEBUG_L1,
				"recommended size = %s", sizeString);
			xmstr = XmStringCreateLocalized(sizeString);
			XtVaSetValues(recommText, XmNlabelString, xmstr,
				NULL);
			XmStringFree(xmstr);
		}
	}
}

/* ARGSUSED */
void
sizeFocusCB(Widget w, XtPointer mainText, XtPointer callD)
{
	char sizeString[80];
	Widget recommText, minText;
	XmString xmstr;
	pfDiskW_t *pfdisk;
	char *string;
	XmTextPosition position;
	int total, slice;

	string = XmTextFieldGetString(w);

	if (prevCell)
		XtVaSetValues(prevCell,
			XmNcursorPositionVisible, False,
			NULL);
	XtVaSetValues(w,
		XmNcursorPositionVisible, True,
		NULL);

	XmTextFieldSetString((Widget) mainText, string);
	position = XmTextFieldGetLastPosition(w);
	XmTextFieldSetInsertionPosition(w, position);

	XtVaGetValues(XtParent(XtParent(w)),
		XmNuserData, &pfdisk,
		NULL);

	XtVaGetValues(mainText,
		XmNuserData, &minText,
		NULL);
	XtVaGetValues(w,
		XmNuserData, &slice,
		NULL);
	if (minText) {
		total = blocks2size(pfdisk->d,
			get_minimum_fs_size(slice_mntpnt(pfdisk->d, slice),
			pfdisk->d, ROLLUP), ROUNDUP);
		(void) sprintf(sizeString, "%d", total);
		write_debug(GUI_DEBUG_L1,
			"minimum size = %s", sizeString);
		xmstr = XmStringCreateLocalized(sizeString);
		XtVaSetValues(minText, XmNlabelString, xmstr, NULL);
		XmStringFree(xmstr);
		XtVaGetValues(minText,
			XmNuserData, &recommText,
			NULL);
		if (recommText) {
			total = blocks2size(pfdisk->d,
				get_default_fs_size(slice_mntpnt(pfdisk->d,
					slice), pfdisk->d, ROLLUP), ROUNDUP);
			(void) sprintf(sizeString, "%d", total);
			write_debug(GUI_DEBUG_L1,
				"recommended size = %s", sizeString);
			xmstr = XmStringCreateLocalized(sizeString);
			XtVaSetValues(recommText, XmNlabelString, xmstr,
				NULL);
			XmStringFree(xmstr);
		}
	}
}

/* ARGSUSED */
void
mountLosingFocus(Widget w, XtPointer clientD, XtPointer callD)
{
	(void) modifyMount(w);
}

int
modifyMount(Widget w)
{
	char *string;
	pfDiskW_t *pfdisk;
	int err = D_OK;
	int slice;
	char *oldMount;
	Defmnt_t mountEnt;

	/*
	 * set global so main text area knows where to replicate the input
	 * it receives
	 */
	SlavedCell = w;

	prevCell = w;

	XtVaGetValues(w,
		XmNuserData, &slice,
		NULL);

	XtVaGetValues(XtParent(XtParent(w)),
		XmNuserData, &pfdisk,
		NULL);

	string = XmTextFieldGetString(w);
	if (strcmp(string, APP_FS_UNNAMED) != 0) {
		if (strcmp(string, slice_mntpnt(pfdisk->d, slice)) != 0) {
			oldMount = xstrdup(slice_mntpnt(pfdisk->d, slice));
			if (strcmp(string, "/") == 0) {
				if (pfgIsBootDrive(pfdisk->d) == False) {
					XtRemoveCallback(w,
						XmNlosingFocusCallback,
						mountLosingFocus, NULL);
					if (pfgNewAltBootQuery(pfdisk->d)
						== False) {
						XmTextFieldSetString(w,
							oldMount);

						XtAddCallback(w,
							XmNlosingFocusCallback,
							mountLosingFocus,
							NULL);
						return (D_OK);
					}
					XtAddCallback(w,
						XmNlosingFocusCallback,
						mountLosingFocus,
						NULL);
				}
			}
			err = set_slice_mnt(pfdisk->d, slice, string, NULL);
			if (err != D_OK) {
				pfgDiskError(disks_dialog,
					"set_slice_mnt", err);
				/* reset mount point name on disk */
				XmTextFieldSetString(w,
					slice_mntpnt(pfdisk->d, slice));
			} else {
				/*
				 * changed default mount table entry, this
				 * is required so that min and recommended
				 * sizes are calculated correctly
				 */
				if (get_dfltmnt_ent(&mountEnt,
					oldMount) == D_OK) {
					mountEnt.status = DFLT_IGNORE;
					write_debug(GUI_DEBUG_L1,
						"status is ignored");
					set_dfltmnt_ent(&mountEnt, oldMount);
				}
				if (get_dfltmnt_ent(&mountEnt, string)
					== D_OK) {
					mountEnt.status = DFLT_SELECT;
					write_debug(GUI_DEBUG_L1,
						"status is select");
					set_dfltmnt_ent(&mountEnt, string);
				}
			}
		}
	}
	return (err);
}

/* ARGSUSED */
void
sizeLosingFocus(Widget w, XtPointer clientD, XtPointer callD)
{
	(void) modifySize(w);
}

int
modifySize(Widget w)
{
	int value;
	pfDiskW_t *pfdisk;
	int err = D_OK;
	char *string;
	int slice;
	int valueChanged = False;

	/*
	 * set global so main text area knows where to replicate the input
	 * it receives
	 */
	SlavedCell = w;

	prevCell = w;

	string = XmTextFieldGetString(w);

	if (string[0] == '\0') {
		value = 0;
	} else {
		value = atoi(string);
	}
	XtVaGetValues(w,
		XmNuserData, &slice,
		NULL);

	XtVaGetValues(XtParent(XtParent(w)),
		XmNuserData, &pfdisk,
		NULL);

	if (value != blocks2size(pfdisk->d, slice_size(pfdisk->d, slice),
		ROUNDDOWN)) {
		valueChanged = True;
		value = size2blocks(pfdisk->d, value);
		err = set_slice_geom(pfdisk->d, slice, GEOM_IGNORE, value);
		if (err != D_OK) {
			pfgDiskError(disks_dialog, NULL, err);
			(void) sprintf(string, "%d",
				blocks2size(pfdisk->d, slice_size(pfdisk->d,
					slice), ROUNDDOWN));
			XmTextFieldSetString(w, string);
			return (err);
		}
		if (get_units() == D_CYLS) {
			pfgUpdateCylinders();
		}
	}
	if (valueChanged) {
		updateTotals(pfdisk);
	}
	return (err);
}

/* ARGSUSED */
void
mountActivateCB(Widget w, XtPointer clientD, XtPointer callD)
{
	/*
	 * the process traversal below will cause the losing focus
	 * callback to be called, which will do the verification
	 */
	XmProcessTraversal(w, XmTRAVERSE_DOWN);
}

/* ARGSUSED */
void
sizeActivateCB(Widget w, XtPointer clientD, XtPointer callD)
{
	/*
	 * the process traversal below will cause the losing focus
	 * callback to be called, which will do the verification
	 */
	XmProcessTraversal(w, XmTRAVERSE_DOWN);
}

/* ARGSUSED */
void
cellValueChanged(Widget w, XtPointer mainText, XtPointer callD)
{
	char *string;
	XmTextPosition position;

	string = XmTextFieldGetString(w);

	if (mainText == NULL) {
		/* if mainText NULL then set string in slave cell */
		if (SlavedCell != NULL) {
			XmTextFieldSetString(SlavedCell, string);
			position = XmTextFieldGetInsertionPosition(w);
			XmTextFieldSetInsertionPosition(SlavedCell, position);
		}
	} else {
		XmTextFieldSetString((Widget) mainText, string);
	}
	XtFree(string);
}

/* ARGSUSED */
void
sizeVerifyCB(Widget w, XtPointer clientD, XmTextVerifyCallbackStruct *cbs)
{
	int i;

	for (i = 0; i < cbs->text->length; i++) {
		if (!isdigit(cbs->text->ptr[i])) {
			cbs->doit = False;
			break;
		}
	}
}

/* ARGSUSED */
void
mountVerifyCB(Widget w, XtPointer clientD, XmTextVerifyCallbackStruct *cbs)
{
	int i;

	if (cbs->text->length == 0) {
		return;
	}
	if (cbs->startPos == 0) {
		if (cbs->text->ptr[0] != '/' && cbs->text->ptr[0] != 's' &&
			cbs->text->ptr[0] != 'o' && cbs->text->ptr[0] != 'a') {
			cbs->doit = False;
			return;
		}
	}
	for (i = 0; i < cbs->text->length; i++) {
		if (cbs->text->ptr[i] == ' ') {
			cbs->doit = False;
			break;
		}
	}
}

/* ARGSUSED */
void
startLosingFocus(Widget w, XtPointer cellType, XtPointer callD)
{
	(void) modifyStart(w);
}

/* ARGSUSED */
void
startActivateCB(Widget w, XtPointer clientD, XtPointer callD)
{
	/*
	 * the process traversal below will cause the losing focus
	 * callback to be called, which will do the verification
	 */
	XmProcessTraversal(w, XmTRAVERSE_DOWN);
}

int
modifyStart(Widget w)
{
	int value;
	pfDiskW_t *pfdisk;
	int err = D_OK;
	char *string;
	int slice;
	int valueChanged = False;

	/*
	 * set global so main text area knows where to replicate the input
	 * it receives
	 */
	SlavedCell = w;

	prevCell = w;

	string = XmTextFieldGetString(w);

	if (string[0] == '\0') {
		value = 0;
	} else {
		value = atoi(string);
	}
	XtVaGetValues(w,
		XmNuserData, &slice,
		NULL);

	XtVaGetValues(XtParent(XtParent(w)),
		XmNuserData, &pfdisk,
		NULL);

	if (value != slice_start(pfdisk->d, slice)) {
		valueChanged = True;
		slice_stuck_on(pfdisk->d, slice);
		err = set_slice_geom(pfdisk->d, slice, value, GEOM_IGNORE);
		if (err != D_OK) {
			pfgDiskError(disks_dialog, NULL, err);
			(void) sprintf(string, "%d",
				slice_start(pfdisk->d, slice));
			XmTextFieldSetString(w, string);
			return (err);
		}
		pfgUpdateCylinders();
	}
	if (valueChanged) {
		updateTotals(pfdisk);
	}
	return (err);
}

/* ARGSUSED */
void
mainFocusCB(Widget mainText, XtPointer clientD, XtPointer callD)
{
	int maxLength;
	/*
	 * set callbacks based on whether the slaved cell is for mount point
	 * names or slice sizes.
	 */
	if (SlavedCell == NULL) {
		return;
	}
	XtVaGetValues(SlavedCell,
		XmNmaxLength, &maxLength,
		NULL);
	XtVaSetValues(mainText,
		XmNmaxLength, maxLength,
		XmNcursorPositionVisible, True,
		NULL);
	if (prevCell)
		XtVaSetValues(prevCell,
			XmNcursorPositionVisible, False,
			NULL);

	XtRemoveCallback(SlavedCell, XmNvalueChangedCallback,
		cellValueChanged, mainText);

	XtAddCallback(mainText, XmNvalueChangedCallback, cellValueChanged,
		NULL);
	XtAddCallback(mainText, XmNmotionVerifyCallback, mainMotionCB, NULL);

	if (strcmp(XtName(SlavedCell), "mountText") == 0) {
		write_debug(GUI_DEBUG_L1, "slaved cell is a mount cell");
		XtAddCallback(mainText, XmNmodifyVerifyCallback,
			mountVerifyCB, NULL);
	} else {
		write_debug(GUI_DEBUG_L1,
			"slaved cell is a size cell");
		XtAddCallback(mainText, XmNmodifyVerifyCallback,
			sizeVerifyCB, NULL);
	}
}

/* ARGSUSED */
void
mainLosingFocus(Widget mainText, XtPointer diskRC, XtPointer callD)
{
	if (SlavedCell == NULL) {
		return;
	}

	XtRemoveCallback(mainText, XmNvalueChangedCallback, cellValueChanged,
		NULL);
	XtRemoveCallback(mainText, XmNmotionVerifyCallback, mainMotionCB,
		NULL);

	XtAddCallback(SlavedCell, XmNvalueChangedCallback, cellValueChanged,
		mainText);

	if (strcmp(XtName(SlavedCell), "mountText") == 0) {
		write_debug(GUI_DEBUG_L1,
			"slaved cell is a mount cell");
		XtRemoveCallback(mainText, XmNmodifyVerifyCallback,
			mountVerifyCB,
			NULL);
		(void) modifyMount(SlavedCell);
	} else if (strcmp(XtName(SlavedCell), "sizeText") == 0) {
		write_debug(GUI_DEBUG_L1,
			"slaved cell is a size cell");
		XtRemoveCallback(mainText, XmNmodifyVerifyCallback,
			sizeVerifyCB, NULL);
		(void) modifySize(SlavedCell);
	} else if (strcmp(XtName(SlavedCell), "startText") == 0) {
		XtRemoveCallback(mainText, XmNmodifyVerifyCallback,
			sizeVerifyCB, NULL);
		(void) modifyStart(SlavedCell);
	}
	/* set initial focus to SlavedCell */
	XtVaSetValues(diskRC,
		XmNinitialFocus, SlavedCell,
		NULL);

	prevCell = mainText;
}

/* ARGSUSED */
void
mainMotionCB(Widget mainText, XtPointer clientD,
	XmTextVerifyCallbackStruct *cbs)
{
	if (SlavedCell != NULL) {
		XmTextFieldSetInsertionPosition(SlavedCell, cbs->newInsert);
	}
}

/* ARGSUSED */
void
mainActivateCB(Widget mainText, XtPointer diskRC, XtPointer callD)
{
	/* set initial focus to SlavedCell */
	if (SlavedCell != NULL) {
		XtVaSetValues(diskRC,
			XmNinitialFocus, SlavedCell,
			NULL);
	}
	XmProcessTraversal(mainText, XmTRAVERSE_NEXT_TAB_GROUP);
}

void
pfgDiskError(Widget dialog, char *function, int error)
{

	switch (error) {
		case D_BADDISK:
		if ((function != NULL) && (strcmp("set_dfltmnt_list",
			function) == 0)) {
			pfgWarning(dialog, pfErBADMOUNTLIST);
		} else {
			pfgWarning(dialog, pfErBADDISK);
		}
		break;
	case D_NOTSELECT:
		pfgWarning(dialog, pfErNOTSELECTED);
		break;
	case D_NOSPACE:
		pfgWarning(dialog, pfErNOSPACE);
		break;
	case D_NOFIT:
		pfgWarning(dialog, pfErNOFIT);
		break;
	case D_DUPMNT:
		pfgWarning(dialog, pfErDUPMOUNT);
		break;
	case D_LOCKED:
		pfgWarning(dialog, pfErLOCKED);
		break;
	case D_BOOTFIXED:
		pfgWarning(dialog, pfErBOOTFIXED);
		break;
	case D_IGNORED:
		pfgWarning(dialog, pfErDISKIGNORE);
		break;
	case D_PRESERVED:
		pfgWarning(dialog, pfErMODIFYPRESERVE);
		break;
	case D_CANTPRES:
		pfgWarning(dialog, pfErRENAMEPRESERVE);
		break;
	case D_ZERO:
		if (function == NULL) {
			pfgWarning(dialog, pfErZEROPRES);
		} else if (strcmp(function, "check_disk") == 0) {

			pfgWarning(dialog, pfErZERODISK);
		}
		break;
	case D_OFF:
		pfgWarning(dialog, pfErSLICEOFF);
		break;
	case D_UNUSED:
		pfgWarning(dialog, pfErUNUSEDSPACE);
		break;
	case D_CHANGED:
		pfgWarning(dialog, pfErCHANGED);
		break;
	case D_GEOMCHNG:
		pfgWarning(dialog, pfErGEOMCHANGED);
		break;
	case D_BADARG:
		if (function == NULL) {
			break;
		}
		if (strcmp(function, "set_slice_geom") == 0) {
			pfgWarning(dialog, pfErINVALIDSTART);
		} else if (strcmp(function, "set_slice_mnt") == 0) {
			pfgWarning(dialog, pfErMOUNT);
		}
		break;
	case D_OUTOFREACH:
		if (function == NULL) {
			pfgWarning(dialog, pfErOUTREACH);
		} else {
			pfgWarning(dialog, pfErDISKOUTREACH);
		}
		break;
	case D_BADORDER:
		pfgWarning(dialog, pfErORDER);
		break;
	case D_OVER:
		pfgWarning(dialog, pfErSLICESOVERLAP);
		break;
	case D_BOOTCONFIG:
		pfgWarning(dialog, pfErNOROOT);
		break;
	default:
		write_debug(GUI_DEBUG_L1, "problem with disk");
		pfgWarning(dialog, pfErUNKNOWN);
		break;
	}
}

/* ARGSUSED */
void
disksOkCB(Widget w, XtPointer clientD, XtPointer callD)
{
	int		ret = True;
	int		num_errors;
	Disk_t		*dp;
	Errmsg_t	*error_list;

	write_debug(GUI_DEBUG_L1,
		"Entering disksOkCB - calling updatefilesys");

	/*
	 * validate_disks used to be called here, but since it is an
	 * obsolete function in the disk library it has been removed
	 * and replaced with check_disk to fix bug id 1219123
	 */

	WALK_DISK_LIST(dp) {
		if (disk_not_selected(dp))
			continue;
		num_errors = check_disk(dp);
		if (num_errors > 0) {
			write_debug(GUI_DEBUG_L1,
				"check_disk found an error");
			WALK_LIST(error_list, get_error_list()) {
				if (error_list->code < 0) {
					/*
					 * an error was found
					 */
					pfgDiskError(disks_dialog, "check_disk",
							error_list->code);
					free_error_list();
					return;
				} else if (error_list->code > 0) {
					/*
					 * a warning was found
					 */
					ret = pfgAppQuery(disks_dialog,
						error_list->msg);
					free_error_list();
				}
			}
		}
	}

	if (ret == True) {
		pfgCommitDisks();
		pfgUpdateFilesysSummary();
		XtUnmanageChild(pfgShell(disks_dialog));
		XtDestroyWidget(pfgShell(disks_dialog));

		pfgUnbusy(pfgShell(XtParent(pfgShell(w))));
		prevCell = NULL;
	}
}

/* ARGSUSED */
void
disksCancelCB(Widget w, XtPointer client, XtPointer callD)
{
	write_debug(GUI_DEBUG_L1, "Entering disksCancelCB");

	pfgResetDisks();
	/* reset default mount list to previous settings */
	set_dfltmnt_list(MountList);
	XtUnmanageChild(pfgShell(disks_dialog));
	XtDestroyWidget(pfgShell(disks_dialog));

	pfgUnbusy(pfgShell(XtParent(pfgShell(w))));
	prevCell = NULL;
}

/* launch modal cylinder popup */
/* ARGSUSED */
void
cylOpenCB(Widget w, XtPointer client, XtPointer callD)
{
	struct disk *d;
	Widget parent;

	XtVaGetValues(w, XmNuserData, &d, NULL);

	/* XtParent calls raise us up in the resource name tree */
	parent = pfgShell(w);

	pfgBusy(parent);

	write_debug(GUI_DEBUG_L1, "cylOpenCB: parent =%s=", XtName(parent));

	(void) pfgCreateCylinder(parent, d);
}
/*
 * action callback invoked when users move, from the cell in the disk or
 * cylinder editor, using the arrow keys
 */
void
CellChanged(Widget w, XEvent * event, String * params, Cardinal * numParams)
{
	if (*numParams != 2) {
		return;
	}

	if (event->type == KeyPress || event->type == KeyRelease) {
		if (strcmp(params[1], "left") == 0) {
			XmProcessTraversal(w, XmTRAVERSE_LEFT);
		} else if (strcmp(params[1], "right") == 0) {
			XmProcessTraversal(w, XmTRAVERSE_RIGHT);
		} else if (strcmp(params[1], "up") == 0) {
			XmProcessTraversal(w, XmTRAVERSE_UP);
		} else if (strcmp(params[1], "down") == 0) {
			XmProcessTraversal(w, XmTRAVERSE_DOWN);
		}
	}
}

static int
pfgNewAltBootQuery(Disk_t *disk)
{
	Disk_t *	bootDisk;
	char *		buff;
	int		ret;

	if (DiskobjFindBoot(CFG_CURRENT, &bootDisk) != D_OK ||
			bootDisk == NULL)
		return (True);

	buff = xmalloc(strlen(MSG_NEWALTBOOT_BASE) +
		strlen(MSG_NEWALTBOOT_SPARC) +
		strlen(MSG_NEWALTBOOT_X86) +
		strlen(MSG_NEWALTBOOT_PPC) +
		strlen(disk_name(disk))
		+ strlen(disk_name(bootDisk)) + 100);

	if (IsIsa("sparc")) {
		(void) sprintf(buff, MSG_NEWALTBOOT_BASE,
			disk_name(disk),
			MSG_NEWALTBOOT_SPARC);
	} else if (IsIsa("i386")) {
		(void) sprintf(buff, MSG_NEWALTBOOT_BASE,
			disk_name(disk),
			MSG_NEWALTBOOT_X86);
	} else if (IsIsa("ppc")) {
		(void) sprintf(buff, MSG_NEWALTBOOT_BASE,
			disk_name(disk),
			MSG_NEWALTBOOT_PPC);
	}

	ret = pfgAppQuery(disks_dialog, buff);

	free(buff);

	return (ret);
}
