#ifndef lint
#pragma ident "@(#)inst_space_meter.c 1.12 96/06/21 SMI"
#endif

/*
 * Copyright (c) 1993-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	inst_space_meter.c
 * Group:	ttinstall
 * Description:
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <libintl.h>
#include <sys/param.h>
#include <sys/types.h>

#include "pf.h"
#include "inst_msgs.h"
#include "disk_fs_util.h"

#include "v_types.h"
#include "v_disk.h"
#include "v_lfs.h"

typedef struct {
	NRowCol loc;
	char *label;
	char *prompt;
} _Item_Field_t;

/*
 * a row in the file systems table consists of 2 items:
 *	field for preserve toggle
 *	field for mount point & size info
 */
typedef struct {

	_Item_Field_t	fld[2];

}	_FS_Row_t;

static _FS_Row_t *_load_table(_FS_Row_t *, int);
static void _free_fstable_labels(int, _FS_Row_t *);
static void show_fstable(WINDOW *, int, int, int, _FS_Row_t *, int);

void
inst_space_meter(WINDOW * parent)
{
	WINDOW *win;
	_FS_Row_t *fstable = (_FS_Row_t *) NULL;

	int ch;
	int nfs;
	int fs;
	int top;
	int fs_per_page;
	int last_row;
	int top_row;
	int dirty;
	int r, c;

	u_int fkeys;
	V_Units_t units = v_get_disp_units();

	win = newwin(LINES, COLS, 0, 0);
	wcolor_set_bkgd(win, BODY);
	(void) keypad(win, 1);

	(void) werase(win);
	(void) wclear(win);

	/* show title */
	wheader(win, TITLE_WARNING);

	top_row = HeaderLines + 2;

	(void) mvwprintw(win, top_row, INDENT2,
	    "%-25.24s  %-12.10s%-12.10s",
	/* i18n: 24 chars max */
	    gettext("File System"),
	/* i18n: 10 chars max */
	    gettext(" Minimum"),
	/* i18n: 10 chars max */
	    gettext("Suggested"));
	top_row++;

	(void) mvwprintw(win, top_row, INDENT2, "%.*s",
	    25 + 10 + 10 + 6, EQUALS_STR);
	top_row++;

	v_save_current_default_fs();
	v_restore_default_fs_table();

	v_set_disp_units(V_MBYTES);
	nfs = v_get_n_default_fs();
	fstable = _load_table(fstable, nfs);

	fs = 0;
	top = 0;
	dirty = 1;
	fs_per_page = LINES - top_row - FooterLines - 1;
	last_row = top_row + fs_per_page - 1;

	fkeys = F_OKEYDOKEY | F_HELP;

	for (;;) {

		if (dirty) {

			(void) show_fstable(win, nfs, fs_per_page,
			    top_row, fstable, top);

			scroll_prompts(win, top_row, 1, top, fs, fs_per_page);

			wfooter(win, fkeys);
			dirty = 0;
		}
		/* highlight current */
		wfocus_on(win, fstable[fs].fld[0].loc.r,
		    fstable[fs].fld[0].loc.c,
		    fstable[fs].fld[0].label);

		(void) getsyx(r, c);
		(void) wnoutrefresh(win);
		(void) setsyx(r, c);
		(void) doupdate();

		ch = wzgetch(win, fkeys);

		if ((sel_cmd(ch) != 0) || (alt_sel_cmd(ch) != 0)) {

			if (v_get_default_fs_status(fs))
				(void) v_set_default_fs_status(fs, 0);
			else
				(void) v_set_default_fs_status(fs, 1);

			fstable = _load_table(fstable, nfs);
			dirty = 1;
		} else if (is_ok(ch) != 0) {

			break;

		} else if (is_help(ch) != 0) {

			do_help_index(win, HELP_TOPIC, (char *) NULL);

		} else if (is_escape(ch) != 0) {

			continue;

		} else if (ch == U_ARROW || ch == D_ARROW ||
			ch == CTRL_F || ch == CTRL_D ||
			ch == CTRL_N || ch == CTRL_P ||
			ch == CTRL_B || ch == CTRL_U) {

			dirty = 0;

			/* unhighlight */
			wfocus_off(win, fstable[fs].fld[0].loc.r,
			    fstable[fs].fld[0].loc.c,
			    fstable[fs].fld[0].label);

			/* move */
			if (ch == CTRL_D) {

				/* page down */
				if ((fs + fs_per_page) < nfs) {

					/* advance a page */
					top += fs_per_page;
					fs += fs_per_page;
					dirty = 1;

				} else if (fs < nfs - 1) {

					/* advance to last line */
					fs = nfs - 1;
					top = fs - 2;
					dirty = 1;

				} else
					beep();	/* at end */

			} else if (ch == CTRL_U) {

				/* page up */
				if ((fs - fs_per_page) >= 0) {

					/* reverse a page */
					top = (top > fs_per_page ?
					    top - fs_per_page : 0);
					fs -= fs_per_page;
					dirty = 1;

				} else if (fs > 0) {

					/* back to first */
					top = 0;
					fs = 0;
					dirty = 1;

				} else
					beep();	/* at top */

			} else if (ch == U_ARROW || ch == CTRL_P ||
			    ch == CTRL_B) {

				if (fstable[fs].fld[0].loc.r == top_row) {

					if (top) {	/* scroll down */
						fs = --top;
						dirty = 1;
					} else
						beep();	/* very top */

				} else {
					fs--;
				}

			} else if (ch == D_ARROW || ch == CTRL_N ||
			    ch == CTRL_F) {

				if (fstable[fs].fld[0].loc.r == last_row) {

					if ((fs + 1) < nfs) {

						/* scroll up */
						top++;
						fs++;
						dirty = 1;

					} else
						beep();	/* bottom */

				} else {

					if ((fs + 1) < nfs) {
						fs++;
					} else
						beep();	/* last, no wrap */
				}

			}
		} else
			beep();

	}

	_free_fstable_labels(nfs, fstable);
	free((void *) fstable);

	v_restore_current_default_fs();
	v_set_disp_units(units);

	(void) delwin(win);
	(void) clearok(curscr, TRUE);	/* gets around refresh problems... */
	(void) touchwin(parent);
	(void) wnoutrefresh(parent);
	(void) clearok(curscr, FALSE);	/* gets around refresh problems... */
}

