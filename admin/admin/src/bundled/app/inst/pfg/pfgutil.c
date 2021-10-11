#ifndef lint
#pragma ident "@(#)pfgutil.c 1.16 96/07/11 SMI"
#endif

/*
 * Copyright (c) 1994-1995 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	pfgutil.c
 * Group:	installtool
 * Description:
 */

#include "pf.h"
#include "pfg.h"

#include <Xm/CascadeB.h>
#include <X11/cursorfont.h>

static parAction_t UIaction;

static void busyCB(Widget, XtPointer, XEvent*, Boolean*);
static void busy_handler(Widget, Boolean);

static void set_itimer(int sec, int usec);
static void remove_itimer(void);
static void refresh_display(int sig);

/*
 * This function is called by all UI functions that create toplevel
 * screens.  The function processes all events for the screen until
 * the action variable being monitored is set to a value not equal to
 * parANone.  action will be set by the callbacks associated with the
 * screen whose events are being processed.
 */
parAction_t
pfgEventLoop(void)
{
	XEvent event;

	while (pfgGetAction() == parANone) {
		XtAppNextEvent(pfgAppContext, &event);
		XtDispatchEvent(&event);
	}

	return (pfgGetAction());
}

/*
 * function used to set the action to be taken
 */
void
pfgSetAction(parAction_t action)
{
	UIaction = action;
}

/*
 * function to get action specified by the screen whose events
 * were just processed
 */
parAction_t
pfgGetAction(void)
{
	return (UIaction);
}

/*
 * This function holds the widget id for the current screen.  Each time
 * the function is called it destroies the previous screen and save the
 * value of the input widget in current.  This is done so that the
 * the previous screen can be destroyed after the current screen has been
 * created
 */
static Widget current = NULL;
void
pfgSetCurrentScreen(Widget screen)
{
	pfgDestroyPrevScreen(current);
	current = screen;
}

void
pfgSetCurrentScreenWidget(Widget screen)
{
	current = screen;
}

/*
 * this function is used to destroy the previous screen in the
 * parade sequence.  This is necessary because we don't want to
 * destroy the previous screen until the new screen has already
 * been created.  If the screen is destroyed inside its own callback
 * then the user desktop is left blank while the new screen is being
 * created
 */
void
pfgDestroyPrevScreen(Widget previousScreen)
{
	if (previousScreen) {
		XtUnmanageChild(pfgShell(previousScreen));
		XtDestroyWidget(pfgShell(previousScreen));
	}
}

Widget
pfgShell(Widget w)
{
	while (w && !XtIsWMShell(w))
		w = XtParent(w);
	return (w);
}

void
pfgDestroyDialog(Widget parent, Widget dialog)
{
	/* unmanage & destroy the dialog */
	XtUnmanageChild(dialog);
	XtDestroyWidget(dialog);

	/* unbusy the parent */
	pfgUnbusy(pfgShell(parent));
}

void
pfgBusy(Widget w)
{
	static Cursor cursor = NULL;
	Display *display = XtDisplay(pfgTopLevel);
	Window window = XtWindow(w);

	if (cursor == NULL)
		cursor = XCreateFontCursor(display, XC_watch);

	if (window != NULL && w != NULL) {
		XDefineCursor(display, window, cursor);
		xm_ForceDisplayUpdate(pfgTopLevel, w);

		busy_handler(w, True);
	}
}

void
pfgUnbusy(Widget w)
{
	Display *display = XtDisplay(pfgTopLevel);
	Window window = XtWindow(w);

	if (window != NULL && w != NULL) {
		XUndefineCursor(display, XtWindow(w));
		xm_ForceDisplayUpdate(pfgTopLevel, w);

		busy_handler(w, False);
	}
}

static void
busy_handler(Widget w, Boolean operation)
{
	WidgetList children;
	Cardinal i, numChildren;
	EventMask xtmask = KeyPressMask | KeyReleaseMask | ButtonPressMask
		| ButtonReleaseMask | PointerMotionMask;

	if (!w || !(XtIsWidget(w) && XtIsRealized(w)))
		return;

	if (XtIsComposite(w)) {
		write_debug(GUI_DEBUG_L1, "busy_handler for %s",
			XtName(w));
		XtVaGetValues(w,
			XmNchildren, &children,
			XmNnumChildren, &numChildren,
			NULL);
		for (i = 0; i < numChildren; i++)
			busy_handler(children[i], operation);
	}
	if (operation == True)
		XtInsertEventHandler(w, xtmask, True,
			busyCB, (XtPointer)NULL, XtListHead);
	else
		XtRemoveEventHandler(w, XtAllEvents, True,
			busyCB, (XtPointer)NULL);
}

