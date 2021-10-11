/* Copyright (c) 1993 Sun Microsystems, Inc. */

#pragma ident "@(#)modify_user.h	1.4 95/02/27 Sun Microsystems"

/*******************************************************************************
       modifyUserDialog.hh
       This header file is included by modifyUserDialog.cc

*******************************************************************************/

#ifndef	_MODIFYUSERDIALOG_INCLUDED
#define	_MODIFYUSERDIALOG_INCLUDED


#include <stdio.h>
#include <Xm/Xm.h>
#include <Xm/MenuShell.h>
#include "UxXt.h"

#include <Xm/Separator.h>
#include <Xm/ToggleB.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/TextF.h>
#include <Xm/Text.h>
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
	Widget	UxmodifyUserDialog;
	Widget	Uxform2;
	Widget	Uxlabel4;
	Widget	Uxtext1;
	Widget	UxtextField1;
	Widget	Uxlabel5;
	Widget	Uxlabel99;
	Widget	Uxlabel6;
	Widget	UxtextField3;
	Widget	Uxlabel7;
	Widget	UxtextField4;
	Widget	Uxlabel8;
	Widget	UxtextField5;
	Widget	Uxlabel9;
	Widget	Uxmenu1_p1;
	Widget	Uxmenu1_p1_b1;
	Widget	Uxmenu1_p1_b2;
	Widget	Uxmenu1_p1_b3;
	Widget	Uxmenu1_p1_b4;
	Widget	Uxmenu1;
	Widget	Uxlabel11;
	Widget	Uxlabel12;
	Widget	Uxmenu3_p1;
	Widget	Uxmenu3_p1_b1;
	Widget	Uxmenu3_p1_b2;
	Widget	Uxmenu3_p1_b3;
	Widget	Uxmenu3_p1_b4;
	Widget	Uxmenu3;
	Widget	Uxlabel13;
	Widget	Uxlabel14;
	Widget	Uxlabel15;
	Widget	UxtextField6;
	Widget	UxtextField7;
	Widget	UxtextField8;
	Widget	Uxlabel16;
	Widget	Uxlabel17;
	Widget	Uxlabel18;
	Widget	Uxlabel19;
	Widget	Uxmenu4_p1;
	Widget	Uxmenu4_p1_b1;
	Widget	Uxmenu4_p1_b2;
	Widget	Uxmenu4_p1_b3;
	Widget	Uxmenu4_p1_b4;
	Widget	Uxmenu4_p1_b5;
	Widget	Uxmenu4_p1_b6;
	Widget	Uxmenu4_p1_b7;
	Widget	Uxmenu4_p1_b8;
	Widget	Uxmenu4_p1_b9;
	Widget	Uxmenu4_p1_b10;
	Widget	Uxmenu4_p1_b11;
	Widget	Uxmenu4_p1_b12;
	Widget	Uxmenu4_p1_b13;
	Widget	Uxmenu4_p1_b14;
	Widget	Uxmenu4_p1_b15;
	Widget	Uxmenu4_p1_b16;
	Widget	Uxmenu4_p1_b17;
	Widget	Uxmenu4_p1_b18;
	Widget	Uxmenu4_p1_b19;
	Widget	Uxmenu4_p1_b20;
	Widget	Uxmenu4_p1_b21;
	Widget	Uxmenu4_p1_b22;
	Widget	Uxmenu4_p1_b23;
	Widget	Uxmenu4_p1_b24;
	Widget	Uxmenu4_p1_b25;
	Widget	Uxmenu4_p1_b26;
	Widget	Uxmenu4_p1_b27;
	Widget	Uxmenu4_p1_b28;
	Widget	Uxmenu4_p1_b29;
	Widget	Uxmenu4_p1_b30;
	Widget	Uxmenu4_p1_b31;
	Widget	Uxmenu4_p1_b32;
	Widget	Uxmenu6;
	Widget	Uxlabel20;
	Widget	UxtextField36;
	Widget	UxpushButton1;
	Widget	Uxlabel10;
	Widget	Uxmenu5_p2;
	Widget	Uxmenu5_p1_b12;
	Widget	Uxmenu5_p1_b13;
	Widget	Uxmenu5_p1_b14;
	Widget	Uxmenu5_p1_b15;
	Widget	Uxmenu5_p1_b16;
	Widget	Uxmenu5_p1_b17;
	Widget	Uxmenu5_p1_b18;
	Widget	Uxmenu5_p1_b19;
	Widget	Uxmenu5_p1_b20;
	Widget	Uxmenu5_p1_b21;
	Widget	Uxmenu5_p1_b22;
	Widget	Uxmenu5_p1_b23;
	Widget	Uxmenu5_p1_b24;
	Widget	Uxmenu5;
	Widget	Uxlabel21;
	Widget	UxtextField9;
	Widget	Uxlabel22;
	Widget	Uxlabel23;
	Widget	UxtextField10;
	Widget	Uxlabel25;
	Widget	Uxlabel98;
	Widget	Uxlabel26;
	Widget	UxtoggleButton2;
	Widget	Uxlabel28;
	Widget	Uxlabel29;
	Widget	UxtoggleButton3;
	Widget	UxtoggleButton4;
	Widget	UxtoggleButton5;
	Widget	Uxlabel30;
	Widget	UxtoggleButton6;
	Widget	UxtoggleButton7;
	Widget	UxtoggleButton8;
	Widget	Uxlabel31;
	Widget	UxtoggleButton9;
	Widget	UxtoggleButton10;
	Widget	UxtoggleButton11;
	Widget	Uxlabel32;
	Widget	Uxlabel33;
	Widget	UxtextField13;
	Widget	Uxlabel36;
	Widget	Uxseparator2;
	Widget	UxpushButton4;
	Widget	UxpushButton5;
	Widget	UxpushButton6;
	Widget	UxpushButton10;
	Widget	UxtoggleButton23;
	Widget	Uxlabel73;
	Widget	Uxmenu5_p1;
	Widget	Uxmenu5_p1_b1;
	Widget	Uxmenu5_p1_b2;
	Widget	Uxmenu5_p1_b3;
	Widget	Uxmenu5_p1_b4;
	Widget	Uxmenu5_p1_b5;
	Widget	Uxmenu5_p1_b6;
	Widget	Uxmenu5_p1_b7;
	Widget	Uxmenu5_p1_b8;
	Widget	Uxmenu5_p1_b9;
	Widget	Uxmenu5_p1_b10;
	Widget	Uxmenu5_p1_b11;
	Widget	Uxmenu4;

	SysmanUserArg	user;
} _UxCmodifyUserDialog;

