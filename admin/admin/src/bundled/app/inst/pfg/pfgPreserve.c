#ifndef lint
#pragma ident "@(#)pfgpreserve.c 1.51 96/02/09 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	pfgpreserve.c
 * Group:	installtool
 * Description:
 */

#include "pf.h"
#include "pfg.h"
#include "pfgpreserve.h"

#include "pfgPreserve_ui.h"

static void preserveContinueCB(Widget, XtPointer, XtPointer);
static void mountLosingFocusCB(Widget, XtPointer, XtPointer);
static void mountPresActivateCB(Widget, XtPointer, XtPointer);
static int mountChanged(Widget, Widget);

static Widget preserve_dialog;

static char *empty_string = "         ";

/* ARGSUSED */
Widget
pfgCreatePreserve(Widget parent)
{
	Widget rowColumn;
	Widget preserve_entry;
	WidgetList widget_list;
	WidgetList preserve_list;

	XmString xmstr;
	int maxLength = 0, i, sliceLength = 2, size_length = 4;
	char *diskSlice, size[32];
	char *mountPoint;
	struct disk *d;
	Dimension width, height;
	PreserveStruct *preserveHead = NULL, *current;
	int been_here = 0;

	/*
	 * commit current disk configurations so we can reset back to them
	 * if necessary
	 */
	pfgCommitDisks();

	/* determine maximum string length of disk name */
	for (d = first_disk(); d; d = next_disk(d)) {
		if ((int) (strlen(disk_name(d))) > maxLength) {
			maxLength = strlen(d->name);
		}
	}

	diskSlice = (char *) xmalloc(maxLength + 1 + sliceLength + 1);

	preserve_dialog = tu_preserve_dialog_widget("preserve_dialog",
		parent, &widget_list);

	XtVaSetValues(pfgShell(preserve_dialog),
		XmNtitle, TITLE_PRESERVE,
		XmNdeleteResponse, XmDO_NOTHING,
		NULL);

	XmAddWMProtocolCallback(pfgShell(preserve_dialog), pfgWMDeleteAtom,
	    (XtCallbackProc) pfgExit, NULL);

	pfgSetWidgetString(widget_list, "panelhelpText", MSG_PRESERVE);
	pfgSetWidgetString(widget_list, "sliceColumnLabel", PFG_PS_DSKSLICE);
	pfgSetWidgetString(widget_list, "fileSystemColumnLabel",
		PFG_PS_FILESYS);
	pfgSetWidgetString(widget_list, "sizeColumnLabel", PFG_PS_SIZE);
	pfgSetWidgetString(widget_list, "preserveColumnLabel", PFG_PS_PRESERVE);
	pfgSetWidgetString(widget_list, "continueButton", PFG_CONTINUE);
	pfgSetWidgetString(widget_list, "cancelButton", PFG_CANCEL);
	pfgSetWidgetString(widget_list, "helpButton", PFG_HELP);

	rowColumn = pfgGetNamedWidget(widget_list, "preserveRowColumn");

	/*
	 * loop through linked list of disks creating an entry on the
	 * Preserve screen for each disk partition
	 */

	for (d = first_disk(); d; d = next_disk(d)) {
		if (disk_not_selected(d)) {
			continue;
		}
		for (i = 0; i < NUMPARTS; i++) {
			if ((orig_slice_size(d, i) > 0) &&
			    !slice_locked(d, i)) {
				if (!been_here) {
					been_here = 1;
					preserveHead = (PreserveStruct *)
					    xmalloc(sizeof (PreserveStruct));
					current = preserveHead;
				} else {
					current->next = (PreserveStruct *)
					    xmalloc(sizeof (PreserveStruct));
					current = current->next;
				}

				preserve_entry = tu_preserve_entry_widget(
					"preserve_entry", rowColumn,
					&preserve_list);

				current->slice = i;
				current->disk = d;
				(void) sprintf(diskSlice, "%*ss%-*d", maxLength,
				    disk_name(d), sliceLength, i);

				current->diskSlice =
					pfgGetNamedWidget(preserve_list,
					"preserveSliceLabel");
				xmstr = XmStringCreateLocalized(diskSlice);
				XtVaSetValues(current->diskSlice,
					XmNlabelString, xmstr,
					NULL);
				XmStringFree(xmstr);

				mountPoint = (char *) xmalloc(MAXNAMELEN);
				(void) strcpy(mountPoint,
					slice_preserved(d, i) ?
					slice_mntpnt(d, i) :
					orig_slice_mntpnt(d, i));
				current->mountField =
					pfgGetNamedWidget(preserve_list,
					"preserveMountText");
				XtVaSetValues(current->mountField,
					XmNuserData, mountPoint,
					XmNmaxLength, MAXNAMELEN,
					NULL);

				XmTextFieldSetString(current->mountField,
						slice_preserved(d, i) ?
						slice_mntpnt(d, i) :
						orig_slice_mntpnt(d, i));

				current->preserveButton =
					pfgGetNamedWidget(preserve_list,
					"preserveToggleButton");
				xmstr = XmStringCreateLocalized(empty_string);
				XtVaSetValues(current->preserveButton,
					XmNlabelString, xmstr,
					XmNuserData, current,
					NULL);
				XmStringFree(xmstr);

				XtAddCallback(current->mountField,
					XmNlosingFocusCallback,
					mountLosingFocusCB,
					current->preserveButton);
				XtAddCallback(current->mountField,
					XmNactivateCallback,
					mountPresActivateCB,
					current->preserveButton);

				if (slice_preserved(d, i)) {
					xmstr = XmStringCreateLocalized(
						PFG_PRESERVED);
					XtVaSetValues(current->preserveButton,
						XmNlabelString, xmstr,
						XmNset, True,
						NULL);
					XmStringFree(xmstr);
				}
				set_units(D_MBYTE);
				(void) sprintf(size, "%*d MB", size_length,
				    blocks2size(d, orig_slice_size(d, i),
				    ROUNDDOWN));

				current->sizeField =
					pfgGetNamedWidget(preserve_list,
					"preserveSizeLabel");
				xmstr = XmStringCreateLocalized(size);
				XtVaSetValues(current->sizeField,
					XmNlabelString, xmstr,
					NULL);
				XmStringFree(xmstr);

				free(preserve_list);

				XtManageChild(preserve_entry);
			}
		}
	}

	free(diskSlice);

	current->next = NULL;

	/*
	 * align the scrolled window titles
	 */
	if (current) {
		XtVaGetValues(current->diskSlice,
			XmNwidth, &width,
			NULL);
		XtVaSetValues(
			pfgGetNamedWidget(widget_list, "sliceColumnLabel"),
			XmNwidth, width,
			NULL);

		XtVaGetValues(current->mountField,
			XmNwidth, &width,
			NULL);
		XtVaSetValues(
			pfgGetNamedWidget(widget_list, "fileSystemColumnLabel"),
			XmNwidth, width,
			NULL);

		XtVaGetValues(current->sizeField,
			XmNwidth, &width,
			NULL);
		XtVaSetValues(
			pfgGetNamedWidget(widget_list, "sizeColumnLabel"),
			XmNwidth, width,
			NULL);

		XtVaGetValues(current->preserveButton,
			XmNwidth, &width,
			NULL);
		XtVaSetValues(
			pfgGetNamedWidget(widget_list, "preserveColumnLabel"),
			XmNwidth, width,
			NULL);
	}

	XtAddCallback(pfgGetNamedWidget(widget_list, "continueButton"),
		XmNactivateCallback, preserveContinueCB, preserveHead);

	XtManageChild(preserve_dialog);

	XmProcessTraversal(pfgGetNamedWidget(widget_list, "continueButton"),
		XmTRAVERSE_CURRENT);

	XtVaGetValues(pfgShell(preserve_dialog),
	    XmNwidth, &width,
	    XmNheight, &height,
	    NULL);

	XtVaSetValues(pfgShell(preserve_dialog),
	    XmNminWidth, width,
	    XmNmaxWidth, width,
	    XmNminHeight, height,
	    NULL);

	free(widget_list);

	return (preserve_dialog);
}

