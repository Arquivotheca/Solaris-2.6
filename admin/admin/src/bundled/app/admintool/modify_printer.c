
/* Copyright 1993 Sun Microsystems, Inc. */

#pragma ident "@(#)modify_printer.c	1.18 96/06/25 Sun Microsystems"

/*	modify_printer.c	*/

#include <stdlib.h>
#include <ctype.h>
#include <nl_types.h>

#include "util.h"
#include "sysman_iface.h"

#define CONTEXT_MACRO_ACCESS 1
#include "modify_printer.h"
#undef CONTEXT_MACRO_ACCESS

#define UNKNOWNTYPE	 catgets(_catd, 8, 215, "unknown")

#define	FILE_CONTENTS_PS	"postscript"
#define	FILE_CONTENTS_SIMPLE	"simple"
#define	FILE_CONTENTS_BOTH	FILE_CONTENTS_PS","FILE_CONTENTS_SIMPLE
#define	FILE_CONTENTS_NONE	""
#define	FILE_CONTENTS_ANY	"any"

#define	FAULT_MAIL		"mail"
#define	FAULT_WRITE		"write"
#define	FAULT_NONE		"none"

static void save_dialog_values(_UxCModifyPrinter * UxContext);
static const char * _strip_whitespace(const char * orig);

void	build_port_optionmenu(
	int numports, char** ports, char* currentport, Widget optionmenu);
extern void PortOtherPushbutton_activateCB(
	Widget wgt, XtPointer cd, XtPointer cb);

extern nl_catd	_catd;	/* for catgets(), defined in main.c */

extern char * PrinterTypeLabels[];
extern char * PrinterTypeTokens[];
extern char * FileContentsLabels[];
extern char * FileContentsTokens[];
extern char * FaultLabels[];
extern char * FaultTokens[];

static char *		save_comment;
static Widget		save_port;
static Widget		save_contents;
static Widget		save_fault;
static char*		save_protocol;
static Boolean		save_sysdefault;
static Boolean		save_banner;
static Boolean		save_nis;
static Boolean		save_enable;
static Boolean		save_accept;
static char *		save_user;
static int		save_listcount;
static XmStringTable	save_list;

Widget	modifyprinterdialog;


/*******************************************************************************
       The following are callback functions.
*******************************************************************************/
static void
OKPushbutton_activateCB(
	Widget		wgt, 
	XtPointer	cd, 
	XtPointer	cb
)
{
	_UxCModifyPrinter*	UxContext;
	Widget                  UxWidget = wgt;
	XtPointer               UxClientData = cd;
	XtPointer               UxCallbackArg = cb;
	char *		printername;
	char *		server;
	char *		comment;
	char *		port;
	char *		filecontents;
	char *		faultnotify;
	Boolean		defaultprinter;
	Boolean		banner;
	Boolean		enablequeue;
	Boolean		acceptjobs;
	char *		userlist = NULL;
	int		buflen;
	int		listcount;
	XmStringTable	list;
	char *		tmp;
	XmString	xstr;
	XtPointer	ptr;
	int		index;
	int		i;
	Widget		w;
	char*		def_printer;
	SysmanPrinterArg printer;
	int		sts;
	char		errstr[512] = "";
	char		reqargs[64] = "";

	UxContext = (_UxCModifyPrinter*) UxGetContext( UxWidget );


	/* Get common values from dialog */

	/* Printer Name */
	XtVaGetValues(UxContext->UxNameText,
		XmNlabelString, &xstr,
		NULL);
	XmStringGetLtoR(xstr, XmSTRING_DEFAULT_CHARSET, &printername);
	XmStringFree(xstr);

	/* Server */
	XtVaGetValues(UxContext->UxServerText,
		XmNlabelString, &xstr,
		NULL);
	XmStringGetLtoR(xstr, XmSTRING_DEFAULT_CHARSET, &server);
	XmStringFree(xstr);

	/* Comment */
	XtVaGetValues(UxContext->UxCommentText,
		XmNvalue, &comment,
		NULL);

	/* Default Printer */
	defaultprinter = XmToggleButtonGadgetGetState(
		UxContext->UxDefaultTogglebutton);

	/* Local Printer specific parameters */
	if (strcmp(server, localhost) == 0) {
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
			strcat(reqargs, catgets(_catd, 8, 217, "Printer Port"));
		}

		/* File Contents */
		XtVaGetValues(UxContext->UxContentsOptionMenu,
			XmNmenuHistory, &w,
			NULL);
		XtVaGetValues(w,
			XmNuserData, &ptr,
			NULL);
		index = (int) ptr;
		filecontents = FileContentsTokens[index];

		/* Put up error dialog if required fields are missing */
		if (reqargs[0] != '\0') {
			sprintf(errstr, MISSING_REQ_ARGS, reqargs);
			display_error(UxContext->UxModifyPrinter, errstr);
			return;
		}
	
		/* Fault Notification */
		XtVaGetValues(UxContext->UxFaultOptionMenu,
			XmNmenuHistory, &w,
			NULL);
		XtVaGetValues(w,
			XmNuserData, &ptr,
			NULL);
		index = (int) ptr;
		faultnotify = FaultTokens[index];
	
		/* Print Banner */
		banner = XmToggleButtonGadgetGetState(
			UxContext->UxBannerTogglebutton);
	
		/* Enable Print Queue */
		enablequeue = XmToggleButtonGadgetGetState(
			UxContext->UxEnableQueueTogglebutton);

		/* Accept Jobs */
		acceptjobs = XmToggleButtonGadgetGetState(
			UxContext->UxAcceptJobsTogglebutton);

		/* User Access List */
		XtVaGetValues(UxContext->UxUserList,
			XmNitemCount, &listcount,
			XmNitems, &list,
			NULL);
		if (list && listcount) {
			buflen = 0;
			for (i=0; i<listcount; i++) {
				XmStringGetLtoR(list[i],
					XmSTRING_DEFAULT_CHARSET, &tmp);
				buflen += strlen(tmp) + 1;
				XtFree(tmp);
			}
	
			userlist = (char*)malloc(buflen + 1);
			userlist[0] = '\0';
	
			for (i=0; i<listcount; i++) {
				XmStringGetLtoR(list[i],
					XmSTRING_DEFAULT_CHARSET, &tmp);
				if (i > 0) {
					strcat(userlist, ",");
				}
				strcat(userlist, tmp);
				XtFree(tmp);
			}
		}
	}

	/* modify printer */
	SetBusyPointer(True);

	copy_printer(&printer, &UxContext->printer);

	printer.printername = printername;
	printer.printserver = NULL;
	printer.comment = comment;
	printer.protocol = NULL;	/* always "bsd", never change it */
	printer.num_restarts = 0;
	printer.default_p = defaultprinter;

	if (strcmp(server, localhost) == 0) {
		/* local printer */

		printer.device = port;
		printer.notify = faultnotify;
		printer.printertype = NULL;
		printer.file_contents = filecontents;
		printer.banner_req_p = banner;
		printer.enable_p = enablequeue;
		printer.accept_p = acceptjobs;
		printer.user_allow_list = userlist;

		sts = sysman_modify_local_printer(
			&printer, errbuf, ERRBUF_SIZE);
	}
	else {
		/* remote printer */

		sts = sysman_modify_remote_printer(
			&printer, errbuf, ERRBUF_SIZE);
	}

	if (sts == 0) {
		printer.printserver = server;
		update_entry(&printer);

		sysman_get_default_printer_name(&def_printer,
			errbuf, ERRBUF_SIZE);
		set_default_printer_msg(def_printer);
		if (def_printer)
			free(def_printer);

		free_printer(&UxContext->printer);
		copy_printer(&UxContext->printer, &printer);
		save_dialog_values(UxContext);

		if (wgt == UxContext->UxOKPushbutton) {
			UxPopdownInterface( UxContext->UxModifyPrinter );
		}

	}
	else {
		display_error(modifyprinterdialog, errbuf);
		SetBusyPointer(False);
		return;
	}

	SetBusyPointer(False);

}

