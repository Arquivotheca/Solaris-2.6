/* Copyright (c) 1993 Sun Microsystems, Inc. */

#pragma ident "@(#)add_user.c	1.24 96/04/22 Sun Microsystems"

/*******************************************************************************
	add_user.c
*******************************************************************************/

#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <libintl.h>
#include <nl_types.h>
#include <Xm/Xm.h>
#include <Xm/MenuShell.h>
#include <Xm/Separator.h>
#include <Xm/ToggleB.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/TextF.h>
#include <Xm/Label.h>
#include <Xm/Text.h>
#include <Xm/Form.h>
#include <Xm/DialogS.h>
#include <Xm/List.h>
#include <Xm/MessageB.h>

#include "UxXt.h"
#include "util.h"
#include "valid.h"
#include "sysman_iface.h"
#include "sysman_codes.h"

#define ROOT_ADD_NOTICE_MSG		\
	catgets(_catd, 8, 98, "Admintool cannot be used to add a user account with user id 0 " \
	"or user name root.")

extern nl_catd	_catd;	/* for catgets(), defined in main.c */

int	day_limit[13] = {
	 0,  /* None      */
	31,  /* January   */
	28,  /* February  */
	31,  /* March     */
	30,  /* April     */
	31,  /* May       */
	30,  /* June      */
	31,  /* July      */
	31,  /* August    */
	30,  /* September */
	31,  /* October   */
	30,  /* November  */
	31   /* December  */
	};

void addUserLoginShellCCB(
		Widget widget, 
		XtPointer, 
		XtPointer
	);
void addUserLoginShellBourneCB(
		Widget widget, 
		XtPointer, 
		XtPointer
	);
void addUserLoginShellKornCB(
		Widget widget, 
		XtPointer, 
		XtPointer
	);

Widget 		adduserdialog = NULL;
char		AddPassword[PASS_MAX + 1];
int		UserEnteredAddPassword;


/*******************************************************************************
       The following header file defines the context structure.
*******************************************************************************/

#define CONTEXT_MACRO_ACCESS 1
#include "add_user.h"
#undef CONTEXT_MACRO_ACCESS


/*******************************************************************************
       The following are Auxiliary functions.
*******************************************************************************/

void
addUserInit(Widget widget)
{

	int				owner_read;
	int				owner_write;
	int				owner_exec;
	int				group_read;
	int				group_write;
	int				group_exec;
	int				world_read;
	int				world_write;
	int				world_exec;
	Widget				password_item_value;
	int				day=0;
	int				month=0;
	int				year=0;
	int				permission;
	char				*db_err;
	int				status;
	uid_t				next_avail_uid;
	char				uid_str[32];
	_UxCaddUserDialog		*addUserWin;

	addUserWin = (_UxCaddUserDialog *) UxGetContext(widget);


	UserEnteredAddPassword = FALSE;
	AddPassword[0] = NULL;

	XmTextSetString(addUserWin->UxtextField22, "");

	next_avail_uid = sysman_get_next_avail_uid();
	sprintf(uid_str, "%d", next_avail_uid);
	XmTextSetString(addUserWin->UxtextField21, uid_str);

	XmTextSetString(addUserWin->UxtextField20, "10");
	XmTextSetString(addUserWin->UxtextField19, "");
	XmTextSetString(addUserWin->UxtextField18, "");

	XtVaSetValues(addUserWin->Uxmenu8, 
		XmNmenuHistory, addUserWin->Uxmenu1_p1_b5,
		NULL);
	addUserLoginShellBourneCB(addUserWin->Uxmenu1_p1_b5, NULL, NULL);

	XtVaSetValues(addUserWin->Uxmenu9, 
		XmNmenuHistory, addUserWin->Uxmenu3_p1_b5,
		XmNuserData, addUserWin->Uxmenu3_p1_b5,
		NULL);

	XmTextSetString(addUserWin->UxtextField23, "");
	XmTextSetString(addUserWin->UxtextField24, "");
	XmTextSetString(addUserWin->UxtextField25, "");
	XmTextSetString(addUserWin->UxtextField26, "");

	XtVaSetValues(addUserWin->Uxmenu11, 
		XmNmenuHistory, addUserWin->Uxmenu4_p1_b33,
		NULL);
	XtVaSetValues(addUserWin->Uxmenu10, 
		XmNmenuHistory, addUserWin->Uxmenu5_p1_b36,
		NULL);
	XtVaSetValues(addUserWin->Uxmenu7, 
		XmNmenuHistory, addUserWin->Uxmenu5_p1_b25,
		NULL);

	XmToggleButtonSetState(addUserWin->UxtoggleButton22, True, True);
	XmTextSetString(addUserWin->UxtextField14, "");

}


/*******************************************************************************
       The following are callback functions.
*******************************************************************************/

void
addUserLoginShellBourneCB(
		Widget widget, 
		XtPointer cd, 
		XtPointer cbs)
{
	_UxCaddUserDialog	*addUserWin;
	XmString		str;

	addUserWin = (_UxCaddUserDialog *) UxGetContext(widget);

	XtVaSetValues(addUserWin->UxtextField35, 
		XmNmappedWhenManaged, FALSE,
		NULL);
	str = XmStringCreateLocalized(SHELL_BOURNE_PATH);
	XtVaSetValues(addUserWin->Uxlabel53, 
		XmNmappedWhenManaged, TRUE,
		XmNlabelString, str,
		NULL);
	XmStringFree(str);
}

void
addUserLoginShellCCB(
		Widget widget, 
		XtPointer cd, 
		XtPointer cbs)
{
	_UxCaddUserDialog	*addUserWin;
	XmString		str;

	addUserWin = (_UxCaddUserDialog *) UxGetContext(widget);

	XtVaSetValues(addUserWin->UxtextField35, 
		XmNmappedWhenManaged, FALSE,
		NULL);
	str = XmStringCreateLocalized(SHELL_C_PATH);
	XtVaSetValues(addUserWin->Uxlabel53, 
		XmNmappedWhenManaged, TRUE,
		XmNlabelString, str,
		NULL);
	XmStringFree(str);
}

void
addUserLoginShellKornCB(
		Widget widget, 
		XtPointer cd, 
		XtPointer cbs)
{

	_UxCaddUserDialog	*addUserWin;
	XmString		str;

	addUserWin = (_UxCaddUserDialog *) UxGetContext(widget);

	XtVaSetValues(addUserWin->UxtextField35, 
		XmNmappedWhenManaged, FALSE,
		NULL);
	str = XmStringCreateLocalized(SHELL_KORN_PATH);
	XtVaSetValues(addUserWin->Uxlabel53, 
		XmNmappedWhenManaged, TRUE,
		XmNlabelString, str,
		NULL);
	XmStringFree(str);
}

void
addUserLoginShellOtherCB(
		Widget widget, 
		XtPointer cd, 
		XtPointer cbs)
{

	_UxCaddUserDialog	*addUserWin;

	addUserWin = (_UxCaddUserDialog *) UxGetContext(widget);

	XtVaSetValues(addUserWin->UxtextField35, 
		XmNmappedWhenManaged, TRUE,
		NULL);
	XmTextSetString(addUserWin->UxtextField35, SHELL_OTHER_PATH);
	XmTextFieldSetInsertionPosition(addUserWin->UxtextField35,
		strlen(SHELL_OTHER_PATH)); 
		
	XtVaSetValues(addUserWin->Uxlabel53, 
		XmNmappedWhenManaged, FALSE,
		NULL);
}


void
addUserSetDateNoneCB(
		Widget widget, 
		XtPointer cd, 
		XtPointer cbs)
{
	_UxCaddUserDialog	*addUserWin;

	addUserWin = (_UxCaddUserDialog *) UxGetContext(widget);

	XtVaSetValues(addUserWin->Uxmenu11, 
		XmNmenuHistory, addUserWin->Uxmenu4_p1_b33,
		NULL);
	XtVaSetValues(addUserWin->Uxmenu10, 
		XmNmenuHistory, addUserWin->Uxmenu5_p1_b36,
		NULL);
	XtVaSetValues(addUserWin->Uxmenu7, 
		XmNmenuHistory, addUserWin->Uxmenu5_p1_b25,
		NULL);
}


