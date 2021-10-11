#ifndef lint
#pragma ident "@(#)pfgbootdiskselect.c 1.49 95/11/07 SMI"
#endif

/*
 * Copyright (c) 1994-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	pfgbootdiskselect.c
 * Group:	installtool
 * Description:
 */

#include "pf.h"
#include "pfg.h"
#include "pfgBootDiskQuery_ui.h"

int	get_answer(void);
	
static WidgetList	widget_list;
static Widget		bootdiskquery_dialog;
static int		answer;

Widget
pfgCreateBootDiskQuery(Widget parent, char *msg_buf)
{
	char		diskname[32];

	/*
	 * create the boot device selection dialog
	 */
	bootdiskquery_dialog = tu_bootdisk_query_widget("bootdisk_dialog",
		parent, &widget_list);

	XmAddWMProtocolCallback(pfgShell(bootdiskquery_dialog), pfgWMDeleteAtom,
	    (XtCallbackProc) pfgExit, NULL);

	XtVaSetValues(pfgShell(bootdiskquery_dialog),
		XmNtitle, TITLE_BOOTDISKQUERY,
		XmNdeleteResponse, XmDO_NOTHING,
		NULL);
	xm_SetNoResize(pfgTopLevel, bootdiskquery_dialog);

	/*
	 * set up the on-screen text and button labels
	 */
	(void) BootobjGetAttribute(CFG_CURRENT,
			BOOTOBJ_DISK, diskname,
			NULL);
	pfgSetWidgetString(widget_list, "panelhelpText",
		msg_buf);
	pfgSetWidgetString(widget_list, "changeButton", PFG_CHANGEBOOT);
	pfgSetWidgetString(widget_list, "okButton", PFG_OKAY);
	pfgSetWidgetString(widget_list, "cancelButton", PFG_CANCEL);


	/*
	 * manage the newly created dialog
	 */
	XtManageChild(bootdiskquery_dialog);
	answer = NO_ANSWER;

	/*
	 * set the default focus to the cancel button
	 */
	(void) XmProcessTraversal(pfgGetNamedWidget(widget_list, "cancelButton"),
				XmTRAVERSE_CURRENT);

	return (bootdiskquery_dialog);
}

int
get_answer(void)
{
	return(answer);
}

/* ARGSUSED */
void
changeCB(Widget w, XtPointer clientD, XtPointer callD)
{
	answer = ANSWER_SELECT;
	/*
	 * free the widget list(s)
	 */
	free(widget_list);

}

/* ARGSUSED */
void
okCB(Widget w, XtPointer clientD, XtPointer callD)
{
	answer = ANSWER_OK;
	/*
	 * free the widget list(s)
	 */
	free(widget_list);
}

/* ARGSUSED */
void
cancelCB(Widget w, XtPointer clientD, XtPointer callD)
{
	answer = ANSWER_CANCEL;
	/*
	 * free the widget list(s)
	 */
	free(widget_list);
}

