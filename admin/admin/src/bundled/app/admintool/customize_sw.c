
/* Copyright 1993 Sun Microsystems, Inc. */

#pragma ident "@(#)customize_sw.c	1.36 96/06/11 Sun Microsystems"

/*	customize_sw.c 	*/

#include <stdlib.h>
#include <stdio.h>
#include <nl_types.h>
#include <Xm/Xm.h>
#include <Xm/CascadeB.h>
#include <Xm/CascadeBG.h>
#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/LabelG.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/MenuShell.h>
#include <Xm/MainW.h>
#include <Xm/MessageB.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrolledW.h>
#include <Xm/SelectioB.h>
#include <Xm/SeparatoG.h>
#include <Xm/Text.h>
#include <Xm/ToggleBG.h>
#include <X11/Shell.h>
#include <X11/IntrinsicP.h>
#include <libintl.h>
#include "spmisoft_api.h"
#include "media.h"
#include "software.h"
#include "util.h"


extern nl_catd	_catd;	/* for catgets(), defined in main.c */

extern swFormUserData * FocusData;
extern void fud_selection_status(swFormUserData *, caddr_t);

extern void update_product_sizes(Widget);

extern ViewData * create_software_view(
	Widget, 
	Module *, 
	char *,
	char *, 
	int,
	Boolean,
	Boolean,
	Boolean);

extern void update_size_label(Widget, char *, int);

ViewData	* customView = NULL;
Widget customDialog = NULL;
Widget customDialogForm = NULL;
Widget customDialogButtonBox = NULL;
Widget customDialogOKbutton = NULL;

/* internal functions */
ViewData *	customize_sw_dialog(Widget parent, addCtxt *, char *,
			Module *, char *, Boolean);

/*******************************************************************************
       The following are callback functions.
*******************************************************************************/

#define unresolved_str	catgets(_catd, 8, 181, "Unresolved dependencies remain. Do you want\nto keep your current customized selections?")
/*
 * OK
 */
static	void	
okCB(
	Widget w, 
	XtPointer cd, 
	XtPointer cbs)
{
	char 	* s = NULL;
	WidgetList 	kids;
	ViewData * v;
	addCtxt * c = (addCtxt *)cd;
	int psz = 0;
  	int on = 0;
	ModStatus ms;

	v = c->view;

	extract_install_directory(v->v_basedir_value);
	XtVaGetValues(v->v_dependencyText, XmNvalue, &s, NULL);
	if (strcmp(s, "")) {
	    if (!AskUser(customDialog, unresolved_str, 
				YES_STRING,
				NO_STRING,
				NO))
		return;	
	}
        
	/* free duplicate subtree */
	destroyModule(FocusData->f_copyOfModule);
	FocusData->f_copyOfModule = NULL;

	XtPopdown(customDialog);

	/* 
         * Determine selection status of sub-modules within custom 
         * dialog and propogate that  to add dialog.
         */
	ms = getModuleStatus(FocusData->f_module, FocusData->f_locale);
	XtVaSetValues(FocusData->f_toggle, XmNset, 
		(ms == SELECTED ||
			ms == PARTIALLY_SELECTED) ? True : False, NULL); 

	/*
         * Likewise, use aggregate status of constituent pkgs to
         * determine selection status of fud.
         */
	if (ms == UNSELECTED)
		reset_fud(FocusData);

	update_product_sizes(FocusData->f_toggle);
	traverse_fud_list(selected_fud_size, (caddr_t)&psz);
	update_size_label(c->totalLabel, catgets(_catd, 8, 184, "Total (MB): "), psz);
	traverse_fud_list(fud_selection_status, (caddr_t)&on);
	XtSetSensitive(XmMessageBoxGetChild(c->button_box, 
				XmDIALOG_OK_BUTTON), on ? True : False);
}


/*
 * Cancel
 */
static	void	
cancelCB(
	Widget w, 
	XtPointer cd, 
	XtPointer cbs)
{
	addCtxt * c = (addCtxt *) cd;
	int psz = 0;
  	int on = 0;
	Module * m = NULL;

	/* reset original values */
	resetModule(FocusData->f_module, FocusData->f_copyOfModule);
	destroyModule(FocusData->f_copyOfModule);
	FocusData->f_copyOfModule = NULL;

	/* grab focus module from add window */	
	m = FocusData->f_module;	

	XtPopdown(customDialog);

/*
	update_product_sizes(FocusData->f_toggle);
	traverse_fud_list(selected_fud_size, (caddr_t)&psz);
	update_size_label(c->totalLabel, catgets(_catd, 8, 185, "Total (MB): "), psz);
	traverse_fud_list(fud_selection_status, (caddr_t)&on);
	XtSetSensitive(XmMessageBoxGetChild(c->button_box, 
				XmDIALOG_OK_BUTTON), on ? True : False);
*/
}

