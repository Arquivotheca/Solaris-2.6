#ifndef lint
#pragma ident "@(#)pfgprogressbar.c 1.3 96/07/08 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	pfgprogressbar.c
 * Group:	installtool
 * Description:
 */

#include <unistd.h>

#include "pfg.h"
#include "pfgprogressbar.h"
#include "pfgProgressBar_ui.h"

Widget
pfgProgressBarCreate(
	UIProgressBarInitData *init_data,
	pfgProgressBarDisplayData **display_data,
	int scale_info_cnt)
{
	WidgetList widget_list;
	Widget progressbar_dialog;
	int	cnt;

	if (scale_info_cnt <= 0)
		cnt = 1;
	else
		cnt = scale_info_cnt;

	*display_data = (pfgProgressBarDisplayData *)
		xcalloc(sizeof (pfgProgressBarDisplayData));
	(*display_data)->scale_info = (UIProgressBarScaleInfo *)
		xcalloc(sizeof (UIProgressBarScaleInfo) * cnt);

	/* get the dialog widget & the dialog widget list from teleuse */
	progressbar_dialog = tu_progressbar_dialog_widget(
		"progressbar_dialog", pfgTopLevel, &widget_list);

	/* get the display data to pass back to the caller */
	(*display_data)->dialog = progressbar_dialog;
	(*display_data)->widget_list = widget_list;
	(*display_data)->scale =
		pfgGetNamedWidget(widget_list, "progressScale");
	(*display_data)->main_label =
		pfgGetNamedWidget(widget_list, "mainLabel");
	(*display_data)->detail_label =
		pfgGetNamedWidget(widget_list, "detailLabel");
	(*display_data)->scale_info[0].start = 0;
	(*display_data)->scale_info[0].factor = 1;

	/* set up exit callback off window manager close */
	XmAddWMProtocolCallback(pfgShell(progressbar_dialog), pfgWMDeleteAtom,
		(XtCallbackProc) pfgExit, NULL);

	/* set title */
	XtVaSetValues(pfgShell(progressbar_dialog),
		XtNtitle, init_data->title ? init_data->title : "",
		XmNdeleteResponse, XmDO_NOTHING,
		NULL);

	pfgSetWidgetString(widget_list, "panelhelpText",
		init_data->main_msg ? init_data->main_msg : "");
	pfgSetWidgetString(widget_list, "mainLabel",
		init_data->main_label ? init_data->main_label : "");
	pfgSetWidgetString(widget_list, "detailLabel",
		init_data->detail_label ? init_data->detail_label : "");

	XmScaleSetValue(pfgGetNamedWidget(widget_list, "progressScale"),
		init_data->percent);

	/* manage the dialog */
	xm_SetNoResize(pfgTopLevel, progressbar_dialog);
	XtManageChild(progressbar_dialog);

	/* update the X display prior to going into the progress phase */
	pfgBusy(pfgShell(progressbar_dialog));
	xm_ForceDisplayUpdate(pfgTopLevel, progressbar_dialog);

	return (progressbar_dialog);
}

void
pfgProgressBarUpdate(pfgProgressBarDisplayData *display_data, int pause)
{
	/* force an update of the display */
	xm_ForceDisplayUpdate(pfgTopLevel, display_data->dialog);
	XmUpdateDisplay(pfgTopLevel);

	if (pause)
		(void) sleep(APP_PROGRESS_PAUSE_TIME);
}

