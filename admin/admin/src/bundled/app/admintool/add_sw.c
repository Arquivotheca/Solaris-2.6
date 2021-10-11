/* Copyright 1993 Sun Microsystems, Inc. */

#pragma ident "@(#)add_sw.c	1.80 96/09/23 Sun Microsystems"

/*	add_sw.c 	*/

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
#include <Xm/PanedW.h>
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
#include <libintl.h>
#include "spmisoft_api.h"
#include "media.h"
#include "software.h"
#include "util.h"

#include "Meter.h"

extern nl_catd	_catd;	/* for catgets(), defined in main.c */
extern Widget customDialogForm;
extern Widget customDialog;
extern ViewData * customize_sw_dialog(Widget, 
		addCtxt *, char *, Module *, char *, 
		Boolean);
extern TreeData * reset_software_view(TreeData * , Module * , Modinfo * ,
	char *, char * , int , Boolean , Boolean , Boolean );

extern Module *get_source_sw(char *);

extern Module * selectMediaPath(Widget, 
			enum sw_image_location * image_loc, 
			char **, char **);

extern Module * get_parent_product(Module *);
extern void reset_selected_modules(Module *);
extern int reset_selected_pkgs(Node *, caddr_t);
extern void reset_fud(swFormUserData *, caddr_t);
extern void fud_selection_status(swFormUserData *, caddr_t);

extern Widget create_detail(Widget, char *, int, char *);
extern void update_detail_text(Widget, Module *, Modinfo *);

extern int reset_status(Node *, caddr_t);
extern void apply_to_all_products(addCtxt *, int (*)(Node *, caddr_t), caddr_t);

/* internal functions */
static void	init_add_sw_dialog(addCtxt*, Module *);

Widget 	addsoftwaredialog = NULL;

int do_add_sw(addCtxt *);

/* Msg displayed when user has selected a "risky" pkg for
 * post-install addition.
 */
#define ADD_RISKY_MSG \
catgets(_catd, 8, 503, \
"The package(s) you have selected contain core\n\
functionality required for your system to operate properly.\n\
Such packages are typically installed during an initial\n\
or upgrade Solaris installation. Installing such a package\n\
with admintool may corrupt your system and leave it unusable.\n\
Do you wish to install selected package?")

#define LOCALIZATION_STRING catgets(_catd, 8, 652, "Localization")
#define LOCALIZATIONS_STRING catgets(_catd, 8, 653, "Localizations")

#define REBOOT_REQUIRED_MSG  \
catgets(_catd, 8, 778, \
"Installation was successful.\n\
This machine must now be rebooted in order to ensure\n\
correct operation. Execute:\n\
   shutdown -y -i6 -g0\n\
and wait for the \"Console Login:\" prompt.")

/*
 * Global context for add operations.
 */
addCtxt * ctxt = NULL;

/* 
 * Given a Label gadget/widget update it with int sz
 */
void
update_size_label(Widget w, char * tag, int sz)
{
	XmString xmstr;
	char * tmp;
	static char fmt0[]  = "%s%4d";
	static char fmt3[]  = "%s%4s";
	static char fmt1[]  = "%4d";
	static char fmt2[]  = "%4s";
	int 	truncsz = sz/1024;

	if (tag) {
		tmp = malloc(strlen(tag)+strlen(fmt0)+1+6);
		if (sz == 0)
			sprintf(tmp, fmt0, tag, 0);
		else { 
			if (truncsz == 0) 
				sprintf(tmp, fmt3, tag, catgets(_catd, 8, 80, "<1"));	
			else
				sprintf(tmp, fmt0, tag, truncsz);
		}
	} else {
		tmp = malloc(strlen(fmt1)+1+6);
		if (sz == 0)
			sprintf(tmp, fmt1, 0);
		else { 
			if (truncsz == 0)
				sprintf(tmp, fmt2, catgets(_catd, 8, 81, "<1"));
			else
				sprintf(tmp, fmt1, truncsz);
		}
	}

	XtVaSetValues(w, XmNlabelString, 
			xmstr = XmStringCreateLocalized(tmp), 
/*
			XmNtopAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_FORM,
*/
			NULL);
	free(tmp);
	XmStringFree(xmstr);
}

/* Packages displayed in main add window appear in forms which
 * have swFormUserData attached. This routine calculate sizes
 * of selected product/packages
 */

void
update_product_sizes(Widget w, char * loc)
{
/* arg w is a toggle inside a form inside a row_col */
	Widget rc = XtParent(XtParent(w));
	WidgetList kids, gkids;
	Widget form;
	int i;
	Cardinal nkids;
	swFormUserData * fud;
	extern ModStatus selectionStatus(Module *, char *);
	int sz = 0;
		
	XtVaGetValues(rc, XmNchildren, &kids,
			  XmNnumChildren, &nkids,
			  NULL);
	for (i = 0; i < nkids; i++) {
	/* each kid is a Form. Form has swFormUserData attached */
		XtVaGetValues(kids[i], XmNuserData, &fud, NULL);
		sz = selectedModuleSize(fud->f_module, fud->f_locale);
		XtVaGetValues(kids[i], XmNchildren, &gkids, NULL);
		if (fud->f_selected) {
		    update_size_label(gkids[SZ_LABEL_IX], NULL, sz);
		} else
		    update_size_label(gkids[SZ_LABEL_IX], NULL, 0);

	}
	
}

