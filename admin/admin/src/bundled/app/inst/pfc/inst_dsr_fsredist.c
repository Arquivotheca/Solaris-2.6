#ifndef lint
#pragma ident "@(#)inst_dsr_fsredist.c 1.19 96/08/29 SMI"
#endif

/*
 * Copyright (c) 1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	inst_dsr_fsredist.c
 * Group:	ttinstall
 * Description:
 */

#include <stdio.h>
#include <limits.h>
#include <locale.h>
#include <libintl.h>
#include <string.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/bitmap.h>

#include "pf.h"
#include "tty_pfc.h"
#include "inst_msgs.h"
#include "v_types.h"
#include "v_misc.h"
#include "v_sw.h"

#define	_DSR_FSREDIST_HELP_TITLE	\
	"Select Auto-layout Constraints Screen"

/* spacing between column entries */
#define	_DSR_FSREDIST_SPACING	3

/* total number of columns in table */
#define	_DSR_FSREDIST_COLUMNS	6

/* column indexes */
#define	_DSR_FSREDIST_FS_COL	0
#define	_DSR_FSREDIST_SLICE_COL	1
#define	_DSR_FSREDIST_FREE_SPACE_COL	2
#define	_DSR_FSREDIST_SPACE_NEEDED_COL	3
#define	_DSR_FSREDIST_CONSTRAINTS_COL	4
#define	_DSR_FSREDIST_MIN_SIZE_COL	5

typedef enum {
	DSR_FSREDIST_EDIT_Slice,
	DSR_FSREDIST_EDIT_Filter,
	DSR_FSREDIST_EDIT_Collapse,
	DSR_FSREDIST_EDIT_Reset,
	DSR_FSREDIST_EDIT_Undefined
} _TEditChoices;

/* module globals */
static int 	_collapse_selection_bits;

static void _dsr_fsredist_do_add_lines(int *add_lines);
static void _dsr_get_slice_row_data(
	TSLEntry *slentry, ttyLabelRowData row_data_entry);
static void _get_fsredist_col_data(ttyLabelColData **col_data);
static void _dsr_fsredist_get_menu_data(
	char ***label_rows,
	int *num_label_rows,
	char ***opts,
	int *num_opts);
static TSLEntry *_dsr_fsredist_index_to_slentry(int opt_index);
static void _do_dsr_fsredist_edit(TSLEntry *slentry);
static _TEditChoices
	_dsr_map_edit_selected_to_edit_type(int selected, TSLEntry *slentry);
static int _dsr_map_edit_edit_type_to_selected(
	_TEditChoices edit_type,
	TSLEntry *slentry);
static int _do_dsr_fsredist_edit_slice(TSLEntry *slentry);
static int _do_dsr_fsredist_filter(void);
static int _do_dsr_fsredist_collapse(void);
static void _do_dsr_fsredist_reset(void);
static int _dsr_fsredist_continue(void);

/*
 * edit slice stuff
 */
#define	_SLICE_IS_EDITABLE(slentry) \
	((slentry) && (slentry)->State != SLCollapse)

static void _get_dsr_edit_slice_opts(
	TSLEntry *slentry, char ***opts, int *num_opts);
static TSLState _dsr_edit_slice_index_to_state(
	TSLEntry *slentry, int index);
static int _dsr_edit_slice_state_to_index(TSLEntry *slentry);
static void _dsr_edit_slice_draw_info(TSLEntry *slentry, int *row);
static int _dsr_edit_slice_size(TSLEntry *slentry, int selected);

/*
 * collapse stuff
 */
/* spacing between column entries */
#define	_DSR_FS_COLLAPSE_SPACING	3

/* total number of columns in table */
#define	_DSR_FS_COLLAPSE_COLUMNS	2

/* column indexes */
#define	_DSR_FS_COLLAPSE_FS_COL	0
#define	_DSR_FS_COLLAPSE_PARENT_FS	1

static void _dsr_collapse_get_menu_data(
	char ***label_rows,
	int *num_label_rows,
	char ***opts,
	int *num_opts,
	int *selected);
static void _get_collapse_col_data(ttyLabelColData **col_data);
static int _collapse_select_cb(void *cb_data, void *item);
static int _collapse_deselect_cb(void *cb_data, void *item);
static TSLEntry *_dsr_collapse_index_to_slentry(int opt_index);
static void _dsr_collapse_reparent_opts(char **old_opts);
static int _dsr_collapse_is_ok(int selected);

/*
 * filter stuff...
 */

/* defines screen presentation order */
#define	_DSR_FILTER_NUM_OPTIONS	6

#define	_DSR_FILTER_ALL		0
#define	_DSR_FILTER_FAILED	1
#define	_DSR_FILTER_VFSTAB	2
#define	_DSR_FILTER_NONVFSTAB	3
#define	_DSR_FILTER_SLICE	4
#define	_DSR_FILTER_FS		5

static int _dsr_filter_type_to_index(TSLFilter filter_type);
static TSLFilter _dsr_filter_index_to_type(int index);
static int _do_dsr_filter_pattern(TSLFilter filter_type);
static int _do_dsr_slice_filter(char *retextstr);

/*
 * *************************************************************************
 *	Code for the main DSR "Select Auto-layout Constraints" screen
 * *************************************************************************
 */

parAction_t
do_dsr_fsredist(void)
{
	int ch;
	int row;
	int wmenu_row;
	u_int fkeys;
	HelpEntry _help;
	int done;
	int really_dirty;
	parAction_t action;
	char **opts;
	int num_opts;
	char **label_rows;
	int num_label_rows;
	int indent;
	int menu_indent;
	int i;
	int height;
	int selected;
	char wmenu_label[100];
	int add_lines;
	char *msg;
	char *tmp_cont_footer;

	/* flush any premature user input */
	flush_input();

	_help.win = stdscr;
	_help.type = HELP_REFER;
	_help.title = _DSR_FSREDIST_HELP_TITLE;

	/*
	 * this list is always displayed alphanumerically sorted
	 */
	SLSort(DsrSLHandle, SLSliceNameAscending);

	/*
	 * Use the F_CONTINUE key for the auto-layout key
	 * and temporarily redo the F_CONTINUE label to be the repeat
	 * auto-layout label.
	 * Do this rather than use the F_REDOAUTO key for repeat auto
	 * because we want this key to be F2, NOT F4.
	 */
	fkeys = F_CONTINUE | F_GOBACK | F_EDIT | F_EXIT | F_HELP;
	tmp_cont_footer = wfooter_func_get(F_CONTINUE);
	wfooter_func_set(F_CONTINUE, LABEL_REPEAT_AUTOLAYOUT);

	msg = (char *) xmalloc(strlen(MSG_DSR_FSREDIST) +
		strlen(MSG_CUI_ADD_FSREDIST) + 1);
	(void) sprintf(msg, MSG_DSR_FSREDIST, MSG_CUI_ADD_FSREDIST);

	done = FALSE;
	really_dirty = TRUE;
	selected = 0;
	while (!done) {
		/*
		 * If the whole window is likely to be munged and we
		 * want to redraw the whole thing then make sure that we
		 * redraw the header, message, and footer too.
		 */
		if (really_dirty) {
			really_dirty = FALSE;

			(void) werase(stdscr);
			(void) wclear(stdscr);
			wheader(stdscr, TITLE_DSR_FSREDIST);

			row = HeaderLines;
			row = wword_wrap(stdscr,
				row, INDENT0, COLS - (2 * INDENT0),
				msg);
			++row;

			_dsr_fsredist_do_add_lines(&add_lines);

			wfooter(stdscr, fkeys);
		}
		wmenu_row = row;

		/* draw and manage the slice list menu */

		indent = INDENT0;
		menu_indent = 9;

		_dsr_fsredist_get_menu_data(
			&label_rows,
			&num_label_rows,
			&opts, &num_opts);

		/*
		 * Draw the label:
		 * Draw all but the last row of the label - pass
		 * the last row of the label into the menu routine so that
		 * the wmenu routine will draw a line "----" between the
		 * label and the menu.  (wmenu does not handle multiple
		 * line labels - which is why we hack it together this
		 * way...)
		 */
		for (i = 0; i < num_label_rows - 1; i++) {
			wmove(stdscr, row, indent);
			(void) wclrtoeol(stdscr);
			mvwprintw(stdscr, row, indent + menu_indent, "%s",
				label_rows[i]);
			wmenu_row++;
		}
		(void) sprintf(wmenu_label, "%*.*s%s",
			5, 5, " ",
			label_rows[num_label_rows - 1]);

		height = LINES - wmenu_row - FooterLines - add_lines;
		ch = wmenu(stdscr,
			wmenu_row,
			indent,
			height,
			COLS - indent - 2,
			(Callback_proc *) show_help, (void *) &_help,
			(Callback_proc *) NULL, (void *) NULL,
			(Callback_proc *) NULL, (void *) NULL,
			wmenu_label,
			opts,
			num_opts,
			(void *) &selected,
			M_RADIO,
			fkeys);

		write_debug(CUI_DEBUG_L1, "selected item = %d, selected");

		if (is_goback(ch)) {
			if (yes_no_notice(stdscr, F_OKEYDOKEY | F_CANCEL,
				TRUE, FALSE,
				TITLE_WARNING,
				MSG_FSREDIST_GOBACK_LOSE_EDITS) == TRUE) {
				/* ok - lose edits */

				/*
				 * try to run autolayout and automatically
				 * relayout stuff
				 */
				wstatus_msg(stdscr, PLEASE_WAIT_STR);
				DsrSLUIResetDefaults(DsrSLHandle,
					FsSpaceInfo, TRUE);
				(void) DsrSLAutoLayout(DsrSLHandle,
					FsSpaceInfo, 1);
				action = parAGoback;
				done = 1;
			}
		} else if (is_exit(ch)) {
			if (confirm_exit(stdscr)) {
				action = parAExit;
				done = TRUE;
			} else {
				really_dirty = TRUE;
			}
		} else if (is_continue(ch)) {
			action = parAContinue;
			if (_dsr_fsredist_continue() == SUCCESS) {
				/* ok to continue */
				done = 1;
				action = parAContinue;
			} else {
				/* can't continue */
				really_dirty = TRUE;
			}
		} else if (is_edit(ch)) {
			_do_dsr_fsredist_edit(
				_dsr_fsredist_index_to_slentry(selected));
			really_dirty = TRUE;
		} else
			beep();

		/* free stuff */
		for (i = 0; i < num_opts; i++) {
			free(opts[i]);
		}
		free(opts);

		for (i = 0; i < num_label_rows; i++) {
			free(label_rows[i]);
		}
		free(label_rows);
	}

	free(msg);
	wfooter_func_set(F_CONTINUE, tmp_cont_footer);
	return (action);
}

