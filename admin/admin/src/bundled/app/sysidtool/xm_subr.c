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

#pragma	ident	"@(#)xm_subr.c 1.34 95/10/06"

#include <sys/filio.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <stropts.h>
#include <sys/conf.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <Xm/Xm.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/ScrolledW.h>
#include <Xm/RowColumn.h>
#include <Xm/DialogS.h>
#include <Xm/MenuShell.h>
#include <Xm/MessageB.h>
#include <Xm/SelectioB.h>
#include <Xm/LabelG.h>
#include <Xm/ToggleBG.h>
#include <Xm/TextF.h>
#include "xm_defs.h"
#include "sysid_msgs.h"

#if defined(QA_PARTNER)
#include "partner.h"
#endif

#ifdef XEDITRES
#include <X11/Xmu/Editres.h>
#endif

/*
 * This file contains the implementation of generic forms for
 * the Motif GUI.  Each screen of information is implemented
 * as a popup dialog with a rowColumn form as its Work Area.
 * The first widget in a generic form is either a label, if there
 * is background text associated with the form, or the first field
 * of the form.  If a label, it is followed by the first field.
 *
 * Each field in the form is in turn composed of a simple form
 * (Field Form) containing a label gadget and a widget appropriate
 * for the type of the form.  Generally, this type will be one
 * of a textField (most common), a menu, or a radio box.
 *
 * A field that is handled specially is the form's "summary
 * field" -- a field that is intended to reflect the union of
 * the values of the form's other fields.  When sensitive text
 * fields are updated (losingFocusCallback), the validation
 * routine associated with the summary field is called and upon
 * return the summary field is redisplayed.  Currently the only
 * use for this field is in the "Date and Time" screen, for
 * displaying the concatenation of the date and time's
 * individual components.
 *
 * Association of user data with widgets:
 *
 *	Dialog:		the [rowColumn] form constituting the work area
 *	Work Area:	a pointer to the XmFields describing the form
 *	Field Form:	the initial value of the field (for Reset)
 *	Radio Box:	the initially set ToggleButton (for Reset)
 *	Text Field:	a pointer to the Field_desc describing the field
 *
 * Callback client data:
 *
 *	Ok, Yes, No buttons:
 *			the fd where replies get sent (reply_to)
 *	Text fields:	the XmField describing the form's "summary field"
 */

Widget	toplevel;			/* sysidtool GUI top-level shell */

static int	ui_active = 0;		/* TRUE if we've prompted the user */

static void	consume_timeout(XtPointer, XtIntervalId *);
static void	start_stop(int);
static Boolean	xm_start_stop(XtPointer);

static void	xm_get_message(XtPointer, int *, XtInputId *);
static void	read_values(Widget, XtPointer, XtPointer);
static void	reset_values(Widget, XtPointer, XtPointer);
static void	send_confirm(Widget, XtPointer, XtPointer);
static void	send_ok(Widget, XtPointer, XtPointer);

#ifdef DEV

#include <assert.h>

/*ARGSUSED*/
static void
onintr(int sig)
{
	exit(1);
}
#endif

static int	read_fd;
static int	write_fd;

/*
 * "dummy" function used to consume the timeout
 * event we generated in order to force our work
 * procedure to be called.
 */
/*ARGSUSED*/
static void
consume_timeout(XtPointer client_data, XtIntervalId *id)
{
	return;
}

/*
 * The signal handler for SIGPOLL and SIGUSR1, used
 * to implement the persistent UI server protocol.
 * This function instantiates a work procedure to
 * do input source connection/disconnection cleanly.
 * The full protocol is described below.
 */
static void
start_stop(int sig)
{
	XtAppContext	app = XtWidgetToApplicationContext(toplevel);

	(void) XtAppAddWorkProc(app, xm_start_stop, (XtPointer)sig);
	/*
	 * Force the work procedure to be called
	 * by generating a timeout event
	 */
	(void) XtAppAddTimeOut(app, (u_long)10, consume_timeout, (XtPointer)0);
}

