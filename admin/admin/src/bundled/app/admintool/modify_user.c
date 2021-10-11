/* Copyright (c) 1995 Sun Microsystems, Inc. */
/* All rights reserved. */

#pragma ident "@(#)modify_user.c	1.27 96/06/25 Sun Microsystems"

/*******************************************************************************
	modify_user.c

*******************************************************************************/

#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <libintl.h>
#include <nl_types.h>
#include <Xm/Xm.h>
#include <Xm/MenuShell.h>
#include <Xm/SelectioB.h>
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
#include <Xm/MessageB.h>
#include <Xm/List.h>

#include "UxXt.h"
#include "util.h"
#include "add_user.h"
#include "sysman_iface.h"
#include "sysman_codes.h"

#define EQSTR(A,B)  (strcmp((A),(B)) == 0)

#define ROOT_MODIFY_NOTICE_MSG		\
	catgets(_catd, 8, 374, "Admintool cannot be used to modify a user account with user id 0 \n" \
	"or user name root.")

#define ROOT_MODIFY_CAUTION_MSG	\
	catgets(_catd, 8, 375, "Admintool cannot be used to modify a user account with user id 0 \n" \
	"or user name root.")

Widget			ModOrigPassSetting;

void modifyUserLoginShellCCB(
		Widget widget, 
		XtPointer, 
		XtPointer
	);
void modifyUserLoginShellBourneCB(
		Widget widget, 
		XtPointer, 
		XtPointer
	);
void modifyUserLoginShellKornCB(
		Widget widget, 
		XtPointer, 
		XtPointer
	);

extern nl_catd	_catd;	/* for catgets(), defined in main.c */
extern int	day_limit[];

Widget 		modifyuserdialog = NULL;
char		ModifyPassword[PASS_MAX + 1];
int		UserEnteredModPassword;


/*******************************************************************************
       The following header file defines the context structure.
*******************************************************************************/

#define CONTEXT_MACRO_ACCESS 1
#include "modify_user.h"
#undef CONTEXT_MACRO_ACCESS


