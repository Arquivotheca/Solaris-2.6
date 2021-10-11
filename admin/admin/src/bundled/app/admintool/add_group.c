/* Copyright (c) 1993 Sun Microsystems, Inc. */

#pragma ident "@(#)add_group.c	1.16 96/08/30 Sun Microsystems"


/*******************************************************************************
	add_group.c
*******************************************************************************/

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
#include <Xm/MessageB.h>
#include <Xm/Text.h>
#include <nl_types.h>

#include "sysman_iface.h"
#include "util.h"
#include "valid.h"

extern nl_catd	_catd;	/* for catgets(), defined in main.c */

/*******************************************************************************
       Includes, Defines, and Global variables from the Declarations Editor:
*******************************************************************************/

Widget 	addgroupdialog = NULL;


/*******************************************************************************
       The following header file defines the context structure.
*******************************************************************************/

#define CONTEXT_MACRO_ACCESS 1
#include "add_group.h"
#undef CONTEXT_MACRO_ACCESS


/*******************************************************************************       The following are Auxiliary functions.
*******************************************************************************/
void
addGroupInit(
                Widget  wgt)
{
	gid_t			next_avail_gid;
	char			gid_str[32];
        _UxCaddGroupDialog      *UxContext;

        UxContext = (_UxCaddGroupDialog *) UxGetContext( wgt );

        XmTextSetString(UxContext->UxtextField22, "");

	next_avail_gid = sysman_get_next_avail_gid();
	sprintf(gid_str, "%d", next_avail_gid);
        XmTextSetString(UxContext->UxtextField21, gid_str);

        XmTextSetString(UxContext->UxtextField16, "");

}


/*******************************************************************************
       The following are callback functions.
*******************************************************************************/

