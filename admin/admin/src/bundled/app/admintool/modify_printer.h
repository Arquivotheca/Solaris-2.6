
/* Copyright 1993 Sun Microsystems, Inc. */

#ifndef	_MODIFYPRINTER_HH
#define	_MODIFYPRINTER_HH

#pragma ident "@(#)modify_printer.h	1.5 95/02/27 Sun Microsystems"

/*	modify_printer.h	*/

#include <stdio.h>
#include <Xm/Xm.h>
#include <Xm/DialogS.h>
#include <Xm/MenuShell.h>
#include "UxXt.h"

#include <Xm/SeparatoG.h>
#include <Xm/List.h>
#include <Xm/ScrolledW.h>
#include <Xm/PushBG.h>
#include <Xm/Label.h>
#include <Xm/ToggleBG.h>
#include <Xm/RowColumn.h>
#include <Xm/TextF.h>
#include <Xm/LabelG.h>
#include <Xm/Form.h>

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
	Widget	UxModifyPrinter;
	Widget	UxBigRowCol;
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
	Widget	UxTypeText;
	Widget	UxContentsForm;
	Widget	UxContentsLabel;
	Widget	Uxmenu2_p2;
	Widget	UxContentsPostscriptPushbutton;
	Widget	UxContentsASCIIPushbutton;
	Widget	UxContentsBothPushbutton;
	Widget	UxContentsNonePushbutton;
	Widget	UxContentsAnyPushbutton;
	Widget	UxContentsOptionMenu;
	Widget	UxFaultForm;
	Widget	UxFaultLabel;
	Widget	Uxmenu3_p1;
	Widget	UxWritePushbutton;
	Widget	UxMailPushbutton;
	Widget	UxNonePushbutton;
	Widget	UxFaultOptionMenu;
	Widget	UxOptionsForm;
	Widget	UxOptionsLabel;
	Widget	UxOptionsRowCol;
	Widget	UxDefaultTogglebutton;
	Widget	UxBannerTogglebutton;
	Widget	UxNISTogglebutton;
	Widget	UxEnableQueueTogglebutton;
	Widget	UxAcceptJobsTogglebutton;
	Widget	UxUserForm;
	Widget	UxscrolledWindow2;
	Widget	UxUserList;
	Widget	UxUserListLabel;
	Widget	UxAddPushbutton;
	Widget	UxDeletePushbutton;
	Widget	UxUserText;
	Widget	UxButtonBox;
	Widget	UxSeparator;
	Widget	UxOKPushbutton;
	Widget	UxApplyPushbutton;
	Widget	UxResetPushbutton;
	Widget	UxCancelPushbutton;
	Widget	UxHelpPushbutton;
	Widget	UxUxParent;

	SysmanPrinterArg printer;
} _UxCModifyPrinter;

