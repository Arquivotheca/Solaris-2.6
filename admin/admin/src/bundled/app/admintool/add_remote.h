
/* Copyright 1993 Sun Microsystems, Inc. */

#ifndef	_ADDREMOTEPRINTER_HH
#define	_ADDREMOTEPRINTER_HH

#pragma ident "@(#)add_remote.h	1.1 94/10/25 Sun Microsystems"

/*	add_remote.h	*/

#include <stdio.h>
#include <Xm/Xm.h>
#include <Xm/DialogS.h>
#include <Xm/MenuShell.h>
#include "UxXt.h"

#include <Xm/SeparatoG.h>
#include <Xm/PushBG.h>
#include <Xm/ToggleBG.h>
#include <Xm/RowColumn.h>
#include <Xm/TextF.h>
#include <Xm/Label.h>
#include <Xm/Form.h>
#include <Xm/List.h>
#include <Xm/ScrolledW.h>
#include <Xm/Frame.h>

/*******************************************************************************
       The definition of the context structure:
       If you create multiple instances of your interface, the context
       structure ensures that your callbacks use the variables for the
       correct instance.

       For each swidget in the interface, each argument to the Interface
       function, and each variable in the Instance Specific section of the
       Declarations Editor, there is an entry in the context structure.
       and a #define.  The #define makes the variable name refer to the
       corresponding entry in the context structure.
*******************************************************************************/

typedef	struct
{
	Widget	UxAddRemotePrinter;
	Widget	UxBigRowCol;
	Widget	UxClientLabel;
	Widget	UxClientForm;
	Widget	UxClientText;
	Widget	UxAddPushbutton;
	Widget	UxDeletePushbutton;
	Widget	UxNameForm;
	Widget	UxNameLabel;
	Widget	UxNameText;
	Widget	UxServerForm;
	Widget	UxServerLabel;
	Widget	UxServerText;
	Widget	UxCommentForm;
	Widget	UxCommentLabel;
	Widget	UxCommentText;
	Widget	UxOptionsForm;
	Widget	UxOptionsLabel;
	Widget	UxDefaultToggleButton;
	Widget	UxSeparator;
	Widget	UxOKPushbutton;
	Widget	UxApplyPushbutton;
	Widget	UxResetPushbutton;
	Widget	UxCancelPushbutton;
	Widget	UxHelpPushbutton;
	Widget	UxUxParent;
} _UxCAddRemotePrinter;

#ifdef CONTEXT_MACRO_ACCESS
static _UxCAddRemotePrinter    *UxAddRemotePrinterContext;
#define	AddRemotePrinter	UxAddRemotePrinterContext->UxAddRemotePrinter
#define	BigRowCol		UxAddRemotePrinterContext->UxBigRowCol
#define	ClientLabel		UxAddRemotePrinterContext->UxClientLabel
#define	ClientForm		UxAddRemotePrinterContext->UxClientForm
#define	ClientText		UxAddRemotePrinterContext->UxClientText
#define	AddPushbutton		UxAddRemotePrinterContext->UxAddPushbutton
#define	DeletePushbutton	UxAddRemotePrinterContext->UxDeletePushbutton
#define	NameForm		UxAddRemotePrinterContext->UxNameForm
#define	NameLabel		UxAddRemotePrinterContext->UxNameLabel
#define	NameText		UxAddRemotePrinterContext->UxNameText
#define	ServerForm		UxAddRemotePrinterContext->UxServerForm
#define	ServerLabel		UxAddRemotePrinterContext->UxServerLabel
#define	ServerText		UxAddRemotePrinterContext->UxServerText
#define	CommentForm		UxAddRemotePrinterContext->UxCommentForm
#define	CommentLabel		UxAddRemotePrinterContext->UxCommentLabel
#define	CommentText		UxAddRemotePrinterContext->UxCommentText
#define	OptionsForm		UxAddRemotePrinterContext->UxOptionsForm
#define	OptionsLabel		UxAddRemotePrinterContext->UxOptionsLabel
#define	DefaultToggleButton	UxAddRemotePrinterContext->UxDefaultToggleButton
#define	OKPushbutton		UxAddRemotePrinterContext->UxOKPushbutton
#define	ApplyPushbutton		UxAddRemotePrinterContext->UxApplyPushbutton
#define	ResetPushbutton		UxAddRemotePrinterContext->UxResetPushbutton
#define	CancelPushbutton	UxAddRemotePrinterContext->UxCancelPushbutton
#define	HelpPushbutton		UxAddRemotePrinterContext->UxHelpPushbutton
#define	Separator		UxAddRemotePrinterContext->UxSeparator
#define	UxParent		UxAddRemotePrinterContext->UxUxParent

#endif /* CONTEXT_MACRO_ACCESS */


/*******************************************************************************
       Declarations of global functions.
*******************************************************************************/

Widget	create_AddRemotePrinter( Widget _UxUxParent );
void	show_addremotedialog(Widget parent, sysMgrMainCtxt * ctxt);

#endif	/* _ADDREMOTEPRINTER_HH */

