/* Copyright (c) 1993 Sun Microsystems, Inc. */

#pragma ident "@(#)password.h	1.3 95/02/27 Sun Microsystems"

/*******************************************************************************
       password.h

*******************************************************************************/

#ifndef	_PASSWORDPROMPTDIALOG_INCLUDED
#define	_PASSWORDPROMPTDIALOG_INCLUDED


#include <stdio.h>
#include <Xm/Xm.h>
#include <Xm/DialogS.h>
#include <Xm/MenuShell.h>
#include "UxXt.h"

#include <Xm/TextF.h>
#include <Xm/Label.h>
#include <Xm/BulletinB.h>
#include <Xm/SelectioB.h>

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
	Widget	UxpasswordPromptDialog;
	Widget	UxmainForm;
	Widget	Uxlabel69;
	Widget	UxtextField32;
	Widget	Uxlabel77;
	Widget	UxtextField29;
} _UxCpasswordPromptDialog;

#ifdef CONTEXT_MACRO_ACCESS
static _UxCpasswordPromptDialog *UxPasswordPromptDialogContext;
#define passwordPromptDialog    UxPasswordPromptDialogContext->UxpasswordPromptDialog
#define mainForm		UxPasswordPromptDialogContext->UxmainForm
#define label69                 UxPasswordPromptDialogContext->Uxlabel69
#define textField32             UxPasswordPromptDialogContext->UxtextField32
#define label77                 UxPasswordPromptDialogContext->Uxlabel77
#define textField29             UxPasswordPromptDialogContext->UxtextField29

#endif /* CONTEXT_MACRO_ACCESS */


/*******************************************************************************
       Declarations of global functions.
*******************************************************************************/

Widget	create_passwordPromptDialog( void );

#endif	/* _PASSWORDPROMPTDIALOG_INCLUDED */

