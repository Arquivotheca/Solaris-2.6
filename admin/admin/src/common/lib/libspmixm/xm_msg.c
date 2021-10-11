#ifndef lint
#pragma ident "@(#)xm_msg.c 1.2 96/04/22 SMI"
#endif

/*
 * Copyright (c) 1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	xm_msg.c
 * Group:	libspmixm
 * Description:
 *	Module to implement the Motif version of our generic UI
 *	message dialogs.
 */

#include <stdarg.h>

#include "spmixm_api.h"
#include "xm_utils.h"
#include "spmiapp_api.h"

#define	MSG_DIALOG_WIDGET_NAME "messageDialog"

/* private functions */
static void xm_MsgEventLoop(Widget dialog, xm_MsgAdditionalInfo *);
static void xm_MsgResponseCB(Widget, XtPointer, XmAnyCallbackStruct *);
static void _delete_func(void);

/* used internally to pass client data around to callbacks */
typedef struct {
	char *text;	/* used for help topic */
	xm_MsgAdditionalInfo xm_info;
	Widget other1_btn;
	Widget other2_btn;
} xm_MsgClientData;

static xm_MsgClientData _xm_MsgClientData;

/*
 * Function: xm_MsgFunction
 * Description:
 *	Motif messaging function.
 *	Register this with the UI message stuff...
 *	Creates a new dialog with title, text, buttons...
 *	We pop up the dialog and then enter a subevent loop
 *	to process the users input on this dialog.
 *	A subevent loop may seem strange, but we want this mirror
 *	the paradign used in our curses interface as well.
 *	(i.e. we want a synchronous response...)
 * Scope:       PUBLIC
 * Parameters:  msg_info - a properly initialized msg dialog structure
 * Return:	none
 * Note:
 *	Get the user response from the UI response retrieval function.
 */
