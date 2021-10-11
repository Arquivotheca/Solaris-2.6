
/* Copyright 1993 Sun Microsystems, Inc. */

#pragma ident "@(#)main.c	1.13 96/05/31 Sun Microsystems"

/*	main.c	*/


#ifdef XOPEN_CATALOG
#include <locale.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>		/* for getopt declarations */
#include <sys/stat.h>
#include <nl_types.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xm/Xm.h>
#include <X11/cursorfont.h>
#include <sys/utsname.h>
#include <sys/systeminfo.h>
#include <sys/types.h>
#include <unistd.h>

#include "util.h"

/* #include "adminhelp.h" */

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

Widget		UxTopLevel;	/* TEMPORARY */
Cursor	busypointer;
char *	localhost;

extern Widget sysmgrmain;

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


context_t	initial_ctxt = ctxt_user;
boolean_t	show_browse_menu = B_TRUE;

static char	*swmtool_name = "swmtool";
static char	*admintool_arg_usage =
	    "[ -b ] [ -c user|group|host|printer|serial|sw ]";
static char	*swmtool_arg_usage = "[ -d directory ] [ adminfile ]";

static XrmOptionDescRec	options[] = {
	{"-d", "*path", XrmoptionSkipArg, NULL}
};

nl_catd		_catd;	/* for catgets() */

main(int argc, char * argv[])
{
 	Widget  build_mainwin(void);
	void	init_mainwin(void);
	Pixmap		pix_image=NULL;
	Pixel 		fg, bg;
	char		*prog_basename;
	int		c;
	boolean_t	usage_err = B_FALSE;
	char		*ptr;
	char		*context = NULL;
	char		*path = NULL;
	char		buf[128];
 	char*           cmdline_admin_file = NULL;
        static boolean_t check_admin(char*);

	extern void set_icon_pixmap(Widget shell, Pixmap image, char *label);
	
	XtSetLanguageProc(NULL, NULL, NULL);
  	GtopLevel = XtAppInitialize(&GappContext, "Admin",
				     options, XtNumber(options),
				     &argc, argv, fallback_resources,
				     NULL, 0);

	/*
	 * get the program name from argv[0].  strip off any directory
	 * path, so if somebody ran the program as "/bin/admintool"
	 * we trim it down to just "admintool".
	 */

	if ((ptr = strrchr(argv[0], '/')) != NULL) {
		prog_basename = ptr + 1;
	} else {
		prog_basename = argv[0];
	}

	/* admintool command line arg parsing */

	while ((c = getopt(argc, argv, "bc:d:")) != EOF) {
		switch (c)  {
		case 'b':
			show_browse_menu = B_FALSE;
			break;
		case 'c':
			context = strdup(optarg);
			break;
		case 'd':
			{
			char *p = mungeSolarisPath(optarg);
			if (p && isCdtocPath(p)) {
			    path = p;
			} else {
			    if (p) free(p);
			    path = strdup(optarg);
			}
			break;
			}
		case '?':
			usage_err = B_TRUE;
		}
	}
	if (optind < argc) { /* There exists a admin file name as argument */
            cmdline_admin_file = argv[optind];
            if (!check_admin(cmdline_admin_file))
                usage_err = B_TRUE;
            else
                load_admin_file(cmdline_admin_file);
        }



	if (context != NULL) {
		if (strcmp(context, "user") == 0) {
			initial_ctxt = ctxt_user;
		} else if (strcmp(context, "group") == 0) {
			initial_ctxt = ctxt_group;
		} else if (strcmp(context, "host") == 0) {
			initial_ctxt = ctxt_host;
		} else if (strcmp(context, "printer") == 0) {
			initial_ctxt = ctxt_printer;
		} else if (strcmp(context, "serial") == 0) {
			initial_ctxt = ctxt_serial;
		} else if (strcmp(context, "sw") == 0) {
			initial_ctxt = ctxt_sw;
		} else {
			usage_err = B_TRUE;
		}
	}

	if (usage_err == B_TRUE) {
		if (strcmp(prog_basename, swmtool_name) == 0) {
			sprintf(buf, "%s %s", prog_basename, swmtool_arg_usage);
		}
		else {
			strcpy(buf, prog_basename);
		}
		(void) fprintf(stderr,
			catgets(_catd, 8, 190, "usage: %s\n"), buf);
		exit(1);
	}

	if (strcmp(prog_basename, swmtool_name) == 0) {
		/* start admintool in software context */
		initial_ctxt = ctxt_sw;
		show_browse_menu = B_FALSE;
	}

	_catd = catopen("admintool.cat", 0);
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

	/*----------------------------------------------------------------
	 * Create and popup the first window of the interface.  The 	 
	 * return value can be used in the popdown or destroy functions.
         * The Widget return value of  PJ_INTERFACE_FUNCTION_CALL will 
         * be assigned to "mainIface" from  PJ_INTERFACE_RETVAL_TYPE. 
	 *---------------------------------------------------------------*/

#ifdef SW_INSTALLER
	if (path == NULL) {
		path = MANAGEDCDPATH;
	}

	show_addsoftwaredialog(GtopLevel, NULL, path);
	XtVaSetValues(GtopLevel,
		XtNiconName, "Installer",
		NULL);
#else
	sysmgrmain = build_mainwin();
	init_mainwin();

	XtVaGetValues(XtParent(sysmgrmain), 
		XmNforeground, &fg,
		XmNbackground, &bg,
		NULL);

	pix_image = XmGetPixmap(XtScreen(XtParent(sysmgrmain)), 
		"/usr/snadm/etc/admintool.xpm", fg, bg);

	set_icon_pixmap(XtParent(sysmgrmain), pix_image, "Admintool");

	/*
	 * Watch for MapNotify event, call event handler when it
	 * happens.  The Handler will talk to the launcher (if
	 * the launcher is alive).
	 */

	XtAddEventHandler(XtParent(sysmgrmain), StructureNotifyMask, False,
	    map_window_event_handler, NULL);

	XtManageChild(sysmgrmain);
	XtPopup(XtParent(sysmgrmain), XtGrabNone);

	if ((initial_ctxt == ctxt_sw) && path) {
		sysMgrMainCtxt* ctxt;

		XtVaGetValues(sysmgrmain,
			XmNuserData, &ctxt,
			NULL);
		show_addsoftwaredialog(sysmgrmain, ctxt, path);
	}
#endif

	/* Enable QA partner */
#ifdef PARTNER2
	QAP_BindApp(GappContext);
#endif

	/*-----------------------
	 * Enter the event loop 
	 *----------------------*/

  	XtAppMainLoop (GappContext);

}

static boolean_t
check_admin(char* lpath)
{
        struct stat buf;

        if (stat(lpath, &buf) == 0) {
                if ((buf.st_mode & S_IFREG) &&
                    (buf.st_mode & (S_IRUSR | S_IRGRP | S_IROTH)))
                        return B_TRUE;
                else {
                        (void) fprintf(stderr,
                            catgets(_catd, 8, 150,
                                "Admin file unreadable: %s\n"), lpath);
                        return (B_FALSE);
                }
        } else {
                        (void) fprintf(stderr,
                            catgets(_catd, 8, 151,
                                "Admin file not found: %s\n"), lpath);
                        perror(NULL);
                        return (B_FALSE);
        }
}