/* ARGSUSED */
void
preserveContinueCB(Widget w, XtPointer preserveHead, XtPointer call_data)
{
	PreserveStruct *p, *prevp;
	Widget shell = pfgShell(w);

	if (debug)
		(void) printf("pfgpreserve:preserveContinueCB\n");

	/* reset names of non-preserved slices */
	pfgResetNames();

	/*
	 * null out slices that were changed from preserve to unpreserve
	 */
	pfgNullUnpres();

	/* commit changes */
	pfgCommitDisks();

	/* free linked list pointed to by preserveHead */
	/* LINTED [pointer cast] */
	p = (PreserveStruct *) preserveHead;
	while (p != NULL) {
		prevp = p;
		p = p->next;
		free(prevp);
	}

	XtUnmanageChild(shell);
	XtDestroyWidget(shell);

	pfgUnbusy(pfgShell(XtParent(pfgShell(w))));

	pfgSetAction(parAContinue);
}


/* ARGSUSED */
void
preserveToggleCB(Widget w, XtPointer clientD, XtPointer callD)
{
	XmToggleButtonCallbackStruct *cbs =
		/* LINTED [pointer cast] */
		(XmToggleButtonCallbackStruct *) callD;

	PreserveStruct *ptr;
	char *value;
	XmString xmstr = XmStringCreateLocalized(empty_string);
	int err, i;

	/* array of overlaping slices returned from slice_overlaps */
	int *overlaps;

	XtVaGetValues(w,	/* XtParent (w), */
	    XmNuserData, &ptr,
	    NULL);
	if (cbs->set == True) {
		value = XmTextFieldGetString(ptr->mountField);
		/* check if mount point name has changed */
		if (strcmp(value, slice_mntpnt(ptr->disk, ptr->slice)) != 0) {
			if (err = filesys_preserve_ok(value)) {
				XmToggleButtonSetState(w, False, False);
				pfgDiskError(preserve_dialog, NULL, err);
				return;
			}
			if (err = set_slice_mnt(ptr->disk, ptr->slice, value,
				NULL)) {
				XmToggleButtonSetState(w, False, False);
				pfgDiskError(preserve_dialog,
					"set_slice_mnt", err);
				return;
			}
		}
		if (err = filesys_preserve_ok(value)) {
			XmToggleButtonSetState(w, False, False);
			pfgDiskError(preserve_dialog, NULL, err);
			return;
		}
		if (err = slice_preserve_ok(ptr->disk, ptr->slice)) {
			if (err == D_CHANGED) {
				/* ask user if ok to lose disk edits */
				if (!pfgQuery(preserve_dialog,
						pfQLOSECHANGES)) {
					XmToggleButtonSetState(w,
					    False, False);
					return;
				}
				/*
				 * user wishes to overwrite disk
				 * edits
				 */
				/*
				 * check if original parameters
				 * overlap any slices if so than
				 * reset slice
				 */
				if (err = slice_overlaps(ptr->disk,
					ptr->slice,
					orig_slice_start(ptr->disk,
					    ptr->slice),
					orig_slice_size(ptr->disk,
					    ptr->slice),
					&overlaps)) {
					for (i = 0; i < err; i++) {
						set_slice_geom(
						    ptr->disk,
						    overlaps[i], 0, 0);
						set_slice_mnt(ptr->disk,
						    overlaps[i], "",
						    NULL);
					}
				}
				slice_stuck_on(ptr->disk, ptr->slice);
				set_slice_geom(ptr->disk, ptr->slice,
				    GEOM_ORIG, GEOM_ORIG);
				if ((err = set_slice_preserve(ptr->disk,
				    ptr->slice, PRES_YES)) != D_OK) {
					XmToggleButtonSetState(w,
					    False, False);
					slice_stuck_off(ptr->disk,
					    ptr->slice);
					pfgDiskError(preserve_dialog, NULL,
					    err);
					return;
				}
			} else {
				XmToggleButtonSetState(w, False, False);
				pfgDiskError(preserve_dialog, NULL, err);
				return;
			}
		} else {	/* ok to preserve slice */
			if (err = set_slice_preserve(ptr->disk, ptr->slice,
				PRES_YES)) {
				XmToggleButtonSetState(w, False, False);
				pfgDiskError(preserve_dialog, NULL, err);
				return;
			}
		}
		XmStringFree(xmstr);
		xmstr = XmStringCreateLocalized(PFG_PRESERVED);
	} else {		/* preserve was turned off */
		set_slice_preserve(ptr->disk, ptr->slice, PRES_NO);
	}
	XtVaSetValues(w, XmNlabelString, xmstr, NULL);
	XmStringFree(xmstr);
}


