#ifndef lint
#pragma ident "@(#)pfgdsr_fsredist.c 1.22 96/09/16 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	pfgdsr_fsredist.c
 * Group:	installtool
 * Description:
 */

#include "pfg.h"

#include "pfgDSRFSRedist_ui.h"

typedef struct {
	Widget frame;
	Widget option_menu_widget;
	Widget finalSize_widget;
} pfgDsrSLEntryExtraData;

typedef struct {
	Widget add_space_req;
	Widget add_space_alloced;
} pfgDsrSLListExtraData;

static WidgetList *entries = NULL;
static int num_entries;

static Widget prev_entry_frame;

static void dsr_fsredist_manage_rc(Boolean manage);
static void dsr_fsredist_store_final_sizes(void);
static void dsr_fsredist_update_totals(void);
static void dsr_fsredit_make_option_button(
	char *button_name,
	TSLState button_state,
	Widget parent,
	Widget option_menu,
	TSLEntry *slentry);
static void dsr_fsredist_display_slice_list(void);
static void set_tally_widths(void);
static void set_option_button_label(Widget button, TSLState state);
static void dsr_fsredist_set_option_attrs(TSLEntry *slentry);

static void pfgCreateFilterDialog(void);
static WidgetList widget_list;
static Widget dsr_fsredist_dialog;

#define	_DSR_FSREDIST_ENTRY_TRANSLATIONS \
	"<EnterNotify>:        DsrFSRedistEnterEntry(%s,%d)"
/* 	"<Btn2Down>:        DsrFSRedistEnterEntry(%s,%d)" */
/* 	"<ButtonPress>:        DsrFSRedistEnterEntry(%s,%d)" */

static char *row_column_labels[] = {
	"fsColumnLabel",
	"sliceColumnLabel",
	"currFreeSizeColumnLabel",
	"spaceNeededColumnLabel",
	"ALOptionsColumnLabel",
	"finalSizeColumnLabel",
	NULL
};
static char *row_column_values[] = {
	"fsValue",
	"sliceValue",
	"currFreeSizeValue",
	"spaceNeededValue",
	"ALOptionsMenu",
	"finalSizeTextField",
	NULL
};

static void DsrFSRedistEnterEntry(Widget w, XEvent * event,
	String * params, Cardinal *numParams);
static XtActionsRec dsr_fsredist_entry_actions[] = {
	{"DsrFSRedistEnterEntry", DsrFSRedistEnterEntry}
};

/* filter dialog stuff */
static WidgetList filter_dialog_widget_list;
static Widget dsr_fsredist_filter_dialog;
static void dsr_fsredist_filter_toggle_set_attrs(TSLFilter filter_type);

/* collapse dialog stuff */
static char *collapse_row_column_labels[] = {
	"fsColumnLabel",
	"parentColumnLabel",
	NULL
};

static char *collapse_row_column_values[] = {
	"fsValue",
	"parentValue",
	NULL
};

static void pfgCreateCollapseDialog(void);
static WidgetList collapse_dialog_widget_list;
static Widget dsr_fsredist_collapse_dialog;
static void dsr_fsredist_collapse_set_parents(Widget toggle, Boolean set);
static Widget *collapse_fs = NULL;
static Widget *collapse_parent = NULL;

static void dsr_fsredist_collapse_create_entries(void);
static void dsr_fsredist_collapse_exit_dialog(void);
static int dsr_fsredist_verify_cb(void *mydata, void *cb_data);

/*
 * *************************************************************************
 *	Code for the main DSR FS Redistribution window
 * *************************************************************************
 */

Widget
pfgCreateDsrFSRedist(void)
{
	char buf[100];
	Dimension width;
	Dimension height;
	DsrSLListExtraData *LLextra;
	pfgDsrSLListExtraData *ui_sl_extra;
	ulong add_space_req;
	ulong add_space_alloced;
	char *str;

	XtAppAddActions(pfgAppContext, dsr_fsredist_entry_actions,
		XtNumber(dsr_fsredist_entry_actions));

	/*
	 * this list is always displayed alphanumerically sorted
	 */
	SLSort(DsrSLHandle, SLSliceNameAscending);
	if (get_trace_level() > 2) {
		DsrSLPrint(DsrSLHandle, DEBUG_LOC);
	}

	DsrSLGetSpaceSummary(DsrSLHandle, &add_space_req, &add_space_alloced);

	(void) LLGetSuppliedListData(DsrSLHandle, NULL, (TLLData *)&LLextra);
	ui_sl_extra = (pfgDsrSLListExtraData *)
		xcalloc(sizeof (pfgDsrSLListExtraData));
	LLextra->extra  = (void *) ui_sl_extra;

	set_units(D_MBYTE);

	/* get the dialog widget & the dialog widget list from teleuse */
	dsr_fsredist_dialog = tu_dsr_fsredist_dialog_widget(
		"dsr_fsredist_dialog", pfgTopLevel, &widget_list);

	/* set up exit callback off window manager close */
	XmAddWMProtocolCallback(pfgShell(dsr_fsredist_dialog), pfgWMDeleteAtom,
		(XtCallbackProc) pfgExit, NULL);

	/* set title */
	XtVaSetValues(pfgShell(dsr_fsredist_dialog),
		XtNtitle, TITLE_DSR_FSREDIST,
		XmNdeleteResponse, XmDO_NOTHING,
		NULL);

	/* set most labels on the screen */
	str = (char *) xmalloc(strlen(MSG_DSR_FSREDIST) + 1);
	(void) sprintf(str, MSG_DSR_FSREDIST, "");
	pfgSetWidgetString(widget_list, "panelhelpText", str);
	free(str);

	/* button labels */
	pfgSetWidgetString(widget_list, "continueButton",
		LABEL_REPEAT_AUTOLAYOUT);
	pfgSetWidgetString(widget_list, "gobackButton",
		LABEL_DSR_FSREDIST_GOBACK_BUTTON);
	pfgSetWidgetString(widget_list, "resetButton",
		LABEL_DSR_FSREDIST_RESET_BUTTON);
	pfgSetWidgetString(widget_list, "exitButton",
		LABEL_DSR_FSREDIST_EXIT_BUTTON);
	pfgSetWidgetString(widget_list, "helpButton",
		LABEL_DSR_FSREDIST_HELP_BUTTON);
	pfgSetWidgetString(widget_list, "filterButton",
		LABEL_DSR_FSREDIST_FILTER);
	pfgSetWidgetString(widget_list, "collapseButton",
		LABEL_DSR_FSREDIST_COLLAPSE);

	/*
	 * make the collapse button insensitive if there are no
	 * collapseable file systems.
	 */
	if (DsrSLGetNumCollapseable(DsrSLHandle) == 0) {
		XtSetSensitive(pfgGetNamedWidget(widget_list,
			"collapseButton"), False);
	}

	/* reqd & original size labels */
	pfgSetWidgetString(widget_list, "activeSliceLabel",
		LABEL_DSR_FSREDIST_SLICE);
	pfgSetWidgetString(widget_list, "activeSliceTextField", NULL);
	pfgSetWidgetString(widget_list, "activeReqSizeLabel",
		LABEL_DSR_FSREDIST_REQSIZE);
	pfgSetWidgetString(widget_list, "activeCurrSizeLabel",
		LABEL_DSR_FSREDIST_CURRSIZE);
	pfgSetWidgetString(widget_list, "activeReqSizeTextField", NULL);
	pfgSetWidgetString(widget_list, "activeCurrSizeTextField", NULL);

	/* column labels */
	pfgSetWidgetString(widget_list, "fsColumnLabel",
		LABEL_FILE_SYSTEM);
	pfgSetWidgetString(widget_list, "sliceColumnLabel",
		LABEL_SLICE);
	pfgSetWidgetString(widget_list, "currFreeSizeColumnLabel",
		LABEL_DSR_FSREDIST_CURRFREESIZE);
	pfgSetWidgetString(widget_list, "spaceNeededColumnLabel",
		LABEL_DSR_FSREDIST_SPACE_NEEDED);
	pfgSetWidgetString(widget_list, "ALOptionsColumnLabel",
		LABEL_DSR_FSREDIST_OPTIONS);
	pfgSetWidgetString(widget_list, "finalSizeColumnLabel",
		LABEL_DSR_FSREDIST_FINALSIZE);

	pfgSetWidgetString(widget_list, "additionalSpaceLabel",
		LABEL_DSR_FSREDIST_ADDITIONAL_SPACE);
	pfgSetWidgetString(widget_list, "allocatedSpaceLabel",
		LABEL_DSR_FSREDIST_ALLOCATED_SPACE);

	/* legend labels */
	(void) sprintf(buf, "%s %s",
		LABEL_DSR_FSREDIST_LEGENDTAG_FAILED,
		LABEL_DSR_FSREDIST_LEGEND_FAILED);
	pfgSetWidgetString(widget_list, "failedLegendLabel",  buf);

	/* total labels */
	ui_sl_extra->add_space_req = pfgGetNamedWidget(widget_list,
		"additionalSpaceValue");
	ui_sl_extra->add_space_alloced = pfgGetNamedWidget(widget_list,
		"allocatedSpaceValue");
	dsr_fsredist_update_totals();

	/* make all column labels the same height */
	pfgSetMaxWidgetHeights(widget_list, row_column_labels);

	/* display the slices for all the disks with vtocs */
	dsr_fsredist_display_slice_list();

	/* set widths of space tally #'s to be equal */
	set_tally_widths();

	XtManageChild(dsr_fsredist_dialog);

	/*
	 * it's only resizeable vertically and it's min width and
	 * height are locked down to the current size
	 */
	XtVaGetValues(pfgShell(dsr_fsredist_dialog),
	    XmNwidth, &width,
	    XmNheight, &height,
	    NULL);

	XtVaSetValues(pfgShell(dsr_fsredist_dialog),
	    XmNminWidth, width,
	    XmNmaxWidth, width,
	    XmNminHeight, height,
	    NULL);

	return (dsr_fsredist_dialog);
}

