#ifndef lint
#pragma ident "@(#)pfgallocatesvcquery.c 1.5 96/06/17 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	pfgallocatesvcquery.c
 * Group:	installtool
 * Description:
 */

#include "pf.h"
#include "pfg.h"
#include "pfgAllocateSvcQuery_ui.h"

static void allocatesvcContinueCB(Widget, XtPointer, XtPointer);
static void allocatesvcGobackCB(Widget, XtPointer, XtPointer);
static void allocatesvcAllocateCB(Widget, XtPointer, XtPointer);

Widget
pfgCreateAllocateSvcQuery(void)
{
	Widget allocatesvc_dialog;
	WidgetList widget_list;
	Widget default_button;

	allocatesvc_dialog = tu_allocatesvc_dialog_widget("allocatesvc_dialog",
		pfgTopLevel, &widget_list);

	XmAddWMProtocolCallback(pfgShell(allocatesvc_dialog), pfgWMDeleteAtom,
	    (XtCallbackProc) pfgExit, NULL);

	XtVaSetValues(pfgShell(allocatesvc_dialog),
		XmNtitle, TITLE_ALLOCATE_SVC_QUERY,
		XmNdeleteResponse, XmDO_NOTHING,
		NULL);
	xm_SetNoResize(pfgTopLevel, allocatesvc_dialog);

	pfgSetWidgetString(widget_list, "panelhelpText",
		MSG_ALLOCATE_SVC_QUERY);
	pfgSetWidgetString(widget_list, "allocateButton", PFG_ALLOCATE);
	pfgSetStandardButtonStrings(widget_list,
		ButtonContinue, ButtonGoback, ButtonExit, ButtonHelp,
		NULL);

	switch (get_machinetype()) {
	case MT_STANDALONE:
		default_button = pfgGetNamedWidget(widget_list,
			"continueButton");
		break;
	case MT_SERVER:
		default_button = pfgGetNamedWidget(widget_list,
			"allocateButton");
		break;
	default:
		default_button = pfgGetNamedWidget(widget_list,
			"continueButton");
		break;
	}

	XtAddCallback(pfgGetNamedWidget(widget_list, "continueButton"),
		XmNactivateCallback, allocatesvcContinueCB, NULL);
	XtAddCallback(pfgGetNamedWidget(widget_list, "gobackButton"),
		XmNactivateCallback, allocatesvcGobackCB, NULL);
	XtAddCallback(pfgGetNamedWidget(widget_list, "allocateButton"),
		XmNactivateCallback, allocatesvcAllocateCB, NULL);

	/* set up the default button */
	XtVaSetValues(pfgGetNamedWidget(widget_list, "messageBox"),
		XmNdefaultButton, default_button,
		NULL);

	XtManageChild(allocatesvc_dialog);

	free(widget_list);

	return (allocatesvc_dialog);
}

/* ARGSUSED */
void
allocatesvcContinueCB(Widget continueButton, XtPointer client_data,
	XtPointer call_data)
{
	setSystemType(MT_STANDALONE);
	pfgSetAction(parAContinue);
}

/* ARGSUSED */
void
allocatesvcAllocateCB(Widget continueButton, XtPointer client_data,
	XtPointer call_data)
{
	setSystemType(MT_SERVER);
	pfgSetAction(parAAllocateSvc);
}

/* ARGSUSED */
void
allocatesvcGobackCB(Widget w, XtPointer client, XtPointer cbs)
{
	pfgSetAction(parAGoback);
}
  XtInitializeWidgetClass(xmFormWidgetClass);
  XtInitializeWidgetClass(xmTextWidgetClass);
  XtInitializeWidgetClass(xmMessageBoxWidgetClass);
  XtInitializeWidgetClass(xmPushButtonWidgetClass);
  XtInitializeWidgetClass(xmLabelWidgetClass);
  XtInitializeWidgetClass(xmScrolledWindowWidgetClass);
  XtInitializeWidgetClass(xmRowColumnWidgetClass);
}



/****************************************************************
 *
 *  Main C code for presentation component
 *
 ****************************************************************/

/**************************************************************
 * tu_baseWindow_widget:
 **************************************************************/
