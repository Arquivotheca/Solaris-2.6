/*
 * Copyright (c) 1994 Sun Microsystems, Inc.  All Rights Reserved. Sun
 * considers its source code as an unpublished, proprietary trade secret, and
 * it is available only under strict license provisions.  This copyright
 * notice is placed here only to protect Sun in the event the source is
 * deemed a published work.  Dissassembly, decompilation, or other means of
 * reducing the object code to human readable form is prohibited by the
 * license agreement under which this code is provided to the user or company
 * in possession of this copy.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the Government
 * is subject to restrictions as set forth in subparagraph (c)(1)(ii) of the
 * Rights in Technical Data and Computer Software clause at DFARS 52.227-7013
 * and in similar clauses in the FAR and NASA FAR Supplement.
 */

#pragma ident "@(#)view_sw.c	1.44 95/05/19 Sun Microsystems"

/* view_sw.c */

#include <stdio.h>
#include <nl_types.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xm/Xm.h>
#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/ScrolledW.h>
#include <Xm/RowColumn.h>
#include <Xm/PanedW.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/SashP.h>
#include <Xm/DrawnB.h>
#include <Xm/ArrowB.h>
#include <Xm/ArrowBG.h>
#include <Xm/ToggleB.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/Form.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <libintl.h>
#include <stdlib.h>
#include "spmisoft_api.h"
#include "media.h"
#include "software.h"

#include "util.h"



extern nl_catd	_catd;	/* for catgets(), defined in main.c */


#define BOX_DIMENSION 15
#define ARROW_DIMENSION 16
#define	SIZE_LENGTH 20
#define	DESELECT_ALL_STRING	catgets(_catd, 8, 468, "Deselect All")
#define SELECT_ALL_STRING	catgets(_catd, 8, 469, "Select All")

static Widget createLegend(Widget);
static void initializeLegend(SelectionList *, int);
static void SetInfo(Widget, XtPointer, XtPointer);
static Widget CreateHelpText(Widget, TreeData *);
static void ExpandCluster(Widget, XtPointer, XtPointer);
static void CreateList(Widget, Module *, int, Boolean, Boolean, Boolean);
static void CreateEntry(Widget, int, Module *, Modinfo *, Widget *, Boolean, Boolean);
static void SetSelection(Widget, XtPointer, XtPointer *);
static void CreateSelectPixmap(Widget, Pixmap *, Pixmap *, Pixmap *, Pixmap *);
static void pfgupdate_software(TreeData *, ViewData *);
static void update_selection(Widget, ModStatus, Module *);
static int is_installed(char *);

static int errno;
static SelectionList LegendButtons[4];
static int pfgLowResolution = FALSE;
static SelectPixmaps selectpixmaps;

static void toggleSelectAllCB( Widget w, XtPointer cd, XtPointer cbs);

void display_software(ViewData * v);
void showFilteredDependencies(ViewData * v);
void TurnOffSashTraversal(Widget pane);
void build_list(Widget , ViewData * , Module * , TreeData * , int );
Widget create_detail(Widget , char * , int , char * );
void update_detail_text(Widget, Module *, Modinfo * mi);

extern void customizeCB( Widget, XtPointer, XtPointer);
extern void build_basedir_field(Widget parent, ViewData * v);
extern swFormUserData 		* FocusData;

char * installFileSystems[] = {
	/* If new names are added, make sure and update 
         * LONGESTNAME_FS in software.h
         * If new names are added, make sure to update
	 * N_INSTALL_FS in software.h
	 */

	"/",
	"/usr",
	"/opt",
	"/var",
	"/export",
	"/usr/openwin",
	NULL
};

/********************************************************************/
/********************************************************************/

void fatal(char *);