static void
dsr_fsredit_make_option_button(char *button_name,
	TSLState button_state,
	Widget parent,
	Widget option_menu,
	TSLEntry *slentry)
{
	Widget menu_button;
	char *menu_button_name;

	/* get the menu button widget */
	menu_button_name = (char *) xmalloc(strlen(button_name) + 2);
	(void) sprintf(menu_button_name, "*%s", button_name);
	menu_button = XtNameToWidget(parent, menu_button_name);

	/*
	 * if the state is an allowed state AND
	 * in the case of collapsed, the slice has actually
	 * been collapsed, add this to the button menu
	 */
	if ((slentry->AllowedStates & button_state) &&
		((button_state != SLCollapse) ||
		((button_state == SLCollapse) &&
		(SL_SLICE_IS_COLLAPSEABLE(slentry))))) {

		/* add the button to the menu */
		XtManageChild(menu_button);
		set_option_button_label(menu_button, button_state);
		XtVaSetValues(menu_button,
			XmNuserData, (XtPointer) slentry,
			NULL);

		/* set the default menu option */
		if (slentry->State == button_state) {
			write_debug(GUI_DEBUG_L1,
				"setting menu history for %s to %s",
					slentry->SliceName, button_name);
			XtVaSetValues(option_menu,
				XmNmenuHistory, menu_button,
				NULL);
		}
	} else {
		XtUnmanageChild(menu_button);
	}
}

/*
 * display the slice entry from scratch
 */
static void
dsr_fsredist_display_slice(Widget rc, TSLEntry *slentry)
{
	DsrSLEntryExtraData *SLEntryextra =
		(DsrSLEntryExtraData *)slentry->Extra;
	pfgDsrSLEntryExtraData *ui_extra;
	char *str;
	char buf[UI_FS_SIZE_DISPLAY_LENGTH + 1];
	Widget entry_widget;
	char translations_str[PATH_MAX];
	XtTranslations entry_translations;
	Widget option_menu;

	if (!SLEntryextra->in_filter) {
		return;
	}

	/*
	 * if we haven't already attached ui_extra data to this
	 * slice entry, do it now.
	 */
	if (!SLEntryextra->extra) {
		ui_extra = (pfgDsrSLEntryExtraData *)
			xcalloc(sizeof (pfgDsrSLEntryExtraData));
		SLEntryextra->extra = (void *)ui_extra;
	} else {
		ui_extra = SLEntryextra->extra;
	}

	/* get a new slice entry widget list */
	num_entries++;
	entries = (WidgetList *) xrealloc(entries,
		(num_entries * sizeof (WidgetList)));
	entry_widget = tu_dsr_fsredist_entry_widget(
		"dsr_fsredist_entry", rc, &entries[num_entries - 1]);

	/*
	 * translations...
	 */
	(void) sprintf(translations_str, _DSR_FSREDIST_ENTRY_TRANSLATIONS,
		slentry->MountPoint, slentry->MountPointInstance);
	entry_translations = XtParseTranslationTable(translations_str);
	XtOverrideTranslations(entry_widget, entry_translations);

	ui_extra->frame =
		pfgGetNamedWidget(entries[num_entries - 1], "frame");

	/* file system */
	DsrSLEntryGetAttr(slentry,
		DsrSLAttrTaggedMountPointStr, &str,
		NULL);
	pfgSetWidgetString(entries[num_entries - 1], "fsValue", str);
	free(str);

	/* slice */
	pfgSetWidgetString(entries[num_entries - 1],
		"sliceValue", slentry->SliceName);

	/* current free space */
	DsrSLEntryGetAttr(slentry,
		DsrSLAttrFreeSpaceStr, &str,
		NULL);
	pfgSetWidgetString(entries[num_entries - 1], "currFreeSizeValue", str);
	free(str);

	/* space needed */
	DsrSLEntryGetAttr(slentry,
		DsrSLAttrSpaceReqdStr, &str,
		NULL);
	pfgSetWidgetString(entries[num_entries - 1], "spaceNeededValue", str);
	free(str);


	/* the option buttons */
	option_menu = pfgGetNamedWidget(entries[num_entries - 1],
		"ALOptionsMenu"),
	ui_extra->option_menu_widget =
		pfgGetNamedWidget(entries[num_entries - 1], "ALOptionsMenu");
	dsr_fsredit_make_option_button("fixedButton",
		SLFixed,
		ui_extra->frame,
		option_menu,
		slentry);
	dsr_fsredit_make_option_button("moveButton",
		SLMoveable,
		ui_extra->frame,
		option_menu,
		slentry);
	dsr_fsredit_make_option_button("changeButton",
		SLChangeable,
		ui_extra->frame,
		option_menu,
		slentry);
	dsr_fsredit_make_option_button("availableButton",
		SLAvailable,
		ui_extra->frame,
		option_menu,
		slentry);
	dsr_fsredit_make_option_button("collapseFSButton",
		SLCollapse,
		ui_extra->frame,
		option_menu,
		slentry);

	/* final size */
	if (SLEntryextra->history.final_size)
		(void) sprintf(buf, "%*s", UI_FS_SIZE_DISPLAY_LENGTH,
			SLEntryextra->history.final_size);
	else
		(void) sprintf(buf, "%*s", UI_FS_SIZE_DISPLAY_LENGTH, " ");
	pfgSetWidgetString(entries[num_entries - 1],
		"finalSizeTextField", buf);
	ui_extra->finalSize_widget =
		pfgGetNamedWidget(entries[num_entries - 1],
			"finalSizeTextField");
	XtVaSetValues(pfgGetNamedWidget(entries[num_entries - 1],
			"finalSizeTextField"),
		XmNuserData, (XtPointer) slentry,
		XmNcursorPosition, strlen(buf),
		NULL);

	/*
	 * The state of the option button affects some other settings -
	 * set those other things now.
	 */
	dsr_fsredist_set_option_attrs(slentry);

	XtVaSetValues(ui_extra->frame,
		XmNshadowType, XmSHADOW_ETCHED_IN,
		NULL);
}

