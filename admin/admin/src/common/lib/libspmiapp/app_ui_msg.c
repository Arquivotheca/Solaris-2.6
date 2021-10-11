#ifndef lint
#pragma ident "@(#)app_ui_msg.c 1.5 96/09/03 SMI"
#endif

/*
 * Copyright (c) 1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	app_ui_msg.c
 * Group:	libspmiapp
 * Description:
 *	Using this module in conjunction with registering functions  for
 *	different display paradigms (e.g. Motif/curses), you can use
 *	the UI_Msg* functions as a common interface for generating
 *	basic dialogs.
 *
 *	Below is an example of how code that uses this module would look
 *	for a Motif application:
 *	There is also a unit test routine that shows some more examples
 *	at the end of this file.
 *
 *	// declare a message structure
 *	UI_MsgStruct *msg_info;
 *
 *	// set up ui msg function infrastructure
 *	xmInfo.parent = NULL;
 *	xmInfo.toplevel = pfgTopLevel;
 *	xmInfo.app_context = pfgAppContext;
 *	xmInfo.delete_atom = pfgWMDeleteAtom;
 *	xmInfo.delete_func = pfgExit;
 *	UI_MsgGenericInfoSet((void *)&xmInfo);
 *	UI_MsgFuncRegister(xm_MsgFunction);
 *
 *	// set up a msg info struct to send to the message request
 *	msg_info = UI_MsgStructInit();
 *
 *	// print a message with all the defaults...
 *	msg_info->help_topic = "diskname.t";
 *	msg_info->msg = "This is a default test.";
 *	UI_MsgFunction(msg_info);
 *	write_debug(LOG, 1, "LIBSPMI_APP", DEBUG_LOC, LEVEL1,
 *		"Msg Response = %s",
 *		UI_MsgResponseStr(UI_MsgResponseGet()));
 */

#include <stdio.h>
#include <malloc.h>

#include "spmiapp_api.h"
#include "app_utils.h"

/* static globals */
static UI_MsgResponseType _ui_msgresponse = UI_MSGRESPONSE_NONE;
static UI_MsgFuncType _ui_msgfunc = NULL;
static void *_ui_generic_info = NULL;

/*
 * Function: UI_MsgResponseSet
 * Description:
 *	Record the response that the user entered.
 *	The 'event loop' in the msg_function would register this
 *	when the user pushes a button or whatever.
 * Scope:	PUBLIC
 * Parameters:
 *	msg_type - RO
 *		The user's response.
 * Return:	none
 */
void
UI_MsgResponseSet(UI_MsgResponseType msg_type)
{
	write_debug(APP_DEBUG_L1,
		"Entering UI_MsgResponseSet = %d", msg_type);
	_ui_msgresponse = msg_type;
}

/*
 * Function: UI_MsgResponseGet
 * Description:
 *	Get the values of the response the user just made.
 *	i.e. after the msg_function returns, the app can request
 *	to know what the user hit (ok, cancel)
 *
 * Scope:	PUBLIC
 * Parameters:  none
 * Return:	[UI_MsgResponseType]
 */
UI_MsgResponseType
UI_MsgResponseGet(void)
{
	return (_ui_msgresponse);
}

/*
 * Function: UI_MsgResponseStr
 * Description:
 *	Just return a default string value that corresponds to each
 *	possible UI_MsgResponseType.
 *	This is probably most useful for debugging purposes.
 * Scope: PUBLIC
 * Parameters:  msg_type
 * Return:	[char *]
 *		NULL - unknown msg_type passed in
 * Globals:	<name> - [<RO|RW|WO>][<GLOBAL|MODULE|SEGMENT>]
 * Notes:
 */
char *
UI_MsgResponseStr(UI_MsgResponseType msg_type)
{
	switch (msg_type) {
	case UI_MSGRESPONSE_OK:
		return (UI_BUTTON_OK_STR);
		/* NOTREACHED */
		break;
	case UI_MSGRESPONSE_CANCEL:
		return (UI_BUTTON_CANCEL_STR);
		/* NOTREACHED */
		break;
	case UI_MSGRESPONSE_OTHER1:
		return (UI_BUTTON_OTHER1_STR);
		/* NOTREACHED */
		break;
	case UI_MSGRESPONSE_OTHER2:
		return (UI_BUTTON_OTHER2_STR);
		/* NOTREACHED */
		break;
	case UI_MSGRESPONSE_HELP:
		return (UI_BUTTON_HELP_STR);
		/* NOTREACHED */
		break;
	case UI_MSGRESPONSE_NONE:
	default:
		return (NULL);
	}
}

