
/* Copyright 1993 Sun Microsystems, Inc. */

#pragma ident "@(#)add_host.c	1.14 95/11/14 Sun Microsystems"

/*	add_host.c */

#include <stdlib.h>
#include <libintl.h>
#include <nl_types.h>
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

#define HOSTNAME_LABEL	catgets(_catd, 8, 9, "Host Name")
#define IP_LABEL	catgets(_catd, 8, 10, "IP Address")
#define ALIASES_LABEL	catgets(_catd, 8, 11, "Aliases")

extern nl_catd	_catd;	/* for catgets(), defined in main.c */

typedef	struct
{
	Widget	addHostDialog;
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
} addHostCtxt;

static void init_dialog(Widget dialog);
static void save_dialog_values(addHostCtxt * ctxt);

static char	errstr[1024] = "";

Widget 	addhostdialog = NULL;


/*******************************************************************************
       The following are callback functions.
*******************************************************************************/

static int
get_host_values(
	addHostCtxt* ctxt,
	char**	hostname,
	char**	ip,
	char**	aliases
)
{
	char		reqargs[128] = "";
	char		msgbuf[128] = "";
	int		conflict_status;


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
			sprintf(msgbuf, "%s %s", catgets(_catd, 8, 13, "Invalid"), HOSTNAME_LABEL);
			display_error(ctxt->addHostDialog, msgbuf);
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
		else if (!valid_host_ip_addr(*ip)) {
			sprintf(msgbuf, "%s %s", catgets(_catd, 8, 15, "Invalid"), IP_LABEL);
			display_error(ctxt->addHostDialog, msgbuf);
			return 0;
		}
	}

	if ( conflict_status = check_ns_host_conflicts(*hostname, *ip) ) {
		if (conflict_status == SYSMAN_CONFLICT_BOTH_USED) {
			sprintf(msgbuf, "%s",  
                		catgets(_catd, 8, 638,
				"the host and ip entries are being used in the name service host map"));
			if (!Confirm(ctxt->addHostDialog, msgbuf, NULL, catgets(_catd, 8, 641, "OK"))) {
				/* DO NOT perform the ADD. */
				return 0;
			}
		}
		else if (conflict_status == SYSMAN_CONFLICT_NAME_USED) {
			sprintf(msgbuf, "%s",  
                		catgets(_catd, 8, 639,
				"this host name is already being used in the the name service host map"));
			if (!Confirm(ctxt->addHostDialog, msgbuf, NULL, catgets(_catd, 8, 641, "OK"))) {
				/* DO NOT perform the ADD. */
				return 0;
			}
		}
		else if (conflict_status == SYSMAN_CONFLICT_ID_USED) {
			sprintf(msgbuf, "%s",  
                		catgets(_catd, 8, 640,
				"this ip address is already being used in the name service host map"));
			if (!Confirm(ctxt->addHostDialog, msgbuf, NULL, catgets(_catd, 8, 641, "OK"))) {
				/* DO NOT perform the ADD. */
				return 0;
			}
		}
	}

	if (reqargs[0] != '\0') {
		sprintf(errstr, MISSING_REQ_ARGS, reqargs);
		display_error(ctxt->addHostDialog, errstr);
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
addCB(
	Widget wgt, 
	XtPointer cd, 
	XtPointer cbs)
{
	addHostCtxt* ctxt = (addHostCtxt*)cd;
	SysmanHostArg	host;
	int	sts;
	char*	hostname;
	char*	ipaddr;
	char*	aliases;
	char	buf[256];


	if (!get_host_values(
			(addHostCtxt*)ctxt,
			&hostname,
			&ipaddr,
			NULL)) {
		return;
   	}

	/*
	if (!check_unique(hostname, ipaddr, etheraddr, NULL, addhostdialog))
		return;
	*/

	SetBusyPointer(True);

	memset((void *)&host, 0, sizeof (host));

	host.hostname = hostname;
	host.hostname_key = hostname;
	host.ipaddr = ipaddr;
	host.ipaddr_key = ipaddr;
	host.aliases = NULL;

	sts = sysman_add_host(&host, errbuf, ERRBUF_SIZE);

	if (sts == 0) {
		add_host_to_list(&host);
		save_dialog_values(ctxt);

		if (wgt == ctxt->okPushbutton) {
			XtPopdown(XtParent(addhostdialog));
		}
	}
	else {
		display_error(addhostdialog, errbuf);
	}

	SetBusyPointer(False);

}

static void
ResetPushbutton_activateCB(
	Widget wgt, 
	XtPointer cd, 
	XtPointer cbs)
{
	addHostCtxt* ctxt = (addHostCtxt*)cd;


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
	addHostCtxt* ctxt = (addHostCtxt*)cd;

	XtPopdown(XtParent(ctxt->addHostDialog));
}


Widget	build_addHostDialog(Widget parent)
{
	addHostCtxt*	ctxt;
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


	ctxt = (addHostCtxt*) malloc(sizeof(addHostCtxt));

	ctxt->save_hostname = NULL;
	ctxt->save_ipaddr = NULL;
	ctxt->save_aliases = NULL;
	ctxt->save_comment = NULL;

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

	ctxt->addHostDialog = XtVaCreateWidget( "AddHostDialog",
		xmFormWidgetClass,
		shell,
		XmNunitType, XmPIXELS,
		RES_CONVERT(XmNdialogTitle, catgets(_catd, 8, 16, "Admintool: Add Host")),
		NULL );

	XtVaSetValues(ctxt->addHostDialog,
		XmNuserData, (XtPointer)ctxt,
		NULL);

	bigrc = XtVaCreateManagedWidget( "bigrc",
		xmRowColumnWidgetClass,
		ctxt->addHostDialog,
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
		RES_CONVERT( XmNlabelString, catgets(_catd, 8, 17, "Host Name:") ),
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
		RES_CONVERT( XmNlabelString, catgets(_catd, 8, 18, "IP Address:") ),
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
		RES_CONVERT( XmNlabelString, catgets(_catd, 8, 19, "Aliases:") ),
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

	create_button_box(ctxt->addHostDialog, bigrc, NULL,
		&ctxt->okPushbutton, &ctxt->applyPushbutton,
		&ctxt->resetPushbutton, &ctxt->cancelPushbutton,
		&ctxt->helpPushbutton);

	XtVaSetValues(ctxt->addHostDialog,
		XmNinitialFocus, ctxt->nameText,
		NULL);

	XtAddCallback(ctxt->okPushbutton, XmNactivateCallback,
		(XtCallbackProc) addCB,
		(XtPointer) ctxt);

	XtAddCallback(ctxt->helpPushbutton, XmNactivateCallback,
		(XtCallbackProc) helpCB,
		"hosts_window.r.hlp" );

	XtAddCallback(ctxt->applyPushbutton, XmNactivateCallback,
		(XtCallbackProc) addCB,
		(XtPointer) ctxt);

	XtAddCallback( ctxt->resetPushbutton, XmNactivateCallback,
		(XtCallbackProc) ResetPushbutton_activateCB,
		(XtPointer) ctxt);

	XtAddCallback( ctxt->cancelPushbutton, XmNactivateCallback,
		(XtCallbackProc) CancelPushbutton_activateCB,
		(XtPointer) ctxt);

	XtVaSetValues(ctxt->addHostDialog,
		XmNdefaultButton, ctxt->okPushbutton,
		NULL);

			
	return ( ctxt->addHostDialog );
}


void	show_addhostdialog(Widget parent, sysMgrMainCtxt * mgrctxt)
{
	addHostCtxt*	ctxt;
	Widget		type_widget;


	if (addhostdialog && XtIsManaged(XtParent(addhostdialog))) {
		XtPopup(XtParent(addhostdialog), XtGrabNone);
		return;
	}

	SetBusyPointer(True);

	if (addhostdialog == NULL) {
		addhostdialog = build_addHostDialog(parent);
	}

	mgrctxt->currDialog = addhostdialog;
	XtVaGetValues(addhostdialog,
		XmNuserData, &ctxt,
		NULL);

	init_dialog(addhostdialog);
	save_dialog_values(ctxt);

	XtManageChild(addhostdialog);
	XtPopup(XtParent(addhostdialog), XtGrabNone);
	SetBusyPointer(False);
}

static void
init_dialog(Widget dialog)
{
	addHostCtxt	*ctxt;


	XtVaGetValues(dialog,
		XmNuserData, &ctxt,
		NULL);

	XtVaSetValues(ctxt->nameText,
		XmNvalue, "",
		NULL);

	XtVaSetValues(ctxt->ipText,
		XmNvalue, "",
		NULL);

	XtVaSetValues(ctxt->aliasesText,
	XmNvalue, "",
	NULL);
}

static void
save_dialog_values(addHostCtxt * ctxt)
{
	if (ctxt->save_hostname) XtFree(ctxt->save_hostname);
	if (ctxt->save_ipaddr) XtFree(ctxt->save_ipaddr);
	if (ctxt->save_aliases) XtFree(ctxt->save_aliases);

	ctxt->save_hostname = XmTextFieldGetString(ctxt->nameText);
	ctxt->save_ipaddr = XmTextFieldGetString(ctxt->ipText);
	ctxt->save_aliases = XmTextFieldGetString(ctxt->aliasesText);
}


