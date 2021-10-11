#ifndef lint
#pragma ident "@(#)spmiapp_ui_msg.h 1.4 96/06/17 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	spmiapp_ui_msg.h
 * Group:	libspmiapp
 * Description:
 *	Header file for common message dialogs.
 *	Using this module in conjunction with registering functions  for
 *	different display paradigms (e.g. Motif/curses), you can use
 *	the UI_Msg* functions as a common interface for generating
 *	basic dialogs.
 */

#ifndef _SPMIAPP_UI_MSG_H
#define	_SPMIAPP_UI_MSG_H

#include "spmiapp_api.h"	/* make sure we get LIBAPPSTR definition */

/* ***************************************************************
 *	UI Message Dialogs
 * *************************************************************** */

/*
 * Common (default) title strings for misc windows - msg dialogs/help. etc.
 */
#define	UI_TITLE_EXIT		LIBAPPSTR("Exit")
#define	UI_TITLE_ERROR		LIBAPPSTR("Error")
#define	UI_TITLE_INFORMATION	LIBAPPSTR("Information")
#define	UI_TITLE_MESSAGE	LIBAPPSTR("Message")
#define	UI_TITLE_QUESTION	LIBAPPSTR("Question")
#define	UI_TITLE_WORKING	LIBAPPSTR("Working")
#define	UI_TITLE_WARNING	LIBAPPSTR("Warning")

#define	UI_TITLE_HELP		LIBAPPSTR("Help")
#define	UI_TITLE_MSG_GENERIC	LIBAPPSTR("Message")

/*
 * Common Button Strings
 */
#define	UI_BUTTON_OK_STR	LIBAPPSTR("OK")
#define	UI_BUTTON_CANCEL_STR	LIBAPPSTR("Cancel")
#define	UI_BUTTON_OTHER1_STR	LIBAPPSTR("Other1")
#define	UI_BUTTON_OTHER2_STR	LIBAPPSTR("Other2")
#define	UI_BUTTON_CONTINUE_STR	LIBAPPSTR("Continue")
#define	UI_BUTTON_EXIT_STR	LIBAPPSTR("Exit")
#define	UI_BUTTON_HELP_STR	LIBAPPSTR("Help")


/*
 * The types of messages that can be requested.
 */
typedef enum {
	UI_MSGTYPE_ERROR = 0,
	UI_MSGTYPE_INFORMATION,
	UI_MSGTYPE_MESSAGE,
	UI_MSGTYPE_QUESTION,
	UI_MSGTYPE_WARNING,
	UI_MSGTYPE_WORKING,

	UI_MSGTYPE_MAX
} UI_MsgType;

/*
 * The types of btns that are possible on any given message.
 */
typedef enum {
	UI_MSGBUTTON_OK = 0,
	UI_MSGBUTTON_OTHER1,
	UI_MSGBUTTON_OTHER2,
	UI_MSGBUTTON_CANCEL,
	UI_MSGBUTTON_HELP,

	UI_MSGBUTTON_MAX
} UI_MsgButton;

/*
 * The possible message responses that might get returned.
 */
typedef enum {
	UI_MSGRESPONSE_OK,
	UI_MSGRESPONSE_OTHER1,
	UI_MSGRESPONSE_OTHER2,
	UI_MSGRESPONSE_CANCEL,

	/*
	 * these 2 are useful in the apps, but
	 * don't ever expect this to actually get returned...
	 */
	UI_MSGRESPONSE_HELP,
	UI_MSGRESPONSE_NONE
} UI_MsgResponseType;

/*
 * sub-structure for defining one button label in a message dialog request
 */
typedef struct _UI_MsgButtonStruct {
	UI_MsgButton button;
	char *button_text;
} UI_MsgButtonStruct;

/*
 * The main structure used to describe a message dialog request.
 */
typedef struct _UI_MsgStruct {
	UI_MsgType msg_type;
	UI_MsgButton	default_button;	/* not relevant in curses interface */
	UI_MsgButtonStruct btns[UI_MSGBUTTON_MAX];
	char *title;
	char *msg;
	char *help_topic;

	/*
	 * Provide a spot for any application level data -
	 * (i.e. they might want to store the error code somewhere).
	 */
	void *generic;
} UI_MsgStruct;

/* function typedef for function an app would register with this module */
typedef void (*UI_MsgFuncType) (UI_MsgStruct *msg_info);

/* functional prototypes */

#ifdef __cplusplus
extern "C" {
#endif

/* ui_state.c */
extern void UI_DebugSet(int flag);
extern int UI_DebugGet(void);

/* ui_msg.c */
extern void UI_MsgResponseSet(UI_MsgResponseType response);
extern UI_MsgResponseType UI_MsgResponseGet(void);
extern char * UI_MsgResponseStr(UI_MsgResponseType msg_type);
extern UI_MsgStruct * UI_MsgStructInit(void);
extern void UI_MsgStructFree(UI_MsgStruct *msg_info);
extern void UI_MsgFuncRegister(UI_MsgFuncType msgfunc);
extern UI_MsgResponseType UI_MsgFunction(UI_MsgStruct *msg_info);
extern void UI_MsgGenericInfoSet(void *generic_info);
extern void *UI_MsgGenericInfoGet(void);
extern int UI_MsgGetDefaultBeep(UI_MsgType msg_type);
extern char *UI_MsgGetDefaultTitle(UI_MsgType msg_type);
extern UI_MsgResponseType UI_DisplayBasicMsg(UI_MsgType msg_type,
	char *title, char *message);
extern void UI_MsgUnitTest(void);

#ifdef __cplusplus
}
#endif

#endif /* _SPMIAPP_UI_MSG_H */