static	void	ResetPushbutton_activateCB(
			Widget wgt, 
			XtPointer cd, 
			XtPointer cb)
{
	_UxCModifyPrinter  *UxSaveCtx, *UxContext;
	Widget                  UxWidget = wgt;
	XtPointer               UxClientData = cd;
	XtPointer               UxCallbackArg = cb;

	UxSaveCtx = UxModifyPrinterContext;
	UxModifyPrinterContext = UxContext =
			(_UxCModifyPrinter *) UxGetContext( UxWidget );
	{
	XtVaSetValues(UxContext->UxCommentText,
		XmNvalue, save_comment,
		NULL);
	XtVaSetValues(UxContext->UxPortOptionMenu,
		XmNmenuHistory, save_port,
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
	XmToggleButtonGadgetSetState(UxContext->UxEnableQueueTogglebutton,
				save_enable, True);
	XmToggleButtonGadgetSetState(UxContext->UxAcceptJobsTogglebutton,
				save_accept, True);
	XtVaSetValues(UxContext->UxUserList,
		XmNitemCount, save_listcount,
		XmNitems, save_list,
		NULL);
	XtVaSetValues(UxContext->UxUserText,
		XmNvalue, save_user,
		NULL);
	}

}

static	void	CancelPushbutton_activateCB(
			Widget wgt, 
			XtPointer cd, 
			XtPointer cb)
{
	_UxCModifyPrinter  *UxSaveCtx, *UxContext;
	Widget                  UxWidget = wgt;
	XtPointer               UxClientData = cd;
	XtPointer               UxCallbackArg = cb;

	UxSaveCtx = UxModifyPrinterContext;
	UxModifyPrinterContext = UxContext =
			(_UxCModifyPrinter *) UxGetContext( UxWidget );
	{
	UxPopdownInterface( UxContext->UxModifyPrinter );
	}

}

static void
TextFocusCB(
	Widget wgt, 
	XtPointer cd, 
	XtPointer cb)
{
	_UxCModifyPrinter  *UxSaveCtx, *UxContext;
	Widget                  UxWidget = wgt;
	XtPointer               UxClientData = cd;
	XmAnyCallbackStruct*    cbs = (XmAnyCallbackStruct*)cb;

	UxSaveCtx = UxModifyPrinterContext;
	UxModifyPrinterContext = UxContext =
			(_UxCModifyPrinter *) UxGetContext( UxWidget );
	{

	/* When the text field gets focus, disable the default
	 * pushbutton for the dialog.  This allows the text field's
	 * activate callback to work without dismissing the dialog.
	 * Re-enable default pushbutton when losing focus.
	 */

	if (cbs->reason == XmCR_FOCUS) {
		XtVaSetValues(ModifyPrinter,
			XmNdefaultButton, NULL,
			NULL);
	}
	else if (cbs->reason == XmCR_LOSING_FOCUS) {
		XtVaSetValues(ModifyPrinter,
			XmNdefaultButton, OKPushbutton,
			NULL);
	}

	}

}

void
addPrinterUserListEntryCB(
	Widget    wgt, 
	XtPointer cd, 
	XtPointer cbs)
{
	Widget		userText = (Widget)cd;
	Widget		userList;
	char *		user;
	char *		tmp;
	int		count, i;
	XmString *	itemlist;
	XmString	xstr;

	/* do special "all users" processing */
	XtVaGetValues(userText,
		XmNuserData, &userList,
		NULL);

	user = XmTextFieldGetString(userText);
	xstr = XmStringCreateLocalized(user);
	if ((user[0] == '\0') || XmListItemExists(userList, xstr)) {
		XtFree(user);
		XmStringFree(xstr);
		return;
	}
	XtFree(user);
	XmStringFree(xstr);

	XtVaGetValues(userList,
		XmNitemCount, &count,
		XmNitems, &itemlist,
		NULL);
	
	/* if 'all' is first entry, remove it */
	if (count == 1) {
		XmStringGetLtoR(itemlist[0], XmSTRING_DEFAULT_CHARSET, &tmp);
		if (strcmp(tmp, ALL_USERS) == 0) {
			XmListDeletePos(userList, 1);
		}
		XtFree(tmp);
	}

	/* now call generic add entry callback */
	addListEntryCB(wgt, cd, cbs);
}

void
deletePrinterUserListEntryCB(
	Widget    wgt, 
	XtPointer cd, 
	XtPointer cbs)
{
	Widget		userList = (Widget)cd;
	int		itemcount;
	XmString	xstr;

	/* call generic delete entry callback first */
	deleteListEntryCB(wgt, cd, cbs);

	/* now handle "all users" case */
	XtVaGetValues(userList,
		XmNitemCount, &itemcount,
		NULL);
	
	/* if list is empty, add 'all' */
	if (itemcount == 0) {
		xstr = XmStringCreateLocalized(ALL_USERS);
		XmListAddItemUnselected(userList, xstr, 1);
		XmStringFree(xstr);
	}

}


/*******************************************************************************
       The 'build_' function creates all the widgets
       using the resource values specified in the Property Editor.
*******************************************************************************/

static Widget	_Uxbuild_ModifyPrinter()
{
	Widget		_UxParent;
	Widget		menu2_p2_shell;
	Widget		menu3_p1_shell;
	Widget		tmp;
	int		i;
	int		wnum;
	Widget		wlist[10];
	Dimension	width;
	Dimension	maxwidth = 0;


	_UxParent = UxParent;
	if ( _UxParent == NULL )
	{
		_UxParent = UxTopLevel;
	}

	_UxParent = XtVaCreatePopupShell( "ModifyPrinter_shell",
			xmDialogShellWidgetClass, _UxParent,
			XmNshellUnitType, XmPIXELS,
			XmNminWidth, 340,
			NULL );

	ModifyPrinter = XtVaCreateWidget( "ModifyPrinter",
			xmFormWidgetClass,
			_UxParent,
			XmNunitType, XmPIXELS,
			XmNautoUnmanage, False,
			XmNmarginHeight, 1,
			XmNmarginWidth, 1,
			RES_CONVERT(XmNdialogTitle, catgets(_catd, 8, 220, "Admintool: Modify Printer")),
			NULL );
	UxPutContext( ModifyPrinter, (char *) UxModifyPrinterContext );

	BigRowCol = XtVaCreateManagedWidget( "BigRowCol",
			xmRowColumnWidgetClass,
			ModifyPrinter,
			XmNorientation, XmVERTICAL,
			XmNpacking, XmPACK_COLUMN,
			XmNnumColumns, 1,
			XmNspacing, 0,
			XmNmarginHeight, 1,
			XmNmarginWidth, 1,
			XmNtopAttachment, XmATTACH_FORM,
			XmNtopOffset, 10,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			NULL );
	UxPutContext( BigRowCol, (char *) UxModifyPrinterContext );

	NameForm = XtVaCreateManagedWidget( "NameForm",
			xmFormWidgetClass,
			BigRowCol,
			XmNhorizontalSpacing, 10,
			XmNfractionBase, 1,
			NULL );
	UxPutContext( NameForm, (char *) UxModifyPrinterContext );

	NameLabel = XtVaCreateManagedWidget( "NameLabel",
			xmLabelGadgetClass,
			NameForm,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 221, "Printer Name:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_POSITION,
			XmNtopPosition, 0,
			XmNbottomAttachment, XmATTACH_POSITION,
			XmNbottomPosition, 1,
			NULL );
	UxPutContext( NameLabel, (char *) UxModifyPrinterContext );

	NameText = XtVaCreateManagedWidget( "NameText",
			xmLabelGadgetClass,
			NameForm,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, NameLabel,
			XmNtopAttachment, XmATTACH_POSITION,
			XmNtopPosition, 0,
			XmNbottomAttachment, XmATTACH_POSITION,
			XmNbottomPosition, 1,
			NULL );
	UxPutContext( NameText, (char *) UxModifyPrinterContext );

	ServerForm = XtVaCreateManagedWidget( "ServerForm",
			xmFormWidgetClass,
			BigRowCol,
			XmNhorizontalSpacing, 10,
			XmNfractionBase, 1,
			NULL );
	UxPutContext( ServerForm, (char *) UxModifyPrinterContext );

	ServerLabel = XtVaCreateManagedWidget( "ServerLabel",
			xmLabelGadgetClass,
			ServerForm,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 222, "Print Server:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_POSITION,
			XmNtopPosition, 0,
			XmNbottomAttachment, XmATTACH_POSITION,
			XmNbottomPosition, 1,
			NULL );
	UxPutContext( ServerLabel, (char *) UxModifyPrinterContext );

	ServerText = XtVaCreateManagedWidget( "ServerText",
			xmLabelGadgetClass,
			ServerForm,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, ServerLabel,
			XmNtopAttachment, XmATTACH_POSITION,
			XmNtopPosition, 0,
			XmNbottomAttachment, XmATTACH_POSITION,
			XmNbottomPosition, 1,
			NULL );
	UxPutContext( ServerText, (char *) UxModifyPrinterContext );

	CommentForm = XtVaCreateManagedWidget( "CommentForm",
			xmFormWidgetClass,
			BigRowCol,
			XmNhorizontalSpacing, 10,
			XmNfractionBase, 1,
			NULL );
	UxPutContext( CommentForm, (char *) UxModifyPrinterContext );

	CommentLabel = XtVaCreateManagedWidget( "CommentLabel",
			xmLabelGadgetClass,
			CommentForm,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 223, "Description:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_POSITION,
			XmNtopPosition, 0,
			XmNbottomAttachment, XmATTACH_POSITION,
			XmNbottomPosition, 1,
			NULL );
	UxPutContext( CommentLabel, (char *) UxModifyPrinterContext );

	CommentText = XtVaCreateManagedWidget( "CommentText",
			xmTextFieldWidgetClass,
			CommentForm,
			XmNmaxLength, MAXCOMMENTLEN,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, CommentLabel,
			XmNrightAttachment, XmATTACH_FORM,
			XmNrightOffset, 10,
			XmNtopAttachment, XmATTACH_POSITION,
			XmNtopPosition, 0,
			XmNbottomAttachment, XmATTACH_POSITION,
			XmNbottomPosition, 1,
			NULL );
	UxPutContext( CommentText, (char *) UxModifyPrinterContext );

	PortForm = XtVaCreateManagedWidget( "PortForm",
			xmFormWidgetClass,
			BigRowCol,
			XmNhorizontalSpacing, 10,
			XmNfractionBase, 1,
			NULL );
	UxPutContext( PortForm, (char *) UxModifyPrinterContext );

	PortLabel = XtVaCreateManagedWidget( "PortLabel",
			xmLabelGadgetClass,
			PortForm,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 224, "Printer Port:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_POSITION,
			XmNtopPosition, 0,
			XmNbottomAttachment, XmATTACH_POSITION,
			XmNbottomPosition, 1,
			NULL );
	UxPutContext( PortLabel, (char *) UxModifyPrinterContext );

	menu4_p1_shell = XtVaCreatePopupShell ("menu4_p1_shell",
			xmMenuShellWidgetClass, ModifyPrinter,
			XmNwidth, 1,
			XmNheight, 1,
			XmNallowShellResize, TRUE,
			XmNoverrideRedirect, TRUE,
			NULL );
	UxPutContext( menu4_p1_shell, (char *) UxModifyPrinterContext );

	menu4_p1 = XtVaCreateWidget( "menu4_p1",
			xmRowColumnWidgetClass,
			menu4_p1_shell,
			XmNrowColumnType, XmMENU_PULLDOWN,
			NULL );
	UxPutContext( menu4_p1, (char *) UxModifyPrinterContext );

	PortOptionMenu = XtVaCreateManagedWidget( "PortOptionMenu",
			xmRowColumnWidgetClass,
			PortForm,
			XmNrowColumnType, XmMENU_OPTION,
			XmNsubMenuId, menu4_p1,
			RES_CONVERT( XmNlabelString, "" ),
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, PortLabel,
			XmNleftOffset, 0,
			XmNtopAttachment, XmATTACH_POSITION,
			XmNtopPosition, 0,
			XmNbottomAttachment, XmATTACH_POSITION,
			XmNbottomPosition, 1,
			NULL );
	UxPutContext( PortOptionMenu, (char *) UxModifyPrinterContext );

	TypeForm = XtVaCreateManagedWidget( "TypeForm",
			xmFormWidgetClass,
			BigRowCol,
			XmNhorizontalSpacing, 10,
			XmNfractionBase, 1,
			NULL );
	UxPutContext( TypeForm, (char *) UxModifyPrinterContext );

	TypeLabel = XtVaCreateManagedWidget( "TypeLabel",
			xmLabelGadgetClass,
			TypeForm,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 225, "Printer Type:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_POSITION,
			XmNtopPosition, 0,
			XmNbottomAttachment, XmATTACH_POSITION,
			XmNbottomPosition, 1,
			NULL );
	UxPutContext( TypeLabel, (char *) UxModifyPrinterContext );

	TypeText = XtVaCreateManagedWidget( "TypeText",
			xmLabelGadgetClass,
			TypeForm,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, TypeLabel,
			XmNtopAttachment, XmATTACH_POSITION,
			XmNtopPosition, 0,
			XmNbottomAttachment, XmATTACH_POSITION,
			XmNbottomPosition, 1,
			NULL );
	UxPutContext( TypeText, (char *) UxModifyPrinterContext );

	ContentsForm = XtVaCreateManagedWidget( "ContentsForm",
			xmFormWidgetClass,
			BigRowCol,
			XmNhorizontalSpacing, 10,
			XmNfractionBase, 1,
			NULL );
	UxPutContext( ContentsForm, (char *) UxModifyPrinterContext );

	ContentsLabel = XtVaCreateManagedWidget( "ContentsLabel",
			xmLabelGadgetClass,
			ContentsForm,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 226, "File Contents:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_POSITION,
			XmNtopPosition, 0,
			XmNbottomAttachment, XmATTACH_POSITION,
			XmNbottomPosition, 1,
			NULL );
	UxPutContext( ContentsLabel, (char *) UxModifyPrinterContext );

	menu2_p2_shell = XtVaCreatePopupShell ("menu2_p2_shell",
			xmMenuShellWidgetClass, ModifyPrinter,
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
	UxPutContext( menu2_p2, (char *) UxModifyPrinterContext );

	ContentsPostscriptPushbutton = XtVaCreateManagedWidget( "ContentsPostscriptPushbutton",
			xmPushButtonGadgetClass,
			menu2_p2,
			RES_CONVERT( XmNlabelString, FileContentsLabels[0] ),
			XmNuserData, 0,
			NULL );
	UxPutContext( ContentsPostscriptPushbutton, (char *) UxModifyPrinterContext );

	ContentsASCIIPushbutton = XtVaCreateManagedWidget( "ContentsASCIIPushbutton",
			xmPushButtonGadgetClass,
			menu2_p2,
			RES_CONVERT( XmNlabelString, FileContentsLabels[1] ),
			XmNuserData, 1,
			NULL );
	UxPutContext( ContentsASCIIPushbutton, (char *) UxModifyPrinterContext );

	ContentsBothPushbutton = XtVaCreateManagedWidget( "ContentsBothPushbutton",
			xmPushButtonGadgetClass,
			menu2_p2,
			RES_CONVERT( XmNlabelString, FileContentsLabels[2] ),
			XmNuserData, 2,
			NULL );
	UxPutContext( ContentsBothPushbutton, (char *) UxModifyPrinterContext );

	ContentsNonePushbutton = XtVaCreateManagedWidget( "ContentsNonePushbutton",
			xmPushButtonGadgetClass,
			menu2_p2,
			RES_CONVERT( XmNlabelString, FileContentsLabels[3] ),
			XmNuserData, 3,
			NULL );
	UxPutContext( ContentsNonePushbutton, (char *) UxModifyPrinterContext );

	ContentsAnyPushbutton = XtVaCreateManagedWidget( "ContentsAnyPushbutton",
			xmPushButtonGadgetClass,
			menu2_p2,
			RES_CONVERT( XmNlabelString, FileContentsLabels[4] ),
			XmNuserData, 4,
			NULL );
	UxPutContext( ContentsAnyPushbutton, (char *) UxModifyPrinterContext );

	ContentsOptionMenu = XtVaCreateManagedWidget( "ContentsOptionMenu",
			xmRowColumnWidgetClass,
			ContentsForm,
			XmNrowColumnType, XmMENU_OPTION,
			XmNsubMenuId, menu2_p2,
			RES_CONVERT( XmNlabelString, "" ),
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, ContentsLabel,
			XmNleftOffset, 0,
			XmNtopAttachment, XmATTACH_POSITION,
			XmNtopPosition, 0,
			XmNbottomAttachment, XmATTACH_POSITION,
			XmNbottomPosition, 1,
			NULL );
	UxPutContext( ContentsOptionMenu, (char *) UxModifyPrinterContext );

	FaultForm = XtVaCreateManagedWidget( "FaultForm",
			xmFormWidgetClass,
			BigRowCol,
			XmNhorizontalSpacing, 10,
			XmNfractionBase, 1,
			NULL );
	UxPutContext( FaultForm, (char *) UxModifyPrinterContext );

	FaultLabel = XtVaCreateManagedWidget( "FaultLabel",
			xmLabelGadgetClass,
			FaultForm,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 227, "Fault Notification:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_POSITION,
			XmNtopPosition, 0,
			XmNbottomAttachment, XmATTACH_POSITION,
			XmNbottomPosition, 1,
			NULL );
	UxPutContext( FaultLabel, (char *) UxModifyPrinterContext );

	menu3_p1_shell = XtVaCreatePopupShell ("menu3_p1_shell",
			xmMenuShellWidgetClass, ModifyPrinter,
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
	UxPutContext( menu3_p1, (char *) UxModifyPrinterContext );

	WritePushbutton = XtVaCreateManagedWidget( "WritePushbutton",
			xmPushButtonGadgetClass,
			menu3_p1,
			RES_CONVERT( XmNlabelString, FaultLabels[0] ),
			XmNuserData, 0,
			NULL );
	UxPutContext( WritePushbutton, (char *) UxModifyPrinterContext );

	MailPushbutton = XtVaCreateManagedWidget( "MailPushbutton",
			xmPushButtonGadgetClass,
			menu3_p1,
			RES_CONVERT( XmNlabelString, FaultLabels[1] ),
			XmNuserData, 1,
			NULL );
	UxPutContext( MailPushbutton, (char *) UxModifyPrinterContext );

	NonePushbutton = XtVaCreateManagedWidget( "NonePushbutton",
			xmPushButtonGadgetClass,
			menu3_p1,
			RES_CONVERT( XmNlabelString, FaultLabels[2] ),
			XmNuserData, 2,
			NULL );
	UxPutContext( NonePushbutton, (char *) UxModifyPrinterContext );

	FaultOptionMenu = XtVaCreateManagedWidget( "FaultOptionMenu",
			xmRowColumnWidgetClass,
			FaultForm,
			XmNrowColumnType, XmMENU_OPTION,
			XmNsubMenuId, menu3_p1,
			RES_CONVERT( XmNlabelString, "" ),
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, FaultLabel,
			XmNleftOffset, 0,
			XmNtopAttachment, XmATTACH_POSITION,
			XmNtopPosition, 0,
			XmNbottomAttachment, XmATTACH_POSITION,
			XmNbottomPosition, 1,
			NULL );
	UxPutContext( FaultOptionMenu, (char *) UxModifyPrinterContext );

	OptionsForm = XtVaCreateManagedWidget( "OptionsForm",
			xmFormWidgetClass,
			ModifyPrinter,
			XmNhorizontalSpacing, 10,
			XmNfractionBase, 1,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, BigRowCol,
			NULL );
	UxPutContext( OptionsForm, (char *) UxModifyPrinterContext );

	OptionsLabel = XtVaCreateManagedWidget( "OptionsLabel",
			xmLabelGadgetClass,
			OptionsForm,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 228, "Options:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_FORM,
			XmNtopOffset, 3,
			NULL );
	UxPutContext( OptionsLabel, (char *) UxModifyPrinterContext );

	OptionsRowCol = XtVaCreateManagedWidget( "OptionsRowCol",
			xmRowColumnWidgetClass,
			OptionsForm,
			XmNorientation, XmVERTICAL,
			XmNnumColumns, 1,
			XmNspacing, 0,
			XmNmarginWidth, 0,
			XmNmarginHeight, 0,
			XmNtopAttachment, XmATTACH_FORM,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, OptionsLabel,
			XmNleftOffset, 7,
			NULL );

	DefaultTogglebutton = XtVaCreateManagedWidget( "DefaultTogglebutton",
			xmToggleButtonGadgetClass,
			OptionsRowCol,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 229, "Default Printer") ),
			NULL );
	UxPutContext( DefaultTogglebutton, (char *) UxModifyPrinterContext );

	BannerTogglebutton = XtVaCreateManagedWidget( "BannerTogglebutton",
			xmToggleButtonGadgetClass,
			OptionsRowCol,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 230, "Always Print Banner") ),
			NULL );
	UxPutContext( BannerTogglebutton, (char *) UxModifyPrinterContext );

	EnableQueueTogglebutton = XtVaCreateManagedWidget( "EnableQueueTogglebutton",
			xmToggleButtonGadgetClass,
			OptionsRowCol,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 231, "Accept Print Requests") ),
			NULL );
	UxPutContext( EnableQueueTogglebutton, (char *) UxModifyPrinterContext );

	AcceptJobsTogglebutton = XtVaCreateManagedWidget( "AcceptJobsTogglebutton",
			xmToggleButtonGadgetClass,
			OptionsRowCol,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 232, "Process Print Requests") ),
			NULL );
	UxPutContext( AcceptJobsTogglebutton, (char *) UxModifyPrinterContext );

	UserForm = XtVaCreateManagedWidget( "UserForm",
			xmFormWidgetClass,
			ModifyPrinter,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, OptionsForm,
			XmNtopOffset, 10,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			NULL );
	UxPutContext( UserForm, (char *) UxModifyPrinterContext );

	UserListLabel = XtVaCreateManagedWidget( "UserListLabel",
			xmLabelGadgetClass,
			UserForm,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 233, "User Access List:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNtopAttachment, XmATTACH_FORM,
			XmNleftAttachment, XmATTACH_FORM,
			XmNleftOffset, 10,
			NULL );
	UxPutContext( UserListLabel, (char *) UxModifyPrinterContext );

	scrolledWindow2 = XtVaCreateManagedWidget( "scrolledWindow2",
			xmScrolledWindowWidgetClass,
			UserForm,
			XmNscrollingPolicy, XmAPPLICATION_DEFINED,
			XmNlistSizePolicy, XmRESIZE_IF_POSSIBLE,
			XmNvisualPolicy, XmVARIABLE,
			XmNshadowThickness, 0,
			XmNtopAttachment, XmATTACH_FORM,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, UserListLabel,
			XmNleftOffset, 10,
			XmNrightAttachment, XmATTACH_FORM,
			XmNrightOffset, 10,
			NULL );
	UxPutContext( scrolledWindow2, (char *) UxModifyPrinterContext );

	UserList = XtVaCreateManagedWidget( "UserList",
			xmListWidgetClass,
			scrolledWindow2,
			XmNvisibleItemCount, 3,
			XmNselectionPolicy, XmEXTENDED_SELECT,
			NULL );
	UxPutContext( UserList, (char *) UxModifyPrinterContext );

	UserText = XtVaCreateManagedWidget( "UserText",
			xmTextFieldWidgetClass,
			UserForm,
			XmNmaxLength, MAXUSERNAMELEN,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, UserListLabel,
			XmNleftOffset, 10,
			XmNrightAttachment, XmATTACH_FORM,
			XmNrightOffset, 10,
			NULL );
	UxPutContext( UserText, (char *) UxModifyPrinterContext );

	XtVaSetValues(scrolledWindow2,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, UserText,
		NULL);

	tmp = XtVaCreateManagedWidget( "",
			xmFormWidgetClass,
			UserForm,
			XmNfractionBase, 7,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, UserListLabel,
			XmNleftOffset, 10,
			XmNrightAttachment, XmATTACH_FORM,
			XmNrightOffset, 10,
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
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 234, "Add") ),
			XmNleftAttachment, XmATTACH_POSITION,
			XmNleftPosition, 1,
			XmNrightAttachment, XmATTACH_POSITION,
			XmNrightPosition, 3,
			NULL );
	UxPutContext( AddPushbutton, (char *) UxModifyPrinterContext );

	DeletePushbutton = XtVaCreateManagedWidget( "DeletePushbutton",
			xmPushButtonGadgetClass,
			tmp,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 235, "Delete") ),
			XmNleftAttachment, XmATTACH_POSITION,
			XmNleftPosition, 4,
			XmNrightAttachment, XmATTACH_POSITION,
			XmNrightPosition, 6,
			NULL );
	UxPutContext( DeletePushbutton, (char *) UxModifyPrinterContext );

	ButtonBox = create_button_box(ModifyPrinter, NULL, UxModifyPrinterContext,
		&OKPushbutton, &ApplyPushbutton, &ResetPushbutton,
		&CancelPushbutton, &HelpPushbutton);

	XtVaSetValues(ButtonBox,
		XmNtopAttachment, XmATTACH_NONE,
		NULL);

	XtVaSetValues(UserForm,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, ButtonBox,
		XmNbottomOffset, 10,
		NULL);

	XtVaSetValues(ModifyPrinter,
		XmNinitialFocus, CommentText,
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
		(XtPointer) UxModifyPrinterContext );

	XtAddCallback( UserText, XmNlosingFocusCallback,
		(XtCallbackProc) TextFocusCB,
		(XtPointer) UxModifyPrinterContext );

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
		(XtPointer) UxModifyPrinterContext );

	XtAddCallback( ApplyPushbutton, XmNactivateCallback,
		(XtCallbackProc) OKPushbutton_activateCB,
		(XtPointer) UxModifyPrinterContext );

	XtAddCallback( ResetPushbutton, XmNactivateCallback,
		(XtCallbackProc) ResetPushbutton_activateCB,
		(XtPointer) UxModifyPrinterContext );

	XtAddCallback( CancelPushbutton, XmNactivateCallback,
		(XtCallbackProc) CancelPushbutton_activateCB,
		(XtPointer) UxModifyPrinterContext );

	XtAddCallback( HelpPushbutton, XmNactivateCallback,
		(XtCallbackProc) helpCB,
		"printer_window.r.hlp" );


	XtAddCallback( ModifyPrinter, XmNdestroyCallback,
		(XtCallbackProc) UxDestroyContextCB,
		(XtPointer) UxModifyPrinterContext);


	return ( ModifyPrinter );
}

