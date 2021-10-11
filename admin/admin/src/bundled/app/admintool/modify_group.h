/* Copyright (c) 1993 Sun Microsystems, Inc. */

#pragma ident "@(#)modify_group.h	1.5 95/09/02 Sun Microsystems"


/*******************************************************************************
       modify_group.h

*******************************************************************************/

#ifndef	_ADDGROUPDIALOG_INCLUDED
#define	_ADDGROUPDIALOG_INCLUDED


#include <stdio.h>
#include <Xm/Xm.h>
#include <Xm/MenuShell.h>
#include "UxXt.h"

#include <Xm/TextF.h>
#include <Xm/Separator.h>
#include <Xm/PushB.h>
#include <Xm/Label.h>
#include <Xm/Form.h>
#include <Xm/DialogS.h>

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
	Widget	UxmodGroupDialog;
	Widget	Uxform3;
	Widget	Uxlabel46;
	Widget	UxpushButton4;
	Widget	UxpushButton9;
	Widget	UxpushButton12;
	Widget	UxpushButton5;
	Widget	UxpushButton6;
	Widget	Uxseparator1;
	Widget	UxtextField22;
	Widget	UxgidLabel;
	Widget	Uxlabel47;
	Widget	Uxlabel34;
	Widget	UxtextField16;

	SysmanGroupArg	group;
} _UxCmodGroupDialog;

#ifdef CONTEXT_MACRO_ACCESS
static _UxCmodGroupDialog      *UxModGroupDialogContext;
#define modGroupDialog          UxModGroupDialogContext->UxmodGroupDialog
#define form3                   UxModGroupDialogContext->Uxform3
#define label46                 UxModGroupDialogContext->Uxlabel46
#define pushButton4             UxModGroupDialogContext->UxpushButton4
#define pushButton9             UxModGroupDialogContext->UxpushButton9
#define pushButton12            UxModGroupDialogContext->UxpushButton12
#define pushButton5             UxModGroupDialogContext->UxpushButton5
#define pushButton6             UxModGroupDialogContext->UxpushButton6
#define separator1              UxModGroupDialogContext->Uxseparator1
#define textField22             UxModGroupDialogContext->UxtextField22
#define gidLabel                UxModGroupDialogContext->UxgidLabel
#define label47                 UxModGroupDialogContext->Uxlabel47
#define label34                 UxModGroupDialogContext->Uxlabel34
#define textField16             UxModGroupDialogContext->UxtextField16

#endif /* CONTEXT_MACRO_ACCESS */


/*******************************************************************************
       Declarations of global functions.
*******************************************************************************/

Widget	create_modGroupDialog(Widget parent);

#endif	/* _ADDGROUPDIALOG_INCLUDED */