/* ARGSUSED */
void
mountPresActivateCB(Widget w, XtPointer checkBox, XtPointer callD)
{
	int ret;

	ret = mountChanged(w, (Widget) checkBox);
	if (ret == D_OK)
		XmProcessTraversal(w, XmTRAVERSE_DOWN);
}


/* ARGSUSED */
static void
mountLosingFocusCB(Widget w, XtPointer checkBox, XtPointer callD)
{
	(void) mountChanged(w, (Widget) checkBox);
}


/* ARGSUSED1 */
int
mountChanged(Widget w, Widget checkBox)
{
	char *mountPoint;
	PreserveStruct *ptr;
	int ret = D_OK;
	XmString string;
	char *oldMount;

	XtVaGetValues(checkBox,
	    XmNuserData, &ptr,
	    NULL);
	mountPoint = XmTextFieldGetString(w);

	/* check if mount point has changed */
	if (strcmp(slice_mntpnt(ptr->disk, ptr->slice), mountPoint) != 0 &&
	    strcmp(orig_slice_mntpnt(ptr->disk, ptr->slice), mountPoint) != 0) {
		if ((ret = filesys_preserve_ok(mountPoint)) != D_OK) {
			XmToggleButtonSetState(checkBox, False,
			    False);
			string = XmStringCreateLocalized(empty_string);
			XtVaSetValues(checkBox,
			    XmNlabelString, string,
			    NULL);
			XmStringFree(string);
			pfgDiskError(preserve_dialog, NULL, ret);
			return (ret);
		} else {
/*
			if (XmToggleButtonGetState((Widget) checkBox)) {
				ret = set_slice_mnt(ptr->disk, ptr->slice,
				    mountPoint, NULL);
			} else {
				ret = slice_name_ok(mountPoint);
			}
*/
			ret = set_slice_mnt(ptr->disk, ptr->slice,
			    mountPoint, NULL);

			if (ret != D_OK) {
				XmToggleButtonSetState(checkBox,
				    False, False);
				string = XmStringCreateLocalized(empty_string);
				XtVaSetValues(checkBox,
				    XmNlabelString, string,
				    NULL);
				XmStringFree(string);
				XtVaGetValues(w,
				    XmNuserData, &oldMount,
				    NULL);
				if (oldMount == NULL) {
					XmTextFieldSetString(w,
					    slice_mntpnt(ptr->disk,
					    ptr->slice) ?
					    slice_mntpnt(ptr->disk, ptr->slice)
					    : "");
				} else {
					XmTextFieldSetString(w, oldMount);
				}
				pfgDiskError(preserve_dialog, "set_slice_mnt",
				    ret);
				return (ret);
			} else {
				/*
				 * if mount poit successfully set, store
				 * new name in user data field.  This is
				 * necessary in case we need to recover name
				 */
				XtVaGetValues(w,
				    XmNuserData, &oldMount,
				    NULL);
				(void) strcpy(oldMount, slice_mntpnt(ptr->disk,
				    ptr->slice));
			}
		}
	}
	return (ret);
}