void
xm_MsgFunction(UI_MsgStruct *msg_info)
{
	char *dialog_title_def;
	Widget msg_dialog;
	Widget label = NULL;
	Widget symbol = NULL;
	Widget button = NULL;
	unsigned char default_button_type;
	Widget default_button = NULL;
	Arg arg[5];
	int nargs;
	XmString title_string;
	XmString string;
	int i;
	Boolean bell;
	xm_MsgAdditionalInfo *xm_info;
	Widget parent;

	write_debug(XM_DEBUG_L1, "Entering xm_MsgFunction");

	/*
	 * Get the Motif info we need from the msg struct
	 */
	if (!msg_info) {
		UI_MsgResponseSet(UI_MSGRESPONSE_NONE);
		return;
	}
	xm_info = (xm_MsgAdditionalInfo *)msg_info->generic;

	/* make sure we can pop up a window or that we have a msg */
	write_debug(XM_DEBUG_L1, "Using %s",
		xm_info->parent ? "parent" : "toplevel");
	parent = xm_info->parent ? xm_info->parent : xm_info->toplevel;
	if (!parent || !XtIsRealized(parent)) {
		UI_MsgResponseSet(UI_MSGRESPONSE_NONE);
		return;
	}

	/* make sure we have a message to display */
	if (!msg_info->msg || *(msg_info->msg) == '\0') {
		UI_MsgResponseSet(UI_MSGRESPONSE_NONE);
		return;
	}

	/* Set up dialog args */
	dialog_title_def = UI_MsgGetDefaultTitle(msg_info->msg_type);
	bell = (Boolean) UI_MsgGetDefaultBeep(msg_info->msg_type);

	/* the title string */
	title_string = XmStringCreateLocalized(
		msg_info->title ?
		(char *) msg_info->title :
		(char *) dialog_title_def);

	/* the default button */
	switch ((int) msg_info->default_button) {
	case UI_MSGBUTTON_OK:
		default_button_type = XmDIALOG_OK_BUTTON;
		break;
	case UI_MSGBUTTON_OTHER1:
	case UI_MSGBUTTON_OTHER2:
		default_button_type = '\0';
		break;
	case UI_MSGBUTTON_CANCEL:
	case UI_MSGBUTTON_HELP:
	default:
		default_button_type = XmDIALOG_CANCEL_BUTTON;
		break;
	}

	/*
	 * Create the dialog.
	 *
	 * Recreate the dialog each time since we want to be able to bring
	 * up more than one at a time.
	 *
	 * We unmanage the message string and use a text widget
	 * instead of the XmString so that we can use the text
	 * formatting facilities of the text widget (i.e. which right now
	 * just means specifying XmNcolumns and relying on Motif to
	 * handle wrapping lines for us so we don't have to hard-code
	 * newlines).
	 *
	 * We attach the new text widget to the symbol label to get it where
	 * we want it.
	 */

	nargs = 0;
	XtSetArg(arg[nargs], XmNdialogStyle,
		XmDIALOG_FULL_APPLICATION_MODAL);
	nargs++;
	XtSetArg(arg[nargs], XmNdialogTitle, title_string);
	nargs++;
	XtSetArg(arg[nargs], XmNdeleteResponse, XmDO_NOTHING);
	nargs++;
	if (default_button_type != '\0') {
		XtSetArg(arg[nargs], XmNdefaultButtonType, default_button_type);
		nargs++;
	}

	/*
	 * Actualy create the dialog.
	 * Note: In the switch below I would prefer to just set a
	 * XmNdialogType variable and pass it as another arg to one
	 * XmCreateMessageDialog() call after the switch, but that
	 * doesn't seem to work, so i use the individual convenience
	 * functions here instead for each type of dialog.
	 */
	switch ((int) msg_info->msg_type) {
	case UI_MSGTYPE_ERROR:
		msg_dialog = XmCreateErrorDialog(parent,
			MSG_DIALOG_WIDGET_NAME, arg, nargs);
		break;
	case UI_MSGTYPE_INFORMATION:
		msg_dialog = XmCreateInformationDialog(parent,
			MSG_DIALOG_WIDGET_NAME, arg, nargs);
		break;
	case UI_MSGTYPE_MESSAGE:
		msg_dialog = XmCreateMessageDialog(parent,
			MSG_DIALOG_WIDGET_NAME, arg, nargs);
		break;
	case UI_MSGTYPE_QUESTION:
		msg_dialog = XmCreateQuestionDialog(parent,
			MSG_DIALOG_WIDGET_NAME, arg, nargs);
		break;
	case UI_MSGTYPE_WORKING:
		msg_dialog = XmCreateWorkingDialog(parent,
			MSG_DIALOG_WIDGET_NAME, arg, nargs);
		break;
	case UI_MSGTYPE_WARNING:
	default:
		msg_dialog = XmCreateWarningDialog(parent,
			MSG_DIALOG_WIDGET_NAME, arg, nargs);
		break;
	}

	XmStringFree(title_string);

	label = XmMessageBoxGetChild(msg_dialog, XmDIALOG_MESSAGE_LABEL);
	symbol = XmMessageBoxGetChild(msg_dialog, XmDIALOG_SYMBOL_LABEL);
	XtUnmanageChild(label);

	(void) XtVaCreateManagedWidget("text",
		xmTextWidgetClass, msg_dialog,
		XmNresizeHeight, True,
		XmNeditMode, XmMULTI_LINE_EDIT,
		XmNeditable, False,
		XmNautoShowCursorPosition, False,
		XmNcursorPositionVisible, False,
		XmNspacing, 5,
		XmNshadowThickness, 0,
		XmNtraversalOn, False,
		XmNwordWrap, True,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, symbol,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, symbol,
		XmNvalue, msg_info->msg,
		NULL);

	/* function to call if they chose close from window manager */
	XmAddWMProtocolCallback(xm_GetShell(msg_dialog),
		xm_info->delete_atom,
		xm_info->delete_func ?
			(XtCallbackProc) xm_info->delete_func :
			(XtCallbackProc) _delete_func,
		NULL);


	/*
	 * manage the correct btns, with the correct text in the
	 * button
	 */
	if (!msg_info->btns) {
		UI_MsgResponseSet(UI_MSGRESPONSE_NONE);
		return;
	}
	for (i = 0; i < UI_MSGBUTTON_MAX; i++) {
		switch ((int) msg_info->btns[i].button) {
		case UI_MSGBUTTON_OK:
			button = XmMessageBoxGetChild(msg_dialog,
				XmDIALOG_OK_BUTTON);
			if (msg_info->btns[i].button_text)
				XtManageChild(button);
			else {
				XtUnmanageChild(button);
				break;
			}
			string = XmStringCreateLocalized(
				msg_info->btns[i].button_text);
			XtVaSetValues(msg_dialog,
				XmNokLabelString, string,
				NULL);
			XmStringFree(string);
			break;
		case UI_MSGBUTTON_CANCEL:
			button = XmMessageBoxGetChild(msg_dialog,
				XmDIALOG_CANCEL_BUTTON);
			if (msg_info->btns[i].button_text)
				XtManageChild(button);
			else {
				XtUnmanageChild(button);
				break;
			}
			string = XmStringCreateLocalized(
				msg_info->btns[i].button_text);
			XtVaSetValues(msg_dialog,
				XmNcancelLabelString, string,
				NULL);
			XmStringFree(string);
			break;
		case UI_MSGBUTTON_OTHER1:
		case UI_MSGBUTTON_OTHER2:
			/*
			 * As additional buttons, in Motif 1.2 and
			 * later, other1 and other2, are automatically
			 * added after the Ok button.
			 *
			 * For these buttons, we can't use the
			 * defaultButtonType attribute to set the
			 * default button, so for these two we actually
			 * do a XmProcessTraversal call to get them set up
			 * as the default button.
			 */

			/* no text == no button */
			if (!msg_info->btns[i].button_text)
				break;

			/* button label */
			string = XmStringCreateLocalized(
				msg_info->btns[i].button_text);

			/* create the additional button */
			button = XtVaCreateManagedWidget(
				(msg_info->btns[i].button ==
					UI_MSGBUTTON_OTHER1) ?
					"other1" : "other2",
				xmPushButtonWidgetClass, msg_dialog,
				XmNlabelString, string,
				NULL);

			/*
			 * if requested, save this as the default button -
			 * we will do XmProcessTraversal on it after
			 * the dialog is managed.
			 */
			if (msg_info->btns[i].button ==
				msg_info->default_button) {
				default_button = button;
			}

			/* set up the button callback */
			if (msg_info->btns[i].button == UI_MSGBUTTON_OTHER1) {
				write_debug(XM_DEBUG_L1, "creating other1");
				_xm_MsgClientData.other1_btn = button;
				_xm_MsgClientData.other2_btn = NULL;
			} else {
				write_debug(XM_DEBUG_L1, "creating other2");
				_xm_MsgClientData.other2_btn = button;
				_xm_MsgClientData.other1_btn = NULL;
			}
			XtAddCallback(button,
				XmNactivateCallback, xm_MsgResponseCB,
				(XtPointer) &_xm_MsgClientData);
			break;
		case UI_MSGBUTTON_HELP:
			button = XmMessageBoxGetChild(msg_dialog,
				XmDIALOG_HELP_BUTTON);
			if (msg_info->btns[i].button_text)
				XtManageChild(button);
			else {
				XtUnmanageChild(button);
				break;
			}
			string = XmStringCreateLocalized(
				msg_info->btns[i].button_text);
			XtVaSetValues(msg_dialog,
				XmNhelpLabelString, string,
				NULL);
			XmStringFree(string);
			break;
		default:
			UI_MsgResponseSet(UI_MSGRESPONSE_NONE);
			return;
		}
	}

	/* set up callback  data for all btns */
	UI_MsgResponseSet(UI_MSGRESPONSE_NONE);
	_xm_MsgClientData.text = msg_info->help_topic;
	_xm_MsgClientData.xm_info.toplevel = xm_info->toplevel;
	_xm_MsgClientData.xm_info.parent = xm_info->parent;
	_xm_MsgClientData.xm_info.app_context = xm_info->app_context;

	/* set up callbacks for all 'regular' btns (ok, cancel, help) */
	XtAddCallback(msg_dialog, XmNokCallback, xm_MsgResponseCB,
		(XtPointer) &_xm_MsgClientData);
	XtAddCallback(msg_dialog, XmNcancelCallback, xm_MsgResponseCB,
			(XtPointer) &_xm_MsgClientData);
	XtAddCallback(msg_dialog, XmNhelpCallback, xm_MsgResponseCB,
			(XtPointer) &_xm_MsgClientData);

	/* bring up the dialog */
	XtManageChild(msg_dialog);

	/*
	 * if it's an 'other' button and it should be the default -
	 * set up the process traversal.
	 */
	if (default_button != NULL) {
		(void) XmProcessTraversal(default_button, XmTRAVERSE_CURRENT);
	}

	/* dialog is not resizable */
	xm_SetNoResize(xm_info->toplevel, msg_dialog);

	if (bell)
		XBell(XtDisplay(msg_dialog), 0);

	/* event loop for dialog */
	xm_MsgEventLoop(msg_dialog, xm_info);

	/* the user has responded, pop the dialog down */
	XtUnmanageChild(msg_dialog);
	XSync(XtDisplay(msg_dialog), 0);
	XmUpdateDisplay(msg_dialog);
	XtDestroyWidget(msg_dialog);

	/* reset parent info to null each time so the app doesn't have to */
	xm_info->parent = NULL;
}

