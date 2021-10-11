#ifndef lint
#pragma ident "@(#)tty_wins.c 1.8 96/07/29 SMI"
#endif

/*
 * Copyright (c) 1993-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	tty_wins.c
 * Group:	libspmitty
 * Description:
 *	Contains some common window creation/management routines.
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

/* default number of lines used for wheader and wfooter */
int HeaderLines = 2;
int FooterLines = 2;

char *Sel = "[X]";
char *Unsel = "[ ]";
char *Clear = "   ";

static char *wfooter_label_create(int index, char *str);

/*
 * displays options passed in.  one option per line.
 *
 * side-effect, records row/col in which each option is displayed back into
 * each option struct
 */

int
show_choices(WINDOW * w,	/* window to display menu in	*/
    int n,			/* n items to be displayed	*/
    int npp,			/* n items possible per page	*/
    int row,			/* starting row of menu		*/
    int col,			/* starting col for menu items	*/
    ChoiceItem * opts,		/* array of menu options	*/
    int first)
{				/* first item for this menu page */
	int i;			/* counts items displayed	*/
	int j;			/* current item's index		*/
	int x;			/* current row position		*/

	/* clear menu display portion of window */
	for (x = row; x < row + npp; x++) {

		(void) wmove(w, x, col);
		(void) wclrtoeol(w);

	}

	/* show current page of menu options */
	for (i = 0, x = row, j = first;
	    (i < npp) && (j < n);
	    i++, x++, j++) {

		(void) mvwprintw(w, x, col - 4, "%s %-.*s",
		    opts[j].sel == 1 ? Sel : (opts[j].sel == 0) ? Unsel : Clear,
		    COLS - col - 2, opts[j].label);

		opts[j].loc.r = x;
		opts[j].loc.c = col;

	}

	/* return number of items actually displayed */
	return (i);
}

void
scroll_prompts(WINDOW * w,	/* window to draw in	*/
    int top,			/* starting row	*/
    int col,			/* col for scrollbar	*/
    int scr,			/* scrolled? index of item in top row	*/
    int max,			/* total number of items	*/
    int npp)
{				/* n items possible per page	*/
	int last = top + npp - 1;

	/* draw `cable' and anchors */
	if (max > npp) {
		(void) wmove(w, top + 1, col);
		(void) wvline(w, ACS_VLINE, last - top - 1);

		if (scr) {
			(void) mvwaddch(w, top, col, ACS_UARROW /* '^' */);
		} else {
			(void) mvwaddch(w, top, col, '-');
		}

		if ((scr + npp + 1) <= max) {
			(void) mvwaddch(w, last, col, ACS_DARROW /* 'v' */);
		} else {
			(void) mvwaddch(w, last, col, '-');
		}
	}
}