static void
_dsr_fsredist_do_add_lines(int *add_lines)
{
	int add_row;
	char buf[100];
	int totals_max;
	ulong add_space_req;
	ulong add_space_alloced;
	int add_col;

	/*
	 * line 1: --------------
	 * line 2:    Total Space Needed: xxx
	 * line 3: Total Space Allocated: xxx
	 */
	*add_lines = 3;

	/*
	 * get up to date totals
	 */
	DsrSLGetSpaceSummary(DsrSLHandle, &add_space_req, &add_space_alloced);

	add_row = LINES - FooterLines - *add_lines;

	wmove(stdscr, add_row, 0);
	whline(stdscr, ACS_HLINE, COLS);
	add_row++;

	/* field width for total space needed & required */
	totals_max = MAX(strlen(LABEL_DSR_FSREDIST_ADDITIONAL_SPACE),
		strlen(LABEL_DSR_FSREDIST_ALLOCATED_SPACE));
	add_col = MINCOLS - 4 - UI_FS_SIZE_DISPLAY_LENGTH - totals_max;

	/* Total Space Needed */
	(void) sprintf(buf, "%*s %*lu",
		totals_max,
		LABEL_DSR_FSREDIST_ADDITIONAL_SPACE,
		UI_FS_SIZE_DISPLAY_LENGTH,
		kb_to_mb(add_space_req));
	mvwprintw(stdscr,
		add_row,
		add_col,
		"%s", buf);

	add_row++;

	/* Total Space Required */
	(void) sprintf(buf, "%*s %*lu",
		totals_max,
		LABEL_DSR_FSREDIST_ALLOCATED_SPACE,
		UI_FS_SIZE_DISPLAY_LENGTH,
		kb_to_mb(add_space_alloced));
	wmove(stdscr, add_row, INDENT0);
	mvwprintw(stdscr,
		add_row,
		add_col,
		"%s", buf);
}

static void
_dsr_fsredist_get_menu_data(
	char ***label_rows,
	int *num_label_rows,
	char ***opts,
	int *num_opts)
{
	ttyLabelColData *col_data = NULL;
	ttyLabelRowData *row_data = NULL;
	ttyLabelRowData row_data_entry = NULL;
	TLink slcurrent;
	TSLEntry *slentry;
	TLLError err;
	DsrSLEntryExtraData *SLEntryextra;
	int num_entries;
	int opt_len;
	int i;

	/*
	 * get the row table data - i.e. the slice data we want
	 * displayed in this row.
	 */
	num_entries = 0;
	LL_WALK(DsrSLHandle, slcurrent, slentry, err) {
		SLEntryextra = slentry->Extra;

		if (!SLEntryextra->in_filter)
			continue;

		/* get the row data for this entry */
		num_entries++;
		row_data = (ttyLabelRowData *) xrealloc(row_data,
			(num_entries * sizeof (ttyLabelRowData)));
		row_data_entry = (ttyLabelRowData)
			xmalloc(_DSR_FSREDIST_COLUMNS *
				sizeof (char *));
		row_data[num_entries - 1] = row_data_entry;

		_dsr_get_slice_row_data(slentry, row_data_entry);
	}

	_get_fsredist_col_data(&col_data);
	tty_GetRowColData(
		row_data,
		num_entries,
		col_data,
		_DSR_FSREDIST_COLUMNS,
		_DSR_FSREDIST_SPACING,
		label_rows,
		num_label_rows,
		opts,
		&opt_len);

	/* free stuff... */
	for (i = 0; i < num_entries; i++) {
		free(row_data[i]);
	}
	free(row_data);
	free(col_data);

	*num_opts = num_entries;
}

static void
_dsr_get_slice_row_data(TSLEntry *slentry, ttyLabelRowData row_data_entry)
{
	DsrSLEntryExtraData *SLEntryextra;
	char buf[100];

	SLEntryextra = slentry->Extra;

	/* file system */
	DsrSLEntryGetAttr(slentry,
		DsrSLAttrTaggedMountPointStr,
		&row_data_entry[_DSR_FSREDIST_FS_COL],
		NULL);

	/* slice */
	row_data_entry[_DSR_FSREDIST_SLICE_COL] =
		xstrdup(slentry->SliceName);

	/* current free space */
	DsrSLEntryGetAttr(slentry,
		DsrSLAttrFreeSpaceStr,
		&row_data_entry[_DSR_FSREDIST_FREE_SPACE_COL],
		NULL);

	/* space needed */
	DsrSLEntryGetAttr(slentry,
		DsrSLAttrSpaceReqdStr,
		&row_data_entry[_DSR_FSREDIST_SPACE_NEEDED_COL],
		NULL);

	/* the option buttons */
	row_data_entry[_DSR_FSREDIST_CONSTRAINTS_COL] =
		xstrdup(DsrSLStateStr(slentry->State));

	/* final size */
	if (SLEntryextra->history.final_size)
		(void) sprintf(buf, "%*s", UI_FS_SIZE_DISPLAY_LENGTH,
			SLEntryextra->history.final_size);
	else
		(void) sprintf(buf, "%*s", UI_FS_SIZE_DISPLAY_LENGTH,
			" ");
	row_data_entry[_DSR_FSREDIST_MIN_SIZE_COL] = xstrdup(buf);
}

static void
_get_fsredist_col_data(ttyLabelColData **col_data)
{
	/* malloc col data array */
	(*col_data) = (ttyLabelColData *) xcalloc(
		(_DSR_FSREDIST_COLUMNS * sizeof (ttyLabelColData)));

	/* fill in col data array */
	(*col_data)[_DSR_FSREDIST_FS_COL].heading =
		xstrdup(LABEL_FILE_SYSTEM);
	(*col_data)[_DSR_FSREDIST_SLICE_COL].heading =
		xstrdup(LABEL_SLICE);
	(*col_data)[_DSR_FSREDIST_FREE_SPACE_COL].heading =
		xstrdup(LABEL_DSR_FSREDIST_CURRFREESIZE);
	(*col_data)[_DSR_FSREDIST_SPACE_NEEDED_COL].heading =
		xstrdup(LABEL_DSR_FSREDIST_SPACE_NEEDED);
	(*col_data)[_DSR_FSREDIST_CONSTRAINTS_COL].heading =
		xstrdup(LABEL_DSR_FSREDIST_OPTIONS);
	(*col_data)[_DSR_FSREDIST_MIN_SIZE_COL].heading =
		xstrdup(LABEL_DSR_FSREDIST_FINALSIZE);

	/* limit the length of the file system display */
	(*col_data)[_DSR_FSREDIST_FS_COL].max_width =
		UI_FS_DISPLAY_LENGTH;
}