#ifdef CONTEXT_MACRO_ACCESS
static _UxCModifyPrinter     *UxModifyPrinterContext;
#define ModifyPrinter         UxModifyPrinterContext->UxModifyPrinter
#define BigRowCol               UxModifyPrinterContext->UxBigRowCol
#define NameForm                UxModifyPrinterContext->UxNameForm
#define NameLabel               UxModifyPrinterContext->UxNameLabel
#define NameText                UxModifyPrinterContext->UxNameText
#define ServerForm              UxModifyPrinterContext->UxServerForm
#define ServerLabel             UxModifyPrinterContext->UxServerLabel
#define ServerText              UxModifyPrinterContext->UxServerText
#define CommentForm             UxModifyPrinterContext->UxCommentForm
#define CommentLabel            UxModifyPrinterContext->UxCommentLabel
#define CommentText             UxModifyPrinterContext->UxCommentText
#define PortForm                UxModifyPrinterContext->UxPortForm
#define PortLabel               UxModifyPrinterContext->UxPortLabel
#define menu4_p1_shell		UxModifyPrinterContext->Uxmenu4_p1_shell
#define menu4_p1		UxModifyPrinterContext->Uxmenu4_p1
#define PortOtherPushbutton	UxModifyPrinterContext->UxPortOtherPushbutton
#define PortOptionMenu          UxModifyPrinterContext->UxPortOptionMenu
#define TypeForm                UxModifyPrinterContext->UxTypeForm
#define TypeLabel               UxModifyPrinterContext->UxTypeLabel
#define TypeText                UxModifyPrinterContext->UxTypeText
#define ContentsForm            UxModifyPrinterContext->UxContentsForm
#define ContentsLabel           UxModifyPrinterContext->UxContentsLabel
#define menu2_p2                UxModifyPrinterContext->Uxmenu2_p2
#define ContentsPostscriptPushbutton   UxModifyPrinterContext->UxContentsPostscriptPushbutton
#define ContentsASCIIPushbutton        UxModifyPrinterContext->UxContentsASCIIPushbutton
#define ContentsBothPushbutton         UxModifyPrinterContext->UxContentsBothPushbutton
#define ContentsNonePushbutton         UxModifyPrinterContext->UxContentsNonePushbutton
#define ContentsAnyPushbutton          UxModifyPrinterContext->UxContentsAnyPushbutton
#define ContentsOptionMenu      UxModifyPrinterContext->UxContentsOptionMenu
#define FaultForm               UxModifyPrinterContext->UxFaultForm
#define FaultLabel              UxModifyPrinterContext->UxFaultLabel
#define menu3_p1		UxModifyPrinterContext->Uxmenu3_p1
#define WritePushbutton		UxModifyPrinterContext->UxWritePushbutton
#define MailPushbutton		UxModifyPrinterContext->UxMailPushbutton
#define NonePushbutton		UxModifyPrinterContext->UxNonePushbutton
#define FaultOptionMenu		UxModifyPrinterContext->UxFaultOptionMenu
#define OptionsForm		UxModifyPrinterContext->UxOptionsForm
#define OptionsLabel		UxModifyPrinterContext->UxOptionsLabel
#define OptionsRowCol		UxModifyPrinterContext->UxOptionsRowCol
#define DefaultTogglebutton     UxModifyPrinterContext->UxDefaultTogglebutton
#define BannerTogglebutton      UxModifyPrinterContext->UxBannerTogglebutton
#define NISTogglebutton         UxModifyPrinterContext->UxNISTogglebutton
#define EnableQueueTogglebutton UxModifyPrinterContext->UxEnableQueueTogglebutton
#define AcceptJobsTogglebutton  UxModifyPrinterContext->UxAcceptJobsTogglebutton
#define UserForm		UxModifyPrinterContext->UxUserForm
#define scrolledWindow2         UxModifyPrinterContext->UxscrolledWindow2
#define UserList		UxModifyPrinterContext->UxUserList
#define UserListLabel		UxModifyPrinterContext->UxUserListLabel
#define AddPushbutton		UxModifyPrinterContext->UxAddPushbutton
#define DeletePushbutton	UxModifyPrinterContext->UxDeletePushbutton
#define separatorGadget2	UxModifyPrinterContext->UxseparatorGadget2
#define UserText		UxModifyPrinterContext->UxUserText
#define ButtonBox               UxModifyPrinterContext->UxButtonBox
#define Separator               UxModifyPrinterContext->UxSeparator
#define OKPushbutton            UxModifyPrinterContext->UxOKPushbutton
#define ApplyPushbutton         UxModifyPrinterContext->UxApplyPushbutton
#define ResetPushbutton         UxModifyPrinterContext->UxResetPushbutton
#define CancelPushbutton        UxModifyPrinterContext->UxCancelPushbutton
#define HelpPushbutton          UxModifyPrinterContext->UxHelpPushbutton
#define UxParent                UxModifyPrinterContext->UxUxParent

#endif /* CONTEXT_MACRO_ACCESS */


/*******************************************************************************
       Declarations of global functions.
*******************************************************************************/

Widget	create_ModifyPrinter( Widget _UxUxParent );
void	show_modifyprinterdialog(Widget parent, SysmanPrinterArg* printer,
			sysMgrMainCtxt * ctxt);

#endif	/* _MODIFYPRINTER_HH */

