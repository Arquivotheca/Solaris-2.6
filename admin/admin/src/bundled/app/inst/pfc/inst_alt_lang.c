#ifndef lint
#pragma ident "@(#)inst_alt_lang.c 1.35 96/06/21 SMI"
#endif

/*
 * Copyright (c) 1993-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	inst_alt_lang.c
 * Group:	ttinstall
 * Description:
 */

#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/bitmap.h>
#include <libintl.h>

#include "pf.h"
#include "tty_pfc.h"
#include "inst_msgs.h"
#include "v_types.h"
#include "v_sw.h"

parAction_t
do_alt_lang(void)
{

	char **opts;

	int ch;
	int nlocales;
	int i;
	int row;

	/*
	 * # of locales should not exceed 32.  If it does, `selected' must
	 * become a vector of u_longs.
	 */
	u_long selected = 0L;
	u_int fkeys;
	HelpEntry _help;

	_help.win = stdscr;
	_help.type = HELP_TOPIC;
	_help.title = "Languages";

	nlocales = v_get_n_locales();
	opts = (char **) xcalloc(nlocales * sizeof (char *));

	/*
	 * load array of choices, set selected status on any currently
	 * selected locales
	 */
	for (i = 0; i < nlocales; i++) {
		opts[i] = (char *) v_get_locale_language(i);

		if (v_get_locale_status(i) == B_TRUE) {
			BT_SET(&selected, i);
		}
	}

	(void) werase(stdscr);
	(void) wclear(stdscr);

	wheader(stdscr, TITLE_LOCALES);

	row = HeaderLines;
	row = wword_wrap(stdscr, row, INDENT0, COLS - (2 * INDENT0),
	    MSG_LOCALES);
	row++;

	fkeys = (F_CONTINUE | F_GOBACK | F_EXIT | F_HELP);

	wfooter(stdscr, fkeys);

	ch = wmenu(stdscr, row, INDENT2, LINES - HeaderLines - FooterLines,
	    COLS - INDENT2 - 2,
	    show_help, (void *) &_help,
	    (Callback_proc *) NULL, (void *) NULL,
	    (Callback_proc *) NULL, (void *) NULL,
	    NULL, opts, nlocales, &selected,
	    0,
	    fkeys);

	if (is_ok(ch) != 0 || is_continue(ch) != 0) {

		/* set status on any selected locales */
		for (i = 0; i < nlocales; i++) {
			if (BT_TEST(&selected, i))
				(void) v_set_locale_status(i, TRUE);
			else
				(void) v_set_locale_status(i, FALSE);

		}

	}
	if (opts)
		free((void *) opts);

	if (is_ok(ch) != 0 || is_continue(ch) != 0) {
		wstatus_msg(stdscr, PLEASE_WAIT_STR);
		return (parAContinue);
	} else if (is_goback(ch) != 0)
		return (parAGoback);
	else /* if (is_exit(ch) != 0 || is_cancel(ch) != 0) */
		return (parAExit);

}