void
pfgProgressBarCleanup(pfgProgressBarDisplayData *display_data)
{

	/*
	 * free the teleuse widget list -
	 * make sure this is done last, after the progress call back
	 * is finished  being called since it uses the widget_list
	 */
	free(display_data->widget_list);
	free(display_data->scale_info);
	free(display_data);

	pfgUnbusy(pfgShell(display_data->dialog));
}
                    Widget ** warr_ret)
{
  Arg args[26];
  Widget widget_array[13];
  XtTranslations ttbl;
  int n;
  /* Make sure the classes used are initialized */
  class_init();

  /***************** object of type : XmFormDialog *****************/
  n = 0;
  XtSetArg(args[n], XmNrubberPositioning, False); n++;
  XtSetArg(args[n], XmNresizePolicy, XmRESIZE_ANY); n++;
  widget_array[WI_PROGRESSBAR_DIALOG] =
    XmCreateFormDialog(parent, name, args, n);

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
    XmCreateText(widget_array[WI_PROGRESSBAR_DIALOG], "panelhelpText", args, n);

  /***************** progressForm : XmForm *****************/
  n = 0;
  XtSetArg(args[n], XmNmarginWidth, 20); n++;
  XtSetArg(args[n], XmNrubberPositioning, False); n++;
  XtSetArg(args[n], XmNresizePolicy, XmRESIZE_ANY); n++;
  XtSetArg(args[n], XmNresizable, True); n++;
  widget_array[WI_PROGRESSFORM] =
    XmCreateForm(widget_array[WI_PROGRESSBAR_DIALOG], "progressForm", args, n);

  /***************** mainLabel : XmLabel *****************/
  n = 0;
  XtSetArg(args[n], XmNresizable, True); n++;
  widget_array[WI_MAINLABEL] =
    XmCreateLabel(widget_array[WI_PROGRESSFORM], "mainLabel", args, n);

  /***************** detailLabel : XmLabel *****************/
  n = 0;
  XtSetArg(args[n], XmNresizable, True); n++;
  widget_array[WI_DETAILLABEL] =
    XmCreateLabel(widget_array[WI_PROGRESSFORM], "detailLabel", args, n);

  /***************** progressScale : XmScale *****************/
  n = 0;
  XtSetArg(args[n], XmNorientation, XmHORIZONTAL); n++;
  XtSetArg(args[n], XmNtraversalOn, False); n++;
  XtSetArg(args[n], XmNscaleHeight, 32); n++;
  XtSetArg(args[n], XmNresizable, True); n++;
  widget_array[WI_PROGRESSSCALE] =
    XmCreateScale(widget_array[WI_PROGRESSFORM], "progressScale", args, n);

  /***************** messageBox : XmMessageBox *****************/
  n = 0;
  XtSetArg(args[n], XmNdialogType, XmDIALOG_TEMPLATE); n++;
  widget_array[WI_MESSAGEBOX] =
    XmCreateMessageBox(widget_array[WI_PROGRESSBAR_DIALOG], "messageBox", args, n);

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
  widget_array[12] = NULL;


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

  /***************** progressForm : XmForm *****************/
  n = 0;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNtopWidget, widget_array[WI_PANELHELPTEXT]); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNbottomWidget, widget_array[WI_MESSAGEBOX]); n++;
  XtSetValues(widget_array[WI_PROGRESSFORM], args, n);


  /***************** mainLabel : XmLabel *****************/
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_NONE); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_NONE); n++;
  XtSetValues(widget_array[WI_MAINLABEL], args, n);

  XtManageChild(widget_array[WI_MAINLABEL]);

  /***************** detailLabel : XmLabel *****************/
  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNleftWidget, widget_array[WI_MAINLABEL]); n++;
  XtSetArg(args[n], XmNleftOffset, 10); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_NONE); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_NONE); n++;
  XtSetValues(widget_array[WI_DETAILLABEL], args, n);

  XtManageChild(widget_array[WI_DETAILLABEL]);

  /***************** progressScale : XmScale *****************/
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_NONE); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNtopWidget, widget_array[WI_MAINLABEL]); n++;
  XtSetArg(args[n], XmNtopOffset, 15); n++;
  XtSetValues(widget_array[WI_PROGRESSSCALE], args, n);

  XtManageChild(widget_array[WI_PROGRESSSCALE]);
  XtManageChild(widget_array[WI_PROGRESSFORM]);

  /***************** messageBox : XmMessageBox *****************/
  n = 0;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_MESSAGEBOX], args, n);

  XtManageChild(widget_array[WI_MESSAGEBOX]);

  /*
   *  Allocate memory for the widget array to return
   */
  if (warr_ret != NULL) {
    *warr_ret = (Widget *) malloc(sizeof(Widget)*13);
    (void) memcpy((char *)*warr_ret,
                  (char *)widget_array,
           sizeof(Widget)*13);
  }

  /*
   *   Fix for SMI's X/NeWS server
   */
  tu_ol_fix_hierarchy(widget_array[WI_PROGRESSBAR_DIALOG]);

  /*
   *   Invoke the create callbacks
   */
  tu_widcre_invoke_hooks(widget_array[WI_PROGRESSBAR_DIALOG]);

  /*
   *   Return the first created widget.
   */
  return widget_array[WI_PROGRESSBAR_DIALOG];
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
  if (strcmp(temp, "progressbar_dialog") == 0){
    w = tu_progressbar_dialog_widget(name, parent, (Widget **)retval);
  }

  sDisplay = NULL;
  sScreen = NULL;
  return w;
}