/*
 * Boolean xm_start_stop(Xtpointer client_data)
 *
 * This routine is used to implement the persistent Motif server
 * and is instantiated as a work procedure from the actual signal
 * handler (start_stop).  It returns such that it is automatically
 * removed.  The signal protocol is as follows:
 *
 * We get a SIGUSR1 when we're supposed to start up (add input
 * descriptor) and SIGPOLL when the file descriptor is closed by
 * sysidtool (remove input descriptor).
 *
 * Unfortunately, the signals don't always arrive in the order we
 * expect.  In particular, SIGUSR1 can be delivered before SIGPOLL
 * if a prompt_close/prompt_open pair is used within the same process.
 * To guard against this, we must block both SIGUSR1 and SIGPOLL on
 * entry to this routine and flag SIGUSR1 as "pending" if we haven't
 * gotten SIGPOLL yet (i.e., the input descriptor is active).
 *
 * If this situation occurs, we can essentially consume the [pending]
 * SIGUSR1 signal at the time we get SIGPOLL, since we know sysidtool
 * is waiting for us (we don't need to go through the remove/add cycle).
 */
static Boolean
xm_start_stop(XtPointer client_data)
{
	XtAppContext	app = XtWidgetToApplicationContext(toplevel);
	struct strrecvfd recvfd;
	static int	start_pending;
	static int	ok_to_read;
	static XtInputId input_id;
	int	sig = (int)client_data;
	int	fd;

	(void) sighold(SIGPOLL);
	(void) sighold(SIGUSR1);

	if (sig == SIGUSR1) {	/* start */
		if (ok_to_read)
			start_pending = 1;
		else {
			ok_to_read = 1;
			start_pending = 0;
		}
	} else {		/* stop */
		if (!start_pending) {
			ok_to_read = 0;
			(void) ioctl(write_fd, I_FLUSH, FLUSHRW);
			if (input_id != (XtInputId)0) {
				XtRemoveInput(input_id);
				input_id = (XtInputId)0;
			}
		} else {
			start_pending = 0;
			ok_to_read = 0;
		}
		(void) close(0);
	}
	if (ok_to_read && !start_pending) {
		if (input_id == (XtInputId)0) {
			input_id = XtAppAddInput(app, read_fd,
				(XtPointer)(XtInputReadMask),
				xm_get_message, (XtPointer)write_fd);
		}

		if (ioctl(read_fd, I_RECVFD, &recvfd) >= 0)
			fd = dup2(recvfd.fd, 0);
#ifdef DEV
		else
			assert(errno == 0);

		assert(fd == 0);
#endif
	}

	(void) sigrelse(SIGPOLL);
	(void) sigrelse(SIGUSR1);

	return (True);	/* remove work proc on return */
}

#if !defined(sparc) && defined(QA_PARTNER)
/*ARGSUSED*/
static void
InitQaPartner(Widget w)
{
}
#endif

