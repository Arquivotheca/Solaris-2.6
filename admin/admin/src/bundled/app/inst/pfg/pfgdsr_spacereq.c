#ifndef lint
#pragma ident "@(#)pfgdsr_spacereq.c 1.6 96/07/30 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	pfgdsr_spacereq.c
 * Group:	installtool
 * Description:
 */

#include <libgen.h>

#include "pfg.h"
#include "pfgDSRSpaceReq_ui.h"

void dsr_space_req_continue_cb(
	Widget button, XtPointer client_data, XtPointer call_data);
static void dsr_space_req_ok_cb(
	Widget button, XtPointer client_data, XtPointer call_data);
static void dsr_space_req_summary(FSspace **fs_space);
static WidgetList widget_list;

static Widget dsr_space_req_dialog;
static char *column_labels[] = {
	"fsColumnLabel",
	"sliceColumnLabel",
	"currSizeColumnLabel",
	"reqSizeColumnLabel",
	NULL
};

static char *row_column_values[] = {
	"fsValue",
	"sliceValue",
	"currSizeValue",
	"reqSizeValue",
	NULL
};

Widget
pfgCreateDsrSpaceReq(int main_parade)
{
	Dimension width, height;
	char *msg;

	/* get the dialog widget & the dialog widget list from teleuse */
	dsr_space_req_dialog = tu_dsr_space_req_dialog_widget(
		"dsr_space_req_dialog", pfgTopLevel, &widget_list);

	/* set up exit callback off window manager close */
	XmAddWMProtocolCallback(pfgShell(dsr_space_req_dialog), pfgWMDeleteAtom,
		(XtCallbackProc) pfgExit, NULL);

	if (main_parade) {
		msg = MSG_DSR_SPACE_REQ;
	} else {
		msg = MSG_DSR_SPACE_REQ_FS_COLLAPSE;
	}

	/* set title */
	XtVaSetValues(pfgShell(dsr_space_req_dialog),
		XtNtitle, TITLE_DSR_SPACE_REQ,
		XmNdeleteResponse, XmDO_NOTHING,
		NULL);

	pfgSetWidgetString(widget_list, "panelhelpText", msg);
	pfgSetWidgetString(widget_list, "fsColumnLabel",
		LABEL_FILE_SYSTEM);
	pfgSetWidgetString(widget_list, "sliceColumnLabel",
		LABEL_SLICE);
	pfgSetWidgetString(widget_list, "currSizeColumnLabel",
		LABEL_DSR_SPACE_REQ_CURRSIZE);
	pfgSetWidgetString(widget_list, "reqSizeColumnLabel",
		LABEL_DSR_SPACE_REQ_REQSIZE);

	/* button labels */
	if (main_parade) {
		pfgSetWidgetString(widget_list, "continueButton",
			LABEL_AUTOLAYOUT);
		pfgSetStandardButtonStrings(widget_list,
			ButtonGoback, ButtonExit, ButtonHelp, NULL);
	} else {
		/*
		 * This is not in the main parade -
		 * it's a dialog with just an ok button
		 * Use the continueButton for ok.
		 */
		XtUnmanageChild(pfgGetNamedWidget(widget_list, "gobackButton"));
		XtUnmanageChild(pfgGetNamedWidget(widget_list, "exitButton"));
		XtUnmanageChild(pfgGetNamedWidget(widget_list, "helpButton"));
		pfgSetWidgetString(widget_list, "continueButton",
			UI_BUTTON_OK_STR);
		XtRemoveCallback(
			pfgGetNamedWidget(widget_list, "continueButton"),
			XmNactivateCallback,
			(XtCallbackProc) dsr_space_req_continue_cb,
			(XtPointer) NULL);
		XtAddCallback(pfgGetNamedWidget(widget_list, "continueButton"),
			XmNactivateCallback,
			(XtCallbackProc) dsr_space_req_ok_cb,
			(XtPointer) NULL);
	}

	pfgSetMaxWidgetHeights(widget_list, column_labels);

	dsr_space_req_summary(FsSpaceInfo);

	XtManageChild(dsr_space_req_dialog);

	XtVaGetValues(pfgShell(dsr_space_req_dialog),
	    XmNwidth, &width,
	    XmNheight, &height,
	    NULL);

	XtVaSetValues(pfgShell(dsr_space_req_dialog),
	    XmNminWidth, width,
	    XmNmaxWidth, width,
	    XmNminHeight, height,
	    NULL);

	(void) XmProcessTraversal(
		pfgGetNamedWidget(widget_list, "continueButton"),
		XmTRAVERSE_CURRENT);

	return (dsr_space_req_dialog);
}

