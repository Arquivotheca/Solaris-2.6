/*
 * Copyright (c) 1993 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)ui_layout.c	1.27	96/06/25 SMI"

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include <signal.h>
#include <ucontext.h>
#include <nl_types.h>
#include <sys/systeminfo.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/procset.h>
#include <X11/Intrinsic.h>
#include <X11/cursorfont.h>
#include <Xm/Form.h>
#include <Xm/LabelG.h>
#include <Xm/Label.h>
#include <Xm/MessageB.h>
#include <Xm/PushB.h>
#include <Xm/DrawnB.h>
#include <Xm/ToggleB.h>
#include <Xm/MenuShell.h>
#include <Xm/TextF.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include "app_data.h"
#include "protos.h"
#include "adminhelp.h"
#include "launcher.h"

#define	RES_CONVERT( res_name, res_value) \
	XtVaTypedArg, (res_name), XmRString, (res_value), strlen(res_value) + 1

/* file descriptor for /dev/null, useful for dup2() before exec */
extern int	devnull;

/* launcher pid for sigsend() */
extern pid_t	launcher_pid;

/* Toplevel widget, for un/setting busy (watch) cursor */
extern Widget		toplevel;
extern XtAppContext	app_context;

/* one second timeout for timer proc interval */
extern const unsigned long	one_sec;

extern nl_catd	catd; /* for catgets(), defined in main.c */

/* just to satisfy UxXt.c */
Widget		UxTopLevel;

/* signal state */
static Boolean		sigchld_initialized = False;
static pid_t		sigusr1_pid = -1;
static pid_t		sigchld_pid = -1;
static int		sigchld_status = -1;

static char		error_app_name[PATH_MAX];

/* Number of apps that are "launching" but not yet mapped to screen */
static int		apps_launching = 0;


/* Nameservice settings. */
static Widget		menu_button_nis_plus;
static Widget		menu_button_nis;
static Widget		menu_button_none;
static Widget		ns_menu;
static Widget		domain_label;
static Widget		domain_text;
static char		Domain[256];
static char		Host[256];

static void
set_busy(Display *d, Window w, Bool make_busy)
{

	static Cursor	watch = 0;


	if (make_busy) {

		/*
		 * Turn on the busy cursor.  Check first to see if the
		 * watch has been successfully created; if no, try
		 * to create it here and then "define" the window to use
		 * it.  If it has already been created, just define it.
		 */

		if (watch == 0) {

			if ((watch = XCreateFontCursor(d, XC_watch)) != 0) {
				XDefineCursor(d, w, watch);
				XFlush(d);
			}
		} else {
			XDefineCursor(d, w, watch);
			XFlush(d);
		}
	} else {

		/*
		 * Turn off the busy cursor, if it has been successfully
		 * created.  If it hasn't been created there's no way
		 * it could have been set on the window.
		 */

		if (watch != 0) {
			XUndefineCursor(d, w);
			XFlush(d);
		}
	}
}


/*ARGSUSED*/
void
app_mapped_prop_notify(XEvent *ev)
{

	Atom		actual_type;
	int		actual_format;
	unsigned long	nitems;
	unsigned long	bytes_after;
	unsigned char	*prop;


	if (XGetWindowProperty(ev->xproperty.display,
			       ev->xproperty.window,
			       ev->xproperty.atom,
			       0L, 1L, False, XA_INTEGER,
			       &actual_type, &actual_format, &nitems,
			       &bytes_after, &prop) == Success) {

		if (*(int *)prop == getpid()) {

			/*
			 * A child of this launcher just changed the
			 * property, meaning that its toplevel is mapped.
			 * Decrement the number of apps that this launcher
			 * has in the "launching" state, and if it goes
			 * to 0, change the cursor back to "not busy".
			 */

			if (--apps_launching == 0) {
				set_busy(XtDisplay(toplevel),
					 XtWindow(toplevel), False);
			}
		}
	}
}


