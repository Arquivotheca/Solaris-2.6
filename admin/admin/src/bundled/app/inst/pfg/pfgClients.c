#ifndef lint
#pragma ident "@(#)pfgclients.c 1.34 96/07/26 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	pfgclients.c
 * Group:	installtool
 * Description:
 */

#include "pf.h"
#include "pfg.h"

#include "pfgClients_ui.h"

static void disklessContinueCB(Widget, XtPointer, XtPointer);
static void disklessToggleCB(Widget, XtPointer, XtPointer);

#define	TEXTLENGTH 5

Widget
pfgCreateClients(void)
{
	Widget diskless_dialog;

	Arch *arch, *ptr;
	char tmp[TEXTLENGTH];
	char *nativeArch;
	Dimension width, height;
	Widget checkBox, toggleButton;
	XmString toggleString;

	WidgetList widget_list;

	diskless_dialog = tu_diskless_dialog_widget("diskless_dialog",
		pfgTopLevel, &widget_list);


	XmAddWMProtocolCallback(pfgShell(diskless_dialog), pfgWMDeleteAtom,
	    (XtCallbackProc) pfgExit, NULL);

	XtVaSetValues(pfgShell(diskless_dialog),
	    XmNtitle, TITLE_CLIENTS,
	    XmNdeleteResponse, XmDO_NOTHING,
	    NULL);

	pfgSetWidgetString(widget_list, "panelhelpText", MSG_CLIENTS);
	pfgSetWidgetString(widget_list, "continueButton", PFG_CONTINUE);
	pfgSetWidgetString(widget_list, "gobackButton", PFG_GOBACK);
	pfgSetWidgetString(widget_list, "exitButton", PFG_EXIT);
	pfgSetWidgetString(widget_list, "helpButton", PFG_HELP);

	pfgSetWidgetString(widget_list, "clientNumberLabel", PFG_CL_CLIENTS);
	(void) sprintf(tmp, "%d", getNumClients());
	pfgSetWidgetString(widget_list, "clientNumberText", tmp);

	pfgSetWidgetString(widget_list, "clientSwapLabel", PFG_CL_SWAP);
	(void) sprintf(tmp, "%d", getSwapPerClient());
	pfgSetWidgetString(widget_list, "clientSwapText", tmp);

	pfgSetWidgetString(widget_list, "architectureLabel", PFG_CL_ARCH);

	checkBox = pfgGetNamedWidget(widget_list, "architectureRowColumn");

	arch = get_all_arches(NULL);
	nativeArch = get_default_arch();
	for (ptr = arch; ptr != NULL; ptr = ptr->a_next) {
		toggleString = XmStringCreateLocalized(ptr->a_arch);
		toggleButton = XtVaCreateManagedWidget("Toggle",
			xmToggleButtonWidgetClass, checkBox,
			XmNlabelString, toggleString,
			XmNset, ptr->a_selected == TRUE ?
				True : False,
			XmNuserData, ptr,
			XmNsensitive, strcmp(ptr->a_arch, nativeArch) ?
				True : False,
			NULL);
		XtAddCallback(toggleButton, XmNvalueChangedCallback,
			disklessToggleCB, NULL);
	}

	XtAddCallback(pfgGetNamedWidget(widget_list, "continueButton"),
		XmNactivateCallback, disklessContinueCB, checkBox);

	XtManageChild(diskless_dialog);
	(void) XmProcessTraversal(
		pfgGetNamedWidget(widget_list, "continueButton"),
		XmTRAVERSE_CURRENT);

	XtVaGetValues(pfgShell(diskless_dialog),
	    XmNwidth, &width,
	    XmNheight, &height,
	    NULL);

	XtVaSetValues(pfgShell(diskless_dialog),
	    XmNminWidth, width,
	    XmNmaxWidth, width,
	    XmNminHeight, height,
	    NULL);

	free(widget_list);
	pfgUnbusy(pfgShell(diskless_dialog));

	return (diskless_dialog);
}

/* ARGSUSED */
void
setSwapCB(Widget swap, XtPointer clientD, XtPointer callD)
{
	char *string;
	int swapSize;

	string = XmTextFieldGetString(swap);

	if (string[0] == '\0') {
		swapSize = 0;
	} else {
		swapSize = atoi(string);
	}
	setSwapPerClient(swapSize);
}

