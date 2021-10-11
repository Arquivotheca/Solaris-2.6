#ifndef lint
#pragma ident "@(#)pfgdsr_fssummary.c 1.9 96/08/02 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	pfgdsr_fssummary.c
 * Group:	installtool
 * Description:
 *	DSR File System Summary screen.
 *	The point is tell the user what the outcome of running
 * 	auto-layout with the current set of applied auto-layout
 *	constraints are.
 *	Reading the first portion of the list should tell them their
 *	current system layout.
 *	Remaining portions will tell them Deleted and Collapsed file
 *	systems.
 *
 *	The key words that will appear under "What Happened" are:
 *		- Nothing
 *		- Changed
 *		- Deleted
 *		- Created
 *		- Unused
 *		- Collapsed
 *
 *	An example of each:
 *	File System	New		New	What		Orig	Orig
 *			Slice		Size	Happened	Slice	Size
 *	-----------------------------------------------------------------------
 *	/		c0t0d0s0	130	Nothing		-----	-----
 *	/usr		c0t0d0s1	150	Changed		-----	175
 *	/var		c0t0d0s3	10	Changed		c0t0d0s4 -----
 *	/export/home	c0t0d0s4	200	Created
 *			c0t0d0		10	Unused
 *	/usr/openwin	-----		-----	Collapsed	c0t0d0s5 10
 *	/		-----		-----	Deleted		c0t0d1d0 100
 *
 */

#include "pfg.h"

#include "pfgDSRFSSummary_ui.h"

static void dsr_fssumm_summary(void);
static WidgetList widget_list;
static char *column_labels[] = {
	"fsColumnLabel",
	"origSliceColumnLabel",
	"origSizeColumnLabel",
	"whatHappenedColumnLabel",
	"newSliceColumnLabel",
	"newSizeColumnLabel",
	NULL
};
static char *row_column_values[] = {
	"fsValue",
	"origSliceValue",
	"origSizeValue",
	"whatHappenedValue",
	"newSliceValue",
	"newSizeValue",
	NULL
};

static Widget dsr_fssumm_dialog;

Widget
pfgCreateDsrFSSummary(void)
{
	Dimension width, height;

	/* get the dialog widget & the dialog widget list from teleuse */
	dsr_fssumm_dialog = tu_dsr_fssumm_dialog_widget(
		"dsr_fssumm_dialog", pfgTopLevel, &widget_list);

	/* set up exit callback off window manager close */
	XmAddWMProtocolCallback(pfgShell(dsr_fssumm_dialog),
		pfgWMDeleteAtom, (XtCallbackProc) pfgExit, NULL);

	/* set title */
	XtVaSetValues(pfgShell(dsr_fssumm_dialog),
		XtNtitle, TITLE_DSR_FSSUMMARY,
		XmNdeleteResponse, XmDO_NOTHING,
		NULL);

	pfgSetWidgetString(widget_list, "panelhelpText", MSG_DSR_FSSUMMARY);
	pfgSetWidgetString(widget_list, "fsColumnLabel",
		LABEL_FILE_SYSTEM);
	pfgSetWidgetString(widget_list, "origSliceColumnLabel",
		LABEL_DSR_FSSUMM_ORIGSLICE);
	pfgSetWidgetString(widget_list, "origSizeColumnLabel",
		LABEL_DSR_FSSUMM_ORIGSIZE);
	pfgSetWidgetString(widget_list, "whatHappenedColumnLabel",
		LABEL_DSR_FSSUMM_WHAT_HAPPENED);
	pfgSetWidgetString(widget_list, "newSliceColumnLabel",
		LABEL_DSR_FSSUMM_NEWSLICE);
	pfgSetWidgetString(widget_list, "newSizeColumnLabel",
		LABEL_DSR_FSSUMM_NEWSIZE);
	pfgSetStandardButtonStrings(widget_list,
		ButtonContinue, ButtonGoback, ButtonChange,
		ButtonExit, ButtonHelp, NULL);

	pfgSetMaxWidgetHeights(widget_list, column_labels);

	dsr_fssumm_summary();

	XtManageChild(dsr_fssumm_dialog);

	XtVaGetValues(pfgShell(dsr_fssumm_dialog),
	    XmNwidth, &width,
	    XmNheight, &height,
	    NULL);

	XtVaSetValues(pfgShell(dsr_fssumm_dialog),
	    XmNminWidth, width,
	    XmNmaxWidth, width,
	    XmNminHeight, height,
	    NULL);

	(void) XmProcessTraversal(
		pfgGetNamedWidget(widget_list, "continueButton"),
		XmTRAVERSE_CURRENT);

	return (dsr_fssumm_dialog);
}

