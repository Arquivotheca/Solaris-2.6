/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma	ident	"@(#)main_win.c	1.27	96/08/02 SMI"


/*	main_win.c	*/

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include <Xm/Xm.h>
#include <Xm/ArrowB.h>
#include <Xm/CascadeB.h>
#include <Xm/CascadeBG.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/LabelG.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/MenuShell.h>
#include <Xm/MainW.h>
#include <Xm/MessageB.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrolledW.h>
#include <Xm/SelectioB.h>
#include <Xm/Separator.h>
#include <Xm/SeparatoG.h>
#include <Xm/Text.h>
#include <Xm/ToggleBG.h>
#include <X11/Shell.h>
#include <Xm/Protocols.h>
#include <Xm/AtomMgr.h>
#include <X11/IntrinsicP.h>

#include "util.h"
#include "action.h"
#include "launcher.h"

extern Widget GtopLevel;

char * localRegistry = NULL;
char userConfigFile[128] = { 0, };
char * user_home = NULL;
AppInfo  	* apps;
SolsticeApp	* sys_apps;
SolsticeApp	* local_apps;
int		num_sys_apps = 0;


/* external functions */
extern void	propertyCB(Widget, XtPointer, XtPointer);
extern void 	show_configDialog(Widget);
extern int 	merge_registry(char *, registry_loc_t);

/* extern from list.c */
extern int display_appList(Widget, visibility_t);

/* internal functions */
void		init_mainwin();

Widget		launchermain;
launcherContext_t  * launcherContext = NULL;

#define ERRBUF_SIZE 256
char		errbuf[ERRBUF_SIZE];

extern nl_catd	catd;	/* for catgets(), defined in main.cc */


/*******************************************************************************
       The following are callback functions.
*******************************************************************************/

/*
 *  Exit
 */
static void
exitCB(
	Widget		w, 
	XtPointer	cd, 
	XtPointer	cbs)
{
	if (propertyDialog && XtIsManaged(propertyDialog))
		XtPopdown(propertyDialog);

	if (configDialog && XtIsManaged(configDialog))
		XtPopdown(configDialog);

	if (user_home && (geteuid() != (uid_t)0))
		write_user_config(userConfigFile, 
			  launcherContext->l_appTable, 
			  launcherContext->l_appCount,
			  launcherContext->l_showCount);
	exit(0);
}

static void
configCB(
	Widget		w, 
	XtPointer	cd, 
	XtPointer	cbs)
{
	show_configDialog(get_shell_ancestor(w));
}

Widget
build_mainwin()
{
	launcherContext_t	* ctxt;
	Widget		menubar;
	Widget		tmpw;
	Widget		pulldown;
	Widget		menushell;
	Widget		w, rtn;
	Widget		headerForm;
	Widget		addPulldown;
	Widget		arrowForm;
	Widget		buttonForm;
	Widget		titleForm;
	Widget		widthForm;
	Atom		wmdelete;
	Arg		args[3];
	XtCallbackRec	cb[2];
	Dimension	height;

	ctxt = (launcherContext_t *) malloc(sizeof(launcherContext_t));
	memset((void *) ctxt, 0, sizeof(launcherContext_t));

	if (ctxt == NULL)
		fatal("build_main: unable to malloc");

	rtn = XtVaCreatePopupShell( "Solstice",
			topLevelShellWidgetClass, GtopLevel,
			XmNtitle, catgets(catd, 1, 49, "Solstice Launcher"),
			XmNdeleteResponse, XmDO_NOTHING,
			/* XmNallowShellResize, False, */
			XmNminWidth, 430,
			XmNminHeight, 300,
			XmNx, 300,
			XmNy, 200,
			NULL );

	/* add protocol callback for window manager quitting the shell */
	wmdelete = XmInternAtom(XtDisplay(rtn), "WM_DELETE_WINDOW", False);
	XmAddWMProtocolCallback(rtn, wmdelete, (XtCallbackProc)exitCB, NULL);

	ctxt->l_mainWindow = XtVaCreateManagedWidget( "mainWindow",
			xmMainWindowWidgetClass, rtn,
			XmNunitType, XmPIXELS,
			XmNwidth, 560,
			XmNheight, 270,
			XmNscrollingPolicy, XmAUTOMATIC,
			XmNscrolledWindowMarginHeight, OFFSET,
			NULL );

	XtVaSetValues(ctxt->l_mainWindow,
		XmNuserData, (XtPointer)ctxt,
		NULL);

	/* Menu Bar */
	XtSetArg(args[0], XmNmenuAccelerator, "<KeyUp>F10");
	ctxt->l_menuBar = menubar = 
		XmCreateMenuBar(ctxt->l_mainWindow, "menubar", args, 1);

	/* File menu */
	pulldown = XmCreatePulldownMenu(menubar, "_pulldown", NULL, 0);
	ctxt->l_fileMenu = XtVaCreateManagedWidget("fileMenu",
		xmCascadeButtonWidgetClass, menubar,
		XmNsubMenuId, pulldown,
		RSC_CVT(XmNlabelString, catgets(catd, 1, 50, "Launcher")),
		XmNmnemonic, 'L',
		NULL);
	ctxt->l_addMenuItem = XtVaCreateManagedWidget("addMenuItem",
		xmPushButtonGadgetClass, pulldown,
		RSC_CVT(XmNlabelString, catgets(catd, 1, 51, "Add Application")),
		XmNmnemonic, 'A',
		NULL);
	XtAddCallback( ctxt->l_addMenuItem, XmNactivateCallback,
		(XtCallbackProc) propertyCB,
		(XtPointer) NULL );
	ctxt->l_configMenuItem = XtVaCreateManagedWidget("configMenuItem",
		xmPushButtonGadgetClass, pulldown,
		RSC_CVT(XmNlabelString, catgets(catd, 1, 61, "Properties...")),
		XmNmnemonic, 'C',
		NULL);
	XtAddCallback( ctxt->l_configMenuItem, XmNactivateCallback,
		(XtCallbackProc) configCB,
		(XtPointer) NULL );
	ctxt->l_exitMenuItem = XtVaCreateManagedWidget(catgets(catd, 1, 52, "Exit"),
		xmPushButtonGadgetClass, pulldown,
		RSC_CVT(XmNlabelString, catgets(catd, 1, 53, "Exit")),
		XmNmnemonic, 'x',
		NULL);
	XtAddCallback( ctxt->l_exitMenuItem, XmNactivateCallback,
		(XtCallbackProc) exitCB,
		(XtPointer) ctxt );


	/* Help menu */
	pulldown = XmCreatePulldownMenu(menubar, "_pulldown", NULL, 0);
	ctxt->l_helpMenu = XtVaCreateManagedWidget(catgets(catd, 1, 54, "Help"),
		xmCascadeButtonWidgetClass, menubar,
		XmNsubMenuId, pulldown,
		XmNsensitive, True,
		RSC_CVT(XmNlabelString, catgets(catd, 1, 55, "Help")),
		XmNmnemonic, 'H',
		NULL);
	ctxt->l_aboutMenuItem = XtVaCreateManagedWidget( catgets(catd, 1, 56, "About"),
		xmPushButtonGadgetClass, pulldown,
		RSC_CVT(XmNlabelString, catgets(catd, 1, 57, "About Solstice Launcher...")),
		XmNmnemonic, 'A',
		NULL);

	XtAddCallback(ctxt->l_aboutMenuItem, XmNactivateCallback,
		(XtCallbackProc) helpCB,
		"solstice.t.hlp" );

	XtVaSetValues(menubar,
		XmNmenuHelpWidget, ctxt->l_helpMenu,
		NULL );

	XtManageChild(menubar);

	ctxt->l_workForm = XtVaCreateManagedWidget("workForm",
			xmFormWidgetClass, ctxt->l_mainWindow,
			NULL);
			
	XmMainWindowSetAreas(ctxt->l_mainWindow, menubar, (Widget) NULL,
			(Widget) NULL, (Widget) NULL, ctxt->l_workForm);

	launcherContext = ctxt;
	return (ctxt->l_mainWindow);
}