/* ARGSUSED */
void
setNumClientsCB(Widget numClients, XtPointer clientD, XtPointer callD)
{
	char *string;
	int numClientsSize;

	string = XmTextFieldGetString(numClients);
	if (string[0] == '\0') {
		numClientsSize = 0;
	} else {
		numClientsSize = atoi(string);
	}
	setNumClients(numClientsSize);
}

/* ARGSUSED */
static void
disklessToggleCB(Widget toggle, XtPointer client_data,
    XtPointer callD)
{
	XmToggleButtonCallbackStruct *cbs =
		/* LINTED [pointer cast] */
		(XmToggleButtonCallbackStruct *) callD;

	Module *product = get_current_product();
	Arch *arch;

	XtVaGetValues(toggle,
		XmNuserData, &arch,
		NULL);
	if (arch) {
		if (cbs->set == True) {
			select_arch(product, arch->a_arch);
		} else {
			deselect_arch(product, arch->a_arch);
		}
		mark_arch(product);
	}
}



/* ARGSUSED */
static void
disklessContinueCB(Widget w, XtPointer checkBox, XtPointer callD)
{
	int numChildren;
	WidgetList children;
	Module *product;

	XtVaGetValues(checkBox,
		XmNnumChildren, &numChildren,
		XmNchildren, &children,
		NULL);
	product = get_current_product();
	if (product == NULL) {
		(void) fprintf(stderr,
			"pfgClient:disklessContinueCB: Unable to determine product to install");
		pfgCleanExit(EXIT_INSTALL_FAILURE, (void *) NULL);
	}
	mark_arch(product);
	pfgSetAction(parAContinue);
	pfgBusy(pfgShell(w));
}

