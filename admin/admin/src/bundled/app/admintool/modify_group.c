/* Copyright (c) 1993 Sun Microsystems, Inc. */

#pragma ident "@(#)modify_group.c	1.19 96/08/30 Sun Microsystems"


/*******************************************************************************
	modify_group.c

*******************************************************************************/

#include <stdio.h>
#include <nl_types.h>
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

#include "sysman_iface.h"
#include "util.h"

extern nl_catd	_catd;	/* for catgets(), defined in main.c */


/*******************************************************************************
       Includes, Defines, and Global variables from the Declarations Editor:
*******************************************************************************/

Widget 	modifygroupdialog = NULL;


/*******************************************************************************
       The following header file defines the context structure.
*******************************************************************************/

#define CONTEXT_MACRO_ACCESS 1
#include "modify_group.h"
#undef CONTEXT_MACRO_ACCESS


/*******************************************************************************       The following are Auxiliary functions.
*******************************************************************************/
int
modGroupInit(
	Widget  wgt,
	SysmanGroupArg*	group
)
{
        _UxCmodGroupDialog      *UxContext;
	XmString		xstr;
	int			sts;

        UxContext = (_UxCmodGroupDialog *) UxGetContext( wgt );

	sts = sysman_get_group(group, errbuf, ERRBUF_SIZE);

	if (sts != 0) {
		display_error(modifygroupdialog, errbuf);
		return 0;
	}

        XmTextSetString(UxContext->UxtextField22, (char*)group->groupname);

	xstr = XmStringCreateLocalized(group->gid ? (char*)group->gid : "");
	XtVaSetValues(UxContext->UxgidLabel,
		XmNlabelString, xstr,
		NULL);
	XmStringFree(xstr);

        XmTextSetString(UxContext->UxtextField16,
		group->members ? (char*)group->members : "");

	return 1;
}


/*******************************************************************************
       The following are callback functions.
*******************************************************************************/