void
reset_product_selection(addCtxt * ctxt)
{
	int i;
	Cardinal nkids;
	WidgetList kids, gkids;
        swFormUserData * fud;
	Widget rc = ctxt->rowcol;
        Module * mod = get_parent_product(ctxt->top_level_module);

	/* 
         * walklist only works for a pkg list hanging off of a PRODUCT/
         * NULLPRODUCT module. If we are look at a generic directory of pkgs,
	 * ie, no .clustertoc, we won't have such a list so we will
         * manually reset all the modules.
	 */
	if (mod)
            apply_to_all_products(ctxt, reset_status, (caddr_t) ctxt);
	else {
	    mod = ctxt->top_level_module;
	    while (mod) {
		reset_module(mod);
 		mod = get_next(mod);
	    }
	}
	traverse_fud_list(reset_fud, NULL);

	XtVaGetValues(rc, XmNchildren, &kids,
                          XmNnumChildren, &nkids,
                          NULL);

	 for (i = 0; i < nkids; i++) {
                XtVaGetValues(kids[i], XmNchildren, &gkids, NULL);
                update_size_label(gkids[SZ_LABEL_IX], NULL, 0);
                XtVaSetValues(gkids[0], XmNset, False, NULL);
        }
	update_size_label(ctxt->totalLabel, catgets(_catd, 8, 82, "     Total (MB): "), 0);
	XtSetSensitive(ctxt->customize_btn, False);
	XtSetSensitive(XmMessageBoxGetChild(ctxt->button_box, 
				XmDIALOG_OK_BUTTON), False);
}
		
		

/*
/*******************************************************************************
       The following are callback functions.
*******************************************************************************/
int FudInvalid = 0;

void
SetFocus(Widget form, XtPointer cd, XmAnyCallbackStruct * cbs)
{
	addCtxt* ctxt = (addCtxt*)cd;
	TreeData * sw_tree;
	Module * m;
	Widget	tog;
	swFormUserData	* fud;
	
        /* This is as ugly as it gets. I must prevent this code
         * from being run if Module in fud is invalid because of
	 * failed attempt to read to new module tree.
 	 */
        if (FudInvalid)
	    return;

	XtVaGetValues(form, XmNuserData, &fud, NULL);
	m = fud->f_module;
	tog = fud->f_toggle;

	/* 
         * To prevent unncessary update when SetFocus is called
         * when cursor moves into form widget, check if current
         * focus, FocusData, and pending new focus are the same.
         * 
         * Two fuds are the 'same' for this purpose is their modules
         * have the same name and the have the same value in their
         * f_locale field.
         */
	if (FocusData && 
	   (strcmp(get_mod_name(FocusData->f_module), get_mod_name(m)) == 0) &&
           same_fud_locales(FocusData, fud) &&
	   (cbs != NULL)) {
	        if (XmToggleButtonGadgetGetState(tog)) 
			XtSetSensitive(ctxt->customize_btn, True);
		else
			XtSetSensitive(ctxt->customize_btn, False);
		return;
	}

	FocusData = fud;

	if (XmToggleButtonGadgetGetState(tog) /* && m->sub */) {
		XtSetSensitive(ctxt->customize_btn, True);
	}
	else {
		XtSetSensitive(ctxt->customize_btn, False);
	}
	update_detail_text(ctxt->detail, m, 
			m->type == CLUSTER ? m->info.mod : fud->f_mi);
}

void
swToggleCB(
	Widget w, 
	XtPointer cd, 
	XmToggleButtonCallbackStruct* cbs)
{
	addCtxt* ctxt = (addCtxt*)cd;
	Module * mod;
	Widget  form;
	WidgetList	kids;
	swFormUserData	*fud;
	int psz = 0;
   	int on = 0;
	
	XtVaGetValues(w, XmNuserData, &mod, NULL);
	form = XtParent(w);

	/* As a special case for Solaris images, reduce the risk
         * of adding a "risky" (currently defined as "contained 
         * in Core metacluster) by confirming user intent.
         * In any case, this check only needs to be made 
         * when a package/cluster is being selected (ie. cbs->set == 1)
         */
	if (cbs->set && ctxt->isSolarisImage && isRiskyToAdd(ctxt, mod)) {
		if (!AskUser(NULL, ADD_RISKY_MSG, YES_STRING, NO_STRING, NO)) {
			/* unset the toggle */
			XtVaSetValues(w, XmNset, False, NULL);
			return;
		}
	}

	XtVaGetValues(form, XmNuserData, &fud, NULL);
	fud->f_selected = cbs->set;

	mark_fud(fud, cbs->set ? SELECTED : UNSELECTED);

	traverse_fud_list(set_selected_fud, (caddr_t) fud->f_module);

	XtVaGetValues(XtParent(w), XmNchildren, &kids, NULL);
	if (cbs->set) {
		XtSetSensitive(ctxt->customize_btn, True);
		SetFocus(XtParent(w), cd, NULL);
	}
	else {
		XtSetSensitive(ctxt->customize_btn, False);
	}
	update_product_sizes(FocusData->f_toggle, FocusData->f_locale);

	traverse_fud_list(selected_fud_size, (caddr_t)&psz);
	update_size_label(ctxt->totalLabel, catgets(_catd, 8, 83, "     Total (MB): "), psz);
	/* Traverse fud list, looking at selection status. If any
         * modules are selected, the add button is turned on.
         */
	traverse_fud_list(fud_selection_status, (caddr_t)&on);
	XtSetSensitive(XmMessageBoxGetChild(ctxt->button_box, 
				XmDIALOG_OK_BUTTON), on ? True : False);

#ifdef METER
	updateMeter();
#endif
}

void
showSpaceMeterCB(Widget w, XtPointer cd, XtPointer cbs)
{
    addCtxt* ctxt = (addCtxt*)cd;

#ifdef METER
    showMeter(ctxt->add_dialog);
#endif
}