static	void	passwdTypeCB(
			Widget wgt, 
			XtPointer cd, 
			XtPointer cb)
{
	_UxCaddUserDialog       *UxSaveCtx, *UxContext;
	Widget                  UxWidget = wgt;
	XtPointer               UxClientData = cd;
	XtPointer               UxCallbackArg = cb;

	UxSaveCtx = UxAddUserDialogContext;
	UxAddUserDialogContext = UxContext =
			(_UxCaddUserDialog *) UxGetContext( UxWidget );
	{
		/* Keep track of the last selected password type. */
		XtVaSetValues(UxContext->Uxmenu9, 
			XmNuserData, UxWidget,
			NULL);
	
	}
	UxAddUserDialogContext = UxSaveCtx;
}

static	void	passwdCB(
			Widget wgt, 
			XtPointer cd, 
			XtPointer cb)
{
	Widget			password_widget;
	_UxCaddUserDialog 	* addUserWin;
	extern show_password_dialog(Widget parent, char* pw, 
			int* pw_set, Widget menu, Widget menuHistory);

	addUserWin = (_UxCaddUserDialog *)UxGetContext(adduserdialog);

	XtVaGetValues(addUserWin->Uxmenu9, XmNuserData, &password_widget, NULL);

	show_password_dialog(adduserdialog,
		AddPassword, &UserEnteredAddPassword, 
		addUserWin->Uxmenu9, password_widget);
}


static	void	addCB(
			Widget widget, 
			XtPointer cd, 
			XtPointer cb)
{
	Widget                  UxWidget = widget;
	XtPointer               UxClientData = cd;
	XtPointer               UxCallbackArg = cb;

	{
	char *		username_p;
	char *		uid_p;
	char *		gid_p;
	char *		groups_p;
	char *		gcos_p;
	char *		shell_p;
	char *		min_p;
	char *		max_p;
	char *		inactive_p;
	char *		expire_p;
	char *		warn_p;
	char *		home_path_p;
	char *		skel_path_p;
	char *		mail_server_p;
	char *			passwd_p;
	char *			home_mode_p;
	char *  		charPtr;
	char *  		charPtr2;
	int			createHD_set,	autohome_set;
	int			cred_set;
	int			Oread_set,	Owrite_set,	Oexec_set;
	int			Gread_set,	Gwrite_set,	Gexec_set;
	int			Wread_set,	Wwrite_set,	Wexec_set;
	int			permiss_numeric;
	int			num_entries_to_move;
	int			bytes_to_move;
	int			rownum;
	int			i;
	medstr			text;
	shortstr		home_mode_str;
	shortstr		expire_str;
	medstr			scrolling_list_str;
	int			the_day;
	int			the_month;
	int			the_year;
	XmString		motifStr;
	Widget			shellWidget;
	Widget			pwWidget;
	Widget			expWidget;
	int			root_mod=FALSE;
	volatile _UxCaddUserDialog	*addUserWin;
	extern void display_infomsg(Widget parent, char* const msg);

	extern void			add_user_to_list(SysmanUserArg* user);
	SysmanUserArg			user;
	int				sts;
	int			conflict_status;
	char			msgbuf[200];
	char *			normalized_uid_p;

	addUserWin = (volatile _UxCaddUserDialog *) UxGetContext(widget);

	SetBusyPointer(True);

	/* Make the pointers point to their strings. Some of these pointers may
 	 * later be set to NULL, to satisfy the interface to usr_add_user.
	 */
	passwd_p      = NULL;
	home_mode_p   = home_mode_str;
	expire_p      = expire_str;

	/* Get values from items. */
	username_p    = XmTextGetString(addUserWin->UxtextField22);
	if (strlen(username_p) < (size_t)1) {
		display_error(adduserdialog,
				catgets(_catd, 8, 100, "User name must be at least 1 character long"));
		SetBusyPointer(False);	
		XtFree((char *)username_p);
		return;
	}

	uid_p = XmTextGetString(addUserWin->UxtextField21);
	normalized_uid_p = normalize_uid(uid_p);
	if (normalized_uid_p != NULL) {
		strcpy(uid_p, normalized_uid_p);
		free((void*)normalized_uid_p);
	}

	/* Warn the user if a root user is to be added. */
	if (USR_IS_ROOT(username_p, uid_p)) {
		display_warning(adduserdialog, ROOT_ADD_NOTICE_MSG);
		/* DO NOT perform the ADD. */
		SetBusyPointer(False);
		XtFree((char *)username_p);
		XtFree((char *)uid_p);
		return;
	}



	if ( conflict_status = check_ns_user_conflicts(username_p, atol(uid_p) ) ) {
		if (conflict_status == SYSMAN_CONFLICT_BOTH_USED) {
			sprintf(msgbuf, "%s",  
                		catgets(_catd, 8, 648,
				"the user and uid entries are being used in the name service user map"));
			if (!Confirm(adduserdialog, msgbuf, NULL, catgets(_catd, 8, 641, "OK"))) {
				/* DO NOT perform the ADD. */
				SetBusyPointer(False);	
				XtFree((char *)username_p);
				return ;
			}
		}
		else if (conflict_status == SYSMAN_CONFLICT_NAME_USED) {
			sprintf(msgbuf, "%s",  
                		catgets(_catd, 8, 649,
				"this user name is already being used in the the name service user map"));
			if (!Confirm(adduserdialog, msgbuf, NULL, catgets(_catd, 8, 641, "OK"))) {
				/* DO NOT perform the ADD. */
				SetBusyPointer(False);	
				XtFree((char *)username_p);
				return ;
			}
		}
		else if (conflict_status == SYSMAN_CONFLICT_ID_USED) {
			sprintf(msgbuf, "%s",  
                		catgets(_catd, 8, 650,
				"this uid number is already being used in the name service user map"));
			if (!Confirm(adduserdialog, msgbuf, NULL, catgets(_catd, 8, 641, "OK"))) {
				/* DO NOT perform the ADD. */
				SetBusyPointer(False);	
				XtFree((char *)username_p);
				return ;
			}
		}
	}
	gid_p         = XmTextGetString(addUserWin->UxtextField20);
	groups_p      = XmTextGetString(addUserWin->UxtextField19);
	gcos_p        = XmTextGetString(addUserWin->UxtextField18);

	XtVaGetValues(addUserWin->Uxmenu8, 
		XmNmenuHistory, &shellWidget,
		NULL);

	if (shellWidget == addUserWin->Uxmenu1_p1_b5)
		shell_p = SHELL_BOURNE_PATH;
	else if (shellWidget == addUserWin->Uxmenu1_p1_b6)
		shell_p = SHELL_C_PATH;
	else if (shellWidget == addUserWin->Uxmenu1_p1_b7)
		shell_p = SHELL_KORN_PATH;
	else if (shellWidget == addUserWin->Uxmenu1_p1_b8)
		shell_p = XmTextGetString(addUserWin->UxtextField35);

	/*
	 * Get the password type
	 */
	XtVaGetValues(addUserWin->Uxmenu9, 
		XmNmenuHistory, &pwWidget,
		NULL);

	if (pwWidget == addUserWin->Uxmenu3_p1_b5) {

   		passwd_p = PASSWD_CLEARED;
	}
	else if (pwWidget == addUserWin->Uxmenu3_p1_b6) {

    		passwd_p = PASSWD_LOCKED;
	}
	else if (pwWidget == addUserWin->Uxmenu3_p1_b7) {

    		passwd_p = PASSWD_NONE;
	}
	else if (pwWidget == addUserWin->Uxmenu3_p1_b8) {

    		passwd_p = AddPassword;
	}
	else {

    		/* Unknown password type; should raise an exception */
		/*
    		usr_password_type = User::password_blank;
		*/
    		passwd_p = NULL;
	}

	/* Get minimum days required between password changes */
	min_p = XmTextGetString(addUserWin->UxtextField23);

	/* Get maximum days required between password changes */
	max_p = XmTextGetString(addUserWin->UxtextField24);

	/* Get the number of days of inactivity allowed for the user */
	inactive_p = XmTextGetString(addUserWin->UxtextField25);

	XtVaGetValues(addUserWin->Uxmenu11, 
		XmNmenuHistory, &expWidget,
		NULL);
	XtVaGetValues(expWidget, 
		XmNlabelString, &motifStr,
		NULL);
	XmStringGetLtoR(motifStr, XmSTRING_DEFAULT_CHARSET, &charPtr);
	XmStringFree(motifStr);
	sscanf(charPtr, "%d", &the_day);
	XtFree(charPtr);

	XtVaGetValues(addUserWin->Uxmenu10, 
		XmNmenuHistory, &expWidget,
		NULL);

	if (expWidget == addUserWin->Uxmenu5_p1_b36)
		the_month = 0;
	else if (expWidget == addUserWin->Uxmenu5_p1_b37)
		the_month = 1;
	else if (expWidget == addUserWin->Uxmenu5_p1_b38)
		the_month = 2;
	else if (expWidget == addUserWin->Uxmenu5_p1_b39)
		the_month = 3;
	else if (expWidget == addUserWin->Uxmenu5_p1_b40)
		the_month = 4;
	else if (expWidget == addUserWin->Uxmenu5_p1_b41)
		the_month = 5;
	else if (expWidget == addUserWin->Uxmenu5_p1_b42)
		the_month = 6;
	else if (expWidget == addUserWin->Uxmenu5_p1_b43)
		the_month = 7;
	else if (expWidget == addUserWin->Uxmenu5_p1_b44)
		the_month = 8;
	else if (expWidget == addUserWin->Uxmenu5_p1_b45)
		the_month = 9;
	else if (expWidget == addUserWin->Uxmenu5_p1_b46)
		the_month = 10;
	else if (expWidget == addUserWin->Uxmenu5_p1_b47)
		the_month = 11;
	else if (expWidget == addUserWin->Uxmenu5_p1_b48)
		the_month = 12;
		
	the_day   = min2(the_day, day_limit[the_month]);

	XtVaGetValues(addUserWin->Uxmenu7, 
		XmNmenuHistory, &expWidget,
		NULL);
	XtVaGetValues(expWidget, 
		XmNlabelString, &motifStr,
		NULL);
	XmStringGetLtoR(motifStr, XmSTRING_DEFAULT_CHARSET, &charPtr2);
	XmStringFree(motifStr);
	if (expWidget == addUserWin->Uxmenu5_p1_b25)
		the_year = 0;
	else
		sscanf(charPtr2, "%d", &the_year);
	XtFree(charPtr2);

	format_the_date (the_day, the_month, the_year, expire_p);

	/* Get number of days prior to password expiration to warn user */
	warn_p = XmTextGetString(addUserWin->UxtextField26);

	createHD_set  = XmToggleButtonGetState(addUserWin->UxtoggleButton22);
	home_path_p   = XmTextGetString(addUserWin->UxtextField14);

	memset((void *)&user, 0, sizeof (user));

	user.username = username_p;
	user.uid = uid_p;
	user.passwd = passwd_p;
	user.group = gid_p;
	user.second_grps = groups_p;
	user.comment = gcos_p;
	user.home_dir_flag = (boolean_t)createHD_set;
	user.path = home_path_p;
	user.shell = shell_p;
	user.lastchanged = NULL;
	user.minimum = min_p;
	user.maximum = max_p;
	user.warn = warn_p;
	user.inactive = inactive_p;
	user.expire = expire_p;
	user.flag = NULL;
	user.username_key = username_p;

	sts = sysman_add_user(&user, errbuf, ERRBUF_SIZE);

	if (sts < 0) {
		display_error(adduserdialog, errbuf);
	}
	else {
		if (sts == SYSMAN_INFO) {
			display_infomsg(adduserdialog, errbuf);
		}
		add_user_to_list(&user);
		if (widget == addUserWin->UxpushButton4) {
			UxPopdownInterface(addUserWin->UxaddUserDialog);
		}
	}

	/* Free Motif compound strings. */
	XtFree((char *)username_p);
	XtFree((char *)uid_p);
	XtFree((char *)gcos_p);
	XtFree((char *)gid_p);
	XtFree((char *)groups_p);
	XtFree((char *)min_p);
	XtFree((char *)max_p);
	XtFree((char *)inactive_p);
	XtFree((char *)warn_p);
	XtFree((char *)home_path_p);

	SetBusyPointer(False);

	}
}