static  void    modifyCB(
                        Widget wgt,
                        XtPointer cd,
                        XtPointer cb)
{
        _UxCmodGroupDialog      *UxContext;
        Widget                  UxWidget = wgt;
        XtPointer               UxClientData = cd;
        XtPointer               UxCallbackArg = cb;
 
        UxContext = (_UxCmodGroupDialog *) UxGetContext( UxWidget );
        {
        char                            *name;
        char                            *ID;
        char                            *members;
	SysmanGroupArg			group;
	int				sts=0;
	XmString			xstr;
	extern void update_entry(void*);
        
	SetBusyPointer(True);
 
	memset(&group, 0, sizeof(SysmanGroupArg));

        /* Get the field values. */
        name = XmTextGetString(UxContext->UxtextField22);

	XtVaGetValues(UxContext->UxgidLabel,
		XmNlabelString, &xstr,
		NULL);
	XmStringGetLtoR(xstr, XmSTRING_DEFAULT_CHARSET, &ID);

        members = XmTextGetString(UxContext->UxtextField16);

#define INVALID_GROUPNAME catgets(_catd, 8, 496, "Invalid Group Name specified.")
#define INVALID_DELIMETER catgets(_catd, 8, 498, "Invalid Member List delimiter specified.\nUse a comma to separate member names.")
#define INVALID_MEMBERS catgets(_catd, 8, 499, "Invalid member found in Members List.")

	/* validate the group name */
	if (*name == '\0' || !valid_gname(name)) {
		strncpy(errbuf, INVALID_GROUPNAME, ERRBUF_SIZE);
		sts = -1;
	} else

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
		display_error(modifygroupdialog, errbuf);
        	XtFree(name);
        	XtFree(ID);
        	XtFree(members);
		SetBusyPointer(False);
		return;
	}

	memset((void *)&group, 0, sizeof (group));
	group.groupname = name;
	group.gid = ID;
	group.members = members;
	group.groupname_key = UxContext->group.groupname_key;
	group.gid_key = UxContext->group.gid_key;
	group.passwd = NULL;


	sts = sysman_modify_group(&group, errbuf, ERRBUF_SIZE);

	if (sts == 0) {
		update_entry(&group);
		free_group(&UxContext->group);
		copy_group(&UxContext->group, &group);
		if (wgt == UxContext->UxpushButton4) {
			/* Dismiss the window - OK was selected. */
			UxPopdownInterface(UxContext->UxmodGroupDialog);
		}
	}
	else {
		display_error(modifygroupdialog, errbuf);
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
        _UxCmodGroupDialog      *UxSaveCtx, *UxContext;
        Widget                  UxWidget = wgt;
        XtPointer               UxClientData = cd;
        XtPointer               UxCallbackArg = cb;
 
        UxSaveCtx = UxModGroupDialogContext;
        UxModGroupDialogContext = UxContext =
                        (_UxCmodGroupDialog *) UxGetContext( UxWidget );
        {
		SetBusyPointer(True);
		modGroupInit(wgt, &UxContext->group);
		SetBusyPointer(False);
        
        }
        UxModGroupDialogContext = UxSaveCtx;
}
 
static  void    cancelCB(
                        Widget wgt,
                        XtPointer cd,
                        XtPointer cb)
{
        _UxCmodGroupDialog      *UxSaveCtx, *UxContext;
        Widget                  UxWidget = wgt;
        XtPointer               UxClientData = cd;
        XtPointer               UxCallbackArg = cb;
 
        UxSaveCtx = UxModGroupDialogContext;
        UxModGroupDialogContext = UxContext =
                        (_UxCmodGroupDialog *) UxGetContext( UxWidget );
        {
                UxPopdownInterface(UxContext->UxmodGroupDialog);
        }
        UxModGroupDialogContext = UxSaveCtx;
}
 

/*******************************************************************************
       The 'build_' function creates all the widgets
       using the resource values specified in the Property Editor.
*******************************************************************************/

static Widget	_Uxbuild_modGroupDialog(Widget UxParent)
{
	Widget		_UxParent;

	_UxParent = UxParent;
	if ( _UxParent == NULL )
	{
		_UxParent = GtopLevel;
	}

	modGroupDialog = XtVaCreatePopupShell( "modGroupDialog",
			xmDialogShellWidgetClass,
			_UxParent,
                        XmNallowShellResize,TRUE,
			XmNinitialState, NormalState,
                        XmNminHeight, 158,
                        XmNminWidth, 376,
			XmNtitle, catgets(_catd, 8, 192, "Admintool: Modify Group"),
			NULL );
	UxPutContext( modGroupDialog, (char *) UxModGroupDialogContext );

	form3 = XtVaCreateWidget( "form3",
			xmFormWidgetClass,
			modGroupDialog,
                        XmNmarginHeight,5,
                        XmNresizePolicy, XmRESIZE_ANY,
			XmNautoUnmanage, FALSE,
			XmNrubberPositioning, TRUE,
			XmNallowOverlap, FALSE,
			NULL );
	UxPutContext( form3, (char *) UxModGroupDialogContext );

	label46 = XtVaCreateManagedWidget( "label46",
			xmLabelWidgetClass,
			form3,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 193, "Group Name:") ),
                        XmNalignment, XmALIGNMENT_END,
                        XmNleftAttachment, XmATTACH_FORM,
                        XmNleftOffset, 30,
                        XmNtopAttachment, XmATTACH_FORM,
                        XmNtopOffset,15,
			NULL );
	UxPutContext( label46, (char *) UxModGroupDialogContext );

	textField22 = XtVaCreateManagedWidget( "textField22",
			xmTextFieldWidgetClass,
			form3,
			XmNvalue, "",
			XmNmarginHeight, 1,
			XmNcolumns, 8,
			XmNmaxLength, 8,
                        XmNeditable, TRUE,
                        XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
                        XmNbottomWidget, label46,
                        XmNleftAttachment, XmATTACH_WIDGET,
                        XmNleftWidget, label46,
			NULL );
	UxPutContext( textField22, (char *) UxModGroupDialogContext );

	label47 = XtVaCreateManagedWidget( "label47",
			xmLabelWidgetClass,
			form3,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 194, "Group ID:") ),
			XmNalignment, XmALIGNMENT_END,
                        XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, label46,
                        XmNtopOffset,8,
                        XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
                        XmNrightWidget, label46,
			NULL );
	UxPutContext( label47, (char *) UxModGroupDialogContext );

	gidLabel = XtVaCreateManagedWidget( "gidLabel",
			xmLabelWidgetClass,
			form3,
			RES_CONVERT( XmNlabelString, "" ),
                        XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
                        XmNbottomWidget, label47,
                        XmNleftAttachment, XmATTACH_WIDGET,
                        XmNleftWidget, label47,
			NULL );
	UxPutContext( gidLabel, (char *) UxModGroupDialogContext );

	label34 = XtVaCreateManagedWidget( "label34",
			xmLabelWidgetClass,
			form3,
			RES_CONVERT( XmNlabelString, catgets(_catd, 8, 195, "Members List:") ),
			XmNalignment, XmALIGNMENT_END,
                        XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, label47,
                        XmNtopOffset,8,
                        XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
                        XmNrightWidget, label47,
			NULL );
	UxPutContext( label34, (char *) UxModGroupDialogContext );

	textField16 = XtVaCreateManagedWidget( "textField16",
			xmTextFieldWidgetClass,
			form3,
			XmNvalue, "",
			XmNmarginHeight, 1,
			XmNcolumns, 30,
			XmNmaxLength, 256,
                        XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
                        XmNbottomWidget, label34,
                        XmNleftAttachment, XmATTACH_WIDGET,
                        XmNleftWidget, label34,
                        XmNrightAttachment, XmATTACH_FORM,
                        XmNrightOffset, 15,
			NULL );
	UxPutContext( textField16, (char *) UxModGroupDialogContext );

	create_button_box(form3, textField16, UxModGroupDialogContext,
		&pushButton4, &pushButton9, &pushButton12,
		&pushButton5, &pushButton6);

	XtVaSetValues(form3, XmNinitialFocus, textField22, NULL);
 
        XtAddCallback( pushButton4, XmNactivateCallback,
                (XtCallbackProc) modifyCB,
                (XtPointer) UxModGroupDialogContext );
 
        XtAddCallback( pushButton9, XmNactivateCallback,
                (XtCallbackProc) modifyCB,
                (XtPointer) UxModGroupDialogContext );
 
        XtAddCallback( pushButton12, XmNactivateCallback,
                (XtCallbackProc) resetCB,
                (XtPointer) UxModGroupDialogContext );
 
        XtAddCallback( pushButton5, XmNactivateCallback,
                (XtCallbackProc) cancelCB,
                (XtPointer) UxModGroupDialogContext );
 
        XtAddCallback( pushButton6, XmNactivateCallback,
                (XtCallbackProc) helpCB,
		"group_window.r.hlp" );
 
 
        XtAddCallback( modGroupDialog, XmNdestroyCallback,
                (XtCallbackProc) UxDestroyContextCB,
                (XtPointer) UxModGroupDialogContext);
 


	return ( modGroupDialog );
}

