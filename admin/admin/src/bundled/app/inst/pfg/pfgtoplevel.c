#ifndef lint
#pragma ident "@(#)pfgtoplevel.c 1.57 96/07/26 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	pfgtoplevel.c
 * Group:	installtool
 * Description:
 */

#include "pf.h"
#include "pfg.h"
#include "pfgprocess.h"

int	pfgLowResolution = False;

String highres_fallback_resources[] = {
#include "Installtool.ad.h"
};

String lowres_fallback_resources[] = {
#include "Installtool_lowres.ad.h"
};

/*
 * Set up to allow command line X options (like -M).
 * This section defines and declares a global set of application data that
 * can be loaded via the X resource database (i.e. they can be specified
 * in the resource files), and which can be specified on the command line
 * as well.
 */
pfg_app_data_t pfgAppData;

/* resource specs */
static XtResource options_resources[] = {
	{"maptop", "Maptop", XmRBoolean, sizeof (Boolean),
		XtOffsetOf(pfg_app_data_t, maptop), XmRString,
		(XtPointer) "False"},
	{"dsrSpaceReqColumnSpace", "DsrSpaceReqColumnSpace", XmRInt,
		sizeof (int),
		XtOffsetOf(pfg_app_data_t, dsrSpaceReqColumnSpace),
		XmRImmediate, (XtPointer) 20},
	{"dsrFSSummColumnSpace", "DsrFSSummColumnSpace", XmRInt,
		sizeof (int),
		XtOffsetOf(pfg_app_data_t, dsrFSSummColumnSpace),
		XmRImmediate, (XtPointer) 20},
	{"dsrFSRedistColumnSpace", "DsrFSRedistColumnSpace", XmRInt,
		sizeof (int), XtOffsetOf(pfg_app_data_t,
		dsrFSRedistColumnSpace), XmRImmediate, (XtPointer) 20},
	{"dsrFSCollapseColumnSpace", "DsrFSRedistColumnSpace", XmRInt,
		sizeof (int), XtOffsetOf(pfg_app_data_t,
		dsrFSCollapseColumnSpace), XmRImmediate, (XtPointer) 30}
};

/* command line specs */
static XrmOptionDescRec options[] = {
	{"-M", "maptop", XrmoptionNoArg, (XtPointer) "True"},
	{"-dsrSpaceReqColumnSpace", "dsrSpaceReqColumnSpace",
		XrmoptionSepArg, NULL},
	{"-dsrFSSummColumnSpace", "dsrFSSummColumnSpace",
		XrmoptionSepArg, NULL},
	{"-dsrFSRedistColumnSpace", "dsrFSRedistColumnSpace",
		XrmoptionSepArg, NULL},
	{"-dsrFSCollapseColumnSpace", "dsrFSCollapseColumnSpace",
		XrmoptionSepArg, NULL}
};

void
pfgInit(int *argc, char **argv)
{
	char *width, *height;
	int scr_width, scr_height;

	/* set size of window to size of screen for auto-centering children */

	Display	*display;
	char	app_class_name[128];

	XtToolkitInitialize();
	pfgAppContext = XtCreateApplicationContext();

	(void) strcpy(app_class_name, "Installtool");

	width = getenv("SCREENWIDTH");
	height = getenv("SCREENHEIGHT");
	write_debug(GUI_DEBUG_L1, "screen width = %s",
		width ? width : "NULL");
	write_debug(GUI_DEBUG_L1, "screen height = %s",
		height ? height : "NULL");
	if (width && height) {
		if (atoi(width) <= 640 || atoi(height) <= 480) {
			/*
			 * use the appropriate resource file if this is
			 * a low resolution screen. The regular resource
			 * file is "Installtool", the low res one is
			 * "Installtool_lowres". changing the application
			 * class name will cause installtool to get the
			 * correct version of the resource file from under
			 * /usr/openwin/lib/locale/$LANG/app-defaults
			 */
			(void) strcat(app_class_name, "_lowres");
			pfgLowResolution = True;
			XtAppSetFallbackResources(pfgAppContext,
				lowres_fallback_resources);
		} else {
			pfgLowResolution = False;
			XtAppSetFallbackResources(pfgAppContext,
				highres_fallback_resources);
		}
	} else {
		pfgLowResolution = False;
		XtAppSetFallbackResources(pfgAppContext,
			highres_fallback_resources);
	}

	/*
	 * Try and open the display.  This will the display specified via
	 * the -display on the command line if there was one or via the
	 * display specified in the DISPLAY environment variable.
	 */
	display = XtOpenDisplay(pfgAppContext, NULL,
		"installtool", app_class_name,
		(XrmOptionDescRec *)options, XtNumber(options),
		argc, argv);

	/*
	 * if we couldn't open the display with the display above
	 * then default to DISPLAY=unix:0 and try one more time.
	 */
	if (display == NULL) {
		write_debug(GUI_DEBUG_L1,
			"Could not open display - trying \"unix:0\"");
		display = XtOpenDisplay(pfgAppContext, "unix:0",
			"installtool", app_class_name,
			(XrmOptionDescRec *)options, XtNumber(options),
			argc, argv);
	}


	/* Still no display - we're hosed! */
	if (display == NULL) {
		(void) fprintf(stderr, PFG_ER_NODISPLAY);
		pfgCleanExit(EXIT_INSTALL_FAILURE, (void *) NULL);
	}

	if ((pfgTopLevel = XtVaAppCreateShell("installtool", app_class_name,
		applicationShellWidgetClass, display, NULL)) == NULL) {
		pfgCleanExit(EXIT_INSTALL_FAILURE, (void *) NULL);
	}

	XtGetApplicationResources(pfgTopLevel, &pfgAppData,
		options_resources, XtNumber(options_resources),
		NULL, 0);

	scr_width = DisplayWidth(display, DefaultScreen(display));
	scr_height = DisplayHeight(display, DefaultScreen(display));

	XtVaSetValues(pfgTopLevel,
		XmNwidth, scr_width,
		XmNheight, scr_height,
		XmNmappedWhenManaged, pfgAppData.maptop,
		NULL);

	pfgWMDeleteAtom = XmInternAtom(XtDisplay(pfgTopLevel),
		"WM_DELETE_WINDOW", False);

}
