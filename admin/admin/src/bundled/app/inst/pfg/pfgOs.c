#ifndef lint
#pragma ident "@(#)pfgos.c 1.30 96/10/07 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	pfgos.c
 * Group:	installtool
 * Description:
 */

#include "pf.h"
#include "pfg.h"

#include "pfgOs_ui.h"

void osContinueCB(Widget, XtPointer, XtPointer);
void osGobackCB(Widget, XtPointer, XtPointer);

static Widget os_dialog;
static Widget *slice_toggle_array;
static WidgetList *slice_toggle_array_widgetlist;
static int num_slices;
static char *old_slice = NULL;

static void
os_change_toggle_states(Widget w, XtPointer clientD,
	XmToggleButtonCallbackStruct *state);


/*
 * Function: pfgCreateOs
 * Description:
 *	Create the upgrade window that presents the multiple
 *	slices that we know about that can be upgraded and let the
 *	user pick which one to upgrade.
 * Scope:	public
 * Parameters:  none
 * Return:	[Widget] - the dialog widget
 * Globals:
 * Notes:
 *	- the window is set up so that it can resize vertically, but
 *	  not horizontally, so that the slice list can be expanded
 *	  downward if tehre are lots of them.
 */
Widget
pfgCreateOs(void)
{
	WidgetList widget_list;
	XmString labelString;
	Widget osSlicesForm;
	Widget slice_toggle;
	Widget slice_label;
	int i;
	Dimension max_width;
	Dimension width;
	Dimension height;

	/* get the dialog widget & the dialog widget list from teleuse */
	os_dialog = tu_os_dialog_widget("os_dialog",
		pfgTopLevel, &widget_list);

	/* set up exit callback off window manager close */
	XmAddWMProtocolCallback(pfgShell(os_dialog), pfgWMDeleteAtom,
		(XtCallbackProc) pfgExit, NULL);

	/* set title */
	XtVaSetValues(pfgShell(os_dialog),
		XtNtitle, TITLE_OS_MULTIPLE,
		XmNdeleteResponse, XmDO_NOTHING,
		NULL);

	/* get form we will put slice list into */
	osSlicesForm = pfgGetNamedWidget(widget_list, "osSlicesForm");

	/* how many upgradeable disks are there? */
	num_slices = SliceGetTotalNumUpgradeable(UpgradeSlices);

	/* make the first slice selected if one isn't already selected */
	SliceSelectOne(UpgradeSlices);

	/* make an array to hold the slice list of toggles */
	slice_toggle_array = (Widget *) xmalloc(sizeof (Widget) * num_slices);
	slice_toggle_array_widgetlist = (WidgetList *)
		xmalloc(sizeof (WidgetList) * num_slices);

	/*
	 * Loop through slices and set up a list of slice forms.
	 * Each slice form has a toggle button with the Solaris OS (release)
	 * name and a corresponding label with the slice name this
	 * OS's root file system is on.
	 */
	for (i = 0, max_width = 0; i < num_slices; i++) {
		/* create the string to assign to the release toggle */
		labelString = XmStringCreateLocalized(UpgradeSlices[i].release);

		/* get the slice toggle and label widget from teleuse */
		slice_toggle_array[i] =  tu_osSliceForm_widget("osSliceForm",
			osSlicesForm,
			&slice_toggle_array_widgetlist[i]);
		slice_toggle =
			pfgGetNamedWidget(slice_toggle_array_widgetlist[i],
			"osSliceToggle");
		slice_label =
			pfgGetNamedWidget(slice_toggle_array_widgetlist[i],
			"osSliceNameLabel");

		/*
		 * if this is the first slice form, attach it to the
		 * surrounding form on top.
		 * otherwise, attach it to the slice form above it.
		 */
		if (i == 0) {
			XtVaSetValues(slice_toggle_array[i],
				XmNtopAttachment, XmATTACH_FORM,
				NULL);
		} else {
			XtVaSetValues(slice_toggle_array[i],
				XmNtopAttachment, XmATTACH_WIDGET,
				XmNtopWidget, slice_toggle_array[i-1],
				NULL);
		}

		/* set the label on slice toggle and select it if appropriate */
		XtVaSetValues(slice_toggle,
			XmNlabelString, labelString,
			XmNuserData, &UpgradeSlices[i],
			XmNset, UpgradeSlices[i].selected ? True : False,
			NULL);

		/* find widest toggle width so far */
		XtVaGetValues(slice_toggle,
			XmNwidth, &width,
			XmNheight, &height,
			NULL);
		if (width > max_width)
			max_width = width;

		/* add toggle callbacks */
		XtAddCallback(slice_toggle,
			XmNvalueChangedCallback,
			(XtCallbackProc) os_change_toggle_states,
			(XtPointer) NULL);

		/* set label on disk slice label */
		XtVaSetValues(slice_label,
			XmNlabelString,
				XmStringCreateLocalized(
					UpgradeSlices[i].slice),
			XmNheight, height,
			NULL);

		XmStringFree(labelString);

		if (UpgradeSlices[i].failed)
			XtSetSensitive(slice_toggle_array[i], False);

	}

	/* set all the toggles to be the width of the widest one */
	for (i = 0; i < num_slices; i++) {
		XtVaSetValues(
			pfgGetNamedWidget(slice_toggle_array_widgetlist[i],
			"osSliceToggle"),
			XmNwidth, max_width,
			NULL);
	}

	pfgSetWidgetString(widget_list, "panelhelpText", MSG_OS);
	pfgSetWidgetString(widget_list, "osVersionLabel", OS_VERSION_LABEL);
	pfgSetWidgetString(widget_list, "osSliceLabel", LABEL_SLICE);
	pfgSetStandardButtonStrings(widget_list,
		ButtonContinue, ButtonGoback, ButtonExit, ButtonHelp,
		NULL);

	XtAddCallback(pfgGetNamedWidget(widget_list, "continueButton"),
		XmNactivateCallback, osContinueCB, NULL);

	XtManageChild(os_dialog);

	(void) XmProcessTraversal(
		pfgGetNamedWidget(widget_list, "continueButton"),
		XmTRAVERSE_CURRENT);

	free(widget_list);

	XtVaGetValues(pfgShell(os_dialog),
	    XmNwidth, &width,
	    XmNheight, &height,
	    NULL);

	XtVaSetValues(pfgShell(os_dialog),
	    XmNminWidth, width,
	    XmNmaxWidth, width,
	    XmNminHeight, height,
	    NULL);

	SlicePrintDebugInfo(UpgradeSlices);
	return (os_dialog);
}

