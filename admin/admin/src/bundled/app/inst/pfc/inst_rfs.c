#ifndef lint
#pragma ident "@(#)inst_rfs.c 1.71 96/07/29 SMI"
#endif

/*
 * Copyright (c) 1993-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	inst_rfs.c
 * Group:	ttinstall
 * Description:
 */

#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/bitmap.h>
#include <sys/fs/ufs_fs.h>

#include <sys/utsname.h>
#include <netdb.h>
#include <netdir.h>
#include <libintl.h>

#include "pf.h"
#include "tty_pfc.h"
#include "inst_msgs.h"
#include "rfs_util.h"
#include "v_types.h"
#include "v_rfs.h"
#include "v_sw.h"

/* local functions */
static parAction_t _do_rfs_menu(void);
static int _confirm_rfs(void);
static void _process_rfs(int, int, int);

parAction_t
do_rfs(void)
{
	int ret;

	if ((ret = _confirm_rfs()) == 0)
		return (parAContinue);	/* skip rfs */
	else if (ret == -1)
		return (parAGoback);	/* go back */
	else if (ret == -2)
		return (parAExit);
	else
		return (_do_rfs_menu());
}

static int
_confirm_rfs()
{
	int ch;
	unsigned long fkeys;
	HelpEntry _help;

	_help.win = stdscr;
	_help.type = HELP_TOPIC;
	_help.title = "Mounting Remote File Systems";

	(void) werase(stdscr);
	(void) wclear(stdscr);

	wheader(stdscr, TITLE_MOUNTQUERY);
	(void) wword_wrap(stdscr, HeaderLines, INDENT0, COLS - (2 * INDENT0),
	    MSG_MOUNTQUERY);

	fkeys = F_CONTINUE | F_GOBACK | F_DOREMOTES | F_EXIT | F_HELP;

	for (;;) {

		wfooter(stdscr, fkeys);
		wcursor_hide(stdscr);
		ch = wzgetch(stdscr, fkeys);

		if (is_continue(ch) != 0) {

			break;

		} else if (is_goback(ch) != 0) {

			break;

		} else if (is_doremotes(ch) != 0) {	/* do remotes */

			break;

		} else if (is_exit(ch) != 0) {

			break;

		} else if (is_help(ch) != 0) {

			do_help_index(stdscr, _help.type, _help.title);

		} else if (is_escape(ch) != 0) {

			continue;

		} else
			beep();
	}

	if (is_continue(ch) != 0) {
		return (0);
	} else if (is_goback(ch) != 0) {	/* go back to disk config */
		return (-1);
	} else if (is_exit(ch) != 0) {
		return (-2);
	} else /* if (is_doremotes(ch) != 0) */ {	/* do remotes */
		return (1);
	}
}

static void
_free_opt_strings(int nrfs, ChoiceItem * opts)
{
	int i;

	for (i = 0; i < nrfs; i++) {
		if (opts[i].label != (char *) NULL)
			free((void *) opts[i].label);
	}
}

static ChoiceItem *
_load_rfs_opts(int nrfs, ChoiceItem * opts)
{
	int i;
	char buf[128];
	char buf1[128];
	char *ptr;
	char *mntpt;
	char *srvr;
	char *path;

	if (opts == (ChoiceItem *) NULL) {
		opts = (ChoiceItem *) xcalloc(nrfs * sizeof (ChoiceItem));
	} else {
		opts = (ChoiceItem *) xrealloc((void *) opts,
		    nrfs * sizeof (ChoiceItem));
	}

	for (i = 0; i < nrfs; i++) {

		mntpt = v_get_rfs_mnt_pt(i);
		srvr = v_get_rfs_server(i);
		path = v_get_rfs_server_path(i);

		switch (v_get_rfs_test_status(i)) {
		case V_TEST_SUCCESS:
			/* i18n: 10 chars max */
			ptr = gettext("Yes");
			break;

		case V_TEST_FAILURE:
			/* i18n: 10 chars max */
			ptr = gettext("No");
			break;

		case V_NOT_TESTED:
		default:
			/* i18n: 10 chars max */
			ptr = gettext("?");
			break;
		}

		if (srvr && *srvr && path && *path)
			(void) sprintf(buf1, "%-s:%-s", srvr, path);
		else if (srvr && *srvr)
			(void) strcpy(buf1, srvr);
		else if (path && *path)
			(void) strcpy(buf1, path);
		else
			buf1[0] = '\0';

		if (mntpt == NULL || *mntpt == '\0')
			mntpt = "";

		(void) sprintf(buf, "%-21.20s %-31.30s %-11.10s",
		    mntpt, buf1, ptr);

		opts[i].label = xstrdup(buf);
		opts[i].loc.c = INDENT1;
		opts[i].sel = -1;
		opts[i].help.type = HELP_REFER;
		opts[i].help.title = "Mount Remote File System Screen";
	}

	return (opts);
}