/*
 * Function: xm_MsgEventLoop
 * Description:
 *	Sub-event loop for the dialog.
 * Scope:       PRIVATE
 * Parameters:
 *	dialog - the dialog widget
 *	xm_info - the generic, additional info registered with the
 *		  dialog request.
 * Return:	[none]
 * Notes:
 */
static void
xm_MsgEventLoop(Widget dialog, xm_MsgAdditionalInfo *xm_info)
{
	while (UI_MsgResponseGet() == UI_MSGRESPONSE_NONE) {
		XtAppProcessEvent(xm_info->app_context, XtIMAll);
		XSync(XtDisplay(dialog), 0);
	}
}

/*
 * Function:xm_MsgResponseCB
 * Description:
 *	Handles all the user responses to the dialog.
 * Scope:       PRIVATE
 * Parameters:  std X XB params
 * Return:	none
 */
/* ARGSUSED */
static void
xm_MsgResponseCB(Widget w, XtPointer client, XmAnyCallbackStruct *cbs)
{

	/* LINTED [pointer cast] */
	xm_MsgClientData *client_data = (xm_MsgClientData *) client;
	xm_HelpClientData *help_client;
	xm_MsgAdditionalInfo *xm_info =
		(xm_MsgAdditionalInfo *) &client_data->xm_info;

	switch (cbs->reason) {
	case XmCR_HELP:
		write_debug(XM_DEBUG_L1, "%s", "Help");
		/* LINTED [pointer cast] */
		help_client = (xm_HelpClientData *) XtMalloc(
			sizeof (xm_HelpClientData));
		help_client->toplevel = xm_info->toplevel;
		help_client->text = client_data->text;
		xm_HelpCB(w,
			(XtPointer) help_client,
			(XmAnyCallbackStruct *)NULL);
		UI_MsgResponseSet(UI_MSGRESPONSE_NONE);
		break;
	case XmCR_CANCEL:
		write_debug(XM_DEBUG_L1, "Message response = %s", "XmCancel");
		UI_MsgResponseSet(UI_MSGRESPONSE_CANCEL);
		break;
	case XmCR_OK:
		write_debug(XM_DEBUG_L1, "Message Response = %s", "XmOk");
		UI_MsgResponseSet(UI_MSGRESPONSE_OK);
		break;
	default:
		/*
		 * It's one of the 'other' buttons.
		 * Figure out which one and return the right response.
		 */
		if (w == client_data->other1_btn)
			UI_MsgResponseSet(UI_MSGRESPONSE_OTHER1);
		else
			UI_MsgResponseSet(UI_MSGRESPONSE_OTHER2);
		break;
	}
}

/*
 * Function: _delete_func
 * Description:
 *	Default function to call if the user choses quit from the
 *	window manager.  This one just exits the dialog.
 *	The app may register (in the additional info) a function to
 *	verify and exit the entire app if they want.
 * Scope:       PRIVATE
 * Parameters:  none
 * Return:	none
 */
static void
_delete_func(void)
{
	UI_MsgResponseSet(UI_MSGRESPONSE_CANCEL);
}