/*
 * routine to fill in the failed file system list
 */
static void
dsr_space_req_summary(FSspace **fs_space)
{
	int 		i;
	char 		buf[100];
	int		child;
	int		num_children;
	WidgetList	children;
	Widget		rc;
	WidgetList *entries = NULL;
	int num_entries;

	write_debug(GUI_DEBUG_L1, "Entering dsr_space_req_summary");

	rc = pfgGetNamedWidget(widget_list, "spacereqRowColumn");

	XtVaGetValues(rc,
		XmNnumChildren, &num_children,
		XmNchildren, &children,
		NULL);

	if (num_children > 0) {
		for (child = 0; child < num_children; child++)
			XtDestroyWidget(children[child]);
	}

	num_entries = 0;
	for (i = 0; fs_space && fs_space[i]; i++) {
		/*
		 * If it's a slice that's been marked as having
		 * insufficient space AND it hasn't been marked as collapsed.
		 */
		if ((fs_space[i]->fsp_flags & FS_INSUFFICIENT_SPACE) &&
			!(fs_space[i]->fsp_flags & FS_IGNORE_ENTRY)) {
			write_debug(GUI_DEBUG_L1, "%s failed",
				fs_space[i]->fsp_mntpnt);
			write_debug(GUI_DEBUG_L1_NOHD, "reqd size (kb): %lu",
				fs_space[i]->fsp_reqd_slice_size);
			write_debug(GUI_DEBUG_L1_NOHD, "current size (kb): %lu",
				fs_space[i]->fsp_cur_slice_size);

			/* create teleuse entry widget list */
			num_entries++;
			entries = (WidgetList *) xrealloc(entries,
				(num_entries * sizeof (WidgetList)));
			(void) tu_dsr_space_req_filesys_entry_widget(
				"dsr_space_req_filesys_entry", rc,
				&entries[num_entries - 1]);

			/* file system */
			(void) sprintf(buf, "%-*.*s",
				UI_FS_DISPLAY_LENGTH,
				UI_FS_DISPLAY_LENGTH,
				fs_space[i]->fsp_mntpnt);
			pfgSetWidgetString(entries[num_entries - 1],
				"fsValue", buf);

			/* disk slice for this file system */
			(void) sprintf(buf, "%s",
				basename(fs_space[i]->fsp_fsi->fsi_device));
			pfgSetWidgetString(entries[num_entries - 1],
				"sliceValue", buf);

			/* current size */
			(void) sprintf(buf, "%5lu",
				kb_to_mb_trunc(
					fs_space[i]->fsp_cur_slice_size));
			pfgSetWidgetString(entries[num_entries - 1],
				"currSizeValue", buf);

			/* required size */
			(void) sprintf(buf, "%5lu",
				kb_to_mb(fs_space[i]->fsp_reqd_slice_size));
			pfgSetWidgetString(entries[num_entries - 1],
				"reqSizeValue", buf);

		}
	}

	entries = (WidgetList *) xrealloc(entries,
		((num_entries + 1) * sizeof (WidgetList)));
	entries[num_entries] = NULL;

	/* set label widths */
	pfgSetMaxColumnWidths(widget_list,
		entries,
		column_labels,
		row_column_values,
		False,
		pfgAppData.dsrSpaceReqColumnSpace);
}

/* Ok button callback */
/* ARGSUSED */
void
dsr_space_req_continue_cb(
	Widget button, XtPointer client_data, XtPointer call_data)
{
	int ret;
	UI_MsgStruct *msg_info;

	/* try to run autolayout and automatically relayout stuff so it fits */
	pfgBusy(pfgShell(dsr_space_req_dialog));
	ret = DsrSLAutoLayout(DsrSLHandle, FsSpaceInfo, 1);
	if (ret == SUCCESS) {
		pfgSetAction(parADsrFSSumm);
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
		pfgSetAction(parADsrFSRedist);
	}

	/* free the teleuse widget list */
	free(widget_list);
}

/* ARGSUSED */
void
dsr_space_req_goback_cb(
	Widget button, XtPointer client_data, XtPointer call_data)
{
	/* free the teleuse widget list */
	free(widget_list);

	pfgSetAction(parAGoback);
	return;
}

/* ARGSUSED */
static void
dsr_space_req_ok_cb(
	Widget button, XtPointer client_data, XtPointer call_data)
{
	/* free the teleuse widget list */
	free(widget_list);

	/* destroy the dialog */
	XtUnmanageChild(dsr_space_req_dialog);
	XtDestroyWidget(dsr_space_req_dialog);

	pfgSetAction(parAContinue);

	return;
}
