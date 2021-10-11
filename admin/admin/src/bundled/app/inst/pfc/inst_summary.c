#ifndef lint
#pragma ident "@(#)inst_summary.c 1.75 96/07/29 SMI"
#endif

/*
 * Copyright (c) 1993-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	inst_summary.c
 * Group:	ttinstall
 * Description:
 */

#include <locale.h>
#include <libintl.h>
#include <string.h>
#include <stdlib.h>

#include "pf.h"
#include "tty_pfc.h"
#include "inst_msgs.h"
#include "summary_util.h"

#include "v_types.h"
#include "v_check.h"
#include "v_rfs.h"
#include "v_lfs.h"
#include "v_sw.h"
#include "v_disk.h"
#include "v_misc.h"

#define	ERR_MSG_STRLEN	200

static enum Summary_Codes {
	START = 0,
	MODIFY = 1,
	ABORT = 2,
	EXIT = 3
};

parAction_t
do_initial_summary()
{
	int r, c;
	int ch;
	int top_row;
	int last_row;
	int lines_per_page;
	int nlines;
	int cur;
	int top;
	int dirty;
	int really_dirty = 1;
	unsigned long fkeys;
	parAction_t action;
	char	*buf, *buf1;
	int	error_found, warning_found, num_errors;
	Errmsg_t	*error_list;
	int	answer;
	char *helpString;

	_Summary_Row_t *table = (_Summary_Row_t *) NULL;

	HelpEntry _help;

	_help.win = stdscr;
	_help.type = HELP_TOPIC;
	_help.title = "Creating a Profile";

	cur = top = 0;
	fkeys = F_CHANGE | F_CONTINUE | F_EXIT | F_HELP;

	for (;;) {

		if (really_dirty) {

			(void) werase(stdscr);
			(void) wclear(stdscr);

			helpString = (char *) xmalloc(
				strlen(MSG_SUMMARY) +
				strlen(MSG_SUMMARY_CLIENT_SERVICES) +
				strlen(BOOTOBJ_SUMMARY_NOTE) + 1);

			/* add client services note, if applicable */
			if (get_machinetype() == MT_SERVER) {
				(void) sprintf(helpString, "%s%s",
				MSG_SUMMARY,
				MSG_SUMMARY_CLIENT_SERVICES);
			} else {
				(void) sprintf(helpString, "%s", MSG_SUMMARY);
			}

			/* add BIOS note, if applicable */
			if (IsIsa("i386") &&
				(BootobjCompare(CFG_EXIST, CFG_CURRENT, 0)
					!= D_OK))
					(void) sprintf(helpString, "%s%s",
						helpString,
						BOOTOBJ_SUMMARY_NOTE);

			wheader(stdscr, TITLE_PROFILE);
			top_row = HeaderLines;
			top_row = wword_wrap(stdscr, top_row, INDENT0,
			    COLS - (2 * INDENT0), helpString);
			top_row++;

			(void) mvwprintw(stdscr, top_row, INDENT0, "%.*s",
			    COLS - (2 * INDENT0), EQUALS_STR);
			top_row += 2;

			wfooter(stdscr, fkeys);
			(void) wnoutrefresh(stdscr);

			nlines = 0;
			table = load_install_summary(&nlines);

			lines_per_page = LINES - FooterLines - top_row - 1;
			last_row = top_row + lines_per_page - 1;

			dirty = 1;
			really_dirty = 0;
		}
		if (dirty) {
			show_summary_table(stdscr, nlines, lines_per_page,
			    top_row, table, top);

			scroll_prompts(stdscr, top_row, 1, top, nlines,
			    lines_per_page);

			(void) wnoutrefresh(stdscr);
			dirty = 0;
		}
		/* set footer */
		if (table[cur].fld[0].prompt != (char *) 0 &&
		    table[cur].fld[0].prompt[0] != '\0') {
			wstatus_msg(stdscr, table[cur].fld[0].prompt);
		} else {
			wclear_status_msg(stdscr);
		}

		(void) getsyx(r, c);
		(void) wnoutrefresh(stdscr);
		(void) setsyx(r, c);
		(void) doupdate();

		if (nlines > lines_per_page)
			(void) wmove(stdscr, table[cur].fld[1].loc.r,
			    table[cur].fld[1].loc.c - 1);
		else
			wcursor_hide(stdscr);

		ch = wzgetch(stdscr, fkeys);

		if (ch == U_ARROW || ch == D_ARROW ||
		    ch == CTRL_N || ch == CTRL_D || ch == CTRL_F ||
		    ch == CTRL_P || ch == CTRL_U || ch == CTRL_B) {

			dirty = 0;

			/* move */
			if (ch == CTRL_D) {

				/* page down */
				if ((cur + lines_per_page) < nlines) {

					/* advance a page */
					top += lines_per_page;
					cur += lines_per_page;
					dirty = 1;

				} else if (cur < (nlines - 1)) {

					/* advance to last line */
					cur = nlines - 1;
					top = cur - 2;
					dirty = 1;

				} else
					beep();	/* at end */

			} else if (ch == CTRL_U) {

				/* page up */
				if ((cur - lines_per_page) >= 0) {

					/* reverse a page */
					top = (top > lines_per_page ?
					    top - lines_per_page : 0);
					cur -= lines_per_page;
					dirty = 1;

				} else if (cur > 0) {

					/* back to first line */
					top = 0;
					cur = 0;
					dirty = 1;

				} else
					beep();	/* at top */

			} else if (ch == U_ARROW || ch == CTRL_P ||
			    ch == CTRL_B) {

				if (table[cur].fld[0].loc.r == top_row) {

					if (top) {	/* scroll down */
						cur = --top;
						dirty = 1;
					} else
						beep();	/* very top */

				} else {
					cur--;
				}

			} else if (ch == D_ARROW || ch == CTRL_N ||
			    ch == CTRL_F) {

				if (table[cur].fld[0].loc.r == last_row) {

					if ((cur + 1) < nlines) {

						/* scroll up */
						top++;
						cur++;
						dirty = 1;

					} else
						beep();	/* bottom */

				} else {

					if ((cur + 1) < nlines) {
						cur++;
					} else
						beep();	/* last, no wrap */
				}
			}
		} else if (is_change(ch) != 0) {

			action = parAChange;
			break;

		} else if (is_continue(ch) != 0) {

			/*
			 * check the state of the disks before continuing
			 * it is possible that the user configured the disks
			 * such that the install will fail or the system
			 * will not be bootable after an install, the user must
			 * select Change at this time to "fix" the disks
			 */

			num_errors = check_disks();
			write_debug(CUI_DEBUG_L1,
				"check_disks - %d errors", num_errors);
			if (num_errors > 0) {
				/*
				 * some combination of errors and/or warnings
				 * was found, walk the list and find out if
				 * there are any errors, if there are only
				 * warnings the user will be allowed procede,
				 * if there are any errors, an error message
				 * will be displayed listing those errors
				 */
				error_found = 0;
				warning_found = 0;
				buf = xmalloc(ERR_MSG_STRLEN * num_errors);
				buf1 = (char *) xmalloc(sizeof (buf) + 600);
				WALK_LIST(error_list, get_error_list()) {
					if (error_list->code < 0) {
						error_found = 1;
						(void) sprintf(buf, "%s %s\n",
							gettext("ERROR:"),
							error_list->msg);
					} else if (error_list->code > 0) {
						warning_found = 1;
						(void) sprintf(buf, "%s %s\n",
							gettext("WARNING:"),
							error_list->msg);
					}
				}

				if (error_found == 1 || warning_found == 1) {
					/*
					 * there were errors found, display
					 * an error message
					 */
					(void) sprintf(buf1, "%s\n\n%s",
						ERR_CHECK_DISKS, buf);
					answer = yes_no_notice(stdscr,
						F_OKEYDOKEY | F_CANCEL,
						F_OKEYDOKEY, F_CANCEL,
						TITLE_WARNING,
						buf1);
					if (answer == F_OKEYDOKEY) {
						/*
						 * the errors have been accepted
						 * continue to the reboot query
						 */
						action = parAContinue;
						free(buf1);
					} else if (answer == F_CANCEL) {
						/*
						 * just clear the error screen
						 * and return to the profile
						 * window
						 */
						(void) werase(stdscr);
						(void) wclear(stdscr);
						(void) wnoutrefresh(stdscr);
						action = parANone;
						free(buf1);
					}

				}
			} else {
				/*
				 * no errors or warnings were found
				 * continue normally
				 */

				action = parAContinue;
			}

			break;

		} else if (is_help(ch) != 0) {

			do_help_index(_help.win, _help.type, _help.title);

		} else if (is_escape(ch) != 0) {

			continue;

		} else if (is_exit(ch) != 0) {

			if (confirm_exit(stdscr) == 1) {

				action = parAExit;
				break;

			} else {
				beep();
			}

		} else
			beep();
	}

	free_summary_table(table, nlines);
	return (action);
}