/* ARGSUSED */
void
preserveCancelCB(Widget w, XtPointer clientD, XtPointer call_data)
{
	/* reset disks back to previously commited changes */
	pfgResetDisks();

	XtUnmanageChild(pfgShell(preserve_dialog));
	XtDestroyWidget(pfgShell(preserve_dialog));

	pfgUnbusy(pfgShell(XtParent(pfgShell(w))));
}
t_array[WI_PANELHELPTEXT], ttbl);
  unregister_as_dropsite(widget_array[WI_PANELHELPTEXT],
                         NULL,
                         NULL);

  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_PANELHELPTEXT], args, n);

  XtManageChild(widget_array[WI_PANELHELPTEXT]);

  /***************** preserveForm : XmForm *****************/
  n = 0;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNtopWidget, widget_array[WI_PANELHELPTEXT]); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNbottomWidget, widget_array[WI_MESSAGEBOX]); n++;
  XtSetValues(widget_array[WI_PRESERVEFORM], args, n);


  /***************** sliceColumnLabel : XmLabel *****************/
  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_SLICECOLUMNLABEL], args, n);

  XtManageChild(widget_array[WI_SLICECOLUMNLABEL]);

  /***************** fileSystemColumnLabel : XmLabel *****************/
  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
  XtSetArg(args[n], XmNtopWidget, widget_array[WI_SLICECOLUMNLABEL]); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNleftWidget, widget_array[WI_SLICECOLUMNLABEL]); n++;
  XtSetValues(widget_array[WI_FILESYSTEMCOLUMNLABEL], args, n);

  XtManageChild(widget_array[WI_FILESYSTEMCOLUMNLABEL]);

  /***************** sizeColumnLabel : XmLabel *****************/
  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
  XtSetArg(args[n], XmNtopWidget, widget_array[WI_FILESYSTEMCOLUMNLABEL]); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNleftWidget, widget_array[WI_FILESYSTEMCOLUMNLABEL]); n++;
  XtSetValues(widget_array[WI_SIZECOLUMNLABEL], args, n);

  XtManageChild(widget_array[WI_SIZECOLUMNLABEL]);

  /***************** preserveColumnLabel : XmLabel *****************/
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNleftWidget, widget_array[WI_SIZECOLUMNLABEL]); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
  XtSetArg(args[n], XmNtopWidget, widget_array[WI_SIZECOLUMNLABEL]); n++;
  XtSetValues(widget_array[WI_PRESERVECOLUMNLABEL], args, n);

  XtManageChild(widget_array[WI_PRESERVECOLUMNLABEL]);

  /***************** preserveScrolledWindow : XmScrolledWindow *****************/
  n = 0;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNtopWidget, widget_array[WI_SLICECOLUMNLABEL]); n++;
  XtSetValues(widget_array[WI_PRESERVESCROLLEDWINDOW], args, n);

  XtManageChild(widget_array[WI_PRESERVEROWCOLUMN]);
  XtManageChild(widget_array[WI_PRESERVESCROLLEDWINDOW]);
  XtManageChild(widget_array[WI_PRESERVEFORM]);

  /***************** messageBox : XmMessageBox *****************/
  n = 0;
  XtSetArg(args[n], XmNdefaultButton, widget_array[WI_CONTINUEBUTTON]); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_MESSAGEBOX], args, n);

  XtManageChild(widget_array[WI_CONTINUEBUTTON]);
  XtAddCallback(widget_array[WI_CANCELBUTTON],
                XmNactivateCallback,
                (XtCallbackProc)preserveCancelCB,
                (XtPointer)NULL);

  XtManageChild(widget_array[WI_CANCELBUTTON]);
  XtAddCallback(widget_array[WI_HELPBUTTON],
                XmNactivateCallback,
                pfgHelp,
                (XtPointer)"preserve.h");

  XtManageChild(widget_array[WI_HELPBUTTON]);
  XtManageChild(widget_array[WI_MESSAGEBOX]);

  /*
   *  Allocate memory for the widget array to return
   */
  if (warr_ret != NULL) {
    *warr_ret = (Widget *) malloc(sizeof(Widget)*16);
    (void) memcpy((char *)*warr_ret,
                  (char *)widget_array,
           sizeof(Widget)*16);
  }

  /*
   *   Fix for SMI's X/NeWS server
   */
  tu_ol_fix_hierarchy(widget_array[WI_PRESERVE_DIALOG]);

  /*
   *   Invoke the create callbacks
   */
  tu_widcre_invoke_hooks(widget_array[WI_PRESERVE_DIALOG]);

  /*
   *   Return the first created widget.
   */
  return widget_array[WI_PRESERVE_DIALOG];
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
  if (strcmp(temp, "preserve_entry") == 0){
    w = tu_preserve_entry_widget(name, parent, (Widget **)retval);
  }
  else if (strcmp(temp, "preserve_dialog") == 0){
    w = tu_preserve_dialog_widget(name, parent, (Widget **)retval);
  }

  sDisplay = NULL;
  sScreen = NULL;
  return w;
}