/* ARGSUSED */
void
unregister_as_dropsite(Widget w, XtPointer clientD, XtPointer callD)
{
	/*
	 * unregister this widget as a dropsite for drag-n-drop
	 * operations, which can cause odd behaviour on some user
	 * interface components
	 */

	XmDropSiteUnregister(w);
}

/* ARGSUSED */
static void
busyCB(Widget w, XtPointer client, XEvent * ev, Boolean * dispatch)
{
	*dispatch = False;
}

/*
 * pfgSetWidgetString will take the list of widgets 'widget_list', and a search
 * within that list for the widget with the XtName 'name'. Once/if found,
 * if the widget is subclassed off of XmLabel, it's XmNlabelString will be
 * set to the string 'message_text'. If the widget is an XmText or XmTextField,
 * then it's XmNvalue will be set to the string 'message_text'.
 *
 * this function is intended to be used with TeleUSE, since the creation
 * functions in TeleUSE return a list of additional widgets created for
 * a given widget. I could have just as easily traversed the widget tree
 * and done the same thing - we'll go to that when/if we transition to
 * using the UI library.
 */
void
pfgSetWidgetString(
	WidgetList	widget_list,
	char *		name,
	char *		message_text)
{
	int index = 0;

	if (widget_list == (WidgetList) NULL || name == (char *) NULL) {
		write_debug(GUI_DEBUG_L1,
			"pfgSetWidgetString: null data passed in");
		return;
	}

	while (widget_list[index] != (Widget) NULL &&
		(strcmp(XtName(widget_list[index]), name) != 0))
			index++;

	if (widget_list[index] == (Widget) NULL) {
		write_debug(GUI_DEBUG_L1,
			"pfgSetWidgetString: unable to locate widget %s", name);
		return;
	}

	if (xm_SetWidgetString(widget_list[index], message_text) == FAILURE) {
		write_debug(GUI_DEBUG_L1,
			"xm_SetWidgetString: unknown type for widget %s",
			name);
	}
}

/*
 * pfgGetNamedWidget will take the list of widgets 'widget_list', and a search
 * within that list for the widget with the XtName 'name'. Once/if found,
 * the widget id is returned.
 *
 * this function is intended to be used with TeleUSE, since the creation
 * functions in TeleUSE return a list of additional widgets created for
 * a given widget. I could have just as easily traversed the widget tree
 * and done the same thing - we'll go to that when/if we transition to
 * using the UI library.
 */
Widget
pfgGetNamedWidget(
	WidgetList	widget_list,
	char *		name)
{
	int index = 0;

	if (widget_list == (WidgetList) NULL || name == (char *) NULL) {
		write_debug(GUI_DEBUG_L1,
			"pfgGetNamedWidget: null data passed in");
		return (NULL);
	}

	while (widget_list[index] != (Widget) NULL &&
		(strcmp(XtName(widget_list[index]), name) != 0))
			index++;

	if (widget_list[index] == (Widget) NULL) {
		write_debug(GUI_DEBUG_L1,
			"pfgGetNamedWidget: unable to locate widget %s", name);
		return (NULL);
	}

	return (widget_list[index]);
}

#if 0
#define	_PFG_TIMER_INTERVAL	500000

/* ARGSUSED */
void
start_itimer(void *data)
{
	set_itimer(0, _PFG_TIMER_INTERVAL);
	(void) sigset(SIGALRM, &refresh_display);
}

/* ARGSUSED */
void
clear_itimer(void *data)
{
	remove_itimer();
	(void) sigignore(SIGALRM);
}

/* ARGSUSED */
static void
refresh_display(int sig)
{
	/*
	 * update the display, and reset the timer
	 * Make sure to clear the timer prior to entering
	 * XmUpdateDisplay(), since if it gets an alarm during processing
	 * it hangs on a poll() call.
	 */
	clear_itimer(NULL);
	XmUpdateDisplay(pfgTopLevel);
	start_itimer(NULL);
}

