
/* Copyright 1993 Sun Microsystems, Inc. */

#pragma ident "@(#)password.c	1.8 96/03/11 Sun Microsystems"

/*	password.c	*/

#include <nl_types.h>
#include <limits.h>
#include <Xm/Xm.h>
#include <Xm/MessageB.h>
#include <Xm/DialogS.h>
#include <Xm/SeparatoG.h>
#include <Xm/TextF.h>
#include <Xm/Label.h>
#include "UxXt.h"

#include "util.h"
#include "add_user.h"


#define PASSWORDS_NULL  \
catgets(_catd, 8, 427, "You entered a NULL password. Please enter the password again.")

#define PASSWORDS_DONT_MATCH	\
catgets(_catd, 8, 428, "You entered two different passwords. Please enter the password again.")

#define	NO_ANSWER	-1
#define	ANSWER_OK	 1
#define	ANSWER_CANCEL	 0

typedef	struct
{
	Widget	passwordDialog;
	Widget	passwordLabel;
	Widget	passwordText;
	Widget	verifyLabel;
	Widget	verifyText;
	Widget	buttonBox;
	Widget	separator;
	Widget	okPushbutton;
	Widget	cancelPushbutton;
	Widget	helpPushbutton;
	Widget  menuHistory;
	Widget  optionMenu;
} passwordCtxt;

static Widget		passworddialog;
static char		password[PASS_MAX + 1];
static char		passwordVerify[PASS_MAX + 1];
char*			UserPassword;
int*			UserEnteredPassword;

extern nl_catd	_catd;	/* for catgets(), defined in main.c */


/*******************************************************************************
       The following are callback functions.
*******************************************************************************/

static void
okCB(
	Widget wgt, 
	XtPointer cd, 
	XtPointer cbs)
{
	passwordCtxt*	ctxt;
	extern Cursor	busypointer;
	int		passwd_null = FALSE;
	char*		msg;
	int		error = FALSE;

	XtVaGetValues(passworddialog,
		XmNuserData, &ctxt,
		NULL);

	SetBusyPointer(True);
	XDefineCursor(Gdisplay, XtWindow(XtParent(passworddialog)), busypointer);

	if (password[0] == '\0' || passwordVerify[0] == '\0') {
	        /* Password not provided. */
		error = TRUE;
	        passwd_null = TRUE;
	}
	else if (strcmp(password, passwordVerify) != 0) {
		/* password not verified correctly. */
		error = TRUE;
	}
	else  {
		error = FALSE;
	}

	if (error) {
		/* Clear the text and password fields. */
		*UserEnteredPassword = FALSE;
		UserPassword[0] = NULL;
		XmTextSetString(ctxt->passwordText, "");
		XmTextSetString(ctxt->verifyText, "");
		password[0] = '\0';
		passwordVerify[0] = '\0';
	
		/* Ask the user to try again! */
		if (passwd_null) {
			msg = PASSWORDS_NULL;
		}
		else {
			msg = PASSWORDS_DONT_MATCH;
		}
		display_error(ctxt->passwordDialog, msg);
	} else {
		*UserEnteredPassword = TRUE;
		if (password == NULL)
			stringcopy(UserPassword, "", PASS_MAX+1 );
		else
			stringcopy(UserPassword, password, PASS_MAX+1 );
		
		XmTextSetString(ctxt->passwordText, "");
		XmTextSetString(ctxt->verifyText, "");
		password[0] = '\0';
		passwordVerify[0] = '\0';
		
		XtPopdown(XtParent(passworddialog));
	}

	SetBusyPointer(False);
	XUndefineCursor(Gdisplay, XtWindow(XtParent(passworddialog)));
}

static void
cancelCB(
	Widget wgt, 
	XtPointer cd, 
	XtPointer cbs)
{
	passwordCtxt * ctxt = (passwordCtxt *)cd;
	
	XtVaSetValues(ctxt->optionMenu, 
			XmNmenuHistory, ctxt->menuHistory, 
			NULL);
	XtPopdown(XtParent(passworddialog));
	
}

static void
passwordVerifyCB(
	Widget wgt, 
	char* password_str, 
	XmTextVerifyCallbackStruct *cbs)
{
	int len;

	if (cbs->event == NULL)	/* called because XmTextSetString was called */
		return;

	if (cbs->text->ptr == NULL) {	/* backspace */
		/* delete from here to end. */
		cbs->endPos = strlen(password_str);
		password_str[cbs->startPos] = '\0';
		return;
	}

	if (cbs->text->length > 1) {
		cbs->doit = FALSE; 	/* Don't allow paste. */
		return;
	}

	if ((strlen(password_str)-(cbs->endPos-cbs->startPos)) < PASS_MAX) {
		if (cbs->endPos > cbs->startPos) {
			/* text was selected for replacement */
			cbs->endPos = strlen(password_str);
			password_str[cbs->startPos] = '\0';
		}
		strncat(password_str, cbs->text->ptr, cbs->text->length);
		password_str[cbs->endPos + cbs->text->length] = '\0';
	}
	else {
		cbs->doit = FALSE;
		return;
	}

	for (len=0; len < cbs->text->length; len++) {
		cbs->text->ptr[len] = '*';
	}
}


/*******************************************************************************
       The 'build_' function creates all the widgets
       using the resource values specified in the Property Editor.
*******************************************************************************/

