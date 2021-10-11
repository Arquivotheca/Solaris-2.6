#ifndef lint
#pragma ident "@(#)tty_utils.c 1.8 96/07/26 SMI"
#endif

/*
 * Copyright (c) 1993-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	tty_utils.c
 * Group:	libspmitty
 * Description:
 *	Generic routines to init/start/end curses and
 *	handle generic input functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <libintl.h>
#include <sys/types.h>

#include "spmitty_api.h"
#include "tty_utils.h"
#include "tty_strings.h"

int erasech;
int killch;
int curses_on = FALSE;

static int _clear_screen = 1;
static int _force_alternates = 0;

int
init_curses(void)
{
	write_debug(TTY_DEBUG_L1, "entering init_curses");

	/* if curses already on */
	if (curses_on == TRUE)
		return (1);

	(void) start_curses();

	/* make sure we have a big enough window */
	if (LINES < MINLINES || COLS < MINCOLS) {

		end_curses(get_clearscr(), FALSE);
		(void) fprintf(stderr, TTY_ROWCOL_SIZE_ERR, MINLINES, MINCOLS);
		(void) fprintf(stderr, TTY_ROWCOL_SIZE_INFO_ERR, LINES, COLS);
		(void) fflush(stderr);
		return (0);
	}
	write_debug(TTY_DEBUG_L1, "leaving init_curses");
	return (1);
}

int
start_curses(void)
{
	write_debug(TTY_DEBUG_L1, "entering start_curses");

	if (curses_on == TRUE)
		return (0);

	if (initscr()) {

		/*
		 * init color stuff.
		 */
		if (start_color() == OK) {
			wcolor_init();

			HeaderLines = 2;
			FooterLines = 2;
		}
		(void) cbreak();
		(void) noecho();
		(void) leaveok(stdscr, FALSE);
		(void) scrollok(stdscr, FALSE);
		(void) keypad(stdscr, TRUE);
		(void) typeahead(-1);

		erasech = erasechar();
		killch = killchar();

		curses_on = TRUE;

		/*
		 * bug id 1161823 - we need to turn off the IEXTEN bit - having
		 * it on will cause the down arrow to not work properly.
		 * Eventually, raw mode will take care of this, but possibly not
		 * until after 495.
		 */
		(void) system("/bin/stty -iexten");

		write_debug(TTY_DEBUG_L1, "leaving start_curses");
		return (0);
	}
	write_debug(TTY_DEBUG_L1, "leaving start_curses");

	return (-1);
}

void
end_curses(int clear, int top)
{
	if (curses_on == FALSE)
		return;

	if (has_colors() != 0)
		wcolor_set_bkgd(stdscr, NORMAL);

	if (clear != 0)
		(void) erase();

	(void) nocbreak();
	(void) echo();

	if (top != 0)
		(void) move(stdscr->_maxy, 0);
	else
		(void) wmove(stdscr, 0, 0);

	if (clear != 0)
		(void) refresh();

	(void) endwin();
	(void) fflush(stdout);
	(void) fflush(stderr);

	/*
	 * bug id 1161823 - we need to turn the IEXTEN bit back on.
	 * see comment for start_curses()
	 */
	(void) system("/bin/stty iexten");

	curses_on = FALSE;
}

int
wzgetch(WINDOW * w, u_long fkeys)
{
	static int esc_mode = 0;
	int ready = 0;
	int c = 0;
	int x, y;

	if (curses_on == TRUE) {
		(void) getsyx(y, x);

		(void) wnoutrefresh(w);

		while (!ready) {
			(void) doupdate();
			c = getch();

			if (c == '\014') {
				(void) touchwin(curscr);
				(void) wrefresh(curscr);
				(void) setsyx(y, x);
			} else if (c == ESCAPE) {

				if (!esc_mode) {
					_force_alternates = 1;
					(*_fkeys_init_func)(_force_alternates);
					/* notimeout(w, TRUE); */
					wfooter(w, fkeys);
					(void) setsyx(y, x);
					esc_mode = 1;
				}
			} else if (esc_mode) {
				switch (c) {
				case '1':
					c = KEY_F(1);
					break;
				case '2':
					c = KEY_F(2);
					break;
				case '3':
					c = KEY_F(3);
					break;
				case '4':
					c = KEY_F(4);
					break;
				case '5':
					c = KEY_F(5);
					break;
				case '6':
					c = KEY_F(6);
					break;
				case '7':
					c = KEY_F(7);
					break;
				case '8':
					c = KEY_F(8);
					break;
				case '9':
					c = KEY_F(9);
					break;
				case 'f':	/* turn off escape mode */
				case 'F':
					c = ESCAPE;	/* ??? */
					_force_alternates = 0;
					(*_fkeys_init_func)(_force_alternates);
					wfooter(w, fkeys);
					(void) setsyx(y, x);
					break;
				default:
					/* By default, don't change "c" */
					break;
				}
				/* notimeout(w, FALSE); */
				esc_mode = 0;
				ready = 1;
			} else
				ready = 1;
		}
		(void) setsyx(y, x);
	} else {

		(void) fflush(stdout);
		(void) fflush(stderr);

		c = getchar();
	}

	return (c);
}

