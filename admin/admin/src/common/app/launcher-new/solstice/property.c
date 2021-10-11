/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma	ident	"@(#)property.c	1.19	96/08/02 SMI"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Xm/PushB.h>
#include <Xm/TextF.h>
#include <Xm/Label.h>
#include <Xm/Text.h>
#include <Xm/Form.h>
#include <Xm/PushBG.h>
#include <Xm/DialogS.h>

#include "action.h"
#include "util.h"
#include "launcher.h"

#define FLD_LEN_VIS  20
#define FLD_LEN_MAX 256

extern XtAppContext	GappContext;

propertyContext_t * propertyContext = NULL;
Widget propertyDialog;

Widget label_field(Widget, Widget, FieldItem *);
Widget build_property_dialog(Widget);
void populate_propertyDialog(propertyContext_t *, AppInfo *);
void clear_propertyDialog(propertyContext_t *);

extern AppInfo * itemToAppInfo(XmString);
extern XmString get_appList_item(Widget, int);
extern Widget	which_list(int **, int *);

extern void helpCB();
extern int update_appTable_entry(const char *, const char *, const char *, 
				const char *, const char *, registry_loc_t);

static void
enter_fieldCB(Widget w, XtPointer cd, XtPointer cbs)
{
	FieldItem * fi = (FieldItem *)cd;

	XtVaSetValues(fi->f_fieldWidget, XmNcursorPositionVisible, True, NULL);
	if (fi->f_ellipsisWidget)
		XtVaSetValues(fi->f_ellipsisWidget, XmNsensitive, True, NULL);
}
	
static void
next_fieldCB(Widget w, XtPointer cd, XtPointer cbs)
{
	FieldItem * fi = (FieldItem *)cd;

	if (fi && fi->f_ellipsisWidget)
		XtVaSetValues(fi->f_ellipsisWidget, XmNsensitive, False, NULL);

	XmProcessTraversal(w, XmTRAVERSE_NEXT_TAB_GROUP);
}

static void
next_fieldAction(Widget w, XEvent * e, String * p, Cardinal * np)
{
	FieldItem * fi;
	
	sscanf(p[0], "%d", &fi);
	next_fieldCB(w, (XtPointer)fi, NULL);
}

static void
ellipsisCB(Widget w, XtPointer cd, XtPointer cbs)
{
	FieldItem * fi = (FieldItem *) cd;

	show_fileSelectionDialog(fi);
}

static void
cancelCB(Widget w, XtPointer cd, XtPointer cbs)
{
	propertyContext_t * p_data = (propertyContext_t *) cd;

	clear_propertyDialog(p_data);
	if (fileSelectionDialog && XtIsRealized(fileSelectionDialog))
		XtUnmanageChild(fileSelectionDialog);
	if (propertyDialog)
		XtPopdown(propertyDialog);
}

static void
registerCB(Widget w, XtPointer cd, XtPointer cbs)
{
	propertyContext_t * p_data = (propertyContext_t *)cd;
	int n;
	SolsticeApp sapp;
	char * aname, * apath, * args, * ipath;
	int rc;

	aname = XmTextFieldGetString(p_data->p_appNameField);
	if (!aname || (aname[0] == '\0')) {
		display_error(w, ADD_NONAME_MSG);
		return;
	}
	apath = XmTextFieldGetString(p_data->p_appPathField);
	if (!apath || (apath[0] == '\0')) {
		display_error(w, ADD_NOPATH_MSG);
		return;
	}
	ipath = XmTextFieldGetString(p_data->p_iconPathField);
	args =  XmTextFieldGetString(p_data->p_appArgsField);
	sapp.name = aname;
	sapp.app_args = (args == NULL || args[0] == '\0') ? NULL : args;
	sapp.app_path = apath;
	sapp.icon_path = ipath;
	if ((rc = solstice_add_app(&sapp, localRegistry)) == LAUNCH_OK) {
		n = update_appTable_entry((const char *)aname, 
			   (const char *)apath, 
			   (const char *)args, 
			   (const char *)ipath, 
			   XmTextFieldGetString(p_data->p_scriptNameField),
			   LOCAL);
		launcherContext->l_appTable[n].a_show = SHOW;
		add_appList_entry(configContext->c_appList, 0, aname);
#ifdef SINGLE_SLAP
		slapIconOnPalette(launcherContext->l_workForm, 
			          &(launcherContext->l_appTable[n]), n);
#else
		display_icons(launcherContext->l_workForm, 
				configContext->c_appList);
#endif
	} else {
		solstice_error(w, rc); 
		return;	
	}
	if (fileSelectionDialog && XtIsRealized(fileSelectionDialog))
		XtUnmanageChild(fileSelectionDialog);

	if (propertyDialog)
		XtPopdown(propertyDialog);
}


