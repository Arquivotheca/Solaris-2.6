/*
 * Copyright (c) 1993 by Sun Microsystems, Inc.
 */

#pragma	ident	"@(#)main.c	1.34	94/11/16 SMI"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define _REENTRANT		/* for strtok_r */
#include <string.h>
#undef _REENTRANT
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/systeminfo.h>
#include <nl_types.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <Xm/Form.h>
#include <Xm/LabelG.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include "app_data.h"
#include "dyn_array.h"
#include "protos.h"
#include "adminhelp.h"
#include "launcher.h"

#ifdef PARTNER
#include <partner.h>
#elif PARTNER2
#include <partner.h>
#endif

/* Toplevel app widget */
Widget			toplevel;
XtAppContext		app_context;

static int		app_data_cnt;
static app_data_t	*app_data_array;
int			devnull;
pid_t			launcher_pid;
const unsigned long	one_sec = 1000;	/* milliseconds */


static String	fallback_resources[] = {
	"Admin*inputMethod: None",
	"Admin*FontList: -*-helvetica-bold-r-normal-*-12-120-75-75-p-70-*",
	"Admin*helpform*helptext.rows:		15",
	"Admin*helpform*helptext.columns:	52",
	"Admin*helpform*helpsubjs.visibleItemCount: 4",
	"Admin*helpform*helptext.fontList: \
		-*-lucida sans typewriter-medium-r-normal-*-12-*",
	NULL
};

void	WmDelete(Widget, XEvent *, String *, Cardinal *);
static XtActionsRec	wm_delete_actions[] = {
	{"wmDelete",	WmDelete}
};

extern void		app_mapped_prop_notify(XEvent *);
extern void		exec_fail_sigchld_handler();
extern void		exec_fail_sigusr1_handler();
extern void		exec_fail_timer_proc();


/*ARGSUSED*/
void
WmDelete(Widget w, XEvent *ev, String *params, Cardinal *num_params)
{
    extern void		do_exit_confirm_dialog();

    do_exit_confirm_dialog();
}


void
register_wm_delete_interest(Widget w)
{

    Display	*d = XtDisplay(w);
    Atom	wm_delete_window;


    wm_delete_window = XInternAtom(d, "WM_DELETE_WINDOW", False);

    (void) XSetWMProtocols(d, XtWindow(w), &wm_delete_window, 1);

    XtOverrideTranslations(w, XtParseTranslationTable("<Message>WM_PROTOCOLS: wmDelete()"));
}


void
register_app_mapped_prop_interest(Display *d, Window w)
{

	XWindowAttributes	attr;
	XSetWindowAttributes	set_attr;


	(void) XGetWindowAttributes(d, w, &attr);
	set_attr.event_mask = (attr.your_event_mask | PropertyChangeMask);
	XChangeWindowAttributes(d, w, CWEventMask, &set_attr);
}


/*
 * app_init -- any application-specific initialization that needs
 * to be done.
 */

static int
app_init()
{

	/* save the launcher's pid for child signal sending */

	launcher_pid = getpid();

	/*
	 * Open /dev/null for read/write, store the fd in a global.
	 * Useful for dup'ing (redirecting) stdio/stdout in child
	 * process before exec.
	 */

	if ((devnull = open("/dev/null", O_RDWR)) == -1) {
		return -1;
	}

	/*
	 * Set close-on-exec flag for the display connection so it
	 * isn't inherited by children.
	 */

	(void) fcntl(ConnectionNumber(XtDisplay(toplevel)), F_SETFD, 1);

	return 1;
}




int
process_children(char *parent, Array *ar)
{
	int		i;
	char		**children;
 	app_data_t	*app_data;


	/* Get the children of the passed in class. */
	children = admc_get_children((const char *)parent);

	for (i=0; children[i] != NULL; i++) {
		app_data = (app_data_t *)malloc(sizeof (app_data_t));
		if (app_data == NULL)
			return (NULL);

		/* Set the class, image name and display name. */
		strncpy(app_data->class, children[i], sizeof(app_data->class));

		if (admc_get_classicon(app_data->class, app_data->icon_image,
	    	sizeof(app_data->icon_image)) != E_ADMIN_OK)
			return (NULL);
		if (admc_get_displayname(app_data->class, app_data->icon_text,
	    	sizeof(app_data->icon_text)) != E_ADMIN_OK)
			return (NULL);

		/* Add the root class to the app data array. */
		if (array_add(ar, (const void *)app_data) < 0)
			return (NULL);

		/* Process this childs children. */
		process_children(children[i], ar);
        } 
}




/*
 * load_data_from_class_reg -- load data from the class registry.  
 */

static app_data_t	*
load_data_from_class_reg()
{

	Array		*ar;
	app_data_t	*app_data;

	
	if ((ar = array_create(sizeof (app_data_t), 100)) == NULL)
		return (NULL);

	app_data = (app_data_t *)malloc(sizeof (app_data_t));
	if (app_data == NULL)
		return (NULL);

	/* Get the root class. */
	if (admc_find_rootclass(app_data->class, sizeof(app_data->class)) != 
	    E_ADMIN_OK)
		return (NULL);
	if (admc_get_classicon(app_data->class, app_data->icon_image,
	    sizeof(app_data->icon_image)) != E_ADMIN_OK)
		return (NULL);
	if (admc_get_displayname(app_data->class, app_data->icon_text,
	    sizeof(app_data->icon_text)) != E_ADMIN_OK)
		return (NULL);

	/* Add the root class to the app data array. */
	if (array_add(ar, (const void *)app_data) < 0)
		return (NULL);

	/* Process all the children of the root class. */
	process_children(app_data->class, ar);

	app_data_cnt = array_count(ar);
	app_data_array = (app_data_t *)array_get(ar);
	array_free(ar);

	return (app_data_array);
}