/*ARGSUSED*/
void
dialog_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	/*
	 * This is a convenience dialog's callback function.
	 * 'w' actually refers to the message box widget, not
	 * the "OK" button that was actually pressed.  We want
	 * to destroy the dialog, but there seem to be some
	 * bugs that prevent us from doing this properly.
	 *
	 * When the convenience routine is used to create the
	 * dialog, a function is added to the message box's
	 * XmNdestroyCallback list to clean up the DialogShell
	 * parent of the box.  However, if you have set any
	 * additional translations on some of this stuff,
	 * you'll get a crash in some of the destroy calls.
	 */

	/*
	 * HACK to avoid crashing on Solaris 1093 if translation
	 * resources have been added to widgets in the dialog tree:
	 * remove the destroy callback list from the message box,
	 * which will remove the shell destroy function that the
	 * convenience routine added, then destroy the shell itself
	 * and let it do the clean up.
	 * See bugid 1146055; when it's fixed, these two lines should
	 * be deleted and replaced with
	 *
	 *	XtDestroyWidget(w);
	 */

	XtRemoveAllCallbacks(w, XmNdestroyCallback);
	XtDestroyWidget(XtParent(w));
}


void
show_error_dialog(const char *msg)
{

	Widget		error_dialog;
	XmString	xm_msg;


	if ((error_dialog =
	     XmCreateErrorDialog(toplevel, "fork_exec_error", NULL, 0)) == 0) {

		fprintf(stderr, "%s", msg);
		return;
	}

	XtVaSetValues(XtParent(error_dialog),
		      XtNtitle,	catgets(catd, 1, 17, "Administration Tool: Error"),
		      NULL);

	xm_msg = XmStringCreateLocalized((char*)msg);

	XtVaSetValues(error_dialog,
		      XmNautoUnmanage,		False,
		      XmNdialogStyle,		XmDIALOG_FULL_APPLICATION_MODAL,
		      XmNmessageAlignment,	XmALIGNMENT_CENTER,
		      XmNmessageString,		xm_msg,
		      NULL);

	XmStringFree(xm_msg);

	XtUnmanageChild(XmMessageBoxGetChild(error_dialog,
					     XmDIALOG_CANCEL_BUTTON));
	XtUnmanageChild(XmMessageBoxGetChild(error_dialog,
					     XmDIALOG_HELP_BUTTON));

	XtAddCallback(error_dialog, XmNokCallback, dialog_cb,
		      (XtPointer)XtParent(error_dialog));

	/*
	 * The Motif "convenience" routine set-up takes care of doing
	 * the XtPopup() on the error_dialog's DialogShell (its parent)
	 * when the child is managed, that's why you don't see any
	 * XtPopup() call here.
	 */

	XtManageChild(error_dialog);

	/* I want to get this displayed immediately and don't trust Motif */

	XFlush(XtDisplayOfObject(error_dialog));
}


void
exec_fail_sigusr1_handler(int sig, siginfo_t *info, ucontext_t *context)
{
	/*
	 * Save info and get out as quickly as possible; we have to
	 * let the work proc deal with it as we can't hang out for
	 * long in a signal handler when working with X.
	 */

	/*
	 * This should be a user generated SIGUSR1 when a forked
	 * process failed to exec.  Save the pid for later use
	 * in the work proc, where it determines whether an exit
	 * was normal or the result of a failed exec.
	 */

	if (info != NULL) {

		/*
		 * If user-process generated signal, save pid;
		 * should always be true, but check to make sure.
		 */

		if (info->si_code <= 0) {
			sigusr1_pid = info->si_pid;
		} else {
			sigusr1_pid = -1;
		}

		sigchld_initialized = True;
	}
}


void
exec_fail_sigchld_handler(int sig, siginfo_t *info, ucontext_t *context)
{
	/*
	 * Save info and get out as quickly as possible; we have to
	 * let the work proc deal with it as we can't hang out for
	 * long in a signal handler when working with X.
	 */

	/*
	 * This should be a kernel generated SIGCHLD when one of
	 * the forked child processes exits.  It may be due to
	 * either a normal child termination, or a process that
	 * failed to exec doing an explicit _exit().  Save the
	 * siginfo structure (which contains the child pid) for
	 * later testing in the work proc; if in the work proc
	 * the saved siginfo pid matches the pid that was saved
	 * in the sigusr handler, we know it was a failed exec
	 * and we'll display an error dialog.
	 */

	if (info != NULL) {

		/*
		 * If kernel generated signal, save pid; this
		 * should always be true, but check to make sure.
		 */

		if (info->si_code > 0) {
			sigchld_pid = info->si_pid;
		} else {
			sigchld_pid = -1;
		}

		sigchld_status = info->si_status;

		sigchld_initialized = True;
	}
}