ActionItem prop_action_items[] = {	
		{ "Add",  registerCB, NULL, NULL},
		{ "Cancel",  cancelCB, NULL, NULL},
		{ "Help",  helpCB, "addapp.h.hlp", NULL},
		NULL
};


static void
activate_text_field(Widget field, Boolean flag)
{
	int len = 0;
	char * s;

	s = XmTextFieldGetString(field);
	if (s && (*s != '\0')) {
		len = strlen(s);
		XmTextFieldSetHighlight(field, 0, len, 
			flag ? XmHIGHLIGHT_NORMAL : XmHIGHLIGHT_SELECTED);
	}
	XmTextFieldSetEditable(field, flag);
	XtVaSetValues(field, XmNtraversalOn, flag, NULL);
}

void
show_propertyDialog(Widget w, Boolean flag)
{
	int cnt;
	int * index_list;
	Widget list_w;
	XmString xms;
	AppInfo * ai;

	if (propertyDialog == NULL)
		propertyDialog = build_property_dialog(get_shell_ancestor(w));

	XtVaSetValues(propertyContext->p_form,
		XmNinitialFocus, propertyContext->p_appNameField,
		NULL);

	if (configContext && flag) {
		if ((list_w = which_list(&index_list, &cnt)) == NULL)
			return;
		XtRemoveAllCallbacks(prop_action_items[2].buttonWidget, 
				XmNactivateCallback);
		XtAddCallback(prop_action_items[2].buttonWidget, 
				XmNactivateCallback,
				helpCB, (XtPointer) "modprop.h.hlp");
                XtVaSetValues(propertyDialog, XmNtitle, 
				catgets(catd, 1, 65, "Launcher: Application Properties"),
				NULL);
		xms = get_appList_item(list_w, index_list[0]);
		ai = itemToAppInfo(xms);
		populate_propertyDialog(propertyContext, ai);
	}
	else {
		XtRemoveAllCallbacks(prop_action_items[2].buttonWidget, 
				XmNactivateCallback);
		XtAddCallback(prop_action_items[2].buttonWidget, 
				XmNactivateCallback,
				helpCB, (XtPointer) "addapp.h.hlp");
                XtVaSetValues(propertyDialog, XmNtitle, 
				catgets(catd, 1, 66, "Launcher: Add Application"),
				NULL);
		clear_propertyDialog(propertyContext);
	}

	XtPopup(propertyDialog, XtGrabNone);
}



FieldItem prop_fields[] = {
/*        n_cols       max       ellipsis? label                    */ 
/* f_label field is a hodler. wrapped string is init'd at run time  */
	{ FLD_LEN_VIS, FLD_LEN_MAX, False, "Name:",  NULL, NULL},
	{ FLD_LEN_VIS, FLD_LEN_MAX, True, "Application Path:",  NULL, NULL},
	{ FLD_LEN_VIS, FLD_LEN_MAX, False, "Arguments:",  NULL, NULL},
	{ FLD_LEN_VIS, FLD_LEN_MAX, True, "Icon Path:", NULL, NULL},
	{ FLD_LEN_VIS, FLD_LEN_MAX, False, "Script:",  NULL, NULL},
};