/*******************************************************************************
       The following are Auxiliary functions.
*******************************************************************************/
int
modifyUserInit(
	Widget widget,
	SysmanUserArg*	user
)
{
	_UxCmodifyUserDialog		*modifyUserWin;
	Widget				password_item_value;
	int				posCount;
	int				*posPtr;
	int				day=0;
	int				month=0;
	int				year=0;
	int				status;
	const char*			shell;
	const char*			passwd;
	char				*db_err;
	XmString			xstr;
	int				sts;


	modifyUserWin = (_UxCmodifyUserDialog *) UxGetContext(widget);

	UserEnteredModPassword = FALSE;
	ModifyPassword[0] = NULL;

	user->get_shadow_flag = TRUE;
	sts = sysman_get_user(user, errbuf, ERRBUF_SIZE);

	if (sts != 0) {
		display_error(modifyuserdialog, errbuf);
		return 0;
	}

	XmTextSetString(modifyUserWin->UxtextField1,
		user->username ? (char*)user->username : "");

	xstr = XmStringCreateLocalized((char*)user->uid);
	XtVaSetValues(modifyUserWin->Uxlabel99,
		XmNlabelString, xstr,
		NULL);
	XmStringFree(xstr);

	XmTextSetString(modifyUserWin->UxtextField3,
		user->group ? (char*)user->group : "");
	XmTextSetString(modifyUserWin->UxtextField4,
		user->second_grps ? (char*)user->second_grps : "");
	XmTextSetString(modifyUserWin->UxtextField5,
		user->comment ? (char*)user->comment : "");

	shell = (user->shell ? user->shell : "");
	if (EQSTR(shell, SHELL_C_PATH)) {
		XtVaSetValues(modifyUserWin->Uxmenu1, 
			XmNmenuHistory, modifyUserWin->Uxmenu1_p1_b2,
			NULL);
		modifyUserLoginShellCCB(modifyUserWin->Uxmenu1_p1_b2, NULL, NULL);
	}
	else if (EQSTR(shell, SHELL_BOURNE_PATH) ||
		 EQSTR(shell, "")) {
		XtVaSetValues(modifyUserWin->Uxmenu1, 
			XmNmenuHistory, modifyUserWin->Uxmenu1_p1_b1,
			NULL);
		modifyUserLoginShellBourneCB(modifyUserWin->Uxmenu1_p1_b1, NULL, NULL);
	}
	else if (EQSTR(shell, SHELL_KORN_PATH)) {
		XtVaSetValues(modifyUserWin->Uxmenu1, 
			XmNmenuHistory, modifyUserWin->Uxmenu1_p1_b3,
			NULL);
		modifyUserLoginShellKornCB(modifyUserWin->Uxmenu1_p1_b3, NULL, NULL);
	}
	else {
		XtVaSetValues(modifyUserWin->Uxmenu1, 
			XmNmenuHistory, modifyUserWin->Uxmenu1_p1_b4,
			NULL);
		XtVaSetValues(modifyUserWin->UxtextField36, 
			XmNmappedWhenManaged, TRUE,
			NULL);
		XmTextSetString(modifyUserWin->UxtextField36, (char*)shell);
		XmTextFieldSetInsertionPosition(modifyUserWin->UxtextField36,
			strlen(shell)); 

		XtVaSetValues(modifyUserWin->Uxlabel10, 
			XmNmappedWhenManaged, FALSE,
			NULL);
	}

	passwd = (user->passwd ? user->passwd : "");
	if (strcmp(passwd, PASSWD_CLEARED) == 0) {
		password_item_value = modifyUserWin->Uxmenu3_p1_b1;
	}
	else if (strcmp(passwd, PASSWD_LOCKED) == 0) {
		password_item_value = modifyUserWin->Uxmenu3_p1_b2;
	}
	else if (strcmp(passwd, PASSWD_NONE) == 0) {
		password_item_value = modifyUserWin->Uxmenu3_p1_b3;
	}
	else {
		password_item_value = modifyUserWin->Uxmenu3_p1_b4;
	}
	XtVaSetValues(modifyUserWin->Uxmenu3, 
		XmNmenuHistory, password_item_value,
		XmNuserData, password_item_value,
		NULL);

	ModOrigPassSetting = password_item_value;
	XmTextSetString(modifyUserWin->UxtextField6,
		user->minimum ? (char*)user->minimum : "");
	XmTextSetString(modifyUserWin->UxtextField7,
		user->maximum ? (char*)user->maximum : "");
	XmTextSetString(modifyUserWin->UxtextField8,
		user->inactive ? (char*)user->inactive : "");
	XmTextSetString(modifyUserWin->UxtextField9,
		user->warn ? (char*)user->warn : "");

	if (user->expire != NULL) {
		sscanf (user->expire, "%2d%2d%4d", &day, &month, &year);

		switch (day) {
		case 0:
			XtVaSetValues(modifyUserWin->Uxmenu6, 
				XmNmenuHistory, modifyUserWin->Uxmenu4_p1_b1,
				NULL);
			break;
		case 1:
			XtVaSetValues(modifyUserWin->Uxmenu6, 
				XmNmenuHistory, modifyUserWin->Uxmenu4_p1_b2,
				NULL);
			break;
		case 2:
			XtVaSetValues(modifyUserWin->Uxmenu6, 
				XmNmenuHistory, modifyUserWin->Uxmenu4_p1_b3,
				NULL);
			break;
		case 3:
			XtVaSetValues(modifyUserWin->Uxmenu6, 
				XmNmenuHistory, modifyUserWin->Uxmenu4_p1_b4,
				NULL);
			break;
		case 4:
			XtVaSetValues(modifyUserWin->Uxmenu6, 
				XmNmenuHistory, modifyUserWin->Uxmenu4_p1_b5,
				NULL);
			break;
		case 5:
			XtVaSetValues(modifyUserWin->Uxmenu6, 
				XmNmenuHistory, modifyUserWin->Uxmenu4_p1_b6,
				NULL);
			break;
		case 6:
			XtVaSetValues(modifyUserWin->Uxmenu6, 
				XmNmenuHistory, modifyUserWin->Uxmenu4_p1_b7,
				NULL);
			break;
		case 7:
			XtVaSetValues(modifyUserWin->Uxmenu6, 
				XmNmenuHistory, modifyUserWin->Uxmenu4_p1_b8,
				NULL);
			break;
		case 8:
			XtVaSetValues(modifyUserWin->Uxmenu6, 
				XmNmenuHistory, modifyUserWin->Uxmenu4_p1_b9,
				NULL);
			break;
		case 9:
			XtVaSetValues(modifyUserWin->Uxmenu6, 
				XmNmenuHistory, modifyUserWin->Uxmenu4_p1_b10,
				NULL);
			break;
		case 10:
			XtVaSetValues(modifyUserWin->Uxmenu6, 
				XmNmenuHistory, modifyUserWin->Uxmenu4_p1_b11,
				NULL);
			break;
		case 11:
			XtVaSetValues(modifyUserWin->Uxmenu6, 
				XmNmenuHistory, modifyUserWin->Uxmenu4_p1_b12,
				NULL);
			break;
		case 12:
			XtVaSetValues(modifyUserWin->Uxmenu6, 
				XmNmenuHistory, modifyUserWin->Uxmenu4_p1_b13,
				NULL);
			break;
		case 13:
			XtVaSetValues(modifyUserWin->Uxmenu6, 
				XmNmenuHistory, modifyUserWin->Uxmenu4_p1_b14,
				NULL);
			break;
		case 14:
			XtVaSetValues(modifyUserWin->Uxmenu6, 
				XmNmenuHistory, modifyUserWin->Uxmenu4_p1_b15,
				NULL);
			break;
		case 15:
			XtVaSetValues(modifyUserWin->Uxmenu6, 
				XmNmenuHistory, modifyUserWin->Uxmenu4_p1_b16,
				NULL);
			break;
		case 16:
			XtVaSetValues(modifyUserWin->Uxmenu6, 
				XmNmenuHistory, modifyUserWin->Uxmenu4_p1_b17,
				NULL);
			break;
		case 17:
			XtVaSetValues(modifyUserWin->Uxmenu6, 
				XmNmenuHistory, modifyUserWin->Uxmenu4_p1_b18,
				NULL);
			break;
		case 18:
			XtVaSetValues(modifyUserWin->Uxmenu6, 
				XmNmenuHistory, modifyUserWin->Uxmenu4_p1_b19,
				NULL);
			break;
		case 19:
			XtVaSetValues(modifyUserWin->Uxmenu6, 
				XmNmenuHistory, modifyUserWin->Uxmenu4_p1_b20,
				NULL);
			break;
		case 20:
			XtVaSetValues(modifyUserWin->Uxmenu6, 
				XmNmenuHistory, modifyUserWin->Uxmenu4_p1_b21,
				NULL);
			break;
		case 21:
			XtVaSetValues(modifyUserWin->Uxmenu6, 
				XmNmenuHistory, modifyUserWin->Uxmenu4_p1_b22,
				NULL);
			break;
		case 22:
			XtVaSetValues(modifyUserWin->Uxmenu6, 
				XmNmenuHistory, modifyUserWin->Uxmenu4_p1_b23,
				NULL);
			break;
		case 23:
			XtVaSetValues(modifyUserWin->Uxmenu6, 
				XmNmenuHistory, modifyUserWin->Uxmenu4_p1_b24,
				NULL);
			break;
		case 24:
			XtVaSetValues(modifyUserWin->Uxmenu6, 
				XmNmenuHistory, modifyUserWin->Uxmenu4_p1_b25,
				NULL);
			break;
		case 25:
			XtVaSetValues(modifyUserWin->Uxmenu6, 
				XmNmenuHistory, modifyUserWin->Uxmenu4_p1_b26,
				NULL);
			break;
		case 26:
			XtVaSetValues(modifyUserWin->Uxmenu6, 
				XmNmenuHistory, modifyUserWin->Uxmenu4_p1_b27,
				NULL);
			break;
		case 27:
			XtVaSetValues(modifyUserWin->Uxmenu6, 
				XmNmenuHistory, modifyUserWin->Uxmenu4_p1_b28,
				NULL);
			break;
		case 28:
			XtVaSetValues(modifyUserWin->Uxmenu6, 
				XmNmenuHistory, modifyUserWin->Uxmenu4_p1_b29,
				NULL);
			break;
		case 29:
			XtVaSetValues(modifyUserWin->Uxmenu6, 
				XmNmenuHistory, modifyUserWin->Uxmenu4_p1_b30,
				NULL);
			break;
		case 30:
			XtVaSetValues(modifyUserWin->Uxmenu6, 
				XmNmenuHistory, modifyUserWin->Uxmenu4_p1_b31,
				NULL);
			break;
		case 31:
			XtVaSetValues(modifyUserWin->Uxmenu6, 
				XmNmenuHistory, modifyUserWin->Uxmenu4_p1_b32,
				NULL);
			break;
		default:
			fprintf(stderr, catgets(_catd, 8, 377, "Found an invalid day = %d. \n"), day);
			XtVaSetValues(modifyUserWin->Uxmenu6, 
				XmNmenuHistory, modifyUserWin->Uxmenu4_p1_b1,
				NULL);				
		}

		switch (month) {
		case 0:
			XtVaSetValues(modifyUserWin->Uxmenu5, 
				XmNmenuHistory, modifyUserWin->Uxmenu5_p1_b12,
				NULL);
			break;
		case 1:
			XtVaSetValues(modifyUserWin->Uxmenu5, 
				XmNmenuHistory, modifyUserWin->Uxmenu5_p1_b13,
				NULL);
			break;
		case 2:
			XtVaSetValues(modifyUserWin->Uxmenu5, 
				XmNmenuHistory, modifyUserWin->Uxmenu5_p1_b14,
				NULL);
			break;
		case 3:
			XtVaSetValues(modifyUserWin->Uxmenu5, 
				XmNmenuHistory, modifyUserWin->Uxmenu5_p1_b15,
				NULL);
			break;
		case 4:
			XtVaSetValues(modifyUserWin->Uxmenu5, 
				XmNmenuHistory, modifyUserWin->Uxmenu5_p1_b16,
				NULL);
			break;
		case 5:
			XtVaSetValues(modifyUserWin->Uxmenu5, 
				XmNmenuHistory, modifyUserWin->Uxmenu5_p1_b17,
				NULL);
			break;
		case 6:
			XtVaSetValues(modifyUserWin->Uxmenu5, 
				XmNmenuHistory, modifyUserWin->Uxmenu5_p1_b18,
				NULL);
			break;
		case 7:
			XtVaSetValues(modifyUserWin->Uxmenu5, 
				XmNmenuHistory, modifyUserWin->Uxmenu5_p1_b19,
				NULL);
			break;
		case 8:
			XtVaSetValues(modifyUserWin->Uxmenu5, 
				XmNmenuHistory, modifyUserWin->Uxmenu5_p1_b20,
				NULL);
			break;
		case 9:
			XtVaSetValues(modifyUserWin->Uxmenu5, 
				XmNmenuHistory, modifyUserWin->Uxmenu5_p1_b21,
				NULL);
			break;
		case 10:
			XtVaSetValues(modifyUserWin->Uxmenu5, 
				XmNmenuHistory, modifyUserWin->Uxmenu5_p1_b22,
				NULL);
			break;
		case 11:
			XtVaSetValues(modifyUserWin->Uxmenu5, 
				XmNmenuHistory, modifyUserWin->Uxmenu5_p1_b23,
				NULL);
			break;
		case 12:
			XtVaSetValues(modifyUserWin->Uxmenu5, 
				XmNmenuHistory, modifyUserWin->Uxmenu5_p1_b24,
				NULL);
			break;
		default:
			fprintf(stderr, catgets(_catd, 8, 378, "Found an invalid month = %d. \n"), month);
			XtVaSetValues(modifyUserWin->Uxmenu5, 
				XmNmenuHistory, modifyUserWin->Uxmenu4_p1_b12,
				NULL);				
		}

		switch (year) {
		case 0:
			XtVaSetValues(modifyUserWin->Uxmenu4, 
				XmNmenuHistory, modifyUserWin->Uxmenu5_p1_b1,
				NULL);
			break;	
		case 1993:
			XtVaSetValues(modifyUserWin->Uxmenu4, 
				XmNmenuHistory, modifyUserWin->Uxmenu5_p1_b2,
				NULL);
			break;	
		case 1994:
			XtVaSetValues(modifyUserWin->Uxmenu4, 
				XmNmenuHistory, modifyUserWin->Uxmenu5_p1_b3,
				NULL);
			break;	
		case 1995:
			XtVaSetValues(modifyUserWin->Uxmenu4, 
				XmNmenuHistory, modifyUserWin->Uxmenu5_p1_b4,
				NULL);
			break;	
		case 1996:
			XtVaSetValues(modifyUserWin->Uxmenu4, 
				XmNmenuHistory, modifyUserWin->Uxmenu5_p1_b5,
				NULL);
			break;	
		case 1997:
			XtVaSetValues(modifyUserWin->Uxmenu4, 
				XmNmenuHistory, modifyUserWin->Uxmenu5_p1_b6,
				NULL);
			break;	
		case 1998:
			XtVaSetValues(modifyUserWin->Uxmenu4, 
				XmNmenuHistory, modifyUserWin->Uxmenu5_p1_b7,
				NULL);
			break;	
		case 1999:
			XtVaSetValues(modifyUserWin->Uxmenu4, 
				XmNmenuHistory, modifyUserWin->Uxmenu5_p1_b8,
				NULL);
			break;	
		case 2000:
			XtVaSetValues(modifyUserWin->Uxmenu4, 
				XmNmenuHistory, modifyUserWin->Uxmenu5_p1_b9,
				NULL);
			break;	
		case 2001:
			XtVaSetValues(modifyUserWin->Uxmenu4, 
				XmNmenuHistory, modifyUserWin->Uxmenu5_p1_b10,
				NULL);
			break;	
		case 2002:
			XtVaSetValues(modifyUserWin->Uxmenu4, 
				XmNmenuHistory, modifyUserWin->Uxmenu5_p1_b11,
				NULL);
			break;	
		default:
			fprintf(stderr, catgets(_catd, 8, 379, "Found an invalid year = %d. \n"), year);
			XtVaSetValues(modifyUserWin->Uxmenu4, 
				XmNmenuHistory, modifyUserWin->Uxmenu5_p1_b1,
				NULL);				
		}
	}
	else {
		XtVaSetValues(modifyUserWin->Uxmenu6, 
			XmNmenuHistory, modifyUserWin->Uxmenu4_p1_b1,
			NULL);
		XtVaSetValues(modifyUserWin->Uxmenu5, 
			XmNmenuHistory, modifyUserWin->Uxmenu5_p1_b12,
			NULL);
		XtVaSetValues(modifyUserWin->Uxmenu4, 
			XmNmenuHistory, modifyUserWin->Uxmenu5_p1_b1,
			NULL);
	}

	XmTextSetString(modifyUserWin->UxtextField10,
		user->path ? (char*)user->path : "");


	return 1;
}


/*******************************************************************************
       The following are callback functions.
*******************************************************************************/

void
modifyUserLoginShellBourneCB(
		Widget widget, 
		XtPointer cd, 
		XtPointer cbs)
{

	_UxCmodifyUserDialog	*modifyUserWin;
	XmString		str;

	modifyUserWin = (_UxCmodifyUserDialog *) UxGetContext(widget);

	XtVaSetValues(modifyUserWin->UxtextField36, 
		XmNmappedWhenManaged, FALSE,
		NULL);
	str = XmStringCreateLocalized(SHELL_BOURNE_PATH);
	XtVaSetValues(modifyUserWin->Uxlabel10, 
		XmNmappedWhenManaged, TRUE,
		XmNlabelString, str,
		NULL);
	XmStringFree(str);
}

