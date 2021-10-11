#ifndef lint
#pragma ident "@(#)inst_alloc_client.c 1.14 96/07/29 SMI"
#endif

/*
 * Copyright (c) 1993-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	inst_alloc_client.c
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
#include "tty_pfc.h"
#include "inst_msgs.h"
#include "v_types.h"
#include "v_misc.h"

typedef struct {
	HelpEntry help;
	NRowCol loc;
	FieldType type;
	char *label;
	char *prompt;
} _Item_Field_t;

typedef struct {

	_Item_Field_t fld[7];

} _Alloc_Client_Serv_Row_t;

typedef enum {
	allocSwap,
	allocRoot,
	allocBoth
} AllocService;

static AllocService do_alloc_serv_options(AllocService);
static void
show_alloc_table(WINDOW *w, int row, _Alloc_Client_Serv_Row_t *allocTable,
    AllocService allocServ)
{
	if (allocServ == allocRoot || allocServ == allocBoth) {
		++row;
		(void) mvwprintw(w, row, INDENT1,
		"%-10.10s %10.10s %-1.1s %10.10s %-1.1s %-12.12s %-20s",
		allocTable[0].fld[0].label, allocTable[0].fld[1].label,
		allocTable[0].fld[2].label, allocTable[0].fld[3].label,
		allocTable[0].fld[4].label, allocTable[0].fld[5].label,
		allocTable[0].fld[6].label);

		allocTable[0].fld[0].loc.r = allocTable[0].fld[1].loc.r =
		allocTable[0].fld[2].loc.r = allocTable[0].fld[3].loc.r =
		allocTable[0].fld[4].loc.r = allocTable[0].fld[5].loc.r =
		allocTable[0].fld[6].loc.r = row;

		allocTable[0].fld[1].loc.c = INDENT1 + 17;
		allocTable[0].fld[3].loc.c = INDENT1 + 30;

	}

	if (allocServ == allocSwap || allocServ == allocBoth) {
		++row;
	(void) mvwprintw(w, row, INDENT1,
		"%-10.10s %10.10s %-1.1s %10.10s %-1.1s %-12.12s %-20s",

		allocTable[1].fld[0].label,
		allocServ == allocBoth ? "" : allocTable[1].fld[1].label,
		allocTable[1].fld[2].label, allocTable[1].fld[3].label,
		allocTable[1].fld[4].label, allocTable[1].fld[5].label,
		allocTable[1].fld[6].label);

		allocTable[1].fld[0].loc.r = allocTable[1].fld[1].loc.r =
		allocTable[1].fld[2].loc.r = allocTable[1].fld[3].loc.r =
		allocTable[1].fld[4].loc.r = allocTable[1].fld[5].loc.r =
		allocTable[1].fld[6].loc.r = row;

		allocTable[1].fld[1].loc.c = INDENT1 + 17;
		allocTable[1].fld[3].loc.c = INDENT1 + 30;
	}

}


parAction_t
do_server_params(void)
{
	int ch;
	int j, i;
	int field;
	int tuple;
	int r, c; /* cursor location */
	int row;
	_Alloc_Client_Serv_Row_t allocTable[2];
	static AllocService LastServiceChoice = allocBoth;
	int numFlds = 7;
	char buf[32];
	int dirty = 1;
	int numClients, swapSize, rootSize, totalRootSize, totalSwap;

	unsigned long fkeys;
	HelpEntry _help;

	_help.win = stdscr;
	_help.type = HELP_TOPIC;
	_help.title = "Allocating Client Services";

	werase(stdscr);
	wclear(stdscr);


	fkeys = F_CONTINUE | F_GOBACK | F_OPTIONS | F_EXIT | F_HELP;

	for (j = 0; j < 2; j++) {
		for (i = 0; i < numFlds; ++i) {
			allocTable[j].fld[i].help = _help;
			allocTable[j].fld[i].type = INSENSITIVE;
			allocTable[j].fld[i].prompt = xstrdup("");
		}
	}

	/* set root row for display */
	allocTable[0].fld[0].label = xstrdup("ROOT");
	allocTable[0].fld[1].label = (char *) xmalloc(5 * sizeof (char));
	(void) sprintf(allocTable[0].fld[1].label, "%d",
		v_get_n_diskless_clients());
	allocTable[0].fld[2].label = xstrdup("X");

	allocTable[0].fld[3].label = (char *) xmalloc(5 * sizeof (char));
	(void) sprintf(allocTable[0].fld[3].label, "%d",
		v_get_root_client_size());

	allocTable[0].fld[4].label = xstrdup("=");

	totalRootSize = v_get_n_diskless_clients() * v_get_root_client_size();
	allocTable[0].fld[5].label = (char *) xmalloc(11 * sizeof (char));
	(void) sprintf(allocTable[0].fld[5].label, "%d", totalRootSize);

	allocTable[0].fld[6].label = xstrdup("/export/root");
	allocTable[0].fld[1].type = NUMERIC;
	allocTable[0].fld[3].type = NUMERIC;

	/* set swap row for display */

	allocTable[1].fld[0].label = xstrdup("SWAP");
	allocTable[1].fld[1].label = xstrdup(allocTable[0].fld[1].label);
	allocTable[1].fld[2].label = xstrdup("X");
	allocTable[1].fld[3].label = (char *) xmalloc(5 * sizeof (char));
	(void) sprintf(allocTable[1].fld[3].label, "%d", v_get_diskless_swap());
	allocTable[1].fld[4].label = xstrdup("=");
	allocTable[1].fld[5].label = xmalloc(11 * sizeof (char));
	totalSwap = v_get_n_diskless_clients() * v_get_diskless_swap();
	(void) sprintf(allocTable[1].fld[5].label, "%d", totalSwap);
	allocTable[1].fld[6].label = xstrdup("/export/swap");
	allocTable[1].fld[3].type = NUMERIC;

	tuple = 0;
	field = 1;

	for (;;) {
		if (dirty) {
			werase(stdscr);
			wclear(stdscr);
			wheader(stdscr, TITLE_CLIENTALLOC);
			row = HeaderLines;
			row = wword_wrap(stdscr, row, INDENT0,
				COLS - (2 * INDENT0), MSG_CLIENTSETUP);

			++row;
			(void) mvwprintw(stdscr, row, INDENT1,
				"%-10.10s %-10.10s %-1.1s %-10.10s %-1.1s %-12.12s %-20s",
				/* i18n: 10 chars max */
				gettext("Type"),
				/* i18n: 10 chars max */
				gettext("# Clients"),
				/* i18n: 1 chars max */
				gettext("X"),
				/* i18n: 10 */
				gettext("Size Per"),
				/* i18n: 1 chars max */
				gettext("="),
				/* i18n: 12 chars max */
				gettext("Total Size"),
				/* i18n: 20 chars max */
				gettext("Mount Point"));

			++row;
			(void) mvwprintw(stdscr, row, INDENT1, "%-.*s",
				10 + 10 + 1 + 10 + 1 + 12 + 20, EQUALS_STR);
			show_alloc_table(stdscr, row, allocTable,
				LastServiceChoice);
			wfooter(stdscr, fkeys);
			dirty = 0;
		}


		wfocus_on(stdscr, allocTable[tuple].fld[field].loc.r,
		    allocTable[tuple].fld[field].loc.c,
		    allocTable[tuple].fld[field].label);

		(void) getsyx(r, c);
		(void) wnoutrefresh(stdscr);
		(void) setsyx(r, c);
		(void) doupdate();

		(void) strcpy(buf, allocTable[tuple].fld[field].label);

		ch = wget_field(stdscr, allocTable[tuple].fld[field].type,
			allocTable[tuple].fld[field].loc.r,
			allocTable[tuple].fld[field].loc.c,
			4, 4, allocTable[tuple].fld[field].label, fkeys);


		wnoutrefresh(stdscr);

		if (strcmp(buf, allocTable[tuple].fld[field].label)) {
			dirty = 1;
			(void) strcpy(buf, allocTable[tuple].fld[field].label);
			if (tuple == 0 && field == 1 ||
			    tuple == 1 && field == 1) {
				numClients = atoi(buf);
				(void) v_set_n_diskless_clients(numClients);
				if (tuple == 0) {
					(void) sprintf(
						allocTable[1].fld[1].label,
						"%d", numClients);
				} else {
					(void) sprintf(
						allocTable[0].fld[1].label,
						"%d", numClients);
				}

				totalRootSize = numClients *
				    v_get_root_client_size();
				(void) sprintf(allocTable[0].fld[5].label, "%d",
				    totalRootSize);
				totalSwap = v_get_n_diskless_clients() *
				    v_get_diskless_swap();
				(void) sprintf(allocTable[1].fld[5].label, "%d",
				    totalSwap);
			} else if (tuple == 0 && field == 3) {
				rootSize = atoi(buf);
				v_set_root_client_size(rootSize);
				totalRootSize = v_get_n_diskless_clients() *
				    v_get_root_client_size();
				(void) sprintf(allocTable[0].fld[5].label, "%d",
				    totalRootSize);
			} else if (tuple == 1 && field == 3) { /* swap size */
				swapSize = atoi(buf);
				(void) v_set_diskless_swap(swapSize);
				totalSwap = v_get_n_diskless_clients() *
				    v_get_diskless_swap();
				(void) sprintf(allocTable[1].fld[5].label, "%d",
				    totalSwap);
			}
		}

		if (ch == U_ARROW || ch == D_ARROW ||
			ch == R_ARROW || ch == L_ARROW ||
			ch == CTRL_F || ch == CTRL_N ||
			ch == CTRL_P || ch == CTRL_B ||
			ch == RETURN) {


			/* unhighlight */
			(void) mvwprintw(stdscr,
				allocTable[tuple].fld[field].loc.r,
				allocTable[tuple].fld[field].loc.c, "%4.4s",
				allocTable[tuple].fld[field].label);


			if (ch == R_ARROW || ch == L_ARROW ||
			    ch == CTRL_F || ch == CTRL_B || ch == RETURN) {

				if (LastServiceChoice == allocBoth) {
					if (field == 1 && tuple == 0)
						field = 3;
					else if (field == 3 && tuple == 0)
						field = 1;
					else
						field = 3;
				} else {
					if (field == 1)
						field = 3;
					else
						field = 1;
				}

			} else if (ch == U_ARROW || ch == CTRL_P) {
				if (LastServiceChoice == allocBoth) {
					if (tuple == 0) {
						tuple = 1;
						field = 3;
					} else {
						tuple = 0;
					}
				}

			} else if (ch == D_ARROW || ch == CTRL_N) {

				if (LastServiceChoice == allocBoth) {
					if (tuple == 0) {
						tuple = 1;
						field = 3;
					} else {
						tuple = 0;
					}
				}

			}


		} else if (is_continue(ch) != 0) {

			break;

		} else if (is_exit(ch) != 0) {

			break;

		} else if (is_goback(ch) != 0) {

			break;

		} else if (is_options(ch) != 0) {

			LastServiceChoice =
				do_alloc_serv_options(LastServiceChoice);
			if (LastServiceChoice == allocSwap) {
				field = 1;
				tuple = 1;
				allocTable[1].fld[1].type = NUMERIC;
			} else {
				field = 1;
				tuple = 0;
				allocTable[1].fld[1].type = INSENSITIVE;
			}
			dirty = 1;

		} else if (is_help(ch) != 0) {

			do_help_index(_help.win, _help.type, _help.title);

		} else if (is_escape(ch) != 0) {

			continue;

		} else
			beep();
	}

	if (is_continue(ch) != 0) {
/*
		(void) v_set_n_diskless_clients(atoi(cf[0].value));
		(void) v_set_diskless_swap(atoi(cf[1].value));
		(void) v_set_n_cache_clients(atoi(cf[2].value));
*/

		return (parAContinue);

	} else if (is_exit(ch) != 0) {

		return (parAExit);

	} else /* if (is_goback(ch) != 0) */ {

		return (parAGoback);

	}
}

