
/* Copyright 1993 Sun Microsystems, Inc. */

#pragma ident "@(#)add_remote.c	1.12 95/09/02 Sun Microsystems"

/*	add_remote.c	*/

#include <nl_types.h>
#include "util.h"
#include "sysman_iface.h"

#define CONTEXT_MACRO_ACCESS 1
#include "add_remote.h"
#undef CONTEXT_MACRO_ACCESS

static char *		save_printername;
static char *		save_server;
static char *		save_comment;
static Boolean		save_sysdefault;
static char *		save_client;
static int		save_listcount;
static XmStringTable	save_list;

extern nl_catd	_catd;	/* for catgets(), defined in main.c */

extern void set_default_printer_msg(char* printername);
static void save_dialog_values(_UxCAddRemotePrinter * UxContext);

Widget	addremotedialog = NULL;


/*******************************************************************************
       The following are callback functions.
*******************************************************************************/
static	void	OKPushbutton_activateCB(
			Widget wgt, 
			XtPointer cd, 
			XtPointer cb)
{
	_UxCAddRemotePrinter*	UxContext;
	Widget                  UxWidget = wgt;
	XtPointer               UxClientData = cd;
	XtPointer               UxCallbackArg = cb;
	char *	printername;
	char *	server;
	char *	comment;
	Boolean	defaultprinter;
	SysmanPrinterArg	printer;
	int			sts;
	char*		tmp;
	char	errstr[512] = "";
	char	reqargs[64] = "";

	UxContext = (_UxCAddRemotePrinter*) UxGetContext( UxWidget );
	{

	/* Get values from dialog */

	/* Printer Name */
	XtVaGetValues(UxContext->UxNameText,
		XmNvalue, &printername,
		NULL);
	if (printername[0] == '\0') {
		if (reqargs[0] != '\0')
			strcat(reqargs, ", ");
		strcat(reqargs, catgets(_catd, 8, 65, "Printer Name"));
	}

	/* Server */
	XtVaGetValues(UxContext->UxServerText,
		XmNvalue, &server,
		NULL);
	if (server[0] == '\0') {
		if (reqargs[0] != '\0')
			strcat(reqargs, ", ");
		strcat(reqargs, catgets(_catd, 8, 67, "Printer Server"));
	}

	/* Put up error dialog if required fields are missing */
	if (reqargs[0] != '\0') {
		sprintf(errstr, MISSING_REQ_ARGS, reqargs);
		display_error(UxContext->UxAddRemotePrinter, errstr);
		return;
	}

	XtVaGetValues(UxContext->UxCommentText,
		XmNvalue, &comment,
		NULL);
	
	defaultprinter = XmToggleButtonGadgetGetState(UxContext->UxDefaultToggleButton);

	/* Add printer client */
	SetBusyPointer(True);

	memset((void *)&printer, 0, sizeof (printer));

	printer.printername = printername;
	printer.printserver = server;
	printer.comment = comment;
	printer.protocol = "bsd";
	printer.num_restarts = 999;
	printer.default_p = defaultprinter;
	printer.enable_p = B_TRUE;
	printer.accept_p = B_TRUE;

	sts = sysman_add_remote_printer(&printer, errbuf, ERRBUF_SIZE);

	if (sts == 0) {
		add_printer_to_list(&printer);

		if (defaultprinter)
			set_default_printer_msg(printername);

		save_dialog_values(UxContext);

		if (wgt == UxContext->UxOKPushbutton)
			UxPopdownInterface(UxContext->UxAddRemotePrinter);
	}
	else {
		display_error(addremotedialog, errbuf);
	}

	SetBusyPointer(False);

	}
}

static	void	ResetPushbutton_activateCB(
			Widget wgt, 
			XtPointer cd, 
			XtPointer cb)
{
	_UxCAddRemotePrinter  *UxSaveCtx, *UxContext;
	Widget                  UxWidget = wgt;
	XtPointer               UxClientData = cd;
	XtPointer               UxCallbackArg = cb;

	UxSaveCtx = UxAddRemotePrinterContext;
	UxAddRemotePrinterContext = UxContext =
			(_UxCAddRemotePrinter *) UxGetContext( UxWidget );
	{
	XtVaSetValues(UxContext->UxNameText,
		XmNvalue, save_printername,
		NULL);

	XtVaSetValues(UxContext->UxServerText,
		XmNvalue, save_server,
		NULL);

	XtVaSetValues(UxContext->UxCommentText,
		XmNvalue, save_comment,
		NULL);

	XmToggleButtonGadgetSetState(UxContext->UxDefaultToggleButton,
		save_sysdefault, True);
	}
}