/* ARGSUSED */
static void
os_change_toggle_states(Widget w, XtPointer clientD,
	XmToggleButtonCallbackStruct *state)
{
	Widget		toggle;
	int		i;

	/* make sure all other toggles are set to false */
	for (i = 0; i < num_slices; i++) {
		toggle = pfgGetNamedWidget(slice_toggle_array_widgetlist[i],
			"osSliceToggle");
		if (toggle != w) {
			XmToggleButtonSetState(toggle, False, False);
		}
	}
}

/* ARGSUSED */
void
osGobackCB(Widget w, XtPointer client, XtPointer cbs)
{
	pfgBusy(pfgShell(os_dialog));
	pfgSetAction(parAGoback);
}

/* ARGSUSED */
void
osContinueCB(Widget button, XtPointer client_data, XtPointer call_data)
{
	Widget toggle;
	Widget osSliceForm;
	int i;
	UpgOs_t *new_slice;
	parAction_t action;
	TChildAction status;

	/* unselect the previous slice */
	SliceSetUnselected(UpgradeSlices);

	/* find the newly selected slice */
	for (i = 0, new_slice = NULL; i < num_slices; i++) {
		toggle = pfgGetNamedWidget(slice_toggle_array_widgetlist[i],
			"osSliceToggle");

		if (XmToggleButtonGetState(toggle)) {
			osSliceForm = pfgGetNamedWidget(
				slice_toggle_array_widgetlist[i],
				"osSliceForm");
			new_slice = &UpgradeSlices[i];
			new_slice->selected = 1;
			break;
		}
	}

	if (!new_slice) {
		/* they haven't selected a slice */
		pfgWarning(os_dialog, pfErNOUPGRADEDISK);
	} else {
		pfgBusy(pfgShell(os_dialog));

		/*
		 * initialize sw lib, etc. with the new slice to
		 * upgrade.
		 */
		status = AppParentStartUpgrade(
			&FsSpaceInfo,
			UpgradeSlices,
			&pfgState,
			pfgExit,
			pfgParentReinit,
			(void *) &pfgParentReinitData);

		if (pfgState & AppState_UPGRADE_CHILD) {
			pfgSetAction(parAContinue);
			return;
		} else {
			/* we're in the parent */

			if (status != ChildUpgSliceFailure) {
				/*
				 * Anything but a ChildUpgSliceFailure means
				 * we're either ok and should continue or
				 * we're hosed and can't try any more slices
				 * and should just exit or go into an
				 * initial install.
				 */
				action = AppParentContinueUpgrade(
					status, &pfgState, pfgCleanExit);

				/*
				 * If we haven't exitted yet, then
				 * the selected slice to upgrade is OK
				 * and we can proceed with the upgrade,
				 * or we want to go onto the initial
				 * path instead.
				 */

				if (old_slice)
					free(old_slice);
				old_slice = xstrdup(new_slice->slice);

#if 0
				pfgUnbusy(pfgShell(os_dialog));
#endif
				pfgSetAction(action);
				return;
			}

			/*
			 * A ChildUpgSliceFailure means this slice failed and
			 * there are more possible slices to try and that
			 * we should try another.
			 * Set the currently selected slice insensitive
			 * and select another one for them.
			 */

			/* make unsuccessful insensitive */
			XmToggleButtonSetState(
				toggle, False, False);
			XtSetSensitive(osSliceForm, False);

			/* pick another slice */
			SliceSelectOne(UpgradeSlices);

			/* update the newly selected toggle */
			(void) SliceGetSelected(UpgradeSlices, &i);
			toggle = pfgGetNamedWidget(
				slice_toggle_array_widgetlist[i],
				"osSliceToggle");
			XmToggleButtonSetState(
				toggle, True, False);

			/* drop back to the screen */
			pfgUnbusy(pfgShell(os_dialog));
			return;
		}
	}
}
T]);

  /***************** osForm : XmForm *****************/
  n = 0;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNtopWidget, widget_array[WI_PANELHELPTEXT]); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNbottomWidget, widget_array[WI_MESSAGEBOX]); n++;
  XtSetValues(widget_array[WI_OSFORM], args, n);


  /***************** osLabelForm : XmForm *****************/
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_NONE); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_NONE); n++;
  XtSetArg(args[n], XmNleftPosition, 1); n++;
  XtSetArg(args[n], XmNrightPosition, 5); n++;
  XtSetValues(widget_array[WI_OSLABELFORM], args, n);


  /***************** osVersionLabel : XmLabel *****************/
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNrightPosition, 1); n++;
  XtSetValues(widget_array[WI_OSVERSIONLABEL], args, n);

  XtManageChild(widget_array[WI_OSVERSIONLABEL]);

  /***************** osSliceLabel : XmLabel *****************/
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_NONE); n++;
  XtSetArg(args[n], XmNtopOffset, 0); n++;
  XtSetArg(args[n], XmNleftOffset, 0); n++;
  XtSetArg(args[n], XmNleftPosition, 1); n++;
  XtSetValues(widget_array[WI_OSSLICELABEL], args, n);

  XtManageChild(widget_array[WI_OSSLICELABEL]);
  XtManageChild(widget_array[WI_OSLABELFORM]);

  /***************** osScrolledWindow : XmScrolledWindow *****************/
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION); n++;
  XtSetArg(args[n], XmNleftPosition, 1); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNtopWidget, widget_array[WI_OSLABELFORM]); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNrightPosition, 5); n++;
  XtSetValues(widget_array[WI_OSSCROLLEDWINDOW], args, n);

  XtManageChild(widget_array[WI_OSSLICESFORM]);
  XtManageChild(widget_array[WI_OSSCROLLEDWINDOW]);
  XtManageChild(widget_array[WI_OSFORM]);

  /***************** messageBox : XmMessageBox *****************/
  n = 0;
  XtSetArg(args[n], XmNdefaultButton, widget_array[WI_CONTINUEBUTTON]); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_MESSAGEBOX], args, n);

  XtManageChild(widget_array[WI_CONTINUEBUTTON]);
  XtAddCallback(widget_array[WI_GOBACKBUTTON],
                XmNactivateCallback,
                (XtCallbackProc)osGobackCB,
                (XtPointer)NULL);

  XtManageChild(widget_array[WI_GOBACKBUTTON]);
  XtAddCallback(widget_array[WI_EXITBUTTON],
                XmNactivateCallback,
                (XtCallbackProc)pfgExit,
                (XtPointer)NULL);

  XtManageChild(widget_array[WI_EXITBUTTON]);
  XtAddCallback(widget_array[WI_HELPBUTTON],
                XmNactivateCallback,
                pfgHelp,
                (XtPointer)"version.r");

  XtManageChild(widget_array[WI_HELPBUTTON]);
  XtManageChild(widget_array[WI_MESSAGEBOX]);

  /*
   *  Allocate memory for the widget array to return
   */
  if (warr_ret != NULL) {
    *warr_ret = (Widget *) malloc(sizeof(Widget)*15);
    (void) memcpy((char *)*warr_ret,
                  (char *)widget_array,
           sizeof(Widget)*15);
  }

  /*
   *   Fix for SMI's X/NeWS server
   */
  tu_ol_fix_hierarchy(widget_array[WI_OS_DIALOG]);

  /*
   *   Invoke the create callbacks
   */
  tu_widcre_invoke_hooks(widget_array[WI_OS_DIALOG]);

  /*
   *   Return the first created widget.
   */
  return widget_array[WI_OS_DIALOG];
}