/*ARGSUSED*/
Sysid_err
do_init(int *argcp, char **argv, int read_from, int reply_to)
{
	XtAppContext	app;
	Dimension	width, height;
	Display		*display;
	struct	sigaction sa;

	static String	fallbacks[] = {
		/*
		 * Colors
		 */
		"Sysidtool*background: gray",
		"Sysidtool*foreground: black",
		/*
		 * Layout
		 */
		"Sysidtool*dialogForm.horizontalSpacing: 18",
		"Sysidtool*dialogText.topOffset: 18",
		"Sysidtool*dialogData.topOffset: 27",
		"Sysidtool*dialogData.bottomOffset: 18",
		/*
		 * No ja/ko/zh/zh_TW character input is needed.
		 */
		"Sysidtool*inputMethod: None",
		/*
		 * Sizes
		 */
		/*
		 * Fonts
		 */
		"Sysidtool*fontList: helvetica14",
		"Sysidtool*dialogText.fontList: helvetica14",
		"Sysidtool*errorDialog.XmLabelGadget.fontList: helvetica14",
		"Sysidtool*XmPushButton.fontList: helvetica-bold14",
		"Sysidtool*XmPushButtonGadget.fontList: helvetica-bold14",
		"Sysidtool*XmLabelGadget.fontList: helvetica-bold14",
		"Sysidtool*XmTextWidget.fontList: helvetica-bold14",
		"Sysidtool*XmList.fontList: helvetica14",
		/*
		 * on-line help
		 */
		"*Help*background: gray",
		"*Help*foreground: black",
		"*Help*fontList: helvetica14",
		"*Help*rowcolumn*fontList: helvetica-bold14",
		"*Help*rowcol*fontList: helvetica-bold14",
		"*Help*helptext.fontList: helvetica14",
		"*Help*helpsubjs.fontList: helvetica14",
		"*Help*helptext.rows: 15",
		"*Help*helptext.columns: 52",
		"*Help*helpsubjs.visibleItemCount: 4",
		NULL
	};

	read_fd = read_from;
	write_fd = reply_to;

	/*
	 * Open the X11 library via an explicit dlopen call.  We
	 * do this because the i18n support in libX11 is in turn
	 * dynamically loaded and must be able to refer to symbols
	 * back in libX11.  The only way this is possible is if
	 * libX11 was loaded at program start-up (it wasn't) or
	 * if we dlopen() it and make its symbols globally accessible.
	 */
	if (dlopen("libX11.so", RTLD_LAZY | RTLD_GLOBAL) == NULL)
		return (SYSID_ERR_XTINIT_FAIL);

	XtSetLanguageProc(NULL, NULL, NULL);

	XtToolkitInitialize();
	app = XtCreateApplicationContext();
	XtAppSetFallbackResources(app, fallbacks);

	display = XtOpenDisplay(
		app, NULL, "sysidtool", "Sysidtool", NULL, 0, argcp, argv);

	if (display == (Display *)0) {
		XtDestroyApplicationContext(app);
		return (SYSID_ERR_XTINIT_FAIL);
	}

	toplevel = XtVaAppCreateShell(
	    "sysidtool", "Sysidtool", applicationShellWidgetClass, display,
		NULL);

#ifdef XEDITRES
	XtAddEventHandler(
	    toplevel, (EventMask)0, True, _XEditResCheckMessages, NULL);
#endif

	display = XtDisplayOfObject(toplevel);

	width = (Dimension)DisplayWidth(display, 0);
	height = (Dimension)DisplayHeight(display, 0);

	XtVaSetValues(toplevel,
		XmNx,			0,
		XmNy,			0,
		XmNwidth,		width,
		XmNheight,		height,
		XmNmappedWhenManaged,	False,
		NULL);

	/*
	 * We need to catch stream hangups (SIGPOLL) and remove
	 * the input handler on the read-side FIFO until we're
	 * awakened by SIGUSR1.  If we don't, we go into a tight
	 * loop reading a fifo with one of its ends closed, which
	 * causes read to immediately return 0 bytes.
	 */
	sa.sa_handler = start_stop;
	sa.sa_flags = 0;
	(void) sigemptyset(&sa.sa_mask);
	(void) sigaddset(&sa.sa_mask, SIGPOLL);
	(void) sigaddset(&sa.sa_mask, SIGUSR1);

	(void) close(0);

	(void) sigaction(SIGUSR1, &sa, (struct sigaction *)0);
	(void) sigaction(SIGPOLL, &sa, (struct sigaction *)0);

	(void) sigrelse(SIGUSR1);	/* just in case */
	(void) sigrelse(SIGPOLL);	/* just in case */
#ifdef DEV
	(void) signal(SIGINT, onintr);
#endif

	XtRealizeWidget(toplevel);

#if defined(QA_PARTNER)

	if (getenv("SYSIDTOOL_QAP")) {
		InitQaPartner(toplevel);
	}

#endif

	XtAppMainLoop(app);

	return (SYSID_SUCCESS);
}

void
do_cleanup(char *text, int *do_exit)
{
	if (*do_exit) {
		if (text != (char *)0 && text[0] != '\0') {
			/*
			 * We'll be nice and tack on a newline if
			 * there isn't one already...
			 */
			(void) fprintf(stderr, "%s%s", text,
				text[strlen(text)-1] != '\n' ? "\n" : "");
			(void) fflush(stderr);
		}
	} else if (text != (char *)0 && text[0] != '\0' && ui_active)
		(void) xm_create_working((Widget)0, text);

	ui_active = 0;	/* done with this sequence, reset for next sequence */
}

