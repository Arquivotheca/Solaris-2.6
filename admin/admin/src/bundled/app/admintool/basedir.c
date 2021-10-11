

/* Copyright 1993 Sun Microsystems, Inc. */

#pragma ident "@(#)source_basedir_dialog.c	1.7 95/02/07 Sun Microsystems"

/*	basedir.c		*/

#include <nl_types.h>
#include <Xm/Xm.h>
#include <Xm/Form.h>
#include <Xm/PushB.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/SelectioB.h>
#include <Xm/TextF.h>
#include "util.h"
#include "software.h"

extern nl_catd	_catd;	/* for catgets(), defined in main.c */

extern Widget 		GtopLevel;
extern XtAppContext	GappContext;
extern Display		*Gdisplay;

extern Widget get_shell_ancestor(Widget);

static Widget		basedirDialog = NULL;

static void TextFocusCB(Widget, XtPointer, XtPointer);

/*******************************************************************************
       The following are callback functions.
*******************************************************************************/
#define basedir_confirm		catgets(_catd, 8, 177, "Changing the installation directory from the default has serious\nimplications and may prevent software from functioning correctly.\n Do you want to really want to change this value?")

/*
 * Arg is text field Widget that is used for input of
 * Installation base directory.
 *
 * If input base dir is different than current base dir,
 * ask for user confirmation before updating. If updated,
 * return 1, else return 0;
 *
 * If change has not been confirmed, reset text field to
 * display current base dir.
 */
int
extract_install_directory(Widget w)
{
	EntryData * ed;
	Module * m;
	PkgAdminProps p;
	char * new_base, * old_base, *old_instdir, *base;

	XtVaGetValues(w, XmNuserData, &ed, NULL);

	m = ed->e_m;

	if (m == NULL) return (1);

	new_base = XmTextFieldGetString(w);

	get_admin_file_values(&p);
        if (strcmp(new_base, p.basedir) == 0)
	    return(0);

	old_base = ed->e_mi->m_basedir;
	old_instdir = ed->e_mi->m_instdir;
	if (old_instdir) base = old_instdir;
	else base = old_base;
	
	/* Two cases signal a change to basedir text field:
         * 1) If base is non-NULL and differs from new_base
         * OR
         * 2) base is NULL (ie. current module is a CLUSTER) and
         * new_base is not a zero length string (ie user has typed
         * something interesting).
         *
         * In either case, warn user of danger in changing 
         * base dir.
         */
	if ((base && (strcmp(new_base, base) != 0)) ||
	    ((base == NULL) && (new_base[0] != '\0'))) {
		/* if new base dir entered */
		ed->e_mi->m_instdir = strdup(new_base);
		return(1);
	}
	return(0);
}

/*
 * Callback for <CR> in base dir text field. If base is
 * changed per return of extract_install_directory,
 * then move focus back whence it came, ie. the CURRENT
 * field in the list of Modules.
 */


void
set_basedirCB(
	Widget w, 
	XtPointer cd, 
	XtPointer cbs)
{
	ViewData * v = (ViewData *) cd;
	int rc = 0;
	extern Widget InfoForm;

	rc = extract_install_directory(w);

	if (rc && InfoForm)
		XmProcessTraversal(InfoForm, XmTRAVERSE_CURRENT);
}

/*
 * Action associated with tab action in base dir text field.
 * If base dir is changed per return of extract_install_directory,
 * tab to module name of base dir just changed, else tab to
 * to next field.
 */

static void
tf_tab(Widget w, XEvent * e, String * p, Cardinal * np)
{
	if (extract_install_directory(w))
		XmProcessTraversal(w, XmTRAVERSE_PREV_TAB_GROUP);
	else
		XmProcessTraversal(w, XmTRAVERSE_NEXT_TAB_GROUP);
}

