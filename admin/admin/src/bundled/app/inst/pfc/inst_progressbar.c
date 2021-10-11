#ifndef lint
#pragma ident "@(#)inst_progressbar.c 1.3 96/06/23 SMI"
#endif

/*
 * Copyright (c) 1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	inst_progressbar.c
 * Group:	ttinstall
 * Description:
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <libintl.h>
#include <signal.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/param.h>
#include <time.h>

#include "pf.h"
#include "inst_progressbar.h"

/*
 * implements a `progress' bar screen looking kind of like:
 *
 * ----- Title ----
 *
 *
 *	Main Label: Detail Label
 *
 *
 *       #######################
 *       |           |           |           |           |           |
 *       0          20          40          60          80         100
 *
 */

/*
 * local functions
 */
static void _ProgressBarUpdateMainLabel(
	pfcProgressBarDisplayData *display_data,
	char *main_label);
static void _ProgressBarUpdateDetailLabel(
	pfcProgressBarDisplayData *display_data,
	char *detail_label);
static void _ProgressBarUpdatePercent(
	pfcProgressBarDisplayData *display_data,
	int percent);

/*
 * static variables
 */
pfcProgressBarDisplayData *_display_data = NULL;
#define	_PROGRESS_BAR_HASH_INDENT 10

/*
 * ******************************************************************
 * 	DSR general progress bar routines
 * ******************************************************************
 */
void
pfcProgressBarCreate(
	UIProgressBarInitData *init_data,
	pfcProgressBarDisplayData **display_data,
	WINDOW *win,
	int scale_info_cnt)
{

	int row;
	int cnt;
	WINDOW *scr;

	/* malloc out the display data structure */
	if (scale_info_cnt <= 0)
		cnt = 1;
	else
		cnt = scale_info_cnt;

	*display_data = (pfcProgressBarDisplayData *)
		xcalloc(sizeof (pfcProgressBarDisplayData));
	(*display_data)->scale_info = (UIProgressBarScaleInfo *)
		xcalloc(sizeof (UIProgressBarScaleInfo) * cnt);

	if (win)
		scr = win;
	else
		scr = stdscr;

	/* clear window */
	touchwin(scr);
	wclear(scr);
	werase(scr);

	/* title and msg */
	wheader(scr, init_data->title);
	row = wword_wrap(scr, HeaderLines, INDENT0, COLS - (2 * INDENT0),
		init_data->main_msg ? init_data->main_msg : "");

	/* initialize display data strucutre */
	(*display_data)->main_label.row = row + 3;
	(*display_data)->main_label.col = _PROGRESS_BAR_HASH_INDENT;
	(*display_data)->space_len = 1;

	(*display_data)->scale.row =
		(*display_data)->main_label.row + 2;
	(*display_data)->scale.min_col = _PROGRESS_BAR_HASH_INDENT;
	(*display_data)->scale.max_col = COLS - _PROGRESS_BAR_HASH_INDENT;
	(*display_data)->scale.width =
		(*display_data)->scale.max_col -
		(*display_data)->scale.min_col;
	(*display_data)->win = scr;

	(*display_data)->scale_info[0].start = 0;
	(*display_data)->scale_info[0].factor = 1;

	/* set up the basic the progress scale */
	(void) mvwprintw(scr,
		(*display_data)->scale.row,
		(*display_data)->scale.min_col,
		"|");
	(void) mvwprintw(scr,
		(*display_data)->scale.row,
		(*display_data)->scale.max_col,
		"|");
	(void) mvwprintw(scr,
		(*display_data)->scale.row + 1,
		(*display_data)->scale.min_col,
		"0");
	(void) mvwprintw(scr,
		(*display_data)->scale.row + 1,
		(*display_data)->scale.max_col - 1,
		"100");

	/* start with some default values */
	pfcProgressBarUpdate((*display_data), TRUE,
		PROGRESSBAR_MAIN_LABEL, init_data->main_label,
		PROGRESSBAR_DETAIL_LABEL, init_data->detail_label,
		PROGRESSBAR_PERCENT, init_data->percent,
		NULL);

	/* refresh the window */
	wcursor_hide(scr);
	(void) wnoutrefresh(scr);
}

