/* Copyright (c) 1993 Sun Microsystems, Inc. */

#pragma ident "@(#)add_user.h	1.8 95/10/04 Sun Microsystems"

/*******************************************************************************
       add_user.h

*******************************************************************************/

#ifndef	_ADDUSERDIALOG_INCLUDED
#define	_ADDUSERDIALOG_INCLUDED


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
	Widget	UxaddUserDialog;
	Widget	Uxform3;
	Widget	Uxlabel45;
	Widget	UxtextField22;
	Widget	Uxlabel46;
	Widget	UxtextField21;
	Widget	Uxlabel47;
	Widget	UxtextField20;
	Widget	Uxlabel48;
	Widget	UxtextField19;
	Widget	Uxlabel49;
	Widget	UxtextField18;
	Widget	Uxlabel50;
	Widget	Uxmenu1_p2;
	Widget	Uxmenu1_p1_b5;
	Widget	Uxmenu1_p1_b6;
	Widget	Uxmenu1_p1_b7;
	Widget	Uxmenu1_p1_b8;
	Widget	Uxmenu8;
	Widget	Uxlabel51;
	Widget	Uxlabel52;
	Widget	Uxmenu3_p2;
	Widget	Uxmenu3_p1_b5;
	Widget	Uxmenu3_p1_b6;
	Widget	Uxmenu3_p1_b7;
	Widget	Uxmenu3_p1_b8;
	Widget	Uxmenu9;
	Widget	Uxlabel61;
	Widget	Uxlabel62;
	Widget	Uxlabel63;
	Widget	UxtextField23;
	Widget	UxtextField24;
	Widget	UxtextField25;
	Widget	Uxlabel54;
	Widget	Uxlabel55;
	Widget	Uxlabel56;
	Widget	Uxlabel57;
	Widget	Uxlabel58;
	Widget	UxpushButton4;
	Widget	UxpushButton12;
	Widget	UxpushButton5;
	Widget	UxpushButton6;
	Widget	UxpushButton9;
	Widget	UxtextField35;
	Widget	Uxmenu4_p2;
	Widget	Uxmenu4_p1_b33;
	Widget	Uxmenu4_p1_b34;
	Widget	Uxmenu4_p1_b35;
	Widget	Uxmenu4_p1_b36;
	Widget	Uxmenu4_p1_b37;
	Widget	Uxmenu4_p1_b38;
	Widget	Uxmenu4_p1_b39;
	Widget	Uxmenu4_p1_b40;
	Widget	Uxmenu4_p1_b41;
	Widget	Uxmenu4_p1_b42;
	Widget	Uxmenu4_p1_b43;
	Widget	Uxmenu4_p1_b44;
	Widget	Uxmenu4_p1_b45;
	Widget	Uxmenu4_p1_b46;
	Widget	Uxmenu4_p1_b47;
	Widget	Uxmenu4_p1_b48;
	Widget	Uxmenu4_p1_b49;
	Widget	Uxmenu4_p1_b50;
	Widget	Uxmenu4_p1_b51;
	Widget	Uxmenu4_p1_b52;
	Widget	Uxmenu4_p1_b53;
	Widget	Uxmenu4_p1_b54;
	Widget	Uxmenu4_p1_b55;
	Widget	Uxmenu4_p1_b56;
	Widget	Uxmenu4_p1_b57;
	Widget	Uxmenu4_p1_b58;
	Widget	Uxmenu4_p1_b59;
	Widget	Uxmenu4_p1_b60;
	Widget	Uxmenu4_p1_b61;
	Widget	Uxmenu4_p1_b62;
	Widget	Uxmenu4_p1_b63;
	Widget	Uxmenu4_p1_b64;
	Widget	Uxmenu11;
	Widget	Uxmenu5_p4;
	Widget	Uxmenu5_p1_b36;
	Widget	Uxmenu5_p1_b37;
	Widget	Uxmenu5_p1_b38;
	Widget	Uxmenu5_p1_b39;
	Widget	Uxmenu5_p1_b40;
	Widget	Uxmenu5_p1_b41;
	Widget	Uxmenu5_p1_b42;
	Widget	Uxmenu5_p1_b43;
	Widget	Uxmenu5_p1_b44;
	Widget	Uxmenu5_p1_b45;
	Widget	Uxmenu5_p1_b46;
	Widget	Uxmenu5_p1_b47;
	Widget	Uxmenu5_p1_b48;
	Widget	Uxmenu10;
	Widget	Uxlabel64;
	Widget	UxtextField26;
	Widget	Uxlabel59;
	Widget	Uxlabel43;
	Widget	UxtoggleButton22;
	Widget	Uxlabel44;
	Widget	UxtextField14;
	Widget	Uxlabel60;
	Widget	UxtextField15;
	Widget	Uxlabel42;
	Widget	UxtextField16;
	Widget	Uxlabel34;
	Widget	UxtoggleButton12;
	Widget	Uxlabel35;
	Widget	Uxlabel3;
	Widget	UxtoggleButton15;
	Widget	UxtoggleButton14;
	Widget	UxtoggleButton13;
	Widget	Uxlabel37;
	Widget	UxtoggleButton16;
	Widget	UxtoggleButton17;
	Widget	UxtoggleButton18;
	Widget	Uxlabel38;
	Widget	UxtoggleButton19;
	Widget	UxtoggleButton20;
	Widget	UxtoggleButton21;
	Widget	Uxlabel39;
	Widget	Uxlabel40;
	Widget	UxtextField17;
	Widget	Uxlabel41;
	Widget	Uxseparator1;
	Widget	UxtoggleButton1;
	Widget	Uxlabel1;
	Widget	Uxmenu5_p3;
	Widget	Uxmenu5_p1_b25;
	Widget	Uxmenu5_p1_b26;
	Widget	Uxmenu5_p1_b27;
	Widget	Uxmenu5_p1_b28;
	Widget	Uxmenu5_p1_b29;
	Widget	Uxmenu5_p1_b30;
	Widget	Uxmenu5_p1_b31;
	Widget	Uxmenu5_p1_b32;
	Widget	Uxmenu5_p1_b33;
	Widget	Uxmenu5_p1_b34;
	Widget	Uxmenu5_p1_b35;
	Widget	Uxmenu7;
	Widget	Uxlabel53;
} _UxCaddUserDialog;

