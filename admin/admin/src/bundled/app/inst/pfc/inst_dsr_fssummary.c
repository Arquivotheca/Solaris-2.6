#ifndef lint
#pragma ident "@(#)inst_dsr_fssummary.c 1.11 96/08/13 SMI"
#endif

/*
 * Copyright (c) 1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	inst_dsr_fssummary.c
 * Group:	ttinstall
 * Description:
 */

#include <stdlib.h>

#include "pf.h"
#include "tty_pfc.h"
#include "inst_msgs.h"

/* spacing between column entries */
#define	_DSR_FS_SUMMARY_SPACING		3

/* total number of columns in table */
#define	_DSR_FS_SUMMARY_COLUMNS		6

/* column indexes */
#define	_DSR_FS_SUMMARY_FS_COL		0
#define	_DSR_FS_SUMMARY_NEW_SLICE_COL	1
#define	_DSR_FS_SUMMARY_NEW_SIZE_COL	2
#define	_DSR_FS_SUMMARY_WHAT_COL	3
#define	_DSR_FS_SUMMARY_ORIG_SLICE_COL	4
#define	_DSR_FS_SUMMARY_ORIG_SIZE_COL	5

static void _get_col_data(ttyLabelColData **col_data);
static int _dsr_fs_summary(int row, HelpEntry _help, u_int fkeys);

parAction_t
do_dsr_fssummary(void)
{
	int ch;
	int row;
	u_int fkeys;
	HelpEntry _help;

	/* flush any premature user input */
	flush_input();

	_help.win = stdscr;
	_help.type = HELP_REFER;
	_help.title = "File System Modifications Summary Screen";

	(void) werase(stdscr);
	(void) wclear(stdscr);
	wheader(stdscr, TITLE_DSR_FSSUMMARY);

	row = HeaderLines;
	row = wword_wrap(stdscr, row, INDENT0, COLS - (2 * INDENT0),
	    MSG_DSR_FSSUMMARY);
	++row;

	fkeys = F_CONTINUE | F_GOBACK | F_CHANGE | F_EXIT | F_HELP;

	wfooter(stdscr, fkeys);

	/* display the failed slices */
	ch = _dsr_fs_summary(row, _help, fkeys);

	/* help is handled internally to the scrolling list */
	if (is_exit(ch)) {
		/* exit has already been confirmed inside scrolling list */
		return (parAExit);
	} else if (is_goback(ch)) {
		return (parAGoback);
	} else if (is_continue(ch)) {
		return (parAContinue);
	} else /* (is_change(ch)) */ {
		return (parADsrFSRedist);
	}

	/* NOTREACHED */
}

static void
_get_col_data(
	ttyLabelColData **col_data)
{
	/* malloc col data array */
	(*col_data) = (ttyLabelColData *) xcalloc(
		(_DSR_FS_SUMMARY_COLUMNS * sizeof (ttyLabelColData)));

	/* fill in col data array */
	(*col_data)[_DSR_FS_SUMMARY_FS_COL].heading =
		xstrdup(LABEL_FILE_SYSTEM);
	(*col_data)[_DSR_FS_SUMMARY_NEW_SLICE_COL].heading =
		xstrdup(LABEL_DSR_FSSUMM_NEWSLICE);
	(*col_data)[_DSR_FS_SUMMARY_NEW_SIZE_COL].heading =
		xstrdup(LABEL_DSR_FSSUMM_NEWSIZE);
	(*col_data)[_DSR_FS_SUMMARY_WHAT_COL].heading =
		xstrdup(LABEL_DSR_FSSUMM_WHAT_HAPPENED);
	(*col_data)[_DSR_FS_SUMMARY_ORIG_SLICE_COL].heading =
		xstrdup(LABEL_DSR_FSSUMM_ORIGSLICE);
	(*col_data)[_DSR_FS_SUMMARY_ORIG_SIZE_COL].heading =
		xstrdup(LABEL_DSR_FSSUMM_ORIGSIZE);

	/* limit the length of the file system display */
	(*col_data)[_DSR_FS_SUMMARY_FS_COL].max_width =
		UI_FS_DISPLAY_LENGTH;
}

