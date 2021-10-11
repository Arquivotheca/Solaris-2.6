#ifndef lint
#pragma ident "@(#)tty_list.c 1.6 96/07/30 SMI"
#endif

/*
 * Copyright (c) 1993-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	tty_list.c
 * Group:	libspmitty
 * Description:
 *	Module for displaying a read-only data list that looks
 *	something like:
 *
 *		Label (possibly multi-line)
 *		-------------------------------------------------------
 *	-	entry1
 *	|	entry2
 *	|	entry3
 *	|	entry4
 *	|	entry5
 *	V	entry6
 *
 *
 *	The scroll bars only show up if necessary.
 *	Special keys that are handled internally:
 *		- help
 *		- exit: if an exit confirmation proc is provided,
 *			then it is called and the list is exitted if the
 *			confirmation was positive.  If no exit proc is
 *			supplied, an exit  exits immediately.
 */

#include "spmitty_api.h"
#include "tty_utils.h"

static void
show_scrolling_list_entries(
	WINDOW *win,
	ttyScrollingListTable *entries,
	int num_entries,
	int lines_per_page,
	int start_row,
	int start_col,
	int width,
	int first_entry);


/*
 * Function:	show_scrolling_list
 * Description:
 *	Provide read-only scrolling list capability
 *	See module description above.
 *
 * Scope:	PUBLIC
 * Parameters:
 *	WINDOW *win:
 *		curses window to print to
 *	int start_row:
 *		row for top of list to start (top row of label if there
 *		is one)
 *	int start_col:
 *		column for left hand side of list
 *	int height:
 *		number of rows list can use
 *	int width:
 *		number of columns wide list is
 *	char *list_label:
 *		list label
 *	int num_label_rows:
 *		how many rows tall is label
 *	ttyScrollingListTable *entries:
 *		list entries array
 *	int num_entries:
 *		number of entries in list array
 *	HelpEntry _help:
 *		help data
 *	Callback_ExitProc exit_proc:
 *		exit confirmation routine
 *	u_long  fkeys:
 *		which keys are legal
 * Return:
 *	fkey that caused exit from list routine
 * Globals: none
 * Notes:
 */