/*
 * Function: UI_MsgStructInit
 * Description:
 *	Create space for one msg structure and initialize with
 *	reasonable default values:
 *	- a message dialog
 *	- with ok/cancel/help buttons
 *	- without the other1 or other2 buttons
 *	- ok button is default
 *	- title, text, and help topic are not filled in
 * Scope:	PUBLIC
 * Parameters:  none
 * Return:	[UI_MsgStruct *]
 *		NULL - error creating structure.
 */
UI_MsgStruct *
UI_MsgStructInit(void)
{
	UI_MsgStruct *msg_info;

	msg_info = (UI_MsgStruct *) calloc(1, sizeof (UI_MsgStruct));
	if (!msg_info)
		return (NULL);

	msg_info->msg_type = UI_MSGTYPE_MESSAGE;
	msg_info->default_button = UI_MSGBUTTON_CANCEL;
	msg_info->btns[UI_MSGBUTTON_OK].button = UI_MSGBUTTON_OK;
	msg_info->btns[UI_MSGBUTTON_OK].button_text = UI_BUTTON_OK_STR;
	msg_info->btns[UI_MSGBUTTON_CANCEL].button = UI_MSGBUTTON_CANCEL;
	msg_info->btns[UI_MSGBUTTON_CANCEL].button_text = UI_BUTTON_CANCEL_STR;
	msg_info->btns[UI_MSGBUTTON_HELP].button = UI_MSGBUTTON_HELP;
	msg_info->btns[UI_MSGBUTTON_HELP].button_text = UI_BUTTON_HELP_STR;

	/* by default the other buttons are not shown */
	msg_info->btns[UI_MSGBUTTON_OTHER1].button = UI_MSGBUTTON_OTHER1;
	msg_info->btns[UI_MSGBUTTON_OTHER1].button_text = NULL;
	msg_info->btns[UI_MSGBUTTON_OTHER2].button = UI_MSGBUTTON_OTHER2;
	msg_info->btns[UI_MSGBUTTON_OTHER2].button_text = NULL;

	msg_info->title = NULL;
	msg_info->msg = NULL;
	msg_info->help_topic = NULL;

	msg_info->generic = _ui_generic_info;

	return (msg_info);
}

/*
 * Function: UI_MsgStructFree
 * Description:
 *	Free the message dialog structure.
 * Scope:	PUBLIC
 * Parameters:  msg_info
 * Return:	none
 */
void
UI_MsgStructFree(UI_MsgStruct *msg_info)
{
	if (!msg_info)
		return;

	/* free it */
	free(msg_info);
}

/*
 * Function: UI_MsgFuncRegister
 * Description:
 *	Register the function that will actually implement the
 *	message dialog (e.g. in Motif or curses).
 *	This registered function is the workhorse of the whole module
 *	and is responsible for displaying the dialog and recording
 *	the user's answer.
 * Scope:	PUBLIC
 * Parameters:  msgfunc - function to register.
 * Return:	none
 */
void
UI_MsgFuncRegister(UI_MsgFuncType msgfunc)
{
	_ui_msgfunc = msgfunc;
}

/*
 * Function: UI_MsgFunction
 * Description:
 *	This the function an app will actually call to request that a
 *	message dialog be displayed.
 * Scope:	PUBLIC
 * Parameters:  msg_info - a properly initialized msg dialog structure
 * Return:	[UI_MsgResponseType]
 */
UI_MsgResponseType
UI_MsgFunction(UI_MsgStruct *msg_info)
{
	if (!_ui_msgfunc) {
		write_status(LOG, LEVEL0, msg_info->msg);
		UI_MsgResponseSet(UI_MSGRESPONSE_NONE);
	} else {
		(*_ui_msgfunc) (msg_info);
	}

	return (UI_MsgResponseGet());
}

/*
 * Function: UI_MsgGenericInfoSet
 * Description:
 *	Register a piece of 'generic' information that the
 *	implementation dependent registered function may
 *	require to do its job.  Register this prior to initializing
 *	the msg structure and it will be added to the msg structure.
 *	e.g. a motif msg function might need
 *	some toplevel/parent widget data passed into it.
 * Scope:	PUBLIC
 * Parameters:  generic_info - void pointer to data
 * Return:	none
 */
