/* Copyright 1993 Sun Microsystems, Inc. */

#pragma ident "@(#)source_media_dialog.c	1.20 96/09/19 Sun Microsystems"

/*	source_media_dialog.c		*/

#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <stdlib.h>
#include <nl_types.h>
#include <limits.h>
#include <Xm/Xm.h>
#include <Xm/DialogS.h>
#include <Xm/SeparatoG.h>
#include <Xm/Form.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/RowColumn.h>
#include <Xm/MenuShell.h>
#include <Xm/MessageB.h>
#include <Xm/TextF.h>
#include "software.h"
#include "util.h"
#include "media.h"

#include <sys/types.h>
#include <sys/stat.h>

#define OK	1
#define CANCEL	2

#define MEDIA_SOLARIS_IMAGE \
catgets(_catd, 8, 501, "The specified path points to a Solaris image.\n \
Path will be modified to point to Table of Contents file.")
#define MEDIA_BAD_PATH \
catgets(_catd, 8, 500, "Invalid media path. Check existence and read/search permission of specified directory.")

typedef struct {
	enum sw_image_location*	loc;
	char**			path;
	char**			device;
	int			button;
} devresponse;

typedef	struct
{
	Widget	deviceDialog;
	Widget	locationForm;
	Widget	locationLabel;
	Widget	hardDisk;
	Widget	CDwithVolMgmt;
	Widget	CDwithoutVolMgmt;
	Widget	locationOptionMenu;
	Widget	dirForm;
	Widget	dirLabel;
	Widget	dirText;
	Widget	CDpathForm;
	Widget	CDpathLabel;
	Widget	CDpathText;
	Widget	mountPointForm;
	Widget	mountPointLabel;
	Widget	mountPointText;
	Widget	buttonBox;
} deviceCtxt;

extern nl_catd	_catd;	/* for catgets(), defined in main.c */

extern Widget 		GtopLevel;
extern XtAppContext	GappContext;
extern Display		*Gdisplay;

extern Widget get_shell_ancestor(Widget w);

static Widget		devicedialog = NULL;
devresponse	setMediaAnswer = { NULL, NULL, NULL, CANCEL};

static int displaySolarisWarning = 1;

/*******************************************************************************
       The following are callback functions.
*******************************************************************************/

static void
okCB(
	Widget wgt, 
	XtPointer cd, 
	XtPointer cbs)
{
	deviceCtxt  *ctxt;
	char*		server = NULL;
	char*		device;
	char*		path;
	XmString	xstr;
	Widget		w;
	devresponse*	answer = (devresponse*)cd;

	XtVaGetValues(devicedialog,
		XmNuserData, &ctxt,
		NULL);

	XtVaGetValues(ctxt->locationOptionMenu,
		XmNmenuHistory, &w,
		NULL);
	if (w == ctxt->CDwithoutVolMgmt) {
		*(answer->loc) = cd_without_volmgmt;

		*(answer->device) = NULL;

		XtVaGetValues(ctxt->mountPointText,
			XmNvalue, &path,
			NULL);
		if (*(answer->path)) {
			free(*(answer->path));
		}
		*(answer->path) = strdup(path);
	}
	else if (w == ctxt->CDwithVolMgmt) {
		*(answer->loc) = cd_with_volmgmt;

		XtVaGetValues(ctxt->CDpathText,
			XmNvalue, &path,
			NULL);
		if (*(answer->path)) {
			free(*(answer->path));
		}
		*(answer->path) = strdup(path);
	}
	else {
		*(answer->loc) = hard_disk;

		XtVaGetValues(ctxt->dirText,
			XmNvalue, &path,
			NULL);
		if (*(answer->path)) {
			free(*(answer->path));
		}
		*(answer->path) = strdup(path);
	}

	answer->button = OK;
}

static void
cancelCB(
	Widget wgt, 
	XtPointer cd, 
	XtPointer cbs)
{
	devresponse*	answer = (devresponse*)cd;


	answer->button = CANCEL;
}

