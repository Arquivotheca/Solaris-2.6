#ifndef lint
#pragma ident "@(#)inst_srv_params.c 1.42 96/07/29 SMI"
#endif

/*
 * Copyright (c) 1993-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	inst_srv_params.c
 * Group:	ttinstall
 * Description:
 */

#include <stdio.h>
#include <limits.h>
#include <locale.h>
#include <libintl.h>
#include <stdlib.h>
#include <string.h>

#include "pf.h"
#include "inst_msgs.h"

#include "v_types.h"
#include "v_misc.h"

/*
 * collects client parameters for servers.
 *
 * parameters consist of:
 *	number of diskless clients
 *	default swap space per client
 *	number of cache-only-clients.
 *
 * parameters are used to calculate default filesystem sizes
 * for /export/root and /export/swap.
 */

static EditField cf[3] = {
	{0, 48, 5, 5, NUMERIC, (char *) 0},
	{0, 48, 5, 5, NUMERIC, (char *) 0},
	{0, 48, 5, 5, NUMERIC, (char *) 0}
};

parAction_t
do_server_params(void)
{
	int ch;
	int j;
	int fld;
	int nflds;
	int row;

	char n_d_clients[10];
	char n_c_clients[10];
	char swapsize[10];
	unsigned long fkeys;
	HelpEntry _help;

	_help.win = stdscr;
	_help.type = HELP_REFER;
	_help.title = "Allocate Client Services Screen";

	werase(stdscr);
	wclear(stdscr);
	wheader(stdscr, SERVER_PARAMETERS_TITLE);
	row = HeaderLines;
	row = wword_wrap(stdscr, row, INDENT0, COLS - (2 * INDENT0),
	    SERVER_PARAMETERS_ONSCREEN_HELP);

	fkeys = F_CONTINUE | F_GOBACK | F_EXIT | F_HELP;
	wfooter(stdscr, fkeys);

	row += 2;
	/* display fields */
	cf[0].r = row;
	(void) mvprintw(row++, 0, "%35.35s:",
	    gettext("Number of diskless clients"));
	cf[1].r = row;
	(void) mvprintw(row++, 0, "%35.35s:",
	    gettext("Megabytes of swap per client"));

#ifdef CACHE_ONLY_CLIENTS
	cf[2].r = row;
	(void) mvprintw(row++, 0, "%35.35s:",
	    gettext("Number of cache-only clients"));
#endif				/* CACHE_ONLY_CLIENTS */

	/*
	 * Set default values for now
	 */
	(void) sprintf(swapsize, "%d", v_get_diskless_swap());
	(void) sprintf(n_d_clients, "%d", v_get_n_diskless_clients());
	(void) sprintf(n_c_clients, "%d", v_get_n_cache_clients());

	cf[0].value = (char *) n_d_clients;
	cf[1].value = (char *) swapsize;
	cf[2].value = (char *) n_c_clients;

	cf[0].c = cf[1].c = cf[2].c = 38;

	fld = 0;

#ifdef CACHE_ONLY_CLIENTS
	nflds = 3;
#else
	nflds = 2;
#endif				/* CACHE_ONLY_CLIENTS */

	for (j = 0; j < nflds; j++) {
		(void) mvprintw(cf[j].r, cf[j].c, "%*.*s",
		    cf[j].len, cf[j].len, cf[j].value);
	}

	for (;;) {

		ch = wget_field(stdscr, cf[fld].type, cf[fld].r,
		    cf[fld].c, cf[fld].len, cf[fld].maxlen,
		    cf[fld].value, fkeys);

		if (verify_field_input(cf[fld], NUMERIC, 0, 0) ==
		    FAILURE) {

			/* hack to make fields with `bad' input 0 */
			(void) strcpy(cf[fld].value, "0");
		}
		(void) mvwprintw(stdscr, cf[fld].r, cf[fld].c, "%*.*s",
		    cf[fld].len, cf[fld].len, cf[fld].value);
		wnoutrefresh(stdscr);

		if (fwd_cmd(ch) != 0 || bkw_cmd(ch) != 0 ||
		    ch == RETURN) {

			if (bkw_cmd(ch) != 0) {

				fld = (fld + nflds) % (nflds + 1);

				if (fld == nflds)
					fld = nflds - 1;

			} else if (ch == RETURN || fwd_cmd(ch) != 0) {

				if (++fld == nflds)
					fld = 0;

			}
		} else if (is_continue(ch) != 0) {

			break;

		} else if (is_exit(ch) != 0) {

			break;

		} else if (is_goback(ch) != 0) {

			break;

		} else if (is_help(ch) != 0) {

			do_help_index(_help.win, _help.type, _help.title);

		} else if (is_escape(ch) != 0) {

			continue;

		} else
			beep();
	}

	if (is_continue(ch) != 0) {

		(void) v_set_n_diskless_clients(atoi(cf[0].value));
		(void) v_set_diskless_swap(atoi(cf[1].value));
		(void) v_set_n_cache_clients(atoi(cf[2].value));

		return (parAContinue);

	} else if (is_exit(ch) != 0) {

		return (parAExit);

	} else /* if (is_goback(ch) != 0) */ {

		return (parAGoback);

	}
}
