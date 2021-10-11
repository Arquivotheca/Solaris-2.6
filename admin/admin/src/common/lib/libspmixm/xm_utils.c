#ifndef lint
#pragma ident "@(#)xm_utils.c 1.3 96/10/16 SMI"
#endif

/*
 * Copyright (c) 1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	xm_utils.c
 * Group:	libspmixm
 * Description:
 *	some generic Motif utility functions.
 */

#include "spmixm_api.h"
#include "xm_utils.h"

/*
 * Function: xm_ChildWidgetFindByClass
 * Description:
 *	Start at the parent widget and search down it's children looking
 *	for a child of a certain type (class).
 * Scope:       PUBLIC
 * Parameters:
 *	widget - parent widget
 *	class - widget class to look for
 *
 * Return:	[Widget]
 *	NULL - no child widget of that class
 */
Widget
xm_ChildWidgetFindByClass(Widget widget, WidgetClass class) {

	Cardinal num_children;
	WidgetList children;
	Widget w;
	int i;

	if (!widget)
		return (NULL);

	if (XtIsSubclass(widget, class))
		return (widget);

	XtVaGetValues(widget,
		XmNnumChildren, &num_children,
		XmNchildren, &children,
		NULL);

	for (i = 0; i < num_children; i++) {
		w = xm_ChildWidgetFindByClass(children[i], class);
		if (w)
			return (w);
	}

	return (NULL);
}

/*
 * Function: xm_GetShell
 * Description:
 *	 Find this widgets shell.
 * Scope:       PUBLIC
 * Parameters:  widget - the widget whose parent shell you want to find.
 * Return:	[Widget]
 *	NULL - couldn't find shell
 */
Widget
xm_GetShell(Widget w)
{
	while (w && !XtIsSubclass(w, shellWidgetClass))
		w = XtParent(w);
	return (w);
}

/*
 * Function: xm_SetNoResize
 * Description:
 *	removes resize handles under both the OPEN LOOK
 *	and the mwm window manager
 * Scope:  PUBLIC
 * Parameters:
 *	toplevel - top level widget
 *	w	- widget whose shell you don't want to be resizable
 * Return:	none
 */
void
xm_SetNoResize(Widget toplevel, Widget w)
{
	Display *dis;
	Window xid;
	Atom name, value;

	dis = XtDisplay(toplevel);
	xid = XtWindow(xm_GetShell(w));

	name = XInternAtom(dis, "_OL_DECOR_DEL", False);
	value = XInternAtom(dis, "_OL_DECOR_RESIZE", False);

	XChangeProperty(dis, xid, name, XA_ATOM, 32,
			PropModeAppend, (unsigned char *)&value, 1);

	XtVaSetValues(w,
		XmNnoResize, True,
		NULL);
}

/*
 * Function: xm_ForceDisplayUpdate
 * Description:
 *	Force a display to update by flushing all the x events and
 *	updating the display.
 * Scope:       PUBLIC
 * Parameters:
 *	toplevel - top level widget
 *	dialog - dialog widget
 * Return:	none
 */
void
xm_ForceDisplayUpdate(Widget toplevel, Widget dialog)
{
	XFlush(XtDisplay(toplevel));
	XmUpdateDisplay(dialog);
}

/*
 * Function: xm_ForceEventUpdate
 * Description:
 *	Force a display to update by processing all the pending X events
 *	and *	updating the display.
 * Scope:       PUBLIC
 * Parameters:
 *	app_context - application context
 *	toplevel - top level widget
 * Return:	none
 */
void
xm_ForceEventUpdate(XtAppContext app_context, Widget toplevel)
{
	while (XtAppPending(app_context)) {
		XtAppProcessEvent(app_context, XtIMAll);
		XmUpdateDisplay(toplevel);
		XSync(XtDisplay(toplevel), False);
	}
}

/*
 * Function: xm_SetWidgetString
 * Description:
 *	Take the supplied messge string and set the appropriate
 *	resources in the widget to give it this text value.
 *	The class of the widget is checked in order to do this
 *	appropriately.
 * Scope:       PUBLIC
 * Parameters:
 *	widget - the widget whose text value we want to set
 *	message_text - the text string value to set the widget value to
 * Return:
 *	SUCCESS - the text value has been set
 *	FAILURE - widget is not a known class
 */
int
xm_SetWidgetString(Widget widget, char *message_text)
{
	XmString xmstr;
	char *msg;

	if (message_text)
		msg = message_text;
	else
		msg = "";

	if (XmIsRowColumn(widget)) {
		xmstr = XmStringCreateLocalized(msg);
		XtVaSetValues(widget, XmNlabelString, xmstr, NULL);
		XmStringFree(xmstr);
	} else if (XmIsTextField(widget)) {
		XmTextFieldSetString(widget, msg);
	} else if (XmIsText(widget)) {
		XmTextSetString(widget, msg);
	} else if (XmIsLabel(widget)) {
		xmstr = XmStringCreateLocalized(msg);
		XtVaSetValues(widget, XmNlabelString, xmstr, NULL);
		XmStringFree(xmstr);
	} else {
		return (FAILURE);
	}

	return (SUCCESS);
}


/*
 * Function: xm_IsDescendent
 * Description:
 *	Determines if w is a descendent of base.
 * Scope:       PUBLIC
 * Parameters:
 *      Widget base - possible direct ancestor of w
 *      Widget w    - possible descendent widget of base
 *
 * Return:      [Boolean]
 *	True if w is a descendent of base, False otherwise.
 */
