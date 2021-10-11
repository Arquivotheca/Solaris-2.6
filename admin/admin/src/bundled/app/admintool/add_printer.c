
/* Copyright 1993 Sun Microsystems, Inc. */

#pragma ident "@(#)add_printer.c	1.15 96/06/25 Sun Microsystems"

/*	add_local.c 	*/

#include <stdlib.h>
#include <nl_types.h>
#include <Xm/Xm.h>
#include <Xm/DialogS.h>
#include <Xm/MenuShell.h>
#include <Xm/List.h>
#include <Xm/ScrolledW.h>
#include <Xm/PushBG.h>
#include <Xm/Label.h>
#include <Xm/ToggleBG.h>
#include <Xm/RowColumn.h>
#include <Xm/TextF.h>
#include <Xm/LabelG.h>
#include <Xm/Form.h>

#include "UxXt.h"
#include "util.h"
#include "sysman_iface.h"

extern nl_catd	_catd;	/* for catgets(), defined in main.c */

typedef	struct
{
	Widget	UxAddLocalPrinter;
	Widget	UxNameForm;
	Widget	UxNameLabel;
	Widget	UxNameText;
	Widget	UxServerForm;
	Widget	UxServerLabel;
	Widget	UxServerText;
	Widget	UxCommentForm;
	Widget	UxCommentLabel;
	Widget	UxCommentText;
	Widget	UxPortForm;
	Widget	UxPortLabel;
	Widget	Uxmenu4_p1_shell;
	Widget	Uxmenu4_p1;
	Widget	UxPortOtherPushbutton;
	Widget	UxPortOptionMenu;
	Widget	UxTypeForm;
	Widget	UxTypeLabel;
	Widget	UxContentsForm;
	Widget	UxContentsLabel;
	Widget	Uxmenu2_p2;
	Widget	UxContentsOptionMenu;
	Widget	UxFaultForm;
	Widget	UxFaultLabel;
	Widget	UxDefaultTogglebutton;
	Widget	UxBannerTogglebutton;
	Widget	UxUserListForm;
	Widget	UxUserListLabel;
	Widget	UxscrolledWindow2;
	Widget	UxUserList;
	Widget	UxAddPushbutton;
	Widget	UxDeletePushbutton;
	Widget	UxUserText;
	Widget	Uxmenu3_p1;
	Widget	UxWritePushbutton;
	Widget	UxMailPushbutton;
	Widget	UxNonePushbutton;
	Widget	UxFaultOptionMenu;
	Widget	Uxmenu2_p1;
	Widget	UxTypeOtherPushbutton;
	Widget	UxTypeOptionMenu;
	Widget	UxOptionsForm;
	Widget	UxOptionsLabel;
	Widget	UxOKPushbutton;
	Widget	UxApplyPushbutton;
	Widget	UxResetPushbutton;
	Widget	UxCancelPushbutton;
	Widget	UxHelpPushbutton;
	Widget	UxUxParent;
} _UxCAddLocalPrinter;

static _UxCAddLocalPrinter     *UxAddLocalPrinterContext;
#define AddLocalPrinter		UxAddLocalPrinterContext->UxAddLocalPrinter
#define NameForm		UxAddLocalPrinterContext->UxNameForm
#define NameLabel		UxAddLocalPrinterContext->UxNameLabel
#define NameText		UxAddLocalPrinterContext->UxNameText
#define ServerForm		UxAddLocalPrinterContext->UxServerForm
#define ServerLabel		UxAddLocalPrinterContext->UxServerLabel
#define ServerText		UxAddLocalPrinterContext->UxServerText
#define CommentForm		UxAddLocalPrinterContext->UxCommentForm
#define CommentLabel		UxAddLocalPrinterContext->UxCommentLabel
#define CommentText		UxAddLocalPrinterContext->UxCommentText
#define PortForm		UxAddLocalPrinterContext->UxPortForm
#define PortLabel		UxAddLocalPrinterContext->UxPortLabel
#define menu4_p1_shell		UxAddLocalPrinterContext->Uxmenu4_p1_shell
#define menu4_p1		UxAddLocalPrinterContext->Uxmenu4_p1
#define PortOtherPushbutton	UxAddLocalPrinterContext->UxPortOtherPushbutton
#define PortOptionMenu		UxAddLocalPrinterContext->UxPortOptionMenu
#define TypeForm		UxAddLocalPrinterContext->UxTypeForm
#define TypeLabel		UxAddLocalPrinterContext->UxTypeLabel
#define ContentsForm		UxAddLocalPrinterContext->UxContentsForm
#define ContentsLabel		UxAddLocalPrinterContext->UxContentsLabel
#define menu2_p2		UxAddLocalPrinterContext->Uxmenu2_p2
#define ContentsOptionMenu	UxAddLocalPrinterContext->UxContentsOptionMenu
#define FaultForm		UxAddLocalPrinterContext->UxFaultForm
#define FaultLabel		UxAddLocalPrinterContext->UxFaultLabel
#define DefaultTogglebutton	UxAddLocalPrinterContext->UxDefaultTogglebutton
#define BannerTogglebutton	UxAddLocalPrinterContext->UxBannerTogglebutton
#define UserListForm		UxAddLocalPrinterContext->UxUserListForm
#define UserListLabel		UxAddLocalPrinterContext->UxUserListLabel
#define scrolledWindow2		UxAddLocalPrinterContext->UxscrolledWindow2
#define UserList		UxAddLocalPrinterContext->UxUserList
#define AddPushbutton		UxAddLocalPrinterContext->UxAddPushbutton
#define DeletePushbutton	UxAddLocalPrinterContext->UxDeletePushbutton
#define separatorGadget2	UxAddLocalPrinterContext->UxseparatorGadget2
#define UserText		UxAddLocalPrinterContext->UxUserText
#define menu3_p1		UxAddLocalPrinterContext->Uxmenu3_p1
#define WritePushbutton		UxAddLocalPrinterContext->UxWritePushbutton
#define MailPushbutton		UxAddLocalPrinterContext->UxMailPushbutton
#define NonePushbutton		UxAddLocalPrinterContext->UxNonePushbutton
#define FaultOptionMenu		UxAddLocalPrinterContext->UxFaultOptionMenu
#define menu2_p1		UxAddLocalPrinterContext->Uxmenu2_p1
#define TypeOtherPushbutton	UxAddLocalPrinterContext->UxTypeOtherPushbutton
#define TypeOptionMenu		UxAddLocalPrinterContext->UxTypeOptionMenu
#define OptionsForm		UxAddLocalPrinterContext->UxOptionsForm
#define OptionsLabel		UxAddLocalPrinterContext->UxOptionsLabel
#define OKPushbutton		UxAddLocalPrinterContext->UxOKPushbutton
#define ApplyPushbutton		UxAddLocalPrinterContext->UxApplyPushbutton
#define ResetPushbutton		UxAddLocalPrinterContext->UxResetPushbutton
#define CancelPushbutton	UxAddLocalPrinterContext->UxCancelPushbutton
#define HelpPushbutton		UxAddLocalPrinterContext->UxHelpPushbutton
#define	UxParent		UxAddLocalPrinterContext->UxUxParent