static int
_dsr_fsredist_continue(void)
{
	int ret;
	char *buf;
	TLink slcurrent;
	TSLEntry *slentry;
	TLLError err;
	char *mount_point_str;
	ulong total_swap;
	DsrSLListExtraData *LLextra;

	/* make sure they have enough swap space allocated */
	(void) LLGetSuppliedListData(DsrSLHandle, NULL, (TLLData *)&LLextra);
	DsrSLGetSwapInfo(DsrSLHandle, &total_swap);
	if (total_swap < LLextra->swap.reqd) {
		/* do we force them to fix this or just warn them? */
		buf = (char *) xmalloc(strlen(APP_ER_DSR_NOT_ENOUGH_SWAP) +
			(2 * UI_FS_SIZE_DISPLAY_LENGTH) + 1);
		(void) sprintf(buf, APP_ER_DSR_NOT_ENOUGH_SWAP,
			UI_FS_SIZE_DISPLAY_LENGTH,
			kb_to_mb(LLextra->swap.reqd),
			UI_FS_SIZE_DISPLAY_LENGTH,
			kb_to_mb(total_swap));
		simple_notice(stdscr, F_OKEYDOKEY,
			TITLE_WARNING,
			buf);
		return (FAILURE);
	}

	/*
	 * make sure to warn them about losing space in
	 * slices marked as available
	 */
	buf = NULL;
	LL_WALK(DsrSLHandle, slcurrent, slentry, err) {
		if (slentry->State != SLAvailable)
			continue;

		if (!buf) {
			/* 1st one  - add the initial message */
			buf = (char *) xmalloc(strlen(
				APP_ER_DSR_AVAILABLE_LOSE_DATA)
				+ 1);
			(void) strcpy(buf,
				APP_ER_DSR_AVAILABLE_LOSE_DATA);
		}

		/* add the message itself */
		DsrSLEntryGetAttr(slentry,
			DsrSLAttrMountPointStr,
			&mount_point_str,
			NULL);
		buf = (char *) xrealloc(buf,
			strlen(buf)
			+ strlen(APP_ER_DSR_AVAILABLE_LOSE_DATA_ITEM)
			+ strlen(mount_point_str)
			+ strlen(slentry->SliceName)
			+ 1);
		(void) sprintf(buf,
			APP_ER_DSR_AVAILABLE_LOSE_DATA_ITEM,
			buf,
			UI_FS_SIZE_DISPLAY_LENGTH,
			mount_point_str,
			slentry->SliceName);

		free(mount_point_str);
	}
	if (buf) {
		if (UI_DisplayBasicMsg(UI_MSGTYPE_WARNING,
			NULL, buf) == UI_MSGRESPONSE_CANCEL) {
			/* they cancelled */
			free(buf);
			return (FAILURE);
		}
	}

	/* redo autolayout here */
	wstatus_msg(stdscr, PLEASE_WAIT_STR);
	ret = DsrSLAutoLayout(DsrSLHandle, FsSpaceInfo, 0);
	if (ret != SUCCESS) {
		/* autolayout not ok - stay on this screen */
		simple_notice(stdscr, F_OKEYDOKEY,
			TITLE_APP_ER_CANT_AUTO_LAYOUT,
			APP_ER_DSR_AUTOLAYOUT_FAILED);
	}
	return (ret);
}

static TSLEntry *
_dsr_fsredist_index_to_slentry(int opt_index)
{
	int i;
	TLink slcurrent;
	TSLEntry *slentry;
	TLLError err;
	DsrSLEntryExtraData *SLEntryextra;

	i = 0;
	LL_WALK(DsrSLHandle, slcurrent, slentry, err) {
		SLEntryextra = slentry->Extra;

		if (!SLEntryextra->in_filter)
			continue;

		if (i == opt_index)
			return (slentry);
		else
			i++;
	}

	return (NULL);
}

/*
 * *************************************************************************
 *	Code for the edit screen off of "Select Auto-layout Constraints"
 * *************************************************************************
 */
static void
_do_dsr_fsredist_edit(TSLEntry *slentry)
{
	HelpEntry _help;
	int ch;
	int row;
	int wmenu_row;
	int done;
	int really_dirty;
	int indent;
	int height;
	char *opts[4];
	int num_opts;
	int selected;
	static _TEditChoices edit_type = DSR_FSREDIST_EDIT_Undefined;
	u_int fkeys;
	char *str;
	int num_fs;

	_help.win = stdscr;
	_help.type = HELP_REFER;
	_help.title = _DSR_FSREDIST_HELP_TITLE;

	fkeys = F_OKEYDOKEY | F_CANCEL | F_HELP;

	/*
	 * Initially default the edit_type.
	 * Preferred defaults in order are:
	 * - Edit slice if there is a selected, editable slice.
	 * - collapse (we want to encourage collapsing file systems).
	 * - filtering
	 */
	num_fs = DsrSLGetNumCollapseable(DsrSLHandle);
	if (edit_type == DSR_FSREDIST_EDIT_Undefined) {
		if (_SLICE_IS_EDITABLE(slentry))
			edit_type = DSR_FSREDIST_EDIT_Slice;
		else if (num_fs > 0)
			edit_type = DSR_FSREDIST_EDIT_Collapse;
		else
			edit_type = DSR_FSREDIST_EDIT_Filter;
	}
	num_opts = 0;
	if (_SLICE_IS_EDITABLE(slentry)) {
		DsrSLEntryGetAttr(slentry,
			DsrSLAttrMountPointStr, &str,
			NULL);
		strip_whitespace(str);
		opts[num_opts] = (char *) xmalloc(
			strlen(LABEL_CUI_FSREDIST_EDIT_SLICE) +
			strlen(slentry->SliceName) +
			strlen(str) + 1);
		(void) sprintf(opts[num_opts],
			LABEL_CUI_FSREDIST_EDIT_SLICE,
			slentry->SliceName,
			str);
		num_opts++;
	}
	opts[num_opts++] = LABEL_CUI_FSREDIST_FILTER;
	if (num_fs > 0)
		opts[num_opts++] = LABEL_CUI_FSREDIST_COLLAPSE;
	opts[num_opts++] = LABEL_CUI_FSREDIST_RESET;

	done = FALSE;
	really_dirty = TRUE;
	while (!done) {
		/*
		 * If the whole window is likely to be munged and we
		 * want to redraw the whole thing then make sure that we
		 * redraw the header, message, and footer too.
		 */
		if (really_dirty) {
			really_dirty = FALSE;

			(void) werase(stdscr);
			(void) wclear(stdscr);
			wheader(stdscr, TITLE_CUI_FSREDIST_EDIT);

			row = HeaderLines;
			row = wword_wrap(stdscr,
				row, INDENT0, COLS - (2 * INDENT0),
				MSG_CUI_FSREDIST_EDIT);
			++row;

			wfooter(stdscr, fkeys);
		}
		wmenu_row = row;

		indent = INDENT0;
		height = LINES - wmenu_row - FooterLines;
		selected = _dsr_map_edit_edit_type_to_selected(
				edit_type, slentry);
		write_debug(CUI_DEBUG_L1, "edit_type = %d", edit_type);
		write_debug(CUI_DEBUG_L1_NOHD, "selected = %d", selected);
		ch = wmenu(stdscr,
			wmenu_row,
			indent,
			height,
			COLS - indent - 2,
			(Callback_proc *) show_help, (void *) &_help,
			(Callback_proc *) NULL, (void *) NULL,
			(Callback_proc *) NULL, (void *) NULL,
			NULL,
			opts,
			num_opts,
			(void *) &selected,
			M_RADIO | M_RADIO_ALWAYS_ONE | M_CHOICE_REQUIRED,
			fkeys);

		if (is_ok(ch)) {
			edit_type = _dsr_map_edit_selected_to_edit_type(
				selected, slentry);
			write_debug(CUI_DEBUG_L1, "selected = %d", selected);
			write_debug(CUI_DEBUG_L1_NOHD,
				"edit_type = %d", edit_type);
			switch (edit_type) {
			case DSR_FSREDIST_EDIT_Slice:
				if (_do_dsr_fsredist_edit_slice(slentry))
					done = 1;
				else
					really_dirty = TRUE;
				break;
			case DSR_FSREDIST_EDIT_Filter:
				if (_do_dsr_fsredist_filter())
					done = 1;
				else
					really_dirty = TRUE;
				break;
			case DSR_FSREDIST_EDIT_Collapse:
				/*
				 * get a list of FS's to display here that
				 * are sorted alphabetically.
				 */
				SLSort(DsrSLHandle, SLMountPointAscending);

				if (_do_dsr_fsredist_collapse())
					done = 1;
				else
					really_dirty = TRUE;

				/* sort the original list back to normal */
				SLSort(DsrSLHandle, SLSliceNameAscending);

				break;
			case DSR_FSREDIST_EDIT_Reset:
				_do_dsr_fsredist_reset();
				done = 1;
				break;
			}
		} else if (is_cancel(ch)) {
			/* drop back to previous screen */
			done = 1;
		} else
			beep();
	}

	if (slentry)
		free(opts[0]);
}