int
wget_field(WINDOW * w,
    int type,
    int x,
    int y,
    int len,
    int max,
    char *val,
    u_long fkeys)
{
	char *cp;
	char *cp1;
	int ch;
	int scrolled = 0;
	char buf[BUFSIZ];
	char erase_str[BUFSIZ];
	char disp_fmt[16];
	int first = 1;

	(void) sprintf(erase_str, "%*.*s", len, len, " ");	/* clr field */

	if (type == LSTRING)
		(void) strcpy(disp_fmt, "%s%-s");
	else if (type == RSTRING || type == NUMERIC)
		(void) sprintf(disp_fmt, "%%s%%%d.%ds", len, len);
	else
		(void) strcpy(disp_fmt, "%s%s");

	(void) memset(buf, '\0', BUFSIZ);

	(void) strcpy(buf, val);

	if ((int) strlen(buf) > len) {
		scrolled = 1;
		cp1 = buf + (strlen(buf) - len);	/* cp1 == len chars */
	} else
		cp1 = buf;

	cp = buf + strlen(buf);


	for (;;) {

		if (scrolled == 1) {
			/*
			 * if right justified, or scrolled, put cursor on
			 * top of last character
			 */
			(void) mvwprintw(w, x, y, "%*.*s", len, len, " ");
			wcolor_on(w, CURSOR);
			(void) mvwprintw(w, x, y, "%s%-*.*s", "<", len - 1,
			    len - 1, cp1);
			wcolor_off(w, CURSOR);

			/*
			 * un-hilite last character since that is where the
			 * reverse video cursor goes...
			 */
			if (has_colors() == 0)
				(void) mvwprintw(w, x, y + (len - 1), "%s",
				    *cp1 ? &cp1[strlen(cp1) - 1] : " ");

			(void) wmove(w, x, y + (len - 1));
			(void) setsyx(x, y + (len - 1));

		} else if ((strlen(buf) == len) || (type == RSTRING) ||
		    (type == NUMERIC)) {
			/*
			 * if right justified, or scrolled, put cursor on
			 * top of last character
			 */
			wcolor_on(w, CURSOR);
			(void) mvwprintw(w, x, y, "%*.*s", len, len, cp1);
			wcolor_off(w, CURSOR);

			/*
			 * un-hilite last character since that is where the
			 * reverse video cursor goes...
			 */
			if (has_colors() == 0)
				(void) mvwprintw(w, x, y + (len - 1), "%s",
				    *cp1 ? &cp1[strlen(cp1) - 1] : " ");

			(void) wmove(w, x, y + (len - 1));
			(void) setsyx(x, y + (len - 1));

		} else {

			wcolor_on(w, CURSOR);
			(void) mvwprintw(w, x, y, "%*.*s", len, len, " ");
			(void) mvwprintw(w, x, y, "%-s", cp1);
			wcolor_off(w, CURSOR);
		}

		ch = wzgetch(w, fkeys);

		if (fwd_cmd(ch) || bkw_cmd(ch) || sel_cmd(ch) || is_help(ch) ||
		    is_continue(ch) || is_ok(ch) || is_cancel(ch) ||
		    is_exit(ch) || is_fkey(ch)) {

			flush_input();
			break;
		}

		if (ch == ESCAPE) {
			continue;
		}

		if (ch == erasech || ch == '\010') {

			if (first == 1)
				first = 0;

			if (cp > buf)
				*(--cp) = '\0';
			else
				*cp = '\0';

			if (scrolled)
				if (--cp1 == buf)
					scrolled = 0;

		} else if (ch == killch) {

			if (first == 1)
				first = 0;

			(void) memset(buf, '\0', BUFSIZ);
			cp = buf;
			scrolled = 0;

		} else if (ch == ' ' || (isalnum(ch) != 0) ||
		    (ispunct(ch) != 0)) {

			if ((type == NUMERIC) &&
			    ((isdigit(ch) == 0) /* && (ch != '.') */)) {
				beep();
				continue;
			}

			/*
			 * implement `wipe-to-type' or `pending-delete' on
			 * the first non-navigational keystroke, clear the
			 * current contents of the field
			 */
			if (first == 1) {
				(void) memset(buf, '\0', BUFSIZ);
				cp = buf;
				scrolled = 0;
				first = 0;

				if (ch == ' ')
					continue;

			} else
				first = 0;

			if ((cp - buf) < len) {

				*(cp++) = (char) ch;

			} else if ((cp - buf) < max) {	/* scroll */

				*(cp++) = (char) ch;
				++cp1;
				scrolled = 1;

			} else if ((cp - buf) == len && len == 1)
				*(cp - 1) = (char) ch;
			else
				beep();

		}
	}

	(void) strcpy(val, buf);

	return (ch);		/* nav cmd or return */
}

void
wheader(WINDOW * w, const char *title)
{

	if (has_colors() == 0) {
		(void) wmove(w, 0, 0);
		(void) whline(w, ACS_HLINE, 1);

		(void) mvwprintw(w, 0, 1, " %-.*s ", COLS - 3, title);
		(void) whline(w, ACS_HLINE, COLS - strlen(title) - 3);
	} else {
		wcolor_on(w, TITLE);
		(void) mvwprintw(w, 0, 0, "  %-*.*s ", COLS - 3, COLS - 3,
		    title);
		wcolor_off(w, TITLE);
	}
}

static char *
wfooter_label_create(int index, char *str) {

	char *lead;
	char keystr[256];

	if (!tty_GetForceAlternates() && tigetstr(_fkeys[index].f_keycap)) {
		lead = _fkeys[index].f_special;
	} else {
		lead = _fkeys[index].f_fallback;
	}
	(void) sprintf(keystr, "%s_%s", lead, str);
	return (xstrdup(keystr));
}