void
customizeCB(
	Widget w, 
	XtPointer cd, 
	XtPointer cbs)
{
	Widget  parent = w;
	extern ViewData * customView;
	ViewData * v;
	TreeData * sw_tree;
	Module * s;
	int len;
	char   *vers, * locale, * m_name, * title;
	Boolean	single_level = False;
	extern Module * duplicateModule(Module *, Module *, Module *, Module *);

	if (FocusData == NULL)
		return;	
	/* 
 	 * If user is customizing a module w/o subs, set single_level
         * to True. This is passed down to CreateList to prevent it
	 * from traversing sibling modules.
	 */
	single_level = FocusData->f_module->sub ? False : True;
	s = single_level ? FocusData->f_module : FocusData->f_module->sub;
        locale = FocusData->f_locale;
        if (s) {
		FocusData->f_copyOfModule = 
			duplicateModule(FocusData->f_module, NULL, NULL, NULL);
	        SetBusyPointer(True);
		/* construct title for list of pkgs in custom dialog */
		m_name = get_mod_name(FocusData->f_module);
   		vers = (char*)get_prodvers(FocusData->f_module);
		len = strlen(m_name) + strlen(vers) + 
			(locale ? strlen(locale) : 0) + 
			strlen(LOCALIZATION_STRING) + 5;
		title = (char *)malloc(len);
		memset(title, 0, len);
		/*
         	 *  See NOTE ON DISPLAYED LABELS FOR PKGS below
         	 */
		if (locale && (s->info.mod->m_locale == NULL))
		    sprintf(title, "%s %s - (%s %s)", 
			m_name, vers, locale, LOCALIZATION_STRING);
		else	
			sprintf(title, "%s %s", m_name, vers);

		if (!customView) {
			customView = customize_sw_dialog(w, (addCtxt *)cd,
					title, s, locale, single_level);
			FocusData->f_swTree = customView->v_swTree;
		}
		else {
			sw_tree = reset_software_view(
				FocusData->f_swTree,
				s, s->info.mod,
				locale, title,
				MODE_INSTALL, True, single_level, TRUE);
			FocusData->f_swTree = sw_tree;
		}
		free(title);
		XtManageChild(customDialogForm);
		XtPopup(customDialog, XtGrabNone);
	        SetBusyPointer(False);
	}  else
		fprintf(stderr, catgets(_catd, 8, 84, "no sub module for customization\n"));
}


/*
 * OK
 */
static void
okCB(
	Widget w, 
	XtPointer cd, 
	XtPointer cbs)
{
	int sts;
	addCtxt* ctxt = (addCtxt*)cd;

	/* XtDestroyWidget(w); */

	SetBusyPointer(True);

	sts = do_add_sw(ctxt);

	if (sts == 0) {
		deleteMeter();
#ifdef SW_INSTALLER
		exit(0);
#else
		/* Update main list */
#ifndef TEST_ADD
		add_software_to_list(NULL);
#endif
		XtPopdown(XtParent(addsoftwaredialog));
#endif

	}
        else if (sts == 10) {
                add_software_to_list(NULL);
                XtPopdown(XtParent(addsoftwaredialog));
		strncpy(errbuf, REBOOT_REQUIRED_MSG, ERRBUF_SIZE);
                display_warning(addsoftwaredialog, errbuf);
             }

	else {
		display_error(addsoftwaredialog, errbuf);
	}

	SetBusyPointer(False);
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
	XtPopdown(((addCtxt *)cd)->shell);
	if (customDialog)
		XtPopdown(customDialog);

#ifdef METER
	deleteMeter();
#endif

#ifdef SW_INSTALLER
		exit(0);
#endif
}


/*
 * Set Media
 */
static	void	
set_mediaCB(
	Widget w, 
	XtPointer cd, 
	XtPointer cbs)
{
	Widget hold;
	Module * module_tree;
	addCtxt* ctxt = (addCtxt*)cd;

	SetBusyPointer(True);

	/* reset pkg_path. If it needs setting, init_add_sw_dialog
	 * will do so.
	 */
	free_mem(ctxt->pkg_path);
	ctxt->pkg_path = NULL;

        /* reset notion of current product */
        ctxt->current_product = NULL;

	/* reset FocusData to NULL so first pass through
         * SetFocus works correctly (bug: 1197791)
	 */
	FocusData = NULL;

	module_tree = selectMediaPath(ctxt->add_dialog,
			&ctxt->sw_image_loc,
			&ctxt->install_path,
			&ctxt->install_device);
	if (module_tree) {
		init_add_sw_dialog(ctxt, module_tree);
		/* Set the new path in the UI label. */
		XtVaSetValues(ctxt->media_value,
		    	RSC_CVT( XmNlabelString, ctxt->install_path ),
			NULL);
	}

	SetBusyPointer(False);
}

static char SolarisProductName[] = "Solaris";

/*
 * get_anchor_module detects if Module returned by
 * sw lib is a Solaris product. If so, the path of
 * the clustertoc path is used as the path of the
 * source software.  Additionally, for Solaris
 * products, the "top level" is considered
 * the "Entire" meta-cluster and its children are what get
 * get displayed. This hides metaclusters from user.
 */

static Module *
get_anchor_module(addCtxt * ctxt, Module * m)
{
	char *p, * ctoc;
	Module * sub;

	ctxt->isSolarisImage = 0;

	if (m == NULL)
		return(NULL);

	ctxt->pkg_path = m->info.prod->p_pkgdir;
	if ((strcmp(SolarisProductName, get_mod_name(m)) == 0)
             && (m->type == PRODUCT)) {
		ctxt->isSolarisImage = 1;

		/* this is the "Entire Distribution" metacluster */
		sub = m->sub;
		/* this is first of its children */
		return(sub->sub ? sub->sub : sub);	
	}
	return(m);
}


