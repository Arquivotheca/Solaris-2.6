/* Copyright 1993 Sun Microsystems, Inc. */

#pragma ident "@(#)modify_host.c	1.13 95/11/14 Sun Microsystems"

/*	modify_host.c */

#include <stdlib.h>
#include <libintl.h>
#include <nl_types.h>
#include <stdio.h>
#include <Xm/Xm.h>
#include <Xm/RowColumn.h>
#include <Xm/TextF.h>
#include <Xm/Form.h>
#include <Xm/DialogS.h>
#include <Xm/LabelG.h>
#include <Xm/SeparatoG.h>
#include <Xm/PushBG.h>
#include <Xm/MessageB.h>

#include "sysman_iface.h"
#include "util.h"
#include "UxXt.h"
/* #include "valid.h" */

#define HOSTNAME_LABEL	catgets(_catd, 8, 200, "Host Name")
#define IP_LABEL	catgets(_catd, 8, 201, "IP Address")
#define ALIASES_LABEL	catgets(_catd, 8, 202, "Aliases")

extern nl_catd	_catd;	/* for catgets(), defined in main.c */

typedef	struct
{
	Widget	modifyHostDialog;
	Widget	nameForm;
	Widget	nameLabel;
	Widget	nameText;
	Widget	nameList;
	Widget	ipForm;
	Widget	ipLabel;
	Widget	ipText;
	Widget	aliasesForm;
	Widget	aliasesLabel;
	Widget	aliasesText;
	Widget	okPushbutton;
	Widget	applyPushbutton;
	Widget	resetPushbutton;
	Widget	cancelPushbutton;
	Widget	helpPushbutton;

	char*	save_hostname;
	char*	save_ipaddr;
	char*	save_aliases;
	char*	save_comment;

	SysmanHostArg host;
} modifyHostCtxt;

static void save_dialog_values(modifyHostCtxt * ctxt);

static char	errstr[1024] = "";

Widget 	modifyhostdialog = NULL;


/*******************************************************************************
       The following are callback functions.
*******************************************************************************/

static int
get_host_values(
	modifyHostCtxt* ctxt,
	char**	hostname,
	char**	ip,
	char**	aliases
)
{
	char		reqargs[128] = "";
	char		msgbuf[128] = "";


	if (hostname != NULL) {
		XtVaGetValues(ctxt->nameText,
			XmNvalue, hostname,
			NULL);
		if (*hostname[0] == '\0') {
			if (reqargs[0] != '\0')
				strcat(reqargs, ", ");
			strcat(reqargs, HOSTNAME_LABEL);
		}
		else if (!valid_hostname(*hostname)) {
			sprintf(msgbuf, "%s %s", catgets(_catd, 8, 204, "Invalid"), HOSTNAME_LABEL);
			display_error(ctxt->modifyHostDialog, msgbuf);
			return 0;
		}
	}
	
	if (ip != NULL) {
		XtVaGetValues(ctxt->ipText,
			XmNvalue, ip,
			NULL);
		if (*ip[0] == '\0') {
			if (reqargs[0] != '\0')
				strcat(reqargs, ", ");
			strcat(reqargs, IP_LABEL);
		}
	}

	if (reqargs[0] != '\0') {
		sprintf(errstr, MISSING_REQ_ARGS, reqargs);
		display_error(ctxt->modifyHostDialog, errstr);
		return 0;
	}
	reqargs[0] = '\0';

	if (aliases != NULL) {
		XtVaGetValues(ctxt->aliasesText,
			XmNvalue, aliases,
			NULL);
	}

	return 1;
}

static void
modifyCB(
	Widget wgt, 
	XtPointer cd, 
	XtPointer cbs)
{
	extern void update_entry(void*);
	modifyHostCtxt* ctxt = (modifyHostCtxt*)cd;
	SysmanHostArg	host;
	int	sts;
	char*	hostname;
	char*	ipaddr;
	char	buf[256];


	if (!get_host_values(
			(modifyHostCtxt*)ctxt,
			&hostname,
			&ipaddr,
			NULL)) {
		return;
   	}

	/*
	if (!check_unique(hostname, ipaddr, etheraddr, NULL, modifyhostdialog))
		return;
	*/

	SetBusyPointer(True);

	memset((void *)&host, 0, sizeof (host));
	host.hostname = hostname;
	host.hostname_key = ctxt->host.hostname_key;
	host.ipaddr = ipaddr;
	host.ipaddr_key = ctxt->host.ipaddr_key;
	host.aliases = NULL;

	sts = sysman_modify_host(&host, errbuf, ERRBUF_SIZE);

	if (sts == 0) {
		update_entry(&host);

		free_host(&ctxt->host);
		copy_host(&ctxt->host, &host);

		save_dialog_values(ctxt);

		if (wgt == ctxt->okPushbutton) {
			XtPopdown(XtParent(modifyhostdialog));
		}
	}
	else {
		display_error(modifyhostdialog, errbuf);
	}

	SetBusyPointer(False);
}