/*
 * use this to temporarily reset the text of certain buttons.
 * This is useful for our msging routines that want to define button
 * labels on the fly.
 */
void
wfooter_func_set(int fkey, char *str) {
	int i;

	i = fkey_index(fkey);
	_fkeys[i].f_func = xstrdup(str);
	_fkeys[i].f_label = wfooter_label_create(i, _fkeys[i].f_func);
}

char *
wfooter_func_get(int fkey) {
	int i;

	i = fkey_index(fkey);
	return (_fkeys[i].f_func);
}


void
wfooter(WINDOW * w, u_long which_keys)
{
	static int doinit = 1;
	char keystr[256];
	int y = LINES - 2;
	int i, len, s;
	int spacelen;
	char spaces[] = "     ";
	int too_long;
	int max_footer_len;

	if (doinit) {
		if (_fkeys_init_func)
			(*_fkeys_init_func)(0);
		doinit = 0;
	}

	/*
	 * first find out how far apart to space the footer entries.
	 * The ideal is 4, but we'll go lower if we have to...
	 * And, if the lowest value is still not enough - use it anyway
	 * and fit in as much as possible.
	 */
	max_footer_len = has_colors() ? COLS : COLS - 1;
	for (spacelen = 4; spacelen >= 1; spacelen--) {
		len = 0;
		too_long = 0;
		for (i = 0; i < _num_fkeys; i++) {
			if ((which_keys & (1 << i)) == 0)
				continue;
			s = strlen(_fkeys[i].f_label) + spacelen;
			if ((len + s) > max_footer_len) {
				too_long = 1;
				break;
			}
			len += s;
		}
		if (!too_long)
			break;
	}
	write_debug(TTY_DEBUG_L1, "y after spacelen loop %d", y);

	if (!spacelen)
		spacelen = 1;
	write_debug(TTY_DEBUG_L1, "y before setting spaces %d", y);
	spaces[spacelen] = '\0';
	write_debug(TTY_DEBUG_L1, "y after setting spaces %d", y);

	write_debug(TTY_DEBUG_L1, "y before strcpy %d", y);
	(void) strcpy(keystr, "");
	write_debug(TTY_DEBUG_L1, "y after strcpy %d", y);
	len = 0;
	for (i = 0; i < _num_fkeys; i++) {
		if ((which_keys & (1 << i)) == 0)
			continue;
		s = strlen(_fkeys[i].f_label) + spacelen;
		if ((len + s) > max_footer_len)
			break;
		(void) sprintf(&keystr[len], "%s%s", spaces, _fkeys[i].f_label);
		len += s;
	}

	(void) wmove(w, y++, 0);

	if (has_colors() == 0)
		(void) whline(w, ACS_HLINE, COLS);
	else
		(void) wclrtoeol(w);

	wcolor_on(w, FOOTER);
	if (has_colors() == 0)
		(void) mvwprintw(w, y, 1, "%-*.*s", COLS - 1, COLS - 1, keystr);
	else
		(void) mvwprintw(w, y, 0, "%-*.*s", COLS, COLS, keystr);
	(void) wredrawln(w, y, 1);
	wcolor_off(w, FOOTER);

	(void) wnoutrefresh(w);

}

/*
 * uses reverse-reverse-video to hide block cursor on mono screens
 *
 * by default, hides the cursor at (LINES - 1, 0)
 */
void
wcursor_hide(WINDOW * w)
{

	if (has_colors() == 0) {
		wcolor_on(w, CURSOR);
		(void) mvwprintw(w, LINES - 1, 0, " ");
		wcolor_off(w, CURSOR);
	}
	(void) wmove(w, LINES - 1, 0);
	(void) setsyx(LINES - 1, 0);

}

/*
 * displays a simple notice (or warning).  Expects to be dismissed when the
 * user is done reading.
 *
 * input:
 *	win:	parent window to refresh when notice is dismissed.
 * 	fkey:	function keys to display as 'dismiss' key
 *	title:	char pointer to notice title
 * 	text:	char pointer to notice body
 *
 */
