#ifndef lint
#pragma ident "@(#)pfgcyl.c 1.56 96/09/11 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	pfgcyl.c
 * Group:	installtool
 * Description:
 */

#include "pf.h"
#include "pfg.h"
#include "pfgdisks.h"

#include "pfgCyl_ui.h"

static void cylOkCB(Widget, XtPointer, XtPointer);
static void cylLoadCB(Widget w, XtPointer, XtPointer);
static void cylCancelCB(Widget, XtPointer, XtPointer);

static pfDiskW_t *pfgCreateDiskMatrix(WidgetList, Widget, Widget, Disk_t *);
static void createSliceEntry(Widget, Disk_t *, int, Widget);

/* file globals */
static Widget cyl_dialog;
static Defmnt_t **MountList;
static SliceWidgets *CylinderCells = NULL;

Widget
pfgCreateCylinder(Widget parent, Disk_t *diskPtr)
{
	Widget mainText, recText, minText;
	Widget diskRC;
	WidgetList widget_list;

	if (debug)
		(void) printf("pfgdisks: pfgCreateDisks()\n");

	set_units(D_CYLS);
	/*
	 * create an array of widgets to contain widget id's of the cells
	 * containing disk information
	 */
	CylinderCells = (SliceWidgets *) calloc((size_t) NUMPARTS,
		sizeof (SliceWidgets));
	saveDiskConfig(diskPtr);
	MountList = get_dfltmnt_list(NULL);

	cyl_dialog = tu_cyl_dialog_widget("cyl_dialog",
		parent, &widget_list);

	XmAddWMProtocolCallback(pfgShell(cyl_dialog), pfgWMDeleteAtom,
		(XtCallbackProc) pfgExit, NULL);

	XtVaSetValues(pfgShell(cyl_dialog),
		XmNtitle, TITLE_CYLINDERS,
		XmNdeleteResponse, XmDO_NOTHING,
		NULL);
	xm_SetNoResize(pfgTopLevel, pfgShell(cyl_dialog));

	/* INPUT AREA */
	mainText = pfgGetNamedWidget(widget_list, "mainText");

	/* labels */
	pfgSetWidgetString(widget_list, "recommendedLabel", PFG_CY_RECCOM);
	pfgSetWidgetString(widget_list, "minimumLabel", PFG_CY_MINIMUM);

	recText = pfgGetNamedWidget(widget_list, "recommendedValue");
	minText = pfgGetNamedWidget(widget_list, "minimumValue");

	XtVaSetValues(minText,
		XmNuserData, recText,
		NULL);

	XtVaSetValues(mainText,
		XmNuserData, minText,
		NULL);

	diskRC = pfgGetNamedWidget(widget_list, "cylEntryRowColumn");

	XtAddCallback(mainText, XmNlosingFocusCallback, mainLosingFocus,
		diskRC);
	XtAddCallback(mainText, XmNactivateCallback, mainActivateCB,
		diskRC);

	(void) pfgCreateDiskMatrix(widget_list, diskRC, mainText, diskPtr);

	pfgSetWidgetString(widget_list, "okButton", PFG_OKAY);
	pfgSetWidgetString(widget_list, "loadButton", PFG_CY_LOAD);
	pfgSetWidgetString(widget_list, "cancelButton", PFG_CANCEL);
	pfgSetWidgetString(widget_list, "helpButton", PFG_HELP);

	XtAddCallback(pfgGetNamedWidget(widget_list, "okButton"),
		XmNactivateCallback, cylOkCB, (XtPointer) diskPtr);

	XtAddCallback(pfgGetNamedWidget(widget_list, "loadButton"),
		XmNactivateCallback, cylLoadCB, (XtPointer) diskPtr);

	XtAddCallback(pfgGetNamedWidget(widget_list, "cancelButton"),
		XmNactivateCallback, cylCancelCB, (XtPointer) diskPtr);

	XtManageChild(cyl_dialog);

	XmProcessTraversal(pfgGetNamedWidget(widget_list, "okButton"),
		XmTRAVERSE_CURRENT);

	if (pfgLowResolution)
		XtVaSetValues(XtParent(cyl_dialog), XmNy, 0, NULL);

	free(widget_list);

	return (cyl_dialog);
}