/*ARGSUSED*/
void
exec_fail_timer_proc(XtPointer client_data)
{

	char		msg[512];
	const char	*exec_eacces_msg =
		catgets(catd, 1, 18, "Sorry, inappropriate permission to run \"%s\"");
	const char	*exec_enoent_msg =
		catgets(catd, 1, 19, "Sorry, the specified program \"%s\" does not exist");
	const char	*exec_enomem_msg =
		catgets(catd, 1, 20, "Sorry, insufficient memory available to run \"%s\"");
	const char	*exec_failed_msg =
		catgets(catd, 1, 21, "Sorry, insufficient system resources available to run \"%s\" (exec failed, errno %d)");


	if (! sigchld_initialized) {

		/* Do nothing, re-register timeout */
		
		(void) XtAppAddTimeOut(app_context, one_sec,
				       exec_fail_timer_proc, NULL);
		return;
	}

	if (sigusr1_pid == sigchld_pid) {

		switch (sigchld_status) {
		case EACCES:
			sprintf(msg, exec_eacces_msg, error_app_name);
			break;
		case ENOENT:
			sprintf(msg, exec_enoent_msg, error_app_name);
			break;
		case ENOMEM:
			sprintf(msg, exec_enomem_msg, error_app_name);
			break;
		default:
			sprintf(msg, exec_failed_msg, error_app_name, sigchld_status);
			break;
		}

		/* reset sigusr pid, we've handled this failure */
		sigusr1_pid = -1;

		/*
		 * Turn launcher clock back to normal pointer and decrement
		 * launching app count.
		 */

		if (--apps_launching == 0) {
		    set_busy(XtDisplay(toplevel), XtWindow(toplevel), False);
		}

		show_error_dialog((const char *)msg);
		sigchld_initialized = False;
	}

	(void) XtAppAddTimeOut(app_context, one_sec,
			       exec_fail_timer_proc, NULL);
}


/*
 * exec_callback -- execute the program specified by the
 * app_data_t structure passed in via callback client_data.
 */

/*ARGSUSED2*/
static void
exec_callback(Widget w, XtPointer client_data, XmDrawnButtonCallbackStruct  *call_data)
{

	Widget			nsWidget;
	char			*text;
	app_data_t		*app_data = (app_data_t *)client_data;
	admin_geometry_t	d = {0, 0, 0, 0};
	char			root_class[128];
	char			parent_class[128];
	char			app_data_parent_class[128];
	char			*cur_class;
	char			*(classes[128]);
	char			*(values[128]);
	int			count;
	char			obj[256];
	char			ns[25];


	/* Only allow a double click to initiate the default action. */
	/* A single click only selects the image. */
	if (call_data->click_count != 2)
		return;

	if (app_data ) {

		/* Pick-up the current Naming Service settings. */
		XtVaGetValues(ns_menu, 
			XmNmenuHistory, &nsWidget,
			NULL);

		/* Get the domain from the display field. */
		text = XmTextFieldGetString(domain_text);

		/* Set the name service var. and reset the UI Domain or
		 * Host value.
		 */
		if (nsWidget == menu_button_nis_plus)
		{
			strcpy(ns, "nisplus");
			strncpy(Domain, text, sizeof(Domain));

		} else if (nsWidget == menu_button_nis)
		{
			strcpy(ns, "nis");
			strncpy(Domain, text, sizeof(Domain));
		}
		else if (nsWidget == menu_button_none)
		{
			strcpy(ns, "ufs");
			strncpy(Host, text, sizeof(Host));
		}


		/* Get the root class. */
		if (admc_find_rootclass(root_class, sizeof(root_class)) != 
	    	    E_ADMIN_OK)
			return;

		count = 0;

		admc_get_parentname(app_data->class, app_data_parent_class, 
			sizeof(app_data_parent_class));
		cur_class = app_data_parent_class;


		/* We need to create an array of all class values we will 
		 * need for the passed in class to specify the class argument
		 * to admin_execute_method.  We do this by requesting the 
		 * parents of the current class until we hit the 
		 * root class.
		 */ 
		while (strcmp(cur_class, root_class) != 0) {
			admc_get_parentname(cur_class, parent_class, 
				sizeof(parent_class));
			classes[count] =  strdup(parent_class);
			/* We only have the domain value currently 
			 * available through the launcher.  All other 
			 * values will be set to the empty string.
			 */
			if (strcmp(parent_class, "Domain") == 0)
				values[count] = strdup(text);
			else
				values[count] = NULL;

			cur_class = parent_class;
			count++;
		} 

		/* Create the class argument from the constructed 
		 * class and value arrays.
		 */
		if (strcmp(app_data_parent_class, root_class) == 0) {
			if (make_object_from_values(obj, sizeof(obj),
				app_data_parent_class, text, 
	    			NULL,
				NULL,
				0) != E_ADMIN_OK)
				return;
		} else {
			if (make_object_from_values(obj, sizeof(obj),
				app_data_parent_class, "", 
	    			(const char **) classes,
				(const char **) values,
				count) != E_ADMIN_OK)
				return;
		}

		/* Invoke the default action for the selected class.
		 * The default action is hardcoded to the list method.
		 */

		/* ADD ERROR DETECTION !!! */
		admin_execute_method(ns, "snag",
			app_data->class, "list", obj, d, NULL);


		set_busy(XtDisplay(toplevel),
				 XtWindow(toplevel), True);

		apps_launching++;

	}
}