static	void	resetCB(
			Widget wgt, 
			XtPointer cd, 
			XtPointer cb)
{
	_UxCaddUserDialog       *UxSaveCtx, *UxContext;
	Widget                  UxWidget = wgt;
	XtPointer               UxClientData = cd;
	XtPointer               UxCallbackArg = cb;

	UxSaveCtx = UxAddUserDialogContext;
	UxAddUserDialogContext = UxContext =
			(_UxCaddUserDialog *) UxGetContext( UxWidget );
	{
	
	addUserInit(UxWidget);
	
	}
	UxAddUserDialogContext = UxSaveCtx;
}

static	void	cancelCB(
			Widget wgt, 
			XtPointer cd, 
			XtPointer cb)
{
	_UxCaddUserDialog       *UxSaveCtx, *UxContext;
	Widget                  UxWidget = wgt;
	XtPointer               UxClientData = cd;
	XtPointer               UxCallbackArg = cb;

	UxSaveCtx = UxAddUserDialogContext;
	UxAddUserDialogContext = UxContext =
			(_UxCaddUserDialog *) UxGetContext( UxWidget );
	{	
		UxPopdownInterface(UxContext->UxaddUserDialog);
	}
	UxAddUserDialogContext = UxSaveCtx;
}

/*******************************************************************************
       The 'build_' function creates all the widgets
       using the resource values specified in the Property Editor.
*******************************************************************************/

