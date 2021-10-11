#ifndef lint
#pragma ident "@(#)tty_color.c 1.1 96/03/07 SMI"
#endif

/*
 * Copyright (c) 1993-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	tty_color.c
 * Group:	libspmitty
 * Description:
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "spmitty_api.h"
#include "tty_utils.h"

static void wcolor_init_pairs(void);
static void wcolor_init_colors(void);

/*
 * color pairs setup:
 * NOTE: DO NOT use more than 8 color pairs if you want color to work
 *       in a CDE dtterm.
 *
 * The original color pair code was copied from kdm config and
 * assumed the availability of 64 color pairs (which was indeed the case
 * on all the intel and PPC boxes we had seen so far). The original code
 * is inside the MANY_COLOR_PAIRS ifdef's.
 *
 * However, CDE ddterms only provide 8 color pairs (as defined by curses
 * COLOR_PAIRS) and the original code was using the curses COLOR_PAIR
 * macro with invalid indices.
 *
 * Since, the library and the install apps only use 6 color pairs, the code
 * now only defines those colors to alleviate color problems in dtterms.
 */

#ifdef MANY_COLOR_PAIRS
/*
 * stuff for dealing with colors...
 *
 * various color pair possibilities... these are indexes used by
 * the routine which sets the `standout' attribute
 *
 * (arranged so that color-pair number = (FORE * 8) + BACK)
 */

#define	BLACK_BLUE 			1
#define	BLACK_GREEN			2
#define	BLACK_CYAN 			3
#define	BLACK_RED			4
#define	BLACK_MAGENTA			5
#define	BLACK_YELLOW			6
#define	BLACK_WHITE			7

#define	BLUE_BLACK 			8
#define	BLUE_GREEN 			10
#define	BLUE_CYAN			11
#define	BLUE_RED			12
#define	BLUE_MAGENTA			13
#define	BLUE_YELLOW			14
#define	BLUE_WHITE 			15

#define	GREEN_BLACK			16
#define	GREEN_BLUE 			17
#define	GREEN_CYAN 			19
#define	GREEN_RED			20
#define	GREEN_MAGENTA			21
#define	GREEN_YELLOW			22
#define	GREEN_WHITE			23

#define	CYAN_BLACK 			24
#define	CYAN_BLUE			25
#define	CYAN_GREEN 			26
#define	CYAN_RED			28
#define	CYAN_MAGENTA			29
#define	CYAN_YELLOW			30
#define	CYAN_WHITE 			31

#define	RED_BLACK			32
#define	RED_BLUE			33
#define	RED_GREEN			34
#define	RED_CYAN			35
#define	RED_MAGENTA			37
#define	RED_YELLOW 			38
#define	RED_WHITE			39

#define	MAGENTA_BLACK			40
#define	MAGENTA_BLUE			41
#define	MAGENTA_GREEN			42
#define	MAGENTA_CYAN			43
#define	MAGENTA_RED			44
#define	MAGENTA_YELLOW			46
#define	MAGENTA_WHITE			47

#define	YELLOW_BLACK			48
#define	YELLOW_BLUE			49
#define	YELLOW_GREEN			50
#define	YELLOW_CYAN			51
#define	YELLOW_RED 			52
#define	YELLOW_MAGENTA			53
#define	YELLOW_WHITE			55

#define	WHITE_BLACK			56
#define	WHITE_BLUE 			57
#define	WHITE_GREEN			58
#define	WHITE_CYAN 			59
#define	WHITE_RED			60
#define	WHITE_MAGENTA			61
#define	WHITE_YELLOW			62

#else
#define	BLACK_GREEN			1
#define	BLACK_CYAN			2
#define	BLACK_WHITE			3
#define	WHITE_BLUE			4
#define	WHITE_RED			5
#define	WHITE_BLACK			6
#endif

static int std_colors[7];

