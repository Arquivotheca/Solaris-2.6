#ifndef lint
#pragma ident "@(#)pfgmeta.c 1.49 96/09/27 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	pfgmeta.c
 * Group:	installtool
 * Description:
 */

#include "pf.h"
#include "pfg.h"

#include "pfgMeta_ui.h"

static int pfgGetMetaSize(Module * mod);
static void customizeCB(Widget, XtPointer, XtPointer);
static void continueCB(Widget, XtPointer, XtPointer);
static void radioCB(Widget, XtPointer, XmToggleButtonCallbackStruct *);
static Widget createRadioButtons(Widget parent);
static XmString createRadioButtonLabel(Module *module);

/* flag that indicates if software library should be (re)initialized */
static int InitializeSW = True;

/* max field width for printing the metacluster names */
#define	SW_META_MAX_NAME_LEN	53

/* max field width for printing the metacluster sizes */
#define	SW_META_MAX_SIZE_LEN	5

static Widget meta_dialog;

Widget
pfgCreateSw(void)
{
	Widget radioBox;
	WidgetList widget_list;
	Dimension radioWidth;

	meta_dialog = tu_meta_dialog_widget("meta_dialog",
		pfgTopLevel, &widget_list);

	XmAddWMProtocolCallback(pfgShell(meta_dialog), pfgWMDeleteAtom,
	    (XtCallbackProc) pfgExit, NULL);

	XtVaSetValues(pfgShell(meta_dialog),
	    XmNtitle, TITLE_SW,
	    XmNdeleteResponse, XmDO_NOTHING,
	    NULL);
	xm_SetNoResize(pfgTopLevel, meta_dialog);

	pfgSetWidgetString(widget_list, "panelhelpText", MSG_SW);
	pfgSetWidgetString(widget_list, "continueButton", PFG_CONTINUE);
	pfgSetWidgetString(widget_list, "gobackButton", PFG_GOBACK);
	pfgSetWidgetString(widget_list, "customizeButton", PFG_CUSTOMIZE);
	pfgSetWidgetString(widget_list, "exitButton", PFG_EXIT);
	pfgSetWidgetString(widget_list, "helpButton", PFG_HELP);

	pfgSetWidgetString(widget_list, "softwareGroupLabel", PFG_MT_BASE);
	pfgSetWidgetString(widget_list, "recommendedSizeLabel", PFG_MT_SIZE);

	(void) createRadioButtons(pfgGetNamedWidget(widget_list,
		"metaRadioBox"));

	radioBox = pfgGetNamedWidget(widget_list, "metaRadioBox");

	XtVaGetValues(radioBox,
		XmNwidth, &radioWidth,
		NULL);

	write_debug(GUI_DEBUG_L1, "radio width = %d", radioWidth);

	XtVaSetValues(pfgGetNamedWidget(widget_list, "metaScrolledWindow"),
	    XmNwidth, (Dimension) (radioWidth + 30),
	    NULL);

	XtAddCallback(pfgGetNamedWidget(widget_list, "customizeButton"),
		XmNactivateCallback, customizeCB, radioBox);
	XtAddCallback(pfgGetNamedWidget(widget_list, "continueButton"),
		XmNactivateCallback, continueCB, radioBox);

	XtManageChild(meta_dialog);

	(void) XmProcessTraversal(
		pfgGetNamedWidget(widget_list, "continueButton"),
		XmTRAVERSE_CURRENT);

	free(widget_list);
	pfgUnbusy(pfgShell(meta_dialog));

	return (meta_dialog);
}

/*
 * Function: radioButton
 * Input: w - radio button widget who's value changed radioBox - radioBox
 * widget state - state information of the radio button Output: radioBox -
 * the XmNuserData field is set to point to the currently selected meta
 * cluster Return: NONE Description: This function is the callback for the
 * radio buttons.  The module representing the selected meta cluster is
 * stored in the radioBox XmNuserData field.
 */

