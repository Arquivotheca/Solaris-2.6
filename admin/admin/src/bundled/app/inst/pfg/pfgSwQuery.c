#ifndef lint
#pragma ident "@(#)pfgswquery.c 1.28 96/08/07 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	pfgswquery.c
 * Group:	installtool
 * Description:
 */

#include "pf.h"
#include "pfg.h"

#include "pfgSwQuery_ui.h"

Widget
pfgCreateSwQuery(void)
{
	Widget swquery_dialog;
	WidgetList widget_list;

	swquery_dialog = tu_swquery_dialog_widget("swquery_dialog",
		pfgTopLevel, &widget_list);

	XmAddWMProtocolCallback(pfgShell(swquery_dialog), pfgWMDeleteAtom,
	    (XtCallbackProc) pfgExit, NULL);

	XtVaSetValues(pfgShell(swquery_dialog),
	    XmNtitle, TITLE_UPG_CUSTOM_SWQUERY,
	    XmNdeleteResponse, XmDO_NOTHING,
	    NULL);
	xm_SetNoResize(pfgTopLevel, swquery_dialog);

	pfgSetWidgetString(widget_list, "panelhelpText",
		MSG_UPG_CUSTOM_SWQUERY);
	pfgSetWidgetString(widget_list, "continueButton", PFG_CONTINUE);
	pfgSetWidgetString(widget_list, "gobackButton", PFG_GOBACK);
	pfgSetWidgetString(widget_list, "customizeButton", PFG_CUSTOMIZE);
	pfgSetWidgetString(widget_list, "exitButton", PFG_EXIT);
	pfgSetWidgetString(widget_list, "helpButton", PFG_HELP);

	free(widget_list);

	XtManageChild(swquery_dialog);

	return (swquery_dialog);

}


/* ARGSUSED */
void
swQueryCustomCB(Widget w, XtPointer clientD, XtPointer callD)
{

	pfgBusy(pfgShell(w));
	(void) pfgCreateSoftware(pfgShell(w));
}


/* ARGSUSED */
void
swQueryContinueCB(Widget button, XtPointer clientD, XtPointer callD)
{
	int err;

	pfgBusy(pfgShell(button));

	/*
	 * At this point, we have to find out if there really is
	 * enough space on the system to hold all the currently seelcted
	 * software.
	 * A call to verify_fs_layout along with the parDsrAnalyze
	 * progress bar has already been made, so the assumption here is
	 * that this is a fast call, so there is no progress bar here...
	 */
	err = DsrFSAnalyzeSystem(FsSpaceInfo, NULL, NULL, NULL);
	if (err == SP_ERR_NOT_ENOUGH_SPACE) {
		/* There are failed file systems.
		 * Now that we will be entering DSR,
		 * create the slice list.
		 */
		if (DsrSLUICreate(&DsrALHandle, &DsrSLHandle, FsSpaceInfo)) {
			pfAppError(NULL,
				"Internal DSR error - can't create slice list");
			pfgCleanExit(EXIT_INSTALL_FAILURE, (void *) NULL);
		}
		if (get_trace_level() > 2) {
			DsrSLPrint(DsrSLHandle, DEBUG_LOC);
		}

		pfgSetAction(parADsrSpaceReq);
	} else {
		/* there are no failed file systems */
		pfgSetAction(parAContinue);
	}
}


/* ARGSUSED */
void
swQueryGobackCB(Widget w, XtPointer clientD, XtPointer callD)
{
	pfgBusy(pfgShell(w));
	pfgSetAction(parAGoback);
}
r);
  rv[cnt-1].next = NULL;
  return rv;
}



/****************************************************************
 * class_init:
 *     Initializes the classes used by the user interface.
 ****************************************************************/
static void class_init(void)
{
  static int inited = 0;

  if (inited != 0)
    return;

  inited = 1;
  sDisplay = sDisplay;

  XtInitializeWidgetClass(xmFormWidgetClass);
  XtInitializeWidgetClass(xmTextWidgetClass);
  XtInitializeWidgetClass(xmMessageBoxWidgetClass);
  XtInitializeWidgetClass(xmPushButtonWidgetClass);
}



/****************************************************************
 *
 *  Main C code for presentation component
 *
 ****************************************************************/

/**************************************************************
 * tu_swquery_dialog_widget:
 **************************************************************/
