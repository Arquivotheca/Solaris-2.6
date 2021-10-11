#ifndef lint
#pragma ident "@(#)pfgsolarispart.c 1.33 96/04/29 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	pfgsolarispart.c
 * Group:	installtool
 * Description:
 */

#include "pf.h"
#include "pfg.h"

#include "pfgSolarPart_ui.h"

Widget		solarpart_dialog;

static void solarpartOkCB(Widget, XtPointer, XtPointer);
static Disk_t *DiskPtr;

/* parent of the solaris partition window */
static Widget Parent;
static WidgetList widget_list;

void
pfgCreateSolarPart(Widget parent, Disk_t *diskPtr)
{
	Widget radioBox;
	char value[PFG_MBSIZE_LENGTH], *string;
	int maxSize, part;

	pfgBusy(pfgShell(parent));

	/* set file global to be past to pfgCreateSolarCust */
	Parent = parent;

	DiskPtr = diskPtr;

	solarpart_dialog = tu_solarpart_dialog_widget("solarpart_dialog",
		parent, &widget_list);

	XmAddWMProtocolCallback(pfgShell(solarpart_dialog), pfgWMDeleteAtom,
		(XtCallbackProc) pfgExit, NULL);

	XtVaSetValues(pfgShell(solarpart_dialog),
		XmNtitle, TITLE_CREATESOLARIS,
		XmNdeleteResponse, XmDO_NOTHING,
		NULL);
	xm_SetNoResize(pfgTopLevel, solarpart_dialog);

	string = xmalloc(strlen(PFG_SP_ENTIRE) + 32 + 1);
	(void) sprintf(value, "%d", sectors_to_mb(usable_disk_blks(diskPtr)));
	(void) sprintf(string, PFG_SP_MBFMT, PFG_SP_ENTIRE, value);
	pfgSetWidgetString(widget_list, "entireToggle", string);
	free(string);

	string = xmalloc(strlen(PFG_SP_REMAIN) + 32 + 1);
	getLargestPart(diskPtr, &maxSize, &part);
	maxSize = blocks_to_mb_trunc(diskPtr, maxSize);
	(void) sprintf(value, "%d", maxSize);
	(void) sprintf(string, PFG_SP_MBFMT, PFG_SP_REMAIN, value);
	pfgSetWidgetString(widget_list, "remainderToggle", string);
	free(string);

	pfgSetWidgetString(widget_list, "customToggle", PFG_SP_CREATE);

	if (maxSize == 0) {
		XtUnmanageChild(pfgGetNamedWidget(widget_list,
			"remainderToggle"));
	}

	radioBox = pfgGetNamedWidget(widget_list, "solarpartRadioBox");

	XmToggleButtonSetState(pfgGetNamedWidget(widget_list, "entireToggle"),
		True, False);

	pfgSetWidgetString(widget_list, "panelhelpText", MSG_CREATESOLARIS);
	pfgSetWidgetString(widget_list, "okButton", PFG_OKAY);
	pfgSetWidgetString(widget_list, "cancelButton", PFG_CANCEL);
	pfgSetWidgetString(widget_list, "helpButton", PFG_HELP);

	XtAddCallback(pfgGetNamedWidget(widget_list, "okButton"),
		XmNactivateCallback, solarpartOkCB, (Widget) radioBox);

	XtManageChild(solarpart_dialog);

	XmProcessTraversal(pfgGetNamedWidget(widget_list, "okButton"),
		XmTRAVERSE_CURRENT);
}



/* ARGSUSED */
static void
solarpartOkCB(Widget w, XtPointer radioBox, XtPointer callD)
{
	Widget button = NULL;
	int err = D_OK;
	int err1, err2;
	Boolean set;

	XtVaGetValues(radioBox,
		XmNmenuHistory, &button,
		NULL);

	if (button == NULL) {
		if (debug)
			(void) printf("no menu history\n");
		return;
	}

	XtVaGetValues(button,
		XmNset, &set,
		NULL);

	if (set == True) {
		select_disk(DiskPtr, NULL);

		if (button == pfgGetNamedWidget(widget_list, "entireToggle")) {
			if (debug)
				(void) printf("selected entire\n");
			err = useEntireDisk(DiskPtr);
		} else if (button == pfgGetNamedWidget(widget_list,
				"remainderToggle")) {
			if (debug)
				(void) printf("selected remainder\n");
			err = useLargestPart(DiskPtr);
		} else if (button == pfgGetNamedWidget(widget_list,
				"customToggle")) {
			pfgCreateSolarCust(Parent, DiskPtr);
			if (debug)
				(void) printf("selected custom\n");
		}

		if (err == D_OK) {
			if (button != pfgGetNamedWidget(widget_list,
					"customToggle")) {
				err1 = validate_fdisk(DiskPtr);
				if (err1) {
					pfgDiskError(XtParent(radioBox),
						"validate_fdisk", err1);
					deselect_disk(DiskPtr, NULL);
					return;
				}
				err2 = commit_disk_config((Disk_t *) DiskPtr);
				if (err2 != D_OK) {
					pfgDiskError(solarpart_dialog,
						NULL, err2);
					return;
				}
				moveDisk(DiskPtr, True);
			}
			XtUnmanageChild(solarpart_dialog);
			XtDestroyWidget(solarpart_dialog);

		} else {
			deselect_disk(DiskPtr, NULL);
		}
	}


	if (err == D_OK && err1 == D_OK && err2 == D_OK) {
		/*
		 * only free the widget list if all disk checks
		 * are clean
		 */
		free(widget_list);
	}

	pfgUnbusy(pfgShell(Parent));
}

/* ARGSUSED */
void
solarpartCancelCB(Widget w, XtPointer clientD, XtPointer callD)
{
	int err;

	err = restore_disk((Disk_t *) DiskPtr, CFG_COMMIT);
	if (err != D_OK) {
		pfgDiskError(solarpart_dialog, NULL, err);
	}

	free(widget_list);

	deselect_disk(DiskPtr, NULL);
	XtUnmanageChild(solarpart_dialog);
	XtDestroyWidget(solarpart_dialog);

	pfgUnbusy(pfgShell(Parent));
}