static void
ResetPushbutton_activateCB(
	Widget wgt, 
	XtPointer cd, 
	XtPointer cbs)
{
	modifyHostCtxt* ctxt = (modifyHostCtxt*)cd;


	XtVaSetValues(ctxt->nameText,
		XmNvalue, ctxt->save_hostname,
		NULL);
	XtVaSetValues(ctxt->ipText,
		XmNvalue, ctxt->save_ipaddr,
		NULL);
	XtVaSetValues(ctxt->aliasesText,
		XmNvalue, ctxt->save_aliases,
		NULL);
}

static void	CancelPushbutton_activateCB(
		Widget wgt, 
		XtPointer cd, 
		XtPointer cbs)
{
	modifyHostCtxt* ctxt = (modifyHostCtxt*)cd;

	XtPopdown(XtParent(ctxt->modifyHostDialog));
}


Widget	build_modifyHostDialog(Widget parent)
{
	modifyHostCtxt*	ctxt;
	Widget		shell;
	Widget		menushell;
	Widget		pulldown;
	Widget		bigrc;
	char **		s;
	XmString	xstr;
	Widget		w;
	int		i;
	int		wnum;
	Widget		wlist[6];
	Widget		maxlabel;
	Dimension	width;
	Dimension	maxwidth = 0;


	ctxt = (modifyHostCtxt*) malloc(sizeof(modifyHostCtxt));

	ctxt->save_hostname = NULL;
	ctxt->save_ipaddr = NULL;
	ctxt->save_aliases = NULL;
	ctxt->save_comment = NULL;

	/* null structure */
	memset(&ctxt->host, 0, sizeof(SysmanHostArg));

	if (parent == NULL)
	{
		parent = GtopLevel;
	}

	shell = XtVaCreatePopupShell( "AddHostDialog_shell",
		xmDialogShellWidgetClass, parent,
		XmNshellUnitType, XmPIXELS,
		XmNallowShellResize, True,
		XmNminWidth, 340,
		XmNminHeight, 150,
		NULL );

	ctxt->modifyHostDialog = XtVaCreateWidget( "ModifyHostDialog",
		xmFormWidgetClass,
		shell,
		XmNunitType, XmPIXELS,
		RES_CONVERT(XmNdialogTitle, catgets(_catd, 8, 206, "Admintool: Modify Host")),
		NULL );

	XtVaSetValues(ctxt->modifyHostDialog,
		XmNuserData, (XtPointer)ctxt,
		NULL);

	bigrc = XtVaCreateManagedWidget( "bigrc",
		xmRowColumnWidgetClass,
		ctxt->modifyHostDialog,
		XmNorientation, XmVERTICAL,
		XmNnumColumns, 1,
		XmNspacing, 2,
		XmNmarginHeight, 1,
		XmNmarginWidth, 1,
		XmNtopAttachment, XmATTACH_FORM,
		XmNtopOffset, 10,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 2,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 2,
		NULL );

	ctxt->nameForm = XtVaCreateManagedWidget( "NameForm",
		xmFormWidgetClass,
		bigrc,
		XmNhorizontalSpacing, 10,
		XmNfractionBase, 1,
		NULL );

	ctxt->nameLabel = XtVaCreateManagedWidget( "NameLabel",
		xmLabelGadgetClass,
		ctxt->nameForm,
		XmNalignment, XmALIGNMENT_END,
		RES_CONVERT( XmNlabelString, catgets(_catd, 8, 207, "Host Name:") ),
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNtopPosition, 0,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNbottomPosition, 1,
		NULL );

	ctxt->nameText = XtVaCreateManagedWidget( "NameText",
		xmTextFieldWidgetClass,
		ctxt->nameForm,
		XmNmaxLength, 256,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, ctxt->nameLabel,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNtopPosition, 0,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNbottomPosition, 1,
		NULL );

	ctxt->ipForm = XtVaCreateManagedWidget( "IPForm",
		xmFormWidgetClass,
		bigrc,
		XmNhorizontalSpacing, 10,
		XmNfractionBase, 1,
		NULL );

	ctxt->ipLabel = XtVaCreateManagedWidget( "IPLabel",
		xmLabelGadgetClass,
		ctxt->ipForm,
		XmNalignment, XmALIGNMENT_END,
		RES_CONVERT( XmNlabelString, catgets(_catd, 8, 208, "IP Address:") ),
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNtopPosition, 0,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNbottomPosition, 1,
		NULL );

	ctxt->ipText = XtVaCreateManagedWidget( "IPText",
		xmTextFieldWidgetClass,
		ctxt->ipForm,
		XmNmaxLength, 15,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, ctxt->ipLabel,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNtopPosition, 0,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNbottomPosition, 1,
		NULL );

	ctxt->aliasesForm = XtVaCreateWidget( "AliasesForm",
		xmFormWidgetClass,
		bigrc,
		XmNhorizontalSpacing, 10,
		XmNfractionBase, 1,
		NULL );

	ctxt->aliasesLabel = XtVaCreateManagedWidget( "AliasesLabel",
		xmLabelGadgetClass,
		ctxt->aliasesForm,
		XmNalignment, XmALIGNMENT_END,
		RES_CONVERT( XmNlabelString, catgets(_catd, 8, 209, "Aliases:") ),
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNtopPosition, 0,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNbottomPosition, 1,
		NULL );

	ctxt->aliasesText = XtVaCreateManagedWidget( "AliasesText",
		xmTextFieldWidgetClass,
		ctxt->aliasesForm,
		XmNmaxLength, 256,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, ctxt->aliasesLabel,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNtopPosition, 0,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNbottomPosition, 1,
		NULL );


	wnum = 3;
	wlist[0] = ctxt->nameLabel;
	wlist[1] = ctxt->ipLabel;
	wlist[2] = ctxt->aliasesLabel;
	for (i=0; i<wnum; i++) {
		XtVaGetValues(wlist[i],
			XmNwidth, &width,
			NULL);
		if (width > maxwidth) {
			maxwidth = width;
			maxlabel = wlist[i];
		}
	}
	for (i=0; i<wnum; i++) {
		XtVaSetValues(wlist[i],
			XmNwidth, maxwidth,
			NULL);
	}

	create_button_box(ctxt->modifyHostDialog, bigrc, NULL,
		&ctxt->okPushbutton, &ctxt->applyPushbutton,
		&ctxt->resetPushbutton, &ctxt->cancelPushbutton,
		&ctxt->helpPushbutton);

	XtVaSetValues(ctxt->modifyHostDialog,
		XmNinitialFocus, ctxt->nameText,
		NULL);

	XtAddCallback(ctxt->okPushbutton, XmNactivateCallback,
		(XtCallbackProc) modifyCB,
		(XtPointer) ctxt);

	XtAddCallback(ctxt->helpPushbutton, XmNactivateCallback,
		(XtCallbackProc) helpCB,
		"hosts_window.r.hlp" );

	XtAddCallback(ctxt->applyPushbutton, XmNactivateCallback,
		(XtCallbackProc) modifyCB,
		(XtPointer) ctxt);

	XtAddCallback( ctxt->resetPushbutton, XmNactivateCallback,
		(XtCallbackProc) ResetPushbutton_activateCB,
		(XtPointer) ctxt);

	XtAddCallback( ctxt->cancelPushbutton, XmNactivateCallback,
		(XtCallbackProc) CancelPushbutton_activateCB,
		(XtPointer) ctxt);

	XtVaSetValues(ctxt->modifyHostDialog,
		XmNdefaultButton, ctxt->okPushbutton,
		NULL);

			
	return ( ctxt->modifyHostDialog );
}


