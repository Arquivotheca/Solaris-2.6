#ifndef lint
#pragma ident "@(#)inst_dsr_space_req.c 1.9 96/08/13 SMI"
#endif

/*
 * Copyright (c) 1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	inst_dsr_space_req.c
 * Group:	ttinstall
 * Description:
 */

#include <stdlib.h>
#include <libgen.h>

#include "pf.h"
#include "tty_pfc.h"
#include "inst_msgs.h"

static parAction_t _do_dsr_space_req_main(int ch);
static parAction_t _dsr_space_req_auto_layout(void);
static int _dsr_space_req_summary(FSspace **fs_space, int row,
	HelpEntry _help, u_int fkeys);
static parAction_t _do_dsr_space_req_dialog();

/* spacing between column entries */
#define	_DSR_SPACE_REQ_SPACING		3

/* total number of columns in table */
#define	_DSR_SPACE_REQ_COLUMNS		4

/* column indexes */
#define	_DSR_SPACE_REQ_FS_COL		0
#define	_DSR_SPACE_REQ_SLICE_COL	1
#define	_DSR_SPACE_REQ_ORIG_SIZE_COL	2
#define	_DSR_SPACE_REQ_REQ_SIZE_COL	3

static void
_get_col_data(
	ttyLabelColData **col_data)
{
	/* malloc col data array */
	(*col_data) = (ttyLabelColData *) xcalloc(
		(_DSR_SPACE_REQ_COLUMNS * sizeof (ttyLabelColData)));

	/* fill in col data array */
	(*col_data)[_DSR_SPACE_REQ_FS_COL].heading =
		xstrdup(LABEL_FILE_SYSTEM);
	(*col_data)[_DSR_SPACE_REQ_SLICE_COL].heading =
		xstrdup(LABEL_SLICE);
	(*col_data)[_DSR_SPACE_REQ_ORIG_SIZE_COL].heading =
		xstrdup(LABEL_DSR_SPACE_REQ_CURRSIZE);
	(*col_data)[_DSR_SPACE_REQ_REQ_SIZE_COL].heading =
		xstrdup(LABEL_DSR_SPACE_REQ_REQSIZE);

	/* limit the length of the file system display */
	(*col_data)[_DSR_SPACE_REQ_FS_COL].max_width =
		UI_FS_DISPLAY_LENGTH;

}

parAction_t
do_dsr_space_req(int main_parade)
{
	int ch;
	int row;
	u_int fkeys;
	HelpEntry _help;
	char *msg;

	/* flush any premature user input */
	flush_input();

	_help.win = stdscr;
	_help.type = HELP_TOPIC;
	_help.title = "More Space Needed for Upgrade";

	if (main_parade) {
		msg = MSG_DSR_SPACE_REQ;
	} else {
		msg = MSG_DSR_SPACE_REQ_FS_COLLAPSE;
	}

	(void) werase(stdscr);
	(void) wclear(stdscr);
	wheader(stdscr, TITLE_DSR_SPACE_REQ);

	row = HeaderLines;
	row = wword_wrap(stdscr, row, INDENT0, COLS - (2 * INDENT0), msg);
	++row;

	if (main_parade) {
		fkeys = F_AUTO | F_GOBACK | F_EXIT | F_HELP;
	} else {
		fkeys = F_OKEYDOKEY;
	}

	wfooter(stdscr, fkeys);

	/* display the failed slices */
	ch = _dsr_space_req_summary(FsSpaceInfo, row, _help, fkeys);

	if (main_parade)
		return (_do_dsr_space_req_main(ch));
	else
		return (_do_dsr_space_req_dialog());
}


/*
 * handle input for the main parade space required dialog
 */
static parAction_t
_do_dsr_space_req_main(int ch)
{

	if (is_exit(ch)) {
		/* exit has already been confirmed inside scrolling list */
		return (parAExit);
	} else if (is_goback(ch)) {
		return (parAGoback);
	}

	/* if get to here, we want to do the 1st auto-layout pass */
	return (_dsr_space_req_auto_layout());
}

/*
 * The continue/auto-layout 'callback'
 */
static parAction_t
_dsr_space_req_auto_layout()
{
	int ret;
	UI_MsgStruct *msg_info;

	/* try to run autolayout and automatically relayout stuff so it fits */
	wstatus_msg(stdscr, PLEASE_WAIT_STR);
	ret = DsrSLAutoLayout(DsrSLHandle, FsSpaceInfo, 1);
	if (ret == SUCCESS) {
		return (parADsrFSSumm);
	} else {
		/* tell them that we couldn't handle the autolayout for them */
		msg_info = UI_MsgStructInit();
		msg_info->msg_type = UI_MSGTYPE_INFORMATION;
		msg_info->title = TITLE_APP_ER_CANT_AUTO_LAYOUT;
		msg_info->msg = APP_ER_DSR_CANT_AUTO;
		msg_info->btns[UI_MSGBUTTON_CANCEL].button_text = NULL;
		msg_info->btns[UI_MSGBUTTON_HELP].button_text = NULL;

		/* invoke the message */
		(void) UI_MsgFunction(msg_info);

		/* cleanup */
		UI_MsgStructFree(msg_info);

		/* set the list to a default known state before we leave */
		DsrSLUIResetDefaults(DsrSLHandle, FsSpaceInfo, TRUE);
		return (parADsrFSRedist);
	}
}

