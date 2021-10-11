#ifndef lint
#pragma ident "@(#)pfgautoquery.c 1.39 96/07/08 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	pfgautoquery.c
 * Group:	installtool
 * Description:
 */

#include "pf.h"
#include "pfg.h"

#include "pfgAutoquery_ui.h"

static void autoqueryAutoCB(Widget, XtPointer, XtPointer);
static void autoqueryManualCB(Widget, XtPointer, XtPointer);

static Widget autoquery_dialog;
static int first = True;

Widget
pfgCreateAutoQuery(void)
{
	WidgetList widget_list;

	autoquery_dialog = tu_autoquery_dialog_widget("AutoQuery",
						pfgTopLevel, &widget_list);

	XmAddWMProtocolCallback(pfgShell(autoquery_dialog), pfgWMDeleteAtom,
	    (XtCallbackProc) pfgExit, NULL);

	XtVaSetValues(pfgShell(autoquery_dialog),
	    XmNtitle, TITLE_AUTOLAYOUTQRY,
	    XmNdeleteResponse, XmDO_NOTHING,
	    NULL);
	xm_SetNoResize(pfgTopLevel, autoquery_dialog);

	XtAddCallback(
		pfgGetNamedWidget(widget_list, "continueButton"),
		XmNactivateCallback, autoqueryAutoCB, NULL);
	XtAddCallback(
		pfgGetNamedWidget(widget_list, "customizeButton"),
		XmNactivateCallback, autoqueryManualCB, NULL);
	pfgSetWidgetString(widget_list, "panelhelpText",
		MSG_AUTOLAYOUTQRY);
	pfgSetWidgetString(widget_list, "continueButton",
		PFG_AQ_AUTOLAY);
	pfgSetWidgetString(widget_list, "gobackButton", PFG_GOBACK);
	pfgSetWidgetString(widget_list, "customizeButton",
		PFG_AQ_MANLAY);
	pfgSetWidgetString(widget_list, "exitButton", PFG_EXIT);
	pfgSetWidgetString(widget_list, "helpButton", PFG_HELP);

	free(widget_list);

	XtManageChild(autoquery_dialog);

	return (autoquery_dialog);
}

/*
 * This function is called when the user selects the manual layout
 * button.  The user is only given one chance at a manual layout the
 * the first time through the screen.
 */

/* ARGSUSED */
static void
autoqueryManualCB(Widget w, XtPointer clientD, XtPointer callD)
{
	if (first) {
		/* null out all nonpreserved disk slices */
		pfgNullDisks();
		/* set default mount list for manual layout mode */
		pfgSetManualDefaultMounts();
	} else {
		pfgCompareLayout(); /* null out disks not in previous layout */
	}
	first = False;
	/* build layout array */
	pfgBuildLayoutArray();
	pfgSetAction(parAContinue);

}

/*
 * This function is called during a second pass thru the screen
 * when the user chooses continue.
 */

/* ARGSUSED */
static void
autoqueryAutoCB(Widget button, XtPointer clientD, XtPointer callD)
{
	first = False;

	pfgBusy(pfgShell(button));

	(void) pfgCreateAutoLayout(pfgShell(autoquery_dialog));

	/* this is the new auto layout screen */
/* 	pfgCreateLayout(pfgShell(autoquery_dialog)); */
}


/* ARGSUSED */
void
autoqueryGobackCB(Widget w, XtPointer clientD, XtPointer callD)
{
	pfgSetAction(parAGoback);
}
tClass);
  XtInitializeWidgetClass(xmMessageBoxWidgetClass);
  XtInitializeWidgetClass(xmPushButtonWidgetClass);
}



/****************************************************************
 *
 *  Main C code for presentation component
 *
 ****************************************************************/

/**************************************************************
 * tu_autoquery_dialog_widget:
 **************************************************************/
Widget tu_autoquery_dialog_widget(char    * name,
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
  widget_array[WI_AUTOQUERY_DIALOG] =
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
    XmCreateText(widget_array[WI_AUTOQUERY_DIALOG], "panelhelpText", args, n);

  /***************** messageBox : XmMessageBox *****************/
  n = 0;
  XtSetArg(args[n], XmNdialogType, XmDIALOG_TEMPLATE); n++;
  widget_array[WI_MESSAGEBOX] =
    XmCreateMessageBox(widget_array[WI_AUTOQUERY_DIALOG], "messageBox", args, n);

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

  XtManageChild(widget_array[WI_CONTINUEBUTTON]);
  XtAddCallback(widget_array[WI_GOBACKBUTTON],
                XmNactivateCallback,
                (XtCallbackProc)autoqueryGobackCB,
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
                (XtPointer)"autolayout.t");

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
  tu_ol_fix_hierarchy(widget_array[WI_AUTOQUERY_DIALOG]);

  /*
   *   Invoke the create callbacks
   */
  tu_widcre_invoke_hooks(widget_array[WI_AUTOQUERY_DIALOG]);

  /*
   *   Return the first created widget.
   */
  return widget_array[WI_AUTOQUERY_DIALOG];
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
  if (strcmp(temp, "autoquery_dialog") == 0){
    w = tu_autoquery_dialog_widget(name, parent, (Widget **)retval);
  }

  sDisplay = NULL;
  sScreen = NULL;
  return w;
}