#define	NONSTDTYPE	-1

Widget	create_AddLocalPrinter( Widget _UxUxParent );
void	show_addlocaldialog(Widget parent, sysMgrMainCtxt * ctxt);
void	build_port_optionmenu(int numports, char** ports,
		char* currentport, Widget optionmenu);
static void save_dialog_values(_UxCAddLocalPrinter * UxContext);

char * PrinterTypeLabels[] = {
	"PostScript",
	"HP Printer",
	"Reverse PostScript",
	"Epson 2500",
	"IBM ProPrinter",
	"Qume Sprint 5",
	"Daisy",
	"Diablo",
	"Datagraphix",
	"DEC LA100",
	"DEC LN03",
	"DECwriter",
	"Texas Instruments 800",
	NULL
};
char * PrinterTypeTokens[] = {
	"PS",
	"hplaser",
	"PSR",
	"epson2500",
	"ibmproprinter",
	"qume5",
	"daisy",
	"diablo",
	"datagraphix",
	"la100",
	"ln03",
	"decwriter",
	"ti800",
	NULL
};

char * FileContentsLabels[] = {
	"PostScript",
	"ASCII",
	"Both PostScript and ASCII",
	"None",
	"Any",
	NULL
};
char * FileContentsTokens[] = {
	"postscript",
	"simple",
	"postscript,simple",
	"",
	"any",
	NULL
};

char * FaultLabels[] = {
	"Write to superuser",
	"Mail to superuser",
	"None"
};
char * FaultTokens[] = {
	"write",
	"mail",
	"none"
};

static char *		save_printername;
static char *		save_comment;
static Widget		save_port;
static Widget		save_type;
static Widget		save_contents;
static Widget		save_fault;
static Boolean		save_sysdefault;
static Boolean		save_banner;
static char *		save_user;
static int		save_listcount;
static XmStringTable	save_list;

Widget	addlocaldialog;

extern void addPrinterUserListEntryCB(Widget, XtPointer, XtPointer);
extern void deletePrinterUserListEntryCB(Widget, XtPointer, XtPointer);


/*******************************************************************************
       The following are callback functions.
*******************************************************************************/
static void
OKPushbutton_activateCB(
	Widget wgt, 
	XtPointer cd, 
	XtPointer cb)
{
	Widget                  UxWidget = wgt;
	XtPointer               UxClientData = cd;
	XtPointer               UxCallbackArg = cb;
	_UxCAddLocalPrinter*	UxContext;
	char *		printername;
	char *		comment;
	char *		type;
	char *		port;
	char *		filecontents;
	char *		faultnotify;
	Boolean		defaultprinter;
	Boolean		banner;
	char *		userlist = NULL;
	int		listcount = 0;
	XmStringTable	list = NULL;
	Widget		w;
	char *		tmp;
	XmString	xstr;
	XtPointer	ptr;
	int		index;
	int		i;
	int		buflen;
	SysmanPrinterArg	printer;
	int		sts;
	char		errstr[512] = "";
	char		reqargs[64] = "";

	UxContext = (_UxCAddLocalPrinter *) UxGetContext( UxWidget );

	/* Get values from dialog */

	/* Printer Name */
	XtVaGetValues(UxContext->UxNameText,
		XmNvalue, &printername,
		NULL);
	if (printername[0] == '\0') {
		if (reqargs[0] != '\0')
			strcat(reqargs, ", ");
		strcat(reqargs, catgets(_catd, 8, 34, "Printer Name"));
	}

	/* Printer Type */
	XtVaGetValues(UxContext->UxTypeOptionMenu,
		XmNmenuHistory, &w,
		NULL);
	XtVaGetValues(w,
		XmNuserData, &ptr,
		NULL);
	index = (int) ptr;
	if (index >= 0) {	/* standard type, get matching token */
		type = PrinterTypeTokens[index];
	}
	else {			/* user input type, use label string */
		XtVaGetValues(w,
			XmNlabelString, &xstr,
			NULL);
		XmStringGetLtoR(xstr, XmSTRING_DEFAULT_CHARSET, &type);
	}
	if ((type[0] == '\0') || (strcmp(type, OTHERLABEL) == 0)) {
		if (reqargs[0] != '\0')
			strcat(reqargs, ", ");
		strcat(reqargs, catgets(_catd, 8, 36, "Printer Type"));
	}

	/* Printer Port */
	XtVaGetValues(UxContext->UxPortOptionMenu,
		XmNmenuHistory, &w,
		NULL);
	XtVaGetValues(w,
		XmNlabelString, &xstr,
		NULL);
	XmStringGetLtoR(xstr, XmSTRING_DEFAULT_CHARSET, &port);
	if ((port[0] == '\0') || (strcmp(port, OTHERLABEL) == 0)) {
		if (reqargs[0] != '\0')
			strcat(reqargs, ", ");
		strcat(reqargs, catgets(_catd, 8, 38, "Printer Port"));
	}

	/* Put up error dialog if required fields are missing */
	if (reqargs[0] != '\0') {
		sprintf(errstr, MISSING_REQ_ARGS, reqargs);
		display_error(UxContext->UxAddLocalPrinter, errstr);
		return;
	}

	/* Comment */
	XtVaGetValues(UxContext->UxCommentText,
		XmNvalue, &comment,
		NULL);

	/* File Contents */
	XtVaGetValues(UxContext->UxContentsOptionMenu,
		XmNmenuHistory, &w,
		NULL);
	XtVaGetValues(w,
		XmNuserData, &ptr,
		NULL);
	index = (int) ptr;
	filecontents = FileContentsTokens[index];

	/* Fault Notification */
	XtVaGetValues(UxContext->UxFaultOptionMenu,
		XmNmenuHistory, &w,
		NULL);
	XtVaGetValues(w,
		XmNuserData, &ptr,
		NULL);
	index = (int) ptr;
	faultnotify = FaultTokens[index];

	/* Default Printer */
	defaultprinter = XmToggleButtonGadgetGetState(
				UxContext->UxDefaultTogglebutton);

	/* Print Banner */
	banner = XmToggleButtonGadgetGetState(UxContext->UxBannerTogglebutton);

	/* User Access List */
	XtVaGetValues(UxContext->UxUserList,
		XmNitemCount, &listcount,
		XmNitems, &list,
		NULL);
	if (list && listcount) {
		buflen = 0;
		for (i=0; i<listcount; i++) {
			XmStringGetLtoR(
				list[i], XmSTRING_DEFAULT_CHARSET, &tmp);
			buflen += strlen(tmp) + 1;
			XtFree(tmp);
		}

		userlist = (char*)malloc(buflen + 1);
		userlist[0] = '\0';

		for (i=0; i<listcount; i++) {
			XmStringGetLtoR(
				list[i], XmSTRING_DEFAULT_CHARSET, &tmp);
			if (i > 0) {
				strcat(userlist, ",");
			}
			strcat(userlist, tmp);
			XtFree(tmp);
		}
	}

	/* Add local printer */
	SetBusyPointer(True);

	memset((void *)&printer, 0, sizeof (printer));

	printer.printername = printername;
	printer.printertype = type;
	printer.printserver = NULL;
	printer.file_contents = filecontents;
	printer.comment = comment;
	printer.device = port;
	printer.notify = faultnotify;
	printer.protocol = "bsd";
	printer.num_restarts = 999;
	printer.default_p = defaultprinter;
	printer.banner_req_p = banner;
	printer.enable_p = B_TRUE;
	printer.accept_p = B_TRUE;
	printer.user_allow_list = userlist;

	sts = sysman_add_local_printer(&printer, errbuf, ERRBUF_SIZE);

	if (sts == 0) {
		printer.printserver = localhost;
		add_printer_to_list(&printer);

		if (defaultprinter)
			set_default_printer_msg(printername);

		save_dialog_values(UxContext);

		if (wgt == UxContext->UxOKPushbutton)
			UxPopdownInterface( UxContext->UxAddLocalPrinter );
	}
	else {
		display_error(addlocaldialog, errbuf);
	}

	if (userlist != NULL)
		free(userlist);

	SetBusyPointer(False);
}