/*ARGSUSED*/
static void
confirm_ok_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	exit(0);
}


/*ARGSUSED*/
static void
confirm_cancel_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	/*
	 * HACK to avoid crashing on Solaris 1093 if translation
	 * resources have been added to widgets in the dialog tree:
	 * remove the destroy callback list from the message box,
	 * which will remove the shell destroy function that the
	 * convenience routine added, then destroy the shell itself
	 * and let it do the clean up.
	 * See bugid 1146055; when it's fixed, these two lines should
	 * be deleted and replaced with
	 *
	 *	XtDestroyWidget(w);
	 */

	XtRemoveAllCallbacks(w, XmNdestroyCallback);
	XtDestroyWidget(XtParent(w));
}


void
do_exit_confirm_dialog()
{

	Widget		confirm_dialog;
	XmString	xm_msg;


	if ((confirm_dialog =
	     XmCreateQuestionDialog(toplevel, "exitConfirm", NULL, 0)) == 0) {
		exit(1);
	}

	XtVaSetValues(XtParent(confirm_dialog),
		      XtNtitle,	catgets(catd, 1, 22, "Administration Tool: Exit"),
		      NULL);

	xm_msg = XmStringCreateLocalized(catgets(catd, 1, 23, "Really quit Administration Tool?"));

	XtVaSetValues(confirm_dialog,
		      XmNautoUnmanage,		False,
		      XmNdialogStyle,		XmDIALOG_FULL_APPLICATION_MODAL,
		      XmNmessageAlignment,	XmALIGNMENT_CENTER,
		      XmNmessageString,		xm_msg,
		      XmNdefaultButtonType,	XmDIALOG_CANCEL_BUTTON,
		      NULL);

	XmStringFree(xm_msg);

	XtUnmanageChild(XmMessageBoxGetChild(confirm_dialog,
					     XmDIALOG_HELP_BUTTON));

	XtAddCallback(confirm_dialog, XmNokCallback, confirm_ok_cb,
		      (XtPointer)XtParent(confirm_dialog));

	XtAddCallback(confirm_dialog, XmNcancelCallback, confirm_cancel_cb,
		      (XtPointer)XtParent(confirm_dialog));

	/*
	 * The Motif "convenience" routine set-up takes care of doing
	 * the XtPopup() on the confirm_dialog's DialogShell (its parent)
	 * when the child is managed, that's why you don't see any
	 * XtPopup() call here.
	 */

	XtManageChild(confirm_dialog);

	/* I want to get this displayed immediately and don't trust Motif */

	XFlush(XtDisplayOfObject(confirm_dialog));
}


/*
 * file_callback -- callback for the "File" menu on the menubar.
 * The only item currently in the menu is "exit".
 */