static	void	CancelPushbutton_activateCB(
			Widget wgt, 
			XtPointer cd, 
			XtPointer cb)
{
	_UxCAddRemotePrinter  *UxSaveCtx, *UxContext;
	Widget                  UxWidget = wgt;
	XtPointer               UxClientData = cd;
	XtPointer               UxCallbackArg = cb;

	UxSaveCtx = UxAddRemotePrinterContext;
	UxAddRemotePrinterContext = UxContext =
			(_UxCAddRemotePrinter *) UxGetContext( UxWidget );
	{
	UxPopdownInterface( UxContext->UxAddRemotePrinter );
	}

}


/*******************************************************************************
       The 'build_' function creates all the widgets
       using the resource values specified in the Property Editor.
*******************************************************************************/

static Widget	_Uxbuild_AddRemotePrinter()
{
	Widget		_UxParent;

	Widget	bigrc;
	Widget	ButtonBox;
	Widget	tmp;
	int		i;
	int		wnum;
	Widget		wlist[9];
	Dimension	height;
	Dimension	width;
	Dimension	maxwidth = 0;

	_UxParent = UxParent;
	if ( _UxParent == NULL )
	{
		_UxParent = UxTopLevel;
	}

	_UxParent = XtVaCreatePopupShell( "AddRemotePrinter_shell",
			xmDialogShellWidgetClass, _UxParent,
			XmNx, 280,
			XmNy, 260,
			XmNshellUnitType, XmPIXELS,
			XmNminWidth, 325,
			XmNminHeight, 235,
			NULL );

	AddRemotePrinter = XtVaCreateWidget( "AddRemotePrinter",
			xmFormWidgetClass,
			_UxParent,
			XmNunitType, XmPIXELS,
			XmNautoUnmanage, False,
			XmNfractionBase, 31,
			XmNverticalSpacing, 10,
			XmNhorizontalSpacing, 10,
			RES_CONVERT( XmNdialogTitle,
				catgets(_catd, 8, 68, "Admintool: Add Access To Printer") ),
			NULL );
	UxPutContext( AddRemotePrinter, (char *) UxAddRemotePrinterContext );

	bigrc = XtVaCreateManagedWidget( "bigrc",
		xmRowColumnWidgetClass,
		AddRemotePrinter,
		XmNorientation, XmVERTICAL,
		XmNnumColumns, 1,
		XmNspacing, 2,
		XmNmarginHeight, 1,
		XmNmarginWidth, 1,
		XmNtopAttachment, XmATTACH_FORM,
		XmNtopOffset, 10,
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, 10,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, 10,
		NULL );

	ClientForm = XtVaCreateManagedWidget( "ClientForm",
			xmFormWidgetClass,
			bigrc,
			NULL );
	UxPutContext( ClientForm, (char *) UxAddRemotePrinterContext );

	ClientLabel = XtVaCreateManagedWidget( "ClientLabel",
			xmLabelWidgetClass,
			ClientForm,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 69, "Print Client:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_FORM,
			NULL );
	UxPutContext( ClientLabel, (char *) UxAddRemotePrinterContext );

	ClientText = XtVaCreateManagedWidget( "ClientText",
			xmLabelWidgetClass,
			ClientForm,
			RES_CONVERT( XmNlabelString, " " ),
			XmNalignment, XmALIGNMENT_BEGINNING,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, ClientLabel,
			XmNleftOffset, 10,
			NULL );
	UxPutContext( ClientText, (char *) UxAddRemotePrinterContext );

	NameForm = XtVaCreateManagedWidget( "NameForm",
			xmFormWidgetClass,
			bigrc,
			NULL );
	UxPutContext( NameForm, (char *) UxAddRemotePrinterContext );

	NameLabel = XtVaCreateManagedWidget( "NameLabel",
			xmLabelWidgetClass,
			NameForm,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 70, "Printer Name:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_FORM,
			NULL );
	UxPutContext( NameLabel, (char *) UxAddRemotePrinterContext );

	NameText = XtVaCreateManagedWidget( "NameText",
			xmTextFieldWidgetClass,
			NameForm,
			XmNcolumns, 25,
			XmNmaxLength, MAXPRINTERNAMELEN,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, NameLabel,
			XmNleftOffset, 10,
			XmNrightAttachment, XmATTACH_FORM,
			NULL );
	UxPutContext( NameText, (char *) UxAddRemotePrinterContext );

	/* Make client text same height as name textfield */
	XtVaGetValues(NameText,
		XmNheight, &height,
		NULL);
	XtVaSetValues(ClientText,
		XmNheight, height,
		NULL);

	ServerForm = XtVaCreateManagedWidget( "ServerForm",
			xmFormWidgetClass,
			bigrc,
			NULL );
	UxPutContext( ServerForm, (char *) UxAddRemotePrinterContext );

	ServerLabel = XtVaCreateManagedWidget( "ServerLabel",
			xmLabelWidgetClass,
			ServerForm,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 71, "Print Server:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_FORM,
			NULL );
	UxPutContext( ServerLabel, (char *) UxAddRemotePrinterContext );

	ServerText = XtVaCreateManagedWidget( "ServerText",
			xmTextFieldWidgetClass,
			ServerForm,
			XmNcolumns, 25,
			XmNmaxLength, MAXSERVERNAMELEN,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, ServerLabel,
			XmNleftOffset, 10,
			XmNrightAttachment, XmATTACH_FORM,
			NULL );
	UxPutContext( ServerText, (char *) UxAddRemotePrinterContext );

	CommentForm = XtVaCreateManagedWidget( "CommentForm",
			xmFormWidgetClass,
			bigrc,
			NULL );
	UxPutContext( CommentForm, (char *) UxAddRemotePrinterContext );

	CommentLabel = XtVaCreateManagedWidget( "CommentLabel",
			xmLabelWidgetClass,
			CommentForm,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 72, "Description:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_FORM,
			NULL );
	UxPutContext( CommentLabel, (char *) UxAddRemotePrinterContext );

	CommentText = XtVaCreateManagedWidget( "CommentText",
			xmTextFieldWidgetClass,
			CommentForm,
			XmNcolumns, 25,
			XmNmaxLength, MAXCOMMENTLEN,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, CommentLabel,
			XmNleftOffset, 10,
			XmNrightAttachment, XmATTACH_FORM,
			NULL );
	UxPutContext( CommentText, (char *) UxAddRemotePrinterContext );

	OptionsForm = XtVaCreateManagedWidget( "OptionsForm",
			xmFormWidgetClass,
			AddRemotePrinter,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, CommentForm,
			XmNtopOffset, 10,
			XmNleftAttachment, XmATTACH_FORM,
			XmNleftOffset, 10,
			NULL );
	UxPutContext( OptionsForm, (char *) UxAddRemotePrinterContext );

	OptionsLabel = XtVaCreateManagedWidget( "OptionsLabel",
			xmLabelWidgetClass,
			OptionsForm,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 73, "Option:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_FORM,
			NULL );
	UxPutContext( OptionsLabel, (char *) UxAddRemotePrinterContext );

	DefaultToggleButton = XtVaCreateManagedWidget( "DefaultToggleButton",
			xmToggleButtonGadgetClass,
			OptionsForm,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 74, "Default Printer") ),
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, OptionsLabel,
			XmNleftOffset, 7,
			XmNtopAttachment, XmATTACH_FORM,
			NULL );
	UxPutContext( DefaultToggleButton, (char *) UxAddRemotePrinterContext );

	/* Align all labels to right edge of longest label */
	wnum = 5;
	wlist[0] = ClientLabel;
	wlist[1] = NameLabel;
	wlist[2] = ServerLabel;
	wlist[3] = CommentLabel;
	wlist[4] = OptionsLabel;
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

	ButtonBox = create_button_box(AddRemotePrinter, OptionsForm,
		UxAddRemotePrinterContext,
		&OKPushbutton, &ApplyPushbutton, &ResetPushbutton,
		&CancelPushbutton, &HelpPushbutton);

	XtVaSetValues(AddRemotePrinter,
		XmNinitialFocus, NameText,
		NULL);

	XtAddCallback( OKPushbutton, XmNactivateCallback,
		(XtCallbackProc) OKPushbutton_activateCB,
		(XtPointer) UxAddRemotePrinterContext );

	XtAddCallback( ApplyPushbutton, XmNactivateCallback,
		(XtCallbackProc) OKPushbutton_activateCB,
		(XtPointer) UxAddRemotePrinterContext );

	XtAddCallback( ResetPushbutton, XmNactivateCallback,
		(XtCallbackProc) ResetPushbutton_activateCB,
		(XtPointer) UxAddRemotePrinterContext );

	XtAddCallback( CancelPushbutton, XmNactivateCallback,
		(XtCallbackProc) CancelPushbutton_activateCB,
		(XtPointer) UxAddRemotePrinterContext );

	XtAddCallback( HelpPushbutton, XmNactivateCallback,
		(XtCallbackProc) helpCB,
		"printer_remote_window.r.hlp" );

	XtAddCallback( AddRemotePrinter, XmNdestroyCallback,
		(XtCallbackProc) UxDestroyContextCB,
		(XtPointer) UxAddRemotePrinterContext);


	return ( AddRemotePrinter );
}