Widget
build_property_dialog(Widget parent)
{
	Widget area, dialog, label, form;
	propertyContext_t * p_data;
	Widget ellipsis_button;

	p_data = (propertyContext_t *) malloc(sizeof(propertyContext_t));
	if (p_data == NULL)
		fatal(catgets(catd, 1, 72, "FATAL: build_property_dialog can't malloc"));

	dialog = XtVaCreatePopupShell("propertyDialog",
		xmDialogShellWidgetClass,
		parent,
		XmNminHeight, 170,
                XmNminWidth,  590,
		XmNmaxHeight, 180,
                XmNtitle, catgets(catd, 1, 73, "Launcher: Application Properties"),
                XmNinitialState, NormalState,
                NULL);

	p_data->p_form = form = XtVaCreateWidget( "form",
                        xmFormWidgetClass,
                        dialog,
                        XmNresizePolicy, XmRESIZE_ANY,
                        XmNmarginWidth, 15,
                        XmNrubberPositioning, TRUE,
 			XmNallowOverlap, FALSE,
                        XmNautoUnmanage, FALSE,
                        NULL);

	prop_fields[0].f_label = catgets(catd, 1, 67, "Name:");
	p_data->p_appNameField = label_field(form, NULL, &prop_fields[0]);
	XtVaSetValues(prop_fields[0].f_form,
			XmNtopAttachment, XmATTACH_FORM,
			NULL);
	
	prop_fields[1].f_label = catgets(catd, 1, 68, "Application Path:");
	p_data->p_appPathField = label_field(form, prop_fields[0].f_form,
					&prop_fields[1]);

	XtAddCallback(prop_fields[1].f_ellipsisWidget,
		XmNactivateCallback, ellipsisCB,
		&prop_fields[1]);

	prop_fields[2].f_label = catgets(catd, 1, 69, "Arguments:");
	p_data->p_appArgsField = label_field(form, prop_fields[1].f_form,
					&prop_fields[2]);
	
	prop_fields[3].f_label =  catgets(catd, 1, 70, "Icon Path:");
	p_data->p_iconPathField = label_field(form, prop_fields[2].f_form,
					&prop_fields[3]);
	XtAddCallback(prop_fields[3].f_ellipsisWidget,
		XmNactivateCallback, ellipsisCB,
		&prop_fields[3]);

	prop_fields[4].f_label =  catgets(catd, 1, 71, "Script:");
	p_data->p_scriptNameField = label_field(form, prop_fields[3].f_form,
					&prop_fields[4]);
#ifndef SCRIPT_FIELD
	XtUnmanageChild(p_data->p_scriptNameField);
	XtUnmanageChild(prop_fields[4].f_form);
#endif

	prop_action_items[0].label = catgets(catd, 1, 62, "Add");
	prop_action_items[1].label = catgets(catd, 1, 63, "Cancel");
	prop_action_items[2].label = catgets(catd, 1, 64, "Help");

	prop_action_items[0].data = (caddr_t)p_data;
	prop_action_items[1].data = (caddr_t)p_data;

	area = CreateActionArea(form, prop_action_items, 3);
#ifdef SCRIPT_FIELD
	XtVaSetValues(area,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, prop_fields[4].f_form,
		NULL);
#else
	XtVaSetValues(area,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, prop_fields[3].f_form,
		XmNtopOffset, OFFSET,
		NULL);
#endif

	XtManageChild(area);
	XtManageChild(form);
	propertyContext = p_data;
	return(dialog);
}