/* ARGSUSED */
void
disklessGobackCB(Widget w, XtPointer clientD, XtPointer callD)
{
	pfgSetAction(parAGoback);
}
; n++;
  XtSetArg(args[n], XmNtraversalOn, False); n++;
  XtSetArg(args[n], XmNresizeHeight, True); n++;
  XtSetArg(args[n], XmNwordWrap, True); n++;
  XtSetArg(args[n], XmNshadowThickness, 0); n++;
  XtSetArg(args[n], XmNmarginWidth, 18); n++;
  XtSetArg(args[n], XmNmarginHeight, 18); n++;
  XtSetArg(args[n], XmNvalue, "message_text"); n++;
  widget_array[WI_PANELHELPTEXT] =
    XmCreateText(widget_array[WI_DISKLESS_DIALOG], "panelhelpText", args, n);

  /***************** diskless_form : XmForm *****************/
  n = 0;
  XtSetArg(args[n], XmNfractionBase, 5); n++;
  widget_array[WI_DISKLESS_FORM] =
    XmCreateForm(widget_array[WI_DISKLESS_DIALOG], "diskless_form", args, n);

  /***************** clientNumberLabelText : XmForm *****************/
  /* This field has been moved to file pfgInstallClients.pcd, it has been unmanaged on this screen. */
  widget_array[WI_CLIENTNUMBERLABELTEXT] =
    XmCreateForm(widget_array[WI_DISKLESS_FORM], "clientNumberLabelText", NULL, 0);

  /***************** clientNumberLabel : XmLabel *****************/
  n = 0;
  XtSetArg(args[n], XmNalignment, XmALIGNMENT_END); n++;
  widget_array[WI_CLIENTNUMBERLABEL] =
    XmCreateLabel(widget_array[WI_CLIENTNUMBERLABELTEXT], "clientNumberLabel", args, n);

  /***************** clientNumberText : XmTextField *****************/
  n = 0;
  XtSetArg(args[n], XmNcolumns, 3); n++;
  XtSetArg(args[n], XmNmaxLength, 3); n++;
  widget_array[WI_CLIENTNUMBERTEXT] =
    XmCreateTextField(widget_array[WI_CLIENTNUMBERLABELTEXT], "clientNumberText", args, n);

  /***************** clientSwapLabelText : XmForm *****************/
  /* This field has been moved to pfgInstallClients.pcd, it has */
  /* been unmanaged on this screen. */
  widget_array[WI_CLIENTSWAPLABELTEXT] =
    XmCreateForm(widget_array[WI_DISKLESS_FORM], "clientSwapLabelText", NULL, 0);

  /***************** clientSwapLabel : XmLabel *****************/
  n = 0;
  XtSetArg(args[n], XmNalignment, XmALIGNMENT_END); n++;
  widget_array[WI_CLIENTSWAPLABEL] =
    XmCreateLabel(widget_array[WI_CLIENTSWAPLABELTEXT], "clientSwapLabel", args, n);

  /***************** clientSwapText : XmTextField *****************/
  n = 0;
  XtSetArg(args[n], XmNmaxLength, 3); n++;
  XtSetArg(args[n], XmNcolumns, 3); n++;
  widget_array[WI_CLIENTSWAPTEXT] =
    XmCreateTextField(widget_array[WI_CLIENTSWAPLABELTEXT], "clientSwapText", args, n);

  /***************** architectureLabel : XmLabel *****************/
  n = 0;
  XtSetArg(args[n], XmNresizable, True); n++;
  widget_array[WI_ARCHITECTURELABEL] =
    XmCreateLabel(widget_array[WI_DISKLESS_FORM], "architectureLabel", args, n);

  /***************** architectureScrolledWindow : XmScrolledWindow *****************/
  n = 0;
  XtSetArg(args[n], XmNscrollingPolicy, XmAUTOMATIC); n++;
  widget_array[WI_ARCHITECTURESCROLLEDWINDOW] =
    XmCreateScrolledWindow(widget_array[WI_DISKLESS_FORM], "architectureScrolledWindow", args, n);

  /***************** architectureRowColumn : XmRowColumn *****************/
  widget_array[WI_ARCHITECTUREROWCOLUMN] =
    XmCreateRowColumn(widget_array[WI_ARCHITECTURESCROLLEDWINDOW], "architectureRowColumn", NULL, 0);

  /***************** messageBox : XmMessageBox *****************/
  n = 0;
  XtSetArg(args[n], XmNdialogType, XmDIALOG_TEMPLATE); n++;
  widget_array[WI_MESSAGEBOX] =
    XmCreateMessageBox(widget_array[WI_DISKLESS_DIALOG], "messageBox", args, n);

  /***************** continueButton : XmPushButton *****************/
  widget_array[WI_CONTINUEBUTTON] =
    XmCreatePushButton(widget_array[WI_MESSAGEBOX], "continueButton", NULL, 0);

  /***************** gobackButton : XmPushButton *****************/
  widget_array[WI_GOBACKBUTTON] =
    XmCreatePushButton(widget_array[WI_MESSAGEBOX], "gobackButton", NULL, 0);

  /***************** exitButton : XmPushButton *****************/
  widget_array[WI_EXITBUTTON] =
    XmCreatePushButton(widget_array[WI_MESSAGEBOX], "exitButton", NULL, 0);

  /***************** helpButton : XmPushButton *****************/
  widget_array[WI_HELPBUTTON] =
    XmCreatePushButton(widget_array[WI_MESSAGEBOX], "helpButton", NULL, 0);

  /***************** button5 : XmPushButton *****************/
  widget_array[WI_BUTTON5] =
    XmCreatePushButton(widget_array[WI_MESSAGEBOX], "button5", NULL, 0);

  /* Terminate the widget array */
  widget_array[18] = NULL;


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

  /***************** diskless_form : XmForm *****************/
  n = 0;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNtopWidget, widget_array[WI_PANELHELPTEXT]); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNbottomWidget, widget_array[WI_MESSAGEBOX]); n++;
  XtSetValues(widget_array[WI_DISKLESS_FORM], args, n);


  /***************** clientNumberLabelText : XmForm *****************/
  n = 0;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION); n++;
  XtSetArg(args[n], XmNrightPosition, 4); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_CLIENTNUMBERLABELTEXT], args, n);


  /***************** clientNumberLabel : XmLabel *****************/
  n = 0;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNrightWidget, widget_array[WI_CLIENTNUMBERTEXT]); n++;
  XtSetArg(args[n], XmNrightOffset, 5); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
  XtSetArg(args[n], XmNbottomWidget, widget_array[WI_CLIENTNUMBERTEXT]); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
  XtSetArg(args[n], XmNtopWidget, widget_array[WI_CLIENTNUMBERTEXT]); n++;
  XtSetValues(widget_array[WI_CLIENTNUMBERLABEL], args, n);

  XtManageChild(widget_array[WI_CLIENTNUMBERLABEL]);

  /***************** clientNumberText : XmTextField *****************/
  n = 0;
  XtAddCallback(widget_array[WI_CLIENTNUMBERTEXT],
                XmNactivateCallback,
                (XtCallbackProc)setNumClientsCB,
                (XtPointer)NULL);

  XtAddCallback(widget_array[WI_CLIENTNUMBERTEXT],
                XmNlosingFocusCallback,
                (XtCallbackProc)setNumClientsCB,
                (XtPointer)NULL);

  XtAddCallback(widget_array[WI_CLIENTNUMBERTEXT],
                XmNmodifyVerifyCallback,
                (XtCallbackProc)sizeVerifyCB,
                (XtPointer)NULL);

  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_CLIENTNUMBERTEXT], args, n);

  XtManageChild(widget_array[WI_CLIENTNUMBERTEXT]);

  /***************** clientSwapLabelText : XmForm *****************/
  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNtopWidget, widget_array[WI_CLIENTNUMBERLABELTEXT]); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
  XtSetArg(args[n], XmNrightWidget, widget_array[WI_CLIENTNUMBERLABELTEXT]); n++;
  XtSetValues(widget_array[WI_CLIENTSWAPLABELTEXT], args, n);


  /***************** clientSwapLabel : XmLabel *****************/
  n = 0;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNrightWidget, widget_array[WI_CLIENTSWAPTEXT]); n++;
  XtSetArg(args[n], XmNrightOffset, 5); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
  XtSetArg(args[n], XmNbottomWidget, widget_array[WI_CLIENTSWAPTEXT]); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
  XtSetArg(args[n], XmNtopWidget, widget_array[WI_CLIENTSWAPTEXT]); n++;
  XtSetValues(widget_array[WI_CLIENTSWAPLABEL], args, n);

  XtManageChild(widget_array[WI_CLIENTSWAPLABEL]);

  /***************** clientSwapText : XmTextField *****************/
  n = 0;
  XtAddCallback(widget_array[WI_CLIENTSWAPTEXT],
                XmNmodifyVerifyCallback,
                (XtCallbackProc)sizeVerifyCB,
                (XtPointer)NULL);

  XtAddCallback(widget_array[WI_CLIENTSWAPTEXT],
                XmNlosingFocusCallback,
                (XtCallbackProc)setSwapCB,
                (XtPointer)NULL);

  XtAddCallback(widget_array[WI_CLIENTSWAPTEXT],
                XmNactivateCallback,
                (XtCallbackProc)setSwapCB,
                (XtPointer)NULL);

  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_CLIENTSWAPTEXT], args, n);

  XtManageChild(widget_array[WI_CLIENTSWAPTEXT]);

  /***************** architectureLabel : XmLabel *****************/
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION); n++;
  XtSetArg(args[n], XmNleftPosition, 1); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopOffset, 5); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_NONE); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_NONE); n++;
  XtSetValues(widget_array[WI_ARCHITECTURELABEL], args, n);

  XtManageChild(widget_array[WI_ARCHITECTURELABEL]);

  /***************** architectureScrolledWindow : XmScrolledWindow *****************/
  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNtopWidget, widget_array[WI_ARCHITECTURELABEL]); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION); n++;
  XtSetArg(args[n], XmNleftPosition, 1); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION); n++;
  XtSetArg(args[n], XmNrightPosition, 4); n++;
  XtSetValues(widget_array[WI_ARCHITECTURESCROLLEDWINDOW], args, n);

  XtManageChild(widget_array[WI_ARCHITECTUREROWCOLUMN]);
  XtManageChild(widget_array[WI_ARCHITECTURESCROLLEDWINDOW]);
  XtManageChild(widget_array[WI_DISKLESS_FORM]);

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
                (XtCallbackProc)disklessGobackCB,
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
                (XtPointer)"platform.h");

  XtManageChild(widget_array[WI_HELPBUTTON]);
  XtManageChild(widget_array[WI_MESSAGEBOX]);

  /*
   *  Allocate memory for the widget array to return
   */
  if (warr_ret != NULL) {
    *warr_ret = (Widget *) malloc(sizeof(Widget)*19);
    (void) memcpy((char *)*warr_ret,
                  (char *)widget_array,
           sizeof(Widget)*19);
  }

  /*
   *   Fix for SMI's X/NeWS server
   */
  tu_ol_fix_hierarchy(widget_array[WI_DISKLESS_DIALOG]);

  /*
   *   Invoke the create callbacks
   */
  tu_widcre_invoke_hooks(widget_array[WI_DISKLESS_DIALOG]);

  /*
   *   Return the first created widget.
   */
  return widget_array[WI_DISKLESS_DIALOG];
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
  if (strcmp(temp, "diskless_dialog") == 0){
    w = tu_diskless_dialog_widget(name, parent, (Widget **)retval);
  }

  sDisplay = NULL;
  sScreen = NULL;
  return w;
}