void
do_form(
	char		*title,
	char		*text,
	Field_desc	*fields,
	int		nfields,
	int		reply_to)
{
	static Field_help intro_help;
	static Field_help form_help;
	XmString ok;
	XmString cancel;
	XmString help;
	Widget	dialog;

	ui_active = 1;	/* started prompting in this sequence */

	xm_destroy_working();

	ok = XmStringCreateLocalized(CONTINUE_BUTTON);
	cancel = (XmString)0;
	help = XmStringCreateLocalized(HELP_BUTTON);

	dialog = form_create(toplevel, title, ok, cancel, help);
	form_common(dialog, text, fields, nfields);

	XtAddCallback(dialog, XmNokCallback, read_values, (XtPointer)reply_to);
	XtAddCallback(dialog, XmNcancelCallback, reset_values, (XtPointer)0);
	XtAddCallback(dialog, XmNhelpCallback,
			xm_help, (XtPointer)get_attr_help(
				(Sysid_attr)fields->user_data, &form_help));

	/*
	 * unmanage help button in some windows:
	 *	- the locale selection window
	 *      - any bad nis error message windows.
	 */
	if ((Sysid_attr)fields->user_data == ATTR_LOCALE ||
	    (Sysid_attr)fields->user_data == ATTR_BAD_NIS) {
		Widget help_button = XmMessageBoxGetChild(dialog,
			XmDIALOG_HELP_BUTTON);
		if (help_button)
			XtUnmanageChild(help_button);
	}

	if (is_install_environment() &&
	    (Sysid_attr)fields->user_data != ATTR_LOCALE) {	/* XXX */
		form_intro(dialog, fields, nfields, INTRO_TITLE, INTRO_TEXT,
					get_attr_help(ATTR_NONE, &intro_help));
	} else {
		XtManageChild(dialog);
		XtPopup(XtParent(dialog), XtGrabNone);
		xm_set_traversal(dialog, fields, nfields);
	}
}

/*
 * Find the widget in the fields list (if there is one) that has
 * requested initial keyboard focus and set it.
 */
void
xm_set_traversal(
	Widget dialog,
	Field_desc *fields,
	int nfields)
{
	char *tmp;
	int i;
	Widget w;
	Boolean ret;
	int cnt = 1000;

	for (i = 0; i < nfields; i++) {
		if (fields[i].flags & FF_KEYFOCUS) {
			tmp = malloc(strlen(fields[i].label) + 2);
			sprintf(tmp, "*%s", fields[i].label);
			w = XtNameToWidget(dialog, tmp);
			free(tmp);
			if (w) {
				(void) XmProcessTraversal(w,
					XmTRAVERSE_CURRENT);
				break;
			}
		}
	}
}

void
do_confirm(
	char		*title,
	char		*text,
	Field_desc	*fields,
	int		nfields,
	int		reply_to)
{
	static Field_help intro_help;
	static Field_help form_help;
	static XmString	yes;
	static XmString	no;
	static XmString	help;
	Widget	dialog;

	ui_active = 1;	/* started prompting in this sequence */

	xm_destroy_working();

	if (yes == (XmString)0) {
		yes = XmStringCreateLocalized(CONTINUE_BUTTON);
		no = XmStringCreateLocalized(CHANGE_BUTTON);
		help = XmStringCreateLocalized(HELP_BUTTON);
	}
	dialog = form_create(toplevel, title, yes, no, help);
	form_common(dialog, text, fields, nfields);

	XtVaSetValues(dialog,
		XmNdefaultButtonType,	XmDIALOG_CANCEL_BUTTON,
		NULL);

	XtAddCallback(dialog, XmNokCallback,
			send_confirm, (XtPointer)reply_to);
	XtAddCallback(dialog, XmNcancelCallback,
			send_confirm, (XtPointer)reply_to);
	XtAddCallback(dialog, XmNhelpCallback,
			xm_help, (XtPointer)get_attr_help(
						ATTR_CONFIRM, &form_help));

	if (is_install_environment()) {
		/*
		 * confirm's have no input text fields, so we don't
		 * need to pass fields, nfields.
		 */
		form_intro(dialog, NULL, 0, INTRO_TITLE, INTRO_TEXT,
					get_attr_help(ATTR_NONE, &intro_help));
	} else {
		XtManageChild(dialog);
		XtPopup(XtParent(dialog), XtGrabNone);
	}
}