/*ARGSUSED*/
static void
file_callback(Widget w, int item_no, XmAnyCallbackStruct *cbs)
{
	/*
	 * Only item is "exit", so exit.
	 *
	 * NOTE:  There is a Motif "question dialog" that can
	 * be brought up to require the user to confirm the
	 * exit; if this is desired, replace the following
	 * call to exit() with a call to do_exit_confirm_dialog().
	 * The behavior at this time is to go ahead and exit
	 * without confirmation if the user selects "exit" from
	 * the window, but if a WM_DELETE_WINDOW is received
	 * on "toplevel" from a wm (which is terribly error
	 * prone with twm, I know!) go ahead and do the confirm
	 * dialog.
	 */

	exit(0);
}


/*
 * help_callback -- callback for the "Help" menu on the menubar.
 */

/*ARGSUSED2*/
static void
help_callback(Widget w, int item_no, XmAnyCallbackStruct *cbs)
{

	char	*helpfile = NULL;
	char	*s;
	char	type;


	switch (item_no) {
	case 0:
		/* launcher */
		helpfile = "atover.t.hlp";
		break;
	}

	if (helpfile != NULL) {
		s = strchr(helpfile, '.');
		type = s ? *(s+1) : 't';
		switch (type) {
		  case 'h':
			type = HOWTO;
			break;
		  case 'r':
			type = REFER;
			break;
		  default:
			type = TOPIC;
			break;
		};
		adminhelp(toplevel, type, helpfile);
	}
}


void 
nameServiceNISPlusCB(
		Widget widget, 
		XtPointer cd, 
		XtPointer cbs)
{

	XmString		str;

	str = XmStringCreateSimple(catgets(catd, 1, 24, "Domain:"));
	XtVaSetValues(domain_label, 
		XmNlabelString, str,
		NULL);
	XmStringFree(str);
	XmTextSetString(domain_text, Domain);

}

void 
nameServiceNISCB(	
		Widget widget, 
		XtPointer cd, 
		XtPointer cbs)
{

	XmString		str;


	str = XmStringCreateSimple(catgets(catd, 1, 24, "Domain:"));
	XtVaSetValues(domain_label, 
		XmNlabelString, str,
		NULL);
	XmStringFree(str);
	XmTextSetString(domain_text, Domain);

}


void 
nameServiceNoneCB(		
		Widget widget, 
		XtPointer cd, 
		XtPointer cbs)

{

	XmString		str;


	str = XmStringCreateSimple(catgets(catd, 1, 25, "Location:"));
	XtVaSetValues(domain_label, 
		XmNlabelString, str,
		NULL);
	XmStringFree(str);
	XmTextSetString(domain_text, Host);

}


/*
 * create_menu_bar -- create the menu bar and items for the
 * launcher app.  Return the menu bar widget.
 */

static Widget
create_menu_bar(Widget parent)
{

	Widget		mb;
	Widget		m;
	Widget		cascade_button;
	XmString	file, help, ext;
	XmString	at;


	/*
	 * XXX NOTE: all of these string create calls need to be
	 * updated to XmStringCreateLocalize()!
	 */

	file = XmStringCreateLocalized(catgets(catd, 1, 26, "File"));
	help = XmStringCreateLocalized(catgets(catd, 1, 27, "Help"));

	/*
	 * Create the menu bar, attach it to top, left, and right of
	 * its parent form.  ASSUMES THAT PARENT IS A FORM!
	 */

	mb = XmVaCreateSimpleMenuBar(parent, "menubar",
				     XmVaCASCADEBUTTON, file, 'F',
				     XmVaCASCADEBUTTON, help, 'H',
				     /* form constraints */
				     XmNtopAttachment,		XmATTACH_FORM,
				     XmNleftAttachment,		XmATTACH_FORM,
				     XmNrightAttachment,	XmATTACH_FORM,
				     NULL);
	XmStringFree(file);
	XmStringFree(help);

	if (mb == NULL)
		return (NULL);

	/* Create the 'File' menu */

	ext = XmStringCreateLocalized(catgets(catd, 1, 28, "Exit"));

	m = XmVaCreateSimplePulldownMenu(mb, "file_menu", 0, file_callback,
					 XmVaPUSHBUTTON, ext, 'x', NULL, NULL,
					 NULL);

	XmStringFree(ext);

	if (m == NULL)
		return (NULL);

	/* Create the 'Help' menu */

	at = XmStringCreateLocalized(catgets(catd, 1, 29, "About Administration Tool..."));
	XmStringFree(at);

	if (m == NULL)
		return (NULL);

	/* Push the help menu down to the right side */

	cascade_button = XtNameToWidget(mb, "button_1");

	XtVaSetValues(mb, XmNmenuHelpWidget, cascade_button, NULL);

	XtManageChild(mb);

	return (mb);
}

