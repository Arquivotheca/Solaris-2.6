/*
 * Copyright (c) 1993 Sun Microsystems, Inc.  All Rights Reserved.
 * Sun considers its source code as an unpublished, proprietary trade secret,
 * and it is available only under strict license provisions.  This copyright
 * notice is placed here only to protect Sun in the event the source is
 * deemed a published work.  Dissassembly, decompilation, or other means of
 * reducing the object code to human readable form is prohibited by the
 * license agreement under which this code is provided to the user or company
 * in possession of this copy.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the Government
 * is subject to restrictions as set forth in subparagraph (c)(1)(ii) of the
 * Rights in Technical Data and Computer Software clause at DFARS 52.227-7013
 * and in similar clauses in the FAR and NASA FAR Supplement.
 */

#pragma	ident	"@(#)xm_utils.c 1.17 95/11/13"

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <Xm/Xm.h>
#include <Xm/List.h>
#include <Xm/RowColumn.h>
#include <Xm/MessageB.h>
#include "xm_defs.h"
#include "xm_help.h"
#include "sysid_msgs.h"

#ifdef USE_XPG4_WCS
/*
 * If strwidth() is not adopted in CDE/s495, convert x to wide char
 * string and use wcswidth(x).
 */
#define	MBWIDTH(x)		strwidth(x)
#else
#define	MBWIDTH(x)		eucscol(x)
#endif

static void xm_remove_working(Widget, XtPointer, XtPointer);
static void xm_busy_callback(Widget, XtPointer, XEvent *, Boolean *);
static void xm_busy_handler(Widget, int);

Sysid_err
xm_validate_value(Widget parent, Field_desc *f)
{
	Sysid_err	errcode;

	if (f->validate == (Validate_proc *)0)
		return (SYSID_SUCCESS);

	errcode = f->validate(f);

	if (errcode != SYSID_SUCCESS) {
		char	*errstr = get_err_string(errcode, 1, f->value);

		xm_popup_error(parent, VALIDATION_ERROR_TITLE, errstr);
	}
	return (errcode);
}

void
xm_popup_error(Widget parent, const char *title, char *errstr)
{
	Widget		dialog;
	Widget		button;
	XmString	t;

	dialog = XmCreateErrorDialog(
			parent, "errorDialog", (Arg *)0, 0);

	t = xm_format_text(errstr, 40, 0);
	XtVaSetValues(dialog,
	    XmNmessageString,	t,
	    XmNdialogStyle,	XmDIALOG_PRIMARY_APPLICATION_MODAL,
	    NULL);
	XmStringFree(t);

	XtVaSetValues(XtParent(dialog), XmNtitle, title, NULL);

	button = XmMessageBoxGetChild(dialog, XmDIALOG_CANCEL_BUTTON);
	if (button != (Widget)0)
		XtUnmanageChild(button);

	button = XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON);
	if (button != (Widget)0)
		XtUnmanageChild(button);

	XtManageChild(dialog);
	XtPopup(XtParent(dialog), XtGrabNone);
}

void
xm_get_value(Widget widget, Field_desc *f)
{
	Widget	button;
	Menu	*menu;
	int	*pos;
	int	npos;

	switch (f->type) {
	case FIELD_TEXT:
		XtVaGetValues(widget, XmNvalue, &f->value, NULL);
		break;

	case FIELD_EXCLUSIVE_CHOICE:
		menu = (Menu *)f->value;

		if (f->flags & FF_FORCE_SCROLLABLE) {
			if (XmListGetSelectedPos(widget, &pos, &npos) == True) {
				if (npos > 0)
					menu->selected = pos[0] - 1;
				else
					menu->selected = -1;
				XtFree(pos);
			} else
				menu->selected = -1;
		} else {
			XtVaGetValues(widget, XmNmenuHistory, &button, NULL);
			if (button != (Widget)0)
				XtVaGetValues(button,
					XmNuserData,	&menu->selected,
					NULL);
			else
				menu->selected = -1;
		}
		break;

	case FIELD_CONFIRM:
		XtVaGetValues(widget, XmNmenuHistory, &button, NULL);
		if (button != (Widget)0)
			XtVaGetValues(button, XmNuserData, &f->value, NULL);
		else
			f->value = (void *)-1;
		break;
	}
}

XmString
xm_format_text(char *text, int width, int do_prompt)
{
	XmString value;
	XmString l1, l2;
	char	**lines;
	char	**cpp;

	value = XmStringCreateLocalized("");

	if (text == (char *)0)
		return (value);

	lines = format_text(text, width);

	for (cpp = lines; *cpp != (char *)0; cpp++) {
		char	linebuf[256+2];	/* NB: 256 > width */

		if (do_prompt) {
			/*
			 * For prompts, only add a newline
			 * if we had to break the line.
			 */
			if (**cpp == '\0')
				continue;
			(void) sprintf(linebuf, "%s%.*s",
				cpp != lines ? "\n" : "",
				sizeof (linebuf) - 2, *cpp);
		} else
			/*
			 * For other blocks of text, add
			 * a newline after each line.
			 */
			(void) sprintf(linebuf, "%.*s\n",
				sizeof (linebuf) - 2, *cpp);

		l1 = value;
		l2 = XmStringCreateLocalized(linebuf);

		value = XmStringConcat(l1, l2);
		XmStringFree(l1);
		XmStringFree(l2);
	}
	free(lines);

	return (value);
}

