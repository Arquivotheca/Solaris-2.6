
/* Copyright 1993 Sun Microsystems, Inc. */

#pragma ident "@(#)main.c	1.5 94/11/10 Sun Microsystems"

/*	main.c	*/


#ifdef XOPEN_CATALOG
#include <locale.h>
#endif

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xm/Xm.h>
#include <X11/cursorfont.h>
#include <sys/utsname.h>
#include <sys/systeminfo.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/systeminfo.h>
#include <nl_types.h>
#include <unistd.h>

#include "util.h"
#include "xpm.h"

#include "solstice.xpm"
Pixmap	solstice_image;

#ifdef PARTNER2
#include <partner.h>
#endif

/*----------------------------------------------
 * Insert application global declarations here
 *---------------------------------------------*/
XtAppContext	GappContext;
Widget		GtopLevel;
Display		*Gdisplay;
int		Gscreen;

pid_t		launcher_pid;
int		devnull;
unsigned long   one_sec = 1000;

extern Widget		propertyDialog;
extern Widget		fileSelectionDialog;

extern void             app_mapped_prop_notify(XEvent *);
extern void             exec_fail_sigchld_handler();
extern void             exec_fail_sigusr1_handler();
extern void             exec_fail_timer_proc();
extern void 		set_icon_pixmap(Widget shell, Pixmap image, char *label);

Widget		UxTopLevel;	/* TEMPORARY */
Cursor	busypointer;
char *	localhost;

extern Widget launchermain;

char	DefDomain[SYS_NMLN];
char	DefHost[SYS_NMLN];

static char*	fallback_resources[] = {
	"Admin*allowShellResize: True",
	"Admin*background: gray",
	"Admin*foreground: black",
	"Admin*fontList: \
		-*-helvetica-bold-r-normal-*-12-120-75-75-p-70-*",
	"Admin*XmTextField.fontList: \
		-*-helvetica-medium-r-normal-*-12-120-75-75-p-67-*",
	"Admin*XmText.fontList: \
		-*-helvetica-medium-r-normal-*-12-120-75-75-p-67-*",
        "Admin*XmList.fontList: \
                -misc-fixed-medium-r-normal--13-120-75-75-c-70-*",
	"Admin*helpform*helptext.rows:		15",
	"Admin*helpform*helptext.columns:	52",
	"Admin*helpform*helpsubjs.visibleItemCount: 4",
	"Admin*helpform*helptext.fontList: \
		-*-lucida sans typewriter-medium-r-normal-*-12-*",
	NULL
};

void
register_app_mapped_prop_interest(Display *d, Window w)
{
 
        XWindowAttributes       attr;
        XSetWindowAttributes    set_attr;
 
 
        (void) XGetWindowAttributes(d, w, &attr);
        set_attr.event_mask = (attr.your_event_mask | PropertyChangeMask);
        XChangeWindowAttributes(d, w, CWEventMask, &set_attr);
}


void
map_window_event_handler(Widget w, XtPointer p, XEvent *ev, Boolean * f)
{

    pid_t	ppid;
    Atom	mapped_property;
    Display	*d = XtDisplayOfObject(w);


    if (ev->type == MapNotify) {

	mapped_property = XInternAtom(d, "ADM_APP_MAPPED_PROPERTY", True);

	if (mapped_property != None) {

	    ppid = getppid();

	    XChangeProperty(d, RootWindowOfScreen(XtScreen(w)),
		mapped_property, XA_INTEGER, 32, PropModeReplace,
		(unsigned char *)&ppid, 1);
	}

	XtRemoveEventHandler(w, XtAllEvents, True,
	    map_window_event_handler, NULL);
    }
}


nl_catd		catd;	/* for catgets() */

static int
app_init() {
        /* save the launcher's pid for child signal sending */
 
        launcher_pid = getpid();
 
        /*
         * Open /dev/null for read/write, store the fd in a global.
         * Useful for dup'ing (redirecting) stdio/stdout in child
         * process before exec.
         */
 
        if ((devnull = open("/dev/null", O_RDWR)) == -1) {
		perror(catgets(catd, 1, 91, "Can't open /dev/null"));
		return(-1);
        }
 
        /*
         * Set close-on-exec flag for the display connection so it
         * isn't inherited by children.
         */
 
        (void) fcntl(ConnectionNumber(XtDisplay(GtopLevel)), F_SETFD, 1);
 
        return 1;
}