/* ARGSUSED */
void
radioCB(Widget radioButton, XtPointer radioBox,
    XmToggleButtonCallbackStruct * state)
{
	Module *module;
	Boolean changed = False;
	static Widget prev_button = NULL;

	if (prev_button == radioButton)
		return; /* ignore */

	if (!state->set) {
		/*
		 * save widget id of previous radio button, so we can
		 * reset it if the user wants to go back to the previous
		 * metacluster
		 */
		prev_button = radioButton;
	} else {
		/*
		 * if the metacluster has been edited, query for confirmation
		 * that the edits will be lost if changing metaclusters.
		 */
		if (pfGetClusterList() != NULL || pfGetPackageList() != NULL)
			changed = True;

		if (changed) {
			if (!pfgAppQuery(meta_dialog,
					MSG_BASE_CHOICE_OK_CHANGE)) {
				XtVaSetValues(radioBox,
					XmNmenuHistory, prev_button,
					NULL);
				XmToggleButtonSetState(radioButton,
					False, False);
				XmToggleButtonSetState(prev_button,
					True, False);
				XmProcessTraversal(prev_button,
					XmTRAVERSE_CURRENT);
				return;
			}
		}

		prev_button = NULL;

		XtVaGetValues(radioButton,
			XmNuserData, &module,
			NULL);

		XtVaSetValues(radioBox,
			XmNuserData, module,
			NULL);

		/* need to reinitialize sw lib since meta cluster changed */
		InitializeSW = True;
		(void) pfInitializeSw();
	}
}

/*
 * Function: continueCB
 * Input: w - widget id of the continue button radioBox - radio box widget
 * containing the selected meta cluster module call_data - unused Output:
 * NONE Return: NONE Description: This function is the callback function for
 * the continue button.  The function retreives the module for the selected
 * meta cluster from the XmNuserData field of the radioBox widget.  The
 * package id of the meta cluster is then set in the profile structure by a
 * call to pfReconcile
 */

/* ARGSUSED */
void
continueCB(Widget w, XtPointer radioBox, XtPointer call_data)
{
	Module *module;

	XtVaGetValues(radioBox,
		XmNuserData, &module,
		NULL);

	if (InitializeSW == True) {
		pfSetMetaCluster(module);
		InitializeSW = False;
		pfgResetPackages();
	}
	pfgBusy(pfgShell(w));
	pfgSetAction(parAContinue);
}

/* ARGSUSED */
void
customizeCB(Widget w, XtPointer radioBox, XtPointer call_data)
{
	Module *module;

	XtVaGetValues(radioBox,
		XmNuserData, &module,
		NULL);

	if (InitializeSW == True) {
		pfSetMetaCluster(module);
		InitializeSW = False;
		pfgResetPackages();
	}

	pfgBusy(pfgShell(w));

	(void) pfgCreateSoftware(meta_dialog);
}

static Widget
createRadioButtons(Widget parent)
{
	Module *module;
	XmString label;
	Widget radioButton, retval = NULL;
	Dimension radioWidth;
	Module *currentMeta;
	int selectFlag;

	currentMeta = pfGetCurrentMeta();

	module = get_sub(get_current_product());
	while (module != NULL) {
		selectFlag = (strcmp(currentMeta->info.mod->m_pkgid,
			module->info.mod->m_pkgid) == 0) ? True : False;
		label = createRadioButtonLabel(module);
		radioButton = XtVaCreateManagedWidget(module->info.mod->m_name,
			xmToggleButtonWidgetClass, parent,
			XmNset, selectFlag,
			XmNlabelString, label,
			XmNuserData, module,
			NULL);
		XmStringFree(label);

		XtAddCallback(radioButton,
			XmNvalueChangedCallback, radioCB, parent);

		if (selectFlag) {
			retval = radioButton;	/* for setting focus */
			XtVaSetValues(parent,
				XmNuserData, module,
				NULL);
		}
		module = get_next(module);
	}

	XtVaGetValues(radioButton,
		XmNwidth, &radioWidth,
		NULL);
	XtVaSetValues(parent,
		XmNwidth, radioWidth,
		NULL);
	return (retval);
}

static XmString
createRadioButtonLabel(Module *module)
{
	char tmp[100];

	(void) sprintf(tmp, "%-*.*s %*d MB", 
		SW_META_MAX_NAME_LEN, SW_META_MAX_NAME_LEN,
		module->info.mod->m_name,
		SW_META_MAX_SIZE_LEN, pfgGetMetaSize(module));

	return (XmStringCreateLocalized(tmp));
}


/*
 * function to calculate the size of a specified meta cluster.
 */
