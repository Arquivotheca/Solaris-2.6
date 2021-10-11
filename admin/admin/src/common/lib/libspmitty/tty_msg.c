#ifndef lint
#pragma ident "@(#)tty_msg.c 1.6 96/07/30 SMI"
#endif

/*
 * Copyright (c) 1993-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	tty_msg.c
 * Group:	libspmitty
 * Description:
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <libintl.h>

#include "spmitty_api.h"
#include "tty_utils.h"

/*
 * Function: tty_MsgFunction
 * Description:
 *	Top level curses based messaging function
 *	All messages (error, warning, etc. should come through here).
 *	General usage paradigm is that the app will register this
 *	function with a higher level app layer. which will call this when a
 *	message screen is desired.
 *	Can also be invoked directly from the app through.
 *
 *	Puts up a message and waits on the answer from the user.
 *	The answer is stored and is obtainable after the call
 *	via a UI_MsgResponseGet() call.
 * Scope:	PUBLIC
 * Parameters:
 *	msg_info - a structure defining the message to be presented.
 * Return:	none
 * Globals:
 * Notes:
 *	The tty_MsgAdditionalInfo set up by the app MUST be set up such
 *	that the dialog_fkeys are indexed with the values of the enum
 *	type UI_MsgButton and the fkeys are ordered F2-F6.
 */
void
tty_MsgFunction(UI_MsgStruct *msg_info)
{
	WINDOW *win;
	UI_MsgResponseType response;
	int ch;
	int row;
	int nlines;
	int beep;
	int fkeys;
	int i;
	char *dialog_title_def;
	char *str;
	tty_MsgAdditionalInfo *tty_info;
	WINDOW *parent;
	u_long fkey;
	char *tmp_funcs[UI_MSGBUTTON_MAX];

	write_debug(TTY_DEBUG_L1, "tty_MsgFunction");

	/*
	 * Get the tty info we need from the msg struct
	 */
	if (!msg_info) {
		UI_MsgResponseSet(UI_MSGRESPONSE_NONE);
		return;
	}
	tty_info = (tty_MsgAdditionalInfo *)msg_info->generic;

	/* make sure we have a msg */
	if (!msg_info->msg || *(msg_info->msg) == '\0') {
		UI_MsgResponseSet(UI_MSGRESPONSE_NONE);
		return;
	}

	if (!tty_info) {
		UI_MsgResponseSet(UI_MSGRESPONSE_NONE);
		return;
	}

	/*
	 * If no parent specified - use stdscr.
	 */
	if (tty_info->parent)
		parent = tty_info->parent;
	else
		parent = stdscr;

	/* get the default title */
	dialog_title_def = UI_MsgGetDefaultTitle(msg_info->msg_type);
	beep = UI_MsgGetDefaultBeep(msg_info->msg_type);

	/*
	 * Set up the correct function keys.
	 * In the CUI we don't do anything with default buttons.
	 */
	if (!msg_info->btns) {
		UI_MsgResponseSet(UI_MSGRESPONSE_NONE);
		return;
	}
	fkeys = 0;
	for (i = 0; i < UI_MSGBUTTON_MAX; i++) {
		str = msg_info->btns[i].button_text;
		if (!str)
			continue;

		if (msg_info->btns[i].button == UI_MSGBUTTON_HELP)
			continue;

		fkey = tty_info->dialog_fkeys[i];
		fkeys |= fkey;
		tmp_funcs[i] = wfooter_func_get(fkey);
		wfooter_func_set(fkey, str);
	}

	/* set up colors, etc. for the message */
	win = newwin(LINES, COLS, 0, 0);
	wcolor_set_bkgd(win, BODY);
	(void) keypad(win, 1);

	/* clear the window */
	(void) werase(win);
	(void) wclear(win);

	/* show title */
	wheader(win,
		msg_info->title ? msg_info->title : dialog_title_def);

	/* show the text */
	nlines = count_lines(msg_info->msg, COLS - (2 * INDENT1));
	row = (LINES - nlines - 2) / 2;

	(void) wword_wrap(win, row, INDENT1, COLS - (2 * INDENT1),
		msg_info->msg);

	/* show the footer (button selections, etc); */
	wfooter(win, fkeys);
	wcursor_hide(win);

	/* beep at them */
	if (beep)
		beep();

	/* this is the curses event loop */
	for (;;) {
		ch = wzgetch(win, fkeys);

		if (is_fkey_num(ch, 2) &&
			msg_info->btns[UI_MSGRESPONSE_OK].button_text) {
			response = UI_MSGRESPONSE_OK;
			break;
		} else if (is_fkey_num(ch, 3) &&
			msg_info->btns[UI_MSGBUTTON_OTHER1].button_text) {
			response = UI_MSGRESPONSE_OTHER1;
			break;
		} else if (is_fkey_num(ch, 4) &&
			msg_info->btns[UI_MSGBUTTON_OTHER2].button_text) {
			response = UI_MSGRESPONSE_OTHER2;
			break;
		} else if (is_fkey_num(ch, 5) &&
			msg_info->btns[UI_MSGBUTTON_CANCEL].button_text) {
			response = UI_MSGRESPONSE_CANCEL;
			break;
		} else if (is_fkey_num(ch, 6) &&
			msg_info->btns[UI_MSGBUTTON_HELP].button_text) {
			/* need to just plug into adminhelp  - beep for now */
			beep();
			;
		} else {
			beep();
		}
	}

	/* we're done - get rid of the msg window */
	(void) delwin(win);

	/* refresh the parent window */
	if (parent != (WINDOW *) NULL) {
		(void) clearok(curscr, TRUE);
		(void) touchwin(parent);
		(void) wnoutrefresh(parent);
		(void) clearok(curscr, FALSE);
	}

	/* reset the footer labels back to normal before we leave */
	for (i = 0; i < UI_MSGBUTTON_MAX; i++) {
		str = msg_info->btns[i].button_text;
		if (!str)
			continue;

		if (msg_info->btns[i].button == UI_MSGBUTTON_HELP)
			continue;

		fkey = tty_info->dialog_fkeys[i];
		wfooter_func_set(fkey, tmp_funcs[i]);
	}

	/* tell the backend what the user's response was */
	UI_MsgResponseSet(response);
}

#ifdef UNIT_TEST
/*
 * Msg Unit tests
 */
void
tty_MsgUnitTest(void)
{
	UI_MsgStruct *msg_info;
	UI_MsgType msg_type;
	tty_MsgAdditionalInfo *tty_info;

	write_debug(TTY_DEBUG_L1, "Message Testing");

	/* register the tty msg function with the ui msg stuff */
	UI_MsgFuncRegister(tty_MsgFunction);

	/*
	 * Set up curses specific info that the tty_MsgFunction will need
	 * and add it to the msg struct.
	 */
	tty_info =
		(tty_MsgAdditionalInfo *)
			xmalloc(sizeof (tty_MsgAdditionalInfo));
	tty_info->parent = stdscr;
	UI_MsgGenericInfoSet((void *)tty_info);

	/* set up a msg info struct to send to the message request */
	msg_info = UI_MsgStructInit();

	UI_MsgUnitTest(msg_info);

	/* free the message structure now that you're done it */
	UI_MsgStructFree(msg_info);
}
#endif