void
do_error(char *errstr, int reply_to)
{
	static Widget	dialog;
	Widget		button;
	XmString	t;

	ui_active = 1;	/* started prompting in this sequence */

	xm_destroy_working();

	if (dialog == NULL) {
		dialog = XmCreateErrorDialog(
				toplevel, "errorDialog", (Arg *)0, 0);

		XtVaSetValues(XtParent(dialog),
			XmNtitle,	get_attr_title(ATTR_ERROR),
			NULL);

		t = XmStringCreateLocalized(OK_BUTTON);
		XtVaSetValues(dialog, XmNokLabelString, t, NULL);
		XmStringFree(t);

		button = XmMessageBoxGetChild(dialog, XmDIALOG_CANCEL_BUTTON);
		if (button != (Widget)0)
			XtUnmanageChild(button);

		button = XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON);
		if (button != (Widget)0)
			XtUnmanageChild(button);

		XtAddCallback(dialog,
			XmNokCallback, send_ok, (XtPointer)reply_to);
	}

	t = xm_format_text(errstr, XM_DEFAULT_COLUMNS, 0);
	XtVaSetValues(dialog, XmNmessageString, t, NULL);
	XmStringFree(t);

	XtManageChild(dialog);
	XtPopup(XtParent(dialog), XtGrabNone);
}

/*ARGSUSED*/
static void
xm_get_message(XtPointer	client_data,
		int		*source,
		XtInputId	*id)
{
	int	reply_to = (int)client_data;
	MSG	*mp;

	mp = msg_receive(*source);
	if (mp != (MSG *)0) {
#ifdef MSGDEBUG
		msg_dump(mp);
#endif /* MSGDEBUG */
		run_display(mp, reply_to);
	}
}

/*ARGSUSED*/
static void
read_values(Widget	widget,
	XtPointer	client_data,
	XtPointer	call_data)
{
	XmField		*fields, *xmf;
	Menu		*menu;
	char		*str;
	MSG		*mp;
	int		reply_to = (int)client_data;
	Widget		main_form;
	Widget		data_form;
	int		errors;

	errors = 0;

	XtVaGetValues(widget, XmNuserData, &main_form, NULL);
	XtVaGetValues(main_form, XmNuserData, &data_form, NULL);
	XtVaGetValues(data_form, XmNuserData, &fields, NULL);

	mp = msg_new();
	(void) msg_set_type(mp, REPLY_OK);

	for (xmf = fields; xmf != (XmField *)0; xmf = xmf->xf_next) {
		Field_desc *f = xmf->xf_desc;

		if (f->flags & FF_RDONLY)
			continue;

		xm_get_value(xmf->xf_value, f);

		if (f->validate != (Validate_proc *)0 &&
		    xm_validate_value(widget, f) != SYSID_SUCCESS) {
			errors++;
			break;
		}

		switch (f->type) {
		case FIELD_TEXT:
			if (f->value == (void *)0)
				str = "";
			else
				str = (char *)f->value;
			(void) msg_add_arg(mp,
				(Sysid_attr)f->user_data, VAL_STRING,
				(void *)str, strlen(str) + 1);
			break;
		case FIELD_EXCLUSIVE_CHOICE:
			menu = (Menu *)f->value;

			(void) msg_add_arg(mp,
				(Sysid_attr)f->user_data, VAL_INTEGER,
				(void *)&menu->selected, sizeof (int));
			break;
		case FIELD_CONFIRM:
			(void) msg_add_arg(mp,
				(Sysid_attr)f->user_data, VAL_INTEGER,
				(void *)&f->value, sizeof (f->value));
			break;
		}
	}

	if (errors == 0) {
		(void) msg_send(mp, reply_to);
		form_destroy(widget);
	}

	msg_delete(mp);
}