static void
dsr_fsredist_display_slice_list(void)
{

	int		num_children;
	WidgetList	children;
	Widget		rc;
	TLink slcurrent;
	TSLEntry *slentry;
	TLLError err;
	int i;

	dsr_fsredist_manage_rc(False);

	/* get row column widget and free any old entries */
	rc = pfgGetNamedWidget(widget_list, "slicesRowColumn");
	XtVaGetValues(rc,
		XmNnumChildren, &num_children,
		XmNchildren, &children,
		NULL);
	if (num_children > 0) {
		for (i = 0; i < num_children; i++)
			XtDestroyWidget(children[i]);
	}

	if (get_trace_level() > 2) {
		DsrSLPrint(DsrSLHandle, DEBUG_LOC);
	}

	prev_entry_frame = NULL;

	/* display the current slice list */
	entries = NULL;
	num_entries = 0;
	LL_WALK(DsrSLHandle, slcurrent, slentry, err) {
		write_debug(GUI_DEBUG_L1_NOHD, "Slice Name = %s",
			slentry->SliceName);
		dsr_fsredist_display_slice(rc, slentry);
	}
	entries = (WidgetList *) xrealloc(entries,
		((num_entries + 1) * sizeof (WidgetList)));
	entries[num_entries] = NULL;

	/* align all the column widths */
	pfgSetMaxColumnWidths(widget_list,
		entries,
		row_column_labels,
		row_column_values,
		False, pfgAppData.dsrFSRedistColumnSpace);

	dsr_fsredist_manage_rc(True);
}

static void
set_tally_widths()
{
	Dimension add_width;
	Dimension all_width;
	Dimension max_width = 0;

	XtVaGetValues(pfgGetNamedWidget(widget_list, "additionalSpaceValue"),
		XmNwidth, &add_width,
		NULL);
	XtVaGetValues(pfgGetNamedWidget(widget_list, "allocatedSpaceValue"),
		XmNwidth, &all_width,
		NULL);
	max_width = MAX(add_width, all_width);

	XtVaSetValues(pfgGetNamedWidget(widget_list, "additionalSpaceValue"),
		XmNwidth, max_width,
		NULL);
	XtVaSetValues(pfgGetNamedWidget(widget_list, "allocatedSpaceValue"),
		XmNwidth, max_width,
		NULL);
}

static void
set_option_button_label(Widget button, TSLState state)
{
	char *str;

	str = DsrSLStateStr(state);
	xm_SetWidgetString(button, str);
}

/* ARGSUSED */
void
dsr_fsredist_option_cb(
	Widget button, XtPointer client_data, XtPointer call_data)
{
	TSLEntry *slentry;
	String widget_name;

	/* get the slice entry that this option button is associated with */
	XtVaGetValues(button,
		XmNuserData, &slentry,
		NULL);

	/* figure which option they have chosen */
	widget_name = XtName(button);
	if (streq(widget_name, "fixedButton")) {
		slentry->State = SLFixed;
	} else if (streq(widget_name, "moveButton")) {
		slentry->State = SLMoveable;
	} else if (streq(widget_name, "changeButton")) {
		slentry->State = SLChangeable;
	} else if (streq(widget_name, "availableButton")) {
		slentry->State = SLAvailable;
	}

	write_debug(GUI_DEBUG_L1, "set option %s for slice %s",
		widget_name, slentry->SliceName);

	dsr_fsredist_set_option_attrs(slentry);

	/*
	 * update the totals since the constraints just changed.
	 */
	dsr_fsredist_update_totals();
}

static void
dsr_fsredist_update_totals(void)
{
	DsrSLListExtraData *LLextra;
	pfgDsrSLListExtraData *ui_sl_extra;
	ulong add_space_req;
	ulong add_space_alloced;
	char buf[PATH_MAX];

	(void) LLGetSuppliedListData(DsrSLHandle, NULL, (TLLData *)&LLextra);
	ui_sl_extra = (pfgDsrSLListExtraData *)
		((DsrSLListExtraData *)LLextra)->extra;

	DsrSLGetSpaceSummary(DsrSLHandle, &add_space_req, &add_space_alloced);

	/* additional space required */
	(void) sprintf(buf, "%*lu",
		UI_FS_SIZE_DISPLAY_LENGTH,
		kb_to_mb(add_space_req));
	xm_SetWidgetString(ui_sl_extra->add_space_req, buf);

	/* additional space allocated */
	(void) sprintf(buf, "%*lu",
		UI_FS_SIZE_DISPLAY_LENGTH,
		kb_to_mb_trunc(add_space_alloced));
	xm_SetWidgetString(ui_sl_extra->add_space_alloced, buf);
}

static void
dsr_fsredist_set_option_attrs(TSLEntry *slentry)
{
	DsrSLEntryExtraData *SLEntryextra =
		(DsrSLEntryExtraData *)slentry->Extra;
	pfgDsrSLEntryExtraData *ui_extra =
		(pfgDsrSLEntryExtraData *)SLEntryextra->extra;

	/*
	 * set the sensitivity on the final size text widget
	 * depending on the value of the AL option.
	 */
	switch (slentry->State) {
	case SLFixed:
	case SLMoveable:
	case SLCollapse:
	case SLAvailable:
		XtVaSetValues(ui_extra->finalSize_widget,
			XmNsensitive, False,
			XmNcursorPositionVisible, False,
			NULL);

		/* do I clear this entry too? */
		break;
	case SLChangeable:
		XtVaSetValues(ui_extra->finalSize_widget,
			XmNsensitive, True,
			XmNcursorPositionVisible, True,
			NULL);
		break;
	}

	/*
	 * Also, the constraints option menu is insensitive
	 * if it's a failed slice (since the only option is
	 * changeable anyway.
	 *
	 * It's also insensitive if it's a collapsed file system since
	 * a collapseable file system cannot have it's state changed
	 * on this screen either - they have go to the collapse screen
	 * for that.  The menu button here is just for show (for
	 * informative purposes).
	 */
	if ((slentry->State == SLCollapse) ||
		(SL_SLICE_HAS_INSUFF_SPACE(slentry))) {
		XtSetSensitive(ui_extra->option_menu_widget, False);
	} else {
		XtSetSensitive(ui_extra->option_menu_widget, True);
	}
}