static pfDiskW_t *
pfgCreateDiskMatrix(WidgetList widget_list, Widget parent, Widget mainText,
	Disk_t *disk)
{
	pfDiskW_t *ret = (pfDiskW_t *) xmalloc(sizeof (pfDiskW_t));
	char *overhead;
	char buf1[80];
	int i, overhead_value;
	char	overhead_unitString[8];
	Units_t	overhead_units;

	if (debug)
		(void) printf("pfgdisks:create_diskW\n");

	ret->d = disk;

	/*
	 * ***change to subrc parent in the line below, when cylinder
	 * enabled...
	 */
	(void) sprintf(buf1, PFG_CY_CYLS,
		disk_name(disk),
		blocks2size(disk, usable_sdisk_blks(disk), ROUNDUP));
	pfgSetWidgetString(widget_list, "diskLabel", buf1);
	ret->header = pfgGetNamedWidget(widget_list, "diskLabel");

	pfgSetWidgetString(widget_list, "headingLabel", PFG_CY_HEADINGS);

	XtVaSetValues(pfgGetNamedWidget(widget_list, "cylEntryRowColumn"),
		XmNuserData, ret,
		NULL);

	/* create column headers */
	/* create text field for each slice in the disk */
	for (i = 0; i < LAST_STDSLICE + 1; i++) {
		createSliceEntry(parent, disk, i, mainText);
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


	/* Solaris overhead for intel */
	if (disk_fdisk_req(disk) && overhead_value != 0) {
		overhead = xmalloc(11);
		(void) sprintf(overhead, "%6d %s", overhead_value,
						overhead_unitString);
		pfgSetWidgetString(widget_list, "overheadLabel", PFG_OVERHEAD);
		pfgSetWidgetString(widget_list, "overheadValue", overhead);
		free(overhead);
	} else {
		XtUnmanageChild(pfgGetNamedWidget(widget_list,
			"overheadLabel"));
		XtUnmanageChild(pfgGetNamedWidget(widget_list,
			"overheadValue"));
	}

	pfgSetWidgetString(widget_list, "allocatedLabel", PFG_DK_ALLOC);
	pfgSetWidgetString(widget_list, "freeLabel", PFG_DK_FREE);
	pfgSetWidgetString(widget_list, "capacityLabel", PFG_DK_CAPACITY);

	/* 3 totals at bottom */

	/* ALLOCATED */
	ret->total1 = pfgGetNamedWidget(widget_list, "allocatedValue");

	/* FREE */
	ret->total2 = pfgGetNamedWidget(widget_list, "freeValue");

	/* CAPICITY */
	ret->total3 = pfgGetNamedWidget(widget_list, "capacityValue");

	updateTotals(ret);

	ret->next = NULL;

	return (ret);
}

void
createSliceEntry(Widget parent, Disk_t *disk, int slice, Widget mainText)
{
	char tmp[3];
	char *mountName, sizeString[32], startString[32], endString[32];
	int end;
	Widget mountCell, sizeCell, startCell;
	WidgetList slice_list;

	(void) tu_cyl_slice_entry_widget("slice_entry",
		parent, &slice_list);

	/* retrieve name and size of slice from disk lib */
	mountName = strdup(slice_mntpnt(disk, slice));
	if (!slice_size(disk, slice) && !*slice_mntpnt(disk, slice)) {
		sizeString[0] = '\0';
		startString[0] = '\0';
		endString[0] = '\0';
	} else {
		if (!*slice_mntpnt(disk, slice)) {
			mountName = xstrdup(APP_FS_UNNAMED);
		}
		(void) sprintf(sizeString, "%d",
			blocks2size(disk, slice_size(disk, slice), ROUNDUP));
		(void) sprintf(startString, "%d", slice_start(disk, slice));
		end = blocks2size(disk, slice_size(disk, slice), ROUNDUP) +
			slice_start(disk, slice);
		end = (end == 0) ? 0 : end - 1;
		(void) sprintf(endString, "%d", end);
	}

	if (slice == BOOT_SLICE) {
		mountName = xstrdup(PFG_DK_BOOTSLICE);
	}

	(void) sprintf(tmp, "%2d", slice);
	pfgSetWidgetString(slice_list, "sliceLabel", tmp);

	CylinderCells[slice].mountWidget = mountCell =
		pfgGetNamedWidget(slice_list, "mountText");
	XtVaSetValues(mountCell,
		XmNuserData, slice,
		XmNmaxLength, MAXNAMELEN,
		NULL);

	XmTextFieldSetString(mountCell, mountName);

	CylinderCells[slice].sizeWidget = sizeCell =
		pfgGetNamedWidget(slice_list, "sizeText");
	pfgSetWidgetString(slice_list, "sizeText", sizeString);
	XtVaSetValues(sizeCell,
		XmNuserData, slice,
		NULL);

	CylinderCells[slice].startWidget = startCell =
		pfgGetNamedWidget(slice_list, "startText");
	pfgSetWidgetString(slice_list, "startText", startString);
	XtVaSetValues(startCell,
		XmNuserData, slice,
		NULL);

	CylinderCells[slice].endWidget =
		pfgGetNamedWidget(slice_list, "endText");
	pfgSetWidgetString(slice_list, "endText", endString);
	XtVaSetValues(CylinderCells[slice].endWidget,
		XmNuserData, slice,
		NULL);

	/*
	 * add callbacks to copy contents of cell to main text field when
	 * cell receives focus
	 */
	XtAddCallback(mountCell, XmNfocusCallback, sizeFocusCB, mainText);
	XtAddCallback(sizeCell, XmNfocusCallback, sizeFocusCB, mainText);
	XtAddCallback(startCell, XmNfocusCallback, sizeFocusCB, mainText);

	/* add losing focus callback to update disk when cell has changed */

	XtAddCallback(mountCell, XmNlosingFocusCallback,
		mountLosingFocus, mainText);

	/* add callback to duplicate input in cells into main text field */

	XtAddCallback(mountCell, XmNvalueChangedCallback,
		cellValueChanged, mainText);
	XtAddCallback(sizeCell, XmNvalueChangedCallback,
		cellValueChanged, mainText);
	XtAddCallback(startCell, XmNvalueChangedCallback,
		cellValueChanged, mainText);

	/*
	 * add callbacks to verify valid input if valid insert into main
	 * textt field
	 */

	XtAddCallback(mountCell, XmNmodifyVerifyCallback,
		mountVerifyCB, mainText);

	if (slice == BOOT_SLICE || slice == ALT_SLICE) {
		XtVaSetValues(sizeCell,
			XmNtraversalOn, False,
			XmNeditable, False,
			NULL);
		XtVaSetValues(mountCell,
			XmNtraversalOn, False,
			XmNeditable, False,
			NULL);
		XtVaSetValues(startCell,
			XmNtraversalOn, False,
			XmNeditable, False,
			NULL);
	}

	free(slice_list);
}


/* ARGSUSED */
static void
cylOkCB(Widget w, XtPointer disk, XtPointer callD)
{
	int		ret = True;
	int		num_errors;
	Errmsg_t	*error_list;

	if (debug)
		(void) printf("pfgdisks:cylOkCB, calling updatefilesys and close\n");

	/*
	 *	validate_sdisk used to be called here, but since it is an
	 *	obsolete function in the disk library it has been removed
	 *	and replcaed with check_sdisk to fix bug id 1221258
	 */

	/* LINTED [pointer cast] */
	num_errors = check_sdisk((Disk_t *) disk);
	if (num_errors > 0) {
		if (debug)
			(void) printf("check_sdisk found and error");
		WALK_LIST(error_list, get_error_list()) {
			if (error_list-> code < 0) {
				/*
				 * an error was found
				 */
				pfgDiskError(cyl_dialog, "check_disk",
						error_list->code);
				free_error_list();
				return;
			} else if (error_list-> code > 0) {
				/*
				 * a warning was found
				 */
				ret = pfgAppQuery(cyl_dialog,
					error_list->msg);
				free_error_list();
			}
		}
	}

	if (ret == True) {
		set_slice_autoadjust(1);
		set_units(D_MBYTE);
		XtUnmanageChild(cyl_dialog);
		/* LINTED [pointer cast] */
		updateDiskCells((Disk_t *) disk);

		pfgUnbusy(pfgShell(XtParent(pfgShell(w))));
	}

	/* pfgParade(parDisks, parAContinue); */
}


/* ARGSUSED */
static void
cylCancelCB(Widget w, XtPointer diskPtr, XtPointer callD)
{
	if (debug)
		(void) printf("pfgdisks:cylCancelCB, calling pfgQuery\n");

	set_units(D_MBYTE);
	set_dfltmnt_list(MountList);	/* restore default mount list */

	pfgUnbusy(pfgShell(XtParent(pfgShell(w))));

	/* LINTED [pointer cast] */
	restoreDiskConfig((Disk_t *) diskPtr);
	XtUnmanageChild(cyl_dialog);
}

void
pfgUpdateCylinders(void)
{
	pfDiskW_t *pfdisk;
	int slice, start, end;
	char newString[80], *string, *mount;
	int i;

	if (CylinderCells != NULL && CylinderCells[0].startWidget != NULL) {
		XtVaGetValues(XtParent(XtParent(CylinderCells[0].startWidget)),
			XmNuserData, &pfdisk,
			NULL);
	}
	for (i = 0; i < LAST_STDSLICE + 1; i++) {
		XtVaGetValues(CylinderCells[i].startWidget,
			XmNuserData, &slice,
			NULL);
		mount = XmTextGetString(CylinderCells[i].mountWidget);
		if (strcmp(mount, slice_mntpnt(pfdisk->d, slice))) {

			if (slice_mntpnt(pfdisk->d, slice) &&
				strcmp(slice_mntpnt(pfdisk->d, slice), "")) {
				(void) strcpy(newString, slice_mntpnt(pfdisk->d,
					slice));
			} else if (slice_size(pfdisk->d, slice)) {
				(void) strcpy(newString, APP_FS_UNNAMED);
			} else {
				(void) strcpy(newString, "");
			}
			XmTextFieldSetString(CylinderCells[i].mountWidget,
				newString);

		}
		/* update size field */
		string = XmTextFieldGetString(CylinderCells[i].sizeWidget);
		if (atoi(string) != blocks_to_cyls(pfdisk->d,
			slice_size(pfdisk->d, slice))) {
			if (slice_size(pfdisk->d, slice) > 0) {
				(void) sprintf(newString, "%d",
					blocks_to_cyls(pfdisk->d,
					slice_size(pfdisk->d, slice)));
			} else if (slice_mntpnt(pfdisk->d, slice) &&
				strcmp(slice_mntpnt(pfdisk->d, slice), "")) {
				(void) sprintf(newString, "%d", 0);
			} else {
				(void) strcpy(newString, "");
			}
			XmTextFieldSetString(CylinderCells[i].sizeWidget,
				newString);
		}
		/* update start cylinder */
		string = XmTextFieldGetString(CylinderCells[i].startWidget);
		start = atoi(string);
		if (start != slice_start(pfdisk->d, slice)) {
			if (slice_start(pfdisk->d, slice) ||
				slice_start(pfdisk->d, slice)) {
				(void) sprintf(newString, "%d",
					slice_start(pfdisk->d, slice));
			}
			XmTextFieldSetString(CylinderCells[i].startWidget,
				newString);
		}
		/* update end cylinder */
		string = XmTextFieldGetString(CylinderCells[i].endWidget);
		end = blocks_to_cyls(pfdisk->d, slice_size(pfdisk->d, slice)) +
			slice_start(pfdisk->d, slice);
		end = (end == 0) ? 0 : end - 1;
		if (atoi(string) != end) {
			(void) sprintf(newString, "%d", end);
			XmTextFieldSetString(CylinderCells[i].endWidget,
				newString);
		}
	}
}

/* ARGSUSED */
static void
cylLoadCB(Widget w, XtPointer diskPtr, XtPointer callD)
{
	if (pfgQuery(cyl_dialog, pfQLOADVTOC)) {
		/* LINTED [pointer cast] */
		pfgLoadExistingDisk((Disk_t *) diskPtr);
		pfgUpdateCylinders();
	}
}
TACH_FORM); n++;
  XtSetValues(widget_array[WI_PANELHELPTEXT], args, n);


  /***************** cylForm : XmForm *****************/
  n = 0;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopWidget, widget_array[WI_PANELHELPTEXT]); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNbottomWidget, widget_array[WI_MESSAGEBOX]); n++;
  XtSetValues(widget_array[WI_CYLFORM], args, n);


  /***************** mainText : XmTextField *****************/
  n = 0;
  XtAddCallback(widget_array[WI_MAINTEXT],
                XmNfocusCallback,
                (XtCallbackProc)mainFocusCB,
                (XtPointer)NULL);

  XtAddCallback(widget_array[WI_MAINTEXT],
                XmNmotionVerifyCallback,
                (XtCallbackProc)mainMotionCB,
                (XtPointer)NULL);

  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
  XtSetArg(args[n], XmNbottomWidget, widget_array[WI_RECOMMENDEDFORM]); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNrightWidget, widget_array[WI_RECOMMENDEDFORM]); n++;
  XtSetValues(widget_array[WI_MAINTEXT], args, n);

  XtManageChild(widget_array[WI_MAINTEXT]);

  /***************** recommendedForm : XmForm *****************/
  n = 0;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNrightWidget, widget_array[WI_MINIMUMFORM]); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNrightOffset, 15); n++;
  XtSetValues(widget_array[WI_RECOMMENDEDFORM], args, n);


  /***************** recommendedLabel : XmLabel *****************/
  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_RECOMMENDEDLABEL], args, n);

  XtManageChild(widget_array[WI_RECOMMENDEDLABEL]);

  /***************** recommendedFrame : XmFrame *****************/
  n = 0;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNtopWidget, widget_array[WI_RECOMMENDEDLABEL]); n++;
  XtSetValues(widget_array[WI_RECOMMENDEDFRAME], args, n);

  XtManageChild(widget_array[WI_RECOMMENDEDVALUE]);
  XtManageChild(widget_array[WI_RECOMMENDEDFRAME]);
  XtManageChild(widget_array[WI_RECOMMENDEDFORM]);

  /***************** minimumForm : XmForm *****************/
  n = 0;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_MINIMUMFORM], args, n);


  /***************** minimumLabel : XmLabel *****************/
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_MINIMUMLABEL], args, n);

  XtManageChild(widget_array[WI_MINIMUMLABEL]);

  /***************** minimumFrame : XmFrame *****************/
  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNtopWidget, widget_array[WI_MINIMUMLABEL]); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_MINIMUMFRAME], args, n);

  XtManageChild(widget_array[WI_MINIMUMVALUE]);
  XtManageChild(widget_array[WI_MINIMUMFRAME]);
  XtManageChild(widget_array[WI_MINIMUMFORM]);

  /***************** cylMatrixFrame : XmFrame *****************/
  n = 0;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNtopWidget, widget_array[WI_MINIMUMFORM]); n++;
  XtSetArg(args[n], XmNtopOffset, 10); n++;
  XtSetValues(widget_array[WI_CYLMATRIXFRAME], args, n);


  /***************** cylEntryRowColumn : XmRowColumn *****************/
  n = 0;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_CYLENTRYROWCOLUMN], args, n);

  XtManageChild(widget_array[WI_DISKLABEL]);
  XtManageChild(widget_array[WI_HEADINGLABEL]);
  XtManageChild(widget_array[WI_CYLENTRYROWCOLUMN]);

  /***************** labelRowColumn : XmRowColumn *****************/
  n = 0;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNrightWidget, widget_array[WI_VALUEROWCOLUMN]); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNtopWidget, widget_array[WI_CYLENTRYROWCOLUMN]); n++;
  XtSetValues(widget_array[WI_LABELROWCOLUMN], args, n);

  XtManageChild(widget_array[WI_OVERHEADLABEL]);
  XtManageChild(widget_array[WI_ALLOCATEDLABEL]);
  XtManageChild(widget_array[WI_FREELABEL]);
  XtManageChild(widget_array[WI_CAPACITYLABEL]);
  XtManageChild(widget_array[WI_LABELROWCOLUMN]);

  /***************** valueRowColumn : XmRowColumn *****************/
  n = 0;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNtopWidget, widget_array[WI_CYLENTRYROWCOLUMN]); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
  XtSetArg(args[n], XmNrightWidget, widget_array[WI_CYLENTRYROWCOLUMN]); n++;
  XtSetValues(widget_array[WI_VALUEROWCOLUMN], args, n);

  XtManageChild(widget_array[WI_OVERHEADVALUE]);
  XtManageChild(widget_array[WI_ALLOCATEDVALUE]);
  XtManageChild(widget_array[WI_FREEVALUE]);
  XtManageChild(widget_array[WI_CAPACITYVALUE]);
  XtManageChild(widget_array[WI_VALUEROWCOLUMN]);
  XtManageChild(widget_array[WI_CYLMATRIXFORM]);
  XtManageChild(widget_array[WI_CYLMATRIXFRAME]);
  XtManageChild(widget_array[WI_CYLFORM]);

  /***************** messageBox : XmMessageBox *****************/
  n = 0;
  XtSetArg(args[n], XmNdefaultButton, widget_array[WI_OKBUTTON]); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_MESSAGEBOX], args, n);

  XtManageChild(widget_array[WI_OKBUTTON]);
  XtManageChild(widget_array[WI_LOADBUTTON]);
  XtManageChild(widget_array[WI_CANCELBUTTON]);
  XtAddCallback(widget_array[WI_HELPBUTTON],
                XmNactivateCallback,
                pfgHelp,
                (XtPointer)"spotcylinder.r");

  XtManageChild(widget_array[WI_HELPBUTTON]);
  XtManageChild(widget_array[WI_MESSAGEBOX]);

  /*
   *  Allocate memory for the widget array to return
   */
  if (warr_ret != NULL) {
    *warr_ret = (Widget *) malloc(sizeof(Widget)*34);
    (void) memcpy((char *)*warr_ret,
                  (char *)widget_array,
           sizeof(Widget)*34);
  }

  /*
   *   Fix for SMI's X/NeWS server
   */
  tu_ol_fix_hierarchy(widget_array[WI_CYL_DIALOG]);

  /*
   *   Invoke the create callbacks
   */
  tu_widcre_invoke_hooks(widget_array[WI_CYL_DIALOG]);

  /*
   *   Return the first created widget.
   */
  return widget_array[WI_CYL_DIALOG];
}