static parAction_t
_do_rfs_menu()
{

	int ch;
	int cur;
	int edit;
	int immed;
	int nrfs;		/* number of rfs		*/
	int rfs_per_page;	/* rfs items per menu page	*/
	int really_dirty;
	int dirty;
	int top;
	int top_row;
	int last_row;
	unsigned long fkeys;
	ChoiceItem *opts = (ChoiceItem *) NULL;

	HelpEntry _help;

	_help.win = stdscr;
	_help.type = HELP_REFER;
	_help.title = "Mount Remote File System Screen";

	really_dirty = 1;

	if (v_get_n_rfs() == 0) {
		immed = 1;
	}
	for (;;) {

		if (really_dirty == 1) {

			(void) werase(stdscr);
			(void) wclear(stdscr);

			/* show title */
			wheader(stdscr, REMOTE_TITLE);

			/* print headings */
			top_row = HeaderLines;
			(void) mvwprintw(stdscr, top_row, INDENT1,
			    "%-21.21s %-31.31s %-14.14s",
			/* i18n: 21 chars max. */
			    gettext("Local Mount Point"),
			/* i18n: 31 chars max. */
			    gettext("Server:Path"),
			/* i18n: 14 chars max. */
			    gettext("Test Mounted?"));

			++top_row;
			(void) mvwprintw(stdscr, top_row, INDENT1,
			    "%-.*s", 21 + 31 + 14 + 2, EQUALS_STR);
			++top_row;

			nrfs = v_get_n_rfs();
			opts = _load_rfs_opts(nrfs, opts);

			if (nrfs == 0) {
				/*
				 * put a place-holder entry into the option
				 * array...
				 */
				opts = (ChoiceItem *) xcalloc(1 *
				    sizeof (ChoiceItem));

#define	NO_RFS_STRING	gettext(\
	"<no remote file systems currently configured...>")

				opts[0].label = xstrdup(NO_RFS_STRING);
				opts[0].loc.c = INDENT1;
				opts[0].loc.r = top_row;
				opts[0].sel = -1;
				opts[0].help.type = HELP_NONE;
				opts[0].help.title = "";

				nrfs = 1;
				edit = 0;

				fkeys = F_CONTINUE | F_ADDNEW | F_HELP;

			} else {

				edit = 1;
				fkeys = F_CONTINUE | F_ADDNEW | F_GOBACK |
				    F_EDIT | F_HELP;

			}

			/* set up page dimensions */
			cur = 0;
			top = 0;
			rfs_per_page = LINES - top_row - FooterLines - 1;
			last_row = top_row + rfs_per_page - 1;

			dirty = 1;
			really_dirty = 0;

		}
		if (dirty == 1) {

			(void) show_choices(stdscr, nrfs, rfs_per_page,
			    top_row, INDENT1, opts, top);

			scroll_prompts(stdscr, top_row, 1, top, nrfs,
			    rfs_per_page);

			(void) wnoutrefresh(stdscr);
			wfooter(stdscr, fkeys);
			dirty = 0;
		}
		if (immed == 1) {

			/*
			 * if this is the `first' time into this menu, and
			 * there are no remote file systems, jump directly
			 * into the Add screen.
			 */
			_process_rfs(-1, 1, 1);	/* ignore, new, first */
			really_dirty = 1;
			immed = 0;
			continue;

		}
		wfocus_on(stdscr, opts[cur].loc.r, opts[cur].loc.c,
		    opts[cur].label);

		ch = wzgetch(stdscr, fkeys);

		wfocus_off(stdscr, opts[cur].loc.r, opts[cur].loc.c,
		    opts[cur].label);

		wnoutrefresh(stdscr);

		if (is_continue(ch) != 0) {

			break;

		} else if (is_goback(ch) != 0) {

			break;

		} else if (is_edit(ch) != 0 && (edit == 1)) {

			_process_rfs(cur, 0, 0); /* index, edit, notfirst */
			really_dirty = 1;

		} else if (is_addnew(ch) != 0) {

			_process_rfs(-1, 1, 1);	/* ignore, new, notfirst */
			really_dirty = 1;

		} else if (is_help(ch) != 0) {

			do_help_index(stdscr, _help.type, _help.title);

		} else if (is_escape(ch) != 0) {

			continue;

		} else if (ch == U_ARROW || ch == D_ARROW || ch == CTRL_U ||
			ch == CTRL_P || ch == CTRL_F || ch == CTRL_B ||
			ch == CTRL_N || ch == CTRL_P) {

			/* move */
			if (ch == CTRL_D) {

				/* page down */
				if ((cur + rfs_per_page) < nrfs) {

					/* advance a page */
					top += rfs_per_page;
					cur += rfs_per_page;
					dirty = 1;

				} else if (cur < (nrfs - 1)) {

					/* advance to last file system */
					cur = nrfs - 1;
					top = cur - 2;
					dirty = 1;

				} else
					beep(); /* at end */

			} else if (ch == CTRL_U) {

				/* page up */
				if ((cur - rfs_per_page) >= 0) {

					/* reverse a page */
					top = (top > rfs_per_page ?
					top - rfs_per_page : 0);
					cur -= rfs_per_page;
					dirty = 1;

				} else if (cur > 0) {

					/* back to first file system */
					top = 0;
					cur = 0;
					dirty = 1;

				} else
					beep(); /* at top */

			} else if (ch == U_ARROW || ch == CTRL_P ||
				ch == CTRL_B) {

				if (opts[cur].loc.r == top_row) {

					if (top) {	/* scroll down */
						cur = --top;
						dirty = 1;
					} else
						beep();	/* very top */

				} else
					cur--;

			} else if (ch == D_ARROW || ch == CTRL_N ||
				ch == CTRL_F) {

				if (opts[cur].loc.r == last_row) {

					if ((cur + 1) < nrfs) {

						/* scroll up */
						top++;
						cur++;
						dirty = 1;

					} else
						beep();	/* bottom */

				} else {

					if ((cur + 1) < nrfs)
						cur++;
					else
						beep();	/* last, no wrap */
				}

			}
		} else
			beep();
	}

	_free_opt_strings(nrfs, opts);

	if (opts) {
		free((void *) opts);
		opts = (ChoiceItem *) NULL;
	}
	if (is_continue(ch) != 0)
		return (parAContinue);
	else if (is_goback(ch) != 0)
		return (parAGoback);
	else
		return (parAContinue);
}