/*
 * The Edit menus vary depending on if there is a slice selected in the
 * main screen and on whether they have any collapseable file systems.
 * This means that we have to do some funky mapping from the edit_type
 * to the index of the selected item and vice versa.
 * Basically, depending on the situation, we can end up with menus like
 * one of the following:
 * ---------------
 * 0. Edit slice
 * 1. Filter
 * 2. Collapse
 * 3. Reset
 * ---------------
 *
 * ---------------
 * 0. Filter
 * 1. Collapse
 * 2. Reset
 * ---------------
 *
 * ---------------
 * 0. Edit slice
 * 1. Filter
 * 2. Reset
 * ---------------
 *
 * ---------------
 * 0. Filter
 * 1. Reset
 * ---------------
 */
static _TEditChoices
_dsr_map_edit_selected_to_edit_type(int selected, TSLEntry *slentry)
{
	int is_edit;
	int is_collapse;
	int num_fs;

	is_edit = TRUE;
	if (!_SLICE_IS_EDITABLE(slentry))
		is_edit = FALSE;

	num_fs = DsrSLGetNumCollapseable(DsrSLHandle);
	is_collapse = TRUE;
	if (num_fs == 0) {
		is_collapse = FALSE;
	}

	switch (selected) {
	case 0:
		if (is_edit)
			return (DSR_FSREDIST_EDIT_Slice);
		else
			return (DSR_FSREDIST_EDIT_Filter);
	case 1:
		if (is_edit)
			return (DSR_FSREDIST_EDIT_Filter);
		else if (is_collapse)
			return (DSR_FSREDIST_EDIT_Collapse);
		else
			return (DSR_FSREDIST_EDIT_Reset);
	case 2:
		if (is_edit && is_collapse)
			return (DSR_FSREDIST_EDIT_Collapse);
		else
			return (DSR_FSREDIST_EDIT_Reset);
	case 3:
		return (DSR_FSREDIST_EDIT_Reset);
	}

	/* NOTREACHED */
}

static int
_dsr_map_edit_edit_type_to_selected(
	_TEditChoices edit_type,
	TSLEntry *slentry)
{
	int no_edit_offset;
	int no_collapse_offset;
	int num_fs;

	/*
	 * the last setting was edit slice, but there is no
	 * edittable slice this time, so default it.
	 */
	num_fs = DsrSLGetNumCollapseable(DsrSLHandle);
	if (edit_type == DSR_FSREDIST_EDIT_Slice &&
		!_SLICE_IS_EDITABLE(slentry)) {
		if (num_fs > 0)
			edit_type = DSR_FSREDIST_EDIT_Collapse;
		else
			edit_type =  DSR_FSREDIST_EDIT_Filter;
	}

	no_edit_offset = 0;
	if (!_SLICE_IS_EDITABLE(slentry))
		no_edit_offset = -1;

	no_collapse_offset = 0;
	if (num_fs == 0) {
		no_collapse_offset = -1;
	}

	switch (edit_type) {
	case DSR_FSREDIST_EDIT_Slice:
		return (0);
	case DSR_FSREDIST_EDIT_Filter:
		return (1 + no_edit_offset);
	case DSR_FSREDIST_EDIT_Collapse:
		return (2 + no_edit_offset);
	case DSR_FSREDIST_EDIT_Reset:
		return (3 + no_edit_offset + no_collapse_offset);
	}

	/* NOTREACHED */
}

static void
_do_dsr_fsredist_reset(void)
{
	/* reset the defaults */
	DsrSLUIResetDefaults(DsrSLHandle, FsSpaceInfo, TRUE);
}

static int
_do_dsr_fsredist_edit_slice(TSLEntry *slentry)
{
	HelpEntry _help;
	int row;
	u_int fkeys;
	DsrSLListExtraData *LLextra;
	int i;
	int ch;
	char **opts;
	int num_opts;
	int selected;
	int done;
	int really_dirty;
	int width;
	int height;
	char *menu_label;
	int ret;

	_help.win = stdscr;
	_help.type = HELP_REFER;
	_help.title = _DSR_FSREDIST_HELP_TITLE;

	/* get list level user data */
	(void) LLGetSuppliedListData(DsrSLHandle, NULL, (TLLData *)&LLextra);

	/* contraints menu label */
	menu_label = (char *) xmalloc(strlen(LABEL_DSR_FSREDIST_OPTIONS) +2);
	(void) sprintf(menu_label, "%s:", LABEL_DSR_FSREDIST_OPTIONS);

	/* setup the menu options */
	_get_dsr_edit_slice_opts(slentry, &opts, &num_opts);

	/* default selected one */
	selected = _dsr_edit_slice_state_to_index(slentry);

	fkeys = F_OKEYDOKEY | F_CANCEL | F_HELP;

	width = COLS - INDENT2;
	height = LINES - HeaderLines - FooterLines;

	done = FALSE;
	really_dirty = TRUE;
	while (!done) {
		/*
		 * If the whole window is likely to be munged and we
		 * want to redraw the whole thing then make sure that we
		 * redraw the header, message, and footer too.
		 */
		if (really_dirty) {
			really_dirty = FALSE;

			(void) werase(stdscr);
			(void) wclear(stdscr);
			wheader(stdscr, TITLE_CUI_FSREDIST_EDIT_SLICE);

			row = HeaderLines;
			row = wword_wrap(stdscr, row,
				INDENT0, COLS - (2 * INDENT0),
				MSG_CUI_FSREDIST_EDIT_SLICE);
			row++;

			_dsr_edit_slice_draw_info(slentry, &row);
			row++;

			wfooter(stdscr, fkeys);
		}

		/* draw and manage the menu input */
		ch = wmenu(stdscr,
			row,
			INDENT0,
			height,
			width,
			(Callback_proc *) show_help, (void *) &_help,
			(Callback_proc *) NULL, (void *) NULL,
			(Callback_proc *) NULL, (void *) NULL,
			menu_label,
			opts,
			num_opts,
			(void *) &selected,
			M_RADIO | M_RADIO_ALWAYS_ONE | M_CHOICE_REQUIRED,
			fkeys);

		if (is_cancel(ch)) {
			done = 1;
			ret = 0;
		} else if (is_ok(ch)) {
			/*
			 * present screen to get changeable size now,
			 * if necessary,
			 *
			 * SUCCESS: they entered a good size
			 * and hit OK
			 * FAILURE: they exitted the screen via a
			 * cancel
			 */
			if (_dsr_edit_slice_size(slentry, selected)
				== SUCCESS) {
				/* size ok */
				slentry->State = _dsr_edit_slice_index_to_state(
					slentry, selected);
				done = 1;
				ret = 1;
			} else {
				/* user cancel */
				really_dirty = TRUE;
			}
		} else
			beep();
	}

	for (i = 0; i < num_opts; i++) {
		free (opts[i]);
	}
	free (opts);

	return (ret);
}