int
simple_notice(WINDOW * parent, int fkey, char *title, char *text)
{
	UI_MsgStruct *msg_info;
	tty_MsgAdditionalInfo *tty_info;
	WINDOW *old_parent;

	tty_info = UI_MsgGenericInfoGet();
	if (!tty_info) {
		return (0);
	}

	old_parent = tty_info->parent;
	tty_info->parent = parent;

	msg_info = UI_MsgStructInit();
	msg_info->msg_type = UI_MSGTYPE_INFORMATION;
	msg_info->title = title;
	msg_info->msg = text;
	msg_info->btns[UI_MSGBUTTON_CANCEL].button_text = NULL;
	msg_info->btns[UI_MSGBUTTON_HELP].button_text = NULL;

	/* invoke the message */
	UI_MsgFunction(msg_info);

	/* cleanup */
	UI_MsgStructFree(msg_info);
	tty_info->parent = old_parent;

	/* always return 0 since historically that's what it did... */
	return (0);
}

/*
 * displays a binary respronse notice (or warning).  Expects to return
 * either `yes' or `no'.
 *
 * input should be mapped onto standard YES and NO function keys.
 * (F2/F3 or F4 respectively)
 *
 * input:
 *	win:	parent window to refresh when notice is dismissed.
 * 	fkeys:	function keys to display
 * 	yes:	what to return for `yes'
 *	no:	what to return for `no'
 *	title:	char pointer to notice title
 * 	text:	char pointer to notice body
 *
 */
int
yes_no_notice(WINDOW * parent, int fkeys, int yes, int no, char *title,
    char *text)
{
	WINDOW *win;

	int retcode;
	int ch;
	int row;
	int nlines;

	win = newwin(LINES, COLS, 0, 0);
	wcolor_set_bkgd(win, BODY);
	(void) keypad(win, 1);

	(void) werase(win);
	(void) wclear(win);

	/* show title */
	wheader(win, title);

	nlines = count_lines(text, COLS - (2 * INDENT1));
	row = (LINES - nlines - 2) / 2;

	(void) wword_wrap(win, row, INDENT1, COLS - (2 * INDENT1), text);

	wfooter(win, fkeys);
	wcursor_hide(win);

	for (;;) {

		ch = wzgetch(win, fkeys);

		if (is_ok(ch)) {
			retcode = yes;
			break;
		} else if (is_exit(ch) &&
		    (fkeys & F_HALT || fkeys & F_CANCEL || fkeys & F_EXIT)) {

			retcode = no;
			break;
		} else if (_fkey_notice_check_func &&
			(*_fkey_notice_check_func)(fkeys, ch)) {

			retcode = no;
			break;
		} else
			beep();
	}

	(void) delwin(win);

	if (parent != (WINDOW *) NULL) {
		(void) clearok(curscr, TRUE);
		(void) touchwin(parent);
		(void) wnoutrefresh(parent);
		(void) clearok(curscr, FALSE);
	}
	return (retcode);
}

/*
 * clears status message line
 */
void
wclear_status_msg(WINDOW * w)
{
	int x, y;

	(void) getyx(w, x, y);

	(void) wmove(w, LINES - FooterLines - 1, 0);
	(void) wclrtoeol(w);

	(void) wmove(w, x, y);
	(void) wrefresh(w);

}

/*
 * formats and displays a status message
 */
void
wstatus_msg(WINDOW * w, char *format, ...)
{
	va_list args;
	int x, y;

	(void) getyx(w, x, y);

	(void) wmove(w, LINES - FooterLines - 1, 0);
	(void) wclrtoeol(w);

	va_start(args, format);

	(void) wmove(w, LINES - FooterLines - 1, 2);
	wcolor_on(w, CURSOR_INV);
	(void) vwprintw(w, format, args);
	wcolor_off(w, CURSOR_INV);

	va_end(args);

	(void) wmove(w, x, y);
	(void) wrefresh(w);

}

/*
 * split `str' into lines of < `w' characters print each line starting at
 * `r', `c' return last row
 */
