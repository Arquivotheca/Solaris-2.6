#ifndef lint
#pragma ident "@(#)pfgremote.c 1.37 96/06/23 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	pfgremote.c
 * Group:	installtool
 * Description:
 */

#include <signal.h>
#include <unistd.h>

#include "pf.h"
#include "pfg.h"

#include "pfgRemote_ui.h"

/* static functions */

static void updateSummary(void);
static void sigalarm_handler(int);
static void updateRemlist(void);

/* static variables */

static Widget text[4];
static Remote_FS *cancel_remote_list;
static WidgetList widget_list = NULL;

/* create remotes window, populate summary list (on right) if necessary */

Widget
pfgCreateRemote(Widget parent)
{
	Widget remote_dialog;
	Dimension max_width = 0, width, height;
	Remote_FS *remotefs = NULL;

	/* save remote list state for cancel */
	remotefs = pfGetRemoteFS();
	cancel_remote_list = pfDupRemList(remotefs);

	/* create and initialize top window */

	remote_dialog = tu_remote_dialog_widget("remote_dialog",
		parent, &widget_list);

	XmAddWMProtocolCallback(pfgShell(remote_dialog), pfgWMDeleteAtom,
	    (XtCallbackProc) pfgExit, NULL);

	XtVaSetValues(pfgShell(remote_dialog),
		XmNtitle, TITLE_MOUNTREMOTE,
		XmNdeleteResponse, XmDO_NOTHING,
		NULL);
	xm_SetNoResize(pfgTopLevel, pfgShell(remote_dialog));

	pfgSetWidgetString(widget_list, "serverLabel", PFG_RM_SERVER);
	pfgSetWidgetString(widget_list, "ipLabel", PFG_RM_ADDR);
	pfgSetWidgetString(widget_list, "filesysLabel", PFG_RM_REMOTE);
	pfgSetWidgetString(widget_list, "mountPointLabel", PFG_RM_LOCAL);

	pfgSetWidgetString(widget_list, "remoteLabel", PFG_RM_RFS);
	pfgSetWidgetString(widget_list, "addButton", PFG_RM_ADD);
	pfgSetWidgetString(widget_list, "removeButton", PFG_RM_REMOVE);
	pfgSetWidgetString(widget_list, "testButton", PFG_RM_TEST);
	pfgSetWidgetString(widget_list, "continueButton", PFG_CONTINUE);
	pfgSetWidgetString(widget_list, "cancelButton", PFG_CANCEL);
	pfgSetWidgetString(widget_list, "helpButton", PFG_HELP);

	XtVaGetValues(pfgGetNamedWidget(widget_list, "serverLabel"),
		XmNwidth, &width,
		NULL);
	if (width > max_width)
		max_width = width;

	XtVaGetValues(pfgGetNamedWidget(widget_list, "ipLabel"),
		XmNwidth, &width,
		NULL);
	if (width > max_width)
		max_width = width;

	XtVaGetValues(pfgGetNamedWidget(widget_list, "filesysLabel"),
		XmNwidth, &width,
		NULL);
	if (width > max_width)
		max_width = width;

	XtVaGetValues(pfgGetNamedWidget(widget_list, "mountPointLabel"),
		XmNwidth, &width,
		NULL);
	if (width > max_width)
		max_width = width;

	XtVaSetValues(pfgGetNamedWidget(widget_list, "serverLabel"),
		XmNwidth, max_width,
		NULL);
	XtVaSetValues(pfgGetNamedWidget(widget_list, "ipLabel"),
		XmNwidth, max_width,
		NULL);
	XtVaSetValues(pfgGetNamedWidget(widget_list, "filesysLabel"),
		XmNwidth, max_width,
		NULL);
	XtVaSetValues(pfgGetNamedWidget(widget_list, "mountPointLabel"),
		XmNwidth, max_width,
		NULL);

	text[0] = pfgGetNamedWidget(widget_list, "serverText");
	text[1] = pfgGetNamedWidget(widget_list, "ipText");
	text[2] = pfgGetNamedWidget(widget_list, "filesysText");
	text[3] = pfgGetNamedWidget(widget_list, "mountPointText");

	XtManageChild(remote_dialog);

	updateSummary();

	XtVaGetValues(pfgShell(remote_dialog),
	    XmNwidth, &width,
	    XmNheight, &height,
	    NULL);

	XtVaSetValues(pfgShell(remote_dialog),
	    XmNminWidth, width,
	    XmNminHeight, height,
	    NULL);

	return (remote_dialog);
}