int
pfgGetMetaSize(Module * mod)
{
	int total;

	/* mark current meta has unselected */
	mark_module(pfGetCurrentMeta(), UNSELECTED);

	mark_module(mod, SELECTED);
	total = DiskGetContentDefault();
	mark_module(mod, UNSELECTED);

	mark_module(pfGetCurrentMeta(), SELECTED);
	resetPackClustSelects();
	return (total);
}

/* ARGSUSED */
void
metaGobackCB(Widget w, XtPointer clientD, XtPointer callD)
{
	pfgBusy(pfgShell(w));
	pfgSetAction(parAGoback);
}

/*
 * Purpose:
 *	To update a meta cluster string with the proper recommended size,
 *	e.g. after hitting OK on the software customization screen, we want
 *	the new size to appear.
 */
void
pfgUpdateMetaSize(void)
{
	Module *module;
	XmString label;
	char *tmp;
	Widget radioButton;

	/*
	 * Get pointer to the meta cluster that is currently set,
	 * and make the new label with any new size information.
	 */
	module = pfGetCurrentMeta();
	label = createRadioButtonLabel(module);

	/*
	 * Get the correct radio button widget id that corresponds to
	 * the current meta cluster and change its label.
	 */
	tmp = malloc(strlen(module->info.mod->m_name) + 2);
	(void) sprintf(tmp, "*%s", module->info.mod->m_name);
	radioButton = XtNameToWidget(meta_dialog, tmp);
	free(tmp);

	/*
	 * This shouldn't happen, but let's prevent core dumps
	 * just in case...
	 */
	if (!radioButton)
		return;

	XtVaSetValues(radioButton,
		XmNlabelString, label,
		NULL);
	XmStringFree(label);
}
Arg(args[n], XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNrightWidget, widget_array[WI_METASCROLLEDWINDOW]); n++;
  XtSetValues(widget_array[WI_RECOMMENDEDSIZELABEL], args, n);

  XtManageChild(widget_array[WI_RECOMMENDEDSIZELABEL]);

  /***************** metaScrolledWindow : XmScrolledWindow *****************/
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftWidget, widget_array[WI_SOFTWAREGROUPLABEL]); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNtopWidget, widget_array[WI_SOFTWAREGROUPLABEL]); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION); n++;
  XtSetArg(args[n], XmNrightPosition, 90); n++;
  XtSetValues(widget_array[WI_METASCROLLEDWINDOW], args, n);

  XtManageChild(widget_array[WI_METARADIOBOX]);
  XtManageChild(widget_array[WI_METASCROLLEDWINDOW]);
  XtManageChild(widget_array[WI_METAFORM]);

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
                (XtCallbackProc)metaGobackCB,
                (XtPointer)NULL);

  XtManageChild(widget_array[WI_GOBACKBUTTON]);
  XtManageChild(widget_array[WI_CUSTOMIZEBUTTON]);
  XtAddCallback(widget_array[WI_EXITBUTTON],
                XmNactivateCallback,
                (XtCallbackProc)pfgExit,
                (XtPointer)NULL);

  XtManageChild(widget_array[WI_EXITBUTTON]);
  XtAddCallback(widget_array[WI_HELPBUTTON],
                XmNactivateCallback,
                pfgHelp,
                (XtPointer)"softgroup.t");

  XtManageChild(widget_array[WI_HELPBUTTON]);
  XtManageChild(widget_array[WI_MESSAGEBOX]);

  /*
   *  Allocate memory for the widget array to return
   */
  if (warr_ret != NULL) {
    *warr_ret = (Widget *) malloc(sizeof(Widget)*14);
    (void) memcpy((char *)*warr_ret,
                  (char *)widget_array,
           sizeof(Widget)*14);
  }

  /*
   *   Fix for SMI's X/NeWS server
   */
  tu_ol_fix_hierarchy(widget_array[WI_META_DIALOG]);

  /*
   *   Invoke the create callbacks
   */
  tu_widcre_invoke_hooks(widget_array[WI_META_DIALOG]);

  /*
   *   Return the first created widget.
   */
  return widget_array[WI_META_DIALOG];
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
  if (strcmp(temp, "meta_dialog") == 0){
    w = tu_meta_dialog_widget(name, parent, (Widget **)retval);
  }

  sDisplay = NULL;
  sScreen = NULL;
  return w;
}