Widget tu_baseWindow_widget(char    * name,
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
  widget_array[WI_BASEWINDOW] =
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
    XmCreateText(widget_array[WI_BASEWINDOW], "panelhelpText", args, n);

  /***************** messageBox : XmMessageBox *****************/
  n = 0;
  XtSetArg(args[n], XmNdialogType, XmDIALOG_TEMPLATE); n++;
  widget_array[WI_MESSAGEBOX] =
    XmCreateMessageBox(widget_array[WI_BASEWINDOW], "messageBox", args, n);

  /***************** button1 : XmPushButton *****************/
  widget_array[WI_BUTTON1] =
    XmCreatePushButton(widget_array[WI_MESSAGEBOX], "button1", NULL, 0);

  /***************** button2 : XmPushButton *****************/
  widget_array[WI_BUTTON2] =
    XmCreatePushButton(widget_array[WI_MESSAGEBOX], "button2", NULL, 0);

  /***************** button3 : XmPushButton *****************/
  widget_array[WI_BUTTON3] =
    XmCreatePushButton(widget_array[WI_MESSAGEBOX], "button3", NULL, 0);

  /***************** button4 : XmPushButton *****************/
  widget_array[WI_BUTTON4] =
    XmCreatePushButton(widget_array[WI_MESSAGEBOX], "button4", NULL, 0);

  /***************** button5 : XmPushButton *****************/
  widget_array[WI_BUTTON5] =
    XmCreatePushButton(widget_array[WI_MESSAGEBOX], "button5", NULL, 0);

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
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_MESSAGEBOX], args, n);

  XtManageChild(widget_array[WI_BUTTON1]);
  XtManageChild(widget_array[WI_BUTTON2]);
  XtManageChild(widget_array[WI_BUTTON3]);
  XtManageChild(widget_array[WI_BUTTON4]);
  XtManageChild(widget_array[WI_BUTTON5]);
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
  tu_ol_fix_hierarchy(widget_array[WI_BASEWINDOW]);

  /*
   *   Invoke the create callbacks
   */
  tu_widcre_invoke_hooks(widget_array[WI_BASEWINDOW]);

  /*
   *   Return the first created widget.
   */
  return widget_array[WI_BASEWINDOW];
}


/**************************************************************
 * tu_formWindow_widget:
 **************************************************************/
Widget tu_formWindow_widget(char    * name,
                            Widget    parent,
                            Widget ** warr_ret)
{
  Arg args[26];
  Widget widget_array[10];
  XtTranslations ttbl;
  int n;
  /* Make sure the classes used are initialized */
  class_init();

  /***************** object of type : XmFormDialog *****************/
  widget_array[WI_FORMWINDOW] =
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
  widget_array[WI_PANELHELPTEXT1] =
    XmCreateText(widget_array[WI_FORMWINDOW], "panelhelpText", args, n);

  /***************** form : XmForm *****************/
  widget_array[WI_FORM] =
    XmCreateForm(widget_array[WI_FORMWINDOW], "form", NULL, 0);

  /***************** messageBox : XmMessageBox *****************/
  n = 0;
  XtSetArg(args[n], XmNdialogType, XmDIALOG_TEMPLATE); n++;
  widget_array[WI_MESSAGEBOX1] =
    XmCreateMessageBox(widget_array[WI_FORMWINDOW], "messageBox", args, n);

  /***************** button1 : XmPushButton *****************/
  widget_array[WI_BUTTON6] =
    XmCreatePushButton(widget_array[WI_MESSAGEBOX1], "button1", NULL, 0);

  /***************** button2 : XmPushButton *****************/
  widget_array[WI_BUTTON7] =
    XmCreatePushButton(widget_array[WI_MESSAGEBOX1], "button2", NULL, 0);

  /***************** button3 : XmPushButton *****************/
  widget_array[WI_BUTTON8] =
    XmCreatePushButton(widget_array[WI_MESSAGEBOX1], "button3", NULL, 0);

  /***************** button4 : XmPushButton *****************/
  widget_array[WI_BUTTON9] =
    XmCreatePushButton(widget_array[WI_MESSAGEBOX1], "button4", NULL, 0);

  /***************** button5 : XmPushButton *****************/
  widget_array[WI_BUTTON10] =
    XmCreatePushButton(widget_array[WI_MESSAGEBOX1], "button5", NULL, 0);

  /* Terminate the widget array */
  widget_array[9] = NULL;


  /***************** panelhelpText : XmText *****************/
  n = 0;
  ttbl = XtParseTranslationTable("#override\n\
~Ctrl ~Meta<BtnDown>:\n\
~Ctrl ~Meta<BtnUp>:");
  XtOverrideTranslations(widget_array[WI_PANELHELPTEXT1], ttbl);
  unregister_as_dropsite(widget_array[WI_PANELHELPTEXT1],
                         NULL,
                         NULL);

  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_PANELHELPTEXT1], args, n);

  XtManageChild(widget_array[WI_PANELHELPTEXT1]);

  /***************** form : XmForm *****************/
  n = 0;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNtopWidget, widget_array[WI_PANELHELPTEXT1]); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNbottomWidget, widget_array[WI_MESSAGEBOX1]); n++;
  XtSetValues(widget_array[WI_FORM], args, n);

  XtManageChild(widget_array[WI_FORM]);

  /***************** messageBox : XmMessageBox *****************/
  n = 0;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_MESSAGEBOX1], args, n);

  XtManageChild(widget_array[WI_BUTTON6]);
  XtManageChild(widget_array[WI_BUTTON7]);
  XtManageChild(widget_array[WI_BUTTON8]);
  XtManageChild(widget_array[WI_BUTTON9]);
  XtManageChild(widget_array[WI_BUTTON10]);
  XtManageChild(widget_array[WI_MESSAGEBOX1]);

  /*
   *  Allocate memory for the widget array to return
   */
  if (warr_ret != NULL) {
    *warr_ret = (Widget *) malloc(sizeof(Widget)*10);
    (void) memcpy((char *)*warr_ret,
                  (char *)widget_array,
           sizeof(Widget)*10);
  }

  /*
   *   Fix for SMI's X/NeWS server
   */
  tu_ol_fix_hierarchy(widget_array[WI_FORMWINDOW]);

  /*
   *   Invoke the create callbacks
   */
  tu_widcre_invoke_hooks(widget_array[WI_FORMWINDOW]);

  /*
   *   Return the first created widget.
   */
  return widget_array[WI_FORMWINDOW];
}