void
modifyUserLoginShellCCB(
		Widget widget, 
		XtPointer cd, 
		XtPointer cbs)
{

	_UxCmodifyUserDialog	*modifyUserWin;
	XmString		str;

	modifyUserWin = (_UxCmodifyUserDialog *) UxGetContext(widget);

	XtVaSetValues(modifyUserWin->UxtextField36, 
		XmNmappedWhenManaged, FALSE,
		NULL);
	str = XmStringCreateLocalized(SHELL_C_PATH);
	XtVaSetValues(modifyUserWin->Uxlabel10, 
		XmNmappedWhenManaged, TRUE,
		XmNlabelString, str,
		NULL);
	XmStringFree(str);
}

void
modifyUserLoginShellKornCB(
		Widget widget, 
		XtPointer cd, 
		XtPointer cbs)

{

	_UxCmodifyUserDialog	*modifyUserWin;
	XmString		str;

	modifyUserWin = (_UxCmodifyUserDialog *) UxGetContext(widget);

	XtVaSetValues(modifyUserWin->UxtextField36, 
		XmNmappedWhenManaged, FALSE,
		NULL);
	str = XmStringCreateLocalized(SHELL_KORN_PATH);
	XtVaSetValues(modifyUserWin->Uxlabel10, 
		XmNmappedWhenManaged, TRUE,
		XmNlabelString, str,
		NULL);
	XmStringFree(str);
}

void
modifyUserLoginShellOtherCB(
		Widget widget, 
		XtPointer cd, 
		XtPointer cbs)

{

	_UxCmodifyUserDialog	*modifyUserWin;

	modifyUserWin = (_UxCmodifyUserDialog *) UxGetContext(widget);

	XtVaSetValues(modifyUserWin->UxtextField36, 
		XmNmappedWhenManaged, TRUE,
		NULL);
	XmTextSetString(modifyUserWin->UxtextField36, SHELL_OTHER_PATH);
	XmTextFieldSetInsertionPosition(modifyUserWin->UxtextField36,
		strlen(SHELL_OTHER_PATH)); 

	XtVaSetValues(modifyUserWin->Uxlabel10, 
		XmNmappedWhenManaged, FALSE,
		NULL);
}


void
modifyUserSetDateNoneCB(
		Widget widget, 
		XtPointer cd, 
		XtPointer cbs)
{

	_UxCmodifyUserDialog	*modifyUserWin;

	modifyUserWin = (_UxCmodifyUserDialog *) UxGetContext(widget);

	XtVaSetValues(modifyUserWin->Uxmenu6, 
		XmNmenuHistory, modifyUserWin->Uxmenu4_p1_b1,
		NULL);
	XtVaSetValues(modifyUserWin->Uxmenu5, 
		XmNmenuHistory, modifyUserWin->Uxmenu5_p1_b12,
		NULL);
	XtVaSetValues(modifyUserWin->Uxmenu4, 
		XmNmenuHistory, modifyUserWin->Uxmenu5_p1_b1,
		NULL);
}


static	void	passwdTypeCB(
	Widget wgt, 
	XtPointer cd, 
	XtPointer cb)
{
	_UxCmodifyUserDialog    *UxSaveCtx, *UxContext;
	Widget                  UxWidget = wgt;
	XtPointer               UxClientData = cd;
	XtPointer               UxCallbackArg = cb;

	UxSaveCtx = UxModifyUserDialogContext;
	UxModifyUserDialogContext = UxContext =
			(_UxCmodifyUserDialog *) UxGetContext( UxWidget );
	{
		/* Keep track of the last selected password type. */
		XtVaSetValues(UxContext->Uxmenu3, 
			XmNuserData, UxWidget,
			NULL);
	
	}
	UxModifyUserDialogContext = UxSaveCtx;
}

static	void	passwdCB(
	Widget wgt, 
	XtPointer cd, 
	XtPointer cb)
{
	Widget password_widget;
	extern show_password_dialog(Widget parent, char* pw, int* pw_set,
			Widget menu, Widget menuHistory);
	_UxCmodifyUserDialog 	* modUserWin;

	modUserWin = (_UxCmodifyUserDialog *) UxGetContext(wgt);
	XtVaGetValues(modUserWin->Uxmenu3, 
			XmNuserData, &password_widget,
			NULL);

	show_password_dialog(modifyuserdialog,
		ModifyPassword, &UserEnteredModPassword, 
		modUserWin->Uxmenu3, password_widget);
}

static	void	resetCB(
			Widget wgt, 
			XtPointer cd, 
			XtPointer cb)
{
	_UxCmodifyUserDialog    *UxSaveCtx, *UxContext;
	Widget                  UxWidget = wgt;
	XtPointer               UxClientData = cd;
	XtPointer               UxCallbackArg = cb;

	UxSaveCtx = UxModifyUserDialogContext;
	UxModifyUserDialogContext = UxContext =
			(_UxCmodifyUserDialog *) UxGetContext( UxWidget );
	{
	
	modifyUserInit(wgt, &UxContext->user);
	
	}
	UxModifyUserDialogContext = UxSaveCtx;
}

void	modifyCB(
	Widget widget, 
	XtPointer cd, 
	XtPointer cb)
{
	Widget                  UxWidget = widget;
	XtPointer               UxClientData = cd;
	XtPointer               UxCallbackArg = cb;

	{
	char *		username_p=NULL;
	char *		uid_p=NULL;
	char *		gid_p=NULL;
	char *		groups_p=NULL;
	char *		gcos_p=NULL;
	char *		shell_p=NULL;
	char *		passwd_p=NULL;
	char *		min_p=NULL;
	char *		max_p=NULL;
	char *		inactive_p=NULL;
	char *		expire_p;
	char *		warn_p=NULL;
	char *		home_path_p=NULL;
	char *		home_server_p=NULL;
	char *		mail_server_p=NULL;
	char *		home_mode_p;
	char *  	charPtr;
	char *  	charPtr2;
	int		the_day;
	int		the_month;
	int		the_year;
	shortstr	expire_str;
	XmString	motifStr;
	XmString	motifStr1;
	Widget		shellWidget;
	Widget		pwWidget;
	Widget		expWidget;
	SysmanUserArg	user;
	int		sts;
	_UxCmodifyUserDialog	*modifyUserWin;
	char		passwd_buf[32];
        char                    msg[1024];


	modifyUserWin = (_UxCmodifyUserDialog *) UxGetContext(widget);

	expire_p      = expire_str;

	SetBusyPointer(True);

	/* Get the entered user name and ID.  */
	username_p    = XmTextGetString(modifyUserWin->UxtextField1);

	XtVaGetValues(modifyUserWin->Uxlabel99, 
		XmNlabelString, &motifStr,
		NULL);
	XmStringGetLtoR(motifStr, XmSTRING_DEFAULT_CHARSET, &uid_p);


	/* Notify the user if they are attempting to modify the root user
	 * or if they are in effect adding a new root user by modifying
	 * the old user name and /or ID.
	 */
	if (USR_IS_ROOT(modifyUserWin->user.username,
			modifyUserWin->user.uid)) {
		display_warning(modifyuserdialog, ROOT_MODIFY_NOTICE_MSG);
		/* DO NOT perform the MODIFY. */
		SetBusyPointer(False);
		XtFree((char *)username_p);
		return;
	}

	gid_p         = XmTextGetString(modifyUserWin->UxtextField3);

	groups_p      = XmTextGetString(modifyUserWin->UxtextField4);

	gcos_p        = XmTextGetString(modifyUserWin->UxtextField5);

	/* Default shell */
	XtVaGetValues(modifyUserWin->Uxmenu1, 
		XmNmenuHistory, &shellWidget,
		NULL);
	if (shellWidget == modifyUserWin->Uxmenu1_p1_b1)
		shell_p = SHELL_BOURNE_PATH;
	else if (shellWidget == modifyUserWin->Uxmenu1_p1_b2)
		shell_p = SHELL_C_PATH;
	else if (shellWidget == modifyUserWin->Uxmenu1_p1_b3)
		shell_p = SHELL_KORN_PATH;
	else if (shellWidget == modifyUserWin->Uxmenu1_p1_b4)
		shell_p = XmTextGetString(modifyUserWin->UxtextField36);

 	/* We need to determine whether the user has changed anything about
 	 * the password field. If so, we pass appropriate parameters to 
 	 * modify_user. 
 	 * ModOrigPassSetting was set when the modify window was put up (or when
 	 * it was reset). UserEnteredModPassword is set to TRUE if the user
 	 * enters a good password in the password entry popup for normal
	 * passwords.
	 */

	/* initialize passwd_p to existing password value */
	if (modifyUserWin->user.passwd != NULL) {
		strncpy(passwd_buf, modifyUserWin->user.passwd, 32);
		passwd_p = passwd_buf;
	}

	XtVaGetValues(modifyUserWin->Uxmenu3, 
		XmNmenuHistory, &pwWidget,
		NULL);

	if (pwWidget != ModOrigPassSetting) {
		/* Password type changed, set the new values */
		if (pwWidget == modifyUserWin->Uxmenu3_p1_b1) {
			passwd_p = PASSWD_CLEARED;
		} else if (pwWidget == modifyUserWin->Uxmenu3_p1_b2) {
			passwd_p = PASSWD_LOCKED;
		} else if (pwWidget == modifyUserWin->Uxmenu3_p1_b3) {
			passwd_p = PASSWD_NONE;
		} else if (pwWidget == modifyUserWin->Uxmenu3_p1_b4) {
			if (UserEnteredModPassword) {
				passwd_p = ModifyPassword;
			}
		} 
	}
	else if (pwWidget == modifyUserWin->Uxmenu3_p1_b4) {
		/* Password type was not changed and the type
		 * is "normal password". 
		 * Check to see if a new password was entered and pass it along.
		 */
		if (UserEnteredModPassword) {
			passwd_p = ModifyPassword;
		}		 
	}

	min_p         = XmTextGetString(modifyUserWin->UxtextField6);

	max_p         = XmTextGetString(modifyUserWin->UxtextField7);

	inactive_p    = XmTextGetString(modifyUserWin->UxtextField8);

	/* Account expiration date (day, month, year) */
	XtVaGetValues(modifyUserWin->Uxmenu6, 
		XmNmenuHistory, &expWidget,
		NULL);
	XtVaGetValues(expWidget, 
		XmNlabelString, &motifStr,
		NULL);
	XmStringGetLtoR(motifStr, XmSTRING_DEFAULT_CHARSET, &charPtr);
	sscanf(charPtr, "%d", &the_day);
	XtFree(charPtr);
	XmStringFree(motifStr);
	XtVaGetValues(modifyUserWin->Uxmenu5, 
		XmNmenuHistory, &expWidget,
		NULL);

	if (expWidget == modifyUserWin->Uxmenu5_p1_b12)
		the_month = 0;
	else if (expWidget == modifyUserWin->Uxmenu5_p1_b13)
		the_month = 1;
	else if (expWidget == modifyUserWin->Uxmenu5_p1_b14)
		the_month = 2;
	else if (expWidget == modifyUserWin->Uxmenu5_p1_b15)
		the_month = 3;
	else if (expWidget == modifyUserWin->Uxmenu5_p1_b16)
		the_month = 4;
	else if (expWidget == modifyUserWin->Uxmenu5_p1_b17)
		the_month = 5;
	else if (expWidget == modifyUserWin->Uxmenu5_p1_b18)
		the_month = 6;
	else if (expWidget == modifyUserWin->Uxmenu5_p1_b19)
		the_month = 7;
	else if (expWidget == modifyUserWin->Uxmenu5_p1_b20)
		the_month = 8;
	else if (expWidget == modifyUserWin->Uxmenu5_p1_b21)
		the_month = 9;
	else if (expWidget == modifyUserWin->Uxmenu5_p1_b22)
		the_month = 10;
	else if (expWidget == modifyUserWin->Uxmenu5_p1_b23)
		the_month = 11;
	else if (expWidget == modifyUserWin->Uxmenu5_p1_b24)
		the_month = 12;

	the_day   = min2(the_day, day_limit[the_month]);
	XtVaGetValues(modifyUserWin->Uxmenu4, 
		XmNmenuHistory, &expWidget,
		NULL);
	XtVaGetValues(expWidget, 
		XmNlabelString, &motifStr,
		NULL);
	XmStringGetLtoR(motifStr, XmSTRING_DEFAULT_CHARSET, &charPtr2);
	if (expWidget == modifyUserWin->Uxmenu5_p1_b1)
		the_year = 0;
	else
		sscanf(charPtr2, "%d", &the_year);
	XtFree(charPtr2);
	XmStringFree(motifStr);

	format_the_date(the_day, the_month, the_year, expire_p);

	warn_p = XmTextGetString(modifyUserWin->UxtextField9);

	home_path_p = XmTextGetString(modifyUserWin->UxtextField10);

	memset((void *)&user, 0, sizeof (user));

	user.username = username_p;
	user.username_key = modifyUserWin->user.username_key;
	user.uid = uid_p;
	user.group = gid_p;
	user.second_grps = groups_p;
	user.comment = gcos_p;
	user.shell = shell_p;
	user.passwd = passwd_p;
	user.lastchanged = NULL;
	user.minimum = min_p;
	user.maximum = max_p;
	user.inactive = inactive_p;
	user.expire = expire_p;
	user.warn = warn_p;
	user.path = home_path_p;

	sts = sysman_modify_user(&user, errbuf, ERRBUF_SIZE);

	if (sts >= 0) {
		if (sts == SYSMAN_INFO) {
			display_infomsg(modifyuserdialog, errbuf);
		}
		update_entry(&user);
		free_user(&modifyUserWin->user);
		copy_user(&modifyUserWin->user, &user);
		if (widget == modifyUserWin->UxpushButton4) {
			/* Dismiss the window - OK was selected. */
			UxPopdownInterface(modifyUserWin->UxmodifyUserDialog);
		}
	}
	else {
		display_error(modifyuserdialog, errbuf);
	}


	/* Free Motif compound strings. */
	if ((char *)username_p != NULL) 
		XtFree((char *)username_p);
	if (uid_p != NULL) 
		XtFree((char *)uid_p);
	if ((char *)gcos_p != NULL) 
		XtFree((char *)gcos_p);
	if ((char *)gid_p != NULL) 
		XtFree((char *)gid_p);
	if ((char *)groups_p != NULL) 
		XtFree((char *)groups_p);
	if ((char *)min_p != NULL) 
		XtFree((char *)min_p);
	if ((char *)max_p != NULL) 
		XtFree((char *)max_p);
	if ((char *)inactive_p != NULL) 
		XtFree((char *)inactive_p);
	if ((char *)warn_p != NULL) 
		XtFree((char *)warn_p);
	if ((char *)home_path_p != NULL) 
		XtFree((char *)home_path_p);
	if ((char *)home_server_p != NULL) 
		XtFree((char *)home_server_p);
	if ((char *)mail_server_p != NULL) 
		XtFree((char *)mail_server_p);


	SetBusyPointer(False);

	}
}