/*******************************************************************************
       The following is the 'Interface function' which is the
       external entry point for creating this interface.
       This function should be called from your application or from
       a callback function.
*******************************************************************************/

Widget	create_ModifyPrinter(Widget _UxUxParent)
{
	Widget                  rtrn;
	_UxCModifyPrinter       *UxContext;

	UxModifyPrinterContext = UxContext =
		(_UxCModifyPrinter *) UxNewContext( sizeof(_UxCModifyPrinter), False );

	UxParent = _UxUxParent;

	rtrn = _Uxbuild_ModifyPrinter();

	/* null structure */
	memset(&UxContext->printer, 0, sizeof(SysmanPrinterArg));

	return(rtrn);
}

/*******************************************************************************
       END OF FILE
*******************************************************************************/

static void
save_dialog_values(_UxCModifyPrinter * UxContext)
{
	int	i;
	XmStringTable	tmp_list;

	if (save_comment) XtFree(save_comment);
	if (save_user) XtFree(save_user);
	if (save_list) {
		for (i=0; i<save_listcount; i++)
			XmStringFree(save_list[i]);
		XtFree((char *) save_list);
		save_listcount = 0;
	}

	save_comment = XmTextFieldGetString(UxContext->UxCommentText);
	XtVaGetValues(UxContext->UxPortOptionMenu,
		XmNmenuHistory, &save_port,
		NULL);
	XtVaGetValues(UxContext->UxContentsOptionMenu,
		XmNmenuHistory, &save_contents,
		NULL);
	XtVaGetValues(UxContext->UxFaultOptionMenu,
		XmNmenuHistory, &save_fault,
		NULL);
	save_sysdefault = XmToggleButtonGadgetGetState(UxContext->UxDefaultTogglebutton);
	save_banner = XmToggleButtonGadgetGetState(UxContext->UxBannerTogglebutton);
	save_enable = XmToggleButtonGadgetGetState(UxContext->UxEnableQueueTogglebutton);
	save_accept = XmToggleButtonGadgetGetState(UxContext->UxAcceptJobsTogglebutton);
	XtVaGetValues(UxContext->UxUserList,
		XmNitemCount, &save_listcount,
		XmNitems, &tmp_list,
		NULL);
	save_list = (XmString *) XtMalloc(save_listcount * sizeof(XmString *));
	for (i=0; i<save_listcount; i++)
		save_list[i] = XmStringCopy(tmp_list[i]);
	save_user = XmTextFieldGetString(UxContext->UxUserText);
}