/*******************************************************************************
       The following is the 'Interface function' which is the
       external entry point for creating this interface.
       This function should be called from your application or from
       a callback function.
*******************************************************************************/

Widget	create_modGroupDialog(Widget parent)
{
	Widget                  rtrn;
	_UxCmodGroupDialog      *UxContext;
	static int		_Uxinit = 0;

	UxModGroupDialogContext = UxContext =
		(_UxCmodGroupDialog *) UxNewContext( sizeof(_UxCmodGroupDialog), False );

	/* null structure */
	memset(&UxContext->group, 0, sizeof(SysmanGroupArg));

	if ( ! _Uxinit )
	{
		UxLoadResources( "modGroupDialog.rf" );
		_Uxinit = 1;
	}

	rtrn = _Uxbuild_modGroupDialog(parent);

	return(rtrn);
}

void
show_modifygroupdialog(
	Widget	parent,
	SysmanGroupArg*	group,
	sysMgrMainCtxt * ctxt
)
{
	_UxCmodGroupDialog       *UxContext;


	SetBusyPointer(True);

	if (modifygroupdialog == NULL)
		modifygroupdialog = create_modGroupDialog(parent);
	
        UxContext = (_UxCmodGroupDialog *) UxGetContext( modifygroupdialog );

	ctxt->currDialog = modifygroupdialog;
	free_group(&UxContext->group);
	copy_group(&UxContext->group, group);

	if (modGroupInit(modifygroupdialog, &UxContext->group)) {
		UxPopupInterface(modifygroupdialog, no_grab);
	}

	SetBusyPointer(False);
}

/*******************************************************************************
       END OF FILE
*******************************************************************************/