void
UI_MsgGenericInfoSet(void *generic_info)
{
	_ui_generic_info = generic_info;
}

/*
 * Function: UI_MsgGenericInfoGet
 * Description:
 *	Return the registered piece of 'generic' information that
 *	may have been set by the application using UI_MsgGenericInfoSet().
 * Scope:	PUBLIC
 * Parameters:  none
 * Return:	void *
 */
void *
UI_MsgGenericInfoGet(void)
{
	return (_ui_generic_info);
}

/*
 * Function: UI_MsgGetDefaultTitle
 * Description:
 *	Return a reasonable default title string for each type
 *	of msg dialog. This should be used if a title is not
 *	defined in the msg struct.
 * Scope:	PUBLIC
 * Parameters:  msg_type
 * Return:	[char *]
 * Notes:
 *	If an unknown type is passed in - it is treated like
 *	an information dialog type.
 */
char *
UI_MsgGetDefaultTitle(UI_MsgType msg_type)
{
	char *default_title;

	switch ((int) msg_type) {
	case UI_MSGTYPE_ERROR:
		default_title = UI_TITLE_ERROR;
		break;
	case UI_MSGTYPE_INFORMATION:
		default_title = UI_TITLE_INFORMATION;
		break;
	case UI_MSGTYPE_MESSAGE:
		default_title = UI_TITLE_MESSAGE;
		break;
	case UI_MSGTYPE_QUESTION:
		default_title = UI_TITLE_QUESTION;
		break;
	case UI_MSGTYPE_WARNING:
		default_title = UI_TITLE_WARNING;
		break;
	case UI_MSGTYPE_WORKING:
		default_title = UI_TITLE_WORKING;
		break;
	default:
		/* matched default dialog type */
		default_title = UI_TITLE_INFORMATION;
	}

	return (default_title);
}

/*
 * Function: UI_MsgGetDefaultBeep
 * Description:
 *	Return if by default, this type of msg dialog should
 *	'beep' the user when it presents itself.
 * Scope:	PUBLIC
 * Parameters:  msg_type
 * Return:	[int]
 *		1 - beep
 *		0 - no beep
 */
int
UI_MsgGetDefaultBeep(UI_MsgType msg_type)
{
	int beep;

	switch ((int) msg_type) {
	case UI_MSGTYPE_ERROR:
	case UI_MSGTYPE_WARNING:
		beep = 1;
		break;
	case UI_MSGTYPE_INFORMATION:
	case UI_MSGTYPE_MESSAGE:
	case UI_MSGTYPE_QUESTION:
	case UI_MSGTYPE_WORKING:
		beep = 0;
		break;
	default:
		beep = 1;
	}

	return (beep);
}

/*
 * Function: UI_DisplayBasicMsg
 * Description:
 *	Just another wrapper around the UI message stuff to make it
 *	easy to pop up ok/cancel dialogs of different types.
 *
 * Scope:	PUBLIC
 * Parameters:
 *	msg_type:
 *		what type of message is this
 *	title:
 *		screen title
 *	message:
 *		message dialog text
 *
 * Return:
 *	UI_MsgResponseType: what the user responded (ok or cancel)
 * Globals:	none
 * Notes:
 */
UI_MsgResponseType
UI_DisplayBasicMsg(UI_MsgType msg_type, char *title, char *message)
{
	UI_MsgStruct *msg_info;

	/* set up the message */
	msg_info = UI_MsgStructInit();
	msg_info->msg_type = msg_type;
	msg_info->title = title;
	msg_info->msg = message;
	msg_info->btns[UI_MSGBUTTON_OK].button_text = UI_BUTTON_OK_STR;
	msg_info->btns[UI_MSGBUTTON_CANCEL].button_text = UI_BUTTON_CANCEL_STR;
	msg_info->btns[UI_MSGBUTTON_HELP].button_text = NULL;
	msg_info->default_button = UI_MSGBUTTON_CANCEL;

	/* invoke the message */
	return (UI_MsgFunction(msg_info));
}

/*
 * Function: UI_MsgUnitTest
 * Description:
 *	Msg Unit tests
 * Scope:	PUBLIC
 * Parameters:
 * Return:	none
 */