nl_catd		catd; /* for catopen(), catgets() */

int
main(int argc, char **argv)
{

	Display			*d;
	Screen			*screen;
	Window			root;
	Atom			app_mapped;
	XEvent			ev;
	Pixmap			bit_image;
	int			x, y;
	unsigned int		width, height;
	struct sigaction	act;
	char			hostname[30];
	char			title[100];



	/*
	 * XtToolkitInitialize(),
	 * XtCreateApplicationContext(),
	 * XtAppSetFallbackResources(),
	 * XtOpenDisplay(),
	 * and XtVaAppCreateShell()
	 * all in one convenient call!
	 */

	XtSetLanguageProc(NULL, NULL, NULL);

	catd = catopen("launcher.cat", 0);

	/* Get the hostname of the machine we are executing on to put
	 * in the titlebat of Admintool.
	 */
	sysinfo(SI_HOSTNAME, hostname, sizeof(hostname));
	sprintf(title, catgets(catd, 1, 33, "Administration Tool 2.0: %s"), hostname);

	if ((toplevel = XtVaAppInitialize(&app_context, "Admin",
					  (XrmOptionDescList)NULL, 0,
					  &argc, argv, fallback_resources,
					  XtNname,	"adminTool",
					  XtNtitle, 	title,
					  XtNiconName,	catgets(catd, 1, 16, "Admintool 2.0"),
					  XmNdeleteResponse,
					  		XmDO_NOTHING,
					  NULL)) == NULL) {
		exit(1);
	}


	/*
	 * Add an application action for handling delete window
	 * window manager requests on the toplevel window.
	 * A WM_DELETE_WINDOW protocol ClientMessage event
	 * will translate to this action.  Later, register
	 * the toplevel window's interest in the WM_DELETE_WINDOW
	 * protocol.
	 */

	XtAppAddActions(app_context, wm_delete_actions,
			XtNumber(wm_delete_actions));

	/*
	 * Try to read icon image from bitmap and set the pixmap
	 * as the icon image for the program.
	 */

	d = XtDisplay(toplevel);
	screen = XtScreen(toplevel);
	root = RootWindowOfScreen(screen);

	if (XReadBitmapFile(d, root,
			    "/usr/snadm/etc/launcher.icon",
			    &width, &height, &bit_image,
			    &x, &y) == BitmapSuccess) {

		XtVaSetValues(toplevel, XtNiconPixmap,
			      (Pixmap)bit_image, NULL);
	}

	/*
	 * Application-specific initialization.  If this fails we
	 * might as well not even run.
	 */

	if (app_init() < 0) {
		exit(1);
	}

	/* Read the Class registry and set the application array. */
	(void)load_data_from_class_reg();

	display_tool_data(toplevel, app_data_array, app_data_cnt);

	/*
	 * Add SIGCHLD and SIGUSR1 handlers and a timer proc for dealing
	 * with failed launch exec's.  See the comp.windows.x.intrinsics
	 * FAQ for a description of why this is done this way.
	 */

	(void) XtAppAddTimeOut(app_context, one_sec,
			       exec_fail_timer_proc, NULL);

	act.sa_handler = exec_fail_sigchld_handler;
	(void) sigemptyset(&act.sa_mask);
	act.sa_flags = SA_RESTART | SA_SIGINFO;
	(void) sigaction(SIGCHLD, &act, NULL);

	act.sa_handler = exec_fail_sigusr1_handler;
	(void) sigemptyset(&act.sa_mask);
	act.sa_flags = SA_RESTART | SA_SIGINFO;
	(void) sigaction(SIGUSR1, &act, NULL);

	XtRealizeWidget(toplevel);

	/*
	 * Register toplevel's window's interest in WM_DELETE.
	 * Must do this after the widget is realized as it's
	 * window gets the property, and the window doesn't
	 * exist until after realize.
	 */

	register_wm_delete_interest(toplevel);

#ifdef	PARTNER
	InitQaPartner(toplevel);
#elif	PARTNER
	QAP_BindApp(app_context);
#endif

	/*
	 * Atom-ize ADM_APP_MAPPED_PROPERTY.
	 * Launched apps will communicate to the launcher that their
	 * toplevel window is mapped via this property on the root
	 * window of the screen upon which they are displayed.  The
	 * launcher will use this to display a "watch" cursor while
	 * the app is starting; when the launched app notifies the
	 * launcher that its toplevel window is mapped, the launcher
	 * can turn the watch back to a normal pointer cursor.
	 * The registration of interest by the root window ensures
	 * that the root window has selected PropertyChangeMask in
	 * its event mask.
	 */

	app_mapped = XInternAtom(d, "ADM_APP_MAPPED_PROPERTY", False);

	register_app_mapped_prop_interest(d, root);


	/* XtAppMainLoop plus a check for the PropertyNotify event */

	for ( ; ; ) {

		XtAppNextEvent(app_context, &ev);

		if (ev.type == PropertyNotify &&
		    ev.xproperty.window == root &&
		    ev.xproperty.atom == app_mapped) {

			app_mapped_prop_notify(&ev);

		} else {

			XtDispatchEvent(&ev);
		}
	}
}