/*
 * Return XmForm widget within which a new listing for add 
 * window is to be displayed.
 */

static Widget
create_product_form(addCtxt * c, Module * mtmp, Modinfo * mi)
{
	int		sz;
	swFormUserData	*fud;
	Widget 		size, space, form, toggle;
	char 		* n, *vers;
	char 		swname[1024];
	char 		size_str[64];

	form = XtVaCreateManagedWidget("sw_form",
	    	xmFormWidgetClass, c->rowcol,
	    	NULL);

	n = get_mod_name(mtmp);
     	vers = (char*)get_prodvers(mtmp);
	/*
         *  NOTE ON DISPLAYED LABELS FOR PKGS
         *
         * As a rule, locale in Modinfo 'mi' will determine 
         * how pkg name is displayed. However, when the base
         * Module, mtmp, has a locale value, the name is
         * generally sufficiently descriptive. In this case,
         * there is no need to craft a name for display.
         */
	if (mi->m_locale && (mtmp->info.mod->m_locale == NULL)) {
		StringList *list = mi->m_loc_strlist;
		char *locale = NULL;
		int new_len;
		int locale_cnt = 0;

		while (list) {
		    new_len = strlen(list->string_ptr);
		    if (locale == NULL) {
			locale = (char *)malloc(new_len + 1);
			strcpy(locale, list->string_ptr);
		    } else {
			int old_len = strlen(locale);
			locale = (char *)realloc(locale, old_len + new_len + 2);
			strcat(locale, ", ");
			strcat(locale, list->string_ptr);
		    }
		    list = list->next;
		    locale_cnt++;
		}
		sprintf(swname, "%s %s (%s %s)", 
			n, vers, locale, locale_cnt == 1 ?
				LOCALIZATION_STRING :
				LOCALIZATIONS_STRING);
		free(locale);
	} else
		sprintf(swname, "%s %s", n, vers ? vers : "");

	toggle = XtVaCreateManagedWidget( "tgl",
		xmToggleButtonGadgetClass,
		form,
		RSC_CVT( XmNlabelString, swname),
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL );
	XtVaSetValues(toggle, XmNuserData, mtmp, NULL);
	XtAddCallback(toggle, XmNvalueChangedCallback, 
		swToggleCB, (XtPointer)c);

	fud = fud_create(toggle, mtmp, mi);

	XtVaSetValues(form, XmNuserData, fud, NULL);
	XtAddCallback(form, XmNfocusCallback, SetFocus, (XtPointer)c);

	space =  XtVaCreateManagedWidget("space",
		xmLabelGadgetClass, form,
		RSC_CVT( XmNlabelString, "    "),
		XmNalignment, XmALIGNMENT_END,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, toggle,
		XmNleftOffset, 10,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		NULL);

	sz = 0;
	sprintf(size_str, "    %d", (sz / 1024));

	size =  XtVaCreateManagedWidget("size",
		xmLabelGadgetClass, form,
		RSC_CVT( XmNlabelString, size_str),
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, space,
		XmNleftOffset, 20,
		XmNalignment, XmALIGNMENT_END,
		XmNrecomputeSize, False,
		NULL);
	return ( form );
}

/*
 * Called by walklist to force display of all Modules
 * necessary for localization but are not attached to
 * a specific package.
 *
 * Such a module is recognized by a non-null m_locale
 * field and a NULL m_pkglist field.
 */
static int
create_global_localization(Node * n, caddr_t parent)
{
    Modinfo * mi = (Modinfo *)n->data;
    /* 
     * This is a bit of cleverness to supply the pkg identified
     * with mi with a Module required as arg to create_product_form.
     */
    Module * m = (Module *) malloc(sizeof(Module));
    if (m == NULL)
	fatal(CANT_ALLOC_MSG);
    memset(m, 0, sizeof(Module));
    /*  
     * These are the only field relevant for subsequent processing
     * of this module.
     */
    m->type = PACKAGE;
    m->parent = (Module *)parent;
    m->info.mod = mi;

    if (mi->m_locale && ((mi->m_l10n_pkglist == NULL) || 
			(mi->m_l10n_pkglist[0] == '\0'))) {	
        create_product_form(ctxt, m, mi);
    }
}