ViewData *
customize_sw_dialog(Widget w, 
			addCtxt * ctxt,
			char	* title,
			Module * module,
			char * locale,
			Boolean single_level)
{
	Widget		butt;
	Position	x, y;
	Widget 		tmpw; 
	XmString	sel;
	Widget		parent = w;
	char*		win_title;


	/* If the media was changed then the add dialog will need to 
	 * be recreated to reflect a new software tree.  
	 */

	while (!XmIsDialogShell(parent)) 
		parent = XtParent(parent);

	/* Create the Add Software dialog. */
	if (!customDialog) {
#ifdef SW_INSTALLER
		win_title = catgets(_catd, 8, 516, "Customize Installation");
#else
		win_title = catgets(_catd, 8, 186,
				"Admintool: Customize Installation");
#endif
		customDialog = XtVaCreatePopupShell(
			"customize_software_shell",
			xmDialogShellWidgetClass, parent,
			XmNshellUnitType, XmPIXELS,
			XmNallowShellResize, False,
			XmNminWidth, 500,
			XmNminHeight, 300, 
			XmNtitle, win_title,
			NULL);

		customDialogForm = XtVaCreateWidget( "add_dialog",
	    		xmFormWidgetClass, 
			customDialog,
			XmNdialogStyle,	XmDIALOG_FULL_APPLICATION_MODAL,
			XmNunitType, XmPIXELS,
			XmNfractionBase, 100,
			XmNwidth, 600,
			XmNheight, 380, 
			NULL );
	}	
	ctxt->view = create_software_view(customDialogForm, module, locale,
			title, MODE_INSTALL, TRUE, single_level, TRUE);

	if (!customDialogButtonBox) {
		customDialogButtonBox = 
			XmCreateMessageBox(customDialogForm,
			"ButtonBox", NULL, 0);

		XtUnmanageChild(XmMessageBoxGetChild(
			customDialogButtonBox, XmDIALOG_MESSAGE_LABEL));

		XtVaSetValues(customDialogButtonBox,
    			XmNbottomAttachment, XmATTACH_FORM,
			XmNbottomOffset, 1,
    			XmNleftAttachment, XmATTACH_FORM,
			XmNleftOffset, 1,
    			XmNrightAttachment, XmATTACH_FORM,
			XmNrightOffset, 1,
    			RSC_CVT( XmNokLabelString, catgets(_catd, 8, 187, "OK") ),
    			RSC_CVT( XmNcancelLabelString, catgets(_catd, 8, 188, "Cancel")),
    			RSC_CVT( XmNhelpLabelString, catgets(_catd, 8, 189, "Help") ),
    			NULL);
	
		tmpw = customDialogOKbutton = 
			XmMessageBoxGetChild(customDialogButtonBox,
			XmDIALOG_OK_BUTTON);
		XtVaSetValues(tmpw, XmNtraversalOn, False, NULL);

		tmpw = XmMessageBoxGetChild(customDialogButtonBox,
			XmDIALOG_CANCEL_BUTTON);
		XtVaSetValues(tmpw, XmNtraversalOn, False, NULL);

		tmpw = XmMessageBoxGetChild(customDialogButtonBox,
			XmDIALOG_HELP_BUTTON);
		XtVaSetValues(tmpw, XmNtraversalOn, False, NULL);

		XtAddCallback(customDialogButtonBox, 
			XmNcancelCallback,
			(XtCallbackProc) cancelCB,
			(XtPointer) ctxt );
		XtAddCallback(customDialogButtonBox, 
			XmNhelpCallback,
			(XtCallbackProc) helpCB,
			"software_custom_window.r.hlp" );
	}
	else 
        	XtRemoveAllCallbacks(customDialogButtonBox,XmNokCallback);

	XtAddCallback(customDialogButtonBox, XmNokCallback,
		(XtCallbackProc) okCB,
                (XtPointer) ctxt );

	XtVaSetValues(ctxt->view->v_detailPane,
	    	XmNbottomAttachment, XmATTACH_WIDGET,
	    	XmNbottomWidget, customDialogButtonBox,
	    	NULL);
	XtVaSetValues(ctxt->view->v_modulesForm,
	    	XmNbottomAttachment, XmATTACH_WIDGET,
	    	XmNbottomWidget, customDialogButtonBox,
	    	NULL);

	XtManageChild(ctxt->view->v_modulesForm);

	XtManageChild(customDialogButtonBox);
	return (ctxt->view);
}

