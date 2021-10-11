#ifndef lint
#pragma ident "@(#)inst_systype.c 1.53 96/06/21 SMI"
#endif

/*
 * Copyright (c) 1993-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	inst_systype.c
 * Group:	ttinstall
 * Description:
 */

#include <stdio.h>
#include <limits.h>
#include <locale.h>
#include <libintl.h>
#include <string.h>

#include "pf.h"
#include "tty_pfc.h"
#include "inst_msgs.h"
#include "v_types.h"
#include "v_misc.h"
#include "v_sw.h"

/*
 * returns:
 *	0 - exit
 *	1 - continue
 *	-1 - go back
 */
parAction_t
do_systype(void)
{
	int ch;
	int row;
	u_int fkeys;
	HelpEntry _help;

	_help.win = stdscr;
	_help.type = HELP_TOPIC;
	_help.title = "Allocating Client Services";

	(void) werase(stdscr);
	(void) wclear(stdscr);
	wheader(stdscr, TITLE_ALLOCATE_SVC_QUERY);

	row = HeaderLines;
	row = wword_wrap(stdscr, row, INDENT0, COLS - (2 * INDENT0),
	    MSG_ALLOCATE_SVC_QUERY);
	++row;

	fkeys = (F_CONTINUE | F_GOBACK | F_ALLOCATE | F_EXIT | F_HELP);

	wfooter(stdscr, fkeys);

	for (;;) {
		ch = wzgetch(stdscr, fkeys);

		if (is_exit(ch) != 0)
			return (parAExit);
		else if (is_continue(ch) != 0) {
			v_set_system_type(V_STANDALONE);
			wstatus_msg(stdscr, PLEASE_WAIT_STR);
			return (parAContinue);
		} else if (is_allocate(ch) != 0) {
			v_set_system_type(V_SERVER);
			wstatus_msg(stdscr, PLEASE_WAIT_STR);
			return (parAContinue);
		} else if (is_help(ch)) {
			do_help_index(stdscr, _help.type, _help.title);
		} else if (is_goback(ch) != 0) {
			return (parAGoback);
		} else
			beep();
	}
}