static Widget	working_dialog;

Widget
xm_create_working(Widget widget, char *text)
{
	XmString	t;
	Widget		child;

	if (widget == (Widget)0) {
		if (working_dialog == (Widget)0) {
			widget = XmCreateWorkingDialog(
				toplevel, "busyDialog", (Arg *)0, 0);

			XtVaSetValues(XtParent(widget),
			    XmNtitle,	WORKING_TITLE,
			    XmNautoUnmanage,	False,
			    NULL);

			child = XmMessageBoxGetChild(
					widget, XmDIALOG_OK_BUTTON);
			if (child != (Widget)0)
				XtUnmanageChild(child);

			child = XmMessageBoxGetChild(
					widget, XmDIALOG_CANCEL_BUTTON);
			if (child != (Widget)0)
				XtUnmanageChild(child);

			child = XmMessageBoxGetChild(
					widget, XmDIALOG_HELP_BUTTON);
			if (child != (Widget)0)
				XtUnmanageChild(child);

			child = XmMessageBoxGetChild(
					widget, XmDIALOG_SEPARATOR);
			if (child != (Widget)0)
				XtUnmanageChild(child);

			XtAddCallback(widget, XmNdestroyCallback,
						xm_remove_working, NULL);
			working_dialog = widget;
		} else
			widget = working_dialog;
	}

	t = xm_format_text(text, 30, 0);
	XtVaSetValues(widget, XmNmessageString, t, NULL);
	if ((int)MBWIDTH(text) < 30)	/* XXX */
		XtVaSetValues(widget, XmNwidth, 250, XmNheight, 100, NULL);
	XmStringFree(t);

	XtManageChild(widget);
	XtPopup(XtParent(widget), XtGrabNone);

	return (widget);
}

void
xm_destroy_working(void)
{
	if (working_dialog != (Widget)0) {
		/*
		 * pop down widget with incoming argument id
		 */
		XtPopdown(XtParent(working_dialog));
		XtDestroyWidget(working_dialog);
	}
}

/*ARGSUSED*/
static void
xm_remove_working(
	Widget		widget,
	XtPointer	client_data,
	XtPointer	call_data)
{
	working_dialog = (Widget)0;
}

/*ARGSUSED*/
void
xm_help(Widget		widget,
	XtPointer	client_data,
	XtPointer	call_data)
{
	static Widget another_top_level_shell;
	/* LINTED [alignment ok] */
	Field_help *help = (Field_help *)client_data;

	if (another_top_level_shell == (Widget)0)
		another_top_level_shell = XtAppCreateShell("sysidtool",
			"Helper", applicationShellWidgetClass,
			XtDisplay(toplevel), (ArgList) NULL, 0);


	if (help == (Field_help *)0)
		adminhelp(another_top_level_shell, NULL, NULL);
	else
		adminhelp(another_top_level_shell, TOPIC, help->topics);
}

XmString *
xm_create_list(Menu *menu, int nitems)
{
	XmString	*list;
	int		i;

	list = (XmString *)xmalloc(sizeof (XmString) * nitems);
	for (i = 0; i < nitems; i++)
		list[i] = XmStringCreateLocalized(menu->labels[i]);
	return (list);
}

/*ARGSUSED*/
void
xm_destroy_list(Widget	widget,
	XtPointer	client_data,
	XtPointer	call_data)
{
	free(client_data);
}

#include <X11/cursorfont.h>

void
xm_busy(Widget widget)
{
	static	Display	*display;
	static	Cursor cursor;

	/*
	 * Set busy cursor
	 * and block events
	 */
	if (cursor == (Cursor)0) {
		display = XtDisplay(toplevel);
		cursor = XCreateFontCursor(display, XC_watch);
	}
	XDefineCursor(display, XtWindow(widget), cursor);
	xm_busy_handler(widget, True);
}

void
xm_unbusy(Widget widget)
{
	/*
	 * remove busy cursor
	 * and unblock events
	 */
	XUndefineCursor(XtDisplay(widget), XtWindow(widget));
	xm_busy_handler(widget, False);
}

/*ARGSUSED*/
static void
xm_busy_callback(Widget	widget,
	XtPointer	client_data,
	XEvent		*event,
	Boolean		*dispatch)
{
	*dispatch = False;
}

static void
xm_busy_handler(Widget widget, int operation)
{
	WidgetList	children;
	Cardinal	i, numChildren;
	EventMask	xtmask = KeyPressMask | KeyReleaseMask |
				ButtonPressMask | ButtonReleaseMask |
				PointerMotionMask;

	if ((XtIsWidget(widget) == False) || (XtIsRealized(widget) == False))
		return;

	if (XtIsComposite(widget)) {
		XtVaGetValues(widget,
			XmNchildren, &children,
			XmNnumChildren, &numChildren,
			NULL);
		for (i = 0; i < numChildren; ++i)
			xm_busy_handler(children[i], operation);
	}

	if (operation == True)
		XtInsertEventHandler(widget, xtmask, True,
			xm_busy_callback, (XtPointer)0, XtListHead);
	else
		XtRemoveEventHandler(widget, XtAllEvents, True,
			xm_busy_callback, (XtPointer)0);
}

Widget
xm_get_shell(Widget w)
{
	while (w && !XtIsWMShell(w))
		w = XtParent(w);
	return (w);
}