/*
 * create_welcome -- create the "Welcome to Admintool..." label.
 */

static Widget
create_welcome(
	Widget			parent,
	Widget			top_widget)
{

	Widget		field;
	Widget		lw;
	XmString	label;

	/*
	 * Create the field form, this one to manage the relative
	 * layout of the "welcome" label and the button field.
	 */

	field = XtVaCreateManagedWidget("form",
					xmFormWidgetClass, parent,
					XmNhorizontalSpacing,	24,
					XmNverticalSpacing,	24,
					XmNrubberPositioning,	True,
					/* form constraints */
					XmNtopAttachment,	XmATTACH_WIDGET,
					XmNtopWidget,		top_widget,
					XmNleftAttachment,	XmATTACH_FORM,
					XmNrightAttachment,	XmATTACH_FORM,
					NULL);

	if (field == NULL)
		return (NULL);

	/*
	 * Create the "Welcome ..." label, stick it to the
	 * top, left, and right of the field.
	 */

	label = XmStringCreateLocalized(catgets(catd, 1, 30, "Welcome to Administration Tool 2.0"));

	lw = XtVaCreateManagedWidget("label",
				     xmLabelGadgetClass, field,
				     XmNlabelString,	label,
				     /* form constraints */
				     XmNtopAttachment,		XmATTACH_FORM,
				     XmNleftAttachment,		XmATTACH_FORM,
				     XmNrightAttachment,	XmATTACH_FORM,
				     NULL);
	XmStringFree(label);

	return (lw);
}



/*
 * create_context_area -- create the context area of the launcher.
 */