static void	
init_add_sw_dialog(addCtxt* ctxt, Module * module_tree)
{
	Module		* mtmp;
	extern Boolean	is_localized(Module *);
	extern L10N * getL10Ns(Module *);
        L10N 		* l10n;
	Product		* p;
	Widget		form;
	static 		int firstEntry = 1;


	/* reset current product */
        ctxt->current_product = NULL;

	ctxt->top_level_module = module_tree;
	ctxt->module_tree = get_anchor_module(ctxt, module_tree);

	if (module_tree == NULL) 
		return;

	if (ctxt->rowcol != NULL)
		XtDestroyWidget(ctxt->rowcol);

	ctxt->rowcol = XtVaCreateWidget( "rowcol",
		xmRowColumnWidgetClass,
		ctxt->scrollwin,
		XmNorientation, XmVERTICAL,
		XmNnumColumns, 1,
		XmNmarginHeight, 1,
		XmNmarginWidth, 1,
		NULL );

	free_fud_list();
	/* 
         * Since fud list has just been free'd, set FocusData
         * to NULL since its contents are no longer meaningful.
         */
	FocusData = NULL;

	/*
         * There may exist some localization pkgs not associated
         * with any particualar 'base' pkg. These have a non-null
         * m_locale field and m_pkglist. These are found via a
         * walklist. create_global_localization does appropriate
         * checks to determine which pkgs fit constraints.
         */
	mtmp = get_parent_product(ctxt->module_tree);
	if (mtmp && mtmp->type == PRODUCT) {
	    apply_to_all_products(ctxt, create_global_localization, NULL);
	}
	/*
         * Traverse list of modules. If modules is valid, display it
         * name in a form via call to create_product_form. If module
         * is localized, build additional forms which display localization
         * pkgs. 
         */
        mtmp = ctxt->module_tree;
	while (mtmp) {
            char * pkg_path;
            /*  
             * Use pkg directory as named in PRODUCT module if relevant.
             * Otherwise, use what is stored in ctxt structure from user
             * input.
             */
            if (mtmp->type == PRODUCT) {	
                pkg_path = mtmp->info.prod->p_pkgdir;
                ctxt->current_product = mtmp;
            } else {
                pkg_path = ctxt->pkg_path ? ctxt->pkg_path : ctxt->install_path;
                ctxt->current_product = NULL;
            }

	     if (isValidModule(ctxt, mtmp)) {
		form = create_product_form(ctxt, mtmp, mtmp->info.mod);

		/* Top entry set to focus */
		if (firstEntry) {
		    SetFocus(form, (XtPointer)ctxt, NULL);
		    firstEntry = 0;
		}

		if (is_localized(mtmp)) {
  		   char * last_loc = "";
		   l10n = getL10Ns(mtmp);
		   while (l10n) {
			Modinfo * mi = l10n->l10n_package;
			if (pkg_exists(pkg_path, mi->m_pkg_dir) &&
			    strcmp(mi->m_locale, last_loc)) {
			  create_product_form(ctxt, mtmp, l10n->l10n_package);
			}
			last_loc = mi->m_locale;
			l10n = l10n->l10n_next;
		    }
		 }
	    }
    	    mtmp = get_next(mtmp); 
	}
	update_size_label(ctxt->totalLabel, catgets(_catd, 8, 85, "     Total (MB): "), 0);

	XtManageChild(ctxt->rowcol);
}