AllocService
do_alloc_serv_options(AllocService allocServ)
{

	char *opts[4];

	int ch;
	int row;
	u_long selected;
	u_int fkeys;
	HelpEntry _help;

	_help.win = stdscr;
	_help.type = HELP_REFER;
	_help.title = "Allocate Client Services Screen";

	opts[0] = gettext("ROOT");
	opts[1] = gettext("SWAP");
	opts[2] = gettext("BOTH");

	switch (allocServ) {
	case allocRoot:
		selected = 0;
		break;
	case allocSwap:
		selected = 1;
		break;
	case allocBoth:
	default:
		selected = 2;
		break;
	}

	(void) werase(stdscr);
	(void) wclear(stdscr);
	wheader(stdscr, ALLOC_SERV_OPT_TITLE);

	row = HeaderLines;
	row = wword_wrap(stdscr, row, INDENT0, COLS - (2 * INDENT0),
		ALLOC_SERV_OPT_ONSCREEN_HELP);
		++row;

	fkeys = (F_OKEYDOKEY | F_CANCEL);

	wfooter(stdscr, fkeys);

	ch = wmenu(stdscr, row, INDENT2, LINES - HeaderLines - FooterLines,
		COLS - INDENT2 - 2,
		show_help, (void *) &_help,
		(Callback_proc *) NULL, (void *) NULL,
		(Callback_proc *) NULL, (void *) NULL,
		NULL, opts, 3, &selected,
		M_RADIO | M_RADIO_ALWAYS_ONE | M_CHOICE_REQUIRED,
		fkeys);

	if (is_continue(ch) != 0) {
		switch (selected) {
		case 0:
			allocServ = allocRoot;
			break;
		case 1:
			allocServ = allocSwap;
			break;
		case 2:
		default:
			allocServ = allocBoth;
		}
		return (allocServ);
	} else { /* user hit cancel */
		return (allocServ);
	}

}