static int
_dsr_fs_summary(int row, HelpEntry _help, u_int fkeys)
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
	Disk_t *new_dp;
	int new_slice;
	Disk_t *orig_dp;
	int orig_slice;
	TLink slcurrent;
	TSLEntry *slentry;
	TLLError err;
	ulong change_mask;
	int unused;
	int deleted;
	int pass;
	SliceKey *slice_key;
	int row_len;

	write_debug(CUI_DEBUG_L1, "Entering _dsr_space_req_summary");

	/*
	 * First, display the current configuration from
	 * the disk list.
	 * We want this list to be in slice name sorted order, so
	 * this relies on the disk list being sorted by slice name.
	 */
	set_units(D_MBYTE);
	num_entries = 0;
	WALK_DISK_LIST(new_dp) {
		WALK_SLICES_STD(new_slice) {
			/*
			 * slice has to have size in the new layout
			 * to be of interest here.
			 */
			if (!Sliceobj_Size(CFG_CURRENT, new_dp, new_slice)) {
				continue;
			}

			/* don't print out overlap slices */
			if (streq(Sliceobj_Use(CFG_CURRENT, new_dp, new_slice),
				OVERLAP))
				continue;

			/* get the row data for this entry */
			num_entries++;
			row_data = (ttyLabelRowData *) xrealloc(row_data,
				(num_entries * sizeof (ttyLabelRowData)));
			row_data_entry = (ttyLabelRowData)
				xmalloc(_DSR_FS_SUMMARY_COLUMNS *
					sizeof (char *));
			row_data[num_entries - 1] = row_data_entry;

			/* file system */
			row_data_entry[_DSR_FS_SUMMARY_FS_COL] =
				xstrdup(Sliceobj_Use(CFG_CURRENT,
					new_dp, new_slice));
			DsrSLUIRenameUnnamedSlices(
				&row_data_entry[_DSR_FS_SUMMARY_FS_COL]);

			/* new slice */
			row_data_entry[_DSR_FS_SUMMARY_NEW_SLICE_COL] =
				xstrdup(make_slice_name(disk_name(new_dp),
					new_slice));

			/* new size */
			(void) sprintf(buf, "%*d",
				UI_FS_SIZE_DISPLAY_LENGTH,
				(int) blocks2size(new_dp,
					Sliceobj_Size(CFG_CURRENT,
						new_dp, new_slice),
						ROUNDDOWN));
			row_data_entry[_DSR_FS_SUMMARY_NEW_SIZE_COL] =
				xstrdup(buf);

			/*
			 * In order to figure out what changed and to display
			 * the old values, we need to compare against the
			 * original disk list, which has been stored off
			 * with instance numbers in the committed
			 * state..  So, get the old data, compare it and
			 * display here...
			 */
			slentry = DsrSLGetSlice(DsrSLHandle,
				make_slice_name(disk_name(new_dp), new_slice));

			/* what happened */
			change_mask = DsrHowSliceChanged(new_dp, new_slice,
				&orig_dp, &orig_slice);
			row_data_entry[_DSR_FS_SUMMARY_WHAT_COL] =
				xstrdup(DsrHowSliceChangedStr(change_mask));

			/* this shouldn't happen! */
			if (!orig_dp) {
				write_debug(CUI_DEBUG_L1,
					"No original disk pointer for %s",
					Sliceobj_Use(CFG_CURRENT,
						new_dp, new_slice));
				continue;
			}

			/* original slice */
			if (change_mask & SliceChange_Slice_mask) {
				(void) strcpy(buf,
					make_slice_name(
						disk_name(new_dp), new_slice));
			} else {
				(void) strcpy(buf, LABEL_DSR_FSREDIST_NA);
			}
			row_data_entry[_DSR_FS_SUMMARY_ORIG_SLICE_COL] =
				xstrdup(buf);

			/* original size */
			if (change_mask & SliceChange_Size_mask) {
				(void) sprintf(buf, "%*d",
					UI_FS_SIZE_DISPLAY_LENGTH,
					(int) blocks2size(orig_dp,
					Sliceobj_Size(CFG_COMMIT,
						orig_dp, orig_slice),
					ROUNDDOWN));
			} else {
				(void) strcpy(buf, LABEL_DSR_FSREDIST_NA);
			}
			row_data_entry[_DSR_FS_SUMMARY_ORIG_SIZE_COL] =
				xstrdup(buf);
		}

		/*
		 * Now report on unused space for this disk
		 */
		set_units(D_MBYTE);
		unused = blocks2size(new_dp, sdisk_space_avail(new_dp),
			ROUNDDOWN);
		if (unused >= 1) {
			num_entries++;
			row_data = (ttyLabelRowData *) xrealloc(row_data,
				(num_entries * sizeof (ttyLabelRowData)));
			row_data_entry = (ttyLabelRowData)
				xmalloc(_DSR_FS_SUMMARY_COLUMNS *
					sizeof (char *));
			row_data[num_entries - 1] = row_data_entry;

			/* file system */
			row_data_entry[_DSR_FS_SUMMARY_FS_COL] = NULL;

			/* new slice */
			row_data_entry[_DSR_FS_SUMMARY_NEW_SLICE_COL] =
				xstrdup(disk_name(new_dp));

			/* new size */
			(void) sprintf(buf, "%d", unused);
			row_data_entry[_DSR_FS_SUMMARY_NEW_SIZE_COL] =
				xstrdup(buf);

			/* what happened */
			change_mask = SliceChange_Unused_mask;
			row_data_entry[_DSR_FS_SUMMARY_WHAT_COL] =
				xstrdup(DsrHowSliceChangedStr(change_mask));

			/* original slice */
			row_data_entry[_DSR_FS_SUMMARY_ORIG_SLICE_COL] = NULL;

			/* original size */
			row_data_entry[_DSR_FS_SUMMARY_ORIG_SIZE_COL] = NULL;
		}
	}

	/*
	 * Find collapsed file systems to report on (pass 0).
	 * Find deleted (available) file systems to report on   (pass 1).
	 */
	for (pass = 0; pass < 2; pass++) {
		if (pass == 0) {
			change_mask = SliceChange_Collapsed_mask;
		} else {
			change_mask = SliceChange_Deleted_mask;
		}

	LL_WALK(DsrSLHandle, slcurrent, slentry, err) {
		if (change_mask == SliceChange_Deleted_mask) {
			if (slentry->State == SLAvailable)
#if 0
			if (slentry->State == SLAvailable ||
				(slentry->State == SLChangeable &&
				slentry->Size == 0))
#endif
				deleted = TRUE;
			else {
				deleted = FALSE;

				slice_key = SliceobjFindUse(CFG_CURRENT, NULL,
					slentry->MountPoint,
					slentry->MountPointInstance,
					1);
			}
		} else {
			deleted = FALSE;
		}

		/*
		 * If it's collapsed (on pass 0), or it's
		 * been deleted (pass 1)
		 * i.e. deleted means it's in the slice list but
		 * not in the current disk list.
		 */
		if ((change_mask == SliceChange_Collapsed_mask &&
			slentry->State == SLCollapse) ||
			(change_mask == SliceChange_Deleted_mask && deleted)) {

			num_entries++;
			row_data = (ttyLabelRowData *) xrealloc(row_data,
				(num_entries * sizeof (ttyLabelRowData)));
			row_data_entry = (ttyLabelRowData)
				xmalloc(_DSR_FS_SUMMARY_COLUMNS *
					sizeof (char *));
			row_data[num_entries - 1] = row_data_entry;

			/* file system */
			row_data_entry[_DSR_FS_SUMMARY_FS_COL] =
				xstrdup(slentry->MountPoint);

			/* new slice */
			row_data_entry[_DSR_FS_SUMMARY_NEW_SLICE_COL] =
				xstrdup(LABEL_DSR_FSREDIST_NA);

			/* new size */
			row_data_entry[_DSR_FS_SUMMARY_NEW_SIZE_COL] =
				xstrdup(LABEL_DSR_FSREDIST_NA);

			/* what happened */
			row_data_entry[_DSR_FS_SUMMARY_WHAT_COL] =
				xstrdup(DsrHowSliceChangedStr(change_mask));

			/* original slice */
			slice_key = SliceobjFindUse(CFG_COMMIT, NULL,
				slentry->MountPoint,
				slentry->MountPointInstance,
				1);
			(void) strcpy(buf,
				make_slice_name(
					disk_name(slice_key->dp),
					slice_key->slice));
			row_data_entry[_DSR_FS_SUMMARY_ORIG_SLICE_COL] =
				xstrdup(buf);

			/* original size */
			(void) sprintf(buf, "%*d",
				UI_FS_SIZE_DISPLAY_LENGTH,
				(int) blocks2size(
					slice_key->dp,
					Sliceobj_Size(CFG_COMMIT,
						slice_key->dp,
						slice_key->slice),
					ROUNDDOWN));
			row_data_entry[_DSR_FS_SUMMARY_ORIG_SIZE_COL] =
				xstrdup(buf);
		}
	} /* end looping on slices */
	} /* end pass loop */

	_get_col_data(&col_data);
	tty_GetRowColData(
		row_data,
		num_entries,
		col_data,
		_DSR_FS_SUMMARY_COLUMNS,
		_DSR_FS_SUMMARY_SPACING,
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
		row, INDENT0 + 1,
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
