#ifndef lint
#pragma ident "@(#)pfgprequery.c 1.36 96/04/29 SMI"
#endif

/*
 * Copyright (c) 1994-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	pfgprequery.c
 * Group:	installtool
 * Description:
 */

#include "pf.h"
#include "pfg.h"
#include "pfglocales.h"

#include "pfgPrequery_ui.h"

static Widget prequery_dialog;

Widget
pfgCreatePreQuery(void)
{
	WidgetList widget_list;

	prequery_dialog = tu_prequery_dialog_widget("prequery_dialog",
		pfgTopLevel, &widget_list);

	XmAddWMProtocolCallback(pfgShell(prequery_dialog), pfgWMDeleteAtom,
	    (XtCallbackProc) pfgExit, NULL);

	XtVaSetValues(pfgShell(prequery_dialog),
	    XmNtitle, TITLE_PREQUERY,
	    XmNdeleteResponse, XmDO_NOTHING,
	    NULL);
	xm_SetNoResize(pfgTopLevel, prequery_dialog);

	pfgSetWidgetString(widget_list, "panelhelpText", MSG_PREQUERY);
	pfgSetWidgetString(widget_list, "continueButton", PFG_CONTINUE);
	pfgSetWidgetString(widget_list, "gobackButton", PFG_GOBACK);
	pfgSetWidgetString(widget_list, "preserveButton", PFG_PQ_PRESERVE);
	pfgSetWidgetString(widget_list, "exitButton", PFG_EXIT);
	pfgSetWidgetString(widget_list, "helpButton", PFG_HELP);

	free(widget_list);

	XtManageChild(prequery_dialog);

	return (prequery_dialog);
}


/* ARGSUSED */
void
prequeryPreserveCB(Widget w, XtPointer clientD, XtPointer callD)
{
	pfgBusy(pfgShell(w));

	(void) pfgCreatePreserve(pfgShell(prequery_dialog));
}


/* ARGSUSED */
void
prequeryContinueCB(Widget w, XtPointer clientD, XtPointer callD)
{
	pfgSetAction(parAContinue);
}


/* ARGSUSED */
void
prequeryGobackCB(Widget w, XtPointer clientD, XtPointer callD)
{
	pfgSetAction(parAGoback);
}
XtPointer closure,
                    XtPointer calldata);


/****************************************************************
 * get_constraint_widget:
 ****************************************************************/
static Widget get_constraint_widget(Widget child, Widget parent)
{
  Widget w;

  w = child;
  while (XtParent(w) != parent)
    w = XtParent(w);
  return (w);
}


/****************************************************************
 * put_client_data_arg:
 *    set up a client data argument.
 ****************************************************************/
static tu_ccb_arg_p put_client_data_arg(unsigned int cnt, ...)
{
  va_list pvar;
  int i;
  tu_ccb_arg_p rv;

  rv = (tu_ccb_arg_p) malloc(cnt*sizeof(tu_ccb_arg_t));
  if (rv == NULL) return NULL;

  va_start(pvar, cnt);

  for (i=0;i<cnt;i++) {
    rv[i].name = va_arg(pvar, char *);
    rv[i].value = va_arg(pvar, char *);
    rv[i].next = &rv[i+1];
  }
  va_end(pvar);
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
 * tu_prequery_dialog_widget:
 **************************************************************/
Widget tu_prequery_dialog_widget(char    * name,
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
  widget_array[WI_PREQUERY_DIALOG] =
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
    XmCreateText(widget_array[WI_PREQUERY_DIALOG], "panelhelpText", args, n);

  /***************** messageBox : XmMessageBox *****************/
  n = 0;
  XtSetArg(args[n], XmNdialogType, XmDIALOG_TEMPLATE); n++;
  widget_array[WI_MESSAGEBOX] =
    XmCreateMessageBox(widget_array[WI_PREQUERY_DIALOG], "messageBox", args, n);

  /***************** continueButton : XmPushButton *****************/
  widget_array[WI_CONTINUEBUTTON] =
    XmCreatePushButton(widget_array[WI_MESSAGEBOX], "continueButton", NULL, 0);

  /***************** gobackButton : XmPushButton *****************/
  widget_array[WI_GOBACKBUTTON] =
    XmCreatePushButton(widget_array[WI_MESSAGEBOX], "gobackButton", NULL, 0);

  /***************** preserveButton : XmPushButton *****************/
  widget_array[WI_PRESERVEBUTTON] =
    XmCreatePushButton(widget_array[WI_MESSAGEBOX], "preserveButton", NULL, 0);

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
                (XtCallbackProc)prequeryContinueCB,
                (XtPointer)NULL);

  XtManageChild(widget_array[WI_CONTINUEBUTTON]);
  XtAddCallback(widget_array[WI_GOBACKBUTTON],
                XmNactivateCallback,
                (XtCallbackProc)prequeryGobackCB,
                (XtPointer)NULL);

  XtManageChild(widget_array[WI_GOBACKBUTTON]);
  XtAddCallback(widget_array[WI_PRESERVEBUTTON],
                XmNactivateCallback,
                (XtCallbackProc)prequeryPreserveCB,
                (XtPointer)NULL);

  XtManageChild(widget_array[WI_PRESERVEBUTTON]);
  XtAddCallback(widget_array[WI_EXITBUTTON],
                XmNactivateCallback,
                (XtCallbackProc)pfgExit,
                (XtPointer)NULL);

  XtManageChild(widget_array[WI_EXITBUTTON]);
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
    *warr_ret = (Widget *) malloc(sizeof(Widget)*9);
    (void) memcpy((char *)*warr_ret,
                  (char *)widget_array,
           sizeof(Widget)*9);
  }

  /*
   *   Fix for SMI's X/NeWS server
   */
  tu_ol_fix_hierarchy(widget_array[WI_PREQUERY_DIALOG]);

  /*
   *   Invoke the create callbacks
   */
  tu_widcre_invoke_hooks(widget_array[WI_PREQUERY_DIALOG]);

  /*
   *   Return the first created widget.
   */
  return widget_array[WI_PREQUERY_DIALOG];
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
  if (strcmp(temp, "prequery_dialog") == 0){
    w = tu_prequery_dialog_widget(name, parent, (Widget **)retval);
  }

  sDisplay = NULL;
  sScreen = NULL;
  return w;
}