main(int argc, char * argv[])
{
 	Widget  build_mainwin(void);
	void	init_mainwin(void);
	Display		*d;
	Atom		app_mapped;
	Screen		*screen;
	Window		root;
	int		x, y;
	unsigned int	width, height;
	GC		gc;
	struct sigaction	act;
	XEvent		ev;
	
	

	XtSetLanguageProc(NULL, NULL, NULL);
putenv("XFILESEARCHPATH=/opt/SUNWadm/2.1/classes/locale/%L/%T/Solstice:/usr/openwin/lib/locale/%L/%T/%N%S");
 	GtopLevel = XtAppInitialize(&GappContext, "Admin",
				     NULL, 0,
				     &argc, argv, fallback_resources,
				     NULL, 0);
	catd = catopen("solstice.cat", 0);
	UxTopLevel = GtopLevel; /* TEMPORARY */
	Gdisplay = XtDisplay(GtopLevel);
	Gscreen = XDefaultScreen(Gdisplay);

	/* We set the geometry of GtopLevel so that dialogShells
	   that are parented on it will get centered on the screen
	   (if defaultPosition is true). */

	XtVaSetValues(GtopLevel,
			XtNx, 0,
			XtNy, 0,
			XtNwidth, DisplayWidth(Gdisplay, Gscreen),
			XtNheight, DisplayHeight(Gdisplay, Gscreen),
			NULL);

	/*-------------------------------------------------------
	 * Insert initialization code for your application here 
	 *------------------------------------------------------*/
	busypointer = XCreateFontCursor(Gdisplay, XC_watch);
	sysinfo (SI_HOSTNAME,    DefHost,   sizeof(DefHost));
	localhost = strdup(DefHost);

	d = XtDisplay(GtopLevel);
	screen = XtScreen(GtopLevel);
	root = RootWindowOfScreen(screen);

	/*----------------------------------------------------------------
	 * Create and popup the first window of the interface.  The 	 
	 * return value can be used in the popdown or destroy functions.
         * The Widget return value of  PJ_INTERFACE_FUNCTION_CALL will 
         * be assigned to "mainIface" from  PJ_INTERFACE_RETVAL_TYPE. 
	 *---------------------------------------------------------------*/

	if (app_init() < 0) 
		fatal(catgets(catd, 1, 48, "app_init failed"));
	launchermain = build_mainwin();

	create_XPM(launchermain, solstice_xpm, 
		WhitePixelOfScreen(screen), &solstice_image);
	set_icon_pixmap(XtParent(launchermain), solstice_image, "Solstice");


	init_mainwin();

	 /*
         * Add SIGCHLD and SIGUSR1 handlers and a timer proc for dealing
         * with failed launch exec's.  See the comp.windows.x.intrinsics
         * FAQ for a description of why this is done this way.
         */
 
        (void) XtAppAddTimeOut(GappContext, one_sec,
                               exec_fail_timer_proc, NULL);
 
        act.sa_handler = exec_fail_sigchld_handler;
        (void) sigemptyset(&act.sa_mask);
        act.sa_flags = SA_RESTART | SA_SIGINFO;
        (void) sigaction(SIGCHLD, &act, NULL);
 
        act.sa_handler = exec_fail_sigusr1_handler;
        (void) sigemptyset(&act.sa_mask);
        act.sa_flags = SA_RESTART | SA_SIGINFO;
        (void) sigaction(SIGUSR1, &act, NULL);

	/*
	 * Watch for MapNotify event, call event handler when it
	 * happens.  The Handler will talk to the launcher (if
	 * the launcher is alive).
	 */

	XtAddEventHandler(XtParent(launchermain), StructureNotifyMask, False,
	    map_window_event_handler, NULL);

	XtManageChild(launchermain);
	XtPopup(XtParent(launchermain), XtGrabNone);

	/* Enable QA partner */
#ifdef PARTNER2
	QAP_BindApp(GappContext);
#endif

	/*-----------------------
	 * Enter the event loop 
	 *----------------------*/
	app_mapped = XInternAtom(d, "ADM_APP_MAPPED_PROPERTY", False);
 
        register_app_mapped_prop_interest(d, root);
	for ( ; ; ) {
		XtAppNextEvent(GappContext, &ev);
 
                if (ev.type == PropertyNotify &&
                    ev.xproperty.window == root &&
                    ev.xproperty.atom == app_mapped) {
 
                        app_mapped_prop_notify(&ev);
 
                } else {
 
                        XtDispatchEvent(&ev);
                }

	}
#if 0
  	XtAppMainLoop (GappContext);
#endif
}