/*
 * Function: wcolor_init
 * Description:
 *	initializes standard color pairs that the app will use.
 * Scope:       INTERNAL
 * Parameters:  none
 * Return:	none
 * Globals:	none
 * Notes:
 */
void
wcolor_init(void)
{
	wcolor_init_pairs();
	wcolor_init_colors();
}

/*
 * Function: wcolor_on
 * Description:
 * turns color highlight-attribute on in window `w'... if on a color
 * capable system, for simplicity, limit the color attributes to one of four
 * fg/bg pairs indexed by HiLite enum: { TITLE, BODY, FOOTER, CURSOR }
 *
 * on a 1bit display (`monochrome') map any color `on' into reverse-video, and
 * any color `off' to standard video.
 *
 * Scope:       PUBLIC
 * Parameters:
 *	WINDOW *w - RO - curses screen to output to
 *	HiLite i - RO - fg/bg pair to turn on
 *
 * Return:	none
 * Globals:	none
 * Notes:
 */
void
wcolor_on(WINDOW *w, HiLite_t i)
{
	if (has_colors() != 0)
		(void) wattron(w, COLOR_PAIR(std_colors[i]));
	else if (i == CURSOR || i == CURSOR_INV)
		(void) wstandout(w);
}

/*
 * Function: wcolor_off
 * Description:
 * turns color highlight-attribute off in window `w'... if on a color
 * capable system, for simplicity, limit the color attributes to one of four
 * fg/bg pairs indexed by HiLite enum: { TITLE, BODY, FOOTER, CURSOR }
 *
 * on a 1bit display (`monochrome') map any color `on' into reverse-video, and
 * any color `off' to standard video.
 *
 * Scope:       PUBLIC
 * Parameters:
 *	WINDOW *w - RO - curses screen to output to
 *	HiLite i - RO - fg/bg pair to turn off
 *
 * Return:	none
 * Globals:	none
 * Notes:
 */
void
wcolor_off(WINDOW *w, HiLite_t i)
{
	if (has_colors() != 0)
		(void) wattroff(w, COLOR_PAIR(std_colors[i]));
	else
		(void) wstandend(w);
}

/*
 * Function: wcolor_set_bkgd
 * Description:
 *	set foreground/background of window `w'
 *
 * Scope:       PUBLIC
 * Parameters:
 *	WINDOW *w - RO - curses screen to output to
 *	HiLite i - RO - fg/bg pair to set background to
 *
 * Return:	none
 * Globals:	none
 * Notes:
 */
void
wcolor_set_bkgd(WINDOW *w, HiLite_t i)
{
	if (has_colors() != 0) {
		(void) wbkgd(w, COLOR_PAIR(std_colors[i]) | ' ');
		(void) wbkgdset(w, COLOR_PAIR(std_colors[i]) | ' ');
	}
}


/*
 * Function: wcolor_init_colors
 * Description:
 * initialize default color pairs for different items.
 *
 * color pairings are approximately what the old SSS CUI interfaces used.
 *
 * Scope:       PRIVATE
 * Parameters: none
 * Return:	none
 * Globals:	none
 * Notes:
 */
static void
wcolor_init_colors(void)
{
	std_colors[TITLE] = WHITE_RED;
	std_colors[BODY] = WHITE_BLUE;
	std_colors[FOOTER] = BLACK_GREEN;
	std_colors[CURSOR] = BLACK_CYAN;
	std_colors[CURSOR_INV] = BLACK_WHITE;
	std_colors[NORMAL] = WHITE_BLACK;

	wcolor_set_bkgd(stdscr, BODY);
}

/*
 * Function: wfocus_on
 * Description:
 * hilight (focus) on a string.
 *
 * hilighting is based on the capabilities of the terminal. if there's color,
 * just draw the string in `Focus' color.
 *
 * If there is no color, use a trick to avoid having the cursor mess up the
 * focus.  Leave the first character of the field unhighlighted because
 * that's where the reverse video cursor is going to go.
 *
 * Scope:       PUBLIC
 * Parameters:
 *	WINDOW *w - RO
 *	r - RO - row
 *	c - RO - column
 *	s - RO - string to focus on
 * Return:	none
 * Globals:	none
 * Notes:
 */
