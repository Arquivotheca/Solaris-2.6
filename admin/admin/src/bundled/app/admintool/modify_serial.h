/* Copyright (c) 1993 Sun Microsystems, Inc. */

#pragma ident "@(#)modify_serial.h	1.5 95/02/27 Sun Microsystems"

/*******************************************************************************
       modify_serial.h

*******************************************************************************/

#ifndef	_MODIFYDIALOG_INCLUDED
#define	_MODIFYDIALOG_INCLUDED


#include <stdio.h>
#include <Xm/Xm.h>
#include <Xm/DialogS.h>
#include <Xm/MenuShell.h>
#include "UxXt.h"

#include <Xm/PushB.h>
#include <Xm/LabelG.h>
#include <Xm/TextF.h>
#include <Xm/SeparatoG.h>
#include <Xm/ToggleBG.h>
#include <Xm/Label.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/Form.h>
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
	Widget	UxmodifyDialog;
	Widget	Uxform2;
	Widget	Uxmenu1_p1;
	Widget	Uxmenu1_p1_b1;
	Widget	Uxmenu1_p1_b2;
	Widget	Uxmenu1_p1_b3;
	Widget	Uxmenu1_p1_b4;
	Widget	Uxmenu1_p1_b5;
	Widget	Uxmenu1;
	Widget	Uxlabel5;
	Widget	UxrowColumn1;
	Widget	UxtoggleButtonGadget1;
	Widget	UxtoggleButtonGadget2;
	Widget	UxtoggleButtonGadget3;
	Widget	UxseparatorGadget1;
	Widget	Uxform5;
	Widget	Uxlabel6;
	Widget	Uxlabel7;
	Widget	UxseparatorGadget2;
	Widget	UxtoggleButtonGadget4;
	Widget	Uxmenu3_p1;
	Widget	Uxmenu3_p1_b1;
	Widget	Uxmenu3_p1_b2;
	Widget	Uxmenu3_p1_b3;
	Widget	Uxmenu3_p1_b4;
	Widget	Uxmenu3_p1_b5;
	Widget	Uxmenu3_p1_b6;
	Widget	Uxmenu3_p1_b7;
	Widget	Uxmenu3_p1_b8;
	Widget	Uxmenu3;
	Widget	Uxlabel8;
	Widget	UxtextField1;
	Widget	Uxform6;
	Widget	UxlabelGadget1;
	Widget	UxtoggleButtonGadget5;
	Widget	UxtoggleButtonGadget6;
	Widget	UxtoggleButtonGadget7;
	Widget	UxlabelGadget2;
	Widget	UxseparatorGadget3;
	Widget	UxtextField2;
	Widget	UxlabelGadget3;
	Widget	UxtextField3;
	Widget	UxlabelGadget4;
	Widget	Uxlabel9;
	Widget	Uxmenu3_p2;
	Widget	Uxmenu3_p1_b9;
	Widget	Uxmenu3_p1_b10;
	Widget	Uxmenu4;
	Widget	Uxform7;
	Widget	UxlabelGadget5;
	Widget	UxtoggleButtonGadget8;
	Widget	UxtoggleButtonGadget9;
	Widget	UxlabelGadget6;
	Widget	UxseparatorGadget4;
	Widget	UxtextField4;
	Widget	UxlabelGadget7;
	Widget	UxtextField5;
	Widget	Uxmenu3_p3;
	Widget	Uxmenu3_p1_b11;
	Widget	Uxmenu3_p1_b12;
	Widget	Uxmenu3_p3_b3;
	Widget	Uxmenu3_p3_b4;
	Widget	Uxmenu5;
	Widget	Uxform8;
	Widget	UxpushButton2;
	Widget	UxpushButton3;
	Widget	UxpushButton9;
	Widget	UxpushButton10;
	Widget	UxpushButton11;
	swidget	UxUxParent;

	SysmanSerialArg serial;
} _UxCmodifyDialog;