static void
_dsr_edit_slice_draw_info(TSLEntry *slentry, int *row)
{
	DsrSLListExtraData *LLextra;
	char *str;
	char buf[84];
	int sizes_max;
	ttyLabelColData *col_data = NULL;
	ttyLabelRowData *row_data = NULL;
	char **label_rows;
	int num_label_rows;
	int i;
	char **entries;
	int num_entries;

	/*
	 * Required Size: xxx
	 * Existing Size: xxx
	 */

	/* max field width */
	sizes_max = MAX(strlen(LABEL_DSR_FSREDIST_REQSIZE),
		strlen(LABEL_DSR_FSREDIST_CURRSIZE));

	/* req'd size */

	/*
	 * swap is weird.
	 * There can be more than one swap, so presenting a required
	 * size per swap is not really possible.
	 * So present each swap entries reqd size as the total reqd size
	 * and only error check the totals
	 * as the user leaves the screen
	 */
	if (slentry->InVFSTab && slentry->FSType == SLSwap) {
		(void) LLGetSuppliedListData(DsrSLHandle, NULL,
			(TLLData *)&LLextra);

		str = (char *) xmalloc(UI_FS_SIZE_DISPLAY_LENGTH + 1);
		(void) sprintf(str, "%*lu",
			UI_FS_SIZE_DISPLAY_LENGTH,
			kb_to_mb(LLextra->swap.reqd));
	} else {
		DsrSLEntryGetAttr(slentry,
			DsrSLAttrReqdSizeStr, &str,
			NULL);
	}
	(void) sprintf(buf, "%*s %s",
		sizes_max,
		LABEL_DSR_FSREDIST_REQSIZE,
		str);
	mvwprintw(stdscr,
		*row,
		INDENT0,
		buf);
	(*row)++;
	free(str);

	/* current size */
	DsrSLEntryGetAttr(slentry,
		DsrSLAttrExistingSizeStr, &str,
		NULL);
	(void) sprintf(buf, "%*s %s",
		sizes_max,
		LABEL_DSR_FSREDIST_CURRSIZE,
		str);
	mvwprintw(stdscr,
		*row,
		INDENT0,
		buf);
	(*row)++;
	free(str);

	/*
	 * now get the data for the rest of the file system so it looks
	 * formatted the same as the main auto-layout constraints
	 * screen, but for just this one slice.
	 */
	(*row)++;
	_get_fsredist_col_data(&col_data);
	row_data = (ttyLabelRowData *) xmalloc(sizeof (ttyLabelRowData));
	row_data[0] = (ttyLabelRowData) xmalloc(_DSR_FSREDIST_COLUMNS *
			sizeof (char *));
	_dsr_get_slice_row_data(slentry, row_data[0]);
	tty_GetRowColData(
		(ttyLabelRowData *) row_data,
		1,
		col_data,
		_DSR_FSREDIST_COLUMNS,
		_DSR_FSREDIST_SPACING,
		&label_rows,
		&num_label_rows,
		&entries,
		&num_entries);

	/*
	 * draw the labels
	 * then draw a HLINE,
	 * then draw one row data element (which is the slice entry data
	 */
	for (i = 0; i < num_label_rows; i++) {
		mvwprintw(stdscr,
			(*row),
			INDENT0,
			"%s",
			label_rows[i]);
		(*row)++;
	}

	/* HLINE */
	wmove(stdscr, *row, INDENT0);
	whline(stdscr, ACS_HLINE, strlen(entries[0]));
	(*row)++;

	/* slice entry data */
	mvwprintw(stdscr,  *row,
		INDENT0,
		"%s",
		entries[0]);

	(*row) += 2;

	for (i = 0; i < num_label_rows; i++) {
		free (label_rows[i]);
	}
	free (label_rows);
	free (row_data[0]);
	free (row_data);
}

static int
_dsr_edit_slice_size(TSLEntry *slentry, int selected)
{
	HelpEntry _help;
	TSLState state;
	int row;
	int col;
	int done;
	int really_dirty;
	u_int fkeys;
	DsrSLEntryExtraData *SLEntryextra;
	char final_size_text[PATH_MAX + 1];
	ulong reqd_size;
	char *reqd_size_str;
	char *mount_point_str;
	int error = FALSE;
	char *buf;
	int ret;
	int ch;
	ulong final_size;
	char *old_size;

	_help.win = stdscr;
	_help.type = HELP_REFER;
	_help.title = _DSR_FSREDIST_HELP_TITLE;

	SLEntryextra = slentry->Extra;
	state = _dsr_edit_slice_index_to_state(slentry, selected);

	/* only changeable slices need size editting */
	if (state != SLChangeable)
		return (SUCCESS);

	/* seed value with current minimum size */
	if (SLEntryextra->history.final_size)
		(void) strcpy(final_size_text,
			SLEntryextra->history.final_size);
	else
		final_size_text[0] = '\0';
	strip_whitespace(final_size_text);

	/*
	 * save the current size stored in the list
	 * so we can replace it if turns out the new one
	 * is a bad one and they cancel
	 */
	old_size =
		SLEntryextra->history.final_size ?
		xstrdup(SLEntryextra->history.final_size) : NULL;

	done = FALSE;
	really_dirty = TRUE;
	while (!done) {
		if (really_dirty) {
			really_dirty = FALSE;
			(void) werase(stdscr);
			(void) wclear(stdscr);
			wheader(stdscr, TITLE_CUI_FSREDIST_EDIT_SLICE_SIZE);

			row = HeaderLines;
			row = wword_wrap(stdscr, row,
				INDENT0, COLS - (2 * INDENT0),
				MSG_CUI_FSREDIST_EDIT_SLICE_SIZE);
			row++;

			_dsr_edit_slice_draw_info(slentry, &row);
			row++;

			col = INDENT0;

			/* filter search string label */
			mvwprintw(stdscr,
				row,
				col,
				LABEL_CUI_DSR_EDIT_SIZE);
			row++;

			fkeys = F_OKEYDOKEY | F_CANCEL | F_HELP;

			wfooter(stdscr, fkeys);
		}

		ch = wget_field(stdscr,
			LSTRING,
			row,
			col,
			COLS - (2 * col),
			PATH_MAX,
			final_size_text,
			fkeys);

		/* save the entry to seed it next time */
		if (SLEntryextra->history.final_size)
			free(SLEntryextra->history.final_size);
		if (!final_size_text || !strlen(final_size_text))
			SLEntryextra->history.final_size = NULL;
		else
			SLEntryextra->history.final_size =
				xstrdup(final_size_text);
		strip_whitespace(SLEntryextra->history.final_size);

		if (is_ok(ch) || ch == '\n') {
			/* at least as big as required? */
			DsrSLEntryGetAttr(slentry,
				DsrSLAttrReqdSize, &reqd_size,
				DsrSLAttrReqdSizeStr, &reqd_size_str,
				DsrSLAttrMountPointStr, &mount_point_str,
				NULL);

			if ((!DsrSLValidFinalSize(
				final_size_text, &final_size)) ||
				((slentry->FSType != SLSwap) &&
				(final_size < reqd_size))) {

				error = TRUE;

				/* add the initial message */
				buf = (char *) xmalloc(
					strlen(APP_ER_DSR_MSG_FINAL_TOO_SMALL)
					+ 1);
				(void) strcpy(buf,
					APP_ER_DSR_MSG_FINAL_TOO_SMALL);

				/* add the message itself */
				buf = (char *) xrealloc(buf,
					strlen(buf) +
					strlen(APP_ER_DSR_ITEM_FINAL_TOO_SMALL)
					+ strlen(mount_point_str)
					+ strlen(slentry->SliceName)
					+ (2 * UI_FS_SIZE_DISPLAY_LENGTH)
					+ 1);
				(void) sprintf(buf,
					APP_ER_DSR_ITEM_FINAL_TOO_SMALL,
					buf,
					mount_point_str,
					slentry->SliceName,
					reqd_size_str,
					SLEntryextra->history.final_size);

				simple_notice(stdscr, F_OKEYDOKEY,
					TITLE_APP_ER_DSR_MSG_FINAL_TOO_SMALL,
					buf);
				free(buf);
			}
			free(reqd_size_str);
			free(mount_point_str);

			if (error) {
				done = FALSE;
				really_dirty = TRUE;
			} else {
				done = TRUE;
				ret = SUCCESS;
			}

		} else if (is_cancel(ch)) {
			if (SLEntryextra->history.final_size)
				free(SLEntryextra->history.final_size);
			SLEntryextra->history.final_size = xstrdup(old_size);
			ret = FAILURE;
			done = TRUE;
		} else if (is_help(ch)) {
			do_help_index(_help.win, _help.type, _help.title);
			really_dirty = TRUE;
		} else
			beep();
	}

	if (old_size)
		free(old_size);

	return (ret);
}