/* losing focus and activate callback */
/* ARGSUSED */
void
dsr_fsredist_finalsize_cb(
	Widget w, XtPointer client_data, XtPointer call_data)
{
	char *value;
	ulong final_size;
	ulong reqd_size;
	char *reqd_size_str;
	char *mount_point_str;
	char *buf;
	TSLEntry *slentry;
	DsrSLEntryExtraData *SLEntryextra;
	int error = FALSE;

	/*
	 * temporarily remove the final_size_cb while we're in so we
	 * don't get more of them...
	 */
	XtRemoveCallback(w, XmNactivateCallback,
		(XtCallbackProc)dsr_fsredist_finalsize_cb,
		(XtPointer)NULL);
#if 0
	XtRemoveCallback(w, XmNlosingFocusCallback,
		(XtCallbackProc)dsr_fsredist_finalsize_cb,
		(XtPointer)NULL);
#endif

	write_debug(GUI_DEBUG_L1, "Entering dsr_fsredist_finalsize_cb");

	XtVaGetValues(w,
		XmNvalue, &value,
		XmNuserData, &slentry,
		NULL);
	SLEntryextra = (DsrSLEntryExtraData *)slentry->Extra;

	if (SLEntryextra->history.final_size) {
		free(SLEntryextra->history.final_size);
	}
	SLEntryextra->history.final_size  = xstrdup(value);
	strip_whitespace(SLEntryextra->history.final_size);

	/* at least as big as required? */
	DsrSLEntryGetAttr(slentry,
		DsrSLAttrReqdSize, &reqd_size,
		DsrSLAttrReqdSizeStr, &reqd_size_str,
		DsrSLAttrMountPointStr, &mount_point_str,
		NULL);

	if ((!DsrSLValidFinalSize(value, &final_size)) ||
		((slentry->FSType != SLSwap) && (final_size < reqd_size))) {

		error = TRUE;

		/* add the initial message */
		buf = (char *) xmalloc(
			strlen(APP_ER_DSR_MSG_FINAL_TOO_SMALL) + 1);
		(void) strcpy(buf, APP_ER_DSR_MSG_FINAL_TOO_SMALL);

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

		pfAppError(TITLE_APP_ER_DSR_MSG_FINAL_TOO_SMALL, buf);
		free(buf);
	}
	free(reqd_size_str);
	free(mount_point_str);

	if (!error) {
		/*
		 * update the totals since a final size just changed
		 */
		dsr_fsredist_update_totals();
	}

	/*
	 * add the callback back on the size field
	 */
	XtAddCallback(w, XmNactivateCallback,
		(XtCallbackProc)dsr_fsredist_finalsize_cb,
		(XtPointer)NULL);

	write_debug(GUI_DEBUG_L1, "final size = %d", final_size);
}

static void
dsr_fsredist_store_final_sizes(void)
{
	TSLEntry *slentry;
	TLink slcurrent;
	TLLError err;
	DsrSLEntryExtraData *SLEntryextra;
	pfgDsrSLEntryExtraData *ui_extra;
	char *value;

	LL_WALK(DsrSLHandle, slcurrent, slentry, err) {
		/* save the editable ones */
		if (slentry->InVFSTab && slentry->State == SLChangeable) {
			SLEntryextra = (DsrSLEntryExtraData *)slentry->Extra;
			ui_extra = (pfgDsrSLEntryExtraData *)
				SLEntryextra->extra;

			XtVaGetValues(ui_extra->finalSize_widget,
				XmNvalue, &value,
				NULL);
			write_debug(GUI_DEBUG_L1, "Final size value = %s",
				value);

			if (SLEntryextra->history.final_size) {
				free(SLEntryextra->history.final_size);
			}
			SLEntryextra->history.final_size  = xstrdup(value);
			strip_whitespace(SLEntryextra->history.final_size);
		}
	}
}

/* ARGSUSED */
void
dsr_fsredist_continue_cb(
	Widget button, XtPointer client_data, XtPointer call_data)
{
	TSLEntry *slentry;
	TLink slcurrent;
	TLLError err;
	DsrSLEntryExtraData *SLEntryextra;
	DsrSLListExtraData *LLextra;
	ulong total_swap;
	int ret;
	char *buf;
	ulong reqd_size;
	char *reqd_size_str;
	char *mount_point_str;

	pfgBusy(pfgShell(dsr_fsredist_dialog));
	(void) LLGetSuppliedListData(DsrSLHandle, NULL, (TLLData *)&LLextra);
	/*
	 * First, make sure all the final sizes in the text fields
	 * are stored away
	 */
	dsr_fsredist_store_final_sizes();

	/*
	 * make sure the final size fields are ok
	 */
	buf = NULL;
	LL_WALK(DsrSLHandle, slcurrent, slentry, err) {
		if (slentry->InVFSTab && slentry->State == SLChangeable) {
			SLEntryextra = (DsrSLEntryExtraData *)slentry->Extra;

			DsrSLEntryGetAttr(slentry,
				DsrSLAttrReqdSize, &reqd_size,
				NULL);

			/*
			 * If there's an invalid integer in a final
			 * size field, or the final size is not big
			 * enough, throw up an error
			 * (handle reqd size checking on swap below)
			 */
			if ((!DsrSLValidFinalSize(
				SLEntryextra->history.final_size,
				&slentry->Size)) ||
				((slentry->FSType != SLSwap) &&
				(reqd_size > slentry->Size))) {

				if (!buf) {
					/* 1st one  - add the initial message */
					buf = (char *) xmalloc(
					strlen(APP_ER_DSR_MSG_FINAL_TOO_SMALL)
					+ 1);
					(void) strcpy(buf,
						APP_ER_DSR_MSG_FINAL_TOO_SMALL);
				}

				/* add the message itself */
				DsrSLEntryGetAttr(slentry,
					DsrSLAttrReqdSizeStr,
						&reqd_size_str,
					DsrSLAttrMountPointStr,
						&mount_point_str,
					NULL);
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

				free(reqd_size_str);
				free(mount_point_str);
			}

		}
	}

	if (buf) {
		pfAppError(TITLE_APP_ER_DSR_MSG_FINAL_TOO_SMALL, buf);
		free(buf);
		pfgUnbusy(pfgShell(dsr_fsredist_dialog));
		return;
	}

	/* make sure they have enough swap space allocated */
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
		pfAppError(NULL, buf);
		free(buf);
		pfgUnbusy(pfgShell(dsr_fsredist_dialog));
		return;
	}

	/*
	 * make sure to warn them about losing space in slices marked as
	 * available
	 */
	buf = NULL;
	LL_WALK(DsrSLHandle, slcurrent, slentry, err) {
		if (slentry->State != SLAvailable)
			continue;

		if (!buf) {
			/* 1st one  - add the initial message */
			buf = (char *) xmalloc(
				strlen(APP_ER_DSR_AVAILABLE_LOSE_DATA) + 1);
			(void) strcpy(buf, APP_ER_DSR_AVAILABLE_LOSE_DATA);
		}

		/* add the message itself */
		DsrSLEntryGetAttr(slentry,
			DsrSLAttrMountPointStr, &mount_point_str,
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
		if (UI_DisplayBasicMsg(UI_MSGTYPE_WARNING, NULL, buf) ==
			UI_MSGRESPONSE_CANCEL) {
			/* they cancelled */
			free(buf);
			pfgUnbusy(pfgShell(dsr_fsredist_dialog));
			return;
		}
		free(buf);
	}

	/* redo autolayout here */
	ret = DsrSLAutoLayout(DsrSLHandle, FsSpaceInfo, 0);
	if (ret == SUCCESS) {
		/* autolayout ok */

		/* free the teleuse widget list */
		free(widget_list);

		if (LLextra->extra)
			free(LLextra->extra);

		pfgSetAction(parAContinue);
	} else {
		/* autolayout not ok - stay on this screen */
		pfAppError(TITLE_APP_ER_CANT_AUTO_LAYOUT,
			APP_ER_DSR_AUTOLAYOUT_FAILED);
		pfgUnbusy(pfgShell(dsr_fsredist_dialog));
		return;
	}
}