#ifdef CONTEXT_MACRO_ACCESS
static _UxCmodifyDialog        *UxModifyDialogContext;
#define modifyDialog            UxModifyDialogContext->UxmodifyDialog
#define form2                   UxModifyDialogContext->Uxform2
#define menu1_p1                UxModifyDialogContext->Uxmenu1_p1
#define menu1_p1_b1             UxModifyDialogContext->Uxmenu1_p1_b1
#define menu1_p1_b2             UxModifyDialogContext->Uxmenu1_p1_b2
#define menu1_p1_b3             UxModifyDialogContext->Uxmenu1_p1_b3
#define menu1_p1_b4             UxModifyDialogContext->Uxmenu1_p1_b4
#define menu1_p1_b5             UxModifyDialogContext->Uxmenu1_p1_b5
#define menu1                   UxModifyDialogContext->Uxmenu1
#define label5                  UxModifyDialogContext->Uxlabel5
#define rowColumn1              UxModifyDialogContext->UxrowColumn1
#define toggleButtonGadget1     UxModifyDialogContext->UxtoggleButtonGadget1
#define toggleButtonGadget2     UxModifyDialogContext->UxtoggleButtonGadget2
#define toggleButtonGadget3     UxModifyDialogContext->UxtoggleButtonGadget3
#define separatorGadget1        UxModifyDialogContext->UxseparatorGadget1
#define form5                   UxModifyDialogContext->Uxform5
#define label6                  UxModifyDialogContext->Uxlabel6
#define label7                  UxModifyDialogContext->Uxlabel7
#define separatorGadget2        UxModifyDialogContext->UxseparatorGadget2
#define toggleButtonGadget4     UxModifyDialogContext->UxtoggleButtonGadget4
#define menu3_p1                UxModifyDialogContext->Uxmenu3_p1
#define menu3_p1_b1             UxModifyDialogContext->Uxmenu3_p1_b1
#define menu3_p1_b2             UxModifyDialogContext->Uxmenu3_p1_b2
#define menu3_p1_b3             UxModifyDialogContext->Uxmenu3_p1_b3
#define menu3_p1_b4             UxModifyDialogContext->Uxmenu3_p1_b4
#define menu3_p1_b5             UxModifyDialogContext->Uxmenu3_p1_b5
#define menu3_p1_b6             UxModifyDialogContext->Uxmenu3_p1_b6
#define menu3_p1_b7             UxModifyDialogContext->Uxmenu3_p1_b7
#define menu3_p1_b8             UxModifyDialogContext->Uxmenu3_p1_b8
#define menu3                   UxModifyDialogContext->Uxmenu3
#define label8                  UxModifyDialogContext->Uxlabel8
#define textField1              UxModifyDialogContext->UxtextField1
#define form6                   UxModifyDialogContext->Uxform6
#define labelGadget1            UxModifyDialogContext->UxlabelGadget1
#define toggleButtonGadget5     UxModifyDialogContext->UxtoggleButtonGadget5
#define toggleButtonGadget6     UxModifyDialogContext->UxtoggleButtonGadget6
#define toggleButtonGadget7     UxModifyDialogContext->UxtoggleButtonGadget7
#define labelGadget2            UxModifyDialogContext->UxlabelGadget2
#define separatorGadget3        UxModifyDialogContext->UxseparatorGadget3
#define textField2              UxModifyDialogContext->UxtextField2
#define labelGadget3            UxModifyDialogContext->UxlabelGadget3
#define textField3              UxModifyDialogContext->UxtextField3
#define labelGadget4            UxModifyDialogContext->UxlabelGadget4
#define label9                  UxModifyDialogContext->Uxlabel9
#define menu3_p2                UxModifyDialogContext->Uxmenu3_p2
#define menu3_p1_b9             UxModifyDialogContext->Uxmenu3_p1_b9
#define menu3_p1_b10            UxModifyDialogContext->Uxmenu3_p1_b10
#define menu4                   UxModifyDialogContext->Uxmenu4
#define form7                   UxModifyDialogContext->Uxform7
#define labelGadget5            UxModifyDialogContext->UxlabelGadget5
#define toggleButtonGadget8     UxModifyDialogContext->UxtoggleButtonGadget8
#define toggleButtonGadget9     UxModifyDialogContext->UxtoggleButtonGadget9
#define labelGadget6            UxModifyDialogContext->UxlabelGadget6
#define separatorGadget4        UxModifyDialogContext->UxseparatorGadget4
#define textField4              UxModifyDialogContext->UxtextField4
#define labelGadget7            UxModifyDialogContext->UxlabelGadget7
#define textField5              UxModifyDialogContext->UxtextField5
#define menu3_p3                UxModifyDialogContext->Uxmenu3_p3
#define menu3_p1_b11            UxModifyDialogContext->Uxmenu3_p1_b11
#define menu3_p1_b12            UxModifyDialogContext->Uxmenu3_p1_b12
#define menu3_p3_b3             UxModifyDialogContext->Uxmenu3_p3_b3
#define menu3_p3_b4             UxModifyDialogContext->Uxmenu3_p3_b4
#define menu5                   UxModifyDialogContext->Uxmenu5
#define form8                   UxModifyDialogContext->Uxform8
#define pushButton2             UxModifyDialogContext->UxpushButton2
#define pushButton3             UxModifyDialogContext->UxpushButton3
#define pushButton9             UxModifyDialogContext->UxpushButton9
#define pushButton10            UxModifyDialogContext->UxpushButton10
#define pushButton11            UxModifyDialogContext->UxpushButton11
#define UxParent                UxModifyDialogContext->UxUxParent

#endif /* CONTEXT_MACRO_ACCESS */


/*******************************************************************************
       Declarations of global functions.
*******************************************************************************/

Widget	create_modifyDialog( swidget _UxUxParent );

#endif	/* _MODIFYDIALOG_INCLUDED */