static Widget	
build_add_sw_dialog(Widget parent, enum sw_image_location sw_loc,
				   char * sw_path,
				   char * sw_device,
				   Module * module_tree)
{
	Widget		butt, pane;
	Widget		vbar;
	Widget		title_widget;
	Widget		tmpw;
	Widget		info;
	XmFontList	fl;
	XFontStruct	* f0;
	int		top_level_only;	
	Position	x, y;
	XmString	total_string, meter_string;
	char*		title;

	vbar = NULL;


	/* If the media was changed then the add dialog will need to 
	 * be recreated to reflect a new software tree.  
	 */

	if (ctxt == NULL) 
	{
		ctxt = (addCtxt *) malloc(sizeof(addCtxt));
		memset(ctxt, 0, sizeof(addCtxt));

		ctxt->sw_image_loc = sw_loc;
		ctxt->install_path = sw_path ? strdup(sw_path) : NULL;
		ctxt->install_device = sw_device ? strdup(sw_device) : NULL;

		XtVaGetValues(parent,
			XmNx, &x,
			XmNy, &y,
			NULL);

		/* Create the Add Software dialog. */
#ifdef SW_INSTALLER
		title = catgets(_catd, 8, 515, "Install Software");
#else
		title = catgets(_catd, 8, 86, "Admintool: Add Software");
#endif
		ctxt->shell = XtVaCreatePopupShell( "add_software_shell",
			xmDialogShellWidgetClass, parent,
			XmNshellUnitType, XmPIXELS,
			XmNallowShellResize, True,
			XmNminWidth, 425,
			XmNminHeight, 240, 
			XmNtitle, title,
			XmNx, x+20, XmNy, y+20,	
			NULL);

		ctxt->add_dialog = XtVaCreateWidget( "add_dialog",
	    		xmFormWidgetClass, 
			ctxt->shell,
			XmNunitType, XmPIXELS,
			XmNfractionBase, 100,
			NULL );

		ctxt->media_form = XtVaCreateManagedWidget("media_form",
	    		xmFormWidgetClass, 
			ctxt->add_dialog,
	    		XmNtopAttachment, XmATTACH_FORM,
	    		XmNtopOffset, 10,
	    		XmNleftAttachment, XmATTACH_FORM,
	    		XmNleftOffset, 10,
	    		XmNrightAttachment, XmATTACH_FORM,
	    		XmNrightOffset, 10,
	    		NULL);

		ctxt->set_media_btn = XtVaCreateManagedWidget("set_media_btn",
	    		xmPushButtonWidgetClass, ctxt->media_form,
	    		RSC_CVT( XmNlabelString, catgets(_catd, 8, 87, "Set Source Media...") ),
	    		XmNtopAttachment, XmATTACH_FORM,
	    		XmNleftAttachment, XmATTACH_FORM,
			XmNtraversalOn, False,
	    		NULL);

		ctxt->media_label = XtVaCreateManagedWidget("media_label",
	    		xmLabelGadgetClass, ctxt->media_form,
	    		RSC_CVT( XmNlabelString, catgets(_catd, 8, 88, "Source Media:") ),
	    		XmNtopAttachment, XmATTACH_FORM,
	    		XmNleftAttachment, XmATTACH_WIDGET,
	    		XmNleftWidget, ctxt->set_media_btn,
	    		XmNleftOffset, 10,
	    		XmNbottomAttachment, XmATTACH_FORM,
	    		NULL);

		ctxt->media_value = XtVaCreateManagedWidget("media_value",
	    		xmLabelWidgetClass, ctxt->media_form,
	    		RSC_CVT( XmNlabelString,
				ctxt->install_path ? ctxt->install_path : ""),
	    		XmNleftAttachment, XmATTACH_WIDGET,
	    		XmNleftWidget, ctxt->media_label,
	    		XmNleftOffset, 10,
	    		XmNtopAttachment, XmATTACH_FORM,
	    		XmNbottomAttachment, XmATTACH_FORM,
	    		NULL);

		XtAddCallback(ctxt->set_media_btn, XmNactivateCallback,
			(XtCallbackProc) set_mediaCB,
			(XtPointer) ctxt );

	
		ctxt->separator = XtVaCreateManagedWidget("",
			xmSeparatorGadgetClass, ctxt->add_dialog,
	    		XmNleftAttachment, XmATTACH_FORM,
	    		XmNrightAttachment, XmATTACH_FORM,
		   	XmNtopAttachment, XmATTACH_WIDGET,
		    	XmNtopWidget, ctxt->media_form,
	    		XmNtopOffset, 10,
			NULL);

		ctxt->sw_form = XtVaCreateManagedWidget("sw_form",
	    		xmFormWidgetClass, 
			ctxt->add_dialog,
		   	XmNtopAttachment, XmATTACH_WIDGET,
	    		XmNtopWidget, ctxt->separator,
	    		XmNtopOffset, 10,
	    		XmNleftAttachment, XmATTACH_FORM,
	    		XmNleftOffset, 10,
	    		XmNrightAttachment, XmATTACH_POSITION,
	    		XmNrightPosition, 49,
	    		NULL);

		title_widget =  XtVaCreateManagedWidget("title",
		    xmLabelGadgetClass, ctxt->sw_form,
		    XmNtopAttachment, XmATTACH_FORM,
		    XmNleftAttachment, XmATTACH_FORM,
	    	    RSC_CVT( XmNlabelString, catgets(_catd, 8, 89, "Software")),
		    NULL);

		ctxt->scrollwin = XtVaCreateManagedWidget( "scrollwin",
			xmScrolledWindowWidgetClass,
			ctxt->sw_form,
			XmNscrollingPolicy, XmAUTOMATIC,
		   	XmNtopAttachment, XmATTACH_WIDGET,
		    	XmNtopWidget, title_widget,
		    	XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_FORM,
			XmNheight, 200,
 			XmNwidth, 300,
			NULL );

		XtVaGetValues(ctxt->scrollwin, XmNverticalScrollBar, &vbar, NULL);
		if (vbar != NULL)
			XtVaSetValues(vbar, XmNtraversalOn, False, NULL);

		total_string = XmStringCreateLocalized(catgets(_catd, 8, 90, "     Total (MB): 0"));
		ctxt->totalLabel = 
			XtVaCreateManagedWidget("productListTotalLabel",
    			xmLabelGadgetClass, ctxt->sw_form,
    			XmNlabelString, total_string,
			XmNalignment, XmALIGNMENT_END,
			XmNrightAttachment, XmATTACH_FORM,
			XmNrecomputeSize, False,
    			NULL);
		XmStringFree(total_string);

		meter_string = 
			XmStringCreateLocalized(
				catgets(_catd, 8, 783, "Space Meter..."));

                ctxt->meter_btn = XtVaCreateManagedWidget("spaceMeterButton",
			xmPushButtonWidgetClass, ctxt->sw_form,
		        XmNlabelString, meter_string,
			XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNrightWidget, ctxt->totalLabel,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, ctxt->totalLabel,
#ifndef METER
			XmNsensitive, False,
#endif
			NULL);
		XmStringFree(meter_string);
                XtAddCallback(ctxt->meter_btn, XmNactivateCallback, 
			showSpaceMeterCB, ctxt);

		XtVaSetValues(ctxt->scrollwin,
                        XmNbottomAttachment, XmATTACH_WIDGET,
                        XmNbottomWidget,  ctxt->totalLabel,
                        NULL);

		ctxt->customize_btn = XtVaCreateManagedWidget("customize_btn",
			xmPushButtonWidgetClass, ctxt->sw_form,
			RSC_CVT( XmNlabelString, catgets(_catd, 8, 91, "Customize...")),
			XmNleftAttachment, XmATTACH_FORM,
	    		XmNbottomAttachment, XmATTACH_FORM,
			XmNtraversalOn, False,
			XmNsensitive, False,
			NULL);	 

		XtVaSetValues(ctxt->totalLabel,
                        XmNbottomAttachment, XmATTACH_WIDGET,
                        XmNbottomWidget,  ctxt->customize_btn,
                        NULL);
	

		ctxt->info_form = XtVaCreateManagedWidget("info_form",
	    		xmFormWidgetClass, 
			ctxt->add_dialog,
		   	XmNtopAttachment, XmATTACH_WIDGET,
	    		XmNtopWidget, ctxt->separator,
	    		XmNtopOffset, 10,
	    		XmNrightAttachment, XmATTACH_FORM,
	    		XmNrightOffset, 10,
	    		XmNleftAttachment, XmATTACH_POSITION,
	    		XmNleftPosition, 51,
	    		NULL);

		ctxt->detail = create_detail(ctxt->info_form,
				catgets(_catd, 8, 92, "Description"), 5, catgets(_catd, 8, 93, "packages"));


		XtVaSetValues(XtParent(ctxt->detail),
	    		XmNbottomAttachment, XmATTACH_WIDGET,
	    		XmNbottomWidget, ctxt->info_form,
	    		XmNbottomOffset, 10,
	    		NULL);

		/* Enabled only for SunPro */
		ctxt->license_btn = XtVaCreateWidget("license_btn",
			xmPushButtonWidgetClass, ctxt->info_form,
			RSC_CVT( XmNlabelString, catgets(_catd, 8, 94, "License...")),
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, ctxt->customize_btn,
			XmNleftOffset, 10,
	    		XmNbottomAttachment, XmATTACH_FORM,
			XmNtraversalOn, False,
			NULL);	 

		XtAddCallback(ctxt->customize_btn, XmNactivateCallback,
			customizeCB, (XtPointer) ctxt);

		ctxt->button_box = XmCreateMessageBox(ctxt->add_dialog,
			"ButtonBox", NULL, 0);

		XtUnmanageChild(XmMessageBoxGetChild(
			ctxt->button_box, XmDIALOG_MESSAGE_LABEL));

		XtVaSetValues(ctxt->button_box,
	    		XmNbottomAttachment, XmATTACH_FORM,
			XmNbottomOffset, 1,
	    		XmNleftAttachment, XmATTACH_FORM,
			XmNleftOffset, 1,
	    		XmNrightAttachment, XmATTACH_FORM,
			XmNrightOffset, 1,
	    		RSC_CVT( XmNokLabelString, catgets(_catd, 8, 95, "Add") ),
	    		RSC_CVT( XmNcancelLabelString, catgets(_catd, 8, 96, "Cancel") ),
	    		RSC_CVT( XmNhelpLabelString, catgets(_catd, 8, 97, "Help") ),
	    		NULL);

		tmpw = XmMessageBoxGetChild(ctxt->button_box,
				XmDIALOG_OK_BUTTON);
		XtVaSetValues(tmpw, XmNtraversalOn, False, 
			            XmNsensitive, False,
				NULL);

		tmpw = XmMessageBoxGetChild(ctxt->button_box,
				XmDIALOG_CANCEL_BUTTON);
		XtVaSetValues(tmpw, XmNtraversalOn, False, 
				NULL);

		tmpw = XmMessageBoxGetChild(ctxt->button_box,
				XmDIALOG_HELP_BUTTON);
		XtVaSetValues(tmpw, XmNtraversalOn, False,
				NULL);

		XtAddCallback(ctxt->button_box, XmNcancelCallback,
			(XtCallbackProc) cancelCB,
			(XtPointer) ctxt );

		XtAddCallback(ctxt->button_box, XmNokCallback,
			(XtCallbackProc) okCB,
			(XtPointer) ctxt );

		XtAddCallback(ctxt->button_box, XmNhelpCallback,
			(XtCallbackProc) helpCB,
			"software_window.r.hlp" );

		XtVaSetValues(ctxt->sw_form,
	    		XmNbottomAttachment, XmATTACH_WIDGET,
	    		XmNbottomWidget, ctxt->button_box,
	    		NULL);

		XtVaSetValues(ctxt->info_form,
	    		XmNbottomAttachment, XmATTACH_WIDGET,
	    		XmNbottomWidget, ctxt->button_box,
	    		NULL);

		XtVaSetValues(ctxt->add_dialog,
			XmNuserData, (XtPointer)ctxt,
			NULL);

		XtManageChild(ctxt->totalLabel);
		XtManageChild(ctxt->sw_form);
		XtManageChild(ctxt->button_box);

	}

	ctxt->rowcol = NULL;

	if (module_tree) {
		XtVaSetValues(ctxt->media_value,
			RSC_CVT( XmNlabelString, ctxt->install_path ),
			NULL);
		init_add_sw_dialog(ctxt,module_tree);
	}
	return (ctxt->add_dialog);
}

