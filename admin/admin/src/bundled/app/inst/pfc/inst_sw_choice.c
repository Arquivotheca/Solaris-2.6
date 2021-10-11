#ifndef lint
#pragma ident "@(#)inst_sw_choice.c 1.50 96/07/29 SMI"
#endif

/*
 * Copyright (c) 1993-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	inst_sw_choice.c
 * Group:	ttinstall
 * Description:
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libintl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/bitmap.h>

#include "pf.h"
#include "tty_pfc.h"
#include "v_types.h"
#include "v_disk.h"
#include "v_lfs.h"
#include "v_sw.h"

#include "inst_msgs.h"

static parAction_t _do_sw_choice(void);
static int _unselect_cb(void *, void *);
static int _select_cb(void *, void *);

static char *dots =
"...........................................................................";

/*
 * Entry point into the software part of ttinstall.
 *
 * Top level of sw selection is the `metacluster' selection menu.  This
 * function is done by `do_sw_choice()' which returns DONE/CANCEL/EDIT
 * status.  Package/Cluster editing is done by `do_sw_edit()'
 */
parAction_t
do_sw()
{
	parAction_t ret;

	while ((ret = _do_sw_choice()) == parACustomize)
		(void) do_sw_edit();

	return (ret);		/* exit/continue/goback */

}

/*
 * helper routine to format the menu strings.
 *
 * formatted string is sprintf'ed into 'buf' (buf must be at least 80 chars)
 * i is the metacluster of interest
 * sel indicates if i'th metacluster is selected or not
 *
 * if metacluster is unselected, need to pad the end of the string, leaving
 * room for the 'F4 to customize' prompt which may get put there later
 *
 */
static void
format_str(char *buf, int i, int sel)
{
	int promptlen;
	int namelen;
	char *prompt;
	char *name;

	prompt = gettext("(F4 to Customize)");
	promptlen = strlen(prompt);

	name = v_get_metaclst_name(i);
	namelen = strlen(name);

	(void) sprintf(buf, "%.*s %.*s %s %*s",
	    (namelen > 37) ? 37 : namelen, name,
	    (namelen > 37) ? 1 : 38 - namelen,
	    (namelen > 37) ? ">" : dots,
	    v_get_metaclst_size(i),
	    promptlen, sel ? prompt : " ");

}

/*
 * do_sw_choice() -
 *
 * Top level of sw selection is the `metacluster' selection menu.
 *
 * Present a menu of the metaclusters, allow user to choose one and
 * optionally decide to `customize' it.  Metacluster selection is
 * exclusive, make sure underlying menu code enforces exclusion.
 *
 * Returns:
 *	-1 - go back a screen in the parade
 *	0 - cancel install
 *	1 - done, continue to next screen in parade
 *	2 - customize
 *
 */
static parAction_t
_do_sw_choice(void)
{

	int i;
	int ch;
	int row;
	int nmods;
	char **opts;
	char buf[128];

	int selected;
	u_int fkeys;
	HelpEntry _help;

	_help.win = stdscr;
	_help.type = HELP_TOPIC;
	_help.title = "Software Groups: What They Contain";

	v_set_disp_units(V_MBYTES);

	/* set up default file systems and sizes for the metaclusters */
	v_set_n_lfs();
	v_set_metaclst_dflt_sizes();

	/* load up choices */
	nmods = v_get_n_metaclsts();
	opts = (char **) xcalloc((nmods + 1) * sizeof (char *));

	selected = -1;
	for (i = 0; i < nmods; i++) {

		/*
		 * remember currently selected metacluster
		 */
		if (v_is_current_metaclst(i)) {
			selected = i;
		}
		format_str(buf, i, selected == i);
		opts[i] = (char *) xstrdup(buf);
	}
	opts[i] = (char *) NULL;	/* mark end */

	(void) werase(stdscr);
	(void) wclear(stdscr);
	wheader(stdscr, TITLE_SW);
	row = HeaderLines;
	row = wword_wrap(stdscr, row, INDENT0, COLS - (2 * INDENT0), MSG_SW);
	row++;

	fkeys = F_CONTINUE | F_GOBACK | F_CUSTOMIZE | F_HELP | F_EXIT;

	wfooter(stdscr, fkeys);

	/* display options */
	ch = wmenu(stdscr, row, INDENT0, LINES - HeaderLines - FooterLines,
	    COLS,
	    show_help, (void *) &_help,
	    _select_cb, (void *) opts,
	    _unselect_cb, (void *) opts,
	    NULL, opts, nmods, (void *) &selected,
	    M_RADIO | M_RADIO_ALWAYS_ONE,
	    fkeys);

	if (opts) {
		for (i = 0; i < nmods; i++)
			if (opts[i])
				free((void *) opts[i]);
		free((void *) opts);
	}
	if (is_goback(ch) != 0)
		return (parAGoback);
	else if (is_ok(ch) != 0 || is_continue(ch) != 0) {
		wstatus_msg(stdscr, PLEASE_WAIT_STR);
		return (parAContinue);
	} else if (is_customize(ch) != 0)
		return (parACustomize);
	else	/* if (is_exit(ch) != 0 || is_cancel(ch) != 0) */
		return (parAExit);

}

/*
 * _select_cb()/_unselect_cb()
 *
 * 	callbacks for wmenu() to call when a metacluster is selected.
 * 	handles moving the 'F4 to Customize' prompt along with the currently
 *	selected metacluster.
 *
 *	when selection changes, the unselect callback happens first.
 *	do any prompting in that callback.
 *
 * input:
 *	opts:	array of metacluster description strings which wmenu uses
 *		to display the choices.
 *	item:	integer index of menu item being (de)selected.
 *
 * return:
 *	always return 1
 *
 */
static int
_select_cb(void *opts, void *item)
{
	int i;

	if (v_metaclst_edited() > 0) {

		v_set_current_metaclst((int) item);

		/*
		 * recalc default sizes only if necesary, this
		 * takes a noticeable moment so avoid recalc if possible...
		 */
		wstatus_msg(stdscr, PLEASE_WAIT_STR);
		v_set_metaclst_dflt_sizes();
		(void) sleep(1);

		for (i = 0; ((char **) opts)[i] != (char *) NULL; i++) {
			format_str(((char **) opts)[i], i,
			    i == (int) item);
		}

		((char **) opts)[i] = (char *) NULL;	/* mark end */
		wclear_status_msg(stdscr);

	} else {

		v_set_current_metaclst((int) item);
		format_str(((char **) opts)[(int) item], (int) item, 1);

	}

	return (1);
}

static int
_unselect_cb(void *opts, void *item)
{
	if (v_metaclst_edited() > 0) {

		if (yes_no_notice(stdscr, F_OKEYDOKEY | F_CANCEL,
			F_OKEYDOKEY, F_CANCEL, SW_BASE_CHOICE_OK_CHANGE_TITLE,
			SW_BASE_CHOICE_OK_CHANGE) == F_CANCEL) {
			return (0);
		}

	}
	format_str(((char **) opts)[(int) item], (int) item, 0);
	return (1);

}