void
build_basedir_field(Widget parent, ViewData * v)
{
	Arg	args[16];
	int	ac = 0;
	XtActionsRec act;
	EntryData * ed;

        act.string = "tf_tab";
	act.proc   = tf_tab;
	XtAppAddActions(GappContext, &act, 1);

	
	/* 
 	 * Create an EntryData struct to put on userData of
         * text field widget. 
         */
	ed = (EntryData *) malloc(sizeof(EntryData));
	if (ed == NULL)
	    fatal(CANT_ALLOC_MSG);
	memset(ed, 0, sizeof(EntryData));

	v->v_basedir_form = XtVaCreateManagedWidget("basedir_form",
    		xmFormWidgetClass, 
		parent,
    		XmNbottomAttachment, XmATTACH_FORM,
    		XmNleftAttachment, XmATTACH_FORM,
 		        XmNrightAttachment, XmATTACH_FORM,
    		NULL);

	v->v_basedir_label = XtVaCreateManagedWidget("basedir_label",
    		xmLabelGadgetClass, v->v_basedir_form,
    		RSC_CVT( XmNlabelString, catgets(_catd, 8, 180, "Installation Directory:") ),
    		XmNtopAttachment, XmATTACH_FORM,
    		XmNleftAttachment, XmATTACH_FORM,
    		NULL);

	v->v_basedir_value = XtVaCreateManagedWidget("textField",
		xmTextFieldWidgetClass, 
		v->v_basedir_form,
		XmNvalue, "",
		XmNmarginHeight, 1,
		XmNmaxLength, 256,
		XmNautoShowCursorPosition, True,
                XmNcursorPositionVisible, True,
		XmNeditable, True,
                XmNnavigationType, XmTAB_GROUP,
                XmNtraversalOn, True,
		XmNtopAttachment, XmATTACH_WIDGET,
	 	XmNtopWidget, v->v_basedir_label,
	 	XmNleftAttachment, XmATTACH_FORM,
	 	XmNbottomAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNuserData, ed,
		NULL);

	XtOverrideTranslations(v->v_basedir_value,
		XtParseTranslationTable("<Key>Tab: tf_tab()"));

	XtAddCallback(v->v_basedir_value, 
			XmNfocusCallback, TextFocusCB,
			NULL);
	XtAddCallback(v->v_basedir_value, 
			XmNlosingFocusCallback, TextFocusCB,
			NULL);
	XtAddCallback(v->v_basedir_value, 
			XmNactivateCallback, 
			set_basedirCB,
			v);
	XtAddCallback(v->v_basedir_value, 
			XmNactivateCallback, 
			XmProcessTraversal,
			XmTRAVERSE_NEXT_TAB_GROUP);
}

void
update_basedir_field(Widget tw, Module * m, Modinfo * mi)
{
	EntryData * ed;
	PkgAdminProps p;

        get_admin_file_values(&p);
	/* If basedir has been changed, ie m_instdir != NULL, display it.
         * If admin file contains a basedir value, display it.
         * Else, display m_basedir.
         */
	XmTextFieldSetString(tw, mi->m_instdir ? mi->m_instdir :
			         strcmp(p.basedir, "default") ? p.basedir :
				 mi->m_basedir ? mi->m_basedir : "");

	/* 
         * Update the current values on the test widget 
         */
	XtVaGetValues(tw, XmNuserData, &ed, NULL);
	ed->e_m = m;
	ed->e_mi = mi;
	XtVaSetValues(tw, XmNuserData, ed, NULL);
}
static void
TextFocusCB(
	Widget wgt, 
	XtPointer cd, 
	XtPointer cb)
{
	XmAnyCallbackStruct*    cbs = (XmAnyCallbackStruct*)cb;
	extern Widget customDialogForm;
	extern Widget customDialogOKbutton;

	/* When the text field gets focus, disable the default
	 * pushbutton for the dialog.  This allows the text field's
	 * activate callback to work without dismissing the dialog.
	 * Re-enable default pushbutton when losing focus.
	 */

	if (cbs->reason == XmCR_FOCUS) {
		XtVaSetValues(customDialogForm,
			XmNdefaultButton, NULL,
			NULL);
	}
	else if (cbs->reason == XmCR_LOSING_FOCUS) {
		XtVaSetValues(customDialogForm,
			XmNdefaultButton, customDialogOKbutton,
			NULL);
	}
}