static enum sw_image_location image_loc = cd_with_volmgmt;
static char * install_path = NULL;
static char * install_device = NULL;

void	
show_addsoftwaredialog(Widget parent, sysMgrMainCtxt * mgrctxt, char* path)
{
	addCtxt*	ctxt;
	Widget		type_widget;

	image_loc = cd_with_volmgmt;
	install_path = path;
	install_device = NULL;


	if (addsoftwaredialog) {
		XtVaGetValues(addsoftwaredialog, XmNuserData, &ctxt, NULL);
		/* module_tree may be NULL if user cancelled out of
                 * source media dialog w/o selecting a legal media
		 */
		if (ctxt->module_tree) 
			reset_product_selection(ctxt);

	        XtManageChild(addsoftwaredialog);
		XtPopup(XtParent(addsoftwaredialog), XtGrabNone);
		return;
	}

	SetBusyPointer(True);

	if (addsoftwaredialog == NULL) {
		Module * module_tree;

#ifdef SW_INSTALLER
		/* Use path passed in on command line */
		addsoftwaredialog = build_add_sw_dialog(parent,
			image_loc, path, install_device, NULL);

		/* Hide all media-setting widgets */
		XtVaGetValues(addsoftwaredialog, XmNuserData, &ctxt, NULL);
		XtUnmanageChild(ctxt->media_form);
		XtUnmanageChild(ctxt->separator);

		/* Popup add software window */
		XtManageChild(addsoftwaredialog);
		ForceDialog(addsoftwaredialog);
		SetBusyPointer(True);

		/* Look for software */
		module_tree = get_source_sw(path);
		if (module_tree) {
			init_add_sw_dialog(ctxt, module_tree);
		}
		else {
			display_error(addsoftwaredialog, "No software found.");
			exit(1);
		}
#else
		if (install_path == NULL) {
			module_tree = selectMediaPath(parent,
				&image_loc, &install_path, &install_device);
		}
		else {
			module_tree = get_source_sw(install_path);
		}

		if (module_tree == NULL) {
		    SetBusyPointer(False);
		    return;
		}

		addsoftwaredialog = build_add_sw_dialog(parent,
			image_loc, install_path, install_device, module_tree);
#endif

	}

	if (mgrctxt) {
		mgrctxt->currDialog = addsoftwaredialog;
	}
	XtManageChild(addsoftwaredialog);
	XtPopup(XtParent(addsoftwaredialog), XtGrabNone);
	SetBusyPointer(False);
}