static void
ResetPushbutton_activateCB(
	Widget wgt, 
	XtPointer cd, 
	XtPointer cb)
{
	_UxCAddLocalPrinter  *UxSaveCtx, *UxContext;
	Widget                  UxWidget = wgt;
	XtPointer               UxClientData = cd;
	XtPointer               UxCallbackArg = cb;

	UxSaveCtx = UxAddLocalPrinterContext;
	UxAddLocalPrinterContext = UxContext =
			(_UxCAddLocalPrinter *) UxGetContext( UxWidget );
	{
	XtVaSetValues(UxContext->UxNameText,
		XmNvalue, save_printername,
		NULL);
	XtVaSetValues(UxContext->UxCommentText,
		XmNvalue, save_comment,
		NULL);
	XtVaSetValues(UxContext->UxPortOptionMenu,
		XmNmenuHistory, save_port,
		NULL);
	XtVaSetValues(UxContext->UxTypeOptionMenu,
		XmNmenuHistory, save_type,
		NULL);
	XtVaSetValues(UxContext->UxContentsOptionMenu,
		XmNmenuHistory, save_contents,
		NULL);
	XtVaSetValues(UxContext->UxFaultOptionMenu,
		XmNmenuHistory, save_fault,
		NULL);
	XmToggleButtonGadgetSetState(UxContext->UxDefaultTogglebutton,
				save_sysdefault, True);
	XmToggleButtonGadgetSetState(UxContext->UxBannerTogglebutton,
				save_banner, True);
	XtVaSetValues(UxContext->UxUserList,
		XmNitemCount, save_listcount,
		XmNitems, save_list,
		NULL);
	XtVaSetValues(UxContext->UxUserText,
		XmNvalue, save_user,
		NULL);
	}

}

static void
CancelPushbutton_activateCB(
	Widget wgt, 
	XtPointer cd, 
	XtPointer cb)
{
	_UxCAddLocalPrinter  *UxSaveCtx, *UxContext;
	Widget                  UxWidget = wgt;
	XtPointer               UxClientData = cd;
	XtPointer               UxCallbackArg = cb;

	UxSaveCtx = UxAddLocalPrinterContext;
	UxAddLocalPrinterContext = UxContext =
			(_UxCAddLocalPrinter *) UxGetContext( UxWidget );
	{
	UxPopdownInterface( UxContext->UxAddLocalPrinter );
	}

}

static void
TextFocusCB(
	Widget wgt, 
	XtPointer cd, 
	XtPointer cb)
{
	_UxCAddLocalPrinter  *UxSaveCtx, *UxContext;
	Widget                  UxWidget = wgt;
	XtPointer               UxClientData = cd;
	XmAnyCallbackStruct*    cbs = (XmAnyCallbackStruct*)cb;

	UxSaveCtx = UxAddLocalPrinterContext;
	UxAddLocalPrinterContext = UxContext =
			(_UxCAddLocalPrinter *) UxGetContext( UxWidget );
	{

	/* When the text field gets focus, disable the default
	 * pushbutton for the dialog.  This allows the text field's
	 * activate callback to work without dismissing the dialog.
	 * Re-enable default pushbutton when losing focus.
	 */

	if (cbs->reason == XmCR_FOCUS) {
		XtVaSetValues(AddLocalPrinter,
			XmNdefaultButton, NULL,
			NULL);
	}
	else if (cbs->reason == XmCR_LOSING_FOCUS) {
		XtVaSetValues(AddLocalPrinter,
			XmNdefaultButton, OKPushbutton,
			NULL);
	}

	}

}


static void
OptionMenuPushbutton_activateCB(
	Widget wgt, 
	XtPointer cd, 
	XtPointer cbs)
{
	/* record current setting in option menu user data */
	Widget	menu;
	Widget	optionmenu;
	menu = XtParent(wgt);
	XtVaGetValues(menu,
		XmNuserData, &optionmenu,
		NULL);
	XtVaSetValues(optionmenu,
		XmNuserData, wgt,
		NULL);
}

void
PortOtherPushbutton_activateCB(
	Widget wgt, 
	XtPointer cd, 
	XtPointer cbs)
{
	char*		str;
	XmString	xstr;
	Widget		w;
	Widget		menu;
	Widget		optionmenu;
	Widget		dialog;

	menu = XtParent(wgt);
	XtVaGetValues(menu,
		XmNuserData, &optionmenu,
		NULL);
	dialog = get_shell_ancestor(optionmenu);

	if (str = GetPromptInput(dialog,
			catgets(_catd, 8, 40, "Printer Manager: Specify Printer Port"),
			catgets(_catd, 8, 41, "Enter Printer Port"))) {
		if (str[0] == '\0') {
			XtFree(str);
			return;
		}

		/* Destroy "Other" button */
		XtDestroyWidget(wgt);

		/* Add new button */
		xstr = XmStringCreateLocalized(str);
		w = XtVaCreateManagedWidget( "",
			xmPushButtonGadgetClass,
			menu,
			XmNlabelString, xstr,
			NULL );
		XmStringFree(xstr);
		XtFree(str);
		XtVaSetValues(optionmenu,
			XmNmenuHistory, w,
			XmNuserData, w,
			NULL);
		XtAddCallback(w, XmNactivateCallback,
			(XtCallbackProc) OptionMenuPushbutton_activateCB,
			NULL );

		/* Add "Other" button */
		xstr = XmStringCreateLocalized(OTHERLABEL);
		w = XtVaCreateManagedWidget( "",
			xmPushButtonGadgetClass,
			menu,
			XmNlabelString, xstr,
			NULL );
		XmStringFree(xstr);
		XtAddCallback(w, XmNactivateCallback,
			(XtCallbackProc) PortOtherPushbutton_activateCB,
			NULL );

	}
	else {
		/* user canceled, set back to previous value */
		Widget	last;
		XtVaGetValues(optionmenu,
			XmNuserData, &last,
			NULL);
		XtVaSetValues(optionmenu,
			XmNmenuHistory, last,
			NULL);
	}

}