/*******************************************************************************
       The following is the 'Interface function' which is the
       external entry point for creating this interface.
       This function should be called from your application or from
       a callback function.
*******************************************************************************/

Widget	create_AddRemotePrinter( Widget _UxUxParent )
{
	Widget                  rtrn;
	_UxCAddRemotePrinter    *UxContext;

	UxAddRemotePrinterContext = UxContext = (_UxCAddRemotePrinter *)
		UxNewContext( sizeof(_UxCAddRemotePrinter), False );

	UxParent = _UxUxParent;

	rtrn = _Uxbuild_AddRemotePrinter();

	return(rtrn);
}

/*******************************************************************************
       END OF FILE
*******************************************************************************/

void	show_addremotedialog(Widget parent, sysMgrMainCtxt * ctxt)
{
	_UxCAddRemotePrinter  *UxSaveCtx, *UxContext;
	XmString	str;


	if (addremotedialog && XtIsManaged(addremotedialog)) {
		XtPopup(XtParent(addremotedialog), XtGrabNone);
		return;
	}

	if (addremotedialog == NULL)
		addremotedialog = create_AddRemotePrinter(parent);

	ctxt->currDialog = addremotedialog;
	UxSaveCtx = UxAddRemotePrinterContext;
	UxAddRemotePrinterContext = UxContext =
		(_UxCAddRemotePrinter *) UxGetContext(addremotedialog);

	/* Initialize to default values */

	/* client */
	str = XmStringCreateLocalized(localhost);
	XtVaSetValues(UxContext->UxClientText,
		XmNlabelString, str,
		NULL);
	XmStringFree(str);

	/* printer */
	XtVaSetValues(UxContext->UxNameText,
		XmNvalue, "",
		NULL);

	/* server */
	XtVaSetValues(UxContext->UxServerText,
		XmNvalue, "",
		NULL);

	/* comment */
	XtVaSetValues(UxContext->UxCommentText,
		XmNvalue, "",
		NULL);

	/* default system printer */
	XmToggleButtonGadgetSetState(UxContext->UxDefaultToggleButton,
				False, True);

	save_dialog_values(UxContext);

	UxPopupInterface(addremotedialog, no_grab);

	UxAddRemotePrinterContext = UxSaveCtx;
}

static void save_dialog_values(_UxCAddRemotePrinter * UxContext)
{
	int	i;
	XmStringTable	tmp_list;

	if (save_printername) XtFree(save_printername);
	if (save_server) XtFree(save_server);
	if (save_comment) XtFree(save_comment);
	if (save_client) XtFree(save_client);
	if (save_listcount && save_list) {
		for (i=0; i<save_listcount; i++)
			XmStringFree(save_list[i]);
		XtFree((char *) save_list);
		save_listcount = 0;
	}

	save_printername = XmTextFieldGetString(UxContext->UxNameText);
	save_server = XmTextFieldGetString(UxContext->UxServerText);
	save_comment = XmTextFieldGetString(UxContext->UxCommentText);

	save_sysdefault = XmToggleButtonGadgetGetState(
		UxContext->UxDefaultToggleButton);

}

