#ifndef lint
#pragma ident "@(#)pfgautolayout.c 1.38 96/09/17 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	pfgautolayout.c
 * Group:	installtool
 * Description:
 */

#include "pf.h"
#include "pfg.h"

#include "pfgAutolayout_ui.h"

static void autolayoutCancelCB(Widget, XtPointer, XtPointer);
static void autolayoutOkCB(Widget, XtPointer, XtPointer);
static void checkCB(Widget, XtPointer, XtPointer);
static void createToggleButtons(Widget parent, Defmnt_t **);

/*
 * flag that indicates if software library should be
 * initialized/reinitialized
 */
static Widget autolayout_dialog;

Widget
pfgCreateAutoLayout(Widget parent)
{
	Dimension radioWidth;
	Defmnt_t **MountList;
	Defmnt_t **origMountList; /* used to restore if user cancels screen */
	WidgetList widget_list;
	Widget checkBox;

	autolayout_dialog = tu_autolayout_dialog_widget("autolayout_dialog",
		parent, &widget_list);

	XmAddWMProtocolCallback(pfgShell(autolayout_dialog), pfgWMDeleteAtom,
	    (XtCallbackProc) pfgExit, NULL);

	XtVaSetValues(pfgShell(autolayout_dialog),
	    XmNtitle, TITLE_AUTOLAYOUT,
	    XmNdeleteResponse, XmDO_NOTHING,
	    NULL);
	xm_SetNoResize(pfgTopLevel, autolayout_dialog);

	origMountList = get_dfltmnt_list(NULL);

	/*
	 * reset the mount list to the default for the machine type
	 * selected.  Set preserved slice to SELECTED
	 */
	pfgResetDefaults();
	MountList = get_dfltmnt_list(NULL);

	checkBox = pfgGetNamedWidget(widget_list, "autolayoutCheckBox");

	createToggleButtons(checkBox, MountList);

	XtVaGetValues(checkBox,
	    XmNwidth, &radioWidth,
	    NULL);

	write_debug(GUI_DEBUG_L1, "radio width = %d", radioWidth);

	pfgSetWidgetString(widget_list, "panelhelpText", MSG_AUTOLAYOUT);
	pfgSetWidgetString(widget_list, "createLabel", PFG_SY_HEADING);
	pfgSetWidgetString(widget_list, "continueButton", PFG_CONTINUE);
	pfgSetWidgetString(widget_list, "cancelButton", PFG_CANCEL);
	pfgSetWidgetString(widget_list, "helpButton", PFG_HELP);

	XtAddCallback(
		pfgGetNamedWidget(widget_list, "continueButton"),
		XmNactivateCallback, autolayoutOkCB, checkBox);
	XtAddCallback(
		pfgGetNamedWidget(widget_list, "cancelButton"),
		XmNactivateCallback, autolayoutCancelCB, origMountList);


	XtManageChild(autolayout_dialog);

	(void) XmProcessTraversal(
		pfgGetNamedWidget(widget_list, "continueButton"),
		XmTRAVERSE_CURRENT);

	free(widget_list);
	return (autolayout_dialog);
}


/*
 * Function: toggleButton
 *
 * Input:
 *   w - radio button widget who's value changed
 *   checkBox - checkBox widget
 *   state - state information of the radio button
 * Output:
 *   checkBox - the XmNuserData field is set to point to the
 *	currently selected meta cluster
 * Return:
 *  NONE
 * Description:
 *  This function is the callback for the radio buttons.  The module
 *  representing the selected meta cluster is stored in the checkBox
 *  XmNuserData field.
 */

/* ARGSUSED */
void
checkCB(Widget toggleButton, XtPointer clientD, XtPointer callD)
{
	XmToggleButtonCallbackStruct *state =
		/* LINTED [pointer cast] */
		(XmToggleButtonCallbackStruct *) callD;

	char *name;
	Defmnt_t mountList;
	int err;

	XtVaGetValues(toggleButton,
	    XmNuserData, &name,
	    NULL);

	if (name != NULL) {
		if (state->set) {
			get_dfltmnt_ent(&mountList, name);
			mountList.status = DFLT_SELECT;
			err = set_dfltmnt_ent(&mountList, name);
			if (err != D_OK) {
				pfgWarning(autolayout_dialog,
					pfErUNSUPPORTEDFS);
			}
		} else {
			get_dfltmnt_ent(&mountList, name);
			if (mountList.status == DFLT_SELECT &&
			    mountList.allowed == 0) {
				pfgWarning(autolayout_dialog, pfErREQUIREDFS);
				return;
			}
			if (name[0] == '/') {
				mountList.status = DFLT_DONTCARE;
			} else {
				mountList.status = DFLT_IGNORE;
			}
			err = set_dfltmnt_ent(&mountList, name);
			if (err != D_OK) {
				pfgWarning(autolayout_dialog, pfErREQUIREDFS);
			}
		}
	}
}

/* ARGSUSED */
void
autolayoutOkCB(Widget w, XtPointer checkBox, XtPointer callD)
{
	int err;

	pfgBusy(pfgShell(w));

	pfgNullDisks();

	/* sets up default partitioning of disk */
	err = pfgInitializeDisks();
	if (err != D_OK) {
		/* error performing auto layout */
		if (pfgQuery(autolayout_dialog, pfQAUTOFAIL) == False) {
			pfgUnbusy(pfgShell(w));
			return;
		} else {
			pfgNullDisks();
			pfgSetManualDefaultMounts();
		}
	}
	pfgBuildLayoutArray();
	XtUnmanageChild(pfgShell(w));
	XtDestroyWidget(pfgShell(w));
	pfgSetAction(parAContinue);
}