/* callbacks */

/* ARGSUSED */
void
remoteContinueCB(Widget w, XtPointer clientD, XtPointer callD)
{
	/* destroy self, free memory for cancel */
	XtUnmanageChild(pfgShell(w));
	XtDestroyWidget(pfgShell(w));

	pfgUnbusy(pfgShell(XtParent(pfgShell(w))));

	pfFreeRemList(cancel_remote_list);

	free(widget_list);

	pfgSetAction(parAContinue);
}


/* ARGSUSED */
void
remoteCancelCB(Widget w, XtPointer clientD, XtPointer callD)
{
	/* put back cancel list, destroy self */
	pfSetRemoteFS(cancel_remote_list);

	free(widget_list);

	XtUnmanageChild(pfgShell(w));
	XtDestroyWidget(pfgShell(w));

	pfgUnbusy(pfgShell(XtParent(pfgShell(w))));
}


/* ARGSUSED */
void
remoteServerCB(Widget w, XtPointer clientD, XtPointer callD)
{
	char *name, *ip;

	if (debug)
		(void) printf("pfgremote:remoteServerCB\n");

	name = XmTextFieldGetString(w);
	if (debug)
		(void) printf("\t looking for ip of '%s'\n", name);

	XmTextFieldSetString(text[2], "");
	XmTextFieldSetString(text[3], "");
	if (strlen(ip = name2ipaddr(name))) {
		XmTextFieldSetString(text[1], ip);

		pfgBusy(pfgShell(w));

		updateRemlist();

		pfgUnbusy(pfgShell(w));
	} else {
		XmTextFieldSetString(text[1], "");
		XmListDeleteAllItems(
			pfgGetNamedWidget(widget_list, "remoteList"));
	}

	XmProcessTraversal(w, XmTRAVERSE_NEXT_TAB_GROUP);
}


/* ARGSUSED */
void
remoteRemlistCB(Widget w, XtPointer clientD, XtPointer callD)
{
	char *str;

	XmListCallbackStruct *cbs =
		/* LINTED [pointer cast] */
		(XmListCallbackStruct *) callD;

	if (debug)
		(void) printf("pfgremote: remoteRemlistCB\n");

	XmStringGetLtoR(cbs->item, XmSTRING_DEFAULT_CHARSET, &str);
	XmTextFieldSetString(text[2], str);
	if (strncmp(str, "/export", 7) == 0)
		XmTextFieldSetString(text[3], str + 7);
	else
		XmTextFieldSetString(text[3], str);

}

/* ARGSUSED */
void
remoteAddCB(Widget w, XtPointer clientD, XtPointer callD)
{
	Remote_FS *rem, *remotefs = NULL, *newremotefs = NULL;
	pfErCode er;
	char *data[4];
	int i;

	if (debug)
		(void) printf("pfgremote:remoteAddCB\n");

	/* gather data from widgets */

	for (i = 0; i < 4; ++i) {
		data[i] = XmTextFieldGetString(text[i]);
		if (i != 1 && !strlen(data[i])) {
			XmProcessTraversal(text[i],
				XmTRAVERSE_CURRENT);
			switch (i) {
				case 0: pfAppWarn(0, PFG_RM_NOSERVER); break;
				case 2: pfAppWarn(0, PFG_RM_NOREM); break;
				case 3: pfAppWarn(0, PFG_RM_NOLOCAL); break;
			}
			while (--i >= 0)
				if (*data[i])
					XtFree(data[i]);
			return;
		}
	}
	rem = pfNewRem(NOT_TESTED, data[3], data[0], data[1], data[2], "-");

	for (i = 0; i < 4; ++i)
		if (*data[i])
			XtFree(data[i]);

	/* validate data */

	if (er = pfValidRem(rem)) {
		pfWarn(er, pfErMessage(er), NULL);
		pfFreeRem(rem);
		return;
	}
	/* update the profile and summary */
	remotefs = pfGetRemoteFS();
	if (remotefs) {
		newremotefs = pfDupRemList(remotefs);
		(void) pfAppendRem(&newremotefs, rem);
	} else
		newremotefs = rem;
	pfFreeRemList(remotefs);
	pfSetRemoteFS(newremotefs);
	updateSummary();

}