static void
TypeOtherPushbutton_activateCB(
	Widget wgt, 
	XtPointer cd, 
	XtPointer cb)
{
	_UxCAddLocalPrinter  *UxSaveCtx, *UxContext;
	Widget                  UxWidget = wgt;
	XtPointer               UxClientData = cd;
	XtPointer               UxCallbackArg = cb;

	UxSaveCtx = UxAddLocalPrinterContext;
	UxAddLocalPrinterContext = UxContext =
			(_UxCAddLocalPrinter *) UxGetContext( UxWidget );
	{
	char*		str;
	XmString	xstr;
	Widget		tmp;

	if (str = GetPromptInput(addlocaldialog, catgets(_catd, 8, 42, "Printer Manager: Specify Printer Type"),
			catgets(_catd, 8, 43, "Enter Printer Type"))) {
		if (str[0] == '\0') {
			XtFree(str);
			return;
		}

		/* Destroy "Other" button */
		XtDestroyWidget(TypeOtherPushbutton);

		/* Add new button */
		xstr = XmStringCreateLocalized(str);
		tmp = XtVaCreateManagedWidget( "",
				xmPushButtonGadgetClass,
				menu2_p1,
				XmNlabelString, xstr,
				XmNuserData, NONSTDTYPE,
				NULL );
		XmStringFree(xstr);
		XtFree(str);
		XtVaSetValues(TypeOptionMenu,
			XmNmenuHistory, tmp,
			XmNuserData, tmp,
			NULL);
		XtAddCallback(tmp, XmNactivateCallback,
			(XtCallbackProc) OptionMenuPushbutton_activateCB,
			NULL );

		/* Add "Other" button */
		xstr = XmStringCreateLocalized(OTHERLABEL);
		TypeOtherPushbutton = XtVaCreateManagedWidget( "",
				xmPushButtonGadgetClass,
				menu2_p1,
				XmNlabelString, xstr,
				XmNuserData, NONSTDTYPE,
				NULL );
		XmStringFree(xstr);
		UxPutContext( TypeOtherPushbutton, (char *) UxAddLocalPrinterContext );
		XtAddCallback( TypeOtherPushbutton, XmNactivateCallback,
			(XtCallbackProc) TypeOtherPushbutton_activateCB,
			(XtPointer) UxAddLocalPrinterContext );
	}
	else {
		/* user canceled, set back to previous value */
		Widget	last;
		XtVaGetValues(TypeOptionMenu,
			XmNuserData, &last,
			NULL);
		XtVaSetValues(TypeOptionMenu,
			XmNmenuHistory, last,
			NULL);
	}


	}

}


/*******************************************************************************
       The 'build_' function creates all the widgets
       using the resource values specified in the Property Editor.
*******************************************************************************/