int
show_scrolling_list(
	WINDOW *win,
	int start_row,
	int start_col,
	int height,
	int width,
	char **label_rows,
	int num_label_rows,
	ttyScrollingListTable *entries,
	int num_entries,
	HelpEntry _help,
	Callback_ExitProc exit_proc,
	u_long  fkeys)
{
	int really_dirty = 1;
	int dirty;
	int lines_per_page;
	int list_start_row;
	int list_last_row;
	int i;
	int row;
	int curr;
	int top;
	int r, c;
	int ch;

	/*
	 * Make sure the width they gave us is within reasonable limits...
	 * i.e. the width of the screen minus an equal size buffer
	 * on each side of the screen.
	 */
	if (width > (COLS - (start_col * 2)))
		width = COLS - (start_col * 2);

	/*
	 * makre sure that there is enough room on the left side for a
	 * scroll bar
	 * (i.e. start_col must be at least 1 - at least 3 is preferable)
	 */
	if (start_col < 1)
		start_col = 1;

	if (label_rows) {
		/* display the label */
		for (i = 0, row = start_row; i < num_label_rows;
			i++, row++) {
			wmove(win, row, 0);
			(void) wclrtoeol(win);

			(void) mvwprintw(win, row, start_col,
				"%-*.*s", width, width, label_rows[i]);
		}

		/* draw the separator line */
		wmove(win, start_row + num_label_rows, start_col);
		whline(win, ACS_HLINE, width);

		/* adjust for label height */
		lines_per_page = height - num_label_rows - 1;
		list_start_row = start_row + num_label_rows + 1;
	} else {
		/* adjust for label height */
		lines_per_page = height;
		list_start_row = start_row;
	}

	list_last_row = list_start_row + lines_per_page - 1;
	curr = top = 0;
	for (;;) {
		/* if we really have to redraw all the data in the list */
		if (really_dirty) {
			/* totally clear the scrolling list portion of screen */

			/* clear list */
			for (i = 0, row = list_start_row; i < lines_per_page;
				i++, row++) {
				wmove(win, row, 0);
				(void) wclrtoeol(win);
			}

			dirty = 1;
			really_dirty = 0;
		}
		if (dirty) {
			show_scrolling_list_entries(win,
				entries,
				num_entries,
				lines_per_page,
				list_start_row,
				start_col,
				width,
				top);

			scroll_prompts(win,
				list_start_row,
				start_col >= 2 ?
					/* 1 space between scrollbar and list */
					start_col - 2 :
					/* 0 space between scrollbar and list */
					start_col - 1,
				top,
				num_entries,
				lines_per_page);

			(void) wnoutrefresh(win);
			dirty = 0;
		}

		(void) getsyx(r, c);
		(void) wnoutrefresh(win);
		(void) setsyx(r, c);
		(void) doupdate();

		/* move the cursor to the scroll position */
		if (num_entries > lines_per_page)
			(void) wmove(win,
				entries[curr].row,
				start_col - 1);
		else
			wcursor_hide(win);

		ch = wzgetch(win, fkeys);

		if (ch == U_ARROW || ch == D_ARROW ||
		    ch == CTRL_N || ch == CTRL_D || ch == CTRL_F ||
		    ch == CTRL_P || ch == CTRL_U || ch == CTRL_B) {

			dirty = 0;

			/* move */
			if (ch == CTRL_D) {

				/* page down */
				if ((curr + lines_per_page) < num_entries) {

					/* advance a page */
					top += lines_per_page;
					curr += lines_per_page;
					dirty = 1;

				} else if (curr < (num_entries - 1)) {

					/* advance to last line */
					curr = num_entries - 1;
					top = curr - 2;
					dirty = 1;

				} else
					beep();	/* at end */

			} else if (ch == CTRL_U) {

				/* page up */
				if ((curr - lines_per_page) >= 0) {

					/* reverse a page */
					top = (top > lines_per_page ?
					    top - lines_per_page : 0);
					curr -= lines_per_page;
					dirty = 1;

				} else if (curr > 0) {

					/* back to first line */
					top = 0;
					curr = 0;
					dirty = 1;

				} else
					beep();	/* at top */

			} else if (ch == U_ARROW || ch == CTRL_P ||
			    ch == CTRL_B) {

				if (entries[curr].row == list_start_row) {

					if (top) {	/* scroll down */
						curr = --top;
						dirty = 1;
					} else
						beep();	/* very top */

				} else {
					curr--;
				}

			} else if (ch == D_ARROW || ch == CTRL_N ||
			    ch == CTRL_F) {

				if (entries[curr].row == list_last_row) {

					if ((curr + 1) < num_entries) {

						/* scroll up */
						top++;
						curr++;
						dirty = 1;

					} else
						beep();	/* bottom */

				} else {

					if ((curr + 1) < num_entries) {
						curr++;
					} else
						beep();	/* last, no wrap */
				}
			}
		} else if (is_escape(ch)) {
			continue;
		} else if (tty_CheckInput(fkeys, ch)) {
			/*
			 * figure out if the key press was one of the
			 * keys that were passed in as valid keys.
			 * If so, handle any standard ones and return if
			 * appropriate,
			 * Otherwise, beep and stay in the input loop
			 */
			if (is_help(ch))  {
				do_help_index(
					_help.win, _help.type, _help.title);
			} else if (is_exit(ch)) {
				if (exit_proc) {
					if ((*exit_proc)(stdscr))
						return (ch);
				} else
					return (ch);
			} else
				return (ch);
		} else
			beep();
	}
}

/*
 * Function:	show_scrolling_list_entries
 * Description:
 *	internal function used to keep the currently displayed entries
 *	in the entries array mapped to the rows that they are displayed
 *	on and to actually display them.
 *
 * Scope:	INTERNAL
 * Parameters:
 *	WINDOW *win,
 *		curses window to print to
 *	ttyScrollingListTable *entries,
 *		list entries array
 *	int num_entries:
 *		number of entries in list array
 *	int lines_per_page:
 *		how many entries of the can be displayed at once
 *    	int start_row:
 *		row for top of list (first visible entry)
 *    	int start_col:
 *		column for left hand side of list
 *    	int width:
 *		list width
 *	int first_entry:
 *		index # of entry on the entry list that should be
 *		displayed in the first position
 *
 * Return:
 * Globals:
 * Notes:
 */
static void
show_scrolling_list_entries(
	WINDOW *win,
	ttyScrollingListTable *entries,
	int num_entries,
	int lines_per_page,
	int start_row,
	int start_col,
	int width,
	int first_entry)
{
	int lines_displayed;
	int curr_entry;
	int row;

	curr_entry = first_entry;
	for (lines_displayed = 0, row = start_row;
		(lines_displayed < lines_per_page) &&
		(curr_entry < num_entries);
		lines_displayed++, curr_entry++, row++) {

		wmove(win, row, 0);
		(void) wclrtoeol(win);

		/* cut off the entry if it's too long */
		(void) mvwprintw(win, row, start_col,
			"%.*s",
			width,
			entries[curr_entry].str ? entries[curr_entry].str : "");

		entries[curr_entry].row = row;
	}

	/* clear out any remaining lines if we didn't fill up the list */
	for (; lines_displayed < lines_per_page; lines_displayed++, row++) {
		wmove(win, row, 0);
		(void) wclrtoeol(win);
	}

	(void) wredrawln(win, start_row, start_row + lines_per_page);
}