/*
 * this function deals with adding new and editing existing remote file
 * system specs.
 *
 * args:
 *	rfs:		index of rfs spec to edit
 *	addnew:		1 to add new rfs, 0 to edit
 *	first:		1 if very first add, 0 if not.
 */
static void
_process_rfs(int rfs, int addnew, int first)
{

	RFS_t rfs_spec;
	char *title;
	char *text;
	int index;
	unsigned long fkeys;
	HelpEntry _help;

	_help.win = stdscr;
	_help.type = HELP_REFER;
	_help.title = "Mount Remote File System Screen";

	/* load values into backing store */
	if (addnew) {
		*(rfs_spec.mnt_pt) = '\0';
		*(rfs_spec.server) = '\0';
		*(rfs_spec.ip_addr) = '\0';
		*(rfs_spec.server_path) = '\0';
	} else {
		(void) strncpy(rfs_spec.mnt_pt, v_get_rfs_mnt_pt(rfs),
		    MAXMNTLEN);
		(void) strncpy(rfs_spec.server, v_get_rfs_server(rfs),
		    MAXMNTLEN);
		(void) strncpy(rfs_spec.ip_addr, v_get_rfs_ip_addr(rfs),
		    16);
		(void) strncpy(rfs_spec.server_path,
		    v_get_rfs_server_path(rfs), MAXMNTLEN);
	}

	text = "";
	if (addnew) {

		index = -1;
		title = REMOTE_ADD_NEW_REMOTE_FILE_SYSTEM_TITLE;

		if (first)
			text = REMOTE_ADD_NEW_REMOTE_FILE_SYSTEM;

	} else {

		index = rfs;
		title = REMOTE_EDIT_REMOTE_FILE_SYSTEM_TITLE;
	}

	fkeys = F_CONTINUE | F_TESTMOUNT | F_SHOWEXPORTS | F_CANCEL |
	    F_HELP;

	if (get_rfs_spec(stdscr, title, text, &rfs_spec, &index, fkeys,
	    _help) == 0 && addnew == 1) {

		/* add of new RFS was cancelled, delete it */
		v_delete_rfs(index);
	}
}