/* ARGSUSED */
void
dsr_fsredist_reset_cb(
	Widget button, XtPointer client_data, XtPointer call_data)
{
	pfgBusy(pfgShell(dsr_fsredist_dialog));

	write_debug(GUI_DEBUG_L1, "Resetting DSR Redistribution defaults");

	if (get_trace_level() > 2) {
		DsrSLPrint(DsrSLHandle, DEBUG_LOC);
	}

	/* reset the defaults */
	DsrSLUIResetDefaults(DsrSLHandle, FsSpaceInfo, TRUE);

	if (get_trace_level() > 2) {
		DsrSLPrint(DsrSLHandle, DEBUG_LOC);
	}

	dsr_fsredist_display_slice_list();
	dsr_fsredist_update_totals();

	pfgUnbusy(pfgShell(dsr_fsredist_dialog));
}

/* ARGSUSED */
void
dsr_fsredist_goback_cb(
	Widget button, XtPointer client_data, XtPointer call_data)
{
	DsrSLListExtraData *LLextra;

	pfgBusy(pfgShell(dsr_fsredist_dialog));

	if (pfgAppQuery(dsr_fsredist_dialog, MSG_FSREDIST_GOBACK_LOSE_EDITS)
		== TRUE) {
		/*
		 * ok - lose the edits
		 *
		 * We need to find out if we can goback to the FS
		 * Summary screen or not.
		 * i.e. one got presented if the 'under the covers'
		 * autolayout was successful.
		 * If not, then we came here from the space req screen.
		 */
		DsrSLUIResetDefaults(DsrSLHandle, FsSpaceInfo, TRUE);
		(void)  DsrSLAutoLayout(DsrSLHandle, FsSpaceInfo, 1);
#if 0
		ret = DsrSLAutoLayout(DsrSLHandle, FsSpaceInfo, 1);
		if (ret == SUCCESS) {
			pfgSetAction(parADsrFSSumm);
		} else {
			pfgSetAction(parADsrSpaceReq);
		}
#else
		pfgSetAction(parAGoback);
#endif

		/* free the teleuse widget list */
		free(widget_list);
		(void) LLGetSuppliedListData(DsrSLHandle, NULL,
			(TLLData *)&LLextra);
		free(LLextra->extra);
		pfgUnbusy(pfgShell(dsr_fsredist_dialog));
	} else {
		/* cancel - don't lose the edits */
		pfgUnbusy(pfgShell(dsr_fsredist_dialog));
		return;
	}

	return;
}

/* ARGSUSED */
static void
DsrFSRedistEnterEntry(Widget w, XEvent * event,
	String * params, Cardinal *numParams)
{
	TSLEntry *slentry;
	DsrSLEntryExtraData *SLEntryextra;
	DsrSLListExtraData *LLextra;
	pfgDsrSLEntryExtraData *ui_extra;
	char *str;

	write_debug(GUI_DEBUG_L1, "Entering DsrFSRedistEnterEntry");
	write_debug(GUI_DEBUG_NOHD, LEVEL2, "Mount Point: %s",
		params[0]);
	write_debug(GUI_DEBUG_NOHD, LEVEL2, "Instance Number: %s",
		params[1]);

	slentry = DsrSLGetEntry(DsrSLHandle, params[0], atoi(params[1]));
	if (!slentry)
		return;

	/* set the fields for this slice that are above the list */
	/* slice */
	pfgSetWidgetString(widget_list, "activeSliceTextField",
		slentry->SliceName);

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
	pfgSetWidgetString(widget_list, "activeReqSizeTextField", str);
	free(str);

	/* current size */
	DsrSLEntryGetAttr(slentry,
		DsrSLAttrExistingSizeStr, &str,
		NULL);
	pfgSetWidgetString(widget_list, "activeCurrSizeTextField", str);
	free(str);

	SLEntryextra = (DsrSLEntryExtraData *)slentry->Extra;
	if (!SLEntryextra)
		return;

	ui_extra = (pfgDsrSLEntryExtraData *)SLEntryextra->extra;
	if (!ui_extra)
		return;

	if (prev_entry_frame) {
		XtVaSetValues(prev_entry_frame,
/* 			XmNborderWidth, 1, */
			XmNshadowType, XmSHADOW_ETCHED_IN,
			NULL);
	}

	XtVaSetValues(ui_extra->frame,
/* 		XmNborderWidth, 2, */
		XmNshadowType, XmSHADOW_ETCHED_OUT,
		NULL);

	prev_entry_frame = ui_extra->frame;
}

static void
dsr_fsredist_manage_rc(Boolean manage)
{

	Widget rc;

	rc = pfgGetNamedWidget(widget_list, "slicesRowColumn");

	if (manage) {
		XtManageChild(rc);
	} else {
		XtUnmanageChild(rc);
	}
}

/*
 * *************************************************************************
 *	Filtering related code...
 * *************************************************************************
 */

/* ARGSUSED */
void
dsr_fsredist_filter_button_cb(
	Widget button, XtPointer client_data, XtPointer call_data)
{
	write_debug(GUI_DEBUG_L1, "Entering filter button callback...");

	pfgCreateFilterDialog();
}