/* ARGSUSED */
void
autolayoutCancelCB(Widget w, XtPointer origMountList, XtPointer callD)
{
	int err;

	pfgUnbusy(pfgShell(XtParent(pfgShell(w))));

	XtUnmanageChild(pfgShell(w));
	XtDestroyWidget(pfgShell(w));
	/* LINTED [pointer cast] */
	err = set_dfltmnt_list((Defmnt_t **) origMountList);
	if (err != D_OK) {
		pfgDiskError(w, "set_dfltmnt_list", err);
	}
}


void
createToggleButtons(Widget parent, Defmnt_t ** mountList)
{
	XmString label;
	Widget toggleButton;
	char *name;
	Dimension radioWidth;
	int i;
	int sensitivity;

	for (i = 0; mountList[i] != NULL; i++) {
		if (mountList[i]->status == DFLT_IGNORE &&
		    mountList[i]->allowed == 0) {
			continue;
		}

		label = XmStringCreateLocalized(mountList[i]->name);
		name = xstrdup(mountList[i]->name);
		sensitivity = mountList[i]->status == DFLT_SELECT &&
			mountList[i]->allowed == 0 ? False : True;
		toggleButton = XtVaCreateManagedWidget("autoToggleButton",
			xmToggleButtonWidgetClass, parent,
			XmNset, mountList[i]->status == DFLT_SELECT ?
				True : False,
			XmNlabelString, label,
			XmNuserData, name,
			XmNsensitive, sensitivity,
			NULL);
		XmStringFree(label);

		XtAddCallback(toggleButton, XmNvalueChangedCallback,
		    checkCB, mountList);
	}

	XtVaGetValues(toggleButton,
	    XmNwidth, &radioWidth,
	    NULL);
	XtVaSetValues(parent,
	    XmNwidth, radioWidth,
	    NULL);
}
tton(widget_array[WI_MESSAGEBOX], "button4", NULL, 0);

  /***************** button5 : XmPushButton *****************/
  widget_array[WI_BUTTON5] =
    XmCreatePushButton(widget_array[WI_MESSAGEBOX], "button5", NULL, 0);

  /* Terminate the widget array */
  widget_array[11] = NULL;


  /***************** panelhelpText : XmText *****************/
  n = 0;
  ttbl = XtParseTranslationTable("#override\n\
~Ctrl ~Meta<BtnDown>:\n\
~Ctrl ~Meta<BtnUp>:");
  XtOverrideTranslations(widget_array[WI_PANELHELPTEXT], ttbl);
  unregister_as_dropsite(widget_array[WI_PANELHELPTEXT],
                         NULL,
                         NULL);

  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_PANELHELPTEXT], args, n);

  XtManageChild(widget_array[WI_PANELHELPTEXT]);

  /***************** autolayoutForm : XmForm *****************/
  n = 0;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNtopWidget, widget_array[WI_PANELHELPTEXT]); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNbottomWidget, widget_array[WI_MESSAGEBOX]); n++;
  XtSetValues(widget_array[WI_AUTOLAYOUTFORM], args, n);


  /***************** createLabel : XmLabel *****************/
  n = 0;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION); n++;
  XtSetArg(args[n], XmNrightPosition, 4); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopOffset, 5); n++;
  XtSetValues(widget_array[WI_CREATELABEL], args, n);

  XtManageChild(widget_array[WI_CREATELABEL]);

  /***************** autolayoutCheckBox : XmRowColumn *****************/
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION); n++;
  XtSetArg(args[n], XmNleftPosition, 4); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_AUTOLAYOUTCHECKBOX], args, n);

  XtManageChild(widget_array[WI_AUTOLAYOUTCHECKBOX]);
  XtManageChild(widget_array[WI_AUTOLAYOUTFORM]);

  /***************** messageBox : XmMessageBox *****************/
  n = 0;
  XtSetArg(args[n], XmNdefaultButton, widget_array[WI_CONTINUEBUTTON]); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_MESSAGEBOX], args, n);

  XtManageChild(widget_array[WI_CONTINUEBUTTON]);
  XtManageChild(widget_array[WI_CANCELBUTTON]);
  XtAddCallback(widget_array[WI_HELPBUTTON],
                XmNactivateCallback,
                pfgHelp,
                (XtPointer)"autolayout.t");

  XtManageChild(widget_array[WI_HELPBUTTON]);
  XtManageChild(widget_array[WI_MESSAGEBOX]);

  /*
   *  Allocate memory for the widget array to return
   */
  if (warr_ret != NULL) {
    *warr_ret = (Widget *) malloc(sizeof(Widget)*12);
    (void) memcpy((char *)*warr_ret,
                  (char *)widget_array,
           sizeof(Widget)*12);
  }

  /*
   *   Fix for SMI's X/NeWS server
   */
  tu_ol_fix_hierarchy(widget_array[WI_AUTOLAYOUT_DIALOG]);

  /*
   *   Invoke the create callbacks
   */
  tu_widcre_invoke_hooks(widget_array[WI_AUTOLAYOUT_DIALOG]);

  /*
   *   Return the first created widget.
   */
  return widget_array[WI_AUTOLAYOUT_DIALOG];
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
  if (strcmp(temp, "autolayout_dialog") == 0){
    w = tu_autolayout_dialog_widget(name, parent, (Widget **)retval);
  }

  sDisplay = NULL;
  sScreen = NULL;
  return w;
}