int
wword_wrap(WINDOW * win, const int r, const int c, const int w, const char *str)
{
	register char *cp, *cp1;
	char *start;
	int row;
	int n;

	row = r;
	/*
	 * 09/28/94 MMT added char * cast to str to resolve compiler warning
	 * argument incompatible with prototype
	 */
	cp = start = xstrdup((char *)str);

	while (cp && *cp) {

		n = 0;
		cp1 = cp;

		while (cp && *cp && (*cp != '\n') && (n < w)) {
			++cp;
			++n;
		}

		/* clear to end of line before we print on this line */
		if (curses_on) {
			(void) wmove(win, row, c);
			(void) wclrtoeol(win);
		}

		/* print the next line */
		if (*cp == '\0')	/* last line */
			if (curses_on) {
				(void) mvwprintw(win, row++, c, "%-.*s", w, cp1);
			} else {
				(void) printf("\n%-.*s", w, cp1);
			}
		else if (*cp == '\n') {	/* print up to newline */
			*cp = '\0';
			if (curses_on) {
				(void) mvwprintw(win, row++, c, "%-.*s", w, cp1);
			} else {
				(void) printf("\n%-.*s", w, cp1);
			}
			++cp;
		} else if (n == w) {

			/*
			 * no newline, find a space, break line there
			 */
			while ((*cp != ' ') && (cp != cp1))
				--cp;

			if (cp == cp1) {
				/* no space, just chop at (cp + w)'th char */

				*(cp + w - 1) = '\0';
				if (curses_on) {
					(void) mvwprintw(win, row++, c,
						"%-.*s", w, cp1);
				} else {
					(void) printf("\n%-.*s", w, cp1);
				}
				cp += w;

			} else if (*cp == ' ') {

				*cp = '\0';
				if (curses_on) {
					(void) mvwprintw(win, row++, c,
						"%-.*s", w, cp1);
				} else {
					(void) printf("\n%-.*s", w, cp1);
				}
				++cp;

			}
		}
	}

	if (cp && *cp) {
		/* clear to end of line before we print on this line */
		if (curses_on) {
			(void) wmove(win, row, c);
			(void) wclrtoeol(win);

			(void) mvwprintw(win, row++, c, "%-.*s", w, cp);
		} else {
			(void) printf("\n%-.*s", w, cp);
		}
	}

	if (start)
		free(start);

	return (row);

}

/*
 *
 */
int
verify_field_input(EditField * f, FieldType type, int low, int hi)
{

	char *cp;
	int val;

	if (type == NUMERIC) {
		for (cp = f->value; cp && *cp; ++cp)
			if (isdigit(*cp) == 0)
				return (FAILURE);

		val = atoi(f->value);

		if (low && val < low)
			return (FAILURE);

		if (hi && val > hi)
			return (FAILURE);

		return (SUCCESS);

	} else if (type == LSTRING || type == RSTRING) {
		return (SUCCESS);
	}
	return (SUCCESS);
}

/*
 * mvwgets -- move to specified spot in specified window and retrieve
 *	a line of input.  Like gets, mvwgets throws away the trailing
 *	newline character.  The cursor is placed following the initial
 *	value in 'buf' (buf must contain a null-terminated string).
 *	The user's erase character backspaces and the line kill character
 *	re-starts the input line.  The Escape key aborts the routine
 *	leaving the original input buffer unchanged and restoring the
 *	value on the screen to the initial contents of the buffer.
 *	The returned string is guaranteed to be null-terminated (and
 *	thus "buf" should be 1 larger than "len").
 */