void
pfgCreateFilterDialog()
{
	Widget toggle;
	DsrSLListExtraData *LLextra;
	char *buf;

	/* get list level user data */
	(void) LLGetSuppliedListData(DsrSLHandle, NULL, (TLLData *)&LLextra);

	/* the parent is busy while we're in here */
	pfgBusy(pfgShell(dsr_fsredist_dialog));

	/* get the dialog widget & the dialog widget list from teleuse */
	dsr_fsredist_filter_dialog = tu_dsr_fsredist_filter_dialog_widget(
		"dsr_fsredist_filter_dialog", pfgTopLevel,
		&filter_dialog_widget_list);

	/* set up exit callback off window manager close */
	XmAddWMProtocolCallback(pfgShell(dsr_fsredist_filter_dialog),
		pfgWMDeleteAtom,
		(XtCallbackProc) pfgExit, NULL);

	/* set title */
	XtVaSetValues(pfgShell(dsr_fsredist_filter_dialog),
		XtNtitle, TITLE_DSR_FILTER,
		XmNdeleteResponse, XmDO_NOTHING,
		NULL);

	pfgSetStandardButtonStrings(filter_dialog_widget_list,
		ButtonOk, ButtonCancel, ButtonHelp, NULL);

	/* set most labels on the screen */
	pfgSetWidgetString(filter_dialog_widget_list, "radioLabel",
		LABEL_DSR_FSREDIST_FILTER_RADIO);
	buf = (char *) xmalloc (strlen(MSG_GUI_DSR_FILTER) +
		strlen(LABEL_DSR_FSREDIST_FILTER_SLICE) +
		strlen(LABEL_DSR_FSREDIST_FILTER_MNTPNT) + 1);
	(void) sprintf(buf, MSG_GUI_DSR_FILTER,
		LABEL_DSR_FSREDIST_FILTER_SLICE,
		LABEL_DSR_FSREDIST_FILTER_MNTPNT);
	pfgSetWidgetString(filter_dialog_widget_list, "panelhelpText", buf);
	free(buf);
	pfgSetWidgetString(filter_dialog_widget_list, "allToggle",
		LABEL_DSR_FSREDIST_FILTER_ALL);
	pfgSetWidgetString(filter_dialog_widget_list, "failedFSToggle",
		LABEL_DSR_FSREDIST_FILTER_FAILED);
	pfgSetWidgetString(filter_dialog_widget_list, "vfstabSlicesToggle",
		LABEL_DSR_FSREDIST_FILTER_VFSTAB);
	pfgSetWidgetString(filter_dialog_widget_list, "nonVfstabSlicesToggle",
		LABEL_DSR_FSREDIST_FILTER_NONVFSTAB);
	pfgSetWidgetString(filter_dialog_widget_list, "sliceNameToggle",
		LABEL_DSR_FSREDIST_FILTER_SLICE);
	pfgSetWidgetString(filter_dialog_widget_list, "mntpntNameToggle",
		LABEL_DSR_FSREDIST_FILTER_MNTPNT);

	/* filter search string label */
	buf = (char *) xmalloc(strlen(LABEL_DSR_FSREDIST_FILTER_RETEXT) +
		strlen(LABEL_DSR_FSREDIST_FILTER_RE_EG) + 1);
	(void) sprintf(buf, LABEL_DSR_FSREDIST_FILTER_RETEXT,
		LABEL_DSR_FSREDIST_FILTER_RE_EG);
	pfgSetWidgetString(filter_dialog_widget_list, "reLabel", buf);
	free(buf);

	/* set user data of filter type on each toggle */
	XtVaSetValues(pfgGetNamedWidget(filter_dialog_widget_list,
		"allToggle"),
		XmNuserData, (XtPointer) SLFilterAll,
		NULL);
	XtVaSetValues(pfgGetNamedWidget(filter_dialog_widget_list,
		"failedFSToggle"),
		XmNuserData, (XtPointer) SLFilterFailed,
		NULL);
	XtVaSetValues(pfgGetNamedWidget(filter_dialog_widget_list,
		"vfstabSlicesToggle"),
		XmNuserData, (XtPointer) SLFilterVfstabSlices,
		NULL);
	XtVaSetValues(pfgGetNamedWidget(filter_dialog_widget_list,
		"nonVfstabSlicesToggle"),
		XmNuserData, (XtPointer) SLFilterNonVfstabSlices,
		NULL);
	XtVaSetValues(pfgGetNamedWidget(filter_dialog_widget_list,
		"sliceNameToggle"),
		XmNuserData, (XtPointer) SLFilterSliceNameSearch,
		NULL);
	XtVaSetValues(pfgGetNamedWidget(filter_dialog_widget_list,
		"mntpntNameToggle"),
		XmNuserData, (XtPointer) SLFilterMountPntNameSearch,
		NULL);

	/* set the default toggle */
	switch (LLextra->history.filter_type) {
	case SLFilterAll:
		toggle = pfgGetNamedWidget(filter_dialog_widget_list,
			"allToggle");
		break;
	case SLFilterFailed:
		toggle = pfgGetNamedWidget(filter_dialog_widget_list,
			"failedFSToggle");
		break;
	case SLFilterVfstabSlices:
		toggle = pfgGetNamedWidget(filter_dialog_widget_list,
			"vfstabSlicesToggle");
		break;
	case SLFilterNonVfstabSlices:
		toggle = pfgGetNamedWidget(filter_dialog_widget_list,
			"nonVfstabSlicesToggle");
		break;
	case SLFilterSliceNameSearch:
		toggle = pfgGetNamedWidget(filter_dialog_widget_list,
			"sliceNameToggle");
		break;
	case SLFilterMountPntNameSearch:
		toggle = pfgGetNamedWidget(filter_dialog_widget_list,
			"mntpntNameToggle");
		break;
	}
	XtVaSetValues(pfgGetNamedWidget(filter_dialog_widget_list,
			"radioBox"),
		XmNmenuHistory, toggle,
		NULL);
	XtVaSetValues(toggle,
		XmNset, True,
		NULL);

	/*
	 * set the re text field with the last value they had in there
	 * and set the cursor to the end of the field
	 */
	pfgSetWidgetString(filter_dialog_widget_list, "reTextField",
		LLextra->history.filter_pattern ?
			LLextra->history.filter_pattern : "");
	if (LLextra->history.filter_pattern) {

	XtVaSetValues(
		pfgGetNamedWidget(filter_dialog_widget_list, "reTextField"),
		XmNcursorPosition, strlen(LLextra->history.filter_pattern),
		NULL);
	}

	dsr_fsredist_filter_toggle_set_attrs(LLextra->history.filter_type);

	xm_SetNoResize(pfgTopLevel, dsr_fsredist_filter_dialog);
	XtManageChild(dsr_fsredist_filter_dialog);
}

/* ARGSUSED */
void
dsr_fsredist_filter_toggle_value_cb(
	Widget toggle, XtPointer client_data, XtPointer call_data)
{
	XmToggleButtonCallbackStruct *cb_data =
		/* LINTED [pointer cast] */
		(XmToggleButtonCallbackStruct *)call_data;
	TSLFilter filter_type;

	/* I'm only interested in the one that just got set */
	if (!cb_data->set)
		return;

	/* figure out which filter just got set */
	XtVaGetValues(toggle,
		XmNuserData, &filter_type,
		NULL);

	write_debug(GUI_DEBUG_L1, "Filter toggle %d selected",
		filter_type);

	/*
	 * make any relevant screen adjustments based on the new filter type
	 */
	dsr_fsredist_filter_toggle_set_attrs(filter_type);
}

static void
dsr_fsredist_filter_toggle_set_attrs(TSLFilter filter_type)
{
	Widget textw;
	Widget labelw;

	textw = pfgGetNamedWidget(filter_dialog_widget_list, "reTextField");
	labelw = pfgGetNamedWidget(filter_dialog_widget_list, "reLabel");
	switch (filter_type) {
	case SLFilterAll:
	case SLFilterFailed:
	case SLFilterVfstabSlices:
	case SLFilterNonVfstabSlices:
		XtSetSensitive(textw, False);
		XtSetSensitive(labelw, False);
		break;
	case SLFilterSliceNameSearch:
	case SLFilterMountPntNameSearch:
		XtSetSensitive(textw, True);
		XtSetSensitive(labelw, True);
		break;
	}
}