static Widget	_Uxbuild_addUserDialog(Widget UxParent)
{
	Widget		_UxParent;
	Widget		menu1_p2_shell;
	Widget		menu3_p2_shell;
	Widget		menu4_p2_shell;
	Widget		menu5_p4_shell;
	Widget		menu5_p3_shell;
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

	addUserDialog = XtVaCreatePopupShell( "addUserDialog",
			xmDialogShellWidgetClass,
			_UxParent,
			XmNminHeight, 553,
			XmNminWidth, 412,
			XmNtitle, catgets(_catd, 8, 102, "Admintool: Add User"),
			NULL );
	UxPutContext( addUserDialog, (char *) UxAddUserDialogContext );

	form3 = XtVaCreateWidget( "form3",
			xmFormWidgetClass,
			addUserDialog,
			XmNresizePolicy, XmRESIZE_ANY,
			XmNmarginWidth, 15,
			XmNmarginHeight, 15,
			XmNrubberPositioning, TRUE,
			XmNautoUnmanage, FALSE,
			NULL );
	UxPutContext( form3, (char *) UxAddUserDialogContext );

	label45 = XtVaCreateManagedWidget( "label45",
			xmLabelWidgetClass,
			form3,
			XmNalignment, XmALIGNMENT_BEGINNING,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 103, "USER IDENTITY") ),
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_FORM,
			XmNleftOffset, 12,
			XmNtopOffset, 15,
			NULL );
	UxPutContext( label45, (char *) UxAddUserDialogContext );

	label46 = XtVaCreateManagedWidget( "label46",
			xmLabelWidgetClass,
			form3,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 104, "User Name:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, label45,
			XmNtopOffset, 8,
			NULL );
	UxPutContext( label46, (char *) UxAddUserDialogContext );

	textField22 = XtVaCreateManagedWidget( "textField22",
			xmTextFieldWidgetClass,
			form3,
			XmNvalue, "",
			XmNmarginHeight, 1,
			XmNcolumns, 8,
			XmNmaxLength, 8,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, label46,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, label46,
			NULL );
	UxPutContext( textField22, (char *) UxAddUserDialogContext );

	label47 = XtVaCreateManagedWidget( "label47",
			xmLabelWidgetClass,
			form3,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 105, "User ID:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, label46,
			XmNtopOffset, 8,
			NULL );
	UxPutContext( label47, (char *) UxAddUserDialogContext );

	textField21 = XtVaCreateManagedWidget( "textField21",
			xmTextFieldWidgetClass,
			form3,
			XmNvalue, "",
			XmNmarginHeight, 1,
			XmNcolumns, 10,
			XmNmaxLength, 10,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, label47,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, label47,
			NULL );
	UxPutContext( textField21, (char *) UxAddUserDialogContext );

	label48 = XtVaCreateManagedWidget( "label48",
			xmLabelWidgetClass,
			form3,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 106, "Primary Group:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, label47,
			XmNtopOffset, 8,
			NULL );
	UxPutContext( label48, (char *) UxAddUserDialogContext );

	textField20 = XtVaCreateManagedWidget( "textField20",
			xmTextFieldWidgetClass,
			form3,
			XmNvalue, "other",
			XmNmarginHeight, 1,
			XmNmaxLength, 80,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, label48,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, label48,
			NULL );
	UxPutContext( textField20, (char *) UxAddUserDialogContext );

	label49 = XtVaCreateManagedWidget( "label49",
			xmLabelWidgetClass,
			form3,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 108, "Secondary Groups:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, label48,
			XmNtopOffset, 8,
			NULL );
	UxPutContext( label49, (char *) UxAddUserDialogContext );

	textField19 = XtVaCreateManagedWidget( "textField19",
			xmTextFieldWidgetClass,
			form3,
			XmNvalue, "",
			XmNmarginHeight, 1,
			XmNmaxLength, 150,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, label49,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, label49,
			XmNrightAttachment, XmATTACH_POSITION,
			XmNrightPosition, 80,
			NULL );
	UxPutContext( textField19, (char *) UxAddUserDialogContext );

	label50 = XtVaCreateManagedWidget( "label50",
			xmLabelWidgetClass,
			form3,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 109, "Comment:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopOffset, 8,
			XmNtopWidget, label49,
			NULL );
	UxPutContext( label50, (char *) UxAddUserDialogContext );

	textField18 = XtVaCreateManagedWidget( "textField18",
			xmTextFieldWidgetClass,
			form3,
			XmNvalue, "",
			XmNmarginHeight, 1,
			XmNmaxLength, 80,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, label50,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, label50,
			XmNrightAttachment, XmATTACH_POSITION,
			XmNrightPosition, 80,
			NULL );
	UxPutContext( textField18, (char *) UxAddUserDialogContext );

	label51 = XtVaCreateManagedWidget( "label51",
			xmLabelWidgetClass,
			form3,
			XmNalignment, XmALIGNMENT_END,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 110, "Login Shell:") ),
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopOffset, 8,
			XmNtopWidget, label50,
			NULL );
	UxPutContext( label51, (char *) UxAddUserDialogContext );

	menu1_p2_shell = XtVaCreatePopupShell ("menu1_p2_shell",
			xmMenuShellWidgetClass, form3,
			XmNwidth, 1,
			XmNheight, 1,
			XmNallowShellResize, TRUE,
			XmNoverrideRedirect, TRUE,
			NULL );

	menu1_p2 = XtVaCreateWidget( "menu1_p2",
			xmRowColumnWidgetClass,
			menu1_p2_shell,
			XmNrowColumnType, XmMENU_PULLDOWN,
			NULL );
	UxPutContext( menu1_p2, (char *) UxAddUserDialogContext );

	menu1_p1_b5 = XtVaCreateManagedWidget( "menu1_p1_b5",
			xmPushButtonGadgetClass,
			menu1_p2,
			RES_CONVERT( XmNlabelString, "Bourne" ),
			XmNshowAsDefault, 0,
			NULL );
	UxPutContext( menu1_p1_b5, (char *) UxAddUserDialogContext );

	menu1_p1_b6 = XtVaCreateManagedWidget( "menu1_p1_b6",
			xmPushButtonGadgetClass,
			menu1_p2,
			RES_CONVERT( XmNlabelString, "C" ),
			XmNshowAsDefault, 0,
			NULL );
	UxPutContext( menu1_p1_b6, (char *) UxAddUserDialogContext );

	menu1_p1_b7 = XtVaCreateManagedWidget( "menu1_p1_b7",
			xmPushButtonGadgetClass,
			menu1_p2,
			RES_CONVERT( XmNlabelString, "Korn" ),
			XmNshowAsDefault, 0,
			NULL );
	UxPutContext( menu1_p1_b7, (char *) UxAddUserDialogContext );

	menu1_p1_b8 = XtVaCreateManagedWidget( "menu1_p1_b8",
			xmPushButtonGadgetClass,
			menu1_p2,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 114, "Other") ),
			XmNshowAsDefault, 0,
			NULL );
	UxPutContext( menu1_p1_b8, (char *) UxAddUserDialogContext );

	menu8 = XtVaCreateManagedWidget( "menu8",
			xmRowColumnWidgetClass,
			form3,
			XmNrowColumnType, XmMENU_OPTION,
			XmNsubMenuId, menu1_p2,
			XmNnumColumns, 1,
			XmNpacking, XmPACK_TIGHT,
			XmNspacing, 3,
			XmNmarginWidth, 0,
			XmNnavigationType, XmTAB_GROUP,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, textField18,
			XmNtopOffset, 5,
			XmNmarginHeight, 0,
			RES_CONVERT( XmNlabelString, "" ),
			XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNleftWidget, textField18,
			XmNleftOffset, -6,
			NULL );
	UxPutContext( menu8, (char *) UxAddUserDialogContext );

	textField35 = XtVaCreateManagedWidget( "textField35",
			xmTextFieldWidgetClass,
			form3,
			XmNvalue, "",
			XmNmarginHeight, 1,
			XmNmaxLength, 80,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, menu8,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, label51,
			XmNbottomOffset, -4,
			XmNleftOffset, 2,
			NULL );
	UxPutContext( textField35, (char *) UxAddUserDialogContext );

	label53 = XtVaCreateManagedWidget( "label53",
			xmLabelWidgetClass,
			form3,
			RES_CONVERT( XmNlabelString, "/bin/sh" ),
			XmNalignment, XmALIGNMENT_BEGINNING,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, menu8,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, label51,
			XmNleftOffset,5,
			NULL );
	UxPutContext( label53, (char *) UxAddUserDialogContext );

	label52 = XtVaCreateManagedWidget( "label52",
			xmLabelWidgetClass,
			form3,
			XmNalignment, XmALIGNMENT_BEGINNING,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 115, "ACCOUNT SECURITY") ),
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopOffset, 11,
			XmNtopWidget, menu8,
			XmNrightAttachment, XmATTACH_NONE,
			XmNleftAttachment, XmATTACH_FORM,
			XmNleftOffset, 12,
			NULL );
	UxPutContext( label52, (char *) UxAddUserDialogContext );

	label54 = XtVaCreateManagedWidget( "label54",
			xmLabelWidgetClass,
			form3,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 116, "Password:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, label52,
			XmNtopOffset, 8,
			NULL );
	UxPutContext( label54, (char *) UxAddUserDialogContext );

	menu3_p2_shell = XtVaCreatePopupShell ("menu3_p2_shell",
			xmMenuShellWidgetClass, form3,
			XmNwidth, 1,
			XmNheight, 1,
			XmNallowShellResize, TRUE,
			XmNoverrideRedirect, TRUE,
			NULL );

	menu3_p2 = XtVaCreateWidget( "menu3_p2",
			xmRowColumnWidgetClass,
			menu3_p2_shell,
			XmNrowColumnType, XmMENU_PULLDOWN,
			NULL );
	UxPutContext( menu3_p2, (char *) UxAddUserDialogContext );

	menu3_p1_b5 = XtVaCreateManagedWidget( "menu3_p1_b5",
			xmPushButtonGadgetClass,
			menu3_p2,
			RES_CONVERT( XmNlabelString,
			catgets(_catd, 8, 117, "Cleared until first login") ),
			NULL );
	UxPutContext( menu3_p1_b5, (char *) UxAddUserDialogContext );

	menu3_p1_b6 = XtVaCreateManagedWidget( "menu3_p1_b6",
			xmPushButtonGadgetClass,
			menu3_p2,
			RES_CONVERT( XmNlabelString,
			catgets(_catd, 8, 118, "Account is locked") ),
			NULL );
	UxPutContext( menu3_p1_b6, (char *) UxAddUserDialogContext );

	menu3_p1_b7 = XtVaCreateManagedWidget( "menu3_p1_b7",
			xmPushButtonGadgetClass,
			menu3_p2,
			RES_CONVERT( XmNlabelString,
			catgets(_catd, 8, 119, "No password -- setuid only") ),
			NULL );
	UxPutContext( menu3_p1_b7, (char *) UxAddUserDialogContext );

	menu3_p1_b8 = XtVaCreateManagedWidget( "menu3_p1_b8",
			xmPushButtonGadgetClass,
			menu3_p2,
			RES_CONVERT( XmNlabelString,
			catgets(_catd, 8, 120, "Normal Password...") ),
			NULL );
	UxPutContext( menu3_p1_b8, (char *) UxAddUserDialogContext );

	menu9 = XtVaCreateManagedWidget( "menu9",
			xmRowColumnWidgetClass,
			form3,
			XmNrowColumnType, XmMENU_OPTION,
			XmNsubMenuId, menu3_p2,
			XmNnavigationType, XmTAB_GROUP,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopOffset, 0,
			XmNtopWidget, label52,
			RES_CONVERT( XmNlabelString, "" ),
			XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNleftWidget, menu8,
			XmNleftOffset, -4,
			NULL );
	UxPutContext( menu9, (char *) UxAddUserDialogContext );

	label55 = XtVaCreateManagedWidget( "label55",
			xmLabelWidgetClass,
			form3,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 121, "Min Change:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopOffset, 8,
			XmNtopWidget, menu9,
			NULL );
	UxPutContext( label55, (char *) UxAddUserDialogContext );

	textField23 = XtVaCreateManagedWidget( "textField23",
			xmTextFieldWidgetClass,
			form3,
			XmNvalue, "0",
			XmNmarginHeight, 1,
			XmNcolumns, 8,
			XmNmaxLength, 5,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, label55,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, label55,
			NULL );
	UxPutContext( textField23, (char *) UxAddUserDialogContext );

	label61 = XtVaCreateManagedWidget( "label61",
			xmLabelWidgetClass,
			form3,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 123, "days") ),
			XmNalignment, XmALIGNMENT_BEGINNING,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, textField23,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, textField23,
			NULL );
	UxPutContext( label61, (char *) UxAddUserDialogContext );

	label56 = XtVaCreateManagedWidget( "label56",
			xmLabelWidgetClass,
			form3,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 124, "Max Change:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopOffset, 8,
			XmNtopWidget, label55,
			NULL );
	UxPutContext( label56, (char *) UxAddUserDialogContext );

	textField24 = XtVaCreateManagedWidget( "textField24",
			xmTextFieldWidgetClass,
			form3,
			XmNvalue, "",
			XmNmarginHeight, 1,
			XmNcolumns, 8,
			XmNmaxLength, 5,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, label56,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, label56,
			NULL );
	UxPutContext( textField24, (char *) UxAddUserDialogContext );

	label62 = XtVaCreateManagedWidget( "label62",
			xmLabelWidgetClass,
			form3,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 125, "days") ),
			XmNalignment, XmALIGNMENT_BEGINNING,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, textField24,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, textField24,
			NULL );
	UxPutContext( label62, (char *) UxAddUserDialogContext );

	label57 = XtVaCreateManagedWidget( "label57",
			xmLabelWidgetClass,
			form3,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 126, "Max Inactive:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopOffset, 8,
			XmNtopWidget, label56,
			NULL );
	UxPutContext( label57, (char *) UxAddUserDialogContext );

	textField25 = XtVaCreateManagedWidget( "textField25",
			xmTextFieldWidgetClass,
			form3,
			XmNvalue, "",
			XmNmarginHeight, 1,
			XmNcolumns, 8,
			XmNmaxLength, 5,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, label57,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, label57,
			NULL );
	UxPutContext( textField25, (char *) UxAddUserDialogContext );

	label63 = XtVaCreateManagedWidget( "label63",
			xmLabelWidgetClass,
			form3,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 127, "days") ),
			XmNalignment, XmALIGNMENT_BEGINNING,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, textField25,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, textField25,
			NULL );
	UxPutContext( label63, (char *) UxAddUserDialogContext );

	label58 = XtVaCreateManagedWidget( "label58",
			xmLabelWidgetClass,
			form3,
			RES_CONVERT( XmNlabelString,
				catgets(_catd, 8, 128, "Expiration Date:\n(dd/mm/yy)") ),
			XmNalignment, XmALIGNMENT_END,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopOffset, 8,
			XmNtopWidget, label57,
			XmNleftAttachment, XmATTACH_FORM,
			NULL );
	UxPutContext( label58, (char *) UxAddUserDialogContext );

	menu4_p2_shell = XtVaCreatePopupShell ("menu4_p2_shell",
			xmMenuShellWidgetClass, form3,
			XmNwidth, 1,
			XmNheight, 1,
			XmNallowShellResize, TRUE,
			XmNoverrideRedirect, TRUE,
			NULL );

	menu4_p2 = XtVaCreateWidget( "menu4_p2",
			xmRowColumnWidgetClass,
			menu4_p2_shell,
			XmNrowColumnType, XmMENU_PULLDOWN,
			XmNnumColumns, 4,
			XmNpacking, XmPACK_COLUMN,
			XmNorientation, XmHORIZONTAL,
			NULL );
	UxPutContext( menu4_p2, (char *) UxAddUserDialogContext );

	menu4_p1_b33 = XtVaCreateManagedWidget( "menu4_p1_b33",
			xmPushButtonGadgetClass,
			menu4_p2,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 129, "None") ),
			NULL );
	UxPutContext( menu4_p1_b33, (char *) UxAddUserDialogContext );

	menu4_p1_b34 = XtVaCreateManagedWidget( "menu4_p1_b34",
			xmPushButtonGadgetClass,
			menu4_p2,
			RES_CONVERT( XmNlabelString, "1" ),
			NULL );
	UxPutContext( menu4_p1_b34, (char *) UxAddUserDialogContext );

	menu4_p1_b35 = XtVaCreateManagedWidget( "menu4_p1_b35",
			xmPushButtonGadgetClass,
			menu4_p2,
			RES_CONVERT( XmNlabelString, "2" ),
			NULL );
	UxPutContext( menu4_p1_b35, (char *) UxAddUserDialogContext );

	menu4_p1_b36 = XtVaCreateManagedWidget( "menu4_p1_b36",
			xmPushButtonGadgetClass,
			menu4_p2,
			RES_CONVERT( XmNlabelString, "3" ),
			NULL );
	UxPutContext( menu4_p1_b36, (char *) UxAddUserDialogContext );

	menu4_p1_b37 = XtVaCreateManagedWidget( "menu4_p1_b37",
			xmPushButtonGadgetClass,
			menu4_p2,
			RES_CONVERT( XmNlabelString, "4" ),
			NULL );
	UxPutContext( menu4_p1_b37, (char *) UxAddUserDialogContext );

	menu4_p1_b38 = XtVaCreateManagedWidget( "menu4_p1_b38",
			xmPushButtonGadgetClass,
			menu4_p2,
			RES_CONVERT( XmNlabelString, "5" ),
			NULL );
	UxPutContext( menu4_p1_b38, (char *) UxAddUserDialogContext );

	menu4_p1_b39 = XtVaCreateManagedWidget( "menu4_p1_b39",
			xmPushButtonGadgetClass,
			menu4_p2,
			RES_CONVERT( XmNlabelString, "6" ),
			NULL );
	UxPutContext( menu4_p1_b39, (char *) UxAddUserDialogContext );

	menu4_p1_b40 = XtVaCreateManagedWidget( "menu4_p1_b40",
			xmPushButtonGadgetClass,
			menu4_p2,
			RES_CONVERT( XmNlabelString, "7" ),
			NULL );
	UxPutContext( menu4_p1_b40, (char *) UxAddUserDialogContext );

	menu4_p1_b41 = XtVaCreateManagedWidget( "menu4_p1_b41",
			xmPushButtonGadgetClass,
			menu4_p2,
			RES_CONVERT( XmNlabelString, "8" ),
			NULL );
	UxPutContext( menu4_p1_b41, (char *) UxAddUserDialogContext );

	menu4_p1_b42 = XtVaCreateManagedWidget( "menu4_p1_b42",
			xmPushButtonGadgetClass,
			menu4_p2,
			RES_CONVERT( XmNlabelString, "9" ),
			NULL );
	UxPutContext( menu4_p1_b42, (char *) UxAddUserDialogContext );

	menu4_p1_b43 = XtVaCreateManagedWidget( "menu4_p1_b43",
			xmPushButtonGadgetClass,
			menu4_p2,
			RES_CONVERT( XmNlabelString, "10" ),
			NULL );
	UxPutContext( menu4_p1_b43, (char *) UxAddUserDialogContext );

	menu4_p1_b44 = XtVaCreateManagedWidget( "menu4_p1_b44",
			xmPushButtonGadgetClass,
			menu4_p2,
			RES_CONVERT( XmNlabelString, "11" ),
			NULL );
	UxPutContext( menu4_p1_b44, (char *) UxAddUserDialogContext );

	menu4_p1_b45 = XtVaCreateManagedWidget( "menu4_p1_b45",
			xmPushButtonGadgetClass,
			menu4_p2,
			RES_CONVERT( XmNlabelString, "12" ),
			NULL );
	UxPutContext( menu4_p1_b45, (char *) UxAddUserDialogContext );

	menu4_p1_b46 = XtVaCreateManagedWidget( "menu4_p1_b46",
			xmPushButtonGadgetClass,
			menu4_p2,
			RES_CONVERT( XmNlabelString, "13" ),
			NULL );
	UxPutContext( menu4_p1_b46, (char *) UxAddUserDialogContext );

	menu4_p1_b47 = XtVaCreateManagedWidget( "menu4_p1_b47",
			xmPushButtonGadgetClass,
			menu4_p2,
			RES_CONVERT( XmNlabelString, "14" ),
			NULL );
	UxPutContext( menu4_p1_b47, (char *) UxAddUserDialogContext );

	menu4_p1_b48 = XtVaCreateManagedWidget( "menu4_p1_b48",
			xmPushButtonGadgetClass,
			menu4_p2,
			RES_CONVERT( XmNlabelString, "15" ),
			NULL );
	UxPutContext( menu4_p1_b48, (char *) UxAddUserDialogContext );

	menu4_p1_b49 = XtVaCreateManagedWidget( "menu4_p1_b49",
			xmPushButtonGadgetClass,
			menu4_p2,
			RES_CONVERT( XmNlabelString, "16" ),
			NULL );
	UxPutContext( menu4_p1_b49, (char *) UxAddUserDialogContext );

	menu4_p1_b50 = XtVaCreateManagedWidget( "menu4_p1_b50",
			xmPushButtonGadgetClass,
			menu4_p2,
			RES_CONVERT( XmNlabelString, "17" ),
			NULL );
	UxPutContext( menu4_p1_b50, (char *) UxAddUserDialogContext );

	menu4_p1_b51 = XtVaCreateManagedWidget( "menu4_p1_b51",
			xmPushButtonGadgetClass,
			menu4_p2,
			RES_CONVERT( XmNlabelString, "18" ),
			NULL );
	UxPutContext( menu4_p1_b51, (char *) UxAddUserDialogContext );

	menu4_p1_b52 = XtVaCreateManagedWidget( "menu4_p1_b52",
			xmPushButtonGadgetClass,
			menu4_p2,
			RES_CONVERT( XmNlabelString, "19" ),
			NULL );
	UxPutContext( menu4_p1_b52, (char *) UxAddUserDialogContext );

	menu4_p1_b53 = XtVaCreateManagedWidget( "menu4_p1_b53",
			xmPushButtonGadgetClass,
			menu4_p2,
			RES_CONVERT( XmNlabelString, "20" ),
			NULL );
	UxPutContext( menu4_p1_b53, (char *) UxAddUserDialogContext );

	menu4_p1_b54 = XtVaCreateManagedWidget( "menu4_p1_b54",
			xmPushButtonGadgetClass,
			menu4_p2,
			RES_CONVERT( XmNlabelString, "21" ),
			NULL );
	UxPutContext( menu4_p1_b54, (char *) UxAddUserDialogContext );

	menu4_p1_b55 = XtVaCreateManagedWidget( "menu4_p1_b55",
			xmPushButtonGadgetClass,
			menu4_p2,
			RES_CONVERT( XmNlabelString, "22" ),
			NULL );
	UxPutContext( menu4_p1_b55, (char *) UxAddUserDialogContext );

	menu4_p1_b56 = XtVaCreateManagedWidget( "menu4_p1_b56",
			xmPushButtonGadgetClass,
			menu4_p2,
			RES_CONVERT( XmNlabelString, "23" ),
			NULL );
	UxPutContext( menu4_p1_b56, (char *) UxAddUserDialogContext );

	menu4_p1_b57 = XtVaCreateManagedWidget( "menu4_p1_b57",
			xmPushButtonGadgetClass,
			menu4_p2,
			RES_CONVERT( XmNlabelString, "24" ),
			NULL );
	UxPutContext( menu4_p1_b57, (char *) UxAddUserDialogContext );

	menu4_p1_b58 = XtVaCreateManagedWidget( "menu4_p1_b58",
			xmPushButtonGadgetClass,
			menu4_p2,
			RES_CONVERT( XmNlabelString, "25" ),
			NULL );
	UxPutContext( menu4_p1_b58, (char *) UxAddUserDialogContext );

	menu4_p1_b59 = XtVaCreateManagedWidget( "menu4_p1_b59",
			xmPushButtonGadgetClass,
			menu4_p2,
			RES_CONVERT( XmNlabelString, "26" ),
			NULL );
	UxPutContext( menu4_p1_b59, (char *) UxAddUserDialogContext );

	menu4_p1_b60 = XtVaCreateManagedWidget( "menu4_p1_b60",
			xmPushButtonGadgetClass,
			menu4_p2,
			RES_CONVERT( XmNlabelString, "27" ),
			NULL );
	UxPutContext( menu4_p1_b60, (char *) UxAddUserDialogContext );

	menu4_p1_b61 = XtVaCreateManagedWidget( "menu4_p1_b61",
			xmPushButtonGadgetClass,
			menu4_p2,
			RES_CONVERT( XmNlabelString, "28" ),
			NULL );
	UxPutContext( menu4_p1_b61, (char *) UxAddUserDialogContext );

	menu4_p1_b62 = XtVaCreateManagedWidget( "menu4_p1_b62",
			xmPushButtonGadgetClass,
			menu4_p2,
			RES_CONVERT( XmNlabelString, "29" ),
			NULL );
	UxPutContext( menu4_p1_b62, (char *) UxAddUserDialogContext );

	menu4_p1_b63 = XtVaCreateManagedWidget( "menu4_p1_b63",
			xmPushButtonGadgetClass,
			menu4_p2,
			RES_CONVERT( XmNlabelString, "30" ),
			NULL );
	UxPutContext( menu4_p1_b63, (char *) UxAddUserDialogContext );

	menu4_p1_b64 = XtVaCreateManagedWidget( "menu4_p1_b64",
			xmPushButtonGadgetClass,
			menu4_p2,
			RES_CONVERT( XmNlabelString, "31" ),
			NULL );
	UxPutContext( menu4_p1_b64, (char *) UxAddUserDialogContext );

	menu11 = XtVaCreateManagedWidget( "menu11",
			xmRowColumnWidgetClass,
			form3,
			XmNrowColumnType, XmMENU_OPTION,
			XmNsubMenuId, menu4_p2,
			XmNnumColumns, 8,
			XmNpacking, XmPACK_COLUMN,
			XmNorientation, XmHORIZONTAL,
			XmNnavigationType, XmTAB_GROUP,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, textField25,
			XmNrightAttachment, XmATTACH_NONE,
			XmNrightWidget, NULL,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, label58,
			XmNleftOffset, -8,
			RES_CONVERT( XmNlabelString, "" ),
			NULL );
	UxPutContext( menu11, (char *) UxAddUserDialogContext );

	menu5_p4_shell = XtVaCreatePopupShell ("menu5_p4_shell",
			xmMenuShellWidgetClass, form3,
			XmNwidth, 1,
			XmNheight, 1,
			XmNallowShellResize, TRUE,
			XmNoverrideRedirect, TRUE,
			NULL );

	menu5_p4 = XtVaCreateWidget( "menu5_p4",
			xmRowColumnWidgetClass,
			menu5_p4_shell,
			XmNrowColumnType, XmMENU_PULLDOWN,
			NULL );
	UxPutContext( menu5_p4, (char *) UxAddUserDialogContext );

	menu5_p1_b36 = XtVaCreateManagedWidget( "menu5_p1_b36",
			xmPushButtonGadgetClass,
			menu5_p4,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 161, "None") ),
			NULL );
	UxPutContext( menu5_p1_b36, (char *) UxAddUserDialogContext );

	menu5_p1_b37 = XtVaCreateManagedWidget( "menu5_p1_b37",
			xmPushButtonGadgetClass,
			menu5_p4,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 130, "Jan" ) ),
			NULL );
	UxPutContext( menu5_p1_b37, (char *) UxAddUserDialogContext );

	menu5_p1_b38 = XtVaCreateManagedWidget( "menu5_p1_b38",
			xmPushButtonGadgetClass,
			menu5_p4,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 131, "Feb" ) ),
			NULL );
	UxPutContext( menu5_p1_b38, (char *) UxAddUserDialogContext );

	menu5_p1_b39 = XtVaCreateManagedWidget( "menu5_p1_b39",
			xmPushButtonGadgetClass,
			menu5_p4,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 132, "March" ) ),
			NULL );
	UxPutContext( menu5_p1_b39, (char *) UxAddUserDialogContext );

	menu5_p1_b40 = XtVaCreateManagedWidget( "menu5_p1_b40",
			xmPushButtonGadgetClass,
			menu5_p4,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 133, "April" ) ),
			NULL );
	UxPutContext( menu5_p1_b40, (char *) UxAddUserDialogContext );

	menu5_p1_b41 = XtVaCreateManagedWidget( "menu5_p1_b41",
			xmPushButtonGadgetClass,
			menu5_p4,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 134, "May" ) ),
			NULL );
	UxPutContext( menu5_p1_b41, (char *) UxAddUserDialogContext );

	menu5_p1_b42 = XtVaCreateManagedWidget( "menu5_p1_b42",
			xmPushButtonGadgetClass,
			menu5_p4,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 135, "June" ) ),
			NULL );
	UxPutContext( menu5_p1_b42, (char *) UxAddUserDialogContext );

	menu5_p1_b43 = XtVaCreateManagedWidget( "menu5_p1_b43",
			xmPushButtonGadgetClass,
			menu5_p4,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 136, "July" ) ),
			NULL );
	UxPutContext( menu5_p1_b43, (char *) UxAddUserDialogContext );

	menu5_p1_b44 = XtVaCreateManagedWidget( "menu5_p1_b44",
			xmPushButtonGadgetClass,
			menu5_p4,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 137, "Aug" ) ),
			NULL );
	UxPutContext( menu5_p1_b44, (char *) UxAddUserDialogContext );

	menu5_p1_b45 = XtVaCreateManagedWidget( "menu5_p1_b45",
			xmPushButtonGadgetClass,
			menu5_p4,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 138, "Sept" ) ),
			NULL );
	UxPutContext( menu5_p1_b45, (char *) UxAddUserDialogContext );

	menu5_p1_b46 = XtVaCreateManagedWidget( "menu5_p1_b46",
			xmPushButtonGadgetClass,
			menu5_p4,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 139, "Oct" ) ),
			NULL );
	UxPutContext( menu5_p1_b46, (char *) UxAddUserDialogContext );

	menu5_p1_b47 = XtVaCreateManagedWidget( "menu5_p1_b47",
			xmPushButtonGadgetClass,
			menu5_p4,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 140, "Nov" ) ),
			NULL );
	UxPutContext( menu5_p1_b47, (char *) UxAddUserDialogContext );

	menu5_p1_b48 = XtVaCreateManagedWidget( "menu5_p1_b48",
			xmPushButtonGadgetClass,
			menu5_p4,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 141, "Dec" ) ),
			NULL );
	UxPutContext( menu5_p1_b48, (char *) UxAddUserDialogContext );

	menu10 = XtVaCreateManagedWidget( "menu10",
			xmRowColumnWidgetClass,
			form3,
			XmNrowColumnType, XmMENU_OPTION,
			XmNsubMenuId, menu5_p4,
			XmNnavigationType, XmTAB_GROUP,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, textField25,
			XmNrightAttachment, XmATTACH_NONE,
			XmNrightWidget, NULL,
			RES_CONVERT( XmNlabelString, "" ),
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, menu11,
			NULL );
	UxPutContext( menu10, (char *) UxAddUserDialogContext );

	menu5_p3_shell = XtVaCreatePopupShell ("menu5_p3_shell",
			xmMenuShellWidgetClass, form3,
			XmNwidth, 1,
			XmNheight, 1,
			XmNallowShellResize, TRUE,
			XmNoverrideRedirect, TRUE,
			NULL );

	menu5_p3 = XtVaCreateWidget( "menu5_p3",
			xmRowColumnWidgetClass,
			menu5_p3_shell,
			XmNrowColumnType, XmMENU_PULLDOWN,
			NULL );
	UxPutContext( menu5_p3, (char *) UxAddUserDialogContext );

	menu5_p1_b25 = XtVaCreateManagedWidget( "menu5_p1_b25",
			xmPushButtonGadgetClass,
			menu5_p3,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 142, "None") ),
			NULL );
	UxPutContext( menu5_p1_b25, (char *) UxAddUserDialogContext );

	menu5_p1_b26 = XtVaCreateManagedWidget( "menu5_p1_b26",
			xmPushButtonGadgetClass,
			menu5_p3,
			RES_CONVERT( XmNlabelString, "1993" ),
			NULL );
	UxPutContext( menu5_p1_b26, (char *) UxAddUserDialogContext );

	menu5_p1_b27 = XtVaCreateManagedWidget( "menu5_p1_b27",
			xmPushButtonGadgetClass,
			menu5_p3,
			RES_CONVERT( XmNlabelString, "1994" ),
			NULL );
	UxPutContext( menu5_p1_b27, (char *) UxAddUserDialogContext );

	menu5_p1_b28 = XtVaCreateManagedWidget( "menu5_p1_b28",
			xmPushButtonGadgetClass,
			menu5_p3,
			RES_CONVERT( XmNlabelString, "1995" ),
			NULL );
	UxPutContext( menu5_p1_b28, (char *) UxAddUserDialogContext );

	menu5_p1_b29 = XtVaCreateManagedWidget( "menu5_p1_b29",
			xmPushButtonGadgetClass,
			menu5_p3,
			RES_CONVERT( XmNlabelString, "1996" ),
			NULL );
	UxPutContext( menu5_p1_b29, (char *) UxAddUserDialogContext );

	menu5_p1_b30 = XtVaCreateManagedWidget( "menu5_p1_b30",
			xmPushButtonGadgetClass,
			menu5_p3,
			RES_CONVERT( XmNlabelString, "1997" ),
			NULL );
	UxPutContext( menu5_p1_b30, (char *) UxAddUserDialogContext );

	menu5_p1_b31 = XtVaCreateManagedWidget( "menu5_p1_b31",
			xmPushButtonGadgetClass,
			menu5_p3,
			RES_CONVERT( XmNlabelString, "1998" ),
			NULL );
	UxPutContext( menu5_p1_b31, (char *) UxAddUserDialogContext );

	menu5_p1_b32 = XtVaCreateManagedWidget( "menu5_p1_b32",
			xmPushButtonGadgetClass,
			menu5_p3,
			RES_CONVERT( XmNlabelString, "1999" ),
			NULL );
	UxPutContext( menu5_p1_b32, (char *) UxAddUserDialogContext );

	menu5_p1_b33 = XtVaCreateManagedWidget( "menu5_p1_b33",
			xmPushButtonGadgetClass,
			menu5_p3,
			RES_CONVERT( XmNlabelString, "2000" ),
			NULL );
	UxPutContext( menu5_p1_b33, (char *) UxAddUserDialogContext );

	menu5_p1_b34 = XtVaCreateManagedWidget( "menu5_p1_b34",
			xmPushButtonGadgetClass,
			menu5_p3,
			RES_CONVERT( XmNlabelString, "2001" ),
			NULL );
	UxPutContext( menu5_p1_b34, (char *) UxAddUserDialogContext );

	menu5_p1_b35 = XtVaCreateManagedWidget( "menu5_p1_b35",
			xmPushButtonGadgetClass,
			menu5_p3,
			RES_CONVERT( XmNlabelString, "2002" ),
			NULL );
	UxPutContext( menu5_p1_b35, (char *) UxAddUserDialogContext );

	menu7 = XtVaCreateManagedWidget( "menu7",
			xmRowColumnWidgetClass,
			form3,
			XmNrowColumnType, XmMENU_OPTION,
			XmNsubMenuId, menu5_p3,
			XmNnavigationType, XmTAB_GROUP,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, textField25,
			XmNrightAttachment, XmATTACH_NONE,
			XmNrightOffset, 0,
			RES_CONVERT( XmNlabelString, "" ),
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, menu10,
			NULL );
	UxPutContext( menu7, (char *) UxAddUserDialogContext );

	label59 = XtVaCreateManagedWidget( "label59",
			xmLabelWidgetClass,
			form3,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 162, "Warning:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, menu7,
			XmNtopOffset, 12,
			NULL );
	UxPutContext( label59, (char *) UxAddUserDialogContext );

	textField26 = XtVaCreateManagedWidget( "textField26",
			xmTextFieldWidgetClass,
			form3,
			XmNvalue, "",
			XmNmarginHeight, 1,
			XmNcolumns, 8,
			XmNmaxLength, 5,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, label59,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, label59,
			NULL );
	UxPutContext( textField26, (char *) UxAddUserDialogContext );

	label64 = XtVaCreateManagedWidget( "label64",
			xmLabelWidgetClass,
			form3,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 163, "days") ),
			XmNalignment, XmALIGNMENT_BEGINNING,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, textField26,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, label59,
			NULL );
	UxPutContext( label64, (char *) UxAddUserDialogContext );

	label43 = XtVaCreateManagedWidget( "label43",
			xmLabelWidgetClass,
			form3,
			XmNalignment, XmALIGNMENT_BEGINNING,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 164, "HOME DIRECTORY") ),
			XmNleftAttachment, XmATTACH_FORM,
			XmNleftOffset, 12,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopOffset, 8,
			XmNtopWidget, label59,
			NULL );
	UxPutContext( label43, (char *) UxAddUserDialogContext );

	label44 = XtVaCreateManagedWidget( "label44",
			xmLabelWidgetClass,
			form3,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 165, "Create Home Dir:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, label43,
			XmNtopOffset, 8,
			NULL );
	UxPutContext( label44, (char *) UxAddUserDialogContext );

	toggleButton22 = XtVaCreateManagedWidget( "toggleButton22",
			xmToggleButtonWidgetClass,
			form3,
			XmNheight, 18,
			XmNwidth, 18,
			XmNspacing, 0,
			RES_CONVERT( XmNlabelString, "" ),
			XmNstringDirection, XmSTRING_DIRECTION_L_TO_R,
			XmNnavigationType, XmTAB_GROUP,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, label44,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNbottomWidget, label44,
			NULL );
	UxPutContext( toggleButton22, (char *) UxAddUserDialogContext );

	label60 = XtVaCreateManagedWidget( "label60",
			xmLabelWidgetClass,
			form3,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 166, "Path:") ),
			XmNalignment, XmALIGNMENT_END,
			XmNleftAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, label44,
			XmNtopOffset, 10,
			NULL );
	UxPutContext( label60, (char *) UxAddUserDialogContext );

	textField14 = XtVaCreateManagedWidget( "textField14",
			xmTextFieldWidgetClass,
			form3,
			XmNvalue, "",
			XmNmarginHeight, 1,
			XmNmaxLength, 80,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, label60,
			XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNrightAttachment, XmATTACH_POSITION,
			XmNrightPosition, 80,
			XmNbottomWidget, label60,
			NULL );
	UxPutContext( textField14, (char *) UxAddUserDialogContext );

	/* align labels */
	wlist[0] = label46;
	wlist[1] = label47;
	wlist[2] = label48;
	wlist[3] = label49;
	wlist[4] = label50;
	wlist[5] = label51;
	wlist[6] = label54;
	wlist[7] = label55;
	wlist[8] = label56;
	wlist[9] = label57;
	wlist[10] = label58;
	wlist[11] = label59;
	wlist[12] = label44;
	wlist[13] = label60;
	wnum = 14;
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

	create_button_box(form3, label60, UxAddUserDialogContext,
		&pushButton4, &pushButton9, &pushButton12,
		&pushButton5, &pushButton6);

	XtVaSetValues(form3, XmNinitialFocus, textField22, NULL);

	XtAddCallback( menu1_p1_b5, XmNactivateCallback,
		(XtCallbackProc) addUserLoginShellBourneCB,
		(XtPointer) UxAddUserDialogContext );

	XtAddCallback( menu1_p1_b6, XmNactivateCallback,
		(XtCallbackProc) addUserLoginShellCCB,
		(XtPointer) UxAddUserDialogContext );

	XtAddCallback( menu1_p1_b7, XmNactivateCallback,
		(XtCallbackProc) addUserLoginShellKornCB,
		(XtPointer) UxAddUserDialogContext );

	XtAddCallback( menu1_p1_b8, XmNactivateCallback,
		(XtCallbackProc) addUserLoginShellOtherCB,
		(XtPointer) UxAddUserDialogContext );

	XtAddCallback( menu3_p1_b5, XmNactivateCallback,
		(XtCallbackProc) passwdTypeCB,
		(XtPointer) UxAddUserDialogContext );
	XtAddCallback( menu3_p1_b6, XmNactivateCallback,
		(XtCallbackProc) passwdTypeCB,
		(XtPointer) UxAddUserDialogContext );
	XtAddCallback( menu3_p1_b7, XmNactivateCallback,
		(XtCallbackProc) passwdTypeCB,
		(XtPointer) UxAddUserDialogContext );
	XtAddCallback( menu3_p1_b8, XmNactivateCallback,
		(XtCallbackProc) passwdCB,
		(XtPointer) UxAddUserDialogContext );

	XtAddCallback( pushButton4, XmNactivateCallback,
		(XtCallbackProc) addCB,
		(XtPointer) UxAddUserDialogContext );

	XtAddCallback( pushButton9, XmNactivateCallback,
		(XtCallbackProc) addCB,
		(XtPointer) UxAddUserDialogContext );

	XtAddCallback( pushButton12, XmNactivateCallback,
		(XtCallbackProc) resetCB,
		(XtPointer) UxAddUserDialogContext );

	XtAddCallback( pushButton5, XmNactivateCallback,
		(XtCallbackProc) cancelCB,
		(XtPointer) UxAddUserDialogContext );

	XtAddCallback( pushButton6, XmNactivateCallback,
		(XtCallbackProc) helpCB,
		"user_window.r.hlp" );

	XtAddCallback( menu4_p1_b33, XmNactivateCallback,
		(XtCallbackProc) addUserSetDateNoneCB,
		(XtPointer) UxAddUserDialogContext );

	XtAddCallback( menu5_p1_b36, XmNactivateCallback,
		(XtCallbackProc) addUserSetDateNoneCB,
		(XtPointer) UxAddUserDialogContext );

	XtAddCallback( menu5_p1_b25, XmNactivateCallback,
		(XtCallbackProc) addUserSetDateNoneCB,
		(XtPointer) UxAddUserDialogContext );


	XtAddCallback( addUserDialog, XmNdestroyCallback,
		(XtCallbackProc) UxDestroyContextCB,
		(XtPointer) UxAddUserDialogContext);


	return ( addUserDialog );
}