static void
set_itimer(int sec, int usec)
{
	struct timeval val;
	struct itimerval itval;

	remove_itimer();

	val.tv_sec = (long) sec;
	val.tv_usec = (long) usec;

	itval.it_interval = val;
	itval.it_value = val;

	(void) setitimer(ITIMER_REAL, &itval, (struct itimerval *) NULL);
}

static void
remove_itimer(void)
{
	struct timeval val;
	struct itimerval itval;

	/*
	 * setting these to 0 should disable the timer
	 */
	val.tv_sec = (long) 0;
	val.tv_usec = (long) 0;

	itval.it_interval = val;
	itval.it_value = val;

	(void) setitimer(ITIMER_REAL, &itval, (struct itimerval *) NULL);
}
#endif

/*
 * Function: pfgSetStandardButtonStrings
 * Description:
 *	Used to set some standard
 * Scope:	PUBLIC
 * Parameters:  <name> -	[<RO|RW|WO>]
 *				[<validation conditions>]
 *				<description>
 * Return:	[<type>]
 *		<value> - <meaning>
 * Globals:	<name> - [<RO|RW|WO>][<GLOBAL|MODULE|SEGMENT>]
 * Notes:
 */
void
pfgSetStandardButtonStrings(WidgetList widget_list, ...)
{
	va_list ap;
	ButtonType btype;

	va_start(ap, widget_list);

	while ((btype = va_arg(ap, ButtonType)) != NULL) {
		switch (btype) {
		case ButtonContinue:
			pfgSetWidgetString(widget_list, "continueButton",
				LABEL_CONTINUE_BUTTON);
			break;
		case ButtonGoback:
			pfgSetWidgetString(widget_list, "gobackButton",
				LABEL_GOBACK_BUTTON);
			break;
		case ButtonChange:
			pfgSetWidgetString(widget_list, "changeButton",
				LABEL_CHANGE_BUTTON);
			break;
		case ButtonExit:
			pfgSetWidgetString(widget_list, "exitButton",
				UI_BUTTON_EXIT_STR);
			break;
		case ButtonHelp:
			pfgSetWidgetString(widget_list, "helpButton",
				UI_BUTTON_HELP_STR);
			break;
		case ButtonOk:
			pfgSetWidgetString(widget_list, "continueButton",
				UI_BUTTON_CONTINUE_STR);
			pfgSetWidgetString(widget_list, "okButton",
				UI_BUTTON_OK_STR);
			break;
		case ButtonCancel:
			pfgSetWidgetString(widget_list, "cancelButton",
				UI_BUTTON_CANCEL_STR);
			break;
		default:
			break;
		}
	}
	va_end(ap);
}

/*
 * Get the maximum height of all the widgets and set all their
 * heights to this maximum.
 */
void
pfgSetMaxWidgetHeights(WidgetList widget_list, char **widget_names)
{
	Widget *widget_array;
	Dimension height;
	Dimension max_height = 0;
	int i;

	if (get_trace_level() == 5) {
		write_debug(GUI_DEBUG_L1,
			"Entering pfgSetMaxWidgetHeights - widget_names:");
	}
	for (i = 0; widget_names[i]; i++) {
		if (get_trace_level() == 5) {
			write_debug(GUI_DEBUG_L1_NOHD, "widget_names[%d] = %s",
				i, widget_names[i]);
		}
	}

	/* how many widget_names do I have to set the height on? */
	for (i = 0; widget_names[i]; i++);

	/* create the widget array for these widget_names */
	widget_array = (Widget *) xmalloc(sizeof (Widget) * (i + 1));
	for (i = 0; widget_names[i]; i++) {
		widget_array[i]  = pfgGetNamedWidget(widget_list,
					widget_names[i]);
	}
	widget_array[i] = NULL;

	/* get the maximum column label height */
	for (i = 0; widget_array[i]; i++) {
		XtVaGetValues(widget_array[i],
			XmNheight, &height,
			NULL);
		max_height = MAX(height, max_height);
	}
	write_debug(GUI_DEBUG_L1, "max column label height = %d\n", max_height);

	/* set all column widget_names to the maximum column height */
	for (i = 0; widget_array[i]; i++) {
		XtVaSetValues(widget_array[i],
			XmNheight, max_height,
			NULL);
	}
}