void
flush_input(void)
{
	/*
	 * try to flush any pending input
	 */
	(void) tcflush(0, TCIFLUSH);
	if (curses_on)
		(void) flushinp();
}

void
set_clearscr(int val)
{
	_clear_screen = (val != 0);
}

int
get_clearscr(void)
{
	return (_clear_screen);
}
/*
 * peeks at next character of input without removing it from the input queue
 */
int
peekch(void)
{
	static int ch;

	ch = wzgetch(stdscr, 0);
	ungetch(ch);
	return (ch);
}

void
tty_cleanup(void)
{
	end_curses(get_clearscr(), FALSE);
}

/*
 * count number of lines 'width' wide in string, return count
 */
int
count_lines(char *str, int w)
{
	register char  *cp, *cp1;
	register int n;
	char *start;
	int nrows;

	cp = start = xstrdup(str);

	nrows = 0;
	while (cp && *cp) {

		n = 0;
		cp1 = cp;

		while (cp && *cp && (*cp != '\n') && (n < w)) {
			++cp;
			++n;
		}

		if (*cp == '\0')
		    ++nrows;
		else if (*cp == '\n') {
			++nrows;
			++cp;
		} else if (n == '\t') {
			n += 8;		/* XXX get normal tabs stops */
			++cp;
		} else if (n >= w) {

			/*
			 * no newline, find a space, break line there
			 */
			while ((*cp != ' ') && (cp != cp1))
			    --cp;

			if (cp == cp1) {
				/* no space, just chop at (cp + w)'th char */
				cp += w;
				++nrows;
			} else if (*cp == ' ') {
				++cp;
				++nrows;
			}
		}
	}

	if (start)
	    free(start);

	return (nrows);

}

int
tty_GetForceAlternates(void) {
	return (_force_alternates);
}

/*
 * See if the character passed in maps to one of the keys in the fkeys mask.
 */
int
tty_CheckInput(ulong which_fkeys, int ch)
{
	int i;
	int fkey_num;

	for (i = 0; i < _num_fkeys; i++) {
		/*
		 * does this _fkeys key entry match one of the keys in
		 * the mask?
		 */
		if ((which_fkeys & (1 << i))) {
			/*
			 * Ok, now I have a pointer into the _fkeys
			 * for one of the valid fkeys (one of which_keys).
			 * Use the f_special field to get the "F#" field.
			 * Then use this # to find out if ch
			 * matches this as input.
			 */
			(void) sscanf(_fkeys[i].f_special, "F%d", &fkey_num);
			if (is_fkey_num(ch, fkey_num))
				return (TRUE);
		}
	}
	return (FALSE);
}

/*
 * Function:	tty_GetRowColData
 * Description:
 *	Format labels and row/columns so all columns line up and
 *	single or multi-line labels get formatted correctly.
 *	i.e. it figures out how to format a table heading and it's
 *	rows so that the columns all line up...
 *
 *	e.g.:
 *	File System    Slice      Original     Required
 *                                Size (MB)    Size (MB)
 *	------------------------------------------------
 *	/usr           c0t0d0s0         20            25
 *	/usr/openwin   c0t0d0s1         15            16
 *
 *
 * Scope:	PUBLIC
 * Parameters:
 *	ttyLabelRowData *row_data:
 *		an array of pointers to an array of character pointers.
 *		The data in here is the data that the table actually
 *		gets filled with.
 *		i.e. there is an array of pointers for each row of data.
 *		Each row of data consists of an array of char * that
 *		contain the actual data for each column in that row.
 *		So,
 *			- row_data is an array.
 *			- row_data[0] holds information for the whole
 *				"/usr" row;  It is a pointer to an array
 *				of char *'s.
 *			- row_data[0][1]: the string "/usr"
 *			- row_data[0][2]: the string "c0t0d0s0"
 *			- etc...
 *	int num_rows:
 *		how many rows of data do we have
 *	ttyLabelColData *col_data:
 *		array of information about each column of data.
 *		This is initially seeded with just the column headings
 *		for each column.  Column headings may have embedded
 *		newlines.
 *	int num_cols:
 *		how many columns of data are there.
 *	int space_len:
 *		how many spaces to place between columns in the table
 *	char ***label_rows:
 *		address of an array of pointers to char * that will be
 *		computed and returned.
 *		The array is an array of strings for each line in the
 *		column headers.
 *		e.g. the column headers in the description above consist
 *		of two rows - each of which is an entry in the array.
 *	int *num_label_rows:
 *		how many label rows are there.
 *	char ***entries:
 *		address of array of char *'s that are the actual rows in
 *		the table, formatted well and returned.
 *	int *entry_row_len:
 *		length of the longest entry row
 *
 * Return:	none
 * Globals:	none
 * Notes:
 */