static void
_get_dsr_edit_slice_opts(TSLEntry *slentry, char ***opts, int *num_opts)
{
	char *str;
	*num_opts = 0;

	/* fund out how many state options will be displayed */
	if (slentry->AllowedStates & SLFixed) {
		(*num_opts)++;
	}
	if (slentry->AllowedStates & SLMoveable) {
		(*num_opts)++;
	}
	if (slentry->AllowedStates & SLChangeable) {
		(*num_opts)++;
	}
	if (slentry->AllowedStates & SLAvailable) {
		(*num_opts)++;
	}

	/* create the options menu */
	*opts = (char **) xcalloc(*num_opts * sizeof (char *));

	/* fill in the options menu */
	*num_opts = 0;
	if (slentry->AllowedStates & SLFixed) {
		(*opts)[*num_opts] = xstrdup(LABEL_DSR_FSREDIST_FIXED);
		(*num_opts)++;
	}
	if (slentry->AllowedStates & SLMoveable) {
		(*opts)[*num_opts] = xstrdup(LABEL_DSR_FSREDIST_MOVE);
		(*num_opts)++;
	}
	if (slentry->AllowedStates & SLChangeable) {
		str = (char *) xmalloc(
			strlen(LABEL_DSR_FSREDIST_CHANGE) + 5);
		(void) sprintf(str, "%s ...", LABEL_DSR_FSREDIST_CHANGE);
		(*opts)[*num_opts] = str;
		(*num_opts)++;
	}
	if (slentry->AllowedStates & SLAvailable) {
		(*opts)[*num_opts] = xstrdup(LABEL_DSR_FSREDIST_AVAILABLE);
		(*num_opts)++;
	}
}

static TSLState
_dsr_edit_slice_index_to_state(TSLEntry *slentry, int index)
{
	int num_opts = 0;

	if (slentry->AllowedStates & SLFixed) {
		if (index == num_opts)
			return (SLFixed);
		num_opts++;
	}
	if (slentry->AllowedStates & SLMoveable) {
		if (index == num_opts)
			return (SLMoveable);
		num_opts++;
	}
	if (slentry->AllowedStates & SLChangeable) {
		if (index == num_opts)
			return (SLChangeable);
		num_opts++;
	}
	if (slentry->AllowedStates & SLAvailable) {
		if (index == num_opts)
			return (SLAvailable);
	}

	/* NOTREACHED */
}

static int
_dsr_edit_slice_state_to_index(TSLEntry *slentry)
{
	int index = 0;

	if (slentry->AllowedStates & SLFixed) {
		if (slentry->State == SLFixed)
			return (index);
		index++;
	}
	if (slentry->AllowedStates & SLMoveable) {
		if (slentry->State == SLMoveable)
			return (index);
		index++;
	}
	if (slentry->AllowedStates & SLChangeable) {
		if (slentry->State == SLChangeable)
			return (index);
		index++;
	}
	if (slentry->AllowedStates & SLAvailable) {
		if (slentry->State == SLAvailable)
			return (index);
	}

	/* NOTREACHED */
}

/*
 * *************************************************************************
 *	Code for the collapse screen off of "Select Auto-layout Constraints"
 * *************************************************************************
 */
static int
_do_dsr_fsredist_collapse(void)
{
	HelpEntry _help;
	int ch;
	int row;
	int wmenu_row;
	int done;
	int really_dirty;
	int indent;
	int height;
	char **label_rows;
	int num_label_rows;
	char **opts;
	int num_opts;
	u_int fkeys;
	int i;
	char wmenu_label[100];
	int selected = 0;
	int ret;

	_help.win = stdscr;
	_help.type = HELP_REFER;
	_help.title = _DSR_FSREDIST_HELP_TITLE;

	_collapse_selection_bits = 0;
	fkeys = F_OKEYDOKEY | F_CANCEL | F_HELP;

	indent = INDENT0;

	done = FALSE;
	really_dirty = TRUE;
	while (!done) {
		/*
		 * If the whole window is likely to be munged and we
		 * want to redraw the whole thing then make sure that we
		 * redraw the header, message, and footer too.
		 */
		if (really_dirty) {
			really_dirty = FALSE;

			(void) werase(stdscr);
			(void) wclear(stdscr);
			wheader(stdscr, TITLE_DSR_FS_COLLAPSE);

			row = HeaderLines;
			row = wword_wrap(stdscr,
				row, INDENT0, COLS - (2 * INDENT0),
				MSG_DSR_FS_COLLAPSE);
			++row;

			wfooter(stdscr, fkeys);
		}
		wmenu_row = row;

		_dsr_collapse_get_menu_data(
			&label_rows,
			&num_label_rows,
			&opts,
			&num_opts,
			&selected);
		if (_collapse_selection_bits == 0)
			_collapse_selection_bits = selected;
		write_debug(CUI_DEBUG_L1, "collapse selections:");
		write_debug(CUI_DEBUG_L1_NOHD, "original: %d",
			_collapse_selection_bits);
		write_debug(CUI_DEBUG_L1_NOHD, "new: %d", selected);

		/*
		 * Draw the label:
		 * Draw all but the last row of the label - pass
		 * the last row of the label into the menu routine so that
		 * the wmenu routine will draw a line "----" between the
		 * label and the menu.  (wmenu does not handle multiple
		 * line labels - which is why we hack it together this
		 * way...)
		 */
		for (i = 0; i < num_label_rows - 1; i++) {
			wmove(stdscr, row, indent);
			(void) wclrtoeol(stdscr);
			mvwprintw(stdscr, row, indent, "%s",
				label_rows[i]);
			wmenu_row++;
		}
		(void) sprintf(wmenu_label, "%*.*s%s",
			5, 5, " ",
			label_rows[num_label_rows - 1]);

		height = LINES - wmenu_row - FooterLines;
		ch = wmenu(stdscr,
			wmenu_row,
			indent,
			height,
			COLS - indent - 2,
			(Callback_proc *) show_help, (void *) &_help,
			(Callback_proc *) _collapse_select_cb, (void *) opts,
			(Callback_proc *) _collapse_deselect_cb, (void *) opts,
			wmenu_label,
			opts,
			num_opts,
			(void *) &selected,
			0,
			fkeys);

		if (is_ok(ch)) {
			/* if anything changed - warn them */
			if (_dsr_collapse_is_ok(selected)) {
				done = 1;
				ret = 1;
			} else {
				really_dirty = 1;
			}
		} else if (is_cancel(ch)) {
			/* drop back to previous screen */
			(void) DsrSLResetSpaceIgnoreEntries(DsrSLHandle);
			done = 1;
			ret = 0;
		} else
			beep();
	}

	return (ret);
}

static void
_dsr_collapse_get_menu_data(
	char ***label_rows,
	int *num_label_rows,
	char ***opts,
	int *num_opts,
	int *selected)
{
	ttyLabelColData *col_data = NULL;
	ttyLabelRowData *row_data = NULL;
	ttyLabelRowData row_data_entry = NULL;
	TLink slcurrent;
	TSLEntry *slentry;
	TLLError err;
	int num_entries;
	int opt_len;
	int i;

	/* how many file systems will we display? */
	num_entries = 0;
	LL_WALK(DsrSLHandle, slcurrent, slentry, err) {
		if (SL_SLICE_IS_COLLAPSEABLE(slentry)) {
			num_entries++;
		}
	}

	/* create the menu entries */
	i = 0;
	row_data = (ttyLabelRowData *) xcalloc(
		(num_entries * sizeof (ttyLabelRowData)));
	LL_WALK(DsrSLHandle, slcurrent, slentry, err) {
		if (!SL_SLICE_IS_COLLAPSEABLE(slentry))
			continue;

		/* now we have something collapseable... */
		row_data_entry = (ttyLabelRowData)
			xmalloc(_DSR_FS_COLLAPSE_COLUMNS *
				sizeof (char *));
		row_data[i] = row_data_entry;

		/* file system */
		row_data_entry[_DSR_FSREDIST_FS_COL] =
			xstrdup(slentry->MountPoint);

		/* set as selected that ones that aren't set to collapse */
		if (!(slentry->Space->fsp_flags & FS_IGNORE_ENTRY))
			BT_SET(selected, i);

		/* display the toggle parent values */
		row_data_entry[_DSR_FS_COLLAPSE_PARENT_FS] =
			xstrdup(DsrSLGetParentFS(FsSpaceInfo,
					slentry->MountPoint));
		i++;
	}


	_get_collapse_col_data(&col_data);
	tty_GetRowColData(
		row_data,
		num_entries,
		col_data,
		_DSR_FS_COLLAPSE_COLUMNS,
		_DSR_FS_COLLAPSE_SPACING,
		label_rows,
		num_label_rows,
		opts,
		&opt_len);

	/* free stuff... */
	for (i = 0; i < num_entries; i++) {
		free(row_data[i]);
	}
	free(row_data);
	free(col_data);

	*num_opts = num_entries;
}