/* note that a \n in re textfield brings us here also */
/* ARGSUSED */
void
dsr_fsredist_filter_ok_cb(
	Widget button, XtPointer client_data, XtPointer call_data)
{
	Widget radioBox;
	Widget toggle;
	TSLFilter filter_type;
	char *retextstr;
	int match_cnt;
	char *buf;
	DsrSLListExtraData *LLextra;

	write_debug(GUI_DEBUG_L1, "Entering filter dialog ok callback");

	pfgBusy(pfgShell(dsr_fsredist_dialog));

	/* get list level user data */
	(void) LLGetSuppliedListData(DsrSLHandle, NULL, (TLLData *)&LLextra);

	radioBox = pfgGetNamedWidget(filter_dialog_widget_list, "radioBox");
	XtVaGetValues(radioBox,
		XmNmenuHistory, &toggle,
		NULL);

	XtVaGetValues(toggle,
		XmNuserData, &filter_type,
		NULL);
	write_debug(GUI_DEBUG_L1, "Filter toggle %d set in ok cb",
		filter_type);

	switch (filter_type) {
	case SLFilterAll:
	case SLFilterFailed:
	case SLFilterVfstabSlices:
	case SLFilterNonVfstabSlices:
		retextstr = NULL;
		break;
	case SLFilterSliceNameSearch:
	case SLFilterMountPntNameSearch:
		XtVaGetValues(pfgGetNamedWidget(filter_dialog_widget_list,
			"reTextField"),
			XmNvalue, &retextstr,
			NULL);
		if (!retextstr || !strlen(retextstr)) {
			pfAppError(NULL, APP_ER_DSR_RE_MISSING);
			return;
		}
		break;
	}
	LLextra->history.filter_type = filter_type;
	if (LLextra->history.filter_pattern)
		free(LLextra->history.filter_pattern);
	LLextra->history.filter_pattern =
		retextstr ? xstrdup(retextstr) : NULL;

	/*
	 * filter the list
	 * warn and don't proceed if there's an RE compile error
	 * or if there are no matches for this filter
	 */
	if (DsrSLFilter(DsrSLHandle, &match_cnt)
		== FAILURE) {
		/* only get here if recomp failed */
		buf = (char *) xmalloc(strlen(APP_ER_DSR_RE_COMPFAIL) +
			strlen(retextstr) + 1);
		(void) sprintf(buf, APP_ER_DSR_RE_COMPFAIL, retextstr);
		pfAppError(NULL, APP_ER_DSR_RE_COMPFAIL);
		return;
	}
	if (match_cnt == 0) {
		pfAppError(NULL, APP_ER_DSR_FILTER_NOMATCH);
		return;
	}

	pfgBusy(pfgShell(dsr_fsredist_filter_dialog));

	/* free the teleuse widget list */
	free(filter_dialog_widget_list);
	pfgDestroyDialog(dsr_fsredist_dialog, dsr_fsredist_filter_dialog);

	/* redraw the slice list */
	dsr_fsredist_display_slice_list();
	pfgUnbusy(pfgShell(dsr_fsredist_dialog));
}

/* ARGSUSED */
void
dsr_fsredist_filter_cancel_cb(
	Widget button, XtPointer client_data, XtPointer call_data)
{
	write_debug(GUI_DEBUG_L1, "Filter toggle selected...");

	/* free the teleuse widget list */
	free(filter_dialog_widget_list);
	pfgDestroyDialog(dsr_fsredist_dialog, dsr_fsredist_filter_dialog);
}

/*
 * *************************************************************************
 *	Collapse file systems related code.
 * *************************************************************************
 */

/* ARGSUSED */
void
dsr_fsredist_collapse_cb(
	Widget button, XtPointer client_data, XtPointer call_data)
{
	write_debug(GUI_DEBUG_L1, "Entering collapsing file systems...");

	pfgCreateCollapseDialog();

}

static void
pfgCreateCollapseDialog()
{
	/* the parent is busy while we're in here */
	pfgBusy(pfgShell(dsr_fsredist_dialog));

	/* get the dialog widget & the dialog widget list from teleuse */
	dsr_fsredist_collapse_dialog =
		tu_dsr_fsredist_collapse_dialog_widget(
			"dsr_fsredist_collapse_dialog", pfgTopLevel,
			&collapse_dialog_widget_list);

	/* set up exit callback off window manager close */
	XmAddWMProtocolCallback(pfgShell(dsr_fsredist_collapse_dialog),
		pfgWMDeleteAtom,
		(XtCallbackProc) pfgExit, NULL);

	/* set title */
	XtVaSetValues(pfgShell(dsr_fsredist_collapse_dialog),
		XtNtitle, TITLE_DSR_FS_COLLAPSE,
		XmNdeleteResponse, XmDO_NOTHING,
		NULL);

	pfgSetStandardButtonStrings(collapse_dialog_widget_list,
		ButtonOk, ButtonCancel, ButtonHelp, NULL);

	/* set most labels on the screen */
	pfgSetWidgetString(collapse_dialog_widget_list, "panelhelpText",
		MSG_DSR_FS_COLLAPSE);
	pfgSetWidgetString(collapse_dialog_widget_list, "fsColumnLabel",
		LABEL_DSR_FS_COLLAPSE_FS);
	pfgSetWidgetString(collapse_dialog_widget_list, "parentColumnLabel",
		LABEL_DSR_FS_COLLAPSE_PARENT);

	/* make all column labels the same height */
	pfgSetMaxWidgetHeights(collapse_dialog_widget_list,
		collapse_row_column_labels);

	/* create the file system entries */
	dsr_fsredist_collapse_create_entries();

	xm_SetNoResize(pfgTopLevel, dsr_fsredist_collapse_dialog);
	XtManageChild(dsr_fsredist_collapse_dialog);
}

/*
 * create the set of vfstab file systems checkboxes for the user
 * to select to collapse/uncollapse.
 */
static void
dsr_fsredist_collapse_create_entries(void)
{
	TSLEntry *slentry;
	TLink slcurrent;
	TLLError err;
	XmString toggle_label;
	XmString parent_label;
	int num_fs;
	int i;
	Widget rc;

	/*
	 * get a list of FS's to display here that are sorted
	 * alphabetically.
	 */
	SLSort(DsrSLHandle, SLMountPointAscending);

	/* how many file systems will we display? */
	num_fs = DsrSLGetNumCollapseable(DsrSLHandle);

	collapse_fs = (Widget *) xmalloc((num_fs + 1) * sizeof (Widget));
	collapse_parent = (Widget *) xmalloc((num_fs + 1) * sizeof (Widget));

	/* get row column widget */
	rc = pfgGetNamedWidget(collapse_dialog_widget_list,
		"collapseRowColumn");

	/* create the file system checkboxes from the sorted list */
	i = 0;
	entries = NULL;
	num_entries = 0;
	LL_WALK(DsrSLHandle, slcurrent, slentry, err) {
		if (!SL_SLICE_IS_COLLAPSEABLE(slentry))
			continue;

		/* now we have something collapseable... */
		num_entries++;
		entries = (WidgetList *) xrealloc(entries,
			(num_entries * sizeof (WidgetList)));
		(void) tu_dsr_fsredist_collapse_entry_widget(
			"dsr_fsredist_collapse_entry", rc,
			&entries[num_entries - 1]);

		/* display the toggle values */
		toggle_label =
			XmStringCreateLocalized(slentry->MountPoint);
		collapse_fs[i] =
			pfgGetNamedWidget(entries[num_entries - 1], "fsValue");
		XtVaSetValues(collapse_fs[i],
			XmNset, (slentry->State != SLCollapse),
			XmNlabelString, toggle_label,
			XmNuserData, slentry,
			NULL);
		XmStringFree(toggle_label);

		/* display the toggle parent values */
		collapse_parent[i] =
			pfgGetNamedWidget(entries[num_entries - 1],
				"parentValue");
		parent_label =
			XmStringCreateLocalized(
				DsrSLGetParentFS(FsSpaceInfo,
					slentry->MountPoint));
		XtVaSetValues(collapse_parent[i],
			XmNlabelString, parent_label,
			NULL);
		XmStringFree(parent_label);

		/* make all row values the same height */
		pfgSetMaxWidgetHeights(collapse_dialog_widget_list,
			collapse_row_column_values);

		i++;
	}
	collapse_fs[i] = NULL;
	collapse_parent[i] = NULL;
	entries = (WidgetList *) xrealloc(entries,
		((num_entries + 1) * sizeof (WidgetList)));
	entries[num_entries] = NULL;

	/*
	 * one more pass on the value labels to make them all the same
	 * width
	 */
	pfgSetMaxColumnWidths(collapse_dialog_widget_list,
		entries,
		collapse_row_column_labels,
		collapse_row_column_values,
		False, pfgAppData.dsrFSCollapseColumnSpace);

	/* sort the original list back to normal */
	SLSort(DsrSLHandle, SLSliceNameAscending);
}