#ifdef CONTEXT_MACRO_ACCESS
static _UxCmodifyUserDialog    *UxModifyUserDialogContext;
#define modifyUserDialog        UxModifyUserDialogContext->UxmodifyUserDialog
#define form2                   UxModifyUserDialogContext->Uxform2
#define label4                  UxModifyUserDialogContext->Uxlabel4
#define text1                   UxModifyUserDialogContext->Uxtext1
#define textField1              UxModifyUserDialogContext->UxtextField1
#define label5                  UxModifyUserDialogContext->Uxlabel5
#define label99              	UxModifyUserDialogContext->Uxlabel99
#define label6                  UxModifyUserDialogContext->Uxlabel6
#define textField3              UxModifyUserDialogContext->UxtextField3
#define label7                  UxModifyUserDialogContext->Uxlabel7
#define textField4              UxModifyUserDialogContext->UxtextField4
#define label8                  UxModifyUserDialogContext->Uxlabel8
#define textField5              UxModifyUserDialogContext->UxtextField5
#define label9                  UxModifyUserDialogContext->Uxlabel9
#define menu1_p1                UxModifyUserDialogContext->Uxmenu1_p1
#define menu1_p1_b1             UxModifyUserDialogContext->Uxmenu1_p1_b1
#define menu1_p1_b2             UxModifyUserDialogContext->Uxmenu1_p1_b2
#define menu1_p1_b3             UxModifyUserDialogContext->Uxmenu1_p1_b3
#define menu1_p1_b4             UxModifyUserDialogContext->Uxmenu1_p1_b4
#define menu1                   UxModifyUserDialogContext->Uxmenu1
#define label11                 UxModifyUserDialogContext->Uxlabel11
#define label12                 UxModifyUserDialogContext->Uxlabel12
#define menu3_p1                UxModifyUserDialogContext->Uxmenu3_p1
#define menu3_p1_b1             UxModifyUserDialogContext->Uxmenu3_p1_b1
#define menu3_p1_b2             UxModifyUserDialogContext->Uxmenu3_p1_b2
#define menu3_p1_b3             UxModifyUserDialogContext->Uxmenu3_p1_b3
#define menu3_p1_b4             UxModifyUserDialogContext->Uxmenu3_p1_b4
#define menu3                   UxModifyUserDialogContext->Uxmenu3
#define label13                 UxModifyUserDialogContext->Uxlabel13
#define label14                 UxModifyUserDialogContext->Uxlabel14
#define label15                 UxModifyUserDialogContext->Uxlabel15
#define textField6              UxModifyUserDialogContext->UxtextField6
#define textField7              UxModifyUserDialogContext->UxtextField7
#define textField8              UxModifyUserDialogContext->UxtextField8
#define label16                 UxModifyUserDialogContext->Uxlabel16
#define label17                 UxModifyUserDialogContext->Uxlabel17
#define label18                 UxModifyUserDialogContext->Uxlabel18
#define label19                 UxModifyUserDialogContext->Uxlabel19
#define menu4_p1                UxModifyUserDialogContext->Uxmenu4_p1
#define menu4_p1_b1             UxModifyUserDialogContext->Uxmenu4_p1_b1
#define menu4_p1_b2             UxModifyUserDialogContext->Uxmenu4_p1_b2
#define menu4_p1_b3             UxModifyUserDialogContext->Uxmenu4_p1_b3
#define menu4_p1_b4             UxModifyUserDialogContext->Uxmenu4_p1_b4
#define menu4_p1_b5             UxModifyUserDialogContext->Uxmenu4_p1_b5
#define menu4_p1_b6             UxModifyUserDialogContext->Uxmenu4_p1_b6
#define menu4_p1_b7             UxModifyUserDialogContext->Uxmenu4_p1_b7
#define menu4_p1_b8             UxModifyUserDialogContext->Uxmenu4_p1_b8
#define menu4_p1_b9             UxModifyUserDialogContext->Uxmenu4_p1_b9
#define menu4_p1_b10            UxModifyUserDialogContext->Uxmenu4_p1_b10
#define menu4_p1_b11            UxModifyUserDialogContext->Uxmenu4_p1_b11
#define menu4_p1_b12            UxModifyUserDialogContext->Uxmenu4_p1_b12
#define menu4_p1_b13            UxModifyUserDialogContext->Uxmenu4_p1_b13
#define menu4_p1_b14            UxModifyUserDialogContext->Uxmenu4_p1_b14
#define menu4_p1_b15            UxModifyUserDialogContext->Uxmenu4_p1_b15
#define menu4_p1_b16            UxModifyUserDialogContext->Uxmenu4_p1_b16
#define menu4_p1_b17            UxModifyUserDialogContext->Uxmenu4_p1_b17
#define menu4_p1_b18            UxModifyUserDialogContext->Uxmenu4_p1_b18
#define menu4_p1_b19            UxModifyUserDialogContext->Uxmenu4_p1_b19
#define menu4_p1_b20            UxModifyUserDialogContext->Uxmenu4_p1_b20
#define menu4_p1_b21            UxModifyUserDialogContext->Uxmenu4_p1_b21
#define menu4_p1_b22            UxModifyUserDialogContext->Uxmenu4_p1_b22
#define menu4_p1_b23            UxModifyUserDialogContext->Uxmenu4_p1_b23
#define menu4_p1_b24            UxModifyUserDialogContext->Uxmenu4_p1_b24
#define menu4_p1_b25            UxModifyUserDialogContext->Uxmenu4_p1_b25
#define menu4_p1_b26            UxModifyUserDialogContext->Uxmenu4_p1_b26
#define menu4_p1_b27            UxModifyUserDialogContext->Uxmenu4_p1_b27
#define menu4_p1_b28            UxModifyUserDialogContext->Uxmenu4_p1_b28
#define menu4_p1_b29            UxModifyUserDialogContext->Uxmenu4_p1_b29
#define menu4_p1_b30            UxModifyUserDialogContext->Uxmenu4_p1_b30
#define menu4_p1_b31            UxModifyUserDialogContext->Uxmenu4_p1_b31
#define menu4_p1_b32            UxModifyUserDialogContext->Uxmenu4_p1_b32
#define menu6                   UxModifyUserDialogContext->Uxmenu6
#define label20                 UxModifyUserDialogContext->Uxlabel20
#define textField36             UxModifyUserDialogContext->UxtextField36
#define pushButton1             UxModifyUserDialogContext->UxpushButton1
#define label10                 UxModifyUserDialogContext->Uxlabel10
#define menu5_p2                UxModifyUserDialogContext->Uxmenu5_p2
#define menu5_p1_b12            UxModifyUserDialogContext->Uxmenu5_p1_b12
#define menu5_p1_b13            UxModifyUserDialogContext->Uxmenu5_p1_b13
#define menu5_p1_b14            UxModifyUserDialogContext->Uxmenu5_p1_b14
#define menu5_p1_b15            UxModifyUserDialogContext->Uxmenu5_p1_b15
#define menu5_p1_b16            UxModifyUserDialogContext->Uxmenu5_p1_b16
#define menu5_p1_b17            UxModifyUserDialogContext->Uxmenu5_p1_b17
#define menu5_p1_b18            UxModifyUserDialogContext->Uxmenu5_p1_b18
#define menu5_p1_b19            UxModifyUserDialogContext->Uxmenu5_p1_b19
#define menu5_p1_b20            UxModifyUserDialogContext->Uxmenu5_p1_b20
#define menu5_p1_b21            UxModifyUserDialogContext->Uxmenu5_p1_b21
#define menu5_p1_b22            UxModifyUserDialogContext->Uxmenu5_p1_b22
#define menu5_p1_b23            UxModifyUserDialogContext->Uxmenu5_p1_b23
#define menu5_p1_b24            UxModifyUserDialogContext->Uxmenu5_p1_b24
#define menu5                   UxModifyUserDialogContext->Uxmenu5
#define label21                 UxModifyUserDialogContext->Uxlabel21
#define textField9              UxModifyUserDialogContext->UxtextField9
#define label22                 UxModifyUserDialogContext->Uxlabel22
#define label23                 UxModifyUserDialogContext->Uxlabel23
#define textField10             UxModifyUserDialogContext->UxtextField10
#define label25                 UxModifyUserDialogContext->Uxlabel25
#define label98             UxModifyUserDialogContext->Uxlabel98
#define label26                 UxModifyUserDialogContext->Uxlabel26
#define toggleButton2           UxModifyUserDialogContext->UxtoggleButton2
#define label28                 UxModifyUserDialogContext->Uxlabel28
#define label29                 UxModifyUserDialogContext->Uxlabel29
#define toggleButton3           UxModifyUserDialogContext->UxtoggleButton3
#define toggleButton4           UxModifyUserDialogContext->UxtoggleButton4
#define toggleButton5           UxModifyUserDialogContext->UxtoggleButton5
#define label30                 UxModifyUserDialogContext->Uxlabel30
#define toggleButton6           UxModifyUserDialogContext->UxtoggleButton6
#define toggleButton7           UxModifyUserDialogContext->UxtoggleButton7
#define toggleButton8           UxModifyUserDialogContext->UxtoggleButton8
#define label31                 UxModifyUserDialogContext->Uxlabel31
#define toggleButton9           UxModifyUserDialogContext->UxtoggleButton9
#define toggleButton10          UxModifyUserDialogContext->UxtoggleButton10
#define toggleButton11          UxModifyUserDialogContext->UxtoggleButton11
#define label32                 UxModifyUserDialogContext->Uxlabel32
#define label33                 UxModifyUserDialogContext->Uxlabel33
#define textField13             UxModifyUserDialogContext->UxtextField13
#define label36                 UxModifyUserDialogContext->Uxlabel36
#define separator2              UxModifyUserDialogContext->Uxseparator2
#define pushButton4             UxModifyUserDialogContext->UxpushButton4
#define pushButton5             UxModifyUserDialogContext->UxpushButton5
#define pushButton6             UxModifyUserDialogContext->UxpushButton6
#define pushButton10             UxModifyUserDialogContext->UxpushButton10
#define toggleButton23          UxModifyUserDialogContext->UxtoggleButton23
#define label73                 UxModifyUserDialogContext->Uxlabel73
#define menu5_p1                UxModifyUserDialogContext->Uxmenu5_p1
#define menu5_p1_b1             UxModifyUserDialogContext->Uxmenu5_p1_b1
#define menu5_p1_b2             UxModifyUserDialogContext->Uxmenu5_p1_b2
#define menu5_p1_b3             UxModifyUserDialogContext->Uxmenu5_p1_b3
#define menu5_p1_b4             UxModifyUserDialogContext->Uxmenu5_p1_b4
#define menu5_p1_b5             UxModifyUserDialogContext->Uxmenu5_p1_b5
#define menu5_p1_b6             UxModifyUserDialogContext->Uxmenu5_p1_b6
#define menu5_p1_b7             UxModifyUserDialogContext->Uxmenu5_p1_b7
#define menu5_p1_b8             UxModifyUserDialogContext->Uxmenu5_p1_b8
#define menu5_p1_b9             UxModifyUserDialogContext->Uxmenu5_p1_b9
#define menu5_p1_b10            UxModifyUserDialogContext->Uxmenu5_p1_b10
#define menu5_p1_b11            UxModifyUserDialogContext->Uxmenu5_p1_b11
#define menu4                   UxModifyUserDialogContext->Uxmenu4

#endif /* CONTEXT_MACRO_ACCESS */


/*******************************************************************************
       Declarations of global functions.
*******************************************************************************/

Widget	create_modifyUserDialog(Widget UxParent);

#endif	/* _MODIFYUSERDIALOG_INCLUDED */