/*
 * Note:
 *	See the tty_get_row_col_data() function description for a
 *	detailed description of the ttyLabelRowData/ttyLabelColData data
 *	structures used here.
 */
static int
_dsr_space_req_summary(FSspace **fs_space, int row,
	HelpEntry _help, u_int fkeys)
{
	int i;
	int ch;
	char buf[100];
	char **label_rows;
	char **row_entries;
	ttyScrollingListTable *entries = NULL;
	int num_entries;
	ttyLabelColData *col_data = NULL;
	ttyLabelRowData *row_data = NULL;
	ttyLabelRowData row_data_entry = NULL;
	int num_label_rows;
	int row_len;

	write_debug(CUI_DEBUG_L1, "Entering _dsr_space_req_summary");

	num_entries = 0;
	for (i = 0; fs_space && fs_space[i]; i++) {
		/*
		 * If it's a slice that's been marked as having
		 * insufficient space AND it hasn't been marked as collapsed.
		 */
		if ((fs_space[i]->fsp_flags & FS_INSUFFICIENT_SPACE) &&
			!(fs_space[i]->fsp_flags & FS_IGNORE_ENTRY)) {
			write_debug(CUI_DEBUG_L1, "%s failed",
				fs_space[i]->fsp_mntpnt);
			write_debug(CUI_DEBUG_L1_NOHD, "reqd size (kb): %lu",
				fs_space[i]->fsp_reqd_slice_size);
			write_debug(CUI_DEBUG_L1_NOHD, "current size (kb): %lu",
				fs_space[i]->fsp_cur_slice_size);

			/* create row data entry */
			num_entries++;
			row_data = (ttyLabelRowData *) xrealloc(row_data,
				(num_entries * sizeof (ttyLabelRowData)));
			row_data_entry = (ttyLabelRowData)
				xmalloc(_DSR_SPACE_REQ_COLUMNS *
					sizeof (char *));
			row_data[num_entries - 1] = row_data_entry;

			/* file system */
			row_data_entry[_DSR_SPACE_REQ_FS_COL] =
				xstrdup(fs_space[i]->fsp_mntpnt);

			/* disk slice for this file system */
			row_data_entry[_DSR_SPACE_REQ_SLICE_COL] =
				xstrdup(basename(
					fs_space[i]->fsp_fsi->fsi_device));

			/* original size */
			(void) sprintf(buf, "%5lu",
				kb_to_mb_trunc(
					fs_space[i]->fsp_cur_slice_size));
			row_data_entry[_DSR_SPACE_REQ_ORIG_SIZE_COL] =
				xstrdup(buf);

			/* required size */
			(void) sprintf(buf, "%5lu",
				kb_to_mb(fs_space[i]->fsp_reqd_slice_size));
			row_data_entry[_DSR_SPACE_REQ_REQ_SIZE_COL] =
				xstrdup(buf);
		}
	}

	_get_col_data(&col_data);
	tty_GetRowColData(
		row_data,
		num_entries,
		col_data,
		_DSR_SPACE_REQ_COLUMNS,
		_DSR_SPACE_REQ_SPACING,
		&label_rows,
		&num_label_rows,
		&row_entries,
		&row_len);

	/* convert the row entries to entries useable by the scrolling list */
	entries = (ttyScrollingListTable *)
		xcalloc((num_entries * sizeof (ttyScrollingListTable)));
	for (i = 0; i < num_entries; i++) {
		entries[i].str = row_entries[i];
	}

	/* free the row and col data and the row_entries */
	for (i = 0; i < num_entries; i++) {
		free(row_data[i]);
	}
	free(row_data);
	free(col_data);
	free(row_entries);

	/* present the summary */
	row++;
	ch = show_scrolling_list(stdscr,
		row, INDENT1,
		LINES - FooterLines - row - 2,
		row_len,
		label_rows, num_label_rows,
		entries, num_entries,
		_help,
		confirm_exit,
		fkeys);

	for (i = 0; i < num_label_rows; i++) {
		if (label_rows[i])
			free(label_rows[i]);
	}
	free(label_rows);

	for (i = 0; i < num_entries; i++) {
		if (entries[i].str)
			free(entries[i].str);
	}
	free(entries);

	return (ch);
}

/*
 * handle input for the space required dialog off of the Auto-layout
 * Constraints screen.  Only button displayed is Ok.
 */
static parAction_t
_do_dsr_space_req_dialog()
{
	return (parAContinue);
}