static Widget
_Uxbuild_AddLocalPrinter(void)
{
	Widget		_UxParent;
	Widget		menu2_p2_shell;
	Widget		menu3_p1_shell;
	Widget		menu2_p1_shell;
	int		i;
	Widget		w, tmp;
	int		wnum;
	Widget		wlist[10];
	Dimension	width;
	Dimension	maxwidth = 0;

	_UxParent = UxParent;
	if ( _UxParent == NULL )
	{
		_UxParent = GtopLevel;
	}

	_UxParent = XtVaCreatePopupShell( "AddLocalPrinter_shell",
			xmDialogShellWidgetClass, _UxParent,
			XmNx, 366,
			XmNy, 155,
			XmNshellUnitType, XmPIXELS,
			XmNminWidth, 340,
			XmNminHeight, 515,
			NULL );

	AddLocalPrinter = XtVaCreateWidget( "AddLocalPrinter",
			xmFormWidgetClass,
			_UxParent,
			XmNunitType, XmPIXELS,
			XmNautoUnmanage, False,
			XmNhorizontalSpacing, 10,
			XmNverticalSpacing, 5,
			RES_CONVERT( XmNdialogTitle, catgets(_catd, 8, 45, "Admintool: Add Local Printer") ),
			NULL );
	UxPutContext( AddLocalPrinter, (char *) UxAddLocalPrinterContext );

	NameForm = XtVaCreateManagedWidget( "NameForm",
			xmFormWidgetClass,
			AddLocalPrinter,
			XmNfractionBase, 1,
			XmNtopAttachment, XmATTACH_FORM,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			NULL );
	UxPutContext( NameForm, (char *) UxAddLocalPrinterContext );

	NameLabel = XtVaCreateManagedWidget( "NameLabel",
			xmLabelGadgetClass,
			NameForm,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 46, "Printer Name:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_POSITION,
			XmNtopPosition, 0,
			XmNbottomAttachment, XmATTACH_POSITION,
			XmNbottomPosition, 1,
			NULL );
	UxPutContext( NameLabel, (char *) UxAddLocalPrinterContext );

	NameText = XtVaCreateManagedWidget( "NameText",
			xmTextFieldWidgetClass,
			NameForm,
			XmNcolumns, 25,
			XmNmaxLength, MAXPRINTERNAMELEN,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, NameLabel,
			XmNleftOffset, 10,
			XmNrightAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_POSITION,
			XmNtopPosition, 0,
			XmNbottomAttachment, XmATTACH_POSITION,
			XmNbottomPosition, 1,
			NULL );
	UxPutContext( NameText, (char *) UxAddLocalPrinterContext );

	ServerForm = XtVaCreateManagedWidget( "ServerForm",
			xmFormWidgetClass,
			AddLocalPrinter,
			XmNfractionBase, 1,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, NameForm,
			XmNleftAttachment, XmATTACH_FORM,
			NULL );
	UxPutContext( ServerForm, (char *) UxAddLocalPrinterContext );

	ServerLabel = XtVaCreateManagedWidget( "ServerLabel",
			xmLabelGadgetClass,
			ServerForm,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 47, "Print Server:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_POSITION,
			XmNtopPosition, 0,
			XmNbottomAttachment, XmATTACH_POSITION,
			XmNbottomPosition, 1,
			NULL );
	UxPutContext( ServerLabel, (char *) UxAddLocalPrinterContext );

	ServerText = XtVaCreateManagedWidget( "ServerText",
			xmLabelGadgetClass,
			ServerForm,
			XmNalignment, XmALIGNMENT_BEGINNING,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, ServerLabel,
			XmNleftOffset, 10,
			XmNrightAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_POSITION,
			XmNtopPosition, 0,
			XmNbottomAttachment, XmATTACH_POSITION,
			XmNbottomPosition, 1,
			NULL );
	UxPutContext( ServerText, (char *) UxAddLocalPrinterContext );

	CommentForm = XtVaCreateManagedWidget( "CommentForm",
			xmFormWidgetClass,
			AddLocalPrinter,
			XmNfractionBase, 1,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, ServerForm,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			NULL );
	UxPutContext( CommentForm, (char *) UxAddLocalPrinterContext );

	CommentLabel = XtVaCreateManagedWidget( "CommentLabel",
			xmLabelGadgetClass,
			CommentForm,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 48, "Description:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_POSITION,
			XmNtopPosition, 0,
			XmNbottomAttachment, XmATTACH_POSITION,
			XmNbottomPosition, 1,
			NULL );
	UxPutContext( CommentLabel, (char *) UxAddLocalPrinterContext );

	CommentText = XtVaCreateManagedWidget( "CommentText",
			xmTextFieldWidgetClass,
			CommentForm,
			XmNcolumns, 25,
			XmNmaxLength, MAXCOMMENTLEN,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, CommentLabel,
			XmNleftOffset, 10,
			XmNrightAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_POSITION,
			XmNtopPosition, 0,
			XmNbottomAttachment, XmATTACH_POSITION,
			XmNbottomPosition, 1,
			NULL );
	UxPutContext( CommentText, (char *) UxAddLocalPrinterContext );

	PortForm = XtVaCreateManagedWidget( "PortForm",
			xmFormWidgetClass,
			AddLocalPrinter,
			XmNfractionBase, 1,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, CommentForm,
			XmNleftAttachment, XmATTACH_FORM,
			NULL );
	UxPutContext( PortForm, (char *) UxAddLocalPrinterContext );

	PortLabel = XtVaCreateManagedWidget( "PortLabel",
			xmLabelGadgetClass,
			PortForm,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 49, "Printer Port:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_POSITION,
			XmNtopPosition, 0,
			XmNbottomAttachment, XmATTACH_POSITION,
			XmNbottomPosition, 1,
			NULL );
	UxPutContext( PortLabel, (char *) UxAddLocalPrinterContext );

	menu4_p1_shell = XtVaCreatePopupShell ("menu4_p1_shell",
			xmMenuShellWidgetClass, PortForm,
			XmNwidth, 1,
			XmNheight, 1,
			XmNallowShellResize, TRUE,
			XmNoverrideRedirect, TRUE,
			NULL );
	UxPutContext( menu4_p1_shell, (char *) UxAddLocalPrinterContext );

	menu4_p1 = XtVaCreateWidget( "menu4_p1",
			xmRowColumnWidgetClass,
			menu4_p1_shell,
			XmNrowColumnType, XmMENU_PULLDOWN,
			NULL );
	UxPutContext( menu4_p1, (char *) UxAddLocalPrinterContext );

	PortOptionMenu = XtVaCreateManagedWidget( "PortOptionMenu",
			xmRowColumnWidgetClass,
			PortForm,
			XmNrowColumnType, XmMENU_OPTION,
			XmNsubMenuId, menu4_p1,
			RES_CONVERT( XmNlabelString, "" ),
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, PortLabel,
			XmNleftOffset, 0,
			NULL );
	UxPutContext( PortOptionMenu, (char *) UxAddLocalPrinterContext );

	TypeForm = XtVaCreateManagedWidget( "TypeForm",
			xmFormWidgetClass,
			AddLocalPrinter,
			XmNfractionBase, 1,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, PortForm,
			XmNleftAttachment, XmATTACH_FORM,
			NULL );
	UxPutContext( TypeForm, (char *) UxAddLocalPrinterContext );

	TypeLabel = XtVaCreateManagedWidget( "TypeLabel",
			xmLabelGadgetClass,
			TypeForm,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 50, "Printer Type:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_POSITION,
			XmNtopPosition, 0,
			XmNbottomAttachment, XmATTACH_POSITION,
			XmNbottomPosition, 1,
			NULL );
	UxPutContext( TypeLabel, (char *) UxAddLocalPrinterContext );

	menu2_p1_shell = XtVaCreatePopupShell ("menu2_p1_shell",
			xmMenuShellWidgetClass, TypeForm,
			XmNwidth, 1,
			XmNheight, 1,
			XmNallowShellResize, TRUE,
			XmNoverrideRedirect, TRUE,
			NULL );

	menu2_p1 = XtVaCreateWidget( "menu2_p1",
			xmRowColumnWidgetClass,
			menu2_p1_shell,
			XmNrowColumnType, XmMENU_PULLDOWN,
			NULL );
	UxPutContext( menu2_p1, (char *) UxAddLocalPrinterContext );

	i = 0;
	for (i=0; PrinterTypeLabels[i] != NULL; i++) {
		w = XtVaCreateManagedWidget( PrinterTypeLabels[i],
			xmPushButtonGadgetClass,
			menu2_p1,
			RES_CONVERT( XmNlabelString, PrinterTypeLabels[i] ),
			XmNuserData, i,
			NULL );
		UxPutContext( w, (char *) UxAddLocalPrinterContext );
		XtAddCallback( w, XmNactivateCallback,
			(XtCallbackProc) OptionMenuPushbutton_activateCB,
			(XtPointer) UxAddLocalPrinterContext );
	}

	TypeOtherPushbutton = XtVaCreateManagedWidget( "TypeOtherPushbutton",
			xmPushButtonGadgetClass,
			menu2_p1,
			RES_CONVERT( XmNlabelString, OTHERLABEL ),
			XmNuserData, NONSTDTYPE,
			NULL );
	UxPutContext( TypeOtherPushbutton, (char *) UxAddLocalPrinterContext );

	TypeOptionMenu = XtVaCreateManagedWidget( "TypeOptionMenu",
			xmRowColumnWidgetClass,
			TypeForm,
			XmNrowColumnType, XmMENU_OPTION,
			XmNsubMenuId, menu2_p1,
			RES_CONVERT( XmNlabelString, "" ),
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, TypeLabel,
			XmNleftOffset, 0,
			NULL );
	UxPutContext( TypeOptionMenu, (char *) UxAddLocalPrinterContext );

	/* Link menu to optionmenu via userdata */
	XtVaSetValues(menu2_p1,
		XmNuserData, TypeOptionMenu,
		NULL);

	ContentsForm = XtVaCreateManagedWidget( "ContentsForm",
			xmFormWidgetClass,
			AddLocalPrinter,
			XmNfractionBase, 1,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, TypeForm,
			XmNleftAttachment, XmATTACH_FORM,
			NULL );
	UxPutContext( ContentsForm, (char *) UxAddLocalPrinterContext );

	ContentsLabel = XtVaCreateManagedWidget( "ContentsLabel",
			xmLabelWidgetClass,
			ContentsForm,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 51, "File Contents:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_POSITION,
			XmNtopPosition, 0,
			XmNbottomAttachment, XmATTACH_POSITION,
			XmNbottomPosition, 1,
			NULL );
	UxPutContext( ContentsLabel, (char *) UxAddLocalPrinterContext );

	menu2_p2_shell = XtVaCreatePopupShell ("menu2_p2_shell",
			xmMenuShellWidgetClass, ContentsForm,
			XmNwidth, 1,
			XmNheight, 1,
			XmNallowShellResize, TRUE,
			XmNoverrideRedirect, TRUE,
			NULL );

	menu2_p2 = XtVaCreateWidget( "menu2_p2",
			xmRowColumnWidgetClass,
			menu2_p2_shell,
			XmNrowColumnType, XmMENU_PULLDOWN,
			NULL );
	UxPutContext( menu2_p2, (char *) UxAddLocalPrinterContext );

	i = 0;
	for (i=0; FileContentsLabels[i] != NULL; i++) {
		w = XtVaCreateManagedWidget( FileContentsLabels[i],
			xmPushButtonGadgetClass,
			menu2_p2,
			RES_CONVERT( XmNlabelString, FileContentsLabels[i] ),
			XmNuserData, i,
			NULL );
		UxPutContext( w, (char *) UxAddLocalPrinterContext );
	}

	ContentsOptionMenu = XtVaCreateManagedWidget( "ContentsOptionMenu",
			xmRowColumnWidgetClass,
			ContentsForm,
			XmNrowColumnType, XmMENU_OPTION,
			XmNsubMenuId, menu2_p2,
			RES_CONVERT( XmNlabelString, "" ),
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, ContentsLabel,
			XmNleftOffset, 0,
			NULL );
	UxPutContext( ContentsOptionMenu, (char *) UxAddLocalPrinterContext );

	FaultForm = XtVaCreateManagedWidget( "FaultForm",
			xmFormWidgetClass,
			AddLocalPrinter,
			XmNfractionBase, 1,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, ContentsForm,
			XmNleftAttachment, XmATTACH_FORM,
			NULL );
	UxPutContext( FaultForm, (char *) UxAddLocalPrinterContext );

	FaultLabel = XtVaCreateManagedWidget( "FaultLabel",
			xmLabelGadgetClass,
			FaultForm,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 52, "Fault Notification:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_POSITION,
			XmNtopPosition, 0,
			XmNbottomAttachment, XmATTACH_POSITION,
			XmNbottomPosition, 1,
			NULL );
	UxPutContext( FaultLabel, (char *) UxAddLocalPrinterContext );

	menu3_p1_shell = XtVaCreatePopupShell ("menu3_p1_shell",
			xmMenuShellWidgetClass, FaultForm,
			XmNwidth, 1,
			XmNheight, 1,
			XmNallowShellResize, TRUE,
			XmNoverrideRedirect, TRUE,
			NULL );

	menu3_p1 = XtVaCreateWidget( "menu3_p1",
			xmRowColumnWidgetClass,
			menu3_p1_shell,
			XmNrowColumnType, XmMENU_PULLDOWN,
			NULL );
	UxPutContext( menu3_p1, (char *) UxAddLocalPrinterContext );

	WritePushbutton = XtVaCreateManagedWidget( "WritePushbutton",
			xmPushButtonGadgetClass,
			menu3_p1,
			RES_CONVERT( XmNlabelString, FaultLabels[0] ),
			XmNuserData, 0,
			NULL );
	UxPutContext( WritePushbutton, (char *) UxAddLocalPrinterContext );

	MailPushbutton = XtVaCreateManagedWidget( "MailPushbutton",
			xmPushButtonGadgetClass,
			menu3_p1,
			RES_CONVERT( XmNlabelString, FaultLabels[1] ),
			XmNuserData, 1,
			NULL );
	UxPutContext( MailPushbutton, (char *) UxAddLocalPrinterContext );

	NonePushbutton = XtVaCreateManagedWidget( "NonePushbutton",
			xmPushButtonGadgetClass,
			menu3_p1,
			RES_CONVERT( XmNlabelString, FaultLabels[2] ),
			XmNuserData, 2,
			NULL );
	UxPutContext( NonePushbutton, (char *) UxAddLocalPrinterContext );

	FaultOptionMenu = XtVaCreateManagedWidget( "FaultOptionMenu",
			xmRowColumnWidgetClass,
			FaultForm,
			XmNrowColumnType, XmMENU_OPTION,
			XmNsubMenuId, menu3_p1,
			RES_CONVERT( XmNlabelString, "" ),
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, FaultLabel,
			XmNleftOffset, 0,
			NULL );
	UxPutContext( FaultOptionMenu, (char *) UxAddLocalPrinterContext );

	OptionsForm = XtVaCreateManagedWidget( "OptionsForm",
			xmFormWidgetClass,
			AddLocalPrinter,
			XmNfractionBase, 1,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, FaultForm,
			XmNleftAttachment, XmATTACH_FORM,
			NULL );
	UxPutContext( OptionsForm, (char *) UxAddLocalPrinterContext );

	OptionsLabel = XtVaCreateManagedWidget( "OptionsLabel",
			xmLabelGadgetClass,
			OptionsForm,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 53, "Options:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_FORM,
			XmNtopOffset, 3,
			NULL );
	UxPutContext( OptionsLabel, (char *) UxAddLocalPrinterContext );

	DefaultTogglebutton = XtVaCreateManagedWidget( "DefaultTogglebutton",
			xmToggleButtonGadgetClass,
			OptionsForm,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 54, "Default Printer") ),
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, OptionsLabel,
			XmNleftOffset, 7,
			XmNtopAttachment, XmATTACH_FORM,
			NULL );
	UxPutContext( DefaultTogglebutton, (char *) UxAddLocalPrinterContext );

	BannerTogglebutton = XtVaCreateManagedWidget( "BannerTogglebutton",
			xmToggleButtonGadgetClass,
			OptionsForm,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 55, "Always Print Banner") ),
			XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNleftWidget, DefaultTogglebutton,
			XmNleftOffset, 0,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, DefaultTogglebutton,
			XmNtopOffset, 0,
			NULL );
	UxPutContext( BannerTogglebutton, (char *) UxAddLocalPrinterContext );

	UserListForm = XtVaCreateManagedWidget( "UserListForm",
			xmFormWidgetClass,
			AddLocalPrinter,
			XmNfractionBase, 1,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, OptionsForm,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			NULL );
	UxPutContext( UserListForm, (char *) UxAddLocalPrinterContext );

	UserListLabel = XtVaCreateManagedWidget( "UserListLabel",
			xmLabelGadgetClass,
			UserListForm,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 56, "User Access List:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_FORM,
			XmNtopOffset, 0,
			NULL );
	UxPutContext( UserListLabel, (char *) UxAddLocalPrinterContext );

	scrolledWindow2 = XtVaCreateManagedWidget( "scrolledWindow2",
			xmScrolledWindowWidgetClass,
			UserListForm,
			XmNscrollingPolicy, XmAPPLICATION_DEFINED,
			XmNvisualPolicy, XmVARIABLE,
			XmNscrollBarDisplayPolicy, XmSTATIC,
			XmNshadowThickness, 0,
			XmNtopAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, UserListLabel,
			XmNleftOffset, 10,
			NULL );
	UxPutContext( scrolledWindow2, (char *) UxAddLocalPrinterContext );

	UserList = XtVaCreateManagedWidget( "UserList",
			xmListWidgetClass,
			scrolledWindow2,
			XmNvisibleItemCount, 3,
			XmNselectionPolicy, XmEXTENDED_SELECT,
			NULL );
	UxPutContext( UserList, (char *) UxAddLocalPrinterContext );

	UserText = XtVaCreateManagedWidget( "UserText",
			xmTextFieldWidgetClass,
			UserListForm,
			XmNmaxLength, MAXUSERNAMELEN,
			XmNrightAttachment, XmATTACH_FORM,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, UserListLabel,
			XmNleftOffset, 10,
			NULL );
	UxPutContext( UserText, (char *) UxAddLocalPrinterContext );

	XtVaSetValues(scrolledWindow2,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, UserText,
		XmNbottomOffset, 0,
		NULL);

	tmp = XtVaCreateManagedWidget( "",
			xmFormWidgetClass,
			UserListForm,
			XmNfractionBase, 7,
			XmNrightAttachment, XmATTACH_FORM,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, UserListLabel,
			XmNleftOffset, 10,
			XmNbottomAttachment, XmATTACH_FORM,
			NULL );

	XtVaSetValues(UserText,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, tmp,
		XmNuserData, UserList,
		NULL);

	AddPushbutton = XtVaCreateManagedWidget( "AddPushbutton",
			xmPushButtonGadgetClass,
			tmp,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 57, "Add") ),
			XmNleftAttachment, XmATTACH_POSITION,
			XmNleftPosition, 1,
			XmNrightAttachment, XmATTACH_POSITION,
			XmNrightPosition, 3,
			NULL );
	UxPutContext( AddPushbutton, (char *) UxAddLocalPrinterContext );

	DeletePushbutton = XtVaCreateManagedWidget( "DeletePushbutton",
			xmPushButtonGadgetClass,
			tmp,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 58, "Delete") ),
			XmNleftAttachment, XmATTACH_POSITION,
			XmNleftPosition, 4,
			XmNrightAttachment, XmATTACH_POSITION,
			XmNrightPosition, 6,
			NULL );
	UxPutContext( DeletePushbutton, (char *) UxAddLocalPrinterContext );

	tmp = create_button_box(AddLocalPrinter, NULL,
		UxAddLocalPrinterContext,
		&OKPushbutton, &ApplyPushbutton, &ResetPushbutton,
		&CancelPushbutton, &HelpPushbutton);

	XtVaSetValues(tmp,
		XmNtopAttachment, XmATTACH_NONE,
		NULL);

	XtVaSetValues(UserListForm,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, tmp,
		XmNbottomOffset, 10,
		NULL);

	XtVaSetValues(AddLocalPrinter,
		XmNinitialFocus, NameText,
		NULL);

	/* Align all labels to right edge of longest label */
	wnum = 9;
	wlist[0] = NameLabel;
	wlist[1] = ServerLabel;
	wlist[2] = CommentLabel;
	wlist[3] = PortLabel;
	wlist[4] = TypeLabel;
	wlist[5] = ContentsLabel;
	wlist[6] = FaultLabel;
	wlist[7] = OptionsLabel;
	wlist[8] = UserListLabel;
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


	XtAddCallback( UserText, XmNfocusCallback,
		(XtCallbackProc) TextFocusCB,
		(XtPointer) UxAddLocalPrinterContext );

	XtAddCallback( UserText, XmNlosingFocusCallback,
		(XtCallbackProc) TextFocusCB,
		(XtPointer) UxAddLocalPrinterContext );

	XtAddCallback( UserText, XmNactivateCallback,
		(XtCallbackProc) addPrinterUserListEntryCB,
		(XtPointer) UserText );

	XtAddCallback( AddPushbutton, XmNactivateCallback,
		(XtCallbackProc) addPrinterUserListEntryCB,
		(XtPointer) UserText );

	XtAddCallback( DeletePushbutton, XmNactivateCallback,
		(XtCallbackProc) deletePrinterUserListEntryCB,
		(XtPointer) UserList );

	XtAddCallback( OKPushbutton, XmNactivateCallback,
		(XtCallbackProc) OKPushbutton_activateCB,
		(XtPointer) UxAddLocalPrinterContext );

	XtAddCallback( ApplyPushbutton, XmNactivateCallback,
		(XtCallbackProc) OKPushbutton_activateCB,
		(XtPointer) UxAddLocalPrinterContext );

	XtAddCallback( ResetPushbutton, XmNactivateCallback,
		(XtCallbackProc) ResetPushbutton_activateCB,
		(XtPointer) UxAddLocalPrinterContext );

	XtAddCallback( CancelPushbutton, XmNactivateCallback,
		(XtCallbackProc) CancelPushbutton_activateCB,
		(XtPointer) UxAddLocalPrinterContext );

	XtAddCallback( HelpPushbutton, XmNactivateCallback,
		(XtCallbackProc) helpCB,
		"printer_window.r.hlp" );

	XtAddCallback( TypeOtherPushbutton, XmNactivateCallback,
		(XtCallbackProc) TypeOtherPushbutton_activateCB,
		(XtPointer) UxAddLocalPrinterContext );


	XtAddCallback( AddLocalPrinter, XmNdestroyCallback,
		(XtCallbackProc) UxDestroyContextCB,
		(XtPointer) UxAddLocalPrinterContext);


	return ( AddLocalPrinter );
}