_Summary_Row_t *
load_install_summary(int *row)
{
	int last = 32;

	char buf[128];

	_Summary_Row_t *table = (_Summary_Row_t *) NULL;

	/*
	 * start table with 32 rows...
	 */
	table = (_Summary_Row_t *) xcalloc(last * sizeof (_Summary_Row_t));

	table = load_init_summary(table, row, &last);

	if (pfgState & AppState_UPGRADE)
		table = load_upg_summary(table, row, &last);

	/*
	 * machine type
	 */
	if (!(pfgState & AppState_UPGRADE)) {
		switch (v_get_system_type()) {
		case V_STANDALONE:
			/* no client services */
			(void) sprintf(buf, "%*.*s:",
				SUMMARY_LABEL_LEN,
				SUMMARY_LABEL_LEN,
				INSTALL_CLIENT_SERVICES);
			table[*row].fld[0].label = xstrdup(buf);
			table[*row].fld[0].loc.c = 0;
			table[*row].fld[0].prompt = (char *) 0;
			table[*row].fld[0].sel_proc = NULL;

			(void) sprintf(buf, "%.*s",
				SUMMARY_LABEL_LEN,
				INSTALL_NONE);
			table[*row].fld[1].label = xstrdup(buf);
			table[*row].fld[1].loc.c = SUMMARY_VALUE_COLUMN;
			table[*row].fld[1].prompt = (char *) 0;
			++(*row);
			table = grow_summary_table(table, row, &last);

			break;
		case V_SERVER:

			/* diskless clients */
			if (v_get_n_diskless_clients() > 0) {

				/* Number of clients */
				(void) sprintf(buf, "%*.*s:",
					SUMMARY_LABEL_LEN,
					SUMMARY_LABEL_LEN,
					INSTALL_NUMCLIENTS);
				table[*row].fld[0].label = xstrdup(buf);
				table[*row].fld[0].loc.c = 0;
				table[*row].fld[0].prompt = (char *) 0;

				(void) sprintf(buf, "%d",
					v_get_n_diskless_clients());
				table[*row].fld[1].label = xstrdup(buf);
				table[*row].fld[1].loc.c =
					SUMMARY_VALUE_COLUMN;
				table[*row].fld[1].prompt = (char *) 0;
				++(*row);
				table = grow_summary_table(table, row, &last);

				/* Swap per client */
				(void) sprintf(buf, "%*.*s:",
					SUMMARY_LABEL_LEN,
					SUMMARY_LABEL_LEN,
					INSTALL_SWAP_PER_CLIENT);
				table[*row].fld[0].label = xstrdup(buf);
				table[*row].fld[0].loc.c = 0;
				table[*row].fld[0].prompt = (char *) 0;

				(void) sprintf(buf, "%d",
					v_get_diskless_swap());
				table[*row].fld[1].label = xstrdup(buf);
				table[*row].fld[1].loc.c =
					SUMMARY_VALUE_COLUMN;
				table[*row].fld[1].prompt = (char *) 0;
				++(*row);
				table = grow_summary_table(table, row, &last);

				/* Root per client */
				(void) sprintf(buf, "%*.*s:",
					SUMMARY_LABEL_LEN,
					SUMMARY_LABEL_LEN,
					INSTALL_ROOT_PER_CLIENT);
				table[*row].fld[0].label = xstrdup(buf);
				table[*row].fld[0].loc.c = 0;
				table[*row].fld[0].prompt = (char *) 0;

				(void) sprintf(buf, "%d",
					v_get_root_client_size());
				table[*row].fld[1].label = xstrdup(buf);
				table[*row].fld[1].loc.c =
					SUMMARY_VALUE_COLUMN;
				table[*row].fld[1].prompt = (char *) 0;
				++(*row);
				table = grow_summary_table(table, row, &last);

				/* show supported client architectures */
				table = load_client_arch_summary(
					table, row, &last);

			}
#ifdef CACHE_ONLY_CLIENTS
			if (v_get_n_cache_clients() > 0)
				(void) sprintf(buf, "%*.*s:",
					SUMMARY_LABEL_LEN,
					SUMMARY_LABEL_LEN,
					gettext("Cache only clients"));
			table[*row].fld[0].label = xstrdup(buf);
			table[*row].fld[0].loc.c = 0;
			table[*row].fld[0].prompt = (char *) 0;

			(void) sprintf(buf, "%d", v_get_n_cache_clients());
			table[*row].fld[1].label = xstrdup(buf);
			table[*row].fld[1].loc.c = SUMMARY_VALUE_COLUMN;
			table[*row].fld[1].prompt = (char *) 0;

			++(*row);
			table = grow_summary_table(table, row, &last);
#endif				/* CACHE_ONLY_CLIENTS */
			break;
		}
	}

	table = load_locale_summary(table, row, &last);
	table = load_sw_summary(table, row, &last);

	/*
	 * File System and Disk Layout summary
	 * - displayed in DSR upgrade and initial
	 */
	if ((pfgState & AppState_UPGRADE_DSR) ||
		!(pfgState & AppState_UPGRADE)) {
		table = load_lfs_summary(table, row, &last);
	}
	table = load_rfs_summary(table, row, &last);

	return (table);
}