static Widget
create_context_area(
	Widget			parent,
	Widget			top_widget)
{

	Widget		context_area;
	Widget		menu_shell;
	Widget		menu_pane;

	/*
	 * Create the form to manage the name service selection
	 * widgets.
	 */

	context_area = XtVaCreateManagedWidget("context_area",
					xmFormWidgetClass, parent,
					XmNrubberPositioning,	True,
					/* form constraints */
					XmNtopAttachment,	XmATTACH_WIDGET,
					XmNtopWidget,		top_widget,
					XmNtopOffset,		20,
					XmNleftAttachment,	XmATTACH_FORM,
					XmNrightAttachment,	XmATTACH_FORM,
					NULL);

	if (context_area == NULL)
		return (NULL);


	menu_shell = XtVaCreatePopupShell ("menu_shell",
			xmMenuShellWidgetClass, context_area,
			XmNwidth, 1,
			XmNheight, 1,
			XmNallowShellResize, TRUE,
			XmNoverrideRedirect, TRUE,
			NULL );

	menu_pane = XtVaCreateWidget( "menu_pane",
			xmRowColumnWidgetClass,
			menu_shell,
			XmNrowColumnType, XmMENU_PULLDOWN,
			NULL );

	menu_button_nis_plus = XtVaCreateManagedWidget( "menu_button_nis_plus+",
			xmPushButtonGadgetClass,
			menu_pane,
			RES_CONVERT( XmNlabelString, "NIS+   " ),
			NULL );

	menu_button_nis = XtVaCreateManagedWidget( "menu_button_nis",
			xmPushButtonGadgetClass,
			menu_pane,
			RES_CONVERT( XmNlabelString, "NIS" ),
			NULL );

	menu_button_none = XtVaCreateManagedWidget( "menu_button_none",
			xmPushButtonGadgetClass,
			menu_pane,
			RES_CONVERT( XmNlabelString, "UFS" ),
			NULL );

	ns_menu = XtVaCreateManagedWidget( "ns_menu",
			xmRowColumnWidgetClass,
			context_area,
			XmNrowColumnType, XmMENU_OPTION,
			XmNsubMenuId, menu_pane,
			RES_CONVERT( XmNlabelString, catgets(catd, 1, 31, "Naming Service:") ),
			XmNleftAttachment, XmATTACH_FORM,
			XmNleftOffset, 80,
			XmNtopAttachment, XmATTACH_FORM,
			XmNtopOffset, 15,
			XmNbottomAttachment, XmATTACH_FORM,
			NULL );

	domain_label = XtVaCreateManagedWidget( "domain_label",
			xmLabelWidgetClass,
			context_area,
			RES_CONVERT( XmNlabelString, catgets(catd, 1, 24, "Domain:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, ns_menu,
			XmNbottomOffset, 10,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, ns_menu,
			XmNleftOffset, 20,
			NULL );

	domain_text = XtVaCreateManagedWidget( "domain_text",
			xmTextFieldWidgetClass,
			context_area,
			XmNvalue, Domain,
			XmNmaxLength, 80,
			XmNeditable, TRUE,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, domain_label,
			XmNbottomOffset, -7,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, domain_label,
			XmNleftOffset, 5,
			XmNrightAttachment, XmATTACH_FORM,
			XmNrightOffset, 80,
			NULL );


	XtAddCallback( menu_button_nis_plus, XmNactivateCallback,
		(XtCallbackProc) nameServiceNISPlusCB,
		NULL );

	XtAddCallback( menu_button_nis, XmNactivateCallback,
		(XtCallbackProc) nameServiceNISCB,
		NULL );

	XtAddCallback( menu_button_none, XmNactivateCallback,
		(XtCallbackProc) nameServiceNoneCB,
		NULL );


	return (context_area);
}


/*
 * create_field -- create the field of labels/buttons for
 * the launchable applications.  Data is specifed in the
 * app_data array.
 */

static Widget
create_field(
	Widget			parent,
	Widget			top_widget,
	const app_data_t	*app_data,
	int			app_data_cnt)
{

	int		i;
	int		rows, cols;
	int		x, y;
	int		pos_x, pos_y;
	unsigned int	width, height;
	Widget		w;
	Widget		bmgr;
	Widget		wrapper;
	Pixmap		pix_image;
	Pixel 		fg, bg;
	char		name[16];
	XmString	label;
	int		j;
	int		num_methods;
	char		**methods;


	/*
	 * Create a manager for the image buttons and labels.
	 * We want to make this thing square-ish, however no
	 * fewer than 2 columns.
	 */

	rows = sqrt((double)app_data_cnt);
	cols = app_data_cnt / rows;
	while (rows * cols < app_data_cnt)
		cols++;

	if (cols < 2)
		cols = 2;

	bmgr = XtVaCreateManagedWidget("buttonMgr",
				       xmFormWidgetClass, parent,
				       XmNfractionBase,	rows * cols,
				       /* form constraints */
				       XmNtopAttachment,	XmATTACH_WIDGET,
				       XmNtopWidget,		top_widget,
				       XmNleftAttachment,	XmATTACH_FORM,
				       XmNrightAttachment,	XmATTACH_FORM,
				       XmNbottomAttachment,	XmATTACH_FORM,
				       XmNtopOffset,		25,
				       XmNbottomOffset,		25,
				       NULL);

	if (bmgr == NULL)
		return (NULL);


	pos_x = pos_y = 0;

	for (i = 0; i < app_data_cnt; i++) {

		/* IF there are no class methods registered for  
  		 * the class, continue with the next class.
		 */
		methods = adma_get_class_methods(app_data[i].class, 
			&num_methods);
		if (num_methods ==0)
			continue;

		/* Free the method array. */
		for (j = 0; i < num_methods; j++) {
			free(methods[j]);
		}
		free(methods);


		/*
		 * Create a wrapper for the button and label.  The idea
		 * here is to have the button sized no larger than its
		 * image, and centered above its label (the label strings
		 * tend to be longer than the image is wide for our
		 * current 48x48 images).  This code is messy, but
		 * trust me, it mostly works.
		 */

		(void) sprintf(name, catgets(catd, 1, 32, "wrapper%d"), i);
		wrapper = XtVaCreateManagedWidget(name, xmFormWidgetClass,
						  bmgr,
						  XmNfractionBase,	3,
						  /* form constraints */
						  XmNtopAttachment,
					       		XmATTACH_POSITION,
						  XmNtopPosition,
					       		pos_y * cols,
						  XmNleftAttachment,
					       		XmATTACH_POSITION,
						  XmNleftPosition,
					       		pos_x * rows,
						  XmNrightAttachment,
					       		XmATTACH_POSITION,
						  XmNrightPosition,
					       		(pos_x + 1) * rows,
						  XmNbottomAttachment,
					       		XmATTACH_POSITION,
						  XmNbottomPosition,
					       		(pos_y + 1) * cols,
						  NULL);

		if (wrapper == NULL)
			return (NULL);

		/*
		 * If row is full, reset column and increment row
		 * (prepare to layout next row).
		 */

		if (++pos_x >= cols) {
			pos_x = 0;
			pos_y++;
		}


		/*
		 * Now create a pixmap that has the same depth as the
		 * windows into which we will render the image.  This
		 * is necessary when using images in Motif buttons, they
		 * won't take depth mis-matches.
		 */

		XtVaGetValues(wrapper, 
			XmNforeground, &fg,
			XmNbackground, &bg,
			NULL);
		pix_image = XmGetPixmap(XtScreen(bmgr), 
			(char*)app_data[i].icon_image, fg, bg);

		if (pix_image == XmUNSPECIFIED_PIXMAP)
			continue;

		/*
		 * Create the image button, leave a bit of space between
		 * the top of the button and the top of its wrapper so
		 * that we'll get some vertical spacing between app buttons.
		 */

		w = XtVaCreateManagedWidget(app_data[i].icon_image,
					    xmDrawnButtonWidgetClass, wrapper,
					    XmNlabelType,	XmPIXMAP,
			      		    XmNlabelPixmap,	pix_image,
					    XmNmultiClick, XmMULTICLICK_KEEP,
					    /* form constraints */
					    XmNtopAttachment,	XmATTACH_FORM,
					    XmNtopOffset,	16,
					    XmNleftAttachment,
					    		XmATTACH_POSITION,
					    XmNleftPosition,	1,
					    XmNrightAttachment,
					    		XmATTACH_POSITION,
					    XmNrightPosition,	2,
					    NULL);

		if (w == NULL)
			continue;

		/* Add the callback to the button that will exec the app. */
		XtAddCallback(w, XmNactivateCallback, exec_callback,
			      &app_data[i]);

		/*
		 * Create a label and place it 8 pixels below the button,
		 * attach to both sides of the wrapper so that the label
		 * will center.
		 */

		label = XmStringCreateLocalized((char*)app_data[i].icon_text);
		w = XtVaCreateManagedWidget(app_data[i].icon_text,
					    xmLabelGadgetClass, wrapper,
					    XmNlabelString,	label,
					    XmNtopAttachment,	XmATTACH_WIDGET,
					    XmNtopWidget,	w,
					    XmNtopOffset,	8,
					    XmNleftAttachment,	XmATTACH_FORM,
					    XmNrightAttachment,	XmATTACH_FORM,
					    XmNbottomAttachment,XmATTACH_FORM,
					    NULL);
		XmStringFree(label);
	}

	return (bmgr);
}


/*
 * display_tool_data -- take a top level "application" shell and
 * data describing the applications to be displayed in the launcher
 * and populate the shell widget with a menubar and app stuff.
 */

void
display_tool_data(
	Widget			top_shell,
	const app_data_t	*app_data,
	int			app_data_cnt)
{

	Widget		top_form;
	Widget		welcome_label;
	Widget		bar;
	Widget		ns_form;


	/* 
	 * Get defaults for domain and host.
	 */
	sysinfo (SI_SRPC_DOMAIN, Domain, sizeof(Domain));
	sysinfo (SI_HOSTNAME, Host, sizeof(Host));


	if (top_shell == NULL || app_data == NULL || app_data_cnt == 0) {
		return;
	}

	/*
	 * Create the outermost manager widget.  Since the passed
	 * in parent is a shell, it can only have one direct child;
	 * this form widget will be that child.
	 */

	top_form = XtVaCreateManagedWidget("top_form", xmFormWidgetClass,
					   top_shell, NULL);


	/* Create the menu bar for the app. */

	bar = create_menu_bar(top_form);

	/* 
	 * Create the welcome message.
	 */
	welcome_label = create_welcome(top_form, bar);

	/* 
	 * Create the context form for display of name service, 
	 * domain and host information.
	 */
	ns_form = create_context_area(top_form, welcome_label);

	/*
	 * Create and layout the field for the application,
	 * including image buttons.
	 */
	(void) create_field(top_form, ns_form, app_data, app_data_cnt);
}