static void
addCB(
	Widget wgt,
	XtPointer cd,
	XtPointer cb
)
{
        volatile _UxCaddGroupDialog      *UxContext;
        Widget                  UxWidget = wgt;
        XtPointer               UxClientData = cd;
        XtPointer               UxCallbackArg = cb;
 
        UxContext = (volatile _UxCaddGroupDialog *) UxGetContext( UxWidget );
        {
        char                            *name;
        char                            *ID;
        char                            *ID_normalized;
        char                            *members;
        char                            str[50];
        XmString                        text;
        static int                      ans;
        Widget                          errorDialog;
        extern Widget                   create_errorDialog(Widget parent);
	extern Widget 			scrolledList1;
	extern void			add_group_to_list(SysmanGroupArg* group);
	SysmanGroupArg			group;
	int				sts=0;
	int				conflict_status;
        char                            msgbuf[200];
        
	SetBusyPointer(True);
 
	memset(&group, 0, sizeof(SysmanGroupArg));

        /* Get the field values. */
        name = XmTextGetString(UxContext->UxtextField22);
        ID = XmTextGetString(UxContext->UxtextField21);
        members = XmTextGetString(UxContext->UxtextField16);

#define INVALID_GROUPNAME catgets(_catd, 8, 496, "Invalid Group Name specified.")
#define INVALID_GID     catgets(_catd, 8, 497, "Invalid Group ID specified.")
#define INVALID_DELIMETER catgets(_catd, 8, 498, "Invalid Member List delimiter specified.\nUse a comma to separate member names.")
#define INVALID_MEMBERS catgets(_catd, 8, 499, "Invalid member found in Members List.")

	/* validate the group name */
	if (*name == '\0' || !valid_gname(name)) {
		strncpy(errbuf, INVALID_GROUPNAME, ERRBUF_SIZE);
		sts = -1;
	} else

	/* Do not allow negative ID value. */
	if (strchr(ID, '-') != NULL) {
		strncpy(errbuf, INVALID_GID, ERRBUF_SIZE);
		sts = -1;
	} else

	/* validate group id */
	if (!valid_gid(ID)) {
		strncpy(errbuf, INVALID_GID, ERRBUF_SIZE);
		sts = -1;
	} else

	/* normalize group id */
        if ( (ID_normalized=normalize_gid(ID))) {
		strcpy (ID, ID_normalized);
		free  ( (void *) ID_normalized);
	}

	/* Do not allow blank delimited member list. */
	if (strchr(members, ' ') != NULL) {
		strncpy(errbuf, INVALID_DELIMETER, ERRBUF_SIZE);
		sts = -1;
	} else

	 /* validate member list */
	 if (!valid_group_members(members)) {
		strncpy(errbuf, INVALID_MEMBERS, ERRBUF_SIZE);
		sts = -1;
	}

	if (sts != 0) {
		display_error(addgroupdialog, errbuf);
        	XtFree(name);
        	XtFree(ID);
        	XtFree(members);
		SetBusyPointer(False);
		return;
	}


	if ( conflict_status = check_ns_group_conflicts(name, atol(ID) ) ) { 
		if (conflict_status == SYSMAN_CONFLICT_BOTH_USED) {
			sprintf(msgbuf, "%s",  
                		catgets(_catd, 8, 645,
				"the group and id entries are being used in the name service group map"));
			if (!Confirm(addgroupdialog, msgbuf, NULL, catgets(_catd, 8, 641, "OK"))) {
				/* DO NOT perform the ADD. */
        			XtFree(name);
        			XtFree(ID);
        			XtFree(members);
				SetBusyPointer(False);
				return ;
			}
		}
		else if (conflict_status == SYSMAN_CONFLICT_NAME_USED) {
			sprintf(msgbuf, "%s",  
                		catgets(_catd, 8, 646,
				"this group name is already being used in the the name service group map"));
			if (!Confirm(addgroupdialog, msgbuf, NULL, catgets(_catd, 8, 641, "OK"))) {
				/* DO NOT perform the ADD. */
        			XtFree(name);
        			XtFree(ID);
        			XtFree(members);
				SetBusyPointer(False);
				return ;
			}
		}
		else if (conflict_status == SYSMAN_CONFLICT_ID_USED) {
			sprintf(msgbuf, "%s",  
                		catgets(_catd, 8, 647,
				"this group id is already being used in the name service group map"));
			if (!Confirm(addgroupdialog, msgbuf, NULL, catgets(_catd, 8, 641, "OK"))) {
				/* DO NOT perform the ADD. */
        			XtFree(name);
        			XtFree(ID);
        			XtFree(members);
				SetBusyPointer(False);
				return ;
			}
		}
	}

	group.groupname = name;
	group.gid = ID;
	group.members = members;
	group.groupname_key = name;
	group.gid_key = ID;


	sts = sysman_add_group(&group, errbuf, ERRBUF_SIZE);

	if (sts == 0) {
		add_group_to_list(&group);
		if (wgt == UxContext->UxpushButton4) {
			/* Dismiss the window - OK was selected. */
			UxPopdownInterface(UxContext->UxaddGroupDialog);
		}
	}
	else {
		display_error(addgroupdialog, errbuf);
	}

        XtFree(name);
        XtFree(ID);
        XtFree(members);
 
	SetBusyPointer(False);

        }
}
 
static  void    resetCB(
                        Widget wgt,
                        XtPointer cd,
                        XtPointer cb)
{
        _UxCaddGroupDialog      *UxSaveCtx, *UxContext;
        Widget                  UxWidget = wgt;
        XtPointer               UxClientData = cd;
        XtPointer               UxCallbackArg = cb;
 
        UxSaveCtx = UxAddGroupDialogContext;
        UxAddGroupDialogContext = UxContext =
                        (_UxCaddGroupDialog *) UxGetContext( UxWidget );
        {
		SetBusyPointer(True);
                addGroupInit(UxWidget);
		SetBusyPointer(False);
        
        }
        UxAddGroupDialogContext = UxSaveCtx;
}
 