Widget
label_field(Widget parent, Widget top, FieldItem * fi)
{
	Widget fieldw, labelw, form;
	char trans[128];
	XtActionsRec act;

	act.string = "next_fieldAction";
	act.proc   = next_fieldAction;
	XtAppAddActions(GappContext, &act, 1);

	
#define FRACTION_BASE 100

	fi->f_form = form = XtVaCreateManagedWidget("fieldForm",
		xmFormWidgetClass,
		parent,
		XmNfractionBase, FRACTION_BASE,
		XmNhorizontalSpacing, 10,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, OFFSET,
		XmNbottomOffset, OFFSET,
		NULL);

	if (top)
		XtVaSetValues(form,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, top,
			NULL);

	labelw = XtVaCreateManagedWidget( "fieldLabel",
                        xmLabelWidgetClass,
                        form,
                        XmNalignment, XmALIGNMENT_END,
                        RSC_CVT( XmNlabelString, fi->f_label ),
                        XmNrightAttachment, XmATTACH_POSITION,
			XmNrightPosition, 19,
			XmNtopAttachment, XmATTACH_POSITION,
                	XmNtopPosition, 0,
                	XmNbottomAttachment, XmATTACH_POSITION,
                	XmNbottomPosition, FRACTION_BASE,
                        NULL );

	fi->f_fieldWidget = fieldw =  XtVaCreateManagedWidget( "textField",
                        xmTextFieldWidgetClass,
                        form,
                        XmNvalue, "",
                        XmNmarginHeight, 1,
                        XmNcolumns, fi->f_ncols,
                        XmNmaxLength, fi->f_maxlen,
                        XmNleftAttachment, XmATTACH_WIDGET,
                        XmNleftWidget, labelw,
			XmNtopAttachment, XmATTACH_POSITION,
                	XmNtopPosition, 0,
                	XmNbottomAttachment, XmATTACH_POSITION,
                	XmNbottomPosition, FRACTION_BASE,
			XmNautoShowCursorPosition, True,
			XmNcursorPositionVisible, False,		
			XmNnavigationType, XmTAB_GROUP,
			XmNtraversalOn, True,
                        NULL );

	sprintf(trans, "<Key>Tab: next_fieldAction(%d)", fi);
	XtOverrideTranslations(fieldw, XtParseTranslationTable(trans));

	if (fi->f_fileSelection)
		fi->f_ellipsisWidget =  XtVaCreateManagedWidget("ellipsis",
			xmPushButtonGadgetClass,
			form,
			XmNwidth, 30,
			XmNtopAttachment, XmATTACH_POSITION,
                	XmNtopPosition, 0,
                	XmNbottomAttachment, XmATTACH_POSITION,
                	XmNbottomPosition, FRACTION_BASE,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, fieldw,
			RSC_CVT(XmNlabelString, "..."),
			XmNtraversalOn, False,
			XmNsensitive, False,
			NULL);

	XtAddCallback(fieldw, XmNactivateCallback, 
			next_fieldCB, (XtPointer)fi); 
	XtAddCallback(form, XmNfocusCallback, enter_fieldCB, (XtPointer)fi);

	return(fieldw);
}

void
populate_propertyDialog(propertyContext_t * p_data, AppInfo * ai)
{
	XmTextFieldSetString(p_data->p_appNameField, ai->a_appName);
	if (ai->a_appPath) {
		XmTextFieldSetString(p_data->p_appPathField, ai->a_appPath);
	} else {
		XmTextFieldSetString(p_data->p_appPathField, "");
	}
	if (ai->a_appArgs) {
		XmTextFieldSetString(p_data->p_appArgsField, ai->a_appArgs);
	} else {
		XmTextFieldSetString(p_data->p_appArgsField, "");
	}
	if (ai->a_iconPath) {
		XmTextFieldSetString(p_data->p_iconPathField, ai->a_iconPath);
	} else {
		XmTextFieldSetString(p_data->p_iconPathField, "");
	}
#ifdef SCRIPT_FIELD
	if (ai->a_scriptName) {
		XmTextFieldSetString(p_data->p_scriptNameField, ai->a_scriptName);
	} else {
		XmTextFieldSetString(p_data->p_scriptNameField, "");
	}
#endif
	
}

void
clear_propertyDialog(propertyContext_t * p_data)
{
	XmTextFieldSetString(p_data->p_appNameField, "");
	XmTextFieldSetString(p_data->p_appPathField, "");
	XmTextFieldSetString(p_data->p_appArgsField, "");
	XmTextFieldSetString(p_data->p_iconPathField, "");
#ifdef SCRIPT_FIELD
	XmTextFieldSetString(p_data->p_scriptNameField, "");
#endif
}