static void
locationCB(
	Widget wgt, 
	XtPointer cd, 
	XtPointer cbs)
{
	deviceCtxt* ctxt = (deviceCtxt*)cd;


	if (wgt == ctxt->hardDisk) {
		XtManageChild(ctxt->dirForm);
		XtUnmanageChild(ctxt->CDpathForm);
		XtUnmanageChild(ctxt->mountPointForm);
	}
	else if (wgt == ctxt->CDwithVolMgmt) {
		XtManageChild(ctxt->CDpathForm);
		XtUnmanageChild(ctxt->dirForm);
		XtUnmanageChild(ctxt->mountPointForm);
	}
	else if (wgt == ctxt->CDwithoutVolMgmt) {
		XtManageChild(ctxt->mountPointForm);
		XtUnmanageChild(ctxt->dirForm);
		XtUnmanageChild(ctxt->CDpathForm);
	}
}


/*******************************************************************************
       The 'build_' function creates all the widgets
       using the resource values specified in the Property Editor.
*******************************************************************************/

static Widget
build_deviceDialog(Widget parent)
{
	deviceCtxt*	ctxt;
	Widget shell;
	Widget menushell;
	Widget pulldown;
	Widget actionForm;
	const int	wnum = 4;
	Widget		wlist[4];
	Dimension	width;
	Dimension	maxwidth = 0;
	int i;


	ctxt = (deviceCtxt*) malloc(sizeof(deviceCtxt));

	if (parent == NULL)
	{
		parent = GtopLevel;
	}

	shell = XtVaCreatePopupShell( "DeviceDialog_shell",
			xmDialogShellWidgetClass, parent,
			XmNshellUnitType, XmPIXELS,
			XmNhorizontalSpacing, 10,
			XmNverticalSpacing, 10,
			XmNminWidth, 300,
			XmNminHeight, 165,
			NULL );

	ctxt->deviceDialog = XtVaCreateWidget( "deviceDialog",
			xmFormWidgetClass,
			shell,
			RSC_CVT(XmNdialogTitle,catgets(_catd, 8, 435, "Admintool: Set Source Media")),
			XmNunitType, XmPIXELS,
 			XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL,
			XmNautoUnmanage, False,
			XmNmarginHeight, 10,
			XmNmarginWidth, 10,
			XmNhorizontalSpacing, 10,
			XmNverticalSpacing, 5,
			NULL );

	XtVaSetValues(ctxt->deviceDialog,
		XmNuserData, (XtPointer)ctxt,
		NULL);

	ctxt->locationForm = XtVaCreateManagedWidget( "locationForm",
			xmFormWidgetClass,
			ctxt->deviceDialog,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_FORM,
			NULL );

	ctxt->locationLabel = XtVaCreateManagedWidget( "locationLabel",
			xmLabelWidgetClass,
			ctxt->locationForm,
			RSC_CVT( XmNlabelString,catgets(_catd, 8, 436, "Software Location:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_FORM,
			NULL );

	menushell = XtVaCreatePopupShell ("menushell",
			xmMenuShellWidgetClass, ctxt->deviceDialog,
			XmNwidth, 1,
			XmNheight, 1,
			XmNallowShellResize, TRUE,
			XmNoverrideRedirect, TRUE,
			NULL );

	pulldown = XtVaCreateWidget( "pulldown",
			xmRowColumnWidgetClass,
			menushell,
			XmNrowColumnType, XmMENU_PULLDOWN,
			NULL );

	ctxt->hardDisk = XtVaCreateManagedWidget( "",
			xmPushButtonWidgetClass,
			pulldown,
			RSC_CVT(XmNlabelString,
				catgets(_catd, 8, 437, "Hard Disk")),
			NULL );
	XtAddCallback( ctxt->hardDisk, XmNactivateCallback,
		(XtCallbackProc) locationCB,
		(XtPointer) ctxt);

	ctxt->CDwithVolMgmt = XtVaCreateManagedWidget( "",
		xmPushButtonWidgetClass,
		pulldown,
		RSC_CVT(XmNlabelString, 
                    catgets(_catd, 8, 438, "CD with Volume Management")),
		NULL );
	XtAddCallback( ctxt->CDwithVolMgmt, XmNactivateCallback,
		(XtCallbackProc) locationCB,
		(XtPointer) ctxt);

	ctxt->CDwithoutVolMgmt = XtVaCreateManagedWidget( "",
		xmPushButtonWidgetClass,
		pulldown,
		RSC_CVT(XmNlabelString,catgets(_catd, 8, 439, 
			"CD without Volume Management")),
		NULL );
	XtAddCallback( ctxt->CDwithoutVolMgmt, XmNactivateCallback,
		(XtCallbackProc) locationCB,
		(XtPointer) ctxt);

	ctxt->locationOptionMenu = XtVaCreateManagedWidget( "location",
			xmRowColumnWidgetClass,
			ctxt->locationForm,
			XmNrowColumnType, XmMENU_OPTION,
			XmNsubMenuId, pulldown,
			RSC_CVT( XmNlabelString, "" ),
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, ctxt->locationLabel,
			XmNleftOffset, 0,
			XmNtopAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_FORM,
			NULL );

	actionForm = XtVaCreateManagedWidget( "actionForm",
			xmFormWidgetClass,
			ctxt->deviceDialog,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, ctxt->locationForm,
			XmNrightAttachment, XmATTACH_FORM,
			NULL );

	ctxt->dirForm = XtVaCreateWidget( "dirForm",
			xmFormWidgetClass,
			actionForm,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			NULL );

	ctxt->dirLabel = XtVaCreateManagedWidget( "dirLabel",
			xmLabelWidgetClass,
			ctxt->dirForm,
			RSC_CVT( XmNlabelString,catgets(_catd, 8, 440, "Directory:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_FORM,
			NULL );

#define MEDIA_PATH_TEXT_LEN  40
	ctxt->dirText = XtVaCreateManagedWidget( "dirText",
			xmTextFieldWidgetClass,
			ctxt->dirForm,
			XmNcolumns, MEDIA_PATH_TEXT_LEN,
			XmNmaxLength, PATH_MAX,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, ctxt->dirLabel,
			XmNleftOffset, 10,
			XmNrightAttachment, XmATTACH_FORM,
			NULL );

	ctxt->CDpathForm = XtVaCreateWidget( "CDpathForm",
			xmFormWidgetClass,
			actionForm,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			NULL );

	ctxt->CDpathLabel = XtVaCreateManagedWidget( "CDpathLabel",
			xmLabelWidgetClass,
			ctxt->CDpathForm,
			RSC_CVT( XmNlabelString,catgets(_catd, 8, 441, "CD Path:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_FORM,
			NULL );

	ctxt->CDpathText = XtVaCreateManagedWidget( "CDpathText",
			xmTextFieldWidgetClass,
			ctxt->CDpathForm,
			XmNcolumns,MEDIA_PATH_TEXT_LEN,
			XmNmaxLength, PATH_MAX,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, ctxt->CDpathLabel,
			XmNleftOffset, 10,
			XmNrightAttachment, XmATTACH_FORM,
			NULL );

	ctxt->mountPointForm = XtVaCreateWidget( "PathForm",
			xmFormWidgetClass,
			actionForm,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			NULL );

	ctxt->mountPointLabel = XtVaCreateManagedWidget( "PathLabel",
			xmLabelWidgetClass,
			ctxt->mountPointForm,
			RSC_CVT( XmNlabelString,catgets(_catd, 8, 442, "Mount Point:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_FORM,
			NULL );

	ctxt->mountPointText = XtVaCreateManagedWidget( "PathText",
			xmTextFieldWidgetClass,
			ctxt->mountPointForm,
			XmNcolumns, MEDIA_PATH_TEXT_LEN,
			XmNmaxLength, PATH_MAX,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, ctxt->mountPointLabel,
			XmNleftOffset, 10,
			XmNrightAttachment, XmATTACH_FORM,
			NULL );

	ctxt->buttonBox = XmCreateMessageBox(ctxt->deviceDialog,
			"ButtonBox", NULL, 0);

	XtVaSetValues(ctxt->buttonBox,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, actionForm,
	    		XmNleftAttachment, XmATTACH_FORM,
	    		XmNrightAttachment, XmATTACH_FORM,
	    		RSC_CVT( XmNokLabelString, catgets(_catd, 8, 443, "OK") ),
	    		RSC_CVT( XmNcancelLabelString, catgets(_catd, 8, 444, "Cancel") ),
	    		RSC_CVT( XmNhelpLabelString, catgets(_catd, 8, 445, "Help") ),
	    		NULL);

	XtAddCallback(ctxt->buttonBox, XmNcancelCallback,
			(XtCallbackProc) cancelCB,
			(XtPointer) &setMediaAnswer );

	XtAddCallback(ctxt->buttonBox, XmNokCallback,
			(XtCallbackProc) okCB,
			(XtPointer) &setMediaAnswer );

	XtAddCallback(ctxt->buttonBox, XmNhelpCallback,
			(XtCallbackProc) helpCB,
			"media.t.hlp" );


	XtManageChild(ctxt->buttonBox);

	/* Align all labels to right edge of longest label. */
	wlist[0] = ctxt->locationLabel;
	wlist[1] = ctxt->dirLabel;
	wlist[2] = ctxt->CDpathLabel;
	wlist[3] = ctxt->mountPointLabel;
	for (i=0; i<wnum; i++) {
		XtVaGetValues(wlist[i],
			XmNwidth, &width,
			NULL);
		if (width > maxwidth) {
			maxwidth = width;
		}
	}
	for (i=0; i<wnum; i++) {
		XtVaSetValues(wlist[i],
			XmNwidth, maxwidth,
			NULL);
	}

	XtVaSetValues(ctxt->deviceDialog,
		XmNdefaultButton, 
		XmMessageBoxGetChild(ctxt->buttonBox, XmDIALOG_OK_BUTTON), 
		NULL);

	/*	XtAddCallback( ctxt->deviceDialog, XmNdestroyCallback,
			(XtCallbackProc) destroyCB,
			(XtPointer) ctxt);
	*/


	return ( ctxt->deviceDialog );
}


static int
get_media_path(
	Widget			parent,
	enum sw_image_location*	loc,
	char**			path,
	char**			device
)
{
	deviceCtxt*	ctxt;
	XmString	xstr;
	Widget		w;


	parent = (parent == NULL) ? GtopLevel : get_shell_ancestor(parent);
	if (!devicedialog)
		devicedialog = build_deviceDialog(parent);

	XtVaGetValues(devicedialog,
		XmNuserData, &ctxt,
		NULL);

	/* Set default values */
	XtVaSetValues(ctxt->dirText,
		XmNvalue, "",
		NULL);
	XtVaSetValues(ctxt->CDpathText,
		XmNvalue, MANAGEDCDPATH,
		NULL);
	XtVaSetValues(ctxt->mountPointText,
		XmNvalue, CDMOUNTPOINT,
		NULL);

	/* Set input values */
	switch (*loc) {
	    case hard_disk:
		w = ctxt->hardDisk;
		if (*path != NULL) {
			XtVaSetValues(ctxt->dirText,
				XmNvalue, *path,
				NULL);
		}
		break;
	    case cd_with_volmgmt:
		w = ctxt->CDwithVolMgmt;
		if (*path != NULL) {
			XtVaSetValues(ctxt->CDpathText,
				XmNvalue, *path,
				NULL);
		}
		break;
	    case cd_without_volmgmt:
		w = ctxt->CDwithoutVolMgmt;
		if (*device != NULL) {
			*device = NULL;
		}
		if (*path != NULL) {
			XtVaSetValues(ctxt->mountPointText,
				XmNvalue, *path,
				NULL);
		}
		break;
	}
	XtVaSetValues(ctxt->locationOptionMenu,
		XmNmenuHistory, w,
		NULL);
	locationCB(w, (XtPointer)ctxt, NULL);


	setMediaAnswer.loc = loc;
	setMediaAnswer.path = path;
	setMediaAnswer.device = device;
	setMediaAnswer.button = 0;
	XtManageChild(devicedialog);
	XtPopup(XtParent(devicedialog), XtGrabNone);

	while (setMediaAnswer.button == 0)
		XtAppProcessEvent(GappContext, XtIMAll);

	XtPopdown(XtParent(devicedialog));
#if 0
	XtDestroyWidget(XtParent(devicedialog));
#endif
	XFlush(Gdisplay);

	return (setMediaAnswer.button == OK) ? 1 : 0;
}

/*
 * Takes a path as input, looks for "Solaris" in last component.
 * If it exists, returns a string with last component, including
 * / stripped off. If "Solaris" does not appear in last component
 * of path, return NULL.
 *
 * Caller must free returned string, if non-NULL.
 */

char * 
mungeSolarisPath(char * path)
{
	char * slash_p;
	char * return_p, * p;

	/* this makes life simpler below */
	if ((path == NULL) || strlen(path) < strlen("Solaris"))
		return(NULL);

	return_p = p = strdup(path);
	if (p[strlen(p)-1] == '/')
		p[strlen(p)-1] = '\0';

	slash_p = strrchr(p, '/');
	if (slash_p && strstr(slash_p+1, "Solaris")) {
		*slash_p = '\0';
		return(return_p);
	}
	else {
		free(p);
		return(NULL);
	}
}

/*
 * Given a directory path name, does .cdtoc file exist
 * in the directory?
 * 
 * If yes, return 1 else return 0.
 */

isCdtocPath(char * mp)
{
	struct stat buf;
	char cdtoc_path[1024];

	sprintf(cdtoc_path, "%s/.cdtoc", mp);
	if (stat(cdtoc_path, &buf) == 0)
		return(1);
	else
		return(0);
}

isValidPath(char * p)
{
	struct stat buf;
	if (stat(p, &buf) == 0)
		return(1);
	else	
		return(0);
}

Module * 
selectMediaPath(Widget parent, 
		enum sw_image_location * image_loc, 
		char ** install_path,
		char ** install_device)
{
	Widget w;
	char * p;
	char * current_directory;
	int vp = 0;
	Module * mtree;

	do { 
	    do {
		/* 
                 * Get a path. If user cancels, bail. Else, do
  	         * a simple stat() validation.
 		 * Continue util isValidPath == 1
		 */
	        if (get_media_path(parent, image_loc, 
				install_path, install_device) == 0) {
			/* User cancelled set source media dialog. */
			return(NULL);
		 }
		 if ((vp = isValidPath(*install_path)) == 0) {
			sprintf(errbuf, "%s", MEDIA_BAD_PATH);
			display_error(parent, errbuf);
		 }

		/* need to ensure we have complete path without . & .. */
		/* save current directory to return */
		if ( (current_directory=getcwd(NULL, FILENAME_MAX)) == NULL ) {
			sprintf(errbuf, "%s", MEDIA_BAD_PATH);
			display_error(parent, errbuf);
		}
		chdir (*install_path);   /* now cd to the path */
		free(*install_path);   
                /* use getcwd() to realloc to ensure buffer is big enough */
		if ( (*install_path=getcwd(NULL, FILENAME_MAX)) == NULL ) {
			sprintf(errbuf, "%s", MEDIA_BAD_PATH);
			display_error(parent, errbuf);
		}
		chdir (current_directory);   /* now cd back */
		free (current_directory);   /* and no memory leaks */

	    } while (!vp);
		
	    /* 
 	     * Look for "Solaris" in last component of path. Strip it
             * if it is there.
             */
	    p = mungeSolarisPath(*install_path);
		
	    /* 
             * If p != NULL, then install_path was munged and I am 
	     * looking at a Solaris CD after user typed a path
             * one dir too deep.
	     */
    
	    if (p && isCdtocPath(p)) {
		if (displaySolarisWarning) {
			w = display_warning(parent, MEDIA_SOLARIS_IMAGE);
			ForceDialog(w);
			/* only display warning once... */
			displaySolarisWarning = 0;
		}
		/* free old install_path and used munged version */
		free(*install_path);
		*install_path = p;
	    }
	    /* Read contents of media */
	    mtree = (Module*) sysman_list_sw(*install_path);
	    
	} while (mtree == NULL); /* until the read is legal... */
   
#ifdef METER
	/* Read a new module tree, reset space meter */
	deleteMeter();
#endif

	return(mtree);
}
