#ifndef lint
#pragma ident "@(#)inst_intro.c 1.22 96/07/29 SMI"
#endif

/*
 * Copyright (c) 1991-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	inst_intro.c
 * Group:	ttinstall
 * Description:
 */

#include <stdio.h>
#include <limits.h>
#include <locale.h>
#include <libintl.h>
#include <string.h>

#include "spmitty_api.h"
#include "tty_pfc.h"

#include "pf.h"
#include "inst_msgs.h"

/*
 * returns:
 *	1 - continue
 *	0 - exit
 *	-1 - go back
 *	2 - skip parade...
 */
parAction_t
do_install_intro(parWin_t win)
{
	int ch;
	unsigned long fkeys;
	HelpEntry _help;
	char *title;
	char *msg;
	char *help_title;

	switch (win) {
	case parIntro:
		title = TITLE_INTRO;

		msg = (char *) xmalloc(
			strlen(MSG_INTRO) + strlen(MSG_INTRO_CUI_NOTE) + 1);
		(void) sprintf(msg, "%s%s", MSG_INTRO, MSG_INTRO_CUI_NOTE);

		help_title = "Navigating Using the Keyboard";
		break;
	case parIntroInitial:
		title = TITLE_INTRO_INITIAL;
		msg = MSG_INTRO_INITIAL;
		help_title = "Creating a Profile";
		break;
	}

	_help.win = stdscr;
	_help.type = HELP_TOPIC;
	_help.title = help_title;

	(void) werase(stdscr);
	(void) wclear(stdscr);

	wheader(stdscr, title);

	(void) wword_wrap(stdscr, HeaderLines, INDENT0, COLS - (2 * INDENT0),
	    msg);

	/*
	 * if there was a window before this one, then put up the go
	 * back button, and o/w skip it.
	 */
	if (parade_prev_win()) {
		fkeys = F_CONTINUE | F_GOBACK | F_EXIT | F_HELP;
	} else {
		fkeys = F_CONTINUE | F_EXIT | F_HELP;
	}

	wfooter(stdscr, fkeys);
	wcursor_hide(stdscr);

	for (;;) {
		ch = wzgetch(stdscr, fkeys);

		if (is_exit(ch)) {
			if (confirm_exit(stdscr))
				return (parAExit);
		} else if (is_continue(ch)) {
			return (parAContinue);
		} else if ((fkeys & F_GOBACK) && is_goback(ch)) {
			return (parAGoback);
		} else if (is_help(ch)) {
			do_help_index(_help.win, _help.type, _help.title);
		} else if (is_escape(ch)) {
			continue;
		} else
			beep();
	}
}
