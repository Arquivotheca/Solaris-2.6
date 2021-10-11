/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma	ident	"@(#)fsb.c	1.11	96/08/02 SMI"


#include <X11/Intrinsic.h>
#include <Xm/FileSB.h>

#include "util.h"
#include "launcher.h"

Widget fileSelectionDialog;

Widget build_fileSelectionDialog(Widget);

void
show_fileSelectionDialog(FieldItem * fi)
{
	char tmp[128];
	char * p = NULL;

	if (fileSelectionDialog == NULL) {
		fileSelectionDialog = build_fileSelectionDialog(
			get_shell_ancestor(fi->f_fieldWidget));
	}

	XtVaSetValues(fileSelectionDialog,
		XmNuserData, fi->f_fieldWidget, NULL);
	sprintf(tmp, catgets(catd, 1, 36, "Launcher: %s Selection"), fi->f_label);
	/* a little kludge to remove : from label :) */
	p = strrchr(tmp, ':');
	if (p) *p = ' ';
	XtVaSetValues(XtParent(fileSelectionDialog),
		XmNtitle, tmp,
		NULL);
	XtManageChild(fileSelectionDialog);
}

static void
okCB(Widget w, XtPointer cd, XmFileSelectionBoxCallbackStruct * cbs)
{
	Widget field;
	char * text;

	if (!XmStringGetLtoR(cbs->value,XmSTRING_DEFAULT_CHARSET,&text)) {
		display_error(fileSelectionDialog, catgets(catd, 1, 37, "Bad file name"));
		return;
	}
	if (!*text) {
		return;
	}
	XtVaGetValues(fileSelectionDialog, XmNuserData, &field, NULL);
	XmTextFieldSetString(field, text);
	XtFree(text);
	XtUnmanageChild(fileSelectionDialog);
}

static void
cancelCB(Widget w, XtPointer cd, XmFileSelectionBoxCallbackStruct * cbs)
{
	XtUnmanageChild(fileSelectionDialog);
}


Widget
build_fileSelectionDialog(Widget parent)
{
	Widget d;
	Widget lab;

	d = XmCreateFileSelectionDialog(parent,
		"fileSelectionDialog",
		NULL,
		NULL);
	XtVaSetValues(XtParent(d), XmNtitle, catgets(catd, 1, 38, "Launcher: File Selection"), NULL);
#ifdef USE_FILTER
	XtUnmanageChild(XmFileSelectionBoxGetChild(d, XmDIALOG_FILTER_LABEL));
	XtUnmanageChild(XmFileSelectionBoxGetChild(d, XmDIALOG_FILTER_TEXT));
	XtUnmanageChild(XmFileSelectionBoxGetChild(d, XmDIALOG_APPLY_BUTTON));
#endif
	/* Extract various labels and wrap for i18n */
	lab = XmFileSelectionBoxGetChild(d, XmDIALOG_SELECTION_LABEL);
	XtVaSetValues(lab, RSC_CVT(XmNlabelString, 
		catgets(catd, 1, 39, "Open File")), NULL);
	lab = XmFileSelectionBoxGetChild(d, XmDIALOG_DIR_LIST_LABEL);
	XtVaSetValues(lab, RSC_CVT(XmNlabelString, 
		catgets(catd, 1, 96, "Directories")), NULL);
	lab = XmFileSelectionBoxGetChild(d, XmDIALOG_FILTER_LABEL);
	XtVaSetValues(lab, RSC_CVT(XmNlabelString, 
		catgets(catd, 1, 97, "Filter")), NULL);
	lab = XmFileSelectionBoxGetChild(d, XmDIALOG_LIST_LABEL);
	XtVaSetValues(lab, RSC_CVT(XmNlabelString, 
		catgets(catd, 1, 98, "Files")), NULL);
	lab = XmFileSelectionBoxGetChild(d, XmDIALOG_OK_BUTTON);
	XtVaSetValues(lab, RSC_CVT(XmNlabelString, 
		catgets(catd, 1, 99, "OK")), NULL);
	lab = XmFileSelectionBoxGetChild(d, XmDIALOG_APPLY_BUTTON);
	XtVaSetValues(lab, RSC_CVT(XmNlabelString, 
		catgets(catd, 1, 100, "Filter")), NULL);
	lab = XmFileSelectionBoxGetChild(d, XmDIALOG_CANCEL_BUTTON);
	XtVaSetValues(lab, RSC_CVT(XmNlabelString, 
		catgets(catd, 1, 101, "Cancel")), NULL);
	XtVaSetValues(lab, RSC_CVT(XmNlabelString, 
		catgets(catd, 1, 102, "Help")),
		 NULL);

	XtAddCallback(d, XmNokCallback, okCB, (XtPointer) NULL);
	XtAddCallback(d, XmNcancelCallback, cancelCB, (XtPointer) NULL);
/*
	XtAddCallback(d, XmNactivateCallback, fsbCB, (XtPointer) NULL);
*/
	XtAddCallback(d, XmNhelpCallback, helpCB, (XtPointer) "path.h.hlp");

	return(d);
}