void
UI_MsgUnitTest(void)
{
	UI_MsgStruct *msg_info;
	UI_MsgType msg_type;

	/* set up a msg info struct to send to the message request */
	msg_info = UI_MsgStructInit();

	/* print a message with all the defaults... */
	msg_info->help_topic = "diskname.t";
	msg_info->msg = "This is a default test.";
	(void) UI_MsgFunction(msg_info);
	write_debug(LOG, 1, "LIBSPMI_APP", DEBUG_LOC, LEVEL1,
		"Msg Response = %s",
		UI_MsgResponseStr(UI_MsgResponseGet()));

	/* a quick test of all the default msgs */
	msg_info->msg = "This is a default dialog-type test";
	for (msg_type = 0; msg_type < UI_MSGTYPE_MAX; msg_type++) {
		msg_info->msg_type = msg_type;
		(void) UI_MsgFunction(msg_info);

		write_debug(LOG, 1, "LIBSPMI_APP", DEBUG_LOC, LEVEL1,
			"Msg Response = %s",
			UI_MsgResponseStr(UI_MsgResponseGet()));
	}

	/* now do some customized tests */

	msg_info->msg = "Test the \"other\" buttons";
	msg_info->btns[UI_MSGBUTTON_OTHER1].button_text =
		UI_BUTTON_OTHER1_STR;
	msg_info->btns[UI_MSGBUTTON_OTHER2].button_text =
		UI_BUTTON_OTHER2_STR;
	msg_info->default_button = UI_MSGBUTTON_OTHER1;
	(void) UI_MsgFunction(msg_info);
	write_debug(LOG, 1, "LIBSPMI_APP", DEBUG_LOC, LEVEL1,
		"Msg Response = %s",
		UI_MsgResponseStr(UI_MsgResponseGet()));

	msg_info->default_button = UI_MSGBUTTON_OTHER2;
	msg_info->btns[UI_MSGBUTTON_OTHER1].button_text = "New1";
	msg_info->btns[UI_MSGBUTTON_OTHER2].button_text = "New2";
	(void) UI_MsgFunction(msg_info);
	write_debug(LOG, 1, "LIBSPMI_APP", DEBUG_LOC, LEVEL1,
		"Msg Response = %s",
		UI_MsgResponseStr(UI_MsgResponseGet()));

	msg_info->btns[UI_MSGBUTTON_OTHER1].button_text = NULL;
	msg_info->btns[UI_MSGBUTTON_OTHER2].button_text = NULL;
	msg_info->default_button = UI_MSGBUTTON_CANCEL;

	msg_info->btns[UI_MSGBUTTON_OK].button_text = UI_BUTTON_EXIT_STR;
	msg_info->btns[UI_MSGBUTTON_CANCEL].button_text = NULL;
	msg_info->btns[UI_MSGBUTTON_HELP].button_text = NULL;
	msg_info->msg = "Test having just one (ok) button.";
	msg_info->msg_type = UI_MSGTYPE_ERROR;
	(void) UI_MsgFunction(msg_info);
	write_debug(LOG, 1, "LIBSPMI_APP", DEBUG_LOC, LEVEL1,
		"Msg Response = %s",
		UI_MsgResponseStr(UI_MsgResponseGet()));

	msg_info->btns[UI_MSGBUTTON_OK].button_text = UI_BUTTON_CONTINUE_STR;
	msg_info->btns[UI_MSGBUTTON_CANCEL].button_text = UI_BUTTON_CANCEL_STR;
	msg_info->title = "A New Title";
	msg_info->msg =
		"Test buttons and using a non-default title bar.";
	msg_info->msg_type = UI_MSGTYPE_WARNING;
	(void) UI_MsgFunction(msg_info);
	write_debug(LOG, 1, "LIBSPMI_APP", DEBUG_LOC, LEVEL1,
		"Msg Response = %s",
		UI_MsgResponseStr(UI_MsgResponseGet()));

	msg_info->msg = "This is a long message and I want it to wrap \
rather than be really long and go all the way across the screen.\n\
I also want to see if tabs work:\n\
\ttab1 \n\
\ttab1\ttab2";
	(void) UI_MsgFunction(msg_info);
	write_debug(LOG, 1, "LIBSPMI_APP", DEBUG_LOC, LEVEL1,
		"Msg Response = %s",
		UI_MsgResponseStr(UI_MsgResponseGet()));

	/* free the message structure now that you're done it */
	UI_MsgStructFree(msg_info);
}
