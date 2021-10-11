#ifndef lint
#pragma ident "@(#)inst_client_arch.c 1.44 96/06/21 SMI"
#endif

/*
 * Copyright (c) 1993-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	inst_client_arch.c
 * Group:	ttinstall
 * Description:
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <locale.h>
#include <libintl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/bitmap.h>

#include "pf.h"
#include "tty_pfc.h"
#include "inst_msgs.h"
#include "v_types.h"
#include "v_sw.h"

/*ARGSUSED0*/
static int
deselect_cb(void *data, void *item)
{

	if (v_is_native_arch((int) item) == B_TRUE) {
		wstatus_msg(stdscr, CLIENT_ARCH_REQUIRED,
		    (char *) v_get_arch_name((int) item));

		beep();
		(void) peekch();
		wclear_status_msg(stdscr);
		return (0);
	} else
		return (1);

}

parAction_t
do_client_arches(void)
{
	char	**opts;

	int	ch;
	int	narchs;
	int	i;
	int	row;

	/*
	 * # of arches should not exceed 32.  If it does, `selected' must
	 * become a vector of u_longs.
	 */
	u_long selected = 0L;
	u_int fkeys;
	HelpEntry _help;

	_help.win = stdscr;
	_help.type = HELP_HOWTO;
	_help.title = "Determine a System's Platform";

	/* how many architectures? */
	narchs = v_get_n_arches();

	/* set up an array of the architecture strings */
	opts = (char **) xcalloc(narchs * sizeof (char *));

	for (i = 0; i < narchs; i++) {
		opts[i] = (char *) v_get_arch_name(i);

		/*
		 * Set default values for now: for each architecture, if it
		 * is currently selected, show it as marked
		 */
		if (v_get_arch_status(i) == SELECTED ||
		    v_is_native_arch(i) == B_TRUE) {
			BT_SET(&selected, i);
		}
	}

	(void) werase(stdscr);
	(void) wclear(stdscr);
	wheader(stdscr, TITLE_CLIENTS);

	row = HeaderLines;
	row = wword_wrap(stdscr, row, INDENT0, COLS - (2 * INDENT0),
	    MSG_CLIENTS);
	row++;

	fkeys = (F_CONTINUE | F_GOBACK | F_EXIT | F_HELP);

	wfooter(stdscr, fkeys);

	ch = wmenu(stdscr, row, INDENT2, LINES - HeaderLines - FooterLines,
	    COLS - INDENT2 - 2,
	    show_help, (void *) &_help,
	    (Callback_proc *) NULL, (void *) NULL,
	    deselect_cb, (void *) NULL,
	    NULL, opts, narchs, &selected,
	    0,
	    fkeys);

	if (is_continue(ch) != 0) {

		/* set status on any selected architectures */
		for (i = 0; i < narchs; i++) {
			if (BT_TEST(&selected, i))
				(void) v_set_arch_status(i, TRUE);
			else
				(void) v_set_arch_status(i, FALSE);

		}
	}
	if (opts)
		free((void *) opts);

	if (is_continue(ch) != 0) {

		wstatus_msg(stdscr, PLEASE_WAIT_STR);
		return (parAContinue);

	} else if (is_exit(ch) != 0)
		return (parAExit);
	else /* if (is_goback(ch) != 0) */
		return (parAGoback);

}