/* ARGSUSED */
void
remoteRemoveCB(Widget w, XtPointer clientD, XtPointer callD)
{
	int i, count, *pos;
	Remote_FS *rem, *axe, *remotefs, *newremotefs;

	/* which item in summary is selected? */

	if (!XmListGetSelectedPos(pfgGetNamedWidget(widget_list, "summaryList"),
			&pos, &count)) {
		pfAppWarn(0, PFG_RM_NOTHING);
		return;
	}
	write_debug(GUI_DEBUG_L1, "selected position is %d", *pos);


	/* remove item from pf */
	remotefs = pfGetRemoteFS();
	newremotefs = pfDupRemList(remotefs);

	if (*pos == 1) {	/* new head */
		axe = newremotefs;
		newremotefs = axe->c_next;
	} else {
		for (i = 1, rem = newremotefs; i < *pos - 1; ++i)
			rem = rem->c_next;
		axe = rem->c_next;
		rem->c_next = axe->c_next;
	}
	pfFreeRemList(remotefs);
	pfSetRemoteFS(newremotefs);
	/* update summary and widgets */
	updateSummary();
	XmTextFieldSetString(text[0], axe->c_hostname);
	remoteServerCB(text[0], clientD, callD);
	XmTextFieldSetString(text[2], axe->c_export_path);
	XmTextFieldSetString(text[3], axe->c_mnt_pt);
	pfFreeRem(axe);

}


/* ARGSUSED */
void
remoteTestCB(Widget w, XtPointer clientD, XtPointer callD)
{
	Remote_FS *rem;
	pfErCode er;
	char *data[4];
	int i, test_result;

	if (debug)
		(void) printf("pfgremote:remoteTestCB\n");


	/* gather data from widgets */

	for (i = 0; i < 4; ++i) {
		data[i] = XmTextFieldGetString(text[i]);
		if (i != 1 && !strlen(data[i])) {
			pfAppWarn(0, PFG_RM_BLANK);
			XmProcessTraversal(text[i],
				XmTRAVERSE_CURRENT);
			while (--i >= 0)
				if (*data[i])
					XtFree(data[i]);
			return;
		}
	}
	rem = pfNewRem(NOT_TESTED, data[3], data[0], data[1], data[2], "");

	for (i = 0; i < 4; ++i)
		if (*data[i])
			XtFree(data[i]);


	/* validate and display result */

	if (er = pfValidRem(rem)) {
		pfWarn(er, pfErMessage(er), NULL);
		pfFreeRem(rem);
		return;
	}

	pfgBusy(pfgShell(w));

	test_result = test_mount(rem, 20);

	pfgUnbusy(pfgShell(w));

	if (test_result)
		pfWarn(5000, PFG_RM_FAILED, rem->c_mnt_pt);
	else {
		Widget dialog, button;
		XmString string;
		Arg arg[10];
		char buf[1000];

		(void) sprintf(buf, PFG_RM_SUCCESS, rem->c_mnt_pt);

		string = XmStringCreateLtoR(buf, XmFONTLIST_DEFAULT_TAG);

		XtSetArg(arg[0], XmNdialogStyle,
		    XmDIALOG_FULL_APPLICATION_MODAL);
		XtSetArg(arg[1], XmNmessageString, string);
		dialog = XmCreateMessageDialog(pfgShell(w),
		    "messageDialog", arg, 2);
		XtVaSetValues(pfgShell(dialog),
		    XmNtitle, TITLE_REMOTEMOUNT_STATUS,
		    NULL);
		XtVaSetValues(dialog,
		    XmNmarginHeight, 0,
		    XmNmarginWidth, 0,
		    NULL);
		/* remove default buttons added by some libraries */
		button = XmMessageBoxGetChild(dialog,
			XmDIALOG_CANCEL_BUTTON);
		if (button)
			XtUnmanageChild(button);
		button = XmMessageBoxGetChild(dialog,
			XmDIALOG_HELP_BUTTON);
		if (button)
			XtUnmanageChild(button);

		XmStringFree(string);
		XtManageChild(dialog);
	}
	pfFreeRem(rem);
}