static  void    cancelCB(
                        Widget wgt,
                        XtPointer cd,
                        XtPointer cb)
{
        _UxCaddGroupDialog      *UxSaveCtx, *UxContext;
        Widget                  UxWidget = wgt;
        XtPointer               UxClientData = cd;
        XtPointer               UxCallbackArg = cb;
 
        UxSaveCtx = UxAddGroupDialogContext;
        UxAddGroupDialogContext = UxContext =
                        (_UxCaddGroupDialog *) UxGetContext( UxWidget );
        {
                UxPopdownInterface(UxContext->UxaddGroupDialog);
        }
        UxAddGroupDialogContext = UxSaveCtx;
}
 

/*******************************************************************************
       The 'build_' function creates all the widgets
       using the resource values specified in the Property Editor.
*******************************************************************************/

static Widget	_Uxbuild_addGroupDialog(Widget UxParent)
{
	Widget		_UxParent;

	_UxParent = UxParent;
	if ( _UxParent == NULL )
	{
		_UxParent = GtopLevel;
	}

	addGroupDialog = XtVaCreatePopupShell( "addGroupDialog",
			xmDialogShellWidgetClass,
			_UxParent,
                        XmNallowShellResize,TRUE,
			XmNinitialState, NormalState,
                        XmNminHeight, 158,
                        XmNminWidth, 376,
			XmNtitle, catgets(_catd, 8, 1, "Admintool: Add Group"),
			NULL );
	UxPutContext( addGroupDialog, (char *) UxAddGroupDialogContext );

	form3 = XtVaCreateWidget( "form3",
			xmFormWidgetClass,
			addGroupDialog,
                        XmNmarginHeight,5,
                        XmNresizePolicy, XmRESIZE_ANY,
			XmNautoUnmanage, FALSE,
			XmNrubberPositioning, TRUE,
			XmNallowOverlap, FALSE,
			NULL );
	UxPutContext( form3, (char *) UxAddGroupDialogContext );

	label46 = XtVaCreateManagedWidget( "label46",
			xmLabelWidgetClass,
			form3,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 2, "Group Name:") ),
                        XmNalignment, XmALIGNMENT_END,
                        XmNleftAttachment, XmATTACH_FORM,
                        XmNleftOffset, 30,
                        XmNtopAttachment, XmATTACH_FORM,
                        XmNtopOffset,15,
			NULL );
	UxPutContext( label46, (char *) UxAddGroupDialogContext );

	textField22 = XtVaCreateManagedWidget( "textField22",
			xmTextFieldWidgetClass,
			form3,
			XmNvalue, "",
			XmNcolumns, 8,
			XmNmaxLength, 8,
                        XmNeditable, TRUE,
			XmNmarginHeight, 1,
                        XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
                        XmNbottomWidget, label46,
                        XmNleftAttachment, XmATTACH_WIDGET,
                        XmNleftWidget, label46,
			NULL );
	UxPutContext( textField22, (char *) UxAddGroupDialogContext );

	label47 = XtVaCreateManagedWidget( "label47",
			xmLabelWidgetClass,
			form3,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 3, "Group ID:") ),
			XmNalignment, XmALIGNMENT_END,
                        XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, label46,
                        XmNtopOffset,8,
                        XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
                        XmNrightWidget, label46,
			NULL );
	UxPutContext( label47, (char *) UxAddGroupDialogContext );

	textField21 = XtVaCreateManagedWidget( "textField21",
			xmTextFieldWidgetClass,
			form3,
			XmNresizeWidth, FALSE,
			XmNvalue, "",
			XmNeditable, TRUE,
			XmNcolumns, 10,
			XmNmaxLength, 10,
			XmNmarginHeight, 1,
                        XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
                        XmNbottomWidget, label47,
                        XmNleftAttachment, XmATTACH_WIDGET,
                        XmNleftWidget, label47,
			NULL );
	UxPutContext( textField21, (char *) UxAddGroupDialogContext );

	label34 = XtVaCreateManagedWidget( "label34",
			xmLabelWidgetClass,
			form3,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 4, "Members List:") ),
			XmNalignment, XmALIGNMENT_END,
                        XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, label47,
                        XmNtopOffset,8,
                        XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
                        XmNrightWidget, label47,
			NULL );
	UxPutContext( label34, (char *) UxAddGroupDialogContext );

	textField16 = XtVaCreateManagedWidget( "textField16",
			xmTextFieldWidgetClass,
			form3,
			XmNvalue, "",
			XmNcolumns, 30,
			XmNmaxLength, 256,
			XmNmarginHeight, 1,
                        XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
                        XmNbottomWidget, label34,
                        XmNleftAttachment, XmATTACH_WIDGET,
                        XmNleftWidget, label34,
                        XmNrightAttachment, XmATTACH_FORM,
                        XmNrightOffset, 15,
			NULL );
	UxPutContext( textField16, (char *) UxAddGroupDialogContext );

	create_button_box(form3, textField16, UxAddGroupDialogContext,
		&pushButton4, &pushButton9, &pushButton12,
		&pushButton5, &pushButton6);

	XtVaSetValues(form3, XmNinitialFocus, textField22, NULL);
 
        XtAddCallback( pushButton4, XmNactivateCallback,
                (XtCallbackProc) addCB,
                (XtPointer) UxAddGroupDialogContext );
 
        XtAddCallback( pushButton9, XmNactivateCallback,
                (XtCallbackProc) addCB,
                (XtPointer) UxAddGroupDialogContext );
 
        XtAddCallback( pushButton12, XmNactivateCallback,
                (XtCallbackProc) resetCB,
                (XtPointer) UxAddGroupDialogContext );
 
        XtAddCallback( pushButton5, XmNactivateCallback,
                (XtCallbackProc) cancelCB,
                (XtPointer) UxAddGroupDialogContext );
 
        XtAddCallback( pushButton6, XmNactivateCallback,
                (XtCallbackProc) helpCB,
		"group_window.r.hlp" );
 
 
        XtAddCallback( addGroupDialog, XmNdestroyCallback,
                (XtCallbackProc) UxDestroyContextCB,
                (XtPointer) UxAddGroupDialogContext);
 


	return ( addGroupDialog );
}