/*
 */
static void
dsr_fssumm_summary(void)
{
	char 		buf[100];
	char *fsname;
	int		child;
	int		num_children;
	WidgetList	children;
	Widget		rc;
	WidgetList *entries = NULL;
	TLink slcurrent;
	TSLEntry *slentry;
	TLLError err;
	Disk_t *new_dp;
	Disk_t *orig_dp;
	int new_slice;
	int orig_slice;
	ulong change_mask;
	int num_entries;
	int deleted;
	SliceKey *slice_key;
	int pass;
	int unused;

	write_debug(GUI_DEBUG_L1, "Entering dsr_fssumm_summary");

	rc = pfgGetNamedWidget(widget_list, "fsSummRowColumn");

	XtVaGetValues(rc,
		XmNnumChildren, &num_children,
		XmNchildren, &children,
		NULL);

	if (num_children > 0) {
		for (child = 0; child < num_children; child++)
			XtDestroyWidget(children[child]);
	}

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

			/* get the teleuse widget list for this entry */
			num_entries++;
			entries = (WidgetList *) xrealloc(entries,
				(num_entries * sizeof (WidgetList)));
			(void) tu_dsr_fssumm_filesys_entry_widget(
				"dsr_fssumm_filesys_entry", rc,
				&entries[num_entries - 1]);

			/* file system */
			fsname = xstrdup(
				Sliceobj_Use(CFG_CURRENT, new_dp, new_slice));
			DsrSLUIRenameUnnamedSlices(&fsname);
			(void) sprintf(buf, "%-*.*s",
				UI_FS_DISPLAY_LENGTH,
				UI_FS_DISPLAY_LENGTH,
				fsname);
			pfgSetWidgetString(entries[num_entries - 1],
				"fsValue", buf);

			/* new slice */
			(void) strcpy(buf,
				make_slice_name(disk_name(new_dp), new_slice));
			pfgSetWidgetString(entries[num_entries - 1],
				"newSliceValue", buf);

			/* new size */
			(void) sprintf(buf, "%*d",
				UI_FS_SIZE_DISPLAY_LENGTH,
				(int) blocks2size(new_dp,
					Sliceobj_Size(CFG_CURRENT,
						new_dp, new_slice),
						ROUNDDOWN));
			pfgSetWidgetString(entries[num_entries - 1],
				"newSizeValue", buf);

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
			(void) strcpy(buf, DsrHowSliceChangedStr(change_mask));
			pfgSetWidgetString(entries[num_entries - 1],
				"whatHappenedValue", buf);

			/* this shouldn't happen! */
			if (!orig_dp) {
				write_debug(GUI_DEBUG_L1,
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
			pfgSetWidgetString(entries[num_entries - 1],
				"origSliceValue", buf);

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
			pfgSetWidgetString(entries[num_entries - 1],
				"origSizeValue", buf);
		}

		/*
		 * Now report on unused space for this disk
		 */
		set_units(D_MBYTE);
		unused = blocks2size(new_dp, sdisk_space_avail(new_dp),
			ROUNDDOWN);
		if (unused >= 1) {
			num_entries++;
			entries = (WidgetList *) xrealloc(entries,
				(num_entries * sizeof (WidgetList)));
			(void) tu_dsr_fssumm_filesys_entry_widget(
				"dsr_fssumm_filesys_entry", rc,
				&entries[num_entries - 1]);

			pfgSetWidgetString(entries[num_entries - 1],
				"fsValue", NULL);
			pfgSetWidgetString(entries[num_entries - 1],
				"newSliceValue",
				disk_name(new_dp));

			(void) sprintf(buf, "%d", unused);
			pfgSetWidgetString(entries[num_entries - 1],
				"newSizeValue", buf);

			change_mask = SliceChange_Unused_mask;
			(void) strcpy(buf, DsrHowSliceChangedStr(change_mask));
			pfgSetWidgetString(entries[num_entries - 1],
				"whatHappenedValue", buf);
			pfgSetWidgetString(entries[num_entries - 1],
				"origSliceValue", NULL);
			pfgSetWidgetString(entries[num_entries - 1],
				"origSizeValue", NULL);
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
			entries = (WidgetList *) xrealloc(entries,
				(num_entries * sizeof (WidgetList)));
			(void) tu_dsr_fssumm_filesys_entry_widget(
				"dsr_fssumm_filesys_entry", rc,
				&entries[num_entries - 1]);

			/* file system */
			(void) sprintf(buf, "%-*.*s",
				UI_FS_DISPLAY_LENGTH,
				UI_FS_DISPLAY_LENGTH,
				slentry->MountPoint);
			pfgSetWidgetString(entries[num_entries - 1],
				"fsValue", buf);

			/* new slice */
			pfgSetWidgetString(entries[num_entries - 1],
				"newSliceValue",
				LABEL_DSR_FSREDIST_NA);

			/* new size */
			pfgSetWidgetString(entries[num_entries - 1],
				"newSizeValue",
				LABEL_DSR_FSREDIST_NA);

			/* what happened */
			(void) strcpy(buf, DsrHowSliceChangedStr(change_mask));
			pfgSetWidgetString(entries[num_entries - 1],
				"whatHappenedValue", buf);

			/* original slice */
			slice_key = SliceobjFindUse(CFG_COMMIT, NULL,
				slentry->MountPoint,
				slentry->MountPointInstance,
				1);
			(void) strcpy(buf,
				make_slice_name(
					disk_name(slice_key->dp),
					slice_key->slice));
			pfgSetWidgetString(entries[num_entries - 1],
				"origSliceValue", buf);

			/* original size */
			(void) sprintf(buf, "%*d",
				UI_FS_SIZE_DISPLAY_LENGTH,
				(int) blocks2size(
					slice_key->dp,
					Sliceobj_Size(CFG_COMMIT,
						slice_key->dp,
						slice_key->slice),
					ROUNDDOWN));
			pfgSetWidgetString(entries[num_entries - 1],
				"origSizeValue", buf);
		}
	} /* end looping on slices */
	} /* end pass loop */
	entries = (WidgetList *) xrealloc(entries,
		((num_entries + 1) * sizeof (WidgetList)));
	entries[num_entries] = NULL;

	/*
	 * final alignment on all labels
	 */
	pfgSetMaxColumnWidths(widget_list,
		entries,
		column_labels,
		row_column_values,
		False, pfgAppData.dsrFSSummColumnSpace);
}

/* Ok button callback */
/* ARGSUSED */
void
dsr_fssumm_continue_cb(
	Widget button, XtPointer client_data, XtPointer call_data)
{
	/* generate the backup list */

	pfgSetAction(parAContinue);

	/* free the teleuse widget list */
	free(widget_list);
}

/* Goback button callback */
/* ARGSUSED */
void
dsr_fssumm_goback_cb(
	Widget button, XtPointer client_data, XtPointer call_data)
{

	pfgBusy(pfgShell(dsr_fssumm_dialog));

	/* free the teleuse widget list */
	free(widget_list);

	pfgSetAction(parAGoback);
	return;
}

/* ARGSUSED */
void
dsr_fssumm_change_cb(
	Widget button, XtPointer client_data, XtPointer call_data)
{
	pfgBusy(pfgShell(dsr_fssumm_dialog));
	pfgSetAction(parADsrFSRedist);
}