/**************************************************************
 * tu_cyl_slice_entry_widget:
 **************************************************************/
Widget tu_cyl_slice_entry_widget(char    * name,
                                 Widget    parent,
                                 Widget ** warr_ret)
{
  Arg args[17];
  Widget widget_array[7];
  int n;
  /* Make sure the classes used are initialized */
  class_init();

  /***************** object of type : XmRowColumn *****************/
  n = 0;
  XtSetArg(args[n], XmNorientation, XmHORIZONTAL); n++;
  XtSetArg(args[n], XmNnavigationType, XmNONE); n++;
  XtSetArg(args[n], XmNmarginHeight, 0); n++;
  widget_array[WI_CYL_SLICE_ENTRY] =
    XmCreateRowColumn(parent, name, args, n);

  /***************** sliceLabel : XmLabel *****************/
  widget_array[WI_SLICELABEL] =
    XmCreateLabel(widget_array[WI_CYL_SLICE_ENTRY], "sliceLabel", NULL, 0);

  /***************** mountText : XmTextField *****************/
  n = 0;
  XtSetArg(args[n], XmNcolumns, 20); n++;
  XtSetArg(args[n], XmNnavigationType, XmNONE); n++;
  XtSetArg(args[n], XmNcursorPositionVisible, False); n++;
  widget_array[WI_MOUNTTEXT] =
    XmCreateTextField(widget_array[WI_CYL_SLICE_ENTRY], "mountText", args, n);

  /***************** sizeText : XmTextField *****************/
  n = 0;
  XtSetArg(args[n], XmNcolumns, 8); n++;
  XtSetArg(args[n], XmNmaxLength, 8); n++;
  XtSetArg(args[n], XmNnavigationType, XmNONE); n++;
  XtSetArg(args[n], XmNcursorPositionVisible, False); n++;
  widget_array[WI_SIZETEXT] =
    XmCreateTextField(widget_array[WI_CYL_SLICE_ENTRY], "sizeText", args, n);

  /***************** startText : XmTextField *****************/
  n = 0;
  XtSetArg(args[n], XmNmaxLength, 8); n++;
  XtSetArg(args[n], XmNcolumns, 8); n++;
  XtSetArg(args[n], XmNnavigationType, XmNONE); n++;
  XtSetArg(args[n], XmNcursorPositionVisible, False); n++;
  widget_array[WI_STARTTEXT] =
    XmCreateTextField(widget_array[WI_CYL_SLICE_ENTRY], "startText", args, n);

  /***************** endText : XmTextField *****************/
  n = 0;
  XtSetArg(args[n], XmNcolumns, 8); n++;
  XtSetArg(args[n], XmNeditable, False); n++;
  XtSetArg(args[n], XmNmaxLength, 8); n++;
  XtSetArg(args[n], XmNnavigationType, XmNONE); n++;
  XtSetArg(args[n], XmNtraversalOn, False); n++;
  XtSetArg(args[n], XmNcursorPositionVisible, False); n++;
  widget_array[WI_ENDTEXT] =
    XmCreateTextField(widget_array[WI_CYL_SLICE_ENTRY], "endText", args, n);

  /* Terminate the widget array */
  widget_array[6] = NULL;

  XtManageChild(widget_array[WI_SLICELABEL]);
  XtAddCallback(widget_array[WI_MOUNTTEXT],
                XmNactivateCallback,
                (XtCallbackProc)mountActivateCB,
                (XtPointer)NULL);

  XtManageChild(widget_array[WI_MOUNTTEXT]);
  XtAddCallback(widget_array[WI_SIZETEXT],
                XmNlosingFocusCallback,
                (XtCallbackProc)sizeLosingFocus,
                (XtPointer)NULL);

  XtAddCallback(widget_array[WI_SIZETEXT],
                XmNactivateCallback,
                (XtCallbackProc)sizeActivateCB,
                (XtPointer)NULL);

  XtAddCallback(widget_array[WI_SIZETEXT],
                XmNmodifyVerifyCallback,
                (XtCallbackProc)sizeVerifyCB,
                (XtPointer)NULL);

  XtManageChild(widget_array[WI_SIZETEXT]);
  XtAddCallback(widget_array[WI_STARTTEXT],
                XmNlosingFocusCallback,
                (XtCallbackProc)startLosingFocus,
                (XtPointer)NULL);

  XtAddCallback(widget_array[WI_STARTTEXT],
                XmNactivateCallback,
                (XtCallbackProc)startActivateCB,
                (XtPointer)NULL);

  XtAddCallback(widget_array[WI_STARTTEXT],
                XmNmodifyVerifyCallback,
                (XtCallbackProc)sizeVerifyCB,
                (XtPointer)NULL);

  XtManageChild(widget_array[WI_STARTTEXT]);
  XtManageChild(widget_array[WI_ENDTEXT]);
  XtManageChild(widget_array[WI_CYL_SLICE_ENTRY]);

  /*
   *  Allocate memory for the widget array to return
   */
  if (warr_ret != NULL) {
    *warr_ret = (Widget *) malloc(sizeof(Widget)*7);
    (void) memcpy((char *)*warr_ret,
                  (char *)widget_array,
           sizeof(Widget)*7);
  }

  /*
   *   Fix for SMI's X/NeWS server
   */
  tu_ol_fix_hierarchy(widget_array[WI_CYL_SLICE_ENTRY]);

  /*
   *   Invoke the create callbacks
   */
  tu_widcre_invoke_hooks(widget_array[WI_CYL_SLICE_ENTRY]);

  /*
   *   Return the first created widget.
   */
  return widget_array[WI_CYL_SLICE_ENTRY];
}



/****************************************************************
 * create_method:
 *     This function creates a widget hierarchy using the
 *     functions generated above.
 ****************************************************************/
static Widget create_method(char               * temp,
                            char               * name,
                            Widget               parent,
                            Display            * disp,
                            Screen             * screen,
                            tu_template_descr  * retval)
{
  Widget w;

  sDisplay = disp;
  sScreen = screen;

  /* check each node against its name and call its
   * create function if appropriate */
  w = NULL;
  if (strcmp(temp, "cyl_dialog") == 0){
    w = tu_cyl_dialog_widget(name, parent, (Widget **)retval);
  }
  else if (strcmp(temp, "cyl_slice_entry") == 0){
    w = tu_cyl_slice_entry_widget(name, parent, (Widget **)retval);
  }

  sDisplay = NULL;
  sScreen = NULL;
  return w;
}