Widget tu_swquery_dialog_widget(char    * name,
                                Widget    parent,
                                Widget ** warr_ret)
{
  Arg args[28];
  Widget widget_array[9];
  XtTranslations ttbl;
  int n;
  /* Make sure the classes used are initialized */
  class_init();

  /***************** object of type : XmFormDialog *****************/
  widget_array[WI_SWQUERY_DIALOG] =
    XmCreateFormDialog(parent, name, NULL, 0);

  /***************** panelhelpText : XmText *****************/
  n = 0;
  XtSetArg(args[n], XmNautoShowCursorPosition, False); n++;
  XtSetArg(args[n], XmNcursorPositionVisible, False); n++;
  XtSetArg(args[n], XmNeditMode, XmMULTI_LINE_EDIT); n++;
  XtSetArg(args[n], XmNtraversalOn, False); n++;
  XtSetArg(args[n], XmNresizeHeight, True); n++;
  XtSetArg(args[n], XmNwordWrap, True); n++;
  XtSetArg(args[n], XmNshadowThickness, 0); n++;
  XtSetArg(args[n], XmNmarginWidth, 18); n++;
  XtSetArg(args[n], XmNmarginHeight, 18); n++;
  XtSetArg(args[n], XmNvalue, "message_text"); n++;
  widget_array[WI_PANELHELPTEXT] =
    XmCreateText(widget_array[WI_SWQUERY_DIALOG], "panelhelpText", args, n);

  /***************** messageBox : XmMessageBox *****************/
  n = 0;
  XtSetArg(args[n], XmNdialogType, XmDIALOG_TEMPLATE); n++;
  widget_array[WI_MESSAGEBOX] =
    XmCreateMessageBox(widget_array[WI_SWQUERY_DIALOG], "messageBox", args, n);

  /***************** continueButton : XmPushButton *****************/
  widget_array[WI_CONTINUEBUTTON] =
    XmCreatePushButton(widget_array[WI_MESSAGEBOX], "continueButton", NULL, 0);

  /***************** gobackButton : XmPushButton *****************/
  widget_array[WI_GOBACKBUTTON] =
    XmCreatePushButton(widget_array[WI_MESSAGEBOX], "gobackButton", NULL, 0);

  /***************** customizeButton : XmPushButton *****************/
  widget_array[WI_CUSTOMIZEBUTTON] =
    XmCreatePushButton(widget_array[WI_MESSAGEBOX], "customizeButton", NULL, 0);

  /***************** exitButton : XmPushButton *****************/
  widget_array[WI_EXITBUTTON] =
    XmCreatePushButton(widget_array[WI_MESSAGEBOX], "exitButton", NULL, 0);

  /***************** helpButton : XmPushButton *****************/
  widget_array[WI_HELPBUTTON] =
    XmCreatePushButton(widget_array[WI_MESSAGEBOX], "helpButton", NULL, 0);

  /* Terminate the widget array */
  widget_array[8] = NULL;


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
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNbottomWidget, widget_array[WI_MESSAGEBOX]); n++;
  XtSetValues(widget_array[WI_PANELHELPTEXT], args, n);

  XtManageChild(widget_array[WI_PANELHELPTEXT]);

  /***************** messageBox : XmMessageBox *****************/
  n = 0;
  XtSetArg(args[n], XmNdefaultButton, widget_array[WI_CONTINUEBUTTON]); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_MESSAGEBOX], args, n);

  XtAddCallback(widget_array[WI_CONTINUEBUTTON],
                XmNactivateCallback,
                (XtCallbackProc)swQueryContinueCB,
                (XtPointer)NULL);

  XtManageChild(widget_array[WI_CONTINUEBUTTON]);
  XtAddCallback(widget_array[WI_GOBACKBUTTON],
                XmNactivateCallback,
                (XtCallbackProc)swQueryGobackCB,
                (XtPointer)NULL);

  XtManageChild(widget_array[WI_GOBACKBUTTON]);
  XtAddCallback(widget_array[WI_CUSTOMIZEBUTTON],
                XmNactivateCallback,
                (XtCallbackProc)swQueryCustomCB,
                (XtPointer)NULL);

  XtManageChild(widget_array[WI_CUSTOMIZEBUTTON]);
  XtAddCallback(widget_array[WI_EXITBUTTON],
                XmNactivateCallback,
                (XtCallbackProc)pfgExit,
                (XtPointer)NULL);

  XtManageChild(widget_array[WI_EXITBUTTON]);
  XtAddCallback(widget_array[WI_HELPBUTTON],
                XmNactivateCallback,
                pfgHelp,
                (XtPointer)"spotsoftcust.r");

  XtManageChild(widget_array[WI_HELPBUTTON]);
  XtManageChild(widget_array[WI_MESSAGEBOX]);

  /*
   *  Allocate memory for the widget array to return
   */
  if (warr_ret != NULL) {
    *warr_ret = (Widget *) malloc(sizeof(Widget)*9);
    (void) memcpy((char *)*warr_ret,
                  (char *)widget_array,
           sizeof(Widget)*9);
  }

  /*
   *   Fix for SMI's X/NeWS server
   */
  tu_ol_fix_hierarchy(widget_array[WI_SWQUERY_DIALOG]);

  /*
   *   Invoke the create callbacks
   */
  tu_widcre_invoke_hooks(widget_array[WI_SWQUERY_DIALOG]);

  /*
   *   Return the first created widget.
   */
  return widget_array[WI_SWQUERY_DIALOG];
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
  if (strcmp(temp, "swquery_dialog") == 0){
    w = tu_swquery_dialog_widget(name, parent, (Widget **)retval);
  }

  sDisplay = NULL;
  sScreen = NULL;
  return w;
}