static void
updateSummary(void)
{
	XmString *xmstrlist;
	int remcount = 0, i;
	Remote_FS *rem, *remotefs = NULL;
	char buf[1000];

	remotefs = pfGetRemoteFS();
	for (rem = remotefs; rem; rem = rem->c_next)
		++remcount;

	if (!remcount) {
		XmListDeleteAllItems(
			pfgGetNamedWidget(widget_list, "summaryList"));
		return;
	}
	xmstrlist = (XmString *) xmalloc(remcount * sizeof (XmString));
	for (i = 0, rem = remotefs; i < remcount; ++i,
	    rem = rem->c_next) {
		(void) sprintf(buf, "%s %s:%s",	/* add \n after first %s */
		    rem->c_mnt_pt,
		    rem->c_hostname,
		    rem->c_export_path);
		xmstrlist[i] = XmStringCreateLtoR(buf, XmFONTLIST_DEFAULT_TAG);
	}

	XmListDeleteAllItems(pfgGetNamedWidget(widget_list, "summaryList"));
	XmListAddItems(pfgGetNamedWidget(widget_list, "summaryList"),
		xmstrlist, remcount, 1);

	for (i = 0; i < remcount; ++i)
		XmStringFree(xmstrlist[i]);
}


/*
 * give the showmount command 60 seconds to time out
 */
#define	SHOWMOUNT_TIMEOUT 60
static FILE *fp;

/* ARGSUSED */
static void
sigalarm_handler(int sig)
{
	(void) pclose(fp);
	if (debug)
		(void) printf("pfgremote: sigalarm_handler...\07\n");
}