static const char *
_strip_whitespace(const char * orig)
{
	char * str;
	char * s;

	
	if (str = (char *) XtMalloc(strlen(orig) + 1)) {
		for (s=str; *orig; orig++)
			if (!isspace(*orig))
				*s++ = *orig;
		*s = '\0';
	}

	return str;
}

int
beingModified(char * printername)
{
	_UxCModifyPrinter  *UxSaveCtx, *UxContext;

	UxSaveCtx = UxModifyPrinterContext;
	UxModifyPrinterContext = UxContext =
		(_UxCModifyPrinter *) UxGetContext(modifyprinterdialog);

	if (modifyprinterdialog && XtIsManaged(modifyprinterdialog)) {
		XmString	xstr;
		char *		modname;

		XtVaGetValues(UxContext->UxNameText,
			XmNlabelString, &xstr,
			NULL);
		XmStringGetLtoR(xstr, XmSTRING_DEFAULT_CHARSET, &modname);
		if (strcmp(printername, modname) == 0)
			return True;
	}

	UxModifyPrinterContext = UxSaveCtx;

	return False;
}

static int
init_modifyprinterdialog(
	Widget			parent,
	SysmanPrinterArg*	printer
)
{
	_UxCModifyPrinter*	UxContext;
	XmString	xstr;
	Widget		w;
	const char*	tmp;
	const char*	name;
	const char*	server;
	const char*	comment;
	const char*	fn;
	const char*	type;
	const char*	type_label;
	char**		portlist;
	char*		users;
	char*		user_p;
	int		numports;
	int		i;
	int		sts;
	Widget		submenu;
	Widget		localPrinterWidgets[4];
	Widget		localPrinterWidgets1[3];


	UxContext = (_UxCModifyPrinter*) UxGetContext(modifyprinterdialog);

	sts = sysman_get_printer(printer, errbuf, ERRBUF_SIZE);

	if (sts != 0) {
		display_error(modifyprinterdialog, errbuf);
		return 0;
	}

	/* printer */
	name = printer->printername ? printer->printername : "";
	xstr = XmStringCreateLocalized((char*)name);
	XtVaSetValues(UxContext->UxNameText,
		XmNlabelString, xstr,
		NULL);
	XmStringFree(xstr);

	/* server */
	server = printer->printserver ? printer->printserver : "";
	xstr = XmStringCreateLocalized((char*)server);
	XtVaSetValues(UxContext->UxServerText,
		XmNlabelString, xstr,
		NULL);
	XmStringFree(xstr);

	/* comment */
	comment = printer->comment ? printer->comment : "";
	XtVaSetValues(UxContext->UxCommentText,
		XmNvalue, comment,
		NULL);

	/* printer type */
	type = printer->printertype ? printer->printertype : 0;
	type_label = NULL;
	for (i=0; PrinterTypeTokens[i] != NULL; i++) {
		if (strcmp(PrinterTypeTokens[i], type) == 0) {
			type_label = PrinterTypeLabels[i];
			break;
		}
	}
	if (type_label == NULL) {
		type_label = *type ? type : UNKNOWNTYPE;
	}
	xstr = XmStringCreateLocalized((char*)type_label);
	XtVaSetValues(UxContext->UxTypeText,
		XmNlabelString, xstr,
		NULL);
	XmStringFree(xstr);

	/* file contents */
	/* strip whitespace to match defined constant */
	tmp = _strip_whitespace(
		printer->file_contents ? printer->file_contents : "");
	w = NULL;
	if (strcmp(tmp, FILE_CONTENTS_PS) == 0)
		w = UxContext->UxContentsPostscriptPushbutton;
	else if (strcmp(tmp, FILE_CONTENTS_SIMPLE) == 0)
		w = UxContext->UxContentsASCIIPushbutton;
	else if (strcmp(tmp, FILE_CONTENTS_BOTH) == 0)
		w = UxContext->UxContentsBothPushbutton;
	else if (strcmp(tmp, FILE_CONTENTS_NONE) == 0)
		w = UxContext->UxContentsNonePushbutton;
	else if (strcmp(tmp, FILE_CONTENTS_ANY) == 0)
		w = UxContext->UxContentsAnyPushbutton;
	if (w)
		XtVaSetValues(UxContext->UxContentsOptionMenu,
			XmNmenuHistory, w,
			NULL);
	XtFree((char*)tmp);

	/* System Default */
	XmToggleButtonGadgetSetState(UxContext->UxDefaultTogglebutton,
		(Boolean) printer->default_p,
		True);

	/* Enable Print Queue */
	XmToggleButtonGadgetSetState(UxContext->UxEnableQueueTogglebutton,
		(Boolean) printer->enable_p,
		True);

	/* Accept Print Jobs */
	XmToggleButtonGadgetSetState(UxContext->UxAcceptJobsTogglebutton,
		(Boolean) printer->accept_p,
		True);

	localPrinterWidgets[0] = UxContext->UxPortForm;
	localPrinterWidgets[1] = UxContext->UxTypeForm;
	localPrinterWidgets[2] = UxContext->UxContentsForm;
	localPrinterWidgets[3] = UxContext->UxFaultForm;
	localPrinterWidgets1[0] = UxContext->UxBannerTogglebutton;
	localPrinterWidgets1[1] = UxContext->UxEnableQueueTogglebutton;
	localPrinterWidgets1[2] = UxContext->UxAcceptJobsTogglebutton;

	if (strcmp(server, localhost) != 0) {
		/* remote printer */
		if (XtIsManaged(UxContext->UxUserForm)) {
			/* local printer currently displayed, need to change
			 * to remote printer layout
			 */
			XtUnmanageChild(ModifyPrinter);

			XtVaSetValues(UserForm,
				XmNtopAttachment, XmATTACH_NONE,
				XmNbottomAttachment, XmATTACH_NONE,
				NULL);
			XtVaSetValues(ButtonBox,
				XmNtopAttachment, XmATTACH_WIDGET,
				XmNtopWidget, OptionsRowCol,
				XmNtopOffset, 10,
				NULL);
			XtUnmanageChildren(localPrinterWidgets, 4);
			XtUnmanageChildren(localPrinterWidgets1, 3);
			XtUnmanageChild(UxContext->UxUserForm);
	
			XtVaSetValues(XtParent(modifyprinterdialog),
				XmNminHeight, 190,
				NULL);

			XtManageChild(ModifyPrinter);
		}

		/* server OS */
		save_protocol =
			(char*)(printer->protocol ? printer->protocol : "");
	}
	else {
		/* local printer */
		if (!XtIsManaged(UxContext->UxUserForm)) {
			/* remote printer currently displayed, need to change
			 * to local printer layout
			 */
			XtUnmanageChild(ModifyPrinter);

			XtVaSetValues(ButtonBox,
				XmNtopAttachment, XmATTACH_NONE,
				NULL);
			XtVaSetValues(UserForm,
				XmNtopAttachment, XmATTACH_WIDGET,
				XmNtopWidget, OptionsRowCol,
				XmNtopOffset, 10,
				XmNbottomAttachment, XmATTACH_WIDGET,
				XmNbottomWidget, ButtonBox,
				XmNbottomOffset, 10,
				NULL);
			XtManageChildren(localPrinterWidgets, 4);
			XtManageChildren(localPrinterWidgets1, 3);
			XtManageChild(UxContext->UxUserForm);
	
			XtVaSetValues(XtParent(modifyprinterdialog),
				XmNminHeight, 570,
				NULL);

			XtManageChild(ModifyPrinter);
		}

		/* serial port */
		numports = sysman_list_printer_devices(
			&portlist, errbuf, ERRBUF_SIZE);
		if (numports >= 0) {
			build_port_optionmenu(numports, portlist,
				(char*)printer->device, PortOptionMenu);
			sysman_free_printer_devices_list(portlist, numports);
		}
		else {
			display_error(modifyprinterdialog, errbuf);
			return 0;
		}

	
		/* fault notification */
		fn = printer->notify ? printer->notify : "";
		w = NULL;
		if (strcmp(fn, FAULT_WRITE) == 0) {
			w = UxContext->UxWritePushbutton;
		}
		else if (strcmp(fn, FAULT_MAIL) == 0) {
			w = UxContext->UxMailPushbutton;
		}
		else if (strcmp(fn, FAULT_NONE) == 0) {
			w = UxContext->UxNonePushbutton;
		}
		if (w) {
			XtVaSetValues(UxContext->UxFaultOptionMenu,
				XmNmenuHistory, w,
				NULL);
		}
	
		/* Print Banner */
		XmToggleButtonGadgetSetState(UxContext->UxBannerTogglebutton,
			(Boolean)printer->banner_req_p,
			True);
	
		/* User Access List */
		XmListDeleteAllItems(UxContext->UxUserList);
		XtVaSetValues(UxContext->UxUserText,
			XmNvalue, "",
			NULL);

		if (printer->user_allow_list && *printer->user_allow_list) {
			users = strdup(printer->user_allow_list);
			user_p = strtok(users, ",");
			while (user_p != NULL) {
				xstr = XmStringCreateLocalized(user_p);
				XmListAddItemUnselected(UxContext->UxUserList,
					xstr, 0);
				XmStringFree(xstr);
				user_p = strtok(NULL, ",");
			}
			free(users);
		}
	}


	return 1;
}

void
show_modifyprinterdialog(
	Widget			parent,
	SysmanPrinterArg*	printer,
	sysMgrMainCtxt * ctxt
)
{
	_UxCModifyPrinter*	UxContext;

	SetBusyPointer(True);

	if (modifyprinterdialog == NULL)
		modifyprinterdialog = create_ModifyPrinter(parent);
	
	ctxt->currDialog = modifyprinterdialog;
	UxContext = (_UxCModifyPrinter*) UxGetContext(modifyprinterdialog);

	free_printer(&UxContext->printer);
	copy_printer(&UxContext->printer, printer);

	if (init_modifyprinterdialog(
		modifyprinterdialog, &UxContext->printer)) {
		save_dialog_values(UxContext);
		XtManageChild(modifyprinterdialog);
		XtPopup(XtParent(modifyprinterdialog), XtGrabNone);
	}

	SetBusyPointer(False);
}