void
wfocus_on(WINDOW *w, int r, int c, char *s)
{
	if (has_colors() != 0) {
		wcolor_on(w, CURSOR);
		(void) mvwprintw(w, r, c, s);
		wcolor_off(w, CURSOR);
		(void) wmove(w, r, c);

	} else {
		(void) mvwprintw(w, r, c, s);
		wcolor_on(w, CURSOR);
		(void) mvwprintw(w, r, c + 1, &s[1]);
		wcolor_off(w, CURSOR);
		(void) wmove(w, r, c);
	}

	(void) setsyx(r, c);
}

void
wfocus_off(WINDOW *w, int r, int c, char *s)
{
	(void) mvwprintw(w, r, c, s);
}

/*
 * Function: wcolor_init_pairs
 * Description:
 * The init_pair() routine changes the definition of  a  color-pair.   It
 * takes  three arguments: the number of the color-pair to be changed, the
 * foreground  color  number,  and  the background  color  number.   The
 * value of the first argument must be between 1 and COLOR_PAIRS - 1.   The
 * value  of  the second and third arguments must be between 0 and COLORS.
 * If the color-pair was previously  initialized,  the  screen  is refreshed
 * and all occurrences of that color-pair is changed to the new definition.
 *
 * See color pairs note above: DO NOT define > 8 color pairs if you want
 * to work inside a dtterm.
 *
 * Scope:       PRIVATE
 * Parameters: none
 * Return:	none
 * Globals:	none
 * Notes:
 */