void
show_modifyhostdialog(
	Widget parent,
	SysmanHostArg* host,
	sysMgrMainCtxt * mgrctxt
)
{
	modifyHostCtxt*	ctxt;
	Widget		type_widget;


	SetBusyPointer(True);

	if (modifyhostdialog == NULL) {
		modifyhostdialog = build_modifyHostDialog(parent);
	}

	mgrctxt->currDialog = modifyhostdialog;
	XtVaGetValues(modifyhostdialog,
		XmNuserData, &ctxt,
		NULL);

	free_host(&ctxt->host);
	copy_host(&ctxt->host, host);

	XtVaSetValues(ctxt->nameText,
		XmNvalue, host->hostname,
		NULL);

	XtVaSetValues(ctxt->ipText,
		XmNvalue, host->ipaddr,
		NULL);

	XtVaSetValues(ctxt->aliasesText,
		XmNvalue, "",
		NULL);
	save_dialog_values(ctxt);

	XtManageChild(modifyhostdialog);
	XtPopup(XtParent(modifyhostdialog), XtGrabNone);
	SetBusyPointer(False);
}

static void
save_dialog_values(modifyHostCtxt * ctxt)
{
	if (ctxt->save_hostname) XtFree(ctxt->save_hostname);
	if (ctxt->save_ipaddr) XtFree(ctxt->save_ipaddr);
	if (ctxt->save_aliases) XtFree(ctxt->save_aliases);

	ctxt->save_hostname = XmTextFieldGetString(ctxt->nameText);
	ctxt->save_ipaddr = XmTextFieldGetString(ctxt->ipText);
	ctxt->save_aliases = XmTextFieldGetString(ctxt->aliasesText);
}