static Widget
build_passwordDialog(Widget parent)
{
	passwordCtxt*	ctxt;
	Widget		shell;


	ctxt = (passwordCtxt*) malloc(sizeof(passwordCtxt));

	if (parent == NULL)
	{
		parent = GtopLevel;
	}

	shell = XtVaCreatePopupShell( "PasswordDialog_shell",
			xmDialogShellWidgetClass, parent,
			XmNshellUnitType, XmPIXELS,
			XmNhorizontalSpacing, 10,
			XmNverticalSpacing, 10,
			NULL );

	ctxt->passwordDialog = XtVaCreateWidget( "passwordDialog",
			xmFormWidgetClass,
			shell,
			RES_CONVERT(XmNdialogTitle, catgets(_catd, 8, 429, "Set User Password")),
			XmNunitType, XmPIXELS,
			XmNautoUnmanage, False,
			XmNnoResize, TRUE,
			XmNdialogStyle,	XmDIALOG_FULL_APPLICATION_MODAL,
			NULL );

	XtVaSetValues(ctxt->passwordDialog,
		XmNuserData, (XtPointer)ctxt,
		NULL);

	ctxt->passwordLabel = XtVaCreateManagedWidget( "passwordLabel",
			xmLabelWidgetClass,
			ctxt->passwordDialog,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 430, "Enter Password:") ),
			XmNalignment, XmALIGNMENT_BEGINNING,
			XmNtopAttachment, XmATTACH_FORM,
			XmNtopOffset, 10,
			XmNleftAttachment, XmATTACH_FORM,
			XmNleftOffset, 10,
			NULL );

	ctxt->passwordText = XtVaCreateManagedWidget( "passwordText",
			xmTextFieldWidgetClass,
			ctxt->passwordDialog,
			XmNmaxLength, PASS_MAX,
			XmNcolumns, 30,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, ctxt->passwordLabel,
			XmNleftAttachment, XmATTACH_FORM,
			XmNleftOffset, 10,
			XmNrightAttachment, XmATTACH_FORM,
			XmNrightOffset, 10,
			NULL );

	ctxt->verifyLabel = XtVaCreateManagedWidget( "verifyLabel",
			xmLabelWidgetClass,
			ctxt->passwordDialog,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 431, "Verify Password:") ),
			XmNalignment, XmALIGNMENT_BEGINNING,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, ctxt->passwordText,
			XmNtopOffset, 10,
			XmNleftAttachment, XmATTACH_FORM,
			XmNleftOffset, 10,
			NULL );

	ctxt->verifyText = XtVaCreateManagedWidget( "verifyText",
			xmTextFieldWidgetClass,
			ctxt->passwordDialog,
			XmNmaxLength, PASS_MAX,
			XmNcolumns, 30,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, ctxt->verifyLabel,
			XmNleftAttachment, XmATTACH_FORM,
			XmNleftOffset, 10,
			XmNrightAttachment, XmATTACH_FORM,
			XmNrightOffset, 10,
			NULL );

	XtAddCallback( ctxt->passwordText, XmNmodifyVerifyCallback,
		(XtCallbackProc) passwordVerifyCB,
		(XtPointer) &password );
	XtAddCallback( ctxt->verifyText, XmNmodifyVerifyCallback,
		(XtCallbackProc) passwordVerifyCB,
		(XtPointer) &passwordVerify );

	create_button_box(ctxt->passwordDialog, ctxt->verifyText, NULL,
		&ctxt->okPushbutton, NULL, NULL,
		&ctxt->cancelPushbutton, &ctxt->helpPushbutton);

	XtVaSetValues(ctxt->passwordDialog,
		XmNinitialFocus, ctxt->passwordText,
		NULL);

	XtAddCallback( ctxt->okPushbutton, XmNactivateCallback,
		(XtCallbackProc) okCB,
		(XtPointer) ctxt );
	XtAddCallback( ctxt->cancelPushbutton, XmNactivateCallback,
		(XtCallbackProc) cancelCB,
		(XtPointer) ctxt );
	XtAddCallback( ctxt->helpPushbutton, XmNactivateCallback,
		(XtCallbackProc) helpCB,
		"user_passwd_setting.t.hlp" );


	return ( ctxt->passwordDialog );
}


int
show_password_dialog(
	Widget	parent,
	char*	pw,
	int*	pw_set,
	Widget	menu,
	Widget	menuHistory
)
{
	passwordCtxt*	ctxt;

	parent = (parent == NULL) ? GtopLevel : get_shell_ancestor(parent);
	if (passworddialog == NULL)
		passworddialog = build_passwordDialog(parent);

	XtVaGetValues(passworddialog,
		XmNuserData, &ctxt,
		NULL);

	ctxt->optionMenu = menu;
	ctxt->menuHistory = menuHistory;

	XmTextSetString(ctxt->passwordText, "");
	XmTextSetString(ctxt->verifyText, "");
	
	XmProcessTraversal(passworddialog, XmTRAVERSE_CURRENT);

	/* password and passwordVerify are used by both the modify and the add 
	 * password windows.  This can be done because the password dialogs are
	 * application modal and values are copied in the activate callback
	 * functions for each password dialog.
	 */
	password[0] = '\0';
	passwordVerify[0] = '\0';

	UserPassword = pw;
	UserEnteredPassword = pw_set;

	XtManageChild(passworddialog);
	XtPopup(XtParent(passworddialog), XtGrabNone);
}