void
tty_GetRowColData(
	ttyLabelRowData *row_data,
	int num_rows,
	ttyLabelColData *col_data,
	int num_cols,
	int space_len,
	char ***label_rows,
	int *num_label_rows,
	char ***entries,
	int *entry_row_len)
{
	int col;
	int row;
	int max_width;
	int heading_width;
	int row_cnt;
	int row_index;
	char *ptr;
	char *start;
	int row_len;

	if (entry_row_len)
		*entry_row_len = 0;
	*num_label_rows = 1;
	row_len = ((num_cols - 1) * space_len);
	for (col = 0; col < num_cols; col++) {
		/*
		 * figure out how many rows are in this column heading
		 * and how wide the largest segment is
		 */
		heading_width = 0;
		start = col_data[col].heading;
		for (ptr = strchr(start, '\n'), row_cnt = 1;
			ptr;
			ptr = strchr(start, '\n'), row_cnt++) {
			*ptr = '\0';
			heading_width = MAX(heading_width, strlen(start));
			*ptr = '\n';
			start = ptr;
			start++;
		}
		if (row_cnt == 1) {
			heading_width = col_data[col].heading ?
				strlen(col_data[col].heading) : 0;
		}
		heading_width = MAX(heading_width, strlen(start));
		col_data[col].heading_rows = row_cnt;
		*num_label_rows = MAX(*num_label_rows, row_cnt);

		/* get the max width of the column */
		max_width = heading_width;

		/*
		 * if the width has already been set, then just take the
		 * max of that width and the column label.
		 * otherwise get the max width of all the entries and
		 * the label.
		 */
		if (col_data[col].max_width) {
			max_width = MAX(col_data[col].max_width, heading_width);
		} else {
			for (row = 0; row < num_rows; row++) {
				max_width = MAX(max_width,
					row_data[row][col] ?
					strlen(row_data[row][col]) : 0);
			}
		}
		col_data[col].max_width = max_width;
		row_len += max_width;

	}

	/*
	 * Put together the label strings
	 */
	(*label_rows) = (char **) xcalloc(*num_label_rows * (sizeof (char *)));
	for (row = 0; row < *num_label_rows; row++) {
		(*label_rows)[row] = xmalloc(row_len + 1);
		(void) strcpy((*label_rows)[row], "");


		/* add on this row's column headings */
		for (col = 0; col < num_cols; col++) {
			/* stick spacer on the front */
			if (col != 0) {
				(void) sprintf((*label_rows)[row], "%s%*.*s",
					(*label_rows)[row],
					space_len,
					space_len,
					SPACES_STR);
			}

			/*
			 * just add spaces if this label doesn't have
			 * anything in this row
			 */
			if (col_data[col].heading_rows <= row) {
				(void) sprintf((*label_rows)[row], "%s%*.*s",
					(*label_rows)[row],
					col_data[col].max_width,
					col_data[col].max_width,
					SPACES_STR);
					continue;
			}

			/*
			 * This label has something in this row.
			 * Add this segment to the row.
			 */
			start = col_data[col].heading;
			for (ptr = strchr(start, '\n'), row_index = 0;
				ptr && row_index < row;
				ptr = strchr(start, '\n'), row_index++) {
				start = ptr;
				start++;
			}
			/*
			 * Handle the case where we fell thru the above
			 * loop w/o doing anything because there are no \n's
			 */
			if (ptr) {
				*ptr = '\0';

				/* add the segment */
				(void) sprintf((*label_rows)[row], "%s%-*.*s",
					(*label_rows)[row],
					col_data[col].max_width,
					col_data[col].max_width,
					start);

				*ptr = '\n';
			} else {
				/* add the segment */
				(void) sprintf((*label_rows)[row], "%s%-*.*s",
					(*label_rows)[row],
					col_data[col].max_width,
					col_data[col].max_width,
					start);
			}
		}
	}

	/*
	 * Put together the row strings if there any
	 */
	if (num_rows <= 0)
		return;
	(*entries) = (char **) xcalloc(num_rows * (sizeof (char *)));
	for (row = 0; row < num_rows; row++) {
		(*entries)[row] = xmalloc(row_len + 1);
		(void) strcpy((*entries)[row], "");

		for (col = 0; col < num_cols; col++) {
			if (col == 0) {
				(void) sprintf((*entries)[row], "%-*.*s",
					col_data[col].max_width,
					col_data[col].max_width,
					row_data[row][col] ?
					row_data[row][col] : " ");
			} else {
				(void) sprintf((*entries)[row], "%s%*.*s%*.*s",
					(*entries)[row],
					space_len,
					space_len,
					SPACES_STR,
					col_data[col].max_width,
					col_data[col].max_width,
					row_data[row][col] ?
					row_data[row][col] : " ");
			}
		}
	}

	if (entry_row_len)
		*entry_row_len = row_len;

}