_FS_Row_t *
_load_table(_FS_Row_t * fstable, int nfs)
{
	int i;
	char *mnt_pt;
	int required;
	int suggested;
	char buf[128];

	if (fstable == (_FS_Row_t *) NULL)
		fstable = (_FS_Row_t *) xcalloc(nfs * sizeof (_FS_Row_t));

	/*
	 * load array of choices, set selected status on
	 */
	for (i = 0; i < nfs; i++) {
		mnt_pt = v_get_default_fs_name(i);
		required = v_get_default_fs_req_size(i);
		suggested = v_get_default_fs_sug_size(i);

		fstable[i].fld[0].loc.c = INDENT2 - 4;
		if (v_get_default_fs_status(i))
			fstable[i].fld[0].label = Sel;
		else
			fstable[i].fld[0].label = Unsel;

		/*
		 * configed = v_get_lfs_configed_size(i);
		 */
		(void) sprintf(buf, "%-25.24s%9.2f MB%9.2f MB",
		    mnt_pt, (float) required, (float) suggested);

		if (fstable[i].fld[1].label == (char *) NULL)
			fstable[i].fld[1].label = (char *) xstrdup(buf);
		else
			(void) strcpy(fstable[i].fld[1].label, buf);

		fstable[i].fld[1].loc.c = INDENT2;

	}

	return (fstable);

}

/*
 * free any existing fstable labels which have been strdup'ed
 */
static void
_free_fstable_labels(int nfs, _FS_Row_t * fstable)
{
	register int i;

	if (fstable != (_FS_Row_t *) NULL) {

		for (i = 0; i < nfs; i++) {

			if (fstable[i].fld[1].label != (char *) NULL)
				free((void *) fstable[i].fld[1].label);

		}

	}
}

static void
show_fstable(WINDOW * w, int max, int npp, int row,
    _FS_Row_t * fstable, int first)
{
	int i;		/* counts modules displayed		*/
	int j;		/* index of current software mod	*/
	int r;		/* counts row positions			*/

	for (i = 0, r = row, j = first;
	    (i < npp) && (j < max);
	    i++, r++, j++) {

		(void) mvwprintw(w, r, INDENT2 - 4, "%s %s",
		    fstable[j].fld[0].label, fstable[j].fld[1].label);

		fstable[j].fld[0].loc.r =
		    fstable[j].fld[1].loc.r = (int) r;

		fstable[j].fld[0].loc.c = INDENT2 - 4;
		fstable[j].fld[1].loc.c = INDENT2;

	}

	/*
	 * clear remaining rows, i counts lines displayed, r counts row
	 * lines are displayed on
	 */
	for (; i < npp; i++, r++) {
		(void) mvwprintw(w, r, 0, "%*s", COLS, " ");
	}

}