#ifdef CONTEXT_MACRO_ACCESS
static _UxCaddUserDialog       *UxAddUserDialogContext;
#define addUserDialog           UxAddUserDialogContext->UxaddUserDialog
#define form3                   UxAddUserDialogContext->Uxform3
#define label45                 UxAddUserDialogContext->Uxlabel45
#define textField22             UxAddUserDialogContext->UxtextField22
#define label46                 UxAddUserDialogContext->Uxlabel46
#define textField21             UxAddUserDialogContext->UxtextField21
#define label47                 UxAddUserDialogContext->Uxlabel47
#define textField20             UxAddUserDialogContext->UxtextField20
#define label48                 UxAddUserDialogContext->Uxlabel48
#define textField19             UxAddUserDialogContext->UxtextField19
#define label49                 UxAddUserDialogContext->Uxlabel49
#define textField18             UxAddUserDialogContext->UxtextField18
#define label50                 UxAddUserDialogContext->Uxlabel50
#define menu1_p2                UxAddUserDialogContext->Uxmenu1_p2
#define menu1_p1_b5             UxAddUserDialogContext->Uxmenu1_p1_b5
#define menu1_p1_b6             UxAddUserDialogContext->Uxmenu1_p1_b6
#define menu1_p1_b7             UxAddUserDialogContext->Uxmenu1_p1_b7
#define menu1_p1_b8             UxAddUserDialogContext->Uxmenu1_p1_b8
#define menu8                   UxAddUserDialogContext->Uxmenu8
#define label51                 UxAddUserDialogContext->Uxlabel51
#define label52                 UxAddUserDialogContext->Uxlabel52
#define menu3_p2                UxAddUserDialogContext->Uxmenu3_p2
#define menu3_p1_b5             UxAddUserDialogContext->Uxmenu3_p1_b5
#define menu3_p1_b6             UxAddUserDialogContext->Uxmenu3_p1_b6
#define menu3_p1_b7             UxAddUserDialogContext->Uxmenu3_p1_b7
#define menu3_p1_b8             UxAddUserDialogContext->Uxmenu3_p1_b8
#define menu9                   UxAddUserDialogContext->Uxmenu9
#define label61                 UxAddUserDialogContext->Uxlabel61
#define label62                 UxAddUserDialogContext->Uxlabel62
#define label63                 UxAddUserDialogContext->Uxlabel63
#define textField23             UxAddUserDialogContext->UxtextField23
#define textField24             UxAddUserDialogContext->UxtextField24
#define textField25             UxAddUserDialogContext->UxtextField25
#define label54                 UxAddUserDialogContext->Uxlabel54
#define label55                 UxAddUserDialogContext->Uxlabel55
#define label56                 UxAddUserDialogContext->Uxlabel56
#define label57                 UxAddUserDialogContext->Uxlabel57
#define label58                 UxAddUserDialogContext->Uxlabel58
#define pushButton4             UxAddUserDialogContext->UxpushButton4
#define pushButton12            UxAddUserDialogContext->UxpushButton12
#define pushButton5             UxAddUserDialogContext->UxpushButton5
#define pushButton6             UxAddUserDialogContext->UxpushButton6
#define pushButton9             UxAddUserDialogContext->UxpushButton9
#define textField35             UxAddUserDialogContext->UxtextField35
#define menu4_p2                UxAddUserDialogContext->Uxmenu4_p2
#define menu4_p1_b33            UxAddUserDialogContext->Uxmenu4_p1_b33
#define menu4_p1_b34            UxAddUserDialogContext->Uxmenu4_p1_b34
#define menu4_p1_b35            UxAddUserDialogContext->Uxmenu4_p1_b35
#define menu4_p1_b36            UxAddUserDialogContext->Uxmenu4_p1_b36
#define menu4_p1_b37            UxAddUserDialogContext->Uxmenu4_p1_b37
#define menu4_p1_b38            UxAddUserDialogContext->Uxmenu4_p1_b38
#define menu4_p1_b39            UxAddUserDialogContext->Uxmenu4_p1_b39
#define menu4_p1_b40            UxAddUserDialogContext->Uxmenu4_p1_b40
#define menu4_p1_b41            UxAddUserDialogContext->Uxmenu4_p1_b41
#define menu4_p1_b42            UxAddUserDialogContext->Uxmenu4_p1_b42
#define menu4_p1_b43            UxAddUserDialogContext->Uxmenu4_p1_b43
#define menu4_p1_b44            UxAddUserDialogContext->Uxmenu4_p1_b44
#define menu4_p1_b45            UxAddUserDialogContext->Uxmenu4_p1_b45
#define menu4_p1_b46            UxAddUserDialogContext->Uxmenu4_p1_b46
#define menu4_p1_b47            UxAddUserDialogContext->Uxmenu4_p1_b47
#define menu4_p1_b48            UxAddUserDialogContext->Uxmenu4_p1_b48
#define menu4_p1_b49            UxAddUserDialogContext->Uxmenu4_p1_b49
#define menu4_p1_b50            UxAddUserDialogContext->Uxmenu4_p1_b50
#define menu4_p1_b51            UxAddUserDialogContext->Uxmenu4_p1_b51
#define menu4_p1_b52            UxAddUserDialogContext->Uxmenu4_p1_b52
#define menu4_p1_b53            UxAddUserDialogContext->Uxmenu4_p1_b53
#define menu4_p1_b54            UxAddUserDialogContext->Uxmenu4_p1_b54
#define menu4_p1_b55            UxAddUserDialogContext->Uxmenu4_p1_b55
#define menu4_p1_b56            UxAddUserDialogContext->Uxmenu4_p1_b56
#define menu4_p1_b57            UxAddUserDialogContext->Uxmenu4_p1_b57
#define menu4_p1_b58            UxAddUserDialogContext->Uxmenu4_p1_b58
#define menu4_p1_b59            UxAddUserDialogContext->Uxmenu4_p1_b59
#define menu4_p1_b60            UxAddUserDialogContext->Uxmenu4_p1_b60
#define menu4_p1_b61            UxAddUserDialogContext->Uxmenu4_p1_b61
#define menu4_p1_b62            UxAddUserDialogContext->Uxmenu4_p1_b62
#define menu4_p1_b63            UxAddUserDialogContext->Uxmenu4_p1_b63
#define menu4_p1_b64            UxAddUserDialogContext->Uxmenu4_p1_b64
#define menu11                  UxAddUserDialogContext->Uxmenu11
#define menu5_p4                UxAddUserDialogContext->Uxmenu5_p4
#define menu5_p1_b36            UxAddUserDialogContext->Uxmenu5_p1_b36
#define menu5_p1_b37            UxAddUserDialogContext->Uxmenu5_p1_b37
#define menu5_p1_b38            UxAddUserDialogContext->Uxmenu5_p1_b38
#define menu5_p1_b39            UxAddUserDialogContext->Uxmenu5_p1_b39
#define menu5_p1_b40            UxAddUserDialogContext->Uxmenu5_p1_b40
#define menu5_p1_b41            UxAddUserDialogContext->Uxmenu5_p1_b41
#define menu5_p1_b42            UxAddUserDialogContext->Uxmenu5_p1_b42
#define menu5_p1_b43            UxAddUserDialogContext->Uxmenu5_p1_b43
#define menu5_p1_b44            UxAddUserDialogContext->Uxmenu5_p1_b44
#define menu5_p1_b45            UxAddUserDialogContext->Uxmenu5_p1_b45
#define menu5_p1_b46            UxAddUserDialogContext->Uxmenu5_p1_b46
#define menu5_p1_b47            UxAddUserDialogContext->Uxmenu5_p1_b47
#define menu5_p1_b48            UxAddUserDialogContext->Uxmenu5_p1_b48
#define menu10                  UxAddUserDialogContext->Uxmenu10
#define label64                 UxAddUserDialogContext->Uxlabel64
#define textField26             UxAddUserDialogContext->UxtextField26
#define label59                 UxAddUserDialogContext->Uxlabel59
#define label43                 UxAddUserDialogContext->Uxlabel43
#define toggleButton22          UxAddUserDialogContext->UxtoggleButton22
#define label44                 UxAddUserDialogContext->Uxlabel44
#define textField14             UxAddUserDialogContext->UxtextField14
#define label60                 UxAddUserDialogContext->Uxlabel60
#define textField15             UxAddUserDialogContext->UxtextField15
#define label42                 UxAddUserDialogContext->Uxlabel42
#define textField16             UxAddUserDialogContext->UxtextField16
#define label34                 UxAddUserDialogContext->Uxlabel34
#define toggleButton12          UxAddUserDialogContext->UxtoggleButton12
#define label35                 UxAddUserDialogContext->Uxlabel35
#define label3                  UxAddUserDialogContext->Uxlabel3
#define toggleButton15          UxAddUserDialogContext->UxtoggleButton15
#define toggleButton14          UxAddUserDialogContext->UxtoggleButton14
#define toggleButton13          UxAddUserDialogContext->UxtoggleButton13
#define label37                 UxAddUserDialogContext->Uxlabel37
#define toggleButton16          UxAddUserDialogContext->UxtoggleButton16
#define toggleButton17          UxAddUserDialogContext->UxtoggleButton17
#define toggleButton18          UxAddUserDialogContext->UxtoggleButton18
#define label38                 UxAddUserDialogContext->Uxlabel38
#define toggleButton19          UxAddUserDialogContext->UxtoggleButton19
#define toggleButton20          UxAddUserDialogContext->UxtoggleButton20
#define toggleButton21          UxAddUserDialogContext->UxtoggleButton21
#define label39                 UxAddUserDialogContext->Uxlabel39
#define label40                 UxAddUserDialogContext->Uxlabel40
#define textField17             UxAddUserDialogContext->UxtextField17
#define label41                 UxAddUserDialogContext->Uxlabel41
#define separator1              UxAddUserDialogContext->Uxseparator1
#define toggleButton1           UxAddUserDialogContext->UxtoggleButton1
#define label1                  UxAddUserDialogContext->Uxlabel1
#define menu5_p3                UxAddUserDialogContext->Uxmenu5_p3
#define menu5_p1_b25            UxAddUserDialogContext->Uxmenu5_p1_b25
#define menu5_p1_b26            UxAddUserDialogContext->Uxmenu5_p1_b26
#define menu5_p1_b27            UxAddUserDialogContext->Uxmenu5_p1_b27
#define menu5_p1_b28            UxAddUserDialogContext->Uxmenu5_p1_b28
#define menu5_p1_b29            UxAddUserDialogContext->Uxmenu5_p1_b29
#define menu5_p1_b30            UxAddUserDialogContext->Uxmenu5_p1_b30
#define menu5_p1_b31            UxAddUserDialogContext->Uxmenu5_p1_b31
#define menu5_p1_b32            UxAddUserDialogContext->Uxmenu5_p1_b32
#define menu5_p1_b33            UxAddUserDialogContext->Uxmenu5_p1_b33
#define menu5_p1_b34            UxAddUserDialogContext->Uxmenu5_p1_b34
#define menu5_p1_b35            UxAddUserDialogContext->Uxmenu5_p1_b35
#define menu7                   UxAddUserDialogContext->Uxmenu7
#define label53                 UxAddUserDialogContext->Uxlabel53

#endif /* CONTEXT_MACRO_ACCESS */


#define SHELL_C_PATH       "/bin/csh"
#define SHELL_BOURNE_PATH  "/bin/sh"
#define SHELL_KORN_PATH    "/bin/ksh"
#define SHELL_OTHER_PATH   "/bin/"

#define PASSWD_CLEARED	""
#define PASSWD_LOCKED	"*LK*"
#define PASSWD_NONE	"NP"
#define PASSWD_NORMAL	"--"

#define USR_ROOT_UNAME	  "root"  /* Root account definition */
#define USR_ROOT_UID	  "0"
#define USR_IS_ROOT(username, uid)	\
	(((username != NULL) && (strcmp(username, USR_ROOT_UNAME) == 0)) || \
	 ((uid != NULL) && (strcmp(uid, USR_ROOT_UID) == 0)))


/*******************************************************************************
       Declarations of global functions.
*******************************************************************************/

Widget	create_addUserDialog(Widget parent);

#endif	/* _ADDUSERDIALOG_INCLUDED */

