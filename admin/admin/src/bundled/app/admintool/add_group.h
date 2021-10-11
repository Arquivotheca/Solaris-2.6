/* Copyright (c) 1993 Sun Microsystems, Inc. */

#pragma ident "@(#)add_group.h	1.4 95/09/02 Sun Microsystems"



/*******************************************************************************
       add_group.h
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
	Widget	UxaddGroupDialog;
	Widget	Uxform3;
	Widget	Uxlabel46;
	Widget	UxpushButton4;
	Widget	UxpushButton9;
	Widget	UxpushButton12;
	Widget	UxpushButton5;
	Widget	UxpushButton6;
	Widget	Uxseparator1;
	Widget	UxtextField22;
	Widget	UxtextField21;
	Widget	Uxlabel47;
	Widget	Uxlabel34;
	Widget	UxtextField16;
} _UxCaddGroupDialog;

#ifdef CONTEXT_MACRO_ACCESS
static _UxCaddGroupDialog      *UxAddGroupDialogContext;
#define addGroupDialog          UxAddGroupDialogContext->UxaddGroupDialog
#define form3                   UxAddGroupDialogContext->Uxform3
#define label46                 UxAddGroupDialogContext->Uxlabel46
#define pushButton4             UxAddGroupDialogContext->UxpushButton4
#define pushButton9             UxAddGroupDialogContext->UxpushButton9
#define pushButton12            UxAddGroupDialogContext->UxpushButton12
#define pushButton5             UxAddGroupDialogContext->UxpushButton5
#define pushButton6             UxAddGroupDialogContext->UxpushButton6
#define separator1              UxAddGroupDialogContext->Uxseparator1
#define textField22             UxAddGroupDialogContext->UxtextField22
#define textField21             UxAddGroupDialogContext->UxtextField21
#define label47                 UxAddGroupDialogContext->Uxlabel47
#define label34                 UxAddGroupDialogContext->Uxlabel34
#define textField16             UxAddGroupDialogContext->UxtextField16

#endif /* CONTEXT_MACRO_ACCESS */


/*******************************************************************************
       Declarations of global functions.
*******************************************************************************/

Widget	create_addGroupDialog(Widget parent);

void    addGroupInit(
                Widget  wgt
        );
 

#endif	/* _ADDGROUPDIALOG_INCLUDED */