/*******************************************************************************
       The following is the 'Interface function' which is the
       external entry point for creating this interface.
       This function should be called from your application or from
       a callback function.
*******************************************************************************/

Widget	create_addUserDialog(Widget parent)
{
	Widget                  rtrn;
	_UxCaddUserDialog       *UxContext;
	static int		_Uxinit = 0;

	UxAddUserDialogContext = UxContext =
		(_UxCaddUserDialog *) UxNewContext( sizeof(_UxCaddUserDialog), False );


	if ( ! _Uxinit )
	{
		UxLoadResources( "addUserDialog.rf" );
		_Uxinit = 1;
	}

	rtrn = _Uxbuild_addUserDialog(parent);

	return(rtrn);
}

void
show_adduserdialog(Widget parent, sysMgrMainCtxt * ctxt)
{
	_UxCaddUserDialog       *UxContext;


	if (adduserdialog && XtIsManaged(adduserdialog)) {
		XtPopup(XtParent(adduserdialog), XtGrabNone);
		return;
	}

	SetBusyPointer(True);

	if (adduserdialog == NULL)
		adduserdialog = create_addUserDialog(parent);

	addUserInit(adduserdialog);
	
	ctxt->currDialog = adduserdialog;
	UxPopupInterface(adduserdialog, no_grab);
	SetBusyPointer(False);
}

/*******************************************************************************
       END OF FILE
*******************************************************************************/