void
init_mainwin(Widget main)
{
	extern Widget build_config_dialog(Widget);
	int rc;
	static int verbose = 0;

	user_home = getenv("HOME");
	if (user_home == NULL) {
		display_error(launchermain, 
			catgets(catd, 1, 58, "Unable get $HOME environment variable"));
		localRegistry = (char *)malloc(strlen("./.solstice_registry") + 2);
		if (localRegistry == NULL)
			fatal(catgets(catd, 1, 59, "init_mainwin: Can't malloc"));
		strcpy(localRegistry, "./.solstice_registry");
	} else {
		localRegistry = (char *)malloc(strlen(user_home)+
				strlen("/.solstice_registry") + 4);
		if (localRegistry == NULL)
			fatal(catgets(catd, 1, 60, "init_mainwin: Can't malloc"));

		sprintf(userConfigFile, "%s/.solsticerc", user_home);
		if (geteuid() != (uid_t)0) {
			read_user_config(userConfigFile);

			sprintf(localRegistry, "%s/.solstice_registry", user_home);
			rc = merge_registry(localRegistry, LOCAL);
			if (rc < 0) {
			    if (verbose) {
				display_error(launchermain, 
				    catgets(catd, 1, 90, 
					"Error reading local registry file"));
			    }
/*
			    launcherContext->l_appCount = 0;
*/
			}
		}
	}

	(void) merge_registry("/etc/.solstice_registry", GLOBAL);

	if ((rc = merge_registry(NULL, GLOBAL)) < 0)
		solstice_error(NULL, rc);

	/* 
         * If an app that had been written to user config file
 	 * no longer appears in a registry, it needs to be
         * removed.
	 */
	remove_obsolete_entries();

	/* we do this early so b/c display of icons depends
         * on an extant appList text widget
	 * this may not be most elegant approach but...
	 * the cost is low.
	 */
	configDialog = build_config_dialog(launchermain);
	display_appList(configContext->c_appList, SHOW);
	display_appList(configContext->c_hideappList, HIDE);
	display_icons(launcherContext->l_workForm, configContext->c_appList);
}


/*
 * The following functions are dummy placeholders required
 * by the libraries for callback progress displays
 */

void
cleanup_and_exit()
{
	exit(2);
}

void
progress_init()
{
}

void
progress_done()
{
}

void
progress_cleanup()
{
}

/*ARGSUSED*/
void
interactive_pkgadd(int *result)
{
}

/*ARGSUSED*/
void
interactive_pkgrm(int *result)
{
}

/*ARGSUSED0*/
int
start_pkgadd(char *pkgdir)
{
	return (1);
}

/*ARGSUSED0*/
int
end_pkgadd(char *pkgdir)
{
	return (1);
}