static	void	cancelCB(
			Widget wgt, 
			XtPointer cd, 
			XtPointer cb)
{
	_UxCmodifyUserDialog    *UxSaveCtx, *UxContext;
	Widget                  UxWidget = wgt;
	XtPointer               UxClientData = cd;
	XtPointer               UxCallbackArg = cb;

	UxSaveCtx = UxModifyUserDialogContext;
	UxModifyUserDialogContext = UxContext =
			(_UxCmodifyUserDialog *) UxGetContext( UxWidget );
	{
		UxPopdownInterface(UxContext->UxmodifyUserDialog);
	}
	UxModifyUserDialogContext = UxSaveCtx;
}

/*******************************************************************************
       The 'build_' function creates all the widgets
       using the resource values specified in the Property Editor.
*******************************************************************************/

static Widget	_Uxbuild_modifyUserDialog(Widget UxParent)
{
	Widget		_UxParent;
	Widget		menu1_p1_shell;
	Widget		menu3_p1_shell;
	Widget		menu4_p1_shell;
	Widget		menu5_p2_shell;
	Widget		menu5_p1_shell;
	Widget		wlist[15];
	int		i, wnum;
	Widget		maxlabel;
	Dimension	width;
	Dimension	maxwidth = 0;

	_UxParent = UxParent;
	if ( _UxParent == NULL )
	{
		_UxParent = GtopLevel;
	}


	modifyUserDialog = XtVaCreatePopupShell( "modifyUserDialog",
			xmDialogShellWidgetClass,
			_UxParent,
			XmNtitle, catgets(_catd, 8, 381, "Admintool: Modify User"),
			XmNinitialState, InactiveState,
			XmNminHeight, 533,
			XmNminWidth, 412,
			NULL );
	UxPutContext( modifyUserDialog, (char *) UxModifyUserDialogContext );

	form2 = XtVaCreateWidget( "form2",
			xmFormWidgetClass,
			modifyUserDialog,
			XmNresizePolicy, XmRESIZE_ANY,
			XmNmarginWidth, 15,
			XmNmarginHeight, 15,
			XmNrubberPositioning, TRUE,
			XmNautoUnmanage, FALSE,
			NULL );
	UxPutContext( form2, (char *) UxModifyUserDialogContext );

	label4 = XtVaCreateManagedWidget( "label4",
			xmLabelWidgetClass,
			form2,
			XmNalignment, XmALIGNMENT_BEGINNING,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 382, "USER IDENTITY") ),
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_FORM,
			XmNleftOffset, 12,
			XmNtopOffset, 15,
			NULL );
	UxPutContext( label4, (char *) UxModifyUserDialogContext );

	label5 = XtVaCreateManagedWidget( "label5",
			xmLabelWidgetClass,
			form2,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 383, "User Name:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, label4,
			XmNtopOffset, 8,
			NULL );
	UxPutContext( label5, (char *) UxModifyUserDialogContext );

	textField1 = XtVaCreateManagedWidget( "textField1",
			xmTextFieldWidgetClass,
			form2,
			XmNvalue, "",
			XmNmarginHeight, 1,
			XmNcolumns, 8,
			XmNmaxLength, 8,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, label5,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, label5,
			NULL );
	UxPutContext( textField1, (char *) UxModifyUserDialogContext );

	label6 = XtVaCreateManagedWidget( "label6",
			xmLabelWidgetClass,
			form2,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 384, "User ID:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, label5,
			XmNtopOffset, 8,
			NULL );
	UxPutContext( label6, (char *) UxModifyUserDialogContext );

	label99 = XtVaCreateManagedWidget( "label99",
			xmLabelWidgetClass,
			form2,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, label6,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, label6,
			NULL );
	UxPutContext( label99, (char *) UxModifyUserDialogContext );

	label7 = XtVaCreateManagedWidget( "label7",
			xmLabelWidgetClass,
			form2,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 385, "Primary Group:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, label6,
			XmNtopOffset, 8,
			NULL );
	UxPutContext( label7, (char *) UxModifyUserDialogContext );

	textField3 = XtVaCreateManagedWidget( "textField3",
			xmTextFieldWidgetClass,
			form2,
			XmNresizeWidth, FALSE,
			XmNvalue, "other",
			XmNmarginHeight, 1,
			XmNmaxLength, 80,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, label7,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, label7,
			NULL );
	UxPutContext( textField3, (char *) UxModifyUserDialogContext );

	label8 = XtVaCreateManagedWidget( "label8",
			xmLabelWidgetClass,
			form2,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 387, "Secondary Groups:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, label7,
			XmNtopOffset, 8,
			NULL );
	UxPutContext( label8, (char *) UxModifyUserDialogContext );

	textField4 = XtVaCreateManagedWidget( "textField4",
			xmTextFieldWidgetClass,
			form2,
			XmNvalue, "",
			XmNmarginHeight, 1,
			XmNmaxLength, 150,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, label8,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, label8,
			XmNrightAttachment, XmATTACH_POSITION,
			XmNrightPosition, 80,
			NULL );
	UxPutContext( textField4, (char *) UxModifyUserDialogContext );

	label9 = XtVaCreateManagedWidget( "label9",
			xmLabelWidgetClass,
			form2,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 388, "Comment:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopOffset, 8,
			XmNtopWidget, label8,
			NULL );
	UxPutContext( label9, (char *) UxModifyUserDialogContext );

	textField5 = XtVaCreateManagedWidget( "textField5",
			xmTextFieldWidgetClass,
			form2,
			XmNvalue, "",
			XmNmarginHeight, 1,
			XmNmaxLength, 80,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, label9,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, label9,
			XmNrightAttachment, XmATTACH_POSITION,
			XmNrightPosition, 80,
			NULL );
	UxPutContext( textField5, (char *) UxModifyUserDialogContext );

	label11 = XtVaCreateManagedWidget( "label11",
			xmLabelWidgetClass,
			form2,
			XmNalignment, XmALIGNMENT_END,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 389, "Login Shell:") ),
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopOffset, 8,
			XmNtopWidget, label9,
			NULL );
	UxPutContext( label11, (char *) UxModifyUserDialogContext );

	menu1_p1_shell = XtVaCreatePopupShell ("menu1_p1_shell",
			xmMenuShellWidgetClass, form2,
			XmNwidth, 1,
			XmNheight, 1,
			XmNallowShellResize, TRUE,
			XmNoverrideRedirect, TRUE,
			NULL );

	menu1_p1 = XtVaCreateWidget( "menu1_p1",
			xmRowColumnWidgetClass,
			menu1_p1_shell,
			XmNrowColumnType, XmMENU_PULLDOWN,
			NULL );
	UxPutContext( menu1_p1, (char *) UxModifyUserDialogContext );

	menu1_p1_b1 = XtVaCreateManagedWidget( "menu1_p1_b1",
			xmPushButtonGadgetClass,
			menu1_p1,
			RES_CONVERT( XmNlabelString, "Bourne" ),
			XmNshowAsDefault, 0,
			NULL );
	UxPutContext( menu1_p1_b1, (char *) UxModifyUserDialogContext );

	menu1_p1_b2 = XtVaCreateManagedWidget( "menu1_p1_b2",
			xmPushButtonGadgetClass,
			menu1_p1,
			RES_CONVERT( XmNlabelString, "C" ),
			XmNshowAsDefault, 0,
			NULL );
	UxPutContext( menu1_p1_b2, (char *) UxModifyUserDialogContext );

	menu1_p1_b3 = XtVaCreateManagedWidget( "menu1_p1_b3",
			xmPushButtonGadgetClass,
			menu1_p1,
			RES_CONVERT( XmNlabelString, "Korn" ),
			XmNshowAsDefault, 0,
			NULL );
	UxPutContext( menu1_p1_b3, (char *) UxModifyUserDialogContext );

	menu1_p1_b4 = XtVaCreateManagedWidget( "menu1_p1_b4",
			xmPushButtonGadgetClass,
			menu1_p1,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 390, "Other") ),
			XmNshowAsDefault, 0,
			NULL );
	UxPutContext( menu1_p1_b4, (char *) UxModifyUserDialogContext );

	menu1 = XtVaCreateManagedWidget( "menu1",
			xmRowColumnWidgetClass,
			form2,
			XmNrowColumnType, XmMENU_OPTION,
			XmNsubMenuId, menu1_p1,
			XmNnumColumns, 1,
			XmNpacking, XmPACK_TIGHT,
			XmNspacing, 3,
			XmNmarginWidth, 0,
			XmNnavigationType, XmTAB_GROUP,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, textField5,
			XmNtopOffset, 5,
			XmNmarginHeight, 0,
			RES_CONVERT( XmNlabelString, " " ),
			XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNleftWidget, textField5,
			XmNleftOffset, -10,
			NULL );
	UxPutContext( menu1, (char *) UxModifyUserDialogContext );

	textField36 = XtVaCreateManagedWidget( "textField36",
			xmTextFieldWidgetClass,
			form2,
			XmNresizeWidth, FALSE,
			XmNvalue, "",
			XmNmarginHeight, 1,
			XmNmaxLength, 80,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, menu1,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, label11,
			XmNbottomOffset, -4,
			XmNleftOffset, 2,
			NULL );
	UxPutContext( textField36, (char *) UxModifyUserDialogContext );

	label10 = XtVaCreateManagedWidget( "label10",
			xmLabelWidgetClass,
			form2,
			RES_CONVERT( XmNlabelString, "/bin/sh" ),
			XmNalignment, XmALIGNMENT_BEGINNING,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, menu1,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, label11,
			XmNleftOffset,5,
			NULL );
	UxPutContext( label10, (char *) UxModifyUserDialogContext );

	label12 = XtVaCreateManagedWidget( "label12",
			xmLabelWidgetClass,
			form2,
			XmNalignment, XmALIGNMENT_BEGINNING,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 391, "ACCOUNT SECURITY") ),
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopOffset, 11,
			XmNtopWidget, menu1,
			XmNrightAttachment, XmATTACH_NONE,
			XmNleftAttachment, XmATTACH_FORM,
			XmNleftOffset, 12,
			NULL );
	UxPutContext( label12, (char *) UxModifyUserDialogContext );

	label16 = XtVaCreateManagedWidget( "label16",
			xmLabelWidgetClass,
			form2,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 392, "Password:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, label12,
			XmNtopOffset, 7,
			NULL );
	UxPutContext( label16, (char *) UxModifyUserDialogContext );

	menu3_p1_shell = XtVaCreatePopupShell ("menu3_p1_shell",
			xmMenuShellWidgetClass, form2,
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
	UxPutContext( menu3_p1, (char *) UxModifyUserDialogContext );

	menu3_p1_b1 = XtVaCreateManagedWidget( "menu3_p1_b1",
			xmPushButtonGadgetClass,
			menu3_p1,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 393, "Cleared until first login") ),
			NULL );
	UxPutContext( menu3_p1_b1, (char *) UxModifyUserDialogContext );

	menu3_p1_b2 = XtVaCreateManagedWidget( "menu3_p1_b2",
			xmPushButtonGadgetClass,
			menu3_p1,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 394, "Account is locked") ),
			NULL );
	UxPutContext( menu3_p1_b2, (char *) UxModifyUserDialogContext );

	menu3_p1_b3 = XtVaCreateManagedWidget( "menu3_p1_b3",
			xmPushButtonGadgetClass,
			menu3_p1,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 395, "No password -- setuid only") ),
			NULL );
	UxPutContext( menu3_p1_b3, (char *) UxModifyUserDialogContext );

	menu3_p1_b4 = XtVaCreateManagedWidget( "menu3_p1_b4",
			xmPushButtonGadgetClass,
			menu3_p1,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 396, "Normal Password...") ),
			NULL );
	UxPutContext( menu3_p1_b4, (char *) UxModifyUserDialogContext );

	menu3 = XtVaCreateManagedWidget( "menu3",
			xmRowColumnWidgetClass,
			form2,
			XmNrowColumnType, XmMENU_OPTION,
			XmNsubMenuId, menu3_p1,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopOffset, 0,
			XmNtopWidget, label12,
			RES_CONVERT( XmNlabelString, " " ),
			XmNnavigationType, XmTAB_GROUP,
			XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNleftWidget, menu1,
			XmNleftOffset, -4,
			NULL );
	UxPutContext( menu3, (char *) UxModifyUserDialogContext );

	label17 = XtVaCreateManagedWidget( "label17",
			xmLabelWidgetClass,
			form2,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 397, "Min Change:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopOffset, 6,
			XmNtopWidget, menu3,
			NULL );
	UxPutContext( label17, (char *) UxModifyUserDialogContext );

	textField6 = XtVaCreateManagedWidget( "textField6",
			xmTextFieldWidgetClass,
			form2,
			XmNvalue, "",
			XmNmarginHeight, 1,
			XmNcolumns, 8,
			XmNmaxLength, 5,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, label17,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, label17,
			NULL );
	UxPutContext( textField6, (char *) UxModifyUserDialogContext );

	label13 = XtVaCreateManagedWidget( "label13",
			xmLabelWidgetClass,
			form2,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 398, "days") ),
			XmNalignment, XmALIGNMENT_BEGINNING,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, textField6,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, textField6,
			NULL );
	UxPutContext( label13, (char *) UxModifyUserDialogContext );

	label18 = XtVaCreateManagedWidget( "label18",
			xmLabelWidgetClass,
			form2,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 399, "Max Change:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopOffset, 8,
			XmNtopWidget, label17,
			NULL );
	UxPutContext( label18, (char *) UxModifyUserDialogContext );

	textField7 = XtVaCreateManagedWidget( "textField7",
			xmTextFieldWidgetClass,
			form2,
			XmNvalue, "",
			XmNmarginHeight, 1,
			XmNcolumns, 8,
			XmNmaxLength, 5,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, label18,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, label18,
			NULL );
	UxPutContext( textField7, (char *) UxModifyUserDialogContext );

	label14 = XtVaCreateManagedWidget( "label14",
			xmLabelWidgetClass,
			form2,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 400, "days") ),
			XmNalignment, XmALIGNMENT_BEGINNING,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, textField7,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, textField7,
			NULL );
	UxPutContext( label14, (char *) UxModifyUserDialogContext );

	label19 = XtVaCreateManagedWidget( "label19",
			xmLabelWidgetClass,
			form2,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 401, "Max Inactive:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopOffset, 8,
			XmNtopWidget, label18,
			NULL );
	UxPutContext( label19, (char *) UxModifyUserDialogContext );

	textField8 = XtVaCreateManagedWidget( "textField8",
			xmTextFieldWidgetClass,
			form2,
			XmNvalue, "",
			XmNmarginHeight, 1,
			XmNcolumns, 8,
			XmNmaxLength, 5,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, label19,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, label19,
			NULL );
	UxPutContext( textField8, (char *) UxModifyUserDialogContext );

	label15 = XtVaCreateManagedWidget( "label15",
			xmLabelWidgetClass,
			form2,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 402, "days") ),
			XmNalignment, XmALIGNMENT_BEGINNING,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, textField8,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, textField8,
			NULL );
	UxPutContext( label15, (char *) UxModifyUserDialogContext );

	label20 = XtVaCreateManagedWidget( "label20",
			xmLabelWidgetClass,
			form2,
			RES_CONVERT( XmNlabelString,
				catgets(_catd, 8, 403, "Expiration Date:\n(dd/mm/yy)") ),
			XmNalignment, XmALIGNMENT_END,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopOffset, 8,
			XmNtopWidget, label19,
			XmNleftAttachment, XmATTACH_FORM,
			NULL );
	UxPutContext( label20, (char *) UxModifyUserDialogContext );


	menu4_p1_shell = XtVaCreatePopupShell ("menu4_p1_shell",
			xmMenuShellWidgetClass, form2,
			XmNwidth, 1,
			XmNheight, 1,
			XmNallowShellResize, TRUE,
			XmNoverrideRedirect, TRUE,
			NULL );

	menu4_p1 = XtVaCreateWidget( "menu4_p1",
			xmRowColumnWidgetClass,
			menu4_p1_shell,
			XmNrowColumnType, XmMENU_PULLDOWN,
			XmNnumColumns, 4,
			XmNpacking, XmPACK_COLUMN,
			XmNorientation, XmHORIZONTAL,
			NULL );
	UxPutContext( menu4_p1, (char *) UxModifyUserDialogContext );

	menu4_p1_b1 = XtVaCreateManagedWidget( "menu4_p1_b1",
			xmPushButtonGadgetClass,
			menu4_p1,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 404, "None") ),
			NULL );
	UxPutContext( menu4_p1_b1, (char *) UxModifyUserDialogContext );

	menu4_p1_b2 = XtVaCreateManagedWidget( "menu4_p1_b2",
			xmPushButtonGadgetClass,
			menu4_p1,
			RES_CONVERT( XmNlabelString, "1" ),
			NULL );
	UxPutContext( menu4_p1_b2, (char *) UxModifyUserDialogContext );

	menu4_p1_b3 = XtVaCreateManagedWidget( "menu4_p1_b3",
			xmPushButtonGadgetClass,
			menu4_p1,
			RES_CONVERT( XmNlabelString, "2" ),
			NULL );
	UxPutContext( menu4_p1_b3, (char *) UxModifyUserDialogContext );

	menu4_p1_b4 = XtVaCreateManagedWidget( "menu4_p1_b4",
			xmPushButtonGadgetClass,
			menu4_p1,
			RES_CONVERT( XmNlabelString, "3" ),
			NULL );
	UxPutContext( menu4_p1_b4, (char *) UxModifyUserDialogContext );

	menu4_p1_b5 = XtVaCreateManagedWidget( "menu4_p1_b5",
			xmPushButtonGadgetClass,
			menu4_p1,
			RES_CONVERT( XmNlabelString, "4" ),
			NULL );
	UxPutContext( menu4_p1_b5, (char *) UxModifyUserDialogContext );

	menu4_p1_b6 = XtVaCreateManagedWidget( "menu4_p1_b6",
			xmPushButtonGadgetClass,
			menu4_p1,
			RES_CONVERT( XmNlabelString, "5" ),
			NULL );
	UxPutContext( menu4_p1_b6, (char *) UxModifyUserDialogContext );

	menu4_p1_b7 = XtVaCreateManagedWidget( "menu4_p1_b7",
			xmPushButtonGadgetClass,
			menu4_p1,
			RES_CONVERT( XmNlabelString, "6" ),
			NULL );
	UxPutContext( menu4_p1_b7, (char *) UxModifyUserDialogContext );

	menu4_p1_b8 = XtVaCreateManagedWidget( "menu4_p1_b8",
			xmPushButtonGadgetClass,
			menu4_p1,
			RES_CONVERT( XmNlabelString, "7" ),
			NULL );
	UxPutContext( menu4_p1_b8, (char *) UxModifyUserDialogContext );

	menu4_p1_b9 = XtVaCreateManagedWidget( "menu4_p1_b9",
			xmPushButtonGadgetClass,
			menu4_p1,
			RES_CONVERT( XmNlabelString, "8" ),
			NULL );
	UxPutContext( menu4_p1_b9, (char *) UxModifyUserDialogContext );

	menu4_p1_b10 = XtVaCreateManagedWidget( "menu4_p1_b10",
			xmPushButtonGadgetClass,
			menu4_p1,
			RES_CONVERT( XmNlabelString, "9" ),
			NULL );
	UxPutContext( menu4_p1_b10, (char *) UxModifyUserDialogContext );

	menu4_p1_b11 = XtVaCreateManagedWidget( "menu4_p1_b11",
			xmPushButtonGadgetClass,
			menu4_p1,
			RES_CONVERT( XmNlabelString, "10" ),
			NULL );
	UxPutContext( menu4_p1_b11, (char *) UxModifyUserDialogContext );

	menu4_p1_b12 = XtVaCreateManagedWidget( "menu4_p1_b12",
			xmPushButtonGadgetClass,
			menu4_p1,
			RES_CONVERT( XmNlabelString, "11" ),
			NULL );
	UxPutContext( menu4_p1_b12, (char *) UxModifyUserDialogContext );

	menu4_p1_b13 = XtVaCreateManagedWidget( "menu4_p1_b13",
			xmPushButtonGadgetClass,
			menu4_p1,
			RES_CONVERT( XmNlabelString, "12" ),
			NULL );
	UxPutContext( menu4_p1_b13, (char *) UxModifyUserDialogContext );

	menu4_p1_b14 = XtVaCreateManagedWidget( "menu4_p1_b14",
			xmPushButtonGadgetClass,
			menu4_p1,
			RES_CONVERT( XmNlabelString, "13" ),
			NULL );
	UxPutContext( menu4_p1_b14, (char *) UxModifyUserDialogContext );

	menu4_p1_b15 = XtVaCreateManagedWidget( "menu4_p1_b15",
			xmPushButtonGadgetClass,
			menu4_p1,
			RES_CONVERT( XmNlabelString, "14" ),
			NULL );
	UxPutContext( menu4_p1_b15, (char *) UxModifyUserDialogContext );

	menu4_p1_b16 = XtVaCreateManagedWidget( "menu4_p1_b16",
			xmPushButtonGadgetClass,
			menu4_p1,
			RES_CONVERT( XmNlabelString, "15" ),
			NULL );
	UxPutContext( menu4_p1_b16, (char *) UxModifyUserDialogContext );

	menu4_p1_b17 = XtVaCreateManagedWidget( "menu4_p1_b17",
			xmPushButtonGadgetClass,
			menu4_p1,
			RES_CONVERT( XmNlabelString, "16" ),
			NULL );
	UxPutContext( menu4_p1_b17, (char *) UxModifyUserDialogContext );

	menu4_p1_b18 = XtVaCreateManagedWidget( "menu4_p1_b18",
			xmPushButtonGadgetClass,
			menu4_p1,
			RES_CONVERT( XmNlabelString, "17" ),
			NULL );
	UxPutContext( menu4_p1_b18, (char *) UxModifyUserDialogContext );

	menu4_p1_b19 = XtVaCreateManagedWidget( "menu4_p1_b19",
			xmPushButtonGadgetClass,
			menu4_p1,
			RES_CONVERT( XmNlabelString, "18" ),
			NULL );
	UxPutContext( menu4_p1_b19, (char *) UxModifyUserDialogContext );

	menu4_p1_b20 = XtVaCreateManagedWidget( "menu4_p1_b20",
			xmPushButtonGadgetClass,
			menu4_p1,
			RES_CONVERT( XmNlabelString, "19" ),
			NULL );
	UxPutContext( menu4_p1_b20, (char *) UxModifyUserDialogContext );

	menu4_p1_b21 = XtVaCreateManagedWidget( "menu4_p1_b21",
			xmPushButtonGadgetClass,
			menu4_p1,
			RES_CONVERT( XmNlabelString, "20" ),
			NULL );
	UxPutContext( menu4_p1_b21, (char *) UxModifyUserDialogContext );

	menu4_p1_b22 = XtVaCreateManagedWidget( "menu4_p1_b22",
			xmPushButtonGadgetClass,
			menu4_p1,
			RES_CONVERT( XmNlabelString, "21" ),
			NULL );
	UxPutContext( menu4_p1_b22, (char *) UxModifyUserDialogContext );

	menu4_p1_b23 = XtVaCreateManagedWidget( "menu4_p1_b23",
			xmPushButtonGadgetClass,
			menu4_p1,
			RES_CONVERT( XmNlabelString, "22" ),
			NULL );
	UxPutContext( menu4_p1_b23, (char *) UxModifyUserDialogContext );

	menu4_p1_b24 = XtVaCreateManagedWidget( "menu4_p1_b24",
			xmPushButtonGadgetClass,
			menu4_p1,
			RES_CONVERT( XmNlabelString, "23" ),
			NULL );
	UxPutContext( menu4_p1_b24, (char *) UxModifyUserDialogContext );

	menu4_p1_b25 = XtVaCreateManagedWidget( "menu4_p1_b25",
			xmPushButtonGadgetClass,
			menu4_p1,
			RES_CONVERT( XmNlabelString, "24" ),
			NULL );
	UxPutContext( menu4_p1_b25, (char *) UxModifyUserDialogContext );

	menu4_p1_b26 = XtVaCreateManagedWidget( "menu4_p1_b26",
			xmPushButtonGadgetClass,
			menu4_p1,
			RES_CONVERT( XmNlabelString, "25" ),
			NULL );
	UxPutContext( menu4_p1_b26, (char *) UxModifyUserDialogContext );

	menu4_p1_b27 = XtVaCreateManagedWidget( "menu4_p1_b27",
			xmPushButtonGadgetClass,
			menu4_p1,
			RES_CONVERT( XmNlabelString, "26" ),
			NULL );
	UxPutContext( menu4_p1_b27, (char *) UxModifyUserDialogContext );

	menu4_p1_b28 = XtVaCreateManagedWidget( "menu4_p1_b28",
			xmPushButtonGadgetClass,
			menu4_p1,
			RES_CONVERT( XmNlabelString, "27" ),
			NULL );
	UxPutContext( menu4_p1_b28, (char *) UxModifyUserDialogContext );

	menu4_p1_b29 = XtVaCreateManagedWidget( "menu4_p1_b29",
			xmPushButtonGadgetClass,
			menu4_p1,
			RES_CONVERT( XmNlabelString, "28" ),
			NULL );
	UxPutContext( menu4_p1_b29, (char *) UxModifyUserDialogContext );

	menu4_p1_b30 = XtVaCreateManagedWidget( "menu4_p1_b30",
			xmPushButtonGadgetClass,
			menu4_p1,
			RES_CONVERT( XmNlabelString, "29" ),
			NULL );
	UxPutContext( menu4_p1_b30, (char *) UxModifyUserDialogContext );

	menu4_p1_b31 = XtVaCreateManagedWidget( "menu4_p1_b31",
			xmPushButtonGadgetClass,
			menu4_p1,
			RES_CONVERT( XmNlabelString, "30" ),
			NULL );
	UxPutContext( menu4_p1_b31, (char *) UxModifyUserDialogContext );

	menu4_p1_b32 = XtVaCreateManagedWidget( "menu4_p1_b32",
			xmPushButtonGadgetClass,
			menu4_p1,
			RES_CONVERT( XmNlabelString, "31" ),
			NULL );
	UxPutContext( menu4_p1_b32, (char *) UxModifyUserDialogContext );

	menu6 = XtVaCreateManagedWidget( "menu6",
			xmRowColumnWidgetClass,
			form2,
			XmNrowColumnType, XmMENU_OPTION,
			XmNsubMenuId, menu4_p1,
			XmNnumColumns, 8,
			XmNpacking, XmPACK_COLUMN,
			XmNorientation, XmHORIZONTAL,
			XmNnavigationType, XmTAB_GROUP,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, textField8,
			XmNrightAttachment, XmATTACH_NONE,
			XmNrightWidget, NULL,
			XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNleftWidget, textField8,
			XmNleftOffset, -8,
			RES_CONVERT( XmNlabelString, "" ),
			NULL );
	UxPutContext( menu6, (char *) UxModifyUserDialogContext );

	menu5_p2_shell = XtVaCreatePopupShell ("menu5_p2_shell",
			xmMenuShellWidgetClass, form2,
			XmNwidth, 1,
			XmNheight, 1,
			XmNallowShellResize, TRUE,
			XmNoverrideRedirect, TRUE,
			NULL );

	menu5_p2 = XtVaCreateWidget( "menu5_p2",
			xmRowColumnWidgetClass,
			menu5_p2_shell,
			XmNrowColumnType, XmMENU_PULLDOWN,
			NULL );
	UxPutContext( menu5_p2, (char *) UxModifyUserDialogContext );

	menu5_p1_b12 = XtVaCreateManagedWidget( "menu5_p1_b12",
			xmPushButtonGadgetClass,
			menu5_p2,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 405, "None") ),
			NULL );
	UxPutContext( menu5_p1_b12, (char *) UxModifyUserDialogContext );

	menu5_p1_b13 = XtVaCreateManagedWidget( "menu5_p1_b13",
			xmPushButtonGadgetClass,
			menu5_p2,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 406, "Jan") ),
			NULL );
	UxPutContext( menu5_p1_b13, (char *) UxModifyUserDialogContext );

	menu5_p1_b14 = XtVaCreateManagedWidget( "menu5_p1_b14",
			xmPushButtonGadgetClass,
			menu5_p2,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 407, "Feb") ),
			NULL );
	UxPutContext( menu5_p1_b14, (char *) UxModifyUserDialogContext );

	menu5_p1_b15 = XtVaCreateManagedWidget( "menu5_p1_b15",
			xmPushButtonGadgetClass,
			menu5_p2,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 408, "March") ),
			NULL );
	UxPutContext( menu5_p1_b15, (char *) UxModifyUserDialogContext );

	menu5_p1_b16 = XtVaCreateManagedWidget( "menu5_p1_b16",
			xmPushButtonGadgetClass,
			menu5_p2,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 409, "April") ),
			NULL );
	UxPutContext( menu5_p1_b16, (char *) UxModifyUserDialogContext );

	menu5_p1_b17 = XtVaCreateManagedWidget( "menu5_p1_b17",
			xmPushButtonGadgetClass,
			menu5_p2,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 410, "May") ),
			NULL );
	UxPutContext( menu5_p1_b17, (char *) UxModifyUserDialogContext );

	menu5_p1_b18 = XtVaCreateManagedWidget( "menu5_p1_b18",
			xmPushButtonGadgetClass,
			menu5_p2,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 411, "June") ),
			NULL );
	UxPutContext( menu5_p1_b18, (char *) UxModifyUserDialogContext );

	menu5_p1_b19 = XtVaCreateManagedWidget( "menu5_p1_b19",
			xmPushButtonGadgetClass,
			menu5_p2,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 412, "July") ),
			NULL );
	UxPutContext( menu5_p1_b19, (char *) UxModifyUserDialogContext );

	menu5_p1_b20 = XtVaCreateManagedWidget( "menu5_p1_b20",
			xmPushButtonGadgetClass,
			menu5_p2,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 413, "Aug") ),
			NULL );
	UxPutContext( menu5_p1_b20, (char *) UxModifyUserDialogContext );

	menu5_p1_b21 = XtVaCreateManagedWidget( "menu5_p1_b21",
			xmPushButtonGadgetClass,
			menu5_p2,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 414, "Sept") ),
			NULL );
	UxPutContext( menu5_p1_b21, (char *) UxModifyUserDialogContext );

	menu5_p1_b22 = XtVaCreateManagedWidget( "menu5_p1_b22",
			xmPushButtonGadgetClass,
			menu5_p2,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 415, "Oct") ),
			NULL );
	UxPutContext( menu5_p1_b22, (char *) UxModifyUserDialogContext );

	menu5_p1_b23 = XtVaCreateManagedWidget( "menu5_p1_b23",
			xmPushButtonGadgetClass,
			menu5_p2,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 416, "Nov") ),
			NULL );
	UxPutContext( menu5_p1_b23, (char *) UxModifyUserDialogContext );

	menu5_p1_b24 = XtVaCreateManagedWidget( "menu5_p1_b24",
			xmPushButtonGadgetClass,
			menu5_p2,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 417, "Dec") ),
			NULL );
	UxPutContext( menu5_p1_b24, (char *) UxModifyUserDialogContext );

	menu5 = XtVaCreateManagedWidget( "menu5",
			xmRowColumnWidgetClass,
			form2,
			XmNrowColumnType, XmMENU_OPTION,
			XmNsubMenuId, menu5_p2,
			XmNnavigationType, XmTAB_GROUP,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, textField8,
			XmNrightAttachment, XmATTACH_NONE,
			XmNrightWidget, NULL,
			RES_CONVERT( XmNlabelString, "" ),
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, menu6,
			NULL );
	UxPutContext( menu5, (char *) UxModifyUserDialogContext );

	menu5_p1_shell = XtVaCreatePopupShell ("menu5_p1_shell",
			xmMenuShellWidgetClass, form2,
			XmNwidth, 1,
			XmNheight, 1,
			XmNallowShellResize, TRUE,
			XmNoverrideRedirect, TRUE,
			NULL );

	menu5_p1 = XtVaCreateWidget( "menu5_p1",
			xmRowColumnWidgetClass,
			menu5_p1_shell,
			XmNrowColumnType, XmMENU_PULLDOWN,
			NULL );
	UxPutContext( menu5_p1, (char *) UxModifyUserDialogContext );

	menu5_p1_b1 = XtVaCreateManagedWidget( "menu5_p1_b1",
			xmPushButtonGadgetClass,
			menu5_p1,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 418, "None") ),
			NULL );
	UxPutContext( menu5_p1_b1, (char *) UxModifyUserDialogContext );

	menu5_p1_b2 = XtVaCreateManagedWidget( "menu5_p1_b2",
			xmPushButtonGadgetClass,
			menu5_p1,
			RES_CONVERT( XmNlabelString, "1993" ),
			NULL );
	UxPutContext( menu5_p1_b2, (char *) UxModifyUserDialogContext );

	menu5_p1_b3 = XtVaCreateManagedWidget( "menu5_p1_b3",
			xmPushButtonGadgetClass,
			menu5_p1,
			RES_CONVERT( XmNlabelString, "1994" ),
			NULL );
	UxPutContext( menu5_p1_b3, (char *) UxModifyUserDialogContext );

	menu5_p1_b4 = XtVaCreateManagedWidget( "menu5_p1_b4",
			xmPushButtonGadgetClass,
			menu5_p1,
			RES_CONVERT( XmNlabelString, "1995" ),
			NULL );
	UxPutContext( menu5_p1_b4, (char *) UxModifyUserDialogContext );

	menu5_p1_b5 = XtVaCreateManagedWidget( "menu5_p1_b5",
			xmPushButtonGadgetClass,
			menu5_p1,
			RES_CONVERT( XmNlabelString, "1996" ),
			NULL );
	UxPutContext( menu5_p1_b5, (char *) UxModifyUserDialogContext );

	menu5_p1_b6 = XtVaCreateManagedWidget( "menu5_p1_b6",
			xmPushButtonGadgetClass,
			menu5_p1,
			RES_CONVERT( XmNlabelString, "1997" ),
			NULL );
	UxPutContext( menu5_p1_b6, (char *) UxModifyUserDialogContext );

	menu5_p1_b7 = XtVaCreateManagedWidget( "menu5_p1_b7",
			xmPushButtonGadgetClass,
			menu5_p1,
			RES_CONVERT( XmNlabelString, "1998" ),
			NULL );
	UxPutContext( menu5_p1_b7, (char *) UxModifyUserDialogContext );

	menu5_p1_b8 = XtVaCreateManagedWidget( "menu5_p1_b8",
			xmPushButtonGadgetClass,
			menu5_p1,
			RES_CONVERT( XmNlabelString, "1999" ),
			NULL );
	UxPutContext( menu5_p1_b8, (char *) UxModifyUserDialogContext );

	menu5_p1_b9 = XtVaCreateManagedWidget( "menu5_p1_b9",
			xmPushButtonGadgetClass,
			menu5_p1,
			RES_CONVERT( XmNlabelString, "2000" ),
			NULL );
	UxPutContext( menu5_p1_b9, (char *) UxModifyUserDialogContext );

	menu5_p1_b10 = XtVaCreateManagedWidget( "menu5_p1_b10",
			xmPushButtonGadgetClass,
			menu5_p1,
			RES_CONVERT( XmNlabelString, "2001" ),
			NULL );
	UxPutContext( menu5_p1_b10, (char *) UxModifyUserDialogContext );

	menu5_p1_b11 = XtVaCreateManagedWidget( "menu5_p1_b11",
			xmPushButtonGadgetClass,
			menu5_p1,
			RES_CONVERT( XmNlabelString, "2002" ),
			NULL );
	UxPutContext( menu5_p1_b11, (char *) UxModifyUserDialogContext );

	menu4 = XtVaCreateManagedWidget( "menu4",
			xmRowColumnWidgetClass,
			form2,
			XmNrowColumnType, XmMENU_OPTION,
			XmNsubMenuId, menu5_p1,
			XmNnavigationType, XmTAB_GROUP,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, textField8,
			XmNrightAttachment, XmATTACH_NONE,
			XmNrightOffset, 0,
			XmNleftAttachment, XmATTACH_WIDGET,
			RES_CONVERT( XmNlabelString, "" ),
			XmNleftWidget, menu5,
			NULL );
	UxPutContext( menu4, (char *) UxModifyUserDialogContext );

	label22 = XtVaCreateManagedWidget( "label22",
			xmLabelWidgetClass,
			form2,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 419, "Warning:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, menu5,
			XmNtopOffset, 12,
			NULL );
	UxPutContext( label22, (char *) UxModifyUserDialogContext );

	textField9 = XtVaCreateManagedWidget( "textField9",
			xmTextFieldWidgetClass,
			form2,
			XmNvalue, "",
			XmNmarginHeight, 1,
			XmNcolumns, 8,
			XmNmaxLength, 5,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, label22,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, label22,
			NULL );
	UxPutContext( textField9, (char *) UxModifyUserDialogContext );

	label21 = XtVaCreateManagedWidget( "label21",
			xmLabelWidgetClass,
			form2,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 420, "days") ),
			XmNalignment, XmALIGNMENT_BEGINNING,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, textField9,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, label22,
			NULL );
	UxPutContext( label21, (char *) UxModifyUserDialogContext );

	label23 = XtVaCreateManagedWidget( "label23",
			xmLabelWidgetClass,
			form2,
			XmNalignment, XmALIGNMENT_BEGINNING,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 421, "HOME DIRECTORY") ),
			XmNleftAttachment, XmATTACH_FORM,
			XmNleftOffset, 12,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopOffset, 8,
			XmNtopWidget, label22,
			NULL );
	UxPutContext( label23, (char *) UxModifyUserDialogContext );

	label25 = XtVaCreateManagedWidget( "label25",
			xmLabelWidgetClass,
			form2,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 422, "Path:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, label23,
			XmNtopOffset, 8,
			XmNleftAttachment, XmATTACH_FORM,
			NULL );
	UxPutContext( label25, (char *) UxModifyUserDialogContext );

	textField10 = XtVaCreateManagedWidget( "textField10",
			xmTextFieldWidgetClass,
			form2,
			XmNresizeWidth, FALSE,
			XmNvalue, "",
			XmNmarginHeight, 1,
			XmNmaxLength, 80,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, label25,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNrightAttachment, XmATTACH_POSITION,
			XmNrightPosition, 80,
			XmNbottomWidget, label25,
			NULL );
	UxPutContext( textField10, (char *) UxModifyUserDialogContext );


	/* align labels */
	wlist[0] = label5;
	wlist[1] = label6;
	wlist[2] = label7;
	wlist[3] = label8;
	wlist[4] = label9;
	wlist[5] = label11;
	wlist[6] = label16;
	wlist[7] = label17;
	wlist[8] = label18;
	wlist[9] = label19;
	wlist[10] = label20;
	wlist[11] = label22;
	wlist[12] = label25;
	wnum = 13;
	for (i=0; i<wnum; i++) {
		XtVaGetValues(wlist[i],
			XmNwidth, &width,
			NULL);
		if (width > maxwidth) {
			maxwidth = width;
			maxlabel = wlist[i];
		}
	}
	for (i=0; i<wnum; i++) {
		XtVaSetValues(wlist[i],
			XmNwidth, maxwidth,
			NULL);
	}

	create_button_box(form2, label25, UxModifyUserDialogContext,
		&pushButton4, &pushButton10, &pushButton1,
		&pushButton5, &pushButton6);

	XtVaSetValues(form2, XmNinitialFocus, textField1, NULL);

	XtAddCallback( menu1_p1_b1, XmNactivateCallback,
		(XtCallbackProc) modifyUserLoginShellBourneCB,
		(XtPointer) UxModifyUserDialogContext );

	XtAddCallback( menu1_p1_b2, XmNactivateCallback,
		(XtCallbackProc) modifyUserLoginShellCCB,
		(XtPointer) UxModifyUserDialogContext );

	XtAddCallback( menu1_p1_b3, XmNactivateCallback,
		(XtCallbackProc) modifyUserLoginShellKornCB,
		(XtPointer) UxModifyUserDialogContext );

	XtAddCallback( menu1_p1_b4, XmNactivateCallback,
		(XtCallbackProc) modifyUserLoginShellOtherCB,
		(XtPointer) UxModifyUserDialogContext );

	XtAddCallback( menu3_p1_b1, XmNactivateCallback,
		(XtCallbackProc) passwdTypeCB,
		(XtPointer) UxModifyUserDialogContext );
	XtAddCallback( menu3_p1_b2, XmNactivateCallback,
		(XtCallbackProc) passwdTypeCB,
		(XtPointer) UxModifyUserDialogContext );
	XtAddCallback( menu3_p1_b3, XmNactivateCallback,
		(XtCallbackProc) passwdTypeCB,
		(XtPointer) UxModifyUserDialogContext );
	XtAddCallback( menu3_p1_b4, XmNactivateCallback,
		(XtCallbackProc) passwdCB,
		(XtPointer) UxModifyUserDialogContext );

	XtAddCallback( menu4_p1_b1, XmNactivateCallback,
		(XtCallbackProc) modifyUserSetDateNoneCB,
		(XtPointer) UxModifyUserDialogContext );

	XtAddCallback( pushButton1, XmNactivateCallback,
		(XtCallbackProc) resetCB,
		(XtPointer) UxModifyUserDialogContext );

	XtAddCallback( menu5_p1_b12, XmNactivateCallback,
		(XtCallbackProc) modifyUserSetDateNoneCB,
		(XtPointer) UxModifyUserDialogContext );

	XtAddCallback( pushButton4, XmNactivateCallback,
		(XtCallbackProc) modifyCB,
		(XtPointer) UxModifyUserDialogContext );

	XtAddCallback( pushButton10, XmNactivateCallback,
		(XtCallbackProc) modifyCB,
		(XtPointer) UxModifyUserDialogContext );

	XtAddCallback( pushButton5, XmNactivateCallback,
		(XtCallbackProc) cancelCB,
		(XtPointer) UxModifyUserDialogContext );

	XtAddCallback( pushButton6, XmNactivateCallback,
		(XtCallbackProc) helpCB,
		"user_window.r.hlp" );

	XtAddCallback( menu5_p1_b1, XmNactivateCallback,
		(XtCallbackProc) modifyUserSetDateNoneCB,
		(XtPointer) UxModifyUserDialogContext );


	XtAddCallback( modifyUserDialog, XmNdestroyCallback,
		(XtCallbackProc) UxDestroyContextCB,
		(XtPointer) UxModifyUserDialogContext);


	return ( modifyUserDialog );
}