/**************************************************************
 * tu_twolistWindow_widget:
 **************************************************************/
Widget tu_twolistWindow_widget(char    * name,
                               Widget    parent,
                               Widget ** warr_ret)
{
  Arg args[26];
  Arg pargs[26];
  Widget tmpw;
  Widget tmpw1;
  Widget widget_array[17];
  XtTranslations ttbl;
  int n;
  int pn;
  /* Make sure the classes used are initialized */
  class_init();

  /***************** object of type : XmFormDialog *****************/
  widget_array[WI_TWOLISTWINDOW] =
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
  widget_array[WI_PANELHELPTEXT2] =
    XmCreateText(widget_array[WI_TWOLISTWINDOW], "panelhelpText", args, n);

  /***************** form : XmForm *****************/
  n = 0;
  XtSetArg(args[n], XmNfractionBase, 9); n++;
  XtSetArg(args[n], XmNmarginWidth, 20); n++;
  widget_array[WI_FORM1] =
    XmCreateForm(widget_array[WI_TWOLISTWINDOW], "form", args, n);

  /***************** availableLabel : XmLabel *****************/
  widget_array[WI_AVAILABLELABEL] =
    XmCreateLabel(widget_array[WI_FORM1], "availableLabel", NULL, 0);

  /***************** availableScrolledList : XmScrolledList *****************/
  n = 0;
  pn = 0;
  XtSetArg(args[n], XmNlistSizePolicy, XmCONSTANT); n++;
  XtSetArg(pargs[pn], XmNresizable, False); pn++;
  widget_array[WI_AVAILABLESCROLLEDLIST] =
    XmCreateScrolledList(widget_array[WI_FORM1], "availableScrolledList", args, n);
  tmpw = get_constraint_widget(widget_array[WI_AVAILABLESCROLLEDLIST], widget_array[WI_FORM1]);
  if (tmpw)
    XtSetValues(tmpw, pargs, pn);

  /***************** buttonForm : XmRowColumn *****************/
  n = 0;
  XtSetArg(args[n], XmNentryAlignment, XmALIGNMENT_CENTER); n++;
  XtSetArg(args[n], XmNmarginWidth, 10); n++;
  XtSetArg(args[n], XmNspacing, 10); n++;
  widget_array[WI_BUTTONFORM] =
    XmCreateRowColumn(widget_array[WI_FORM1], "buttonForm", args, n);

  /***************** addButton : XmPushButton *****************/
  widget_array[WI_ADDBUTTON] =
    XmCreatePushButton(widget_array[WI_BUTTONFORM], "addButton", NULL, 0);

  /***************** removeButton : XmPushButton *****************/
  widget_array[WI_REMOVEBUTTON] =
    XmCreatePushButton(widget_array[WI_BUTTONFORM], "removeButton", NULL, 0);

  /***************** selectedLabel : XmLabel *****************/
  widget_array[WI_SELECTEDLABEL] =
    XmCreateLabel(widget_array[WI_FORM1], "selectedLabel", NULL, 0);

  /***************** selectedScrolledList : XmScrolledList *****************/
  n = 0;
  pn = 0;
  XtSetArg(args[n], XmNlistSizePolicy, XmCONSTANT); n++;
  XtSetArg(pargs[pn], XmNresizable, False); pn++;
  widget_array[WI_SELECTEDSCROLLEDLIST] =
    XmCreateScrolledList(widget_array[WI_FORM1], "selectedScrolledList", args, n);
  tmpw = get_constraint_widget(widget_array[WI_SELECTEDSCROLLEDLIST], widget_array[WI_FORM1]);
  if (tmpw)
    XtSetValues(tmpw, pargs, pn);

  /***************** messageBox : XmMessageBox *****************/
  n = 0;
  XtSetArg(args[n], XmNdialogType, XmDIALOG_TEMPLATE); n++;
  widget_array[WI_MESSAGEBOX2] =
    XmCreateMessageBox(widget_array[WI_TWOLISTWINDOW], "messageBox", args, n);

  /***************** continueButton : XmPushButton *****************/
  widget_array[WI_CONTINUEBUTTON] =
    XmCreatePushButton(widget_array[WI_MESSAGEBOX2], "continueButton", NULL, 0);

  /***************** gobackButton : XmPushButton *****************/
  widget_array[WI_GOBACKBUTTON] =
    XmCreatePushButton(widget_array[WI_MESSAGEBOX2], "gobackButton", NULL, 0);

  /***************** exitButton : XmPushButton *****************/
  widget_array[WI_EXITBUTTON] =
    XmCreatePushButton(widget_array[WI_MESSAGEBOX2], "exitButton", NULL, 0);

  /***************** helpButton : XmPushButton *****************/
  widget_array[WI_HELPBUTTON] =
    XmCreatePushButton(widget_array[WI_MESSAGEBOX2], "helpButton", NULL, 0);

  /***************** button5 : XmPushButton *****************/
  widget_array[WI_BUTTON11] =
    XmCreatePushButton(widget_array[WI_MESSAGEBOX2], "button5", NULL, 0);

  /* Terminate the widget array */
  widget_array[16] = NULL;


  /***************** panelhelpText : XmText *****************/
  n = 0;
  ttbl = XtParseTranslationTable("#override\n\
~Ctrl ~Meta<BtnDown>:\n\
~Ctrl ~Meta<BtnUp>:");
  XtOverrideTranslations(widget_array[WI_PANELHELPTEXT2], ttbl);
  unregister_as_dropsite(widget_array[WI_PANELHELPTEXT2],
                         NULL,
                         NULL);

  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_PANELHELPTEXT2], args, n);

  XtManageChild(widget_array[WI_PANELHELPTEXT2]);

  /***************** form : XmForm *****************/
  n = 0;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNtopWidget, widget_array[WI_PANELHELPTEXT2]); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNbottomWidget, widget_array[WI_MESSAGEBOX2]); n++;
  XtSetValues(widget_array[WI_FORM1], args, n);


  /***************** availableLabel : XmLabel *****************/
  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION); n++;
  XtSetArg(args[n], XmNrightPosition, 4); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_AVAILABLELABEL], args, n);

  XtManageChild(widget_array[WI_AVAILABLELABEL]);

  /***************** availableScrolledList : XmScrolledList *****************/
  pn = 0;
  XtSetArg(pargs[pn], XmNleftAttachment, XmATTACH_FORM); pn++;
  XtSetArg(pargs[pn], XmNtopAttachment, XmATTACH_WIDGET); pn++;
  XtSetArg(pargs[pn], XmNtopWidget, widget_array[WI_SELECTEDLABEL]); pn++;
  XtSetArg(pargs[pn], XmNbottomAttachment, XmATTACH_FORM); pn++;
  XtSetArg(pargs[pn], XmNrightAttachment, XmATTACH_POSITION); pn++;
  XtSetArg(pargs[pn], XmNrightPosition, 4); pn++;
  tmpw = get_constraint_widget(widget_array[WI_AVAILABLESCROLLEDLIST], widget_array[WI_FORM1]);
  if (tmpw)
    XtSetValues(tmpw, pargs, pn);

  XtManageChild(widget_array[WI_AVAILABLESCROLLEDLIST]);

  /***************** buttonForm : XmRowColumn *****************/
  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNtopWidget, widget_array[WI_AVAILABLELABEL]); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION); n++;
  XtSetArg(args[n], XmNleftPosition, 4); n++;
  XtSetValues(widget_array[WI_BUTTONFORM], args, n);

  XtSetSensitive(widget_array[WI_ADDBUTTON], False);
  XtManageChild(widget_array[WI_ADDBUTTON]);
  XtSetSensitive(widget_array[WI_REMOVEBUTTON], False);
  XtManageChild(widget_array[WI_REMOVEBUTTON]);
  XtManageChild(widget_array[WI_BUTTONFORM]);

  /***************** selectedLabel : XmLabel *****************/
  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNleftPosition, 6); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftWidget, widget_array[WI_BUTTONFORM]); n++;
  XtSetValues(widget_array[WI_SELECTEDLABEL], args, n);

  XtManageChild(widget_array[WI_SELECTEDLABEL]);

  /***************** selectedScrolledList : XmScrolledList *****************/
  pn = 0;
  XtSetArg(pargs[pn], XmNleftAttachment, XmATTACH_WIDGET); pn++;
  XtSetArg(pargs[pn], XmNleftWidget, widget_array[WI_BUTTONFORM]); pn++;
  XtSetArg(pargs[pn], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); pn++;
  tmpw1 = get_constraint_widget(
widget_array[WI_AVAILABLESCROLLEDLIST], widget_array[WI_FORM1]);
  XtSetArg(pargs[pn], XmNtopWidget, tmpw1); pn++;
  XtSetArg(pargs[pn], XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET); pn++;
  tmpw1 = get_constraint_widget(
widget_array[WI_AVAILABLESCROLLEDLIST], widget_array[WI_FORM1]);
  XtSetArg(pargs[pn], XmNbottomWidget, tmpw1); pn++;
  XtSetArg(pargs[pn], XmNrightAttachment, XmATTACH_FORM); pn++;
  tmpw = get_constraint_widget(widget_array[WI_SELECTEDSCROLLEDLIST], widget_array[WI_FORM1]);
  if (tmpw)
    XtSetValues(tmpw, pargs, pn);

  XtManageChild(widget_array[WI_SELECTEDSCROLLEDLIST]);
  XtManageChild(widget_array[WI_FORM1]);

  /***************** messageBox : XmMessageBox *****************/
  n = 0;
  XtSetArg(args[n], XmNdefaultButton, widget_array[WI_CONTINUEBUTTON]); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_MESSAGEBOX2], args, n);

  XtManageChild(widget_array[WI_CONTINUEBUTTON]);
  XtManageChild(widget_array[WI_GOBACKBUTTON]);
  XtManageChild(widget_array[WI_EXITBUTTON]);
  XtManageChild(widget_array[WI_HELPBUTTON]);
  XtManageChild(widget_array[WI_MESSAGEBOX2]);

  /*
   *  Allocate memory for the widget array to return
   */
  if (warr_ret != NULL) {
    *warr_ret = (Widget *) malloc(sizeof(Widget)*17);
    (void) memcpy((char *)*warr_ret,
                  (char *)widget_array,
           sizeof(Widget)*17);
  }

  /*
   *   Fix for SMI's X/NeWS server
   */
  tu_ol_fix_hierarchy(widget_array[WI_TWOLISTWINDOW]);

  /*
   *   Invoke the create callbacks
   */
  tu_widcre_invoke_hooks(widget_array[WI_TWOLISTWINDOW]);

  /*
   *   Return the first created widget.
   */
  return widget_array[WI_TWOLISTWINDOW];
}