static void
wcolor_init_pairs(void)
{
#ifdef MANY_COLOR_PAIRS
	init_pair(BLACK_BLUE, COLOR_BLACK, COLOR_BLUE);
	init_pair(BLACK_GREEN, COLOR_BLACK, COLOR_GREEN);
	init_pair(BLACK_CYAN, COLOR_BLACK, COLOR_CYAN);
	init_pair(BLACK_RED, COLOR_BLACK, COLOR_RED);
	init_pair(BLACK_MAGENTA, COLOR_BLACK, COLOR_MAGENTA);
	init_pair(BLACK_YELLOW, COLOR_BLACK, COLOR_YELLOW);
	init_pair(BLACK_WHITE, COLOR_BLACK, COLOR_WHITE);

	init_pair(BLUE_BLACK, COLOR_BLUE, COLOR_BLACK);
	init_pair(BLUE_GREEN, COLOR_BLUE, COLOR_GREEN);
	init_pair(BLUE_CYAN, COLOR_BLUE, COLOR_CYAN);
	init_pair(BLUE_RED, COLOR_BLUE, COLOR_RED);
	init_pair(BLUE_MAGENTA, COLOR_BLUE, COLOR_MAGENTA);
	init_pair(BLUE_YELLOW, COLOR_BLUE, COLOR_YELLOW);
	init_pair(BLUE_WHITE, COLOR_BLUE, COLOR_WHITE);

	init_pair(GREEN_BLACK, COLOR_GREEN, COLOR_BLACK);
	init_pair(GREEN_BLUE, COLOR_GREEN, COLOR_BLUE);
	init_pair(GREEN_CYAN, COLOR_GREEN, COLOR_CYAN);
	init_pair(GREEN_RED, COLOR_GREEN, COLOR_RED);
	init_pair(GREEN_MAGENTA, COLOR_GREEN, COLOR_MAGENTA);
	init_pair(GREEN_YELLOW, COLOR_GREEN, COLOR_YELLOW);
	init_pair(GREEN_WHITE, COLOR_GREEN, COLOR_WHITE);

	init_pair(CYAN_BLACK, COLOR_CYAN, COLOR_BLACK);
	init_pair(CYAN_BLUE, COLOR_CYAN, COLOR_BLUE);
	init_pair(CYAN_GREEN, COLOR_CYAN, COLOR_GREEN);
	init_pair(CYAN_RED, COLOR_CYAN, COLOR_RED);
	init_pair(CYAN_MAGENTA, COLOR_CYAN, COLOR_MAGENTA);
	init_pair(CYAN_YELLOW, COLOR_CYAN, COLOR_YELLOW);
	init_pair(CYAN_WHITE, COLOR_CYAN, COLOR_WHITE);

	init_pair(RED_BLACK, COLOR_RED, COLOR_BLACK);
	init_pair(RED_BLUE, COLOR_RED, COLOR_BLUE);
	init_pair(RED_GREEN, COLOR_RED, COLOR_GREEN);
	init_pair(RED_CYAN, COLOR_RED, COLOR_CYAN);
	init_pair(RED_MAGENTA, COLOR_RED, COLOR_MAGENTA);
	init_pair(RED_YELLOW, COLOR_RED, COLOR_YELLOW);
	init_pair(RED_WHITE, COLOR_RED, COLOR_WHITE);

	init_pair(MAGENTA_BLACK, COLOR_MAGENTA, COLOR_BLACK);
	init_pair(MAGENTA_BLUE, COLOR_MAGENTA, COLOR_BLUE);
	init_pair(MAGENTA_GREEN, COLOR_MAGENTA, COLOR_GREEN);
	init_pair(MAGENTA_CYAN, COLOR_MAGENTA, COLOR_CYAN);
	init_pair(MAGENTA_RED, COLOR_MAGENTA, COLOR_RED);
	init_pair(MAGENTA_YELLOW, COLOR_MAGENTA, COLOR_YELLOW);
	init_pair(MAGENTA_WHITE, COLOR_MAGENTA, COLOR_WHITE);

	init_pair(YELLOW_BLACK, COLOR_YELLOW, COLOR_BLACK);
	init_pair(YELLOW_BLUE, COLOR_YELLOW, COLOR_BLUE);
	init_pair(YELLOW_GREEN, COLOR_YELLOW, COLOR_GREEN);
	init_pair(YELLOW_CYAN, COLOR_YELLOW, COLOR_CYAN);
	init_pair(YELLOW_RED, COLOR_YELLOW, COLOR_RED);
	init_pair(YELLOW_MAGENTA, COLOR_YELLOW, COLOR_MAGENTA);
	init_pair(YELLOW_WHITE, COLOR_YELLOW, COLOR_WHITE);

	init_pair(WHITE_BLACK, COLOR_WHITE, COLOR_BLACK);
	init_pair(WHITE_BLUE, COLOR_WHITE, COLOR_BLUE);
	init_pair(WHITE_GREEN, COLOR_WHITE, COLOR_GREEN);
	init_pair(WHITE_CYAN, COLOR_WHITE, COLOR_CYAN);
	init_pair(WHITE_RED, COLOR_WHITE, COLOR_RED);
	init_pair(WHITE_MAGENTA, COLOR_WHITE, COLOR_MAGENTA);
	init_pair(WHITE_YELLOW, COLOR_WHITE, COLOR_YELLOW);

#else
	init_pair(BLACK_GREEN, COLOR_BLACK, COLOR_GREEN);
	init_pair(BLACK_CYAN, COLOR_BLACK, COLOR_CYAN);
	init_pair(BLACK_WHITE, COLOR_BLACK, COLOR_WHITE);
	init_pair(WHITE_BLUE, COLOR_WHITE, COLOR_BLUE);
	init_pair(WHITE_RED, COLOR_WHITE, COLOR_RED);
	init_pair(WHITE_BLACK, COLOR_WHITE, COLOR_BLACK);
#endif
}