/*
 * The widths for option menus are a bit of a hack here...
 * What I really want is to get the width of an
 * option menu.  However, in order to come up with the
 * right width of an optionmenu, you really need to get
 * the width of the "OptionButton" so that the full
 * width is accounted for.  However, since
 * "OptionButton" is not created directly by TeleUSE (it's
 * done within Motif convenience routines for creating an
 * option button), we can't pass "OptionButton" directly
 * to pfgGetNamedWidget.  So, we improvise, and assume
 * that if a person is trying to get the width of a
 * rowColumn and it has an "OptionButton" child, that they
 * really want the width of the "OptionButton".
 */
void
pfgSetMaxColumnWidths(
	WidgetList widget_list,
	WidgetList *entries,
	char **labels,
	char **values,
	Boolean offset_first,
	int offset)
{
	int entry_index;
	int value_index;
	Widget value;
	Widget option_button;
	Dimension max_width;
	Dimension width;
	Dimension ob_width;
	Dimension om_width;
	Dimension extra_left_offset;
	Dimension extra_width;
	Dimension max_ob_width;

	/* first, get the max width for each column */
	max_ob_width = 0;
	for (value_index = 0; values[value_index]; value_index++) {

		/* get width from column heading */
		XtVaGetValues(pfgGetNamedWidget(widget_list,
			labels[value_index]),
			XmNwidth, &max_width,
			NULL);

		/* get value width */
		for (entry_index = 0; entries[entry_index]; entry_index++) {
			value = pfgGetNamedWidget(entries[entry_index],
				values[value_index]);
			option_button = NULL;
			extra_width = 0;
			if (XtIsSubclass(value,
				xmRowColumnWidgetClass)) {
				option_button = XtNameToWidget(value,
					"OptionButton");
				if (option_button) {
					XtVaGetValues(value,
						XmNwidth, &extra_width,
						NULL);
					value = option_button;
					XtVaGetValues(option_button,
						XmNwidth, &ob_width,
						NULL);
					max_ob_width = 
						MAX(max_ob_width, ob_width);
				}
			}
			XtVaGetValues(value,
				XmNwidth, &width,
				NULL);

			if (option_button) {
				write_debug(GUI_DEBUG_L1,
					"option menu width = %d",
					extra_width);
				write_debug(GUI_DEBUG_L1,
					"option menu button width = %d",
					width);
			}

			max_width = MAX(width + extra_width, max_width);
		}

		write_debug(GUI_DEBUG_L1, "%s Max Column Width = %d",
			labels[value_index], max_width);

		/* now set max width on column */
		if (value_index == 0 && !offset_first) {
			XtVaSetValues(pfgGetNamedWidget(widget_list,
				labels[value_index]),
				XmNwidth, max_width,
				NULL);
		} else {
			XtVaSetValues(pfgGetNamedWidget(widget_list,
				labels[value_index]),
				XmNwidth, max_width,
				XmNleftOffset, offset,
				NULL);
		}

		/* now set max width on values */
		for (entry_index = 0; entries[entry_index]; entry_index++) {
			value = pfgGetNamedWidget(entries[entry_index],
				values[value_index]);
			extra_left_offset = 0;
			if (XtIsSubclass(value,
				xmRowColumnWidgetClass)) {
				option_button = XtNameToWidget(value,
					"OptionButton");

				XtVaGetValues(value,
					XmNwidth, &om_width,
					NULL);
				XtVaGetValues(option_button,
					XmNwidth, &ob_width,
					NULL);
				if (max_width > (om_width + max_ob_width))
					extra_left_offset =
						max_width -
						(om_width + max_ob_width);
			}
			if (value_index == 0 && !offset_first) {
				XtVaSetValues(value,
					XmNwidth, max_width,
					NULL);
			} else {
				write_debug(GUI_DEBUG_L1,
					"extra left offset = %d",
					extra_left_offset);
				XtVaSetValues(value,
					XmNwidth, max_width,
					XmNleftOffset,
						offset + extra_left_offset,
					NULL);
			}
		}

	}
	for (entry_index = 0; entries[entry_index]; entry_index++) {
		free(entries[entry_index]);
	}
	free(entries);
}