static void
_get_collapse_col_data(ttyLabelColData **col_data)
{
	/* malloc col data array */
	(*col_data) = (ttyLabelColData *) xcalloc(
		(_DSR_FS_COLLAPSE_COLUMNS * sizeof (ttyLabelColData)));

	/* fill in col data array */
	(*col_data)[_DSR_FS_COLLAPSE_FS_COL].heading =
		xstrdup(LABEL_DSR_FS_COLLAPSE_FS);
	(*col_data)[_DSR_FS_COLLAPSE_PARENT_FS].heading =
		xstrdup(LABEL_DSR_FS_COLLAPSE_PARENT);

	/* limit the length of the file system display */
	(*col_data)[_DSR_FSREDIST_FS_COL].max_width =
		UI_FS_DISPLAY_LENGTH;

}

static int
_collapse_select_cb(void *cb_data, void *item)
{
	TSLEntry *slentry;
	char **old_opts = (char **)cb_data;

	slentry = _dsr_collapse_index_to_slentry((int) item);
	slentry->Space->fsp_flags &= ~FS_IGNORE_ENTRY;

	/*
	 * redo the opts so when the menu redraws, it
	 * picks up any new parent info
	 */
	_dsr_collapse_reparent_opts(old_opts);
	return (1);
}


static int
_collapse_deselect_cb(void *cb_data, void *item)
{
	TSLEntry *slentry;
	char **old_opts = (char **)cb_data;

	slentry = _dsr_collapse_index_to_slentry((int) item);
	slentry->Space->fsp_flags |= FS_IGNORE_ENTRY;

	/*
	 * redo the opts so when the menu redraws, it
	 * picks up any new parent info
	 */
	_dsr_collapse_reparent_opts(old_opts);
	return (1);
}

static void
_dsr_collapse_reparent_opts(char **old_opts)
{
	char **new_opts = NULL;
	int num_opts;
	int new_select_bits = 0;
	char **dummy_label_rows;
	int dummy_num_label_rows;
	int i;

	_dsr_collapse_get_menu_data(
		&dummy_label_rows,
		&dummy_num_label_rows,
		&new_opts,
		&num_opts,
		&new_select_bits);

	for (i = 0; i < num_opts; i++) {
		if (old_opts[i])
			free(old_opts[i]);
		old_opts[i] = new_opts[i];
	}
	free(new_opts);
	for (i = 0; i < dummy_num_label_rows; i++)
		free(dummy_label_rows[i]);
	free(dummy_label_rows);
}


static TSLEntry *
_dsr_collapse_index_to_slentry(int opt_index)
{
	int i;
	TLink slcurrent;
	TSLEntry *slentry;
	TLLError err;

	i = 0;
	LL_WALK(DsrSLHandle, slcurrent, slentry, err) {
		if (!SL_SLICE_IS_COLLAPSEABLE(slentry))
			continue;
		if (i == opt_index)
			return (slentry);
		else
			i++;
	}

	return (NULL);
}

static int
_dsr_collapse_is_ok(int selected)
{
	int i;
	TLink slcurrent;
	TSLEntry *slentry;
	TLLError err;

	/*
	 * figure out if any settings have actually changed...
	 * if none have - then there's nothing more to do here.
	 */
	write_debug(CUI_DEBUG_L1, "collapse selections:");
	write_debug(CUI_DEBUG_L1_NOHD, "original: %d",
		_collapse_selection_bits);
	write_debug(CUI_DEBUG_L1_NOHD, "new: %d", selected);
	if (selected == _collapse_selection_bits) {
		_collapse_selection_bits = selected;
		return (TRUE);
	}

	/*
	 * Ok - if we get here then we know that settings have changed:
	 * 	- warn the user
	 *	- do space recalc
	 */
	if (yes_no_notice(stdscr, F_OKEYDOKEY | F_CANCEL,
		TRUE, FALSE,
		TITLE_WARNING,
		LABEL_DSR_FS_COLLAPSE_CHANGED) == FALSE) {
		/*
		 * user chose cancel from the warning -
		 * abort normal OK processing and drop back to the
		 * collapse dialog.
		 */
		return (FALSE);
	}

	/*
	 * update the FSspace flags correctly to indicate to the
	 * space checking logic which file systems are and are not
	 * collapsed.
	 */
	i = 0;
	LL_WALK(DsrSLHandle, slcurrent, slentry, err) {
		if (!SL_SLICE_IS_COLLAPSEABLE(slentry))
			continue;

		if (BT_TEST(&selected, i)) {
			/* toggle set ==> not collapsed */
			slentry->Space->fsp_flags &= ~FS_IGNORE_ENTRY;
		} else {
			/* toggle not set ==> collapsed */
			slentry->Space->fsp_flags |= FS_IGNORE_ENTRY;
		}

		DsrSLEntrySetDefaults(slentry);
		DsrSLUIEntrySetDefaults(slentry);

		i++;
	}

	/*
	 * debug...
	 * print out all ignored entries.
	 */
	for (i = 0; FsSpaceInfo[i]; i++) {
		if (FsSpaceInfo[i]->fsp_flags & FS_IGNORE_ENTRY) {
			write_debug(CUI_DEBUG_L1,
				"Collapsed file system: %s (%s)",
				FsSpaceInfo[i]->fsp_mntpnt,
				FsSpaceInfo[i]->fsp_fsi->fsi_device);
		}
	}

	if (verify_fs_layout(FsSpaceInfo, NULL, NULL)
		== SP_ERR_NOT_ENOUGH_SPACE) {
		/*
		 * throw up the space dialog since some file systems
		 * still fail.
		 */
		(void) do_dsr_space_req(FALSE);
	} else {
		(void) simple_notice(stdscr, F_OKEYDOKEY,
			UI_TITLE_INFORMATION,
			APP_DSR_COLLAPSE_SPACE_OK);

	}

	/* reset the defaults */
	DsrSLUIResetDefaults(DsrSLHandle, FsSpaceInfo, FALSE);

	return (TRUE);
}

/*
 * *************************************************************************
 *	Code for the filter screen off of "Select Auto-layout Constraints"
 * *************************************************************************
 */

static int
_do_dsr_fsredist_filter(void)
{
	HelpEntry _help;
	int row;
	u_int fkeys;
	char *str;
	DsrSLListExtraData *LLextra;
	int ch;
	char *opts[_DSR_FILTER_NUM_OPTIONS];
	int selected;
	int done;
	int really_dirty;
	int width;
	int height;
	int ret;
	char buf[100];

	_help.win = stdscr;
	_help.type = HELP_REFER;
	_help.title = _DSR_FSREDIST_HELP_TITLE;

	/* get list level user data */
	(void) LLGetSuppliedListData(DsrSLHandle, NULL, (TLLData *)&LLextra);

	/* the message text */
	str = (char *) xmalloc (strlen(MSG_CUI_DSR_FILTER) +
		strlen(LABEL_DSR_FSREDIST_FILTER_SLICE) +
		strlen(LABEL_DSR_FSREDIST_FILTER_MNTPNT) + 1);
	(void) sprintf(str, MSG_CUI_DSR_FILTER,
		LABEL_DSR_FSREDIST_FILTER_SLICE,
		LABEL_DSR_FSREDIST_FILTER_MNTPNT);

	/* setup the menu options */
	opts[_DSR_FILTER_ALL] =
		LABEL_DSR_FSREDIST_FILTER_ALL;
	opts[_DSR_FILTER_FAILED] =
		LABEL_DSR_FSREDIST_FILTER_FAILED;
	opts[_DSR_FILTER_VFSTAB] =
		LABEL_DSR_FSREDIST_FILTER_VFSTAB;
	opts[_DSR_FILTER_NONVFSTAB] =
		LABEL_DSR_FSREDIST_FILTER_NONVFSTAB;

	(void) sprintf(buf, "%s ...", LABEL_DSR_FSREDIST_FILTER_SLICE);
	opts[_DSR_FILTER_SLICE] = xstrdup(buf);

	(void) sprintf(buf, "%s ...", LABEL_DSR_FSREDIST_FILTER_MNTPNT);
	opts[_DSR_FILTER_FS] = xstrdup(buf);

	/* default selected one */
	selected = _dsr_filter_type_to_index(LLextra->history.filter_type);

	fkeys = F_OKEYDOKEY | F_CANCEL | F_HELP;

	width = COLS - INDENT2;
	height = LINES - HeaderLines - FooterLines;

	done = FALSE;
	really_dirty = TRUE;
	while (!done) {
		/*
		 * If the whole window is likely to be munged and we
		 * want to redraw the whole thing then make sure that we
		 * redraw the header, message, and footer too.
		 */
		if (really_dirty) {
			really_dirty = FALSE;

			(void) werase(stdscr);
			(void) wclear(stdscr);
			wheader(stdscr, TITLE_DSR_FILTER);

			row = HeaderLines;
			row = wword_wrap(stdscr, row,
				INDENT0, COLS - (2 * INDENT0), str);
			++row;

			wfooter(stdscr, fkeys);
		}

		/* draw and manage the menu input */
		ch = wmenu(stdscr,
			row,
			INDENT1,
			height,
			width,
			(Callback_proc *) show_help, (void *) &_help,
			(Callback_proc *) NULL, (void *) NULL,
			(Callback_proc *) NULL, (void *) NULL,
			LABEL_DSR_FSREDIST_FILTER_RADIO,
			opts,
			_DSR_FILTER_NUM_OPTIONS,
			(void *) &selected,
			M_RADIO | M_RADIO_ALWAYS_ONE | M_CHOICE_REQUIRED,
			fkeys);

		/* save the final menu selection */
		LLextra->history.filter_type =
			_dsr_filter_index_to_type(selected);

		if (is_cancel(ch)) {
			done = 1;
			ret = 0;
		} else if (is_ok(ch)) {
			/*
			 * present screen to get filter spec now,
			 * if necessary, and do the filter
			 *
			 * SUCCESS: they entered a good RE if necessary)
			 * and hit OK
			 * FAILURE: they exitted the screen via a
			 * cancel
			 */
			if (_do_dsr_filter_pattern(
				_dsr_filter_index_to_type(selected))
				== SUCCESS) {
				/* filterspec ok */

				done = 1;
				ret = 1;
			} else {
				/* user cancel */
				really_dirty = TRUE;
			}
		} else
			beep();
	}

	free(str);
	free(opts[_DSR_FILTER_SLICE]);
	free(opts[_DSR_FILTER_FS]);

	return (ret);
}