void
pfcProgressBarCleanup(pfcProgressBarDisplayData *display_data)
{
/* 	(void) wtimeout(display_data->win, -1); */

	/* clean up */
	free(display_data->scale_info);
	free(display_data);
}

void
pfcProgressBarUpdate(
	pfcProgressBarDisplayData *display_data,
	int pause,
	...)
{
	va_list ap;
	ProgressBarAttr attr;
	int percent;
	char *main_label;
	char *detail_label;
	int main_label_set = FALSE;
	int detail_label_set = FALSE;
	int percent_set = FALSE;

	/* get the attribute values we want to set */
	va_start(ap, pause);
	while ((attr = va_arg(ap, ProgressBarAttr)) != NULL) {

		switch (attr) {
		case PROGRESSBAR_MAIN_LABEL:
			main_label = va_arg(ap, char *);
			main_label_set = TRUE;
			break;
		case PROGRESSBAR_DETAIL_LABEL:
			detail_label = va_arg(ap, char *);
			detail_label_set = TRUE;
			break;
		case PROGRESSBAR_PERCENT:
			percent = va_arg(ap, int);
			percent_set = TRUE;
			break;
		}
	}
	va_end(ap);

	/*
	 * Set the main label.
	 * A set of the main label implies a nulling out of the detal
	 * label (it will get redrawn below if it was requested as well).
	 */
	if (main_label_set) {
		_ProgressBarUpdateMainLabel(display_data, main_label);
	}

	/* set the detail label */
	if (detail_label_set) {
		_ProgressBarUpdateDetailLabel(display_data, detail_label);
	}

	/* set the percent value */
	if (percent_set) {
		_ProgressBarUpdatePercent(display_data, percent);
	}

	wcursor_hide(display_data->win);
	wnoutrefresh(display_data->win);
	doupdate();

	if (pause)
		(void) sleep(APP_PROGRESS_PAUSE_TIME);
}

static void
_ProgressBarUpdateMainLabel(pfcProgressBarDisplayData *display_data,
	char *main_label)
{

	/* totally clear the labels line */
	(void) wmove(display_data->win,
		display_data->main_label.row,
		display_data->main_label.col);
	(void) wclrtoeol(display_data->win);

	/* rewrite the main label */
	if (main_label) {
		(void) mvwprintw(display_data->win,
			display_data->main_label.row,
			display_data->main_label.col,
			"%s", main_label);
	}
	wcursor_hide(display_data->win);

	display_data->main_label.len =
		main_label ? strlen(main_label) : 0;
}

static void
_ProgressBarUpdateDetailLabel(pfcProgressBarDisplayData *display_data,
	char *detail_label)
{
	int detail_col;

	detail_col =
		display_data->main_label.col +
		display_data->main_label.len +
		display_data->space_len;
	/* clear the labels line from the start of the detail label */
	(void) wmove(display_data->win,
		display_data->main_label.row, detail_col);
	(void) wclrtoeol(display_data->win);

	/* rewrite the detail label */
	if (detail_label) {
		(void) mvwprintw(display_data->win,
			display_data->main_label.row,
			detail_col,
			"%s", detail_label);
	}
	wcursor_hide(display_data->win);
}

/*
 *
 * The proportional indicator is updated with the specified percentage.
 * `percent' must be in the range 0-100 inclusive.
 */
static void
_ProgressBarUpdatePercent(pfcProgressBarDisplayData *display_data,
	int percent)
{
	int proportion;

	proportion = (int) ((float) display_data->scale.width *
		((float) percent / 100.0));

	write_debug(CUI_DEBUG_L1,
		"percent = %d, proportion = %d, width = %d",
		percent, proportion,
		display_data->scale.width);

	wcolor_on(display_data->win, CURSOR);
	(void) mvwprintw(display_data->win,
		display_data->scale.row,
		display_data->scale.min_col,
		"%*.*s ",
		proportion, proportion, " ");
	wcolor_off(display_data->win, CURSOR);
	wcursor_hide(display_data->win);
}