Boolean
xm_IsDescendent(Widget base, Widget w)
{
	while (w && w != base) {
		w = XtParent(w);
	}

	if (w == base)
		return (True);

	return (False);
}


/*
 * Function: xm_AlignWidgetCols
 * Description:
 *    Get the width of w and the max width of all the widget in the
 *    NULL terminated list col. If the width of w is greater than the
 *    max of col. Set the width of all the widgets in col to the
 *    width of w. If the max width of col is greater than the width of
 *    w then set the width of w to the max width of col. This is used as a
 *    tool to align columns with headings.
 * Scope:       PUBLIC
 * Parameters:
 *      Widget w
 *      Widget col
 *
 * Return:      [void]
 */
void
xm_AlignWidgetCols(Widget w, Widget *col)
{
	Dimension width;
	Dimension col_width;
	Dimension tmp;
	Widget    *w_ptr;

	XtVaGetValues(w,
		XmNwidth, &width,
		NULL);

	col_width = 0;
	for (w_ptr = col; *w_ptr != NULL; w_ptr++) {
		XtVaGetValues(*w_ptr,
			XmNwidth, &tmp,
			NULL);
		if (tmp > col_width)
		col_width = tmp;
	}

	if (width > col_width) {
		for (w_ptr = col; *w_ptr != NULL; w_ptr++) {
			XtVaSetValues(*w_ptr,
				XmNwidth, width,
				NULL);
		}
	} else if (col_width > width) {
		XtVaSetValues(w,
			XmNwidth, col_width,
			NULL);
	}
}


/*
 *  Function: xm_SizeScrolledWindowToWorkArea
 *
 *  Description:
 *
 *      Sizes a scrolled window so that the window is the size of the
 *  work area.
 *
 *  Note: The scrolled window must already be managed. That is we need
 *  to find the width and height of the work area widget. The
 *  XmNdisplayPolicy of the scrolled window should be set to XmAS_NEEDED.
 *  The XmNscrollingPolicy should be set to XmAUTOMATIC.
 *  If resizing the scrolled window means resizing the shell that contains
 *  it then XmNallowShellRezie must be set to True for that shell. A good
 *  place to call this routine from is the XmNpopupCallback.
 *
 *  Scope:
 *      PUBLIC
 *
 *  Parameters:
 *
 *      Widget  w;        - scrolled window.
 *      int     doWidth;  - size width to work area width
 *      int     doHeight; - size height to work area height
 *
 *  returns:
 *
 *      int - 0 on error otherwise non-zero.
 */
int
xm_SizeScrolledWindowToWorkArea(Widget w, Boolean doWidth, Boolean doHeight)
{
	Arg		args[4];
	Cardinal	argcnt;
	Widget		workArea;
	Widget		vertScrollBar;
	Widget		horizScrollBar;
	unsigned char	displayPolicy;
	Dimension	shadowThickness;
	Dimension	width, height;
	Dimension	workWidth, workHeight;
	Dimension	workBorder;
	Dimension	marginWidth, marginHeight;
	Dimension	spacing;
	Dimension	extraWidth = 0, extraHeight = 0;
	int		hBarUsed, vBarUsed;

	if (!XtIsSubclass(w, xmScrolledWindowWidgetClass)) {
		return (0);
	}

	XtVaGetValues(w,
		XmNhorizontalScrollBar, &horizScrollBar,
		XmNverticalScrollBar, &vertScrollBar,
		XmNscrollBarDisplayPolicy, &displayPolicy,
		XmNworkWindow, &workArea,
		XmNshadowThickness, &shadowThickness,
		XmNspacing, &spacing,
		XmNwidth, &width,
		XmNheight, &height,
		XmNscrolledWindowMarginWidth, &marginWidth,
		XmNscrolledWindowMarginHeight, &marginHeight,
		NULL);

	if (!workArea || displayPolicy != XmAS_NEEDED)
		return (0);

	XtVaGetValues(workArea,
		XmNwidth, &workWidth,
		XmNheight, &workHeight,
		XmNborderWidth, &workBorder,
		NULL);
	/*
	 *	Multiply by two (<<1) to account for two sides. Add two because
	 *	we want the size to be 1 pixel greater than the size of the
	 *	work area.
	 */

	workWidth   += (workBorder << 1);
	workHeight  += (workBorder << 1);
	extraHeight += ((marginHeight + shadowThickness) << 1);
	extraWidth  += ((marginWidth + shadowThickness) << 1);

	if (workWidth > width - extraWidth && horizScrollBar &&
		XtIsManaged(horizScrollBar))
		hBarUsed = 1;
	else
		hBarUsed = 0;

	if (workHeight > height - extraHeight && vertScrollBar &&
		XtIsManaged(vertScrollBar))
		vBarUsed = 1;
	else
		vBarUsed = 0;

	if (displayPolicy == XmAS_NEEDED) {

		if (doWidth && !doHeight) {
			if (vBarUsed) {
				XtVaGetValues(vertScrollBar,
					XmNwidth, &width,
					NULL);

				extraWidth += width + spacing + 2;

			}
		} else if (doHeight && !doWidth) {
			if (hBarUsed) {
				XtVaGetValues(horizScrollBar,
					XmNheight, &height,
					NULL);

				extraHeight += height + spacing + 2;
			}
		}
	}

	argcnt = 0;
	if (doWidth) {
		workWidth += extraWidth;
		XtSetArg(args[argcnt], XmNwidth, workWidth); argcnt++;
	}
	if (doHeight) {
		workHeight += extraHeight;
		XtSetArg(args[argcnt], XmNheight, workHeight); argcnt++;
	}
	XtSetValues(w, args, argcnt);

	return (1);
}