ViewData *
create_software_view(
	Widget parent,  /* a container FORM for view component widgets */
	Module *module, 
	char * locale,
	char *title,
	int mode, 
	Boolean show_info,
	Boolean single_level,
	Boolean force_on) 
{

	int ac = 0;
	Arg args[16];
	Widget 		bar;
	XmString 	title_string; 
	XmString 	size_string;
	XmString 	total_string;
	TreeData	* sw_tree;
	int		sw_width;
	ViewData	* v;

	if (module == NULL)
		return(NULL);

	v = (ViewData *)malloc(sizeof(ViewData));
	if (v == NULL) 
		fatal(CANT_ALLOC_MSG);

	memset(v, 0, sizeof(ViewData));

	v->v_locale = locale ? strdup(locale) : NULL;
	v->v_modulesForm = XtVaCreateWidget( "swViewForm",
		xmFormWidgetClass,
		parent,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
    		XmNrightAttachment, XmATTACH_POSITION,
    		XmNrightPosition, 49,
		XmNtopOffset, 10,
		XmNleftOffset, 10,
		NULL );

	title_string = XmStringCreateLocalized(title);
	v->v_title =  XtVaCreateManagedWidget("productListScrollWLabel",
    		xmLabelGadgetClass, v->v_modulesForm,
    		XmNtopAttachment, XmATTACH_FORM,
    		XmNleftAttachment, XmATTACH_FORM,
    		XmNlabelString, title_string,
    		NULL);
	XmStringFree(title_string);

	v->v_scrollw = XtVaCreateWidget("productListScrollW",
		xmScrolledWindowWidgetClass,
		v->v_modulesForm,
		XmNscrollingPolicy, XmAUTOMATIC,
   		XmNtopAttachment, XmATTACH_WIDGET,
    		XmNtopWidget, v->v_title,
    		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL );

	v->v_rc = XtVaCreateWidget("list",
    		xmRowColumnWidgetClass, v->v_scrollw,
    		XmNorientation, XmVERTICAL,
    		NULL);

	v->v_selectButton = XtVaCreateWidget(
		"selectToggleButton", 
		xmPushButtonWidgetClass, 
		v->v_modulesForm,
		XmNtraversalOn, False,
		XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftWidget, v->v_scrollw,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, v->v_scrollw,
		XmNtopOffset, 10,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNuserData, v,
		XmNlabelString, XmStringCreateLocalized(SELECT_ALL_STRING), 
		NULL);
	v->v_deselectButton = XtVaCreateWidget(
		"selectToggleButton", 
		xmPushButtonWidgetClass, 
		v->v_modulesForm,
		XmNtraversalOn, False,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, v->v_selectButton,
		XmNleftOffset, 10,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, v->v_scrollw,
		XmNtopOffset, 10,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNuserData, v,
		XmNlabelString, XmStringCreateLocalized(DESELECT_ALL_STRING),
		NULL);

	XtAddCallback(v->v_selectButton, XmNactivateCallback,
			toggleSelectAllCB, 
			    (XtPointer)(module->parent && !single_level ? 
					    module->parent : module));
	XtAddCallback(v->v_deselectButton, XmNactivateCallback,
			toggleSelectAllCB, 
                            (XtPointer)(module->parent && !single_level ?
					    module->parent : module));

	XtVaSetValues(v->v_selectButton, XmNuserData, v, NULL);
	XtVaSetValues(v->v_deselectButton, XmNuserData, v, NULL);

	total_string = XmStringCreateLocalized(catgets(_catd, 8, 471, "Total (MB)"));
	v->v_totalLabel = XtVaCreateManagedWidget("productListTotalLabel",
    		xmLabelGadgetClass, v->v_modulesForm,
    		XmNlabelString, total_string,
		XmNalignment, XmALIGNMENT_END,
    		XmNrightAttachment, XmATTACH_FORM,
 	    	XmNbottomAttachment, XmATTACH_FORM,
		XmNbottomOffset, 20,
    		NULL);
	XmStringFree(total_string);

	XtVaSetValues(v->v_scrollw, 
			XmNbottomAttachment, XmATTACH_WIDGET, 
			XmNbottomWidget,  v->v_totalLabel,
			NULL);

	if (v->v_swTree)
		free(v->v_swTree);

	/* Create software tree structure. */
	sw_tree = (TreeData *)malloc(sizeof(TreeData));
	if (sw_tree == NULL)
	{
		printf(catgets(_catd, 8, 472, "Malloc error. \n"));
		exit(1);
	}
	sw_tree->Count = 0;
	sw_tree->mode = mode;
	sw_tree->total = v->v_totalLabel;
	sw_tree->maxSpace = 0;

	v->v_swTree = sw_tree;

	XtVaSetValues(v->v_rc, XmNuserData, v, NULL);

	CreateList(v->v_rc, module, 0, show_info, single_level, force_on);

	XtSetArg(args[ac], XmNtopAttachment, XmATTACH_FORM); ac++;
	XtSetArg(args[ac], XmNrightAttachment, XmATTACH_FORM); ac++;
	XtSetArg(args[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
	XtSetArg(args[ac], XmNleftPosition, 51); ac++;
	XtSetArg(args[ac], XmNtopOffset, 10); ac++;
	XtSetArg(args[ac], XmNrightOffset, 10); ac++;
	XtSetArg(args[ac], XmNseparatorOn, True); ac++;
	XtSetArg(args[ac], XmNsashHeight, 10); ac++;
	XtSetArg(args[ac], XmNsashWidth, 10); ac++;

	v->v_detailPane = XmCreatePanedWindow(parent, "detailPane", args, ac);

	if (show_info) 
	{
		WidgetList	loc;

		v->v_pkginfoForm = XtVaCreateWidget("detailForm",
	    		xmFormWidgetClass, 
	    		v->v_detailPane, 
/*
			XmNtraversalOn, False, 
*/
	    		NULL);

		v->v_pkginfoText = create_detail(v->v_pkginfoForm, 
						catgets(_catd, 8, 473, "Description"), 5, "package");
		XtManageChild(v->v_pkginfoForm);

		build_basedir_field(v->v_pkginfoForm, v);

		XtVaSetValues(XtParent(v->v_pkginfoText),
			XmNbottomAttachment, XmATTACH_WIDGET,
			XmNbottomWidget, v->v_basedir_label, NULL);

	       /* 
         	* Some initialization work...
         	* Grab the first form from the row-column and use
         	* it as arg to SetInfo. This sets up detail window text
         	* as well as (IMPORTANT) initializing global InfoForm.
         	*/
                XtVaGetValues(v->v_rc, XmNchildren, &loc, NULL);
		SetInfo(loc[0], NULL, NULL);
	}

	v->v_dependencyForm = XtVaCreateWidget("detailForm",
	    	xmFormWidgetClass, 
	    	v->v_detailPane,
	    	XmNtraversalOn, False, 
	    	NULL);

	v->v_dependencyText = create_detail(v->v_dependencyForm, 
				catgets(_catd, 8, 474, "Unresolved Dependencies"), 8, "dependencies");

	XtManageChild(v->v_selectButton);
	XtManageChild(v->v_deselectButton);
	XtManageChild(v->v_dependencyForm);
	XtManageChild(v->v_detailPane);
	TurnOffSashTraversal(v->v_detailPane);

	XtManageChild(v->v_rc);

	XtVaGetValues(v->v_scrollw, XmNverticalScrollBar, &bar, NULL);
	if (bar != NULL)
		XtVaSetValues(bar, XmNtraversalOn, False, NULL);
	XtVaGetValues(v->v_scrollw, XmNhorizontalScrollBar, &bar, NULL);
	if (bar != NULL)
		XtVaSetValues(bar, XmNtraversalOn, False, NULL);

	XtManageChild(v->v_scrollw);
	XtManageChild(v->v_modulesForm);

	CreateSelectPixmap(sw_tree->SelectList[0].select,
		&selectpixmaps.unselect,
		&selectpixmaps.select, &selectpixmaps.partial,
		&selectpixmaps.required);

	display_software(v);
	return (v);
}

TreeData *
reset_software_view(TreeData * sw_tree, Module * module, Modinfo * mi,
	char * locale, char * title, int mode, 
	Boolean show_detail, Boolean single_level, Boolean force_on)
{
	extern ViewData * customView;
	ViewData * v = customView;
	XmString nt = XmStringCreateLocalized(title);
	Widget tmp;	
	WidgetList	loc;

	if (sw_tree == NULL) {
		sw_tree = (TreeData *) malloc (sizeof(TreeData));
		if (sw_tree == NULL) 
			fatal(catgets(_catd, 8, 475, "reset_software_view:can't malloc"));
	}

	sw_tree->mode = mode;
	sw_tree->Count = 0;

	v->v_swTree = sw_tree;

	tmp = v->v_title;

	v->v_title =  XtVaCreateWidget("productListScrollWLabel",
	    	xmLabelGadgetClass, v->v_modulesForm,
	    	XmNtopAttachment, XmATTACH_FORM,
	    	XmNleftAttachment, XmATTACH_FORM,
	    	XmNlabelString, nt,
	    	NULL);
	XtManageChild(v->v_title);
	XtDestroyWidget(tmp);

	XmStringFree(nt);
	
	tmp = v->v_rc;
	v->v_rc = XtVaCreateWidget("list",
	    	xmRowColumnWidgetClass, v->v_scrollw,
	    	XmNorientation, XmVERTICAL,
		NULL);

	XtRemoveAllCallbacks(v->v_selectButton, XmNactivateCallback);
	XtAddCallback(v->v_selectButton, XmNactivateCallback,
			toggleSelectAllCB, 
			    (XtPointer)(module->parent && !single_level ?
					    module->parent : module));

	XtVaSetValues(v->v_selectButton, XmNuserData, v, NULL);

	XtRemoveAllCallbacks(v->v_deselectButton, XmNactivateCallback);
	XtAddCallback(v->v_deselectButton, XmNactivateCallback,
			toggleSelectAllCB, 
			    (XtPointer)(module->parent && !single_level ?
					    module->parent : module));

	XtVaSetValues(v->v_deselectButton, XmNuserData, v, NULL);

	v->v_locale = locale;
	/* 
         * If custom dialog was cancelled, there will be old
         * value in text field. Call below sets things right.
         */
        update_basedir_field(v->v_basedir_value, module, mi);

	XtVaSetValues(v->v_rc, XmNuserData, v, NULL);
	CreateList(v->v_rc, module, 0, show_detail, single_level, force_on);

	/* 
         * Some initialization work...
         * Grab the first form from the row-column and use
         * it as arg to SetInfo. This sets up detail window text
         * as well as (IMPORTANT) initializing global InfoForm.
         */
        XtVaGetValues(v->v_rc, XmNchildren, &loc, NULL);
	SetInfo(loc[0], NULL, NULL);
	XtManageChild(v->v_rc);
	XtDestroyWidget(tmp);
	display_software(v);
	return(sw_tree);
}
	

void
CreateList(Widget parent_rc, Module * module, int level, 
		Boolean show_info, Boolean single_level, Boolean force_on)
{
	Widget child_rc, expand;
	ViewData * v;	
	extern L10N * getL10Ns(Module *);
	extern addCtxt * ctxt;

	XtVaGetValues(parent_rc, XmNuserData, &v, NULL);

	do {
		if (module->type == CLUSTER || module->type == METACLUSTER || module->type == PRODUCT) {
			CreateEntry(parent_rc, level, 
				module, module->info.mod, 
				&expand, show_info, force_on);

			child_rc = XtVaCreateWidget("child_rc",
			        xmRowColumnWidgetClass, parent_rc,
			        XmNnavigationType, XmNONE,
			        XmNmarginWidth, (Dimension) 0,
	    		        XmNuserData, v,
			        NULL);
			XtAddCallback(expand, XmNactivateCallback,
			        ExpandCluster, child_rc);
			/* create child list of cluster */
			CreateList(child_rc, get_sub(module), level + 1, show_info, False, force_on);
		} else if (module->type == PACKAGE) {
		    char * pkg_path;
		    L10N * l = getL10Ns(module);
		    if (ctxt->current_product)
			 pkg_path = ctxt->current_product->info.prod->p_pkgdir;
                    else
			 pkg_path = ctxt->pkg_path ? ctxt->pkg_path :
					ctxt->install_path;
		    if (v->v_locale && l) {
		        while (l) {
			    if ((strcmp(v->v_locale, 
					l->l10n_package->m_locale) == 0) && 
				pkg_exists(pkg_path, l->l10n_package)) {
			        CreateEntry(parent_rc, level, 
				    module, l->l10n_package, 
				    NULL, show_info, force_on);
			    }
			    l = l->l10n_next;
			}
		    }
		    /* if base pkg has a locale OR this is not a 
                     * localization view. 
		     */
		    else if (module->info.mod->m_locale || !v->v_locale) {
			CreateEntry(parent_rc, level, 
				module, module->info.mod, NULL, 
				show_info, force_on);
		    }
		}
	} while (!single_level && (module = get_next(module)));
}

void
CreateEntry(Widget parent, int level, Module * module, Modinfo * mi, 
		Widget * expand, Boolean show_info, Boolean force_on)
{
	Widget space = NULL, select, title, entryForm, size, toggle;
	XmString titleString, sizeString;
	int totalMB;
	char tmp[32];
        char tstring[1024];
	char* vers;
	char prod_name_version[128];
	ModStatus selectstatus;
	TreeData *sw_tree;
	ViewData * v;	
	Widget sw;
	EntryData * ed;

	XtVaGetValues(parent, XmNuserData, &v, NULL);

	entryForm = XtVaCreateWidget("entryForm",
	    xmFormWidgetClass, parent,
	    NULL);

	ed = (EntryData *) malloc(sizeof(EntryData));
	if (ed == NULL)
	    fatal(CANT_ALLOC_MSG);
	memset(ed, 0, sizeof(EntryData));

	ed->e_m = module;
	ed->e_mi = mi;

	XtVaSetValues(entryForm, XmNuserData, ed, NULL);

	if (show_info)
		XtAddCallback(entryForm, XmNfocusCallback, SetInfo, NULL);

	if (expand != NULL) {
		*expand = XtVaCreateManagedWidget("Expand",
		    xmArrowButtonGadgetClass, entryForm,
		    XmNwidth, (Dimension) ARROW_DIMENSION,
		    XmNheight, (Dimension) ARROW_DIMENSION,
		    XmNshadowThickness, (Dimension) 0,
		    XmNtopAttachment, XmATTACH_FORM,
		    XmNbottomAttachment, XmATTACH_FORM,
		    XmNleftAttachment, XmATTACH_FORM,
		    XmNarrowDirection, XmARROW_RIGHT,
		    XmNtraversalOn, False,
		    NULL);

	}
	select = XtVaCreateManagedWidget(mi->m_pkgid,
	    xmPushButtonWidgetClass, entryForm,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNtopOffset, 3,
	    XmNleftAttachment, XmATTACH_FORM,
	    XmNleftOffset, SELECT_OFFSET + INDENT * level,
	    XmNresizable, False,
	    XmNuserData, ed,
	    XmNlabelType, XmPIXMAP,
	    XmNwidth, (Dimension) BOX_DIMENSION,
	    XmNheight, (Dimension) BOX_DIMENSION,
	    XmNrecomputeSize, False,
	    XmNtraversalOn, False,
	    NULL);

	/* 
         * If force_on is true, I want the initial display of
         * toggle buttons to appear selected.
 	 * 
         * Upon reflection, this may not be necessary but it is now
         * working and I am not going to mess with it. (rgordon)
         */
	selectstatus = force_on ? SELECTED : mi->m_status;

	sw_tree = v->v_swTree;

	sw_tree->SelectList[sw_tree->Count].select = select;
	sw_tree->SelectList[sw_tree->Count].level = level;
	sw_tree->SelectList[sw_tree->Count].orig_status = selectstatus;

	if (selectstatus != REQUIRED) {
		XtAddCallback(select, XmNactivateCallback, SetSelection, v);
	}
	if (module->type == PRODUCT) {
		strcpy(prod_name_version, module->info.prod->p_name);
		strcat(prod_name_version, " ");
		strcat(prod_name_version, module->info.prod->p_version);
		strcat(prod_name_version, catgets(_catd, 8, 476, "."));
		strcat(prod_name_version, module->info.prod->p_rev);
		titleString = XmStringCreateLocalized(prod_name_version);
	} else {
		vers = (char*) get_prodvers(module);
		sprintf(tstring, "%s %s", mi->m_name, vers ? vers : "");
		titleString = XmStringCreateLocalized(tstring);
	}


	title = XtVaCreateManagedWidget("title",
	    xmPushButtonGadgetClass, entryForm,
	    XmNlabelString, titleString,
	    XmNtraversalOn, True,
	    XmNhighlightOnEnter, True,
	    XmNhighlightThickness, 2,
	    XmNshadowThickness, (Dimension) 0,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNbottomAttachment, XmATTACH_FORM,
	    XmNleftAttachment, XmATTACH_WIDGET,
	    XmNleftWidget, select,
	    NULL);
	XmStringFree(titleString);


	/*
	 * If the mi arg is _the_ module that matches locale,
         * then there is no need to traverse module (sub)tree
         * calculating size, simply use size of pkg for display.
         */
	if (v->v_locale && strcmp(mi->m_locale, v->v_locale) == 0)
	    totalMB = pkg_size(mi);
    	else
	    totalMB = selectedModuleSize(module, v->v_locale);

	if (totalMB != 0 && (totalMB / 1024) < (int) 1) {
		sprintf(tmp, catgets(_catd, 8, 477, "<1"));
	} else {
		sprintf(tmp, "%d", totalMB / 1024);
	}

	space =  XtVaCreateManagedWidget("space",
		xmLabelGadgetClass, entryForm,
		RSC_CVT( XmNlabelString, "    "),
		XmNalignment, XmALIGNMENT_END,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, title,
		XmNleftOffset, 10,
		NULL);

	sizeString = XmStringCreateLocalized(tmp);
	size = XtVaCreateManagedWidget("moduleSizeString",
	    xmLabelGadgetClass, entryForm,
	    XmNlabelString, sizeString,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNbottomAttachment, XmATTACH_FORM,
	    XmNrightAttachment, XmATTACH_FORM, 
	    XmNleftAttachment, XmATTACH_WIDGET,
	    XmNleftWidget, space == NULL ? title : space,
	    XmNleftOffset, INDENT,
	    XmNalignment, XmALIGNMENT_END,
 	    XmNrecomputeSize, False,
	    NULL);
	XmStringFree(sizeString);

	/* if module type is cluster then set Select size to size */
	if (expand == NULL) {
		sw_tree->SelectList[sw_tree->Count].size = NULL;
	} else {
		sw_tree->SelectList[sw_tree->Count].size = size;
	}

	sw_tree->Count++;

	XtVaSetValues(entryForm,
	    XmNinitialFocus, title,
	    NULL);

	XtManageChild(entryForm);
}


void
ExpandCluster(Widget button, XtPointer child_rc, XtPointer call_data)
{
	if (XtIsManaged((Widget) child_rc) == False) {
		XtManageChild((Widget) child_rc);
		XtVaSetValues(button,
		    XmNarrowDirection, XmARROW_DOWN,
		    NULL);
	} else {
		XtUnmanageChild((Widget) child_rc);
		XtVaSetValues(button,
		    XmNarrowDirection, XmARROW_RIGHT,
		    NULL);
	}
}

static EntryData * InfoEntryData = NULL;
Widget InfoForm = NULL;

void
SetInfo(Widget form, XtPointer call_data, XtPointer client_data)
{
	Modinfo * mi = NULL;
	EntryData *ed;
	ViewData * v;

	XtVaGetValues(form, XmNuserData, &ed, NULL);

/* when invoked as callback, call_data is NULL. when called directly,
   we want to force call to update_detail_text
*/

	if (InfoEntryData && (InfoEntryData == ed))
		return;

	InfoForm = form; 
	InfoEntryData = ed;

	XtVaGetValues(XtParent(form), XmNuserData, &v, NULL);
	set_basedirCB(v->v_basedir_value, NULL, NULL);

	XmProcessTraversal(form, XmTRAVERSE_CURRENT);

	update_detail_text(v->v_pkginfoText, ed->e_m, ed->e_mi);
	update_basedir_field(v->v_basedir_value, ed->e_m, ed->e_mi);
}


/*
 * mod_rm_status() - Like mod_status, except for removals
 */
static int
mod_rm_status(Module *mod)
{
	register int    n, m;
	register Module *mp;

	if (mod->type == MEDIA)
		return (ERR_INVALIDTYPE);

	if (mod->type == PRODUCT || mod->type == NULLPRODUCT) {
		if (mod->sub == (Module *)0)
			return (UNSELECTED);
	} else if (mod->sub == (Module *)0)
		return (mod->info.mod->m_action == TO_BE_REMOVED ?
							SELECTED : UNSELECTED);

	for (n = 0, m = 0, mp = mod->sub; mp; mp = mp->next) {
		if (mp->sub != (Module *)0) {
			if (mod_rm_status(mp) == SELECTED) {
				mp->info.mod->m_action = TO_BE_REMOVED;
			} else if (mod_rm_status(mp) == PARTIALLY_SELECTED) {
				mp->info.mod->m_action = NO_ACTION_DEFINED;
				mp->info.mod->m_status = PARTIALLY_SELECTED;
			} else { 
				mp->info.mod->m_action = NO_ACTION_DEFINED;
				mp->info.mod->m_status = LOADED;
			}
		}
		n++;
		if (mp->info.mod->m_action == TO_BE_REMOVED)
			m++;
	}

	if (n == 0) {
		return (mod->info.mod->m_action == TO_BE_REMOVED ?
							SELECTED : UNSELECTED);
	} else if (m == 0)
		return (UNSELECTED);
	else if (m == n)
		return (SELECTED);
	else if (m < n)
		return (PARTIALLY_SELECTED);
	else
		return (ERR_INVALID);

}

void
set_selection(Module *mod, ModStatus status, char * loc)
{
	Action		action;
	Module		*subp, *parentp;

	parentp = mod;
	while (parentp && parentp->parent != (Module *)0 &&
	    parentp->type != PRODUCT && 
	    parentp->type != METACLUSTER && 
	    parentp->type != NULLPRODUCT)
		parentp = parentp->parent;


	if (mod->type == PRODUCT) {
		mark_cluster(mod, status, loc);
	} else {
		mark_cluster(mod, status, loc);
		/*
		* Propagate status up the tree, if this
	        * module has a parent.
		*/
		if (parentp && (parentp != mod)) {
			status = mod_status(parentp);

			if (mod->type == METACLUSTER) {
				Module * m = parentp->sub;
				while (m) {
					mark_cluster(m, mod_status(m), loc);
					m = m->next;
				}
			}
			if (parentp->type == PRODUCT ||
				   parentp->type == NULLPRODUCT)
				parentp->info.prod->p_status = status;
			else
				parentp->info.mod->m_status = status;
		}
	}
}

void
toggle_selection(Module *mod, Modinfo * mi, char * loc, int mode)
{
	ModStatus	status;
	Action		action;
	Module		*subp, *parentp;

	parentp = get_parent_product(mod);

	if (mode == MODE_INSTALL) {
		if (mod->type == PRODUCT) {
			if (mod->info.prod->p_status == SELECTED)
				status = UNSELECTED;
			else
				status = SELECTED;
		} else if (getModuleStatus(mod, loc) == SELECTED) {
			status = UNSELECTED;
			/* 
                        * FIX ME...if clusters are displayed in
                        * view window this will not correctly
                        * propagate selection. Specifically, all
                        * pkgs for a give L10N will get status set
                        * when only one has been selected.
                        */
			if (mod->type == PACKAGE)
			    mi->m_status = status;
			else
			    mark_cluster(mod, UNSELECTED, loc);
		} else {
			status = SELECTED;
			/* FIX ME also...as above */
			if (mod->type == PACKAGE)
			    mi->m_status = status;
			else
			    mark_cluster(mod, SELECTED, loc);
		}

		if (mod->type == PRODUCT) {
			toggle_product(mod, status);
		} else {
			/*
			 * Propagate status up the tree, if module
			 * has a parent.
			 */
			if (parentp && (parentp != mod)) {
				status = mod_status(parentp);

			    if (mod->type == METACLUSTER) {
				Module * m = parentp->sub;
				while (m) {
					m->info.mod->m_status = mod_status(m);	
					m = m->next;
				}
			    }

			    if (parentp->type == PRODUCT ||
					 parentp->type == NULLPRODUCT)
				parentp->info.prod->p_status = status;
			    else
				parentp->info.mod->m_status = status;
			}
		}
	} 
}

void
display_software(ViewData * v)
{
	XmString totalString;
	char tmp[100];
	int size=0;
	TreeData * sw_tree = v->v_swTree;

	/*
	 * get size of all packages/clusters selected and display in total
	 * label
	 */
	size = selectedModuleSize(FocusData->f_module, v->v_locale);
	update_size_label(v->v_totalLabel, 
			catgets(_catd, 8, 478, "Total (MB): "), size);

	pfgupdate_software(sw_tree, v);
 	showFilteredDependencies(v);
}


void
SetSelection(Widget select, XtPointer v , XtPointer *call_data)
{
	EntryData * ed;
	Modinfo * mi;
	Module *module = NULL;
	ViewData * view = (ViewData *)v;
	TreeData * sw_tree = view->v_swTree;

	if (select != NULL)
		XtVaGetValues(select, XmNuserData, &ed, NULL);

  	module = ed->e_m;	
	mi = ed->e_mi;
	if (module != NULL) {
		SetInfo(XtParent(select), NULL, NULL);
		toggle_selection(module, mi, view->v_locale, sw_tree->mode); 	
	}

	display_software((ViewData *)v);
}


void
CreateSelectPixmap(Widget button, Pixmap *unselectPixmap,
    Pixmap *selectPixmap, Pixmap *partialPixmap, Pixmap *requiredPixmap)
{
	XGCValues values;
	int width = 18, height = 18;
	XPoint points[4];
	GC select_gc, partial_gc, unselect_gc, required_gc;
	Pixel unselectcolor, foreground;
	XWindowAttributes windowattr;
extern Display		*Gdisplay;

	XtVaGetValues(XtParent(button),
	    XmNbackground, &unselectcolor,
	    XmNforeground, &foreground,
	    NULL);

	values.foreground = BlackPixelOfScreen(XtScreen(button));
	values.background = unselectcolor;

	values.fill_style = FillStippled;

	select_gc = XCreateGC(Gdisplay,
	    RootWindowOfScreen(XtScreen(button)), (GCForeground | GCBackground), &values);

	partial_gc = XCreateGC(Gdisplay,
	    RootWindowOfScreen(XtScreen(button)), (GCForeground | GCBackground), &values);

	values.foreground = unselectcolor;
	values.background = unselectcolor;
	values.line_width = 2;
	required_gc = XCreateGC(Gdisplay,
	    RootWindowOfScreen(XtScreen(button)),
	    (GCLineWidth | GCForeground | GCBackground), &values);

	unselect_gc = XCreateGC(Gdisplay,
	    RootWindowOfScreen(XtScreen(button)), (GCForeground | GCBackground), &values);

	*partialPixmap = XCreatePixmap(Gdisplay,
	    RootWindowOfScreen(XtScreen(button)), (unsigned int) width,
	    (unsigned int) height, DefaultDepthOfScreen(XtScreen(button)));

	*selectPixmap = XCreatePixmap(Gdisplay,
	    RootWindowOfScreen(XtScreen(button)), width, height, DefaultDepthOfScreen(XtScreen(button)));

	*unselectPixmap = XCreatePixmap(Gdisplay,
	    RootWindowOfScreen(XtScreen(button)), width, height, DefaultDepthOfScreen(XtScreen(button)));

	*requiredPixmap = XCreatePixmap(Gdisplay,
	    RootWindowOfScreen(XtScreen(button)), width, height, DefaultDepthOfScreen(XtScreen(button)));

	/* Draw criss-cross lines in *partial_gc */

	points[0].x = 0;
	points[0].y = 0;
	points[1].x = width;
	points[1].y = height;
	points[2].x = 0;
	points[2].y = height;
	points[3].x = width;
	points[3].y = 0;
	XFillRectangle(Gdisplay, *requiredPixmap,
	    select_gc, 0, 0, width, height);
	XDrawLines(Gdisplay, *requiredPixmap,
	    required_gc, points, 4, CoordModeOrigin);

	/* Draw filled in triangle in *partial_gc */

	points[0].x = width;
	points[0].y = 0;
	points[1].x = width;
	points[1].y = height;
	points[2].x = 0;
	points[2].y = height;
	XFillRectangle(Gdisplay, *partialPixmap,
	    unselect_gc, 0, 0, width, height);
	XFillPolygon(Gdisplay, *partialPixmap,
	    partial_gc, points, 3,
	    Convex, CoordModeOrigin);

	XFillRectangle(Gdisplay, *selectPixmap,
	    select_gc, 0, 0, width, height);

	XFillRectangle(Gdisplay, *unselectPixmap,
	    unselect_gc, 0, 0, width, height);
}


void
pfgupdate_software(TreeData *sw_tree, ViewData * v)
{
	int i;
	EntryData *ed;
	Module * module;
	int total;
	char tmp[10];
	XmString sizeString;
	ModStatus selectStatus;
	Modinfo * mi;

	for (i = 0; i < sw_tree->Count; i++) {
		XtVaGetValues(sw_tree->SelectList[i].select,
		    XmNuserData, &ed,
		    NULL);
		module = ed->e_m;
		if (module->type == PRODUCT)
		    selectStatus = module->info.prod->p_status;
		else {
		    mi = ed->e_mi;

		    selectStatus = mi ? mi->m_status : 
					module->info.mod->m_status;
		}
		update_selection(sw_tree->SelectList[i].select, 
			selectStatus, module);
		/* set cluster size after selection */
		if (sw_tree->SelectList[i].size != NULL) {
			total = selectedModuleSize(module, v->v_locale);
			if (total != 0 && total / 1024 < (int) 1) {
				sprintf(tmp, catgets(_catd, 8, 479, "<1"));
			} else {
				sprintf(tmp, "%d", total / 1024);
			}
			sizeString = XmStringCreateLocalized(tmp);
			XtVaSetValues(sw_tree->SelectList[i].size,
			    XmNlabelString, sizeString,
			    XmNalignment, XmALIGNMENT_END,
			    NULL);
			XmStringFree(sizeString);
			XmUpdateDisplay(sw_tree->SelectList[i].select);
		}
	}
}

void
update_selection(Widget select, ModStatus selectStatus, Module *mod)
{

	if (selectStatus == UNSELECTED) {
		XtVaSetValues(select,
		    XmNlabelPixmap, XmUNSPECIFIED_PIXMAP,
		    XmNalignment, XmALIGNMENT_BEGINNING,
		    NULL);
	} else if (selectStatus == SELECTED) {
		XtVaSetValues(select,
		    XmNlabelPixmap, selectpixmaps.select,
		    XmNalignment, XmALIGNMENT_CENTER,
		    NULL);
	} else if (selectStatus == PARTIALLY_SELECTED) {
		XtVaSetValues(select,
		    XmNlabelPixmap, selectpixmaps.partial,
		    XmNalignment, XmALIGNMENT_CENTER,
		    NULL);
	} else if (selectStatus == REQUIRED) {
		XtVaSetValues(select,
		    XmNlabelInsensitivePixmap, selectpixmaps.required,
		    XmNalignment, XmALIGNMENT_CENTER,
		    NULL);
		XtVaSetValues(select,
		    XmNsensitive, False,
		    NULL);
	} else if (selectStatus == LOADED) {
		if (mod->info.mod->m_action == NO_ACTION_DEFINED)
			XtVaSetValues(select,
		    		XmNlabelPixmap, XmUNSPECIFIED_PIXMAP,
		    		XmNalignment, XmALIGNMENT_BEGINNING,
		    		NULL);
		else if (mod->info.mod->m_action == TO_BE_REMOVED)
			XtVaSetValues(select,
		    		XmNlabelPixmap, selectpixmaps.select,
		    		XmNalignment, XmALIGNMENT_CENTER,
		    		NULL);

	}

}

#define IsRequiredStr  catgets(_catd, 8, 480, " requires:\n")

void
showFilteredDependencies(ViewData * v)
{
	Depend 				*dependents = NULL;
	Node 				*node, *node_dep;
	Module 				*module;
	char 				* s, * tmp;
	int 				cnt, tmplen = 1024;
	Boolean				titled = False;
	Widget				depend_list = v->v_dependencyText;
	List 				*package_list;
	int 				i = 0, j;
	char 				indent_name[80];
	char 				* pkgid, prev_pkgid[16];

	pkgid = FocusData->f_module->info.mod->m_pkgid;

	if (!check_sw_depends()) {
		XmTextSetString(depend_list, "");
		return;
	}
	dependents = get_depend_pkgs();
	if (dependents == NULL) {
		XmTextSetString(depend_list, "");
		return;
	}
	module = get_current_product();

	package_list = module->info.prod->p_packages;
	if (package_list == NULL) {
		return;
	}
	prev_pkgid[0] = '\0';
	tmp = malloc(tmplen);
	if (tmp == NULL)
		fatal(CANT_ALLOC_MSG);
 	memset(tmp, 0, tmplen);
	cnt = 1;
	while (dependents != NULL) {
		if (is_sub(FocusData->f_module, dependents->d_pkgid)) {
		    	node = findnode((List *) package_list, 
						dependents->d_pkgid);
			node_dep = findnode((List *) package_list, 
				dependents->d_pkgidb);
			if (is_installed(((Modinfo *)node_dep->data)->m_pkgid) < 0)
			{
			    s = ((Modinfo *)node->data)->m_name;
			    if (strcmp(prev_pkgid, dependents->d_pkgid)) {
				i += (strlen(s) + strlen(IsRequiredStr) + 2);
				if (i > tmplen * cnt) {
					tmp = realloc(tmp, tmplen * (++cnt));
					memset(tmp+(tmplen*(cnt-1)),0,tmplen);
				}
				strcat(tmp, s);
				strcat(tmp, IsRequiredStr);
				strcpy(prev_pkgid, dependents->d_pkgid);
			    }
			    s = ((Modinfo *)node_dep->data)->m_name;
			    sprintf(indent_name, "\t%s\n", s);
			    i += (strlen(indent_name) + 1 + 4);
			    if (i > tmplen * cnt)
				tmp = realloc(tmp, tmplen * (++cnt));
			    strcat(tmp, indent_name);
			}
		}
		dependents = dependents->d_next;
	}
	XmTextSetString(depend_list, tmp);
	free(tmp);
}

Widget
createLegend(Widget parent)
{
	Widget frame, rc, form, symbol;
	XmString xmstr;

	frame = XtVaCreateWidget("Frame",
	    xmFrameWidgetClass, parent,
	    XmNshadowType, XmSHADOW_ETCHED_OUT,
	    NULL);

	form = XtVaCreateManagedWidget("form",
	    xmFormWidgetClass, frame,
	    NULL);

	rc = XtVaCreateWidget("Legend",
	    xmRowColumnWidgetClass, form,
	    XmNentryAlignment, XmALIGNMENT_END,
	    XmNnumColumns, 3,
	    XmNpacking, XmPACK_COLUMN,
	    XmNtraversalOn, False,
	    XmNadjustLast, False,
	    XmNleftAttachment, XmATTACH_FORM,
		/* XmNleftOffset, 50, */
	    NULL);

	/* Expand */
	form = XtVaCreateManagedWidget("form",
	    xmFormWidgetClass, rc,
	    NULL);

	symbol = XtVaCreateManagedWidget("symbol",
	    xmArrowButtonGadgetClass, form,
	    XmNshadowThickness, 0,
	    XmNarrowDirection, XmARROW_RIGHT,
	    NULL);
	xmstr = XmStringCreateLocalized(catgets(_catd, 8, 481, "Collapsed cluster"));
	(void) XtVaCreateManagedWidget("label",
	    xmLabelGadgetClass, form,
	    XmNlabelString, xmstr,
	    XmNleftAttachment, XmATTACH_WIDGET,
	    XmNleftWidget, symbol,
	    NULL);
	XmStringFree(xmstr);

	/* Contract */
	form = XtVaCreateManagedWidget("form",
	    xmFormWidgetClass, rc,
	    NULL);

	symbol = XtVaCreateManagedWidget("symbol",
	    xmArrowButtonGadgetClass, form,
	    XmNshadowThickness, 0,
	    XmNarrowDirection, XmARROW_DOWN,
	    NULL);
	xmstr = XmStringCreateLocalized(catgets(_catd, 8, 482, "Expanded cluster"));
	(void) XtVaCreateManagedWidget("label",
	    xmLabelGadgetClass, form,
	    XmNlabelString, xmstr,
	    XmNleftAttachment, XmATTACH_WIDGET,
	    XmNleftWidget, symbol,
	    NULL);
	XmStringFree(xmstr);

	/* Required */
	form = XtVaCreateManagedWidget("form",
	    xmFormWidgetClass, rc,
	    NULL);

	LegendButtons[0].orig_status = REQUIRED;
	LegendButtons[0].select = symbol = XtVaCreateManagedWidget("symbol",
	    xmPushButtonWidgetClass, form,
	    XmNlabelType, XmPIXMAP,
	    XmNwidth, 20,
	    XmNheight, 20,
	    XmNresizable, False,
	    XmNleftAttachment, XmATTACH_FORM,
	    XmNleftOffset, (pfgLowResolution ? 0 : 30),
	    NULL);
	xmstr = XmStringCreateLocalized(catgets(_catd, 8, 483, "Required"));
	(void) XtVaCreateManagedWidget("label",
	    xmLabelGadgetClass, form,
	    XmNlabelString, xmstr,
	    XmNleftAttachment, XmATTACH_WIDGET,
	    XmNleftWidget, symbol,
	    NULL);
	XmStringFree(xmstr);

	/* Partial */
	form = XtVaCreateManagedWidget("form",
	    xmFormWidgetClass, rc,
	    NULL);

	LegendButtons[1].orig_status = PARTIALLY_SELECTED;
	LegendButtons[1].select = symbol = XtVaCreateManagedWidget("symbol",
	    xmPushButtonWidgetClass, form,
	    XmNlabelType, XmPIXMAP,
	    XmNwidth, 20,
	    XmNheight, 20,
	    XmNresizable, False,
	    XmNleftAttachment, XmATTACH_FORM,
	    XmNleftOffset, (pfgLowResolution ? 0 : 30),
	    NULL);
	xmstr = XmStringCreateLocalized(catgets(_catd, 8, 484, "Partial"));
	(void) XtVaCreateManagedWidget("label",
	    xmLabelGadgetClass, form,
	    XmNlabelString, xmstr,
	    XmNleftAttachment, XmATTACH_WIDGET,
	    XmNleftWidget, symbol,
	    NULL);
	XmStringFree(xmstr);

	/* Selected */
	form = XtVaCreateManagedWidget("form",
	    xmFormWidgetClass, rc,
	    NULL);

	LegendButtons[2].orig_status = SELECTED;
	LegendButtons[2].select = symbol = XtVaCreateManagedWidget("symbol",
	    xmPushButtonWidgetClass, form,
	    XmNlabelType, XmPIXMAP,
	    XmNwidth, 20,
	    XmNheight, 20,
	    XmNresizable, False,
	    NULL);
	xmstr = XmStringCreateLocalized(catgets(_catd, 8, 485, "Selected"));
	(void) XtVaCreateManagedWidget("label",
	    xmLabelGadgetClass, form,
	    XmNlabelString, xmstr,
	    XmNleftAttachment, XmATTACH_WIDGET,
	    XmNleftWidget, symbol,
	    NULL);
	XmStringFree(xmstr);

	/* Unselected */
	form = XtVaCreateManagedWidget("form",
	    xmFormWidgetClass, rc,
	    NULL);

	LegendButtons[3].orig_status = UNSELECTED;
	LegendButtons[3].select = symbol = XtVaCreateManagedWidget("symbol",
	    xmPushButtonWidgetClass, form,
	    XmNlabelType, XmPIXMAP,
	    XmNwidth, 20,
	    XmNheight, 20,
	    XmNresizable, False,
	    NULL);
	xmstr = XmStringCreateLocalized(catgets(_catd, 8, 486, "Unselected"));
	(void) XtVaCreateManagedWidget("label",
	    xmLabelGadgetClass, form,
	    XmNlabelString, xmstr,
	    XmNleftAttachment, XmATTACH_WIDGET,
	    XmNleftWidget, symbol,
	    NULL);
	XmStringFree(xmstr);

	XtManageChild(rc);
	return (frame);
}


void
initializeLegend(SelectionList * legendButtons, int buttonCount)
{
	int i;

	for (i = 0; i < buttonCount; i++) {
/*		update_selection(legendButtons[i].select,
		    legendButtons[i].orig_status);
*/
	}
}

void
fatal(char * msg)
{
	fprintf(stderr, catgets(_catd, 8, 487, "Fatal: %s\n"), msg);
	exit(-1);
}

void
update_detail_text(Widget text, Module * m, Modinfo * mi)
{
	int i, sizeLength = 5;
	char *tmp;
	FSspace **space;
	char 	pinfo[1024], prod_name_version[256];
	int ip, lip, dlen;
	short  ncols;
	char * dp, *dhp, *dholder = NULL;
	int adj, sz, k;
	char * loc;
	
        /*
         * FIX ME FIX ME FIX ME
         * When L10N constraints loosen up, 'detail text' should
         * reflect appropriate information for PRODUCT
         */
        if (m->type == PRODUCT || m->type == NULLPRODUCT)
		return;

	XtVaGetValues(text, XmNcolumns, &ncols, NULL);

	loc = mi->m_locale;
	/* 
         * If may be the case that there is no description text. 
         * If so, then dholder will be NULL below and handled
         * appropriately.
         */
	if (dp = mi->m_desc) {
		dlen = strlen(dp)+1;
		ip = ncols * 2;
		dhp = dholder = malloc(dlen + dlen/ip +1);
		memset(dhp, 0, dlen + dlen/ip +1);
		lip = 0;
		while (ip < dlen && dp[ip] != '\0') {
			while (!isspace(dp[ip]) && !ispunct(dp[ip])) ip--;
			strncat(dholder, dp+lip, ip-lip);
			dhp += ip - lip;
			strcat(dholder, "\n\t");
			dhp += 2;
			lip = ip + 1;
			ip += ncols * 2;
		}
		strncat(dholder, dp+lip, dlen - lip);
	}
	sprintf(pinfo, 
	      catgets(_catd, 8, 490, "Name:\t\t%s\nAbbreviation:\t%s\nVendor:\t\t%s\nDescription:\t%s"),
		(mi->m_name ? mi->m_name : ""),
		(mi->m_pkgid ? mi->m_pkgid : ""),
		(mi->m_vendor ? mi->m_vendor : ""),
		(dholder ? dholder : ""));

	strcat(pinfo, catgets(_catd, 8, 491, "\nEstimated Size (MB):"));

	i = 0;
	adj = N_INSTALL_FS % 2 == 0 ? 0 : 1;
#define SPACE_FORMAT 14

	space = calc_cluster_space(m, UNSELECTED);

	/*
         * For display of filesystem size, I iterate through filesystems
         * sprintf'ing two at a time into a string such that entries
         * i and i+N_INSTALL/2 are on same row.
         *
         * The size displayed depends on whether or not I am displaying
         * base odule in which case, I use 'space' array calculated from
         * calc_cluster_space or locale pkg, in which case I simply
         * sum sizes in Modinfo structure.
         */
	for (i = 0; i < N_INSTALL_FS/2 + adj; i++) {
		tmp = (char *) xmalloc((SPACE_FORMAT +
			strlen(installFileSystems[i]) + 1));

		sz = loc ? get_pkg_fs_space(mi, installFileSystems[i]) :
			   get_fs_space(space, installFileSystems[i]) / 1024;
		if (sz != 0 && sz / 1024 == 0)
		    sprintf(tmp,"\n\t%s\t%s",installFileSystems[i],catgets(_catd, 8, 492, "<1"));
		else 
		    sprintf(tmp,"\n\t%s\t%d",installFileSystems[i],sz/1024);

		strcat(pinfo, tmp);
		free(tmp);

		k = N_INSTALL_FS/2 + i;
		if (installFileSystems[k]) {
		    sz = loc ? get_pkg_fs_space(mi, installFileSystems[k]) :
			       get_fs_space(space, installFileSystems[k]) / 1024 ;
		    tmp = (char *) xmalloc((SPACE_FORMAT +
			strlen(installFileSystems[k]) + 1));
		    if (sz != 0 && sz / 1024 == 0)
		        sprintf(tmp,"\t%s\t%s",installFileSystems[k],catgets(_catd, 8, 493, "<1"));
		    else 
		        sprintf(tmp,"\t%s\t%d",installFileSystems[k],sz/1024);
	 	    strcat(pinfo, tmp);
		    free(tmp);
		}
	}
	XmTextSetString(text, pinfo);
}
	
Widget 
create_detail(Widget parent, char * banner, int nrows, char * type)
{
	Widget		title_widget;
	Widget		detail;
	Arg		args[32];
	XmFontList	fontlist;
	int		ac = 0;
	char tmpbuf[128];

	sprintf(tmpbuf, "%dDetailTitle", type);
	title_widget =  XtVaCreateManagedWidget(tmpbuf,
		    xmLabelGadgetClass, parent,
		    XmNtopAttachment, XmATTACH_FORM,
		    XmNleftAttachment, XmATTACH_FORM,
	    	    RSC_CVT( XmNlabelString, banner),
		    NULL);
	XtVaGetValues(title_widget,
                XmNfontList, &fontlist,
                NULL);

	ac = 0;
	XtSetArg(args[ac], XmNblinkRate, 0); ac++;
	XtSetArg(args[ac], XmNautoShowCursorPosition, True); ac++;
	XtSetArg(args[ac], XmNcursorPositionVisible, False); ac++;
	XtSetArg(args[ac], XmNeditable, False); ac++;
	XtSetArg(args[ac], XmNeditMode, XmMULTI_LINE_EDIT); ac++;
	XtSetArg(args[ac], XmNrows, nrows); ac++;
	XtSetArg(args[ac], XmNtraversalOn, False); ac++;
	XtSetArg(args[ac], XmNfontList, fontlist); ac++;
	XtSetArg(args[ac], XmNwordWrap, True); ac++;

	sprintf(tmpbuf, "%dDetailText", type);
	detail = XmCreateScrolledText(parent, tmpbuf, args, ac);

	XtVaSetValues(XtParent(detail),
	    	XmNtopAttachment, XmATTACH_WIDGET,
	    	XmNtopWidget, title_widget,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNbottomOffset, 10,
		XmNvisualPolicy, XmVARIABLE,
	    	NULL);

	XtManageChild(detail);
	return(detail);
}

void
TurnOffSashTraversal(Widget pane)
{
	Widget * children;
	int nchildren;

	XtVaGetValues(pane, 
		XmNchildren, &children,
		XmNnumChildren, &nchildren,
		NULL);
	while (nchildren-- > 0) 
		if (XtIsSubclass(children[nchildren], xmSashWidgetClass))
			XtVaSetValues(children[nchildren], 
				XmNtraversalOn, False, NULL);
}

/*
 * Callback for select/deselect button. 
 */

void
toggleSelectAllCB(
	Widget w, 
	XtPointer cd, 
	XtPointer cbs)
{
	Module 		* parent_module = (Module *) cd;
	XmString 	lab;
	char 		* s;
	ViewData	* v;

	XtVaGetValues(w, XmNlabelString, &lab, NULL);
	XmStringGetLtoR(lab, XmSTRING_DEFAULT_CHARSET, &s);

	XtVaGetValues(w, XmNuserData, &v, NULL);
	
	if (strcmp(s, DESELECT_ALL_STRING) == 0) {
		set_selection(parent_module, UNSELECTED, v->v_locale);
	} else if (strcmp(s, SELECT_ALL_STRING) == 0) {
		set_selection(parent_module, SELECTED, v->v_locale);
	}
        display_software(v);
}


/* 
 * Search sw_list to see if a package pkgid is installed.
 * If not return -1. If true, return index into sw_list table.
 */

int
is_installed(char * pkgid)
{
#ifndef SW_INSTALLER
	extern SWStruct	* sw_list;
	extern int	sw_list_cnt;
	int i;

	for (i = 0; i < sw_list_cnt; i++) {
		if (strcmp(pkgid, sw_list[i].sw_id) == 0) {
			return(i);
		}
	}
#endif

	return(-1);
}