/*******************************************************************************
       The following is the 'Interface function' which is the
       external entry point for creating this interface.
       This function should be called from your application or from
       a callback function.
*******************************************************************************/

Widget
create_AddLocalPrinter(Widget _UxUxParent)
{
	Widget                  rtrn;
	_UxCAddLocalPrinter     *UxContext;

	UxAddLocalPrinterContext = UxContext =
		(_UxCAddLocalPrinter *) UxNewContext( sizeof(_UxCAddLocalPrinter), False );

	UxParent = _UxUxParent;

	rtrn = _Uxbuild_AddLocalPrinter();

	return(rtrn);
}

/*******************************************************************************
       END OF FILE
*******************************************************************************/

void
show_addlocaldialog(Widget parent, sysMgrMainCtxt * ctxt)
{
	_UxCAddLocalPrinter  *UxContext;
	XmString	xstr;
	char**		s;
	int		k;
	Widget		w;
	Widget		submenu;
	char*		label = NULL;
	char*		printer_type = "PS";
	char*		file_contents = "postscript";
	char*		port = "/dev/term/a";
	char**		portlist;
	int		numports;


	if (addlocaldialog && XtIsManaged(addlocaldialog)) {
		XtPopup(XtParent(addlocaldialog), XtGrabNone);
		return;
	}

	SetBusyPointer(True);

	if (addlocaldialog == NULL)
		addlocaldialog = create_AddLocalPrinter(parent);
	
	ctxt->currDialog = addlocaldialog;
	UxContext = (_UxCAddLocalPrinter *) UxGetContext(addlocaldialog);

	/* printer */
	XtVaSetValues(UxContext->UxNameText,
		XmNvalue, "",
		NULL);

	/* server */
	xstr = XmStringCreateLocalized(localhost);
	XtVaSetValues(UxContext->UxServerText,
		XmNlabelString, xstr,
		NULL);
	XmStringFree(xstr);

	/* comment */
	XtVaSetValues(UxContext->UxCommentText,
		XmNvalue, "",
		NULL);

	/* printer type */
	for (s=PrinterTypeTokens,k=0; *s; s++,k++) {
		if (strcmp(printer_type, *s) == 0) {
			label = PrinterTypeLabels[k];
			break;
		}
	}
	if (label != NULL) {
		XtVaGetValues(TypeOptionMenu,
			XmNsubMenuId, &submenu,
			NULL);
		XtVaSetValues(UxContext->UxTypeOptionMenu,
			XmNmenuHistory, XtNameToWidget(submenu, label),
			XmNuserData, XtNameToWidget(submenu, label),
			NULL);
	}

	/* file contents */
	label = NULL;
	for (s=FileContentsTokens,k=0; *s; s++,k++) {
		if (strcmp(file_contents, *s) == 0) {
			label = FileContentsLabels[k];
			break;
		}
	}
	if (label != NULL) {
		XtVaGetValues(ContentsOptionMenu,
			XmNsubMenuId, &submenu,
			NULL);
		XtVaSetValues(ContentsOptionMenu,
			XmNmenuHistory, XtNameToWidget(submenu, label),
			NULL);
	}

	/* fault notification */
	XtVaSetValues(UxContext->UxFaultOptionMenu,
		XmNmenuHistory, UxContext->UxWritePushbutton,
		NULL);

	/* System Default */
	XmToggleButtonGadgetSetState(UxContext->UxDefaultTogglebutton,
				False, True);

	/* Print Banner */
	XmToggleButtonGadgetSetState(UxContext->UxBannerTogglebutton,
				False, True);

	/* User Access List */
	XmListDeleteAllItems(UxContext->UxUserList);
	xstr = XmStringCreateLocalized(ALL_USERS);
	XmListAddItemUnselected(UxContext->UxUserList, xstr, 1);
	XmStringFree(xstr);
	XtVaSetValues(UxContext->UxUserText,
		XmNvalue, "",
		NULL);

	/* Printer Port */
	numports = sysman_list_printer_devices(&portlist, errbuf, ERRBUF_SIZE);
	if (numports >= 0) {
		build_port_optionmenu(numports, portlist, port, PortOptionMenu);
		sysman_free_printer_devices_list(portlist, numports);
	}
	else {
		display_error(addlocaldialog, errbuf);
	}

	save_dialog_values(UxContext);

	UxPopupInterface(addlocaldialog, no_grab);
	SetBusyPointer(False);
}