/*ARGSUSED*/
int
mvwgets(WINDOW		*w,
	int		starty,
	int		startx,
	int		ncols,		/* width of input area */
	Callback_proc	*help_proc,
	void		*help_data,
	char		*buf,
	int		len,		/* NB: should be sizeof (buf) - 1 */
	int		type_to_wipe,
	u_long		keys)
{
	char	scratch[1024];	/* user input buffer */
	char	*bp;		/* buffer pointer */
	char	*sp;		/* [scrolling] buffer pointer */
	int	c;		/* current input character */
	int	count;		/* current character count */
	int	curx;		/* cursor x coordinate */
	int	done;

	if (len > sizeof (scratch))
		len = sizeof (scratch) - 1;

	if (buf != (char *)0) {
		(void) strncpy(scratch, buf, len);
		scratch[len] = '\0';
	} else
		scratch[0] = '\0';

	if (type_to_wipe) {
		bp = sp = scratch;
		curx = startx;
	} else {
		bp = &scratch[strlen(scratch)];
		if ((bp - scratch) > ncols) {
			sp = (bp - ncols) + 1;	/* one extra for '<' */
			curx = startx + strlen(sp) + 1;
		} else {
			sp = scratch;
			curx = startx + strlen(sp);
		}
	}
	count = len - (bp - scratch);

	done = 0;

	while (!done) {
		/*
		 * Output section
		 */
		wcolor_on(w, CURSOR);
		if (sp != scratch)
			(void) mvwprintw(w, starty, startx, "<%-*.*s",
				ncols - 1, ncols - 1, sp);
		else
			(void) mvwprintw(w, starty, startx, "%-*.*s",
				ncols, ncols, sp);
		wcolor_off(w, CURSOR);
		(void) wclrtoeol(w);
		(void) wmove(w, starty, curx);
		(void) wrefresh(w);

		c = wzgetch(w, keys);

		if (c == ERR) {
			done = 1;
		} else if (c == erasechar() || c == CTRL_H) {
			if (bp != scratch) {
				if (sp > scratch + 2)
					--sp;
				else {
					if (sp == scratch)
						--curx;
					else
						sp = scratch;
				}
				*--bp = '\0';
				++count;
			} else if (type_to_wipe && bp[0]) {
				*bp = '\0';
				curx = startx;
				count = len;
			} else
				beep();
		} else if (c == killchar()) {
			if (bp != scratch) {
				bp = sp = scratch;
				*bp = '\0';
				curx = startx;
				count = len;
			} else if (type_to_wipe && bp[0]) {
				*bp = '\0';
				curx = startx;
				count = len;
			} else
				beep();
		} else if ((keys & F_HELP) && is_help(c)) {
			if (help_proc != (Callback_proc *)0)
				(void) (*help_proc)(help_data, (void *)0);
			else
				beep();
#ifdef notdef
		} else if (is_reset(c)) {
			(void) strncpy(scratch, buf, len);
			scratch[len] = '\0';
			bp = sp = scratch;
			count = len;
			curx = startx;	/* type to wipe */
#endif
		} else if (((keys & F_CONTINUE) && is_continue(c)) ||
		    fwd_cmd(c) || bkw_cmd(c) || sel_cmd(c)) {
			if (buf != (char *)0) {
				(void) strncpy(buf, scratch, len);
				buf[len] = '\0';
			}
			done = 1;
		} else if ((keys & F_CANCEL) && is_cancel(c)) {
			done = 1;
		} else if (_fkey_mvwgets_check_func &&
		    (*_fkey_mvwgets_check_func)(keys, c)) {
			done = 1;
		} else if (!is_escape(c) && !nav_cmd(c) && buf != (char *)0) {
			if (--count < 0) {
				count = 0;
				beep();
			} else if (isspace(c) && bp == scratch) {
				beep();
			} else {
				*bp++ = (char)c;
				*bp = '\0';
				if ((curx + 1) > startx + ncols) {
					if (sp == scratch)
						sp = scratch + 2;
					else
						sp++;
				} else
					curx++;	/* advance cursor */
			}
		} else if (!esc_cmd(c))
			beep();
	}
	if (sp != scratch)
		(void) mvwprintw(w, starty, startx, "<%-*.*s",
			ncols - 1, ncols - 1, sp);
	else
		(void) mvwprintw(w, starty, startx, "%-*.*s",
			ncols, ncols, sp);
	return (c);
}

void
werror(WINDOW *w, int row, int col, int width, char *fmt)
{
	(void) wclear(w);
	(void) wrefresh(w);

	if (fmt != (char *)0) {
		row = wword_wrap(w, row, col, width, fmt);
		row += 2;
	} else
		row = 0;

	(void) wword_wrap(w, row, col, width, TTY_ERROR_RETURN_TO_CONT);

	for (;;) {
		if (sel_cmd(getch()))
			break;
		else {
			beep();
			(void) wrefresh(w);
		}
	}

	(void) wclear(w);
	(void) wrefresh(w);
}