static void
updateRemlist(void)
{
	char *cmd = "/usr/sbin/showmount -e";
	char cmdbuf[256], buf[256], *cp, *server;
	int i = 0, count = 0;
	void (*savesig) (int);
	pfName_t *namelist = NULL, *name;
	XmString *xmstrlist;

	server = XmTextFieldGetString(text[0]);
	if (!strlen(server)) {
		XmListDeleteAllItems(
			pfgGetNamedWidget(widget_list, "remoteList"));
		return;
	}

	/*
	 * set up a mechanism to 'break' out of the popen() if the server is
	 * not responding...
	 */
	savesig = signal(SIGALRM, sigalarm_handler);
	(void) alarm(SHOWMOUNT_TIMEOUT);

	(void) sprintf(cmdbuf, "%s %s 2>& 1", cmd, server);

	if (fp = popen(cmdbuf, "r")) {
		while (fgets(buf, 255, fp)) {
			if (!*buf)
				continue;
			else if (strstr(buf, "export list"))
				continue;
			else if (strstr(buf, "Rpcbind failure"))
				break;
			else if (strstr(buf, "not registered"))
				break;
			else if (strstr(buf, "Name to address translation"))
				break;
			else if (strstr(buf, "showmount:"))
				break;
			if ((cp = strrchr(buf, '(')) != NULL ||
			    (cp = strrchr(buf, ' ')) != NULL) {
				*cp-- = '\0';
				while (*cp == ' ')
					*cp-- = '\0';
			}
			(void) pfAppendName(&namelist, pfNewName(buf));
		}
		(void) pclose(fp);
	}
	(void) alarm(0);
	(void) signal(SIGALRM, savesig);

	for (name = namelist; name; name = name->next)
		++count;

	if (!count) {
		XmListDeleteAllItems(
			pfgGetNamedWidget(widget_list, "remoteList"));
		return;
	}
	xmstrlist = (XmString *) xmalloc(count * sizeof (XmString));
	for (i = 0, name = namelist; i < count; ++i, name = name->next)
		xmstrlist[i] = XmStringCreateLocalized(name->name);


	XmListDeleteAllItems(pfgGetNamedWidget(widget_list, "remoteList"));
	XmListAddItems(pfgGetNamedWidget(widget_list, "remoteList"),
			xmstrlist, count, 1);

	for (i = 0; i < count; ++i)
		XmStringFree(xmstrlist[i]);
	pfFreeNameList(namelist);
}
******** serverLabel : XmLabel *****************/
  n = 0;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNrightWidget, widget_array[WI_SERVERTEXT]); n++;
  XtSetArg(args[n], XmNrightOffset, 5); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
  XtSetArg(args[n], XmNbottomWidget, widget_array[WI_SERVERTEXT]); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
  XtSetArg(args[n], XmNtopWidget, widget_array[WI_SERVERTEXT]); n++;
  XtSetValues(widget_array[WI_SERVERLABEL], args, n);

  XtManageChild(widget_array[WI_SERVERLABEL]);

  /***************** serverText : XmTextField *****************/
  n = 0;
  XtAddCallback(widget_array[WI_SERVERTEXT],
                XmNactivateCallback,
                (XtCallbackProc)remoteServerCB,
                (XtPointer)NULL);

  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_SERVERTEXT], args, n);

  XtManageChild(widget_array[WI_SERVERTEXT]);
  XtManageChild(widget_array[WI_SERVERLABELTEXT]);

  /***************** ipLabelText : XmForm *****************/
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNtopWidget, widget_array[WI_SERVERLABELTEXT]); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
  XtSetArg(args[n], XmNrightWidget, widget_array[WI_SERVERLABELTEXT]); n++;
  XtSetValues(widget_array[WI_IPLABELTEXT], args, n);


  /***************** ipLabel : XmLabel *****************/
  n = 0;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNrightWidget, widget_array[WI_IPTEXT]); n++;
  XtSetArg(args[n], XmNrightOffset, 5); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
  XtSetArg(args[n], XmNbottomWidget, widget_array[WI_IPTEXT]); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
  XtSetArg(args[n], XmNtopWidget, widget_array[WI_IPTEXT]); n++;
  XtSetValues(widget_array[WI_IPLABEL], args, n);

  XtManageChild(widget_array[WI_IPLABEL]);

  /***************** ipText : XmTextField *****************/
  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_IPTEXT], args, n);

  XtManageChild(widget_array[WI_IPTEXT]);
  XtManageChild(widget_array[WI_IPLABELTEXT]);

  /***************** filesysLabelText : XmForm *****************/
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNtopWidget, widget_array[WI_IPLABELTEXT]); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
  XtSetArg(args[n], XmNrightWidget, widget_array[WI_IPLABELTEXT]); n++;
  XtSetValues(widget_array[WI_FILESYSLABELTEXT], args, n);


  /***************** filesysLabel : XmLabel *****************/
  n = 0;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNrightWidget, widget_array[WI_FILESYSTEXT]); n++;
  XtSetArg(args[n], XmNrightOffset, 5); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
  XtSetArg(args[n], XmNbottomWidget, widget_array[WI_FILESYSTEXT]); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
  XtSetArg(args[n], XmNtopWidget, widget_array[WI_FILESYSTEXT]); n++;
  XtSetValues(widget_array[WI_FILESYSLABEL], args, n);

  XtManageChild(widget_array[WI_FILESYSLABEL]);

  /***************** filesysText : XmTextField *****************/
  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_FILESYSTEXT], args, n);

  XtManageChild(widget_array[WI_FILESYSTEXT]);
  XtManageChild(widget_array[WI_FILESYSLABELTEXT]);

  /***************** remoteList : XmScrolledList *****************/
  pn = 0;
  XtAddCallback(widget_array[WI_REMOTELIST],
                XmNbrowseSelectionCallback,
                (XtCallbackProc)remoteRemlistCB,
                (XtPointer)NULL);

  XtSetArg(pargs[pn], XmNtopAttachment, XmATTACH_WIDGET); pn++;
  XtSetArg(pargs[pn], XmNtopWidget, widget_array[WI_FILESYSLABELTEXT]); pn++;
  XtSetArg(pargs[pn], XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET); pn++;
  XtSetArg(pargs[pn], XmNrightWidget, widget_array[WI_FILESYSLABELTEXT]); pn++;
  XtSetArg(pargs[pn], XmNrightOffset, -16); pn++;
  XtSetArg(pargs[pn], XmNbottomAttachment, XmATTACH_WIDGET); pn++;
  XtSetArg(pargs[pn], XmNbottomWidget, widget_array[WI_MOUNTPOINTLABELTEXT]); pn++;
  tmpw = get_constraint_widget(widget_array[WI_REMOTELIST], widget_array[WI_FORM]);
  if (tmpw)
    XtSetValues(tmpw, pargs, pn);

  XtManageChild(widget_array[WI_REMOTELIST]);

  /***************** mountPointLabelText : XmForm *****************/
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
  XtSetArg(args[n], XmNrightWidget, widget_array[WI_FILESYSLABELTEXT]); n++;
  XtSetValues(widget_array[WI_MOUNTPOINTLABELTEXT], args, n);


  /***************** mountPointLabel : XmLabel *****************/
  n = 0;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNrightWidget, widget_array[WI_MOUNTPOINTTEXT]); n++;
  XtSetArg(args[n], XmNrightOffset, 5); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
  XtSetArg(args[n], XmNbottomWidget, widget_array[WI_MOUNTPOINTTEXT]); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
  XtSetArg(args[n], XmNtopWidget, widget_array[WI_MOUNTPOINTTEXT]); n++;
  XtSetValues(widget_array[WI_MOUNTPOINTLABEL], args, n);

  XtManageChild(widget_array[WI_MOUNTPOINTLABEL]);

  /***************** mountPointText : XmTextField *****************/
  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_MOUNTPOINTTEXT], args, n);

  XtManageChild(widget_array[WI_MOUNTPOINTTEXT]);
  XtManageChild(widget_array[WI_MOUNTPOINTLABELTEXT]);

  /***************** buttonRowColumn : XmRowColumn *****************/
  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
  tmpw1 = get_constraint_widget(
widget_array[WI_REMOTELIST], widget_array[WI_FORM]);
  XtSetArg(args[n], XmNleftWidget, tmpw1); n++;
  XtSetArg(args[n], XmNtopWidget, widget_array[WI_FILESYSLABELTEXT]); n++;
  XtSetArg(args[n], XmNleftOffset, 10); n++;
  XtSetValues(widget_array[WI_BUTTONROWCOLUMN], args, n);

  XtAddCallback(widget_array[WI_ADDBUTTON],
                XmNactivateCallback,
                (XtCallbackProc)remoteAddCB,
                (XtPointer)NULL);

  XtManageChild(widget_array[WI_ADDBUTTON]);
  XtAddCallback(widget_array[WI_REMOVEBUTTON],
                XmNactivateCallback,
                (XtCallbackProc)remoteRemoveCB,
                (XtPointer)NULL);

  XtManageChild(widget_array[WI_REMOVEBUTTON]);
  XtAddCallback(widget_array[WI_TESTBUTTON],
                XmNactivateCallback,
                (XtCallbackProc)remoteTestCB,
                (XtPointer)NULL);

  XtManageChild(widget_array[WI_TESTBUTTON]);
  XtManageChild(widget_array[WI_BUTTONROWCOLUMN]);

  /***************** remoteLabel : XmLabel *****************/
  n = 0;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNrightOffset, 50); n++;
  XtSetValues(widget_array[WI_REMOTELABEL], args, n);

  XtManageChild(widget_array[WI_REMOTELABEL]);

  /***************** summaryList : XmScrolledList *****************/
  pn = 0;
  XtSetArg(pargs[pn], XmNrightAttachment, XmATTACH_FORM); pn++;
  XtSetArg(pargs[pn], XmNbottomAttachment, XmATTACH_FORM); pn++;
  XtSetArg(pargs[pn], XmNtopAttachment, XmATTACH_WIDGET); pn++;
  XtSetArg(pargs[pn], XmNtopWidget, widget_array[WI_REMOTELABEL]); pn++;
  XtSetArg(pargs[pn], XmNleftAttachment, XmATTACH_WIDGET); pn++;
  XtSetArg(pargs[pn], XmNleftWidget, widget_array[WI_BUTTONROWCOLUMN]); pn++;
  XtSetArg(pargs[pn], XmNleftOffset, 20); pn++;
  tmpw = get_constraint_widget(widget_array[WI_SUMMARYLIST], widget_array[WI_FORM]);
  if (tmpw)
    XtSetValues(tmpw, pargs, pn);

  XtManageChild(widget_array[WI_SUMMARYLIST]);
  XtManageChild(widget_array[WI_FORM]);

  /***************** messageBox : XmMessageBox *****************/
  n = 0;
  XtSetArg(args[n], XmNdefaultButton, widget_array[WI_CONTINUEBUTTON]); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_MESSAGEBOX], args, n);

  XtAddCallback(widget_array[WI_CONTINUEBUTTON],
                XmNactivateCallback,
                (XtCallbackProc)remoteContinueCB,
                (XtPointer)NULL);

  XtManageChild(widget_array[WI_CONTINUEBUTTON]);
  XtAddCallback(widget_array[WI_CANCELBUTTON],
                XmNactivateCallback,
                (XtCallbackProc)remoteCancelCB,
                (XtPointer)NULL);

  XtManageChild(widget_array[WI_CANCELBUTTON]);
  XtAddCallback(widget_array[WI_HELPBUTTON],
                XmNactivateCallback,
                pfgHelp,
                (XtPointer)"mount.r");

  XtManageChild(widget_array[WI_HELPBUTTON]);
  XtManageChild(widget_array[WI_MESSAGEBOX]);

  /*
   *  Allocate memory for the widget array to return
   */
  if (warr_ret != NULL) {
    *warr_ret = (Widget *) malloc(sizeof(Widget)*29);
    (void) memcpy((char *)*warr_ret,
                  (char *)widget_array,
           sizeof(Widget)*29);
  }

  /*
   *   Fix for SMI's X/NeWS server
   */
  tu_ol_fix_hierarchy(widget_array[WI_REMOTE_DIALOG]);

  /*
   *   Invoke the create callbacks
   */
  tu_widcre_invoke_hooks(widget_array[WI_REMOTE_DIALOG]);

  /*
   *   Return the first created widget.
   */
  return widget_array[WI_REMOTE_DIALOG];
}



/****************************************************************
 * create_method:
 *     This function creates a widget hierarchy using the
 *     functions generated above.
 ****************************************************************/
static Widget create_method(char               * temp,
                            char               * name,
                            Widget               parent,
                            Display            * disp,
                            Screen             * screen,
                            tu_template_descr  * retval)
{
  Widget w;

  sDisplay = disp;
  sScreen = screen;

  /* check each node against its name and call its
   * create function if appropriate */
  w = NULL;
  if (strcmp(temp, "remote_dialog") == 0){
    w = tu_remote_dialog_widget(name, parent, (Widget **)retval);
  }

  sDisplay = NULL;
  sScreen = NULL;
  return w;
}