static void
save_dialog_values(_UxCAddLocalPrinter * UxContext)
{
	int	i;
	XmStringTable	tmp_list;

	if (save_printername) XtFree(save_printername);
	if (save_comment) XtFree(save_comment);
	if (save_user) XtFree(save_user);
	if (save_list) {
		for (i=0; i<save_listcount; i++)
			XmStringFree(save_list[i]);
		XtFree((char *) save_list);
		save_listcount = 0;
	}

	save_printername = XmTextFieldGetString(UxContext->UxNameText);
	save_comment = XmTextFieldGetString(UxContext->UxCommentText);
	XtVaGetValues(UxContext->UxPortOptionMenu,
		XmNmenuHistory, &save_port,
		NULL);
	XtVaGetValues(UxContext->UxTypeOptionMenu,
		XmNmenuHistory, &save_type,
		NULL);
	XtVaGetValues(UxContext->UxContentsOptionMenu,
		XmNmenuHistory, &save_contents,
		NULL);
	XtVaGetValues(UxContext->UxFaultOptionMenu,
		XmNmenuHistory, &save_fault,
		NULL);
	save_sysdefault = XmToggleButtonGadgetGetState(UxContext->UxDefaultTogglebutton);
	save_banner = XmToggleButtonGadgetGetState(UxContext->UxBannerTogglebutton);
	XtVaGetValues(UxContext->UxUserList,
		XmNitemCount, &save_listcount,
		XmNitems, &tmp_list,
		NULL);
	save_list = (XmString *) XtMalloc(save_listcount * sizeof(XmString *));
	for (i=0; i<save_listcount; i++)
		save_list[i] = XmStringCopy(tmp_list[i]);
	save_user = XmTextFieldGetString(UxContext->UxUserText);
}