/* ARGSUSED */
void
dsr_fsredist_collapse_toggle_value_cb(
	Widget toggle, XtPointer client_data, XtPointer call_data)
{
	XmToggleButtonCallbackStruct *cb_data =
		/* LINTED [pointer cast] */
		(XmToggleButtonCallbackStruct *)call_data;

	/* recalc the parents based on the current settings */
	dsr_fsredist_collapse_set_parents(toggle, cb_data->set);

}

static void
dsr_fsredist_collapse_set_parents(Widget toggle, Boolean set)
{
	TSLEntry *slentry;
	TSLEntry *slentry_reparent;
	int i;
	XmString parent_label;

	XtVaGetValues(toggle,
		XmNuserData, &slentry,
		NULL);

	if (set) {
		slentry->Space->fsp_flags &= ~FS_IGNORE_ENTRY;
	} else {
		slentry->Space->fsp_flags |= FS_IGNORE_ENTRY;
	}

	for (i = 0; collapse_fs[i]; i++) {
		XtVaGetValues(collapse_fs[i],
			XmNuserData, &slentry_reparent,
			NULL);
		parent_label =
			XmStringCreateLocalized(
				DsrSLGetParentFS(FsSpaceInfo,
					slentry_reparent->MountPoint));
		XtVaSetValues(collapse_parent[i],
			XmNlabelString, parent_label,
			NULL);
		XmStringFree(parent_label);
	}
}

/*
 * OK callback for the collapse dialog
 */
/* ARGSUSED */
void
dsr_fsredist_collapse_ok_cb(
	Widget button, XtPointer client_data, XtPointer call_data)
{

	int i;
	TSLEntry *slentry;
	Boolean	set;
	int changed_state;

	pfgBusy(pfgShell(dsr_fsredist_collapse_dialog));

	/*
	 * figure out if any settings have actually changed...
	 */
	write_debug(GUI_DEBUG_L1, "Collapsed state check:");
	for (i = 0; collapse_fs[i]; i++) {
		XtVaGetValues(collapse_fs[i],
			XmNuserData, &slentry,
			XmNset, &set,
			NULL);
		if (set) {
			if (slentry->State == SLCollapse) {
				changed_state = TRUE;
			} else {
				changed_state = FALSE;
			}
		} else {
			if (slentry->State == SLCollapse) {
				changed_state = FALSE;
			} else {
				changed_state = TRUE;
			}
		}

		write_debug(GUI_DEBUG_L1_NOHD,
			changed_state ?
				"   CHANGE: set = %d\tstate = %s" :
				"NO CHANGE: set = %d\tstate = %s",
			set,
			DsrSLStateStr(slentry->State));

		/*
		 * we can bail this loop as soon as we know anything
		 * has changed
		 */
		if (changed_state)
			break;
	}

	/*
	 * nothing to do if nothing's been collapsed or uncollapsed
	 * just go ahead and exit
	 */
	if (changed_state == FALSE) {
		dsr_fsredist_collapse_exit_dialog();
		return;
	}

	/*
	 * Ok - if we get here then we know that settings have changed:
	 * 	- warn the user
	 *	- do space recalc
	 */
	if (UI_DisplayBasicMsg(UI_MSGTYPE_WARNING, TITLE_WARNING,
		LABEL_DSR_FS_COLLAPSE_CHANGED) == UI_MSGRESPONSE_CANCEL) {
		/*
		 * user chose cancel from the warning -
		 * abort normal OK processing and drop back to the
		 * collapse dialog.
		 */
		pfgUnbusy(pfgShell(dsr_fsredist_collapse_dialog));
		return;
	}

	/*
	 * update the FSspace flags correctly to indicate to the
	 * space checking logic which file systems are and are not
	 * collapsed.
	 */
	for (i = 0; collapse_fs[i]; i++) {
		XtVaGetValues(collapse_fs[i],
			XmNuserData, &slentry,
			XmNset, &set,
			NULL);
		if (set) {
			/* toggle set ==> not collapsed */
			slentry->Space->fsp_flags &= ~FS_IGNORE_ENTRY;
		} else {
			/* toggle not set ==> collapsed */
			slentry->Space->fsp_flags |= FS_IGNORE_ENTRY;
		}
		DsrSLEntrySetDefaults(slentry);
		DsrSLUIEntrySetDefaults(slentry);
	}

	/*
	 * debug...
	 * print out all ignored entries.
	 */
	for (i = 0; FsSpaceInfo[i]; i++) {
		if (FsSpaceInfo[i]->fsp_flags & FS_IGNORE_ENTRY) {
			write_debug(GUI_DEBUG_L1,
				"Collapsed file system: %s (%s)",
				FsSpaceInfo[i]->fsp_mntpnt,
				FsSpaceInfo[i]->fsp_fsi->fsi_device);
		}
	}

	dsr_fsredist_collapse_exit_dialog();

	pfgBusy(pfgShell(dsr_fsredist_dialog));
	if (verify_fs_layout(FsSpaceInfo, dsr_fsredist_verify_cb, NULL)
		== SP_ERR_NOT_ENOUGH_SPACE) {
		/*
		 * throw up the space dialog since some file systems
		 * still fail.
		 * surround this dialog with it's own event loop so
		 * we don't proceed until they've acknowledged it...
		 */
		(void) pfgCreateDsrSpaceReq(FALSE);
		(void) pfgEventLoop();
		pfgSetAction(parANone);
	} else {
		pfAppInfo(NULL, APP_DSR_COLLAPSE_SPACE_OK);
	}

	/* make sure the parent window gets updated with the new values */

	/* reset the defaults and redisplay them */
	DsrSLUIResetDefaults(DsrSLHandle, FsSpaceInfo, FALSE);
	dsr_fsredist_display_slice_list();

	/*
	 * update the totals since the constraints just changed.
	 */
	dsr_fsredist_update_totals();

	pfgUnbusy(pfgShell(dsr_fsredist_dialog));
}

/*
 * Progress routine on verify callback called from collapse dialog.
 * Called only to get display updated - no progress bar actually.
 */
/* ARGSUSED */
static int
dsr_fsredist_verify_cb(void *mydata, void *cb_data)
{
	xm_ForceDisplayUpdate(pfgTopLevel, dsr_fsredist_collapse_dialog);

	return (SUCCESS);
}


/*
 * Cancel callback for the collapse dialog
 */
/* ARGSUSED */
void
dsr_fsredist_collapse_cancel_cb(
	Widget button, XtPointer client_data, XtPointer call_data)
{
	(void) DsrSLResetSpaceIgnoreEntries(DsrSLHandle);

	dsr_fsredist_collapse_exit_dialog();
}

/*
 * Exit the collapse dialog cleanly.
 */
static void
dsr_fsredist_collapse_exit_dialog(void)
{
	/* free the array of collapse toggle widgets */
	free(collapse_fs);

	/* free the teleuse widget list */
	free(collapse_dialog_widget_list);
	pfgDestroyDialog(dsr_fsredist_dialog, dsr_fsredist_collapse_dialog);

	pfgUnbusy(pfgShell(dsr_fsredist_dialog));
}
