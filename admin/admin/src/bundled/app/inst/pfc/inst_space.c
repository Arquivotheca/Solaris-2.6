#ifndef lint
#pragma ident "@(#)inst_space.c 1.38 96/07/29 SMI"
#endif

/*
 * Copyright (c) 1993-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	inst_space.c
 * Group:	ttinstall
 * Description:
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <locale.h>
#include <string.h>
#include <libintl.h>

#include "pf.h"
#include "inst_msgs.h"
#include "v_types.h"
#include "v_disk.h"
#include "v_lfs.h"

typedef struct {
	float required;
	float suggested;
	float configed;
} FS_Totals_t;

static void _free_opts(ChoiceItem *, int);
static ChoiceItem *_load_opts(int *, FS_Totals_t *);

parAction_t
do_fs_space_warning()
{
	WINDOW *win;
	int nlfs;
	int top_row;		/* first row of menu */
	int last_row;		/* last row of menu */
	int cur;		/* remember last selection */

	int ch;
	int top;		/* index of first item displayed */
	int dirty;
	int fs_per_page;
	ChoiceItem *opts = (ChoiceItem *) NULL;
	unsigned long fkeys;
	FS_Totals_t totals;
	char blanks[] = " ";	/* 2 blanks */

	totals.required = 0;
	totals.suggested = 0;
	totals.configed = 0;

	fkeys = F_OKEYDOKEY | F_CANCEL;

	/*
	 * update the view layer's idea of what the sizes are for the
	 * configured file systems... then get the # of configured file
	 * systems
	 */
	v_set_n_lfs();
	v_update_lfs_space();
	(void) v_set_current_disk(-1);

	nlfs = v_get_n_lfs();
	opts = _load_opts(&nlfs, &totals);

	win = newwin(LINES, COLS, 0, 0);
	wcolor_set_bkgd(win, BODY);
	(void) keypad(win, 1);

	(void) werase(win);
	(void) wclear(win);

	/* show title */
	wheader(win, TITLE_WARNING);

	top_row = HeaderLines;
	top_row = wword_wrap(win, top_row, INDENT0, COLS - (2 * INDENT0),
	    SPACE_NOTICE_ONSCREEN_HELP);
	top_row++;

	(void) mvwprintw(win, top_row, INDENT1,
	    "%-25.24s  %-12.10s%-12.10s%-12.10s",
	/* i18n: 24 chars max */
	    gettext("File System"),
	/* i18n: 10 chars max */
	    gettext(" Minimum"),
	/* i18n: 10 chars max */
	    gettext("Suggested"),
	/* i18n: 10 chars max */
	    gettext("Configured"));
	top_row++;

	(void) mvwprintw(win, top_row, INDENT1, "%.*s",
	    25 + 10 + 10 + 10 + 6, DASHES_STR);
	top_row++;

	cur = 0;
	top = 0;
	fs_per_page = LINES - top_row - FooterLines - 3;

	/* position `total' display lines */
	if (nlfs > fs_per_page)
		last_row = top_row + fs_per_page - 1;
	else
		last_row = top_row + nlfs - 1;

	dirty = 1;

	(void) mvwprintw(win, last_row + 1, INDENT1, "%.*s",
	    25 + 10 + 10 + 10 + 6, EQUALS_STR);

	(void) mvwprintw(win, last_row + 2, INDENT1,
	    "%-25.24s%9.2f MB%9.2f MB%9.2f MB",
	/* i18n: 24 chars max */
	    gettext("Totals:"),
	    totals.required, totals.suggested, totals.configed);

	for (;;) {

		if (dirty == 1) {

			(void) show_choices(win, nlfs, fs_per_page,
			    top_row, INDENT1, opts, top);

			scroll_prompts(win, top_row, 1, top, nlfs,
			    fs_per_page);

			wfooter(win, fkeys);
			dirty = 0;

		}

		if (nlfs <= fs_per_page)
			wcursor_hide(win);
		else
			/* highlight current */
			wfocus_on(win, opts[cur].loc.r, opts[cur].loc.c - 2,
			    blanks);

		ch = wzgetch(win, fkeys);

		if (nlfs >= fs_per_page)
			/* unhighlight */
			wfocus_off(win, opts[cur].loc.r, opts[cur].loc.c - 2,
			    blanks);

		if (is_ok(ch) != 0) {

			break;

		} else if (is_cancel(ch) != 0) {

			break;

		} else if (is_escape(ch) != 0) {

			continue;

		} else if ((ch == U_ARROW || ch == D_ARROW || ch == CTRL_N ||
			ch == CTRL_P) && (nlfs > fs_per_page)) {

			/* move */
			if (ch == U_ARROW || ch == CTRL_P) {

				if (opts[cur].loc.r == top_row) {

					if (top) {	/* scroll down */
						cur = --top;
						dirty = 1;
					} else
						beep();	/* very top */

				} else
					cur--;

			} else if (ch == D_ARROW || ch == CTRL_N) {

				if (opts[cur].loc.r == last_row) {

					if ((cur + 1) < nlfs) {

						/* scroll up */
						top++;
						cur++;
						dirty = 1;

					} else
						beep();	/* bottom */

				} else {

					if ((cur + 1) < nlfs)
						cur++;
					else
						beep();	/* last, no wrap */
				}

			}
		} else
			beep();
	}

	(void) delwin(win);
	(void) clearok(curscr, TRUE);	/* gets around refresh problems... */
	(void) touchwin(stdscr);
	(void) wnoutrefresh(stdscr);
	(void) clearok(curscr, FALSE);	/* gets around refresh problems... */

	_free_opts(opts, nlfs);
	if (is_ok(ch) != 0)
		return (parAContinue);
	else
		return (parAGoback);
}

static void
_free_opts(ChoiceItem * opts, int n)
{
	register int i;

	if (opts != (ChoiceItem *) NULL) {
		for (i = 0; i < n; i++) {
			if (opts[i].label != (char *) NULL)
				free((void *) opts[i].label);
		}
	}
	free((void *) opts);
}

static ChoiceItem *
_load_opts(int *n, FS_Totals_t * totals)
{
	int i;
	int j;
	char *mnt_pt;
	char buf[128];
	float required;
	float suggest;
	float configed;
	ChoiceItem *opts;

	opts = (ChoiceItem *) xcalloc((*n) * sizeof (ChoiceItem));
	v_set_disp_units(V_MBYTES);

	for (i = 0, j = 0; i < (*n); i++) {

		if (v_get_lfs_req_size(i) > 0.00) {

			mnt_pt = v_get_lfs_mntpt(i);

			if (strcmp(mnt_pt, Overlap) == 0)
				continue;

			required = v_get_lfs_req_size(i);
			totals->required += required;

			suggest = v_get_lfs_suggested_size(i);
			totals->suggested += suggest;

			configed = v_get_lfs_configed_size(i);
			totals->configed += configed;

			(void) sprintf(buf,
			    "%-25.24s%9.2f MB%9.2f MB%9.2f MB",
			    mnt_pt, required, suggest, configed);

			opts[j].label = (char *) xstrdup(buf);
			opts[j].help.type = HELP_NONE;
			opts[j].help.title = "";
			opts[j].sel = -1;	/* XXX HACK! */
			opts[j].loc.c = INDENT1;

			++j;
		}
	}

	*n = j;			/* actual # of elements */
	return (opts);		/* return array of elements */

}