/**************************************************************
 * tu_allocatesvc_dialog_widget:
 **************************************************************/
Widget tu_allocatesvc_dialog_widget(char    * name,
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
  widget_array[WI_ALLOCATESVC_DIALOG] =
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
  widget_array[WI_PANELHELPTEXT3] =
    XmCreateText(widget_array[WI_ALLOCATESVC_DIALOG], "panelhelpText", args, n);

  /***************** messageBox : XmMessageBox *****************/
  n = 0;
  XtSetArg(args[n], XmNdialogType, XmDIALOG_TEMPLATE); n++;
  widget_array[WI_MESSAGEBOX3] =
    XmCreateMessageBox(widget_array[WI_ALLOCATESVC_DIALOG], "messageBox", args, n);

  /***************** continueButton : XmPushButton *****************/
  widget_array[WI_CONTINUEBUTTON1] =
    XmCreatePushButton(widget_array[WI_MESSAGEBOX3], "continueButton", NULL, 0);

  /***************** gobackButton : XmPushButton *****************/
  widget_array[WI_GOBACKBUTTON1] =
    XmCreatePushButton(widget_array[WI_MESSAGEBOX3], "gobackButton", NULL, 0);

  /***************** allocateButton : XmPushButton *****************/
  widget_array[WI_ALLOCATEBUTTON] =
    XmCreatePushButton(widget_array[WI_MESSAGEBOX3], "allocateButton", NULL, 0);

  /***************** exitButton : XmPushButton *****************/
  widget_array[WI_EXITBUTTON1] =
    XmCreatePushButton(widget_array[WI_MESSAGEBOX3], "exitButton", NULL, 0);

  /***************** helpButton : XmPushButton *****************/
  widget_array[WI_HELPBUTTON1] =
    XmCreatePushButton(widget_array[WI_MESSAGEBOX3], "helpButton", NULL, 0);

  /* Terminate the widget array */
  widget_array[8] = NULL;


  /***************** panelhelpText : XmText *****************/
  n = 0;
  ttbl = XtParseTranslationTable("#override\n\
~Ctrl ~Meta<BtnDown>:\n\
~Ctrl ~Meta<BtnUp>:");
  XtOverrideTranslations(widget_array[WI_PANELHELPTEXT3], ttbl);
  unregister_as_dropsite(widget_array[WI_PANELHELPTEXT3],
                         NULL,
                         NULL);

  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNbottomWidget, widget_array[WI_MESSAGEBOX3]); n++;
  XtSetValues(widget_array[WI_PANELHELPTEXT3], args, n);

  XtManageChild(widget_array[WI_PANELHELPTEXT3]);

  /***************** messageBox : XmMessageBox *****************/
  n = 0;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_MESSAGEBOX3], args, n);

  XtManageChild(widget_array[WI_CONTINUEBUTTON1]);
  XtManageChild(widget_array[WI_GOBACKBUTTON1]);
  XtManageChild(widget_array[WI_ALLOCATEBUTTON]);
  XtAddCallback(widget_array[WI_EXITBUTTON1],
                XmNactivateCallback,
                (XtCallbackProc)pfgExit,
                (XtPointer)NULL);

  XtManageChild(widget_array[WI_EXITBUTTON1]);
  XtAddCallback(widget_array[WI_HELPBUTTON1],
                XmNactivateCallback,
                pfgHelp,
                (XtPointer)"allocate.t");

  XtManageChild(widget_array[WI_HELPBUTTON1]);
  XtManageChild(widget_array[WI_MESSAGEBOX3]);

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
  tu_ol_fix_hierarchy(widget_array[WI_ALLOCATESVC_DIALOG]);

  /*
   *   Invoke the create callbacks
   */
  tu_widcre_invoke_hooks(widget_array[WI_ALLOCATESVC_DIALOG]);

  /*
   *   Return the first created widget.
   */
  return widget_array[WI_ALLOCATESVC_DIALOG];
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
  if (strcmp(temp, "baseWindow") == 0){
    w = tu_baseWindow_widget(name, parent, (Widget **)retval);
  }
  else if (strcmp(temp, "formWindow") == 0){
    w = tu_formWindow_widget(name, parent, (Widget **)retval);
  }
  else if (strcmp(temp, "twolistWindow") == 0){
    w = tu_twolistWindow_widget(name, parent, (Widget **)retval);
  }
  else if (strcmp(temp, "allocatesvc_dialog") == 0){
    w = tu_allocatesvc_dialog_widget(name, parent, (Widget **)retval);
  }

  sDisplay = NULL;
  sScreen = NULL;
  return w;
}