/*
 * puts up the reboot `notice'.
 *
 * input:
 *	parent:		WINDOW * to curses window to repaint when notice
 *			is dismissed.
 *
 * returns:
 *	1 - user really wants to reboot
 * 	0 - user does not want to reboot
 *
 */
int
_confirm_reboot()
{
	int last;
	int cur;
	int row;
	int ch;
	unsigned long fkeys;

	ChoiceItem opts[2];

	opts[0].label = INSTALL_REBOOT_YES;
	opts[0].sel = 1;
	opts[0].loc.c = INDENT2;
	opts[0].help = HelpGetTopLevelEntry();

	opts[1].label = INSTALL_REBOOT_NO;
	opts[1].sel = 0;
	opts[1].loc.c = INDENT2;
	opts[1].help = HelpGetTopLevelEntry();

	(void) werase(stdscr);
	(void) wclear(stdscr);

	wheader(stdscr, TITLE_REBOOT);
	row = HeaderLines;
	row = wword_wrap(stdscr, row, INDENT0, COLS - (2 * INDENT0),
	    MSG_REBOOT);
	row += 2;

	fkeys = F_BEGIN | F_CANCEL;
	last = cur = 0;

	for (;;) {

		(void) show_choices(stdscr, 2, 5, row, INDENT2, opts, 0);

		wfooter(stdscr, fkeys);

		wfocus_on(stdscr, opts[cur].loc.r, INDENT2 - 4,
		    opts[cur].sel ? Sel : Unsel);

		ch = wzgetch(stdscr, fkeys);

		if (ch == D_ARROW || ch == U_ARROW || ch == CTRL_P ||
		    ch == CTRL_N || ch == CTRL_U || ch == CTRL_D) {

			wfocus_off(stdscr, opts[cur].loc.r, INDENT2 - 4,
			    opts[cur].sel ? Sel : Unsel);

			cur = !cur;

		} else if ((sel_cmd(ch) != 0) || (alt_sel_cmd(ch) != 0)) {

			if (opts[cur].sel == 0) {

				opts[cur].sel = 1;
				opts[last].sel = 0;
				last = cur;

			} else if (opts[cur].sel == 1) {

				opts[cur].sel = 0;
				opts[last].sel = 1;

			}
		} else if (is_begin(ch) != 0) {

			break;

		} else if (is_cancel(ch) != 0) {

			break;

		} else if (is_escape(ch) != 0) {

			continue;

		} else
			beep();
	}

	if (is_begin(ch) != 0) {
		if (opts[1].sel == 1)	/* do not reboot */
			return (0);
		else
			return (1);
	} else /* if (is_cancel(ch) != 0) */
		return (-1);
}