void
build_port_optionmenu(
	int	numports,
	char**	ports,
	char*	currentport,
	Widget	optionmenu
)
{
	int		numchild;
	WidgetList	wlist;
	int		i;
	XmString	xstr;
	Widget		first = NULL;
	Widget		setport = NULL;
	Widget		w;
	Widget		menu;
	Widget		dflt;
	const char*	tmp;


	/* Clear existing port menu items */
	XtVaGetValues(optionmenu,
		XmNsubMenuId, &menu,
		NULL);
	XtVaGetValues(menu,
		XtNnumChildren, &numchild,
		XtNchildren, &wlist,
		NULL);
	for (i=0; i<numchild; i++)
		XtDestroyWidget(wlist[i]);

	/* Link menu to optionmenu via userdata */
	XtVaSetValues(menu,
		XmNuserData, optionmenu,
		NULL);

	/* Add port buttons */
	setport = NULL;
	if (ports) {
		for (i=0; i<numports; i++) {
			tmp = ports[i];
			xstr = XmStringCreateLocalized((char*)tmp);
			w = XtVaCreateManagedWidget( "",
				xmPushButtonGadgetClass,
				menu,
				XmNlabelString, xstr,
				NULL );
			XmStringFree(xstr);
			XtAddCallback(w, XmNactivateCallback,
				(XtCallbackProc) OptionMenuPushbutton_activateCB,
				NULL);
			if (!first)
				first = w;
			if (currentport && (strcmp(tmp, currentport) == 0))
				setport = w;
		}
	}

	/*  If printer has a defined port which isn't found in the list
	 *  of devices on the host, go ahead and create a button so it
	 *  can be displayed.
	 */
	if (currentport && !setport) {
		xstr = XmStringCreateLocalized((char*)currentport);
		w = XtVaCreateManagedWidget( "",
			xmPushButtonGadgetClass,
			menu,
			XmNlabelString, xstr,
			NULL );
		XmStringFree(xstr);
		XtAddCallback(w, XmNactivateCallback,
			(XtCallbackProc) OptionMenuPushbutton_activateCB,
			NULL);
		setport = w;
	}

	xstr = XmStringCreateLocalized(OTHERLABEL);
	w = XtVaCreateManagedWidget( "",
			xmPushButtonGadgetClass,
			menu,
			XmNlabelString, xstr,
			NULL );
	XmStringFree(xstr);

	XtAddCallback(w, XmNactivateCallback,
		(XtCallbackProc) PortOtherPushbutton_activateCB,
		NULL);

	/* Set option menu current setting */
	dflt = (setport ? setport : (first ? first : NULL));
	XtVaSetValues(optionmenu,
		XmNmenuHistory, dflt,
		XmNuserData, dflt,
		NULL);
}