/*******************************************************************************
       The following is the 'Interface function' which is the
       external entry point for creating this interface.
       This function should be called from your application or from
       a callback function.
*******************************************************************************/

Widget	create_addGroupDialog(Widget parent)
{
	Widget                  rtrn;
	_UxCaddGroupDialog      *UxContext;
	static int		_Uxinit = 0;

	UxAddGroupDialogContext = UxContext =
		(_UxCaddGroupDialog *) UxNewContext( sizeof(_UxCaddGroupDialog), False );


	if ( ! _Uxinit )
	{
		UxLoadResources( "addGroupDialog.rf" );
		_Uxinit = 1;
	}

	rtrn = _Uxbuild_addGroupDialog(parent);

	return(rtrn);
}

void
show_addgroupdialog(Widget parent, sysMgrMainCtxt * ctxt)
{
	_UxCaddGroupDialog       *UxContext;
	XmString		xstr;


	if (addgroupdialog && XtIsManaged(addgroupdialog)) {
		XtPopup(XtParent(addgroupdialog), XtGrabNone);
		return;
	}

	SetBusyPointer(True);

	if (addgroupdialog == NULL)
		addgroupdialog = create_addGroupDialog(parent);
	
        addGroupInit(addgroupdialog);

	ctxt->currDialog = addgroupdialog;
	UxPopupInterface(addgroupdialog, no_grab);
	SetBusyPointer(False);
}

/*******************************************************************************
       END OF FILE
*******************************************************************************/