static int
_dsr_filter_type_to_index(TSLFilter filter_type)
{
	int index;

	switch (filter_type) {
	case SLFilterAll:
		index = _DSR_FILTER_ALL;
		break;
	case SLFilterFailed:
		index = _DSR_FILTER_FAILED;
		break;
	case SLFilterVfstabSlices:
		index = _DSR_FILTER_VFSTAB;
		break;
	case SLFilterNonVfstabSlices:
		index = _DSR_FILTER_NONVFSTAB;
		break;
	case SLFilterSliceNameSearch:
		index = _DSR_FILTER_SLICE;
		break;
	case SLFilterMountPntNameSearch:
		index = _DSR_FILTER_FS;
		break;
	}

	return (index);
}

static TSLFilter
_dsr_filter_index_to_type(int index)
{
	TSLFilter filter_type;


	switch (index) {
	case _DSR_FILTER_ALL:
		filter_type = SLFilterAll;
		break;
	case _DSR_FILTER_FAILED:
		filter_type = SLFilterFailed;
		break;
	case _DSR_FILTER_VFSTAB:
		filter_type = SLFilterVfstabSlices;
		break;
	case _DSR_FILTER_NONVFSTAB:
		filter_type = SLFilterNonVfstabSlices;
		break;
	case _DSR_FILTER_SLICE:
		filter_type = SLFilterSliceNameSearch;
		break;
	case _DSR_FILTER_FS:
		filter_type = SLFilterMountPntNameSearch;
		break;
	}

	return (filter_type);
}

/*
 * Get the filter spec
 */
static int
_do_dsr_filter_pattern(TSLFilter filter_type)
{
	HelpEntry _help;
	DsrSLListExtraData *LLextra;
	char retextstr[PATH_MAX + 1];
	char *msg;
	int row;
	int col;
	u_int fkeys;
	int ret;
	int ch;
	int done;
	int really_dirty;
	char *filter_str;
	char *retext_label;

	_help.win = stdscr;
	_help.type = HELP_REFER;
	_help.title = _DSR_FSREDIST_HELP_TITLE;

	/* get list level user data */
	(void) LLGetSuppliedListData(DsrSLHandle, NULL, (TLLData *)&LLextra);

	/*
	 * we only need a filter pattern for some filter types.
	 */
	if (filter_type != SLFilterSliceNameSearch &&
		filter_type != SLFilterMountPntNameSearch) {
		/* just do the filter */
		return (_do_dsr_slice_filter(NULL));
	}

	/* message text */
	filter_str = DsrSLFilterTypeStr(LLextra->history.filter_type);
	msg = (char *) xmalloc(
		strlen(MSG_CUI_FSREDIST_FILTER) +
		strlen(filter_str) +
		1);
	(void) sprintf(msg, MSG_CUI_FSREDIST_FILTER, filter_str);

	/* previous filter spec */
	if (LLextra->history.filter_pattern)
		(void) strcpy(retextstr, LLextra->history.filter_pattern);
	else
		retextstr[0] = '\0';

	/* text label for retextstr field */
	retext_label = (char *) xmalloc(strlen(LABEL_CUI_DSR_FILTER_PATTERN) +
		strlen(LABEL_DSR_FSREDIST_FILTER_RE_EG) + 1);
	(void) sprintf(retext_label, LABEL_CUI_DSR_FILTER_PATTERN,
		LABEL_DSR_FSREDIST_FILTER_RE_EG);

	done = FALSE;
	really_dirty = TRUE;
	while (!done) {
		if (really_dirty) {
			really_dirty = FALSE;
			(void) werase(stdscr);
			(void) wclear(stdscr);
			wheader(stdscr, TITLE_CUI_FSREDIST_FILTER);

			row = HeaderLines;
			row = wword_wrap(stdscr, row,
				INDENT0, COLS - (2 * INDENT0), msg);
			row++;

			col = INDENT1;

			/* filter search string label */
			mvwprintw(stdscr,
				row,
				col,
				retext_label);
			row++;

			fkeys = F_OKEYDOKEY | F_CANCEL | F_HELP;

			wfooter(stdscr, fkeys);
		}

		ch = wget_field(stdscr,
			LSTRING,
			row,
			col,
			COLS - (2 * col),
			PATH_MAX,
			retextstr,
			fkeys);

		/* save the entry to seed it next time */
		if (LLextra->history.filter_pattern)
			free(LLextra->history.filter_pattern);
		if (!retextstr || !strlen(retextstr))
			LLextra->history.filter_pattern = NULL;
		else
			LLextra->history.filter_pattern =
				xstrdup(retextstr);

		if (is_ok(ch) || ch == '\n') {

			/* did they enter a search pattern at all? */
			if (!retextstr || !strlen(retextstr)) {
				simple_notice(stdscr, F_OKEYDOKEY,
					TITLE_ERROR,
					APP_ER_DSR_RE_MISSING);
				done = FALSE;
				really_dirty = TRUE;
				continue;
			}

			/*
			 * filter the list
			 * warn and don't proceed if there's an RE compile error
			 * or if there are no matches for this filter
			 */
			if (_do_dsr_slice_filter(retextstr) == SUCCESS) {
				done = TRUE;
				ret = SUCCESS;
			} else {
				done = FALSE;
				really_dirty = TRUE;
			}
		} else if (is_cancel(ch)) {
			ret = FAILURE;
			done = TRUE;
		} else if (is_help(ch)) {
			do_help_index(_help.win, _help.type, _help.title);
			really_dirty = TRUE;
		} else
			beep();
	}

	free(retext_label);
	free(msg);
	return (ret);
}

static int
_do_dsr_slice_filter(char *retextstr)
{
	int match_cnt;
	char *str;

	if (DsrSLFilter(DsrSLHandle, &match_cnt) == FAILURE) {
		/* only get here if recomp failed */
		str = (char *) xmalloc(
			strlen(APP_ER_DSR_RE_COMPFAIL) +
			strlen(retextstr) + 1);
		(void) sprintf(str, APP_ER_DSR_RE_COMPFAIL, retextstr);
		simple_notice(stdscr, F_OKEYDOKEY,
			TITLE_ERROR, APP_ER_DSR_RE_COMPFAIL);
		return (FAILURE);
	}
	if (match_cnt == 0) {
		simple_notice(stdscr, F_OKEYDOKEY,
			TITLE_ERROR,
			APP_ER_DSR_FILTER_NOMATCH);
		return (FAILURE);
	}

	return (SUCCESS);
}