/*ARGSUSED*/
static void
reset_values(Widget	widget,
	XtPointer	client_data,
	XtPointer	call_data)
{
	XmField		*fields;
	XmField		*xmf;
	XmField		*xmf_sum;
	Widget		main_form;
	Widget		data_form;
	Widget		button;

	XtVaGetValues(widget, XmNuserData, &main_form, NULL);
	XtVaGetValues(main_form, XmNuserData, &data_form, NULL);
	XtVaGetValues(data_form, XmNuserData, &fields, NULL);

	xmf_sum = (XmField *)0;

	for (xmf = fields; xmf != (XmField *)0; xmf = xmf->xf_next) {
		Field_desc *f = xmf->xf_desc;

		switch (f->type) {
		case FIELD_TEXT:
			if (f->flags & FF_RDONLY) {
				if (f->flags & FF_SUMMARY)
					xmf_sum = xmf;
				break;
			}
			XtVaGetValues(XtParent(xmf->xf_value),
				XmNuserData,	&f->value,
				NULL);
			XtVaSetValues(xmf->xf_value,
				XmNvalue,	f->value,
				NULL);
			break;
		case FIELD_CONFIRM:
		case FIELD_EXCLUSIVE_CHOICE:
			XtVaGetValues(xmf->xf_value,
				XmNuserData,	&button,
				NULL);
			XmToggleButtonGadgetSetState(button, True, True);
			break;
		}
	}
	update_summary(xmf_sum);
}

static void
send_confirm(Widget	widget,
	XtPointer	client_data,
	XtPointer	call_data)
{
	/* LINTED [alignment ok] */
	XmAnyCallbackStruct *cbs = (XmAnyCallbackStruct *)call_data;
	int	reply_to = (int)client_data;
	int	confirm;
	MSG	*mp;

	switch (cbs->reason) {
	case XmCR_OK:
		confirm = TRUE;
		break;
	case XmCR_CANCEL:
	default:
		confirm = FALSE;
		break;
	}
	form_destroy(widget);

	mp = msg_new();
	(void) msg_set_type(mp, REPLY_OK);
	(void) msg_add_arg(mp, ATTR_CONFIRM, VAL_BOOLEAN,
					(void *)&confirm, sizeof (confirm));
	(void) msg_send(mp, reply_to);
	msg_delete(mp);
}

/*ARGSUSED*/
static void
send_ok(Widget	widget,
	XtPointer	client_data,
	XtPointer	call_data)
{
	int	reply_to = (int)client_data;
	MSG	*mp;

	mp = msg_new();
	(void) msg_set_type(mp, REPLY_OK);
	(void) msg_send(mp, reply_to);
	msg_delete(mp);
}

void
do_message(MSG *mp, int reply_to)
{
	Widget	widget;
	char	str[1024];

	(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
				(void *)&widget, sizeof (widget));
	(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
				(void *)str, sizeof (str));
	msg_delete(mp);

	/*
	 * Only show the user messages if we've prompted
	 * the user for something during this sequence.
	 * This prevents apparently superfluous status
	 * messages from popping up and down.
	 */
	if (ui_active)
		widget = xm_create_working(widget, str);
	else
		widget = (Widget)0;

	/*
	 * reply with widget id of popup containing message
	 */
	mp = msg_new();
	(void) msg_set_type(mp, REPLY_OK);
	(void) msg_add_arg(mp, ATTR_PROMPT, VAL_INTEGER,
				(void *)&widget, sizeof (widget));
	(void) msg_send(mp, reply_to);
	msg_delete(mp);
}

void
do_dismiss(MSG *mp, int reply_to)
{
	Widget	widget;

	(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
				(void *)&widget, sizeof (widget));
	msg_delete(mp);

	/*
	 * the window referenced by widget has already been
	 * destroyed in the form_destroy funtion in file xm_form.c,
	 * since the widget had already been popped down and
	 * destroyed (using XtPopdown and XtDestroyWidget, it was
	 * unnecessary to do this here), this was the cause of
	 * bug id 1199286, removing the following lines of code
	 * resolves the problem
	 *
	 *	if (widget) {
	 *		XtPopdown(XtParent(widget));
	 *		XtDestroyWidget(widget);
	 *	}
	 *
	 * it was possible that widget was still defined, but
	 * that it pointed to nothing, leaving widget dangling
	 */

	mp = msg_new();
	(void) msg_set_type(mp, REPLY_OK);
	(void) msg_send(mp, reply_to);
	msg_delete(mp);
}