/**************************************************************
 * tu_osSliceForm_widget:
 *   XmForm is a container widget with no input semantics. Constraints are placed on its children to define attachments for each of the child's four sides. These attachments can be to the XmForm, to another child, to a relative position or to the initial position of the child. The attachments determine the layout behavior of XmForm when resizing occurs.
 **************************************************************/
Widget tu_osSliceForm_widget(char    * name,
                             Widget    parent,
                             Widget ** warr_ret)
{
  Arg args[19];
  Widget widget_array[4];
  int n;
  /* Make sure the classes used are initialized */
  class_init();

  /***************** object of type : XmForm *****************/
  n = 0;
  XtSetArg(args[n], XmNrubberPositioning, False); n++;
  XtSetArg(args[n], XmNresizePolicy, XmRESIZE_ANY); n++;
  XtSetArg(args[n], XmNfractionBase, 2); n++;
  widget_array[WI_OSSLICEFORM] =
    XmCreateForm(parent, name, args, n);

  /***************** osSliceToggle : XmToggleButton *****************/
  n = 0;
  XtSetArg(args[n], XmNalignment, XmALIGNMENT_BEGINNING); n++;
  XtSetArg(args[n], XmNresizable, True); n++;
  widget_array[WI_OSSLICETOGGLE] =
    XmCreateToggleButton(widget_array[WI_OSSLICEFORM], "osSliceToggle", args, n);

  /***************** osSliceNameLabel : XmLabel *****************/
  n = 0;
  XtSetArg(args[n], XmNmarginWidth, 0); n++;
  XtSetArg(args[n], XmNmarginHeight, 0); n++;
  XtSetArg(args[n], XmNresizable, True); n++;
  widget_array[WI_OSSLICENAMELABEL] =
    XmCreateLabel(widget_array[WI_OSSLICEFORM], "osSliceNameLabel", args, n);

  /* Terminate the widget array */
  widget_array[3] = NULL;


  /***************** osSliceToggle : XmToggleButton *****************/
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_NONE); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_OSSLICETOGGLE], args, n);

  XtManageChild(widget_array[WI_OSSLICETOGGLE]);

  /***************** osSliceNameLabel : XmLabel *****************/
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftOffset, 55); n++;
  XtSetArg(args[n], XmNleftWidget, widget_array[WI_OSSLICETOGGLE]); n++;
  XtSetValues(widget_array[WI_OSSLICENAMELABEL], args, n);

  XtManageChild(widget_array[WI_OSSLICENAMELABEL]);
  XtManageChild(widget_array[WI_OSSLICEFORM]);

  /*
   *  Allocate memory for the widget array to return
   */
  if (warr_ret != NULL) {
    *warr_ret = (Widget *) malloc(sizeof(Widget)*4);
    (void) memcpy((char *)*warr_ret,
                  (char *)widget_array,
           sizeof(Widget)*4);
  }

  /*
   *   Fix for SMI's X/NeWS server
   */
  tu_ol_fix_hierarchy(widget_array[WI_OSSLICEFORM]);

  /*
   *   Invoke the create callbacks
   */
  tu_widcre_invoke_hooks(widget_array[WI_OSSLICEFORM]);

  /*
   *   Return the first created widget.
   */
  return widget_array[WI_OSSLICEFORM];
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
  if (strcmp(temp, "os_dialog") == 0){
    w = tu_os_dialog_widget(name, parent, (Widget **)retval);
  }
  else if (strcmp(temp, "osSliceForm") == 0){
    w = tu_osSliceForm_widget(name, parent, (Widget **)retval);
  }

  sDisplay = NULL;
  sScreen = NULL;
  return w;
}