static int script_fd;
static int show_copyrights;
static int non_interactive;
static int  admin_file_list_len;
static char* admin_file_list[100];

static void
add_pkg_to_list(Modinfo* mod)
{
	SysmanSWArg     pkg;
	PkgAdminProps	pkgprops;

	memset((void*)&pkg, 0, sizeof(SysmanSWArg));

	if (mod->m_instdir != NULL) {
		/* user has modified basedir */
		get_admin_file_values(&pkgprops);
		pkgprops.basedir = mod->m_instdir;
		pkg.admin = write_admin_file(&pkgprops);
		admin_file_list[admin_file_list_len++] = strdup(pkg.admin);
	}
	else {
		pkg.admin = admin_file_list[0];
	}

	pkg.show_copyrights = show_copyrights;
	pkg.non_interactive = non_interactive;

        /* 
         * If current_product was set in apply_to_all_products,
         * then use p_pkgdir.Otherwise, use 'old' strategy.
         * FIX ME FIX ME 'old' strategy may in fact no longer be
         * be necessary with more general solution now used. However,
         * care must be taken not to screw up special treatment of
         * Solaris image. (rkg 10/11/95)
         */
        if (ctxt->current_product)
	    pkg.device = ctxt->current_product->info.prod->p_pkgdir;
        else
	    pkg.device = ctxt->pkg_path ? ctxt->pkg_path : 
				ctxt->install_path;
	pkg.num_pkgs = 1;
	pkg.pkg_names = mod->m_pkg_dir ? (const char **)&(mod->m_pkg_dir) :
					  (const char **)&(mod->m_pkgid);

	sysman_sw_add_cmd_to_script(script_fd, &pkg);
}

static int
list_pkgs(Node * node, caddr_t arg)
{
	Modinfo * mod = (Modinfo *) node->data;

	/* Of course, only do the add if module is selected. */
	if (mod->m_status != SELECTED)
	    return;

	add_pkg_to_list(mod);

        /* Mark with -1 so that it is not double-pkgadded 
         * This will get reset to SELECTED by reset_selection_status.
         */
        mod->m_status = -1;
}

int 
do_add_sw(addCtxt * ctxt)
{
	int		i, retval;
	PkgAdminProps	pkgprops;

        sysman_sw_do_gui(B_TRUE, DisplayString(Gdisplay));

	/* setup default admin file */
	get_admin_file_values(&pkgprops);
	admin_file_list[0] = strdup(write_admin_file(&pkgprops));
	admin_file_list_len = 1;
	show_copyrights = (strcmp(pkgprops.showcr, "yes") == 0) ? 1 : 0;
	non_interactive = (strcmp(pkgprops.interactive, "no") == 0) ? 1 : 0;

	/* 
         * We need to employ two strategies for traversing
         * tree to determine which pkgs should be added.
         *
         * If no .clustertoc file exists, that is, the install 
         * media is simply a directory containing pkg directories,
	 * the root of module tree is a PACKAGE and we traverse the 
         * tree using get_select_modules. In this case, there is
         * no ordered list associated with the pkgs.
         *
         * If a .clustertoc file exists, then root of module tree
         * is either a PRODUCT or a NULLPRODUCT and hence has a
         * p_packages field containing an ordered list of pkgs
         * which can be traversed with walklist.
	 */
	if (ctxt->top_level_module->type == PACKAGE) {
		int num_mods = 0;
		struct modinfo  ** mods = (struct modinfo **)NULL;

     		num_mods = get_selected_modules(ctxt->module_tree, 
                                       (struct modinfo  ***)&mods);

		script_fd = sysman_sw_start_script();
		for (i=0; i < num_mods; i++) {
		    add_pkg_to_list(mods[i]);
		}
		sysman_sw_finish_script(script_fd);

		retval = sysman_add_sw_by_script(errbuf, ERRBUF_SIZE);

		reset_selected_modules(ctxt->module_tree);
		free_mem(mods);
	}
	else {
	    Module*	mod;

	    /* Get PRODUCT/NULLPRODUCT module */
	    mod = get_parent_product(ctxt->top_level_module);

	    /* 
             * This should never happen. If it does, we got a really 
             * goofy module tree from sw lib.
             * It should be fatal() but that would mean a string for
             * L10N folks and that ain't gonna happen right now. (6/21/95)
             * FIX ME, FIX ME, FIX ME.
             */
            if (mod == NULL) {
		return (1);
	    }

            /* Add pkgs */
	    script_fd = sysman_sw_start_script();
	    apply_to_all_products(ctxt, list_pkgs, NULL);
	    sysman_sw_finish_script(script_fd);

	    retval = sysman_add_sw_by_script(errbuf, ERRBUF_SIZE);

            /* Reset pkgs whose status was set to -1, back to SELECTED */
            apply_to_all_products(ctxt, reset_selected_pkgs, NULL);
	}

	for (i=0; i<admin_file_list_len; i++) {
		unlink(admin_file_list[i]);
		free(admin_file_list[i]);
	}

	return(retval);
}