/*******************************************************************************
       The following is the 'Interface function' which is the
       external entry point for creating this interface.
       This function should be called from your application or from
       a callback function.
*******************************************************************************/

Widget	create_modifyUserDialog(Widget parent)
{
	Widget                  rtrn;
	_UxCmodifyUserDialog    *UxContext;
	static int		_Uxinit = 0;

	UxModifyUserDialogContext = UxContext =
		(_UxCmodifyUserDialog *) UxNewContext( sizeof(_UxCmodifyUserDialog), False );

	/* null structure */
	memset(&UxContext->user, 0, sizeof(SysmanUserArg));

	if ( ! _Uxinit )
	{
		UxLoadResources( "modifyUserDialog.rf" );
		_Uxinit = 1;
	}

	rtrn = _Uxbuild_modifyUserDialog(parent);

	return(rtrn);
}

void
show_modifyuserdialog(
	Widget parent,
	SysmanUserArg*	user,
	sysMgrMainCtxt * ctxt
)
{
	_UxCmodifyUserDialog       *UxContext;


	SetBusyPointer(True);

	if (modifyuserdialog == NULL)
		modifyuserdialog = create_modifyUserDialog(parent);

	ctxt->currDialog = modifyuserdialog;
        UxContext = (_UxCmodifyUserDialog *) UxGetContext( modifyuserdialog );

	free_user(&UxContext->user);
	copy_user(&UxContext->user, user);

	if (modifyUserInit(modifyuserdialog, &UxContext->user)) {
		UxPopupInterface(modifyuserdialog, no_grab);

		if (USR_IS_ROOT(user->username, user->uid)) {
			display_warning(modifyuserdialog,
				ROOT_MODIFY_CAUTION_MSG);

			/* OK, Apply, Reset */
			XtVaSetValues(UxContext->UxpushButton4,
			    XtNsensitive, FALSE, NULL);
			XtVaSetValues(UxContext->UxpushButton10,
			    XtNsensitive, FALSE, NULL);
			XtVaSetValues(UxContext->UxpushButton1,
			    XtNsensitive, FALSE, NULL);
		} else {
			XtVaSetValues(UxContext->UxpushButton4,
			    XtNsensitive, TRUE, NULL);
			XtVaSetValues(UxContext->UxpushButton10,
			    XtNsensitive, TRUE, NULL);
			XtVaSetValues(UxContext->UxpushButton1,
			    XtNsensitive, TRUE, NULL);
		}
	}
	
	SetBusyPointer(False);
}

/*******************************************************************************
       END OF FILE
*******************************************************************************/

