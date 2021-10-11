#ifndef lint
#pragma ident "@(#)pfgsoftware.c 1.71 96/09/04 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	pfgsoftware.c
 * Group:	installtool
 * Description:
 */

#include <stdio.h>
#include <Xm/ArrowB.h>

#include "pf.h"
#include "pfg.h"
#include "pfgsoftware.h"
#include "pfgSoftware_ui.h"

static void initializeLegend(int buttonCount);
static void SetInfo(Widget, XtPointer, XtPointer);

static Module *MetaAll;
static SelectionList SelectList[800];
static int Count = 0;
static SelectPixmaps selectpixmaps;
/* indicates whether the package screen has been created */
static int Created = False;

static SelectionList LegendButtons[4];

static WidgetList widget_list;

/* length of format text for space information */
#define	SPACE_FORMAT 8
#define	SPACE_INDENT_LEN 4

static Widget software_dialog;
static Widget software_dialog_parent;

/* ARGSUSED */
Widget
pfgCreateSoftware(Widget parent)
{
	Widget help;
	Module *module;

	software_dialog_parent = parent;

	if (Created == True) {
		XtManageChild(software_dialog);
		return (NULL);
	}

	software_dialog = tu_software_dialog_widget("software_dialog",
		pfgTopLevel, &widget_list);

	XmAddWMProtocolCallback(pfgShell(software_dialog), pfgWMDeleteAtom,
		(XtCallbackProc) pfgExit, NULL);

	XtVaSetValues(pfgShell(software_dialog),
		XmNtitle, TITLE_CUSTOM,
		XmNdeleteResponse, XmDO_NOTHING,
		NULL);

	help = pfgGetNamedWidget(widget_list, "softwareInfoForm");

	module = get_sub(get_current_product());

	if ((mod_status(module)) == ERR_INVALIDTYPE) {
		(void) printf("invalid module past to mod_status\n");
	}

	MetaAll = module;

	CreateList(pfgGetNamedWidget(widget_list, "softwareClustersRowColumn"),
		get_sub(module), 1, help);

	pfgSetWidgetString(widget_list, "clusterLabel", PFG_SW_PACKAGE);
	pfgSetWidgetString(widget_list, "sizeLabel", PFG_SW_SIZE);
	pfgSetWidgetString(widget_list, "softwareInfoLabel", PFG_SW_PKGINFO);
	pfgSetWidgetString(widget_list, "totalSizeLabel", PFG_TOTAL);
	pfgSetWidgetString(widget_list, "expandLegendLabel", PFG_SW_EXPAND);
	pfgSetWidgetString(widget_list, "contractLegendLabel", PFG_SW_CONTRACT);
	pfgSetWidgetString(widget_list, "softwareDependenciesLabel",
		PFG_SW_UNRESOLV);

	pfgSetWidgetString(widget_list, "okButton", PFG_OKAY);
	pfgSetWidgetString(widget_list, "helpButton", PFG_HELP);

	LegendButtons[0].orig_status = REQUIRED;
	LegendButtons[0].select =
		pfgGetNamedWidget(widget_list, "requiredLegendButton");
	pfgSetWidgetString(widget_list, "requiredLegendLabel", PFG_SW_REQD);

	LegendButtons[1].orig_status = PARTIALLY_SELECTED;
	LegendButtons[1].select =
		pfgGetNamedWidget(widget_list, "partialLegendButton");
	pfgSetWidgetString(widget_list, "partialLegendLabel", PFG_SW_PARTIAL);

	LegendButtons[2].orig_status = SELECTED;
	LegendButtons[2].select =
		pfgGetNamedWidget(widget_list, "selectedLegendButton");
	pfgSetWidgetString(widget_list, "selectedLegendLabel", PFG_SW_SELECT);

	LegendButtons[3].orig_status = UNSELECTED;
	LegendButtons[3].select =
		pfgGetNamedWidget(widget_list, "unselectedLegendButton");
	pfgSetWidgetString(widget_list, "unselectedLegendLabel",
		PFG_SW_UNSELECT);

	Created = True;

	SetSelection(SelectList[0].select, NULL, NULL);

	initializeLegend(4);

	XtManageChild(software_dialog);

	XmProcessTraversal(pfgGetNamedWidget(widget_list, "okButton"),
		XmTRAVERSE_CURRENT);

	return (NULL);
}

/* ARGSUSED */
void
CreateList(Widget parent_rc, Module * module, int level, Widget info)
{
	Widget child_rc, expand;

	do {
		if (module->type == CLUSTER) {
			CreateEntry(parent_rc, info, level, module, &expand);
			child_rc = XtVaCreateWidget("child_rc",
				xmRowColumnWidgetClass, parent_rc,
				XmNnavigationType, XmNONE,
				XmNmarginWidth, (Dimension) 0,
				NULL);
			XtAddCallback(expand, XmNactivateCallback,
				ExpandCluster, child_rc);
			/* create child list of cluster */
			CreateList(child_rc, get_sub(module), level + 1, info);
		} else if (module->type == PACKAGE) {
			CreateEntry(parent_rc, info, level, module, NULL);
		}
	} while ((module = get_next(module)));
}


void
CreateEntry(Widget parent, Widget info, int level, Module * module,
	Widget * expand)
{
	Widget select, title, entryForm, size;
	XmString titleString, sizeString;
	int totalMB;
	char tmp[32];
	ModStatus selectstatus;

	entryForm = XtVaCreateWidget("entryForm",
		xmFormWidgetClass, parent,
		NULL);

	/* add entry callback to display help when form is entered */
	XtVaSetValues(entryForm,
		XmNuserData, module,
		NULL);

	XtAddCallback(entryForm, XmNfocusCallback, SetInfo, info);

	if (expand != NULL) {
		*expand = XtVaCreateManagedWidget("Expand",
			xmArrowButtonWidgetClass, entryForm,
			XmNwidth, (Dimension) 20,
			XmNheight, (Dimension) 20,
			XmNshadowThickness, (Dimension) 0,
			XmNtopAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_FORM,
			XmNleftAttachment, XmATTACH_FORM,
			XmNarrowDirection, XmARROW_RIGHT,
			NULL);

	}
	(void) XtVaCreateWidget("toggle",
		xmToggleButtonWidgetClass, entryForm,
		NULL);

	select = XtVaCreateManagedWidget(module->info.mod->m_pkgid,
		xmPushButtonWidgetClass, entryForm,
		XmNtopAttachment, XmATTACH_FORM,
		/* XmNbottomAttachment, XmATTACH_FORM, */
		XmNleftAttachment, XmATTACH_FORM,
		XmNleftOffset, (SELECT_OFFSET + INDENT * level),
		XmNresizable, False,
		XmNuserData, (XtPointer) module,
		XmNlabelType, XmPIXMAP,
		XmNwidth, (Dimension) 20,
		XmNheight, (Dimension) 20,
		XmNrecomputeSize, False,
		NULL);

	selectstatus = module->info.mod->m_status;

	SelectList[Count].select = select;
	SelectList[Count].level = level;
	SelectList[Count].orig_status = selectstatus;

	if (selectstatus != REQUIRED) {
		XtAddCallback(select, XmNactivateCallback, SetSelection, NULL);
	}
	titleString = XmStringCreateLocalized(module->info.mod->m_name);

	title = XtVaCreateManagedWidget("title",
		xmPushButtonWidgetClass, entryForm,
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

	if (expand == NULL) {
		totalMB = getModuleSize(module, UNSELECTED);
	} else {
		totalMB = getModuleSize(module, SELECTED);
	}
	if (totalMB != 0 && totalMB / 1000 < (int) 1) {
		(void) sprintf(tmp, "<1");
	} else {
		(void) sprintf(tmp, "%d", totalMB / 1000);
	}

	sizeString = XmStringCreateLocalized(tmp);
	size = XtVaCreateManagedWidget("size",
		xmLabelWidgetClass, entryForm,
		XmNlabelString, sizeString,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, title,
		XmNleftOffset, INDENT,
		XmNalignment, XmALIGNMENT_END,
		XmNrecomputeSize, False,
		NULL);
	XmStringFree(sizeString);

	/* if module type is cluster then set Select size to size */
	if (expand == NULL) {
		SelectList[Count].size = NULL;
	} else {
		SelectList[Count].size = size;
	}

	Count++;
	XtVaSetValues(entryForm,
		XmNinitialFocus, title,
		NULL);

	XtManageChild(entryForm);
}


/* ARGSUSED */
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


/* ARGSUSED */
void
SetInfo(Widget form, XtPointer info, XtPointer client_data)
{
	Module *mod;
	Widget sizeRC;
	FSspace **space;
	static Widget spaceWidget[MAX_SPACE_FS];
	int i, sizeLength = 5;
	static int maxSpace = 0;
	static int first = True;
	char *tmp;
	XmString xms;

	XtVaGetValues(form,
		XmNuserData, &mod,
		NULL);

	if (first) {
		pfgSetWidgetString(widget_list, "prodLabel", PFG_SW_PRODUCT);
		pfgSetWidgetString(widget_list, "abbrevLabel", PFG_SW_ABBREV);
		pfgSetWidgetString(widget_list, "vendorLabel", PFG_SW_VENDOR);
		pfgSetWidgetString(widget_list, "descriptLabel", PFG_SW_DESC);
		pfgSetWidgetString(widget_list, "estLabel", PFG_SW_ESTIMATE);

		sizeRC = pfgGetNamedWidget(widget_list, "sizeRowColumn");

		/*
		 * create entries for estimated size
		 */
		space = calc_cluster_space(mod, UNSELECTED);
		for (i = 0; space[i] != NULL && i < MAX_SPACE_FS; i++) {
			spaceWidget[i] = XtVaCreateManagedWidget("Space",
				xmLabelWidgetClass, sizeRC, NULL);
			maxSpace++;
		}

		first = False;
	}
	pfgSetWidgetString(widget_list, "prodValue", mod->info.mod->m_name ?
		mod->info.mod->m_name : "");

	pfgSetWidgetString(widget_list, "abbrevValue", mod->info.mod->m_pkgid ?
		mod->info.mod->m_pkgid : "");
	pfgSetWidgetString(widget_list, "vendorValue", mod->info.mod->m_vendor ?
		mod->info.mod->m_vendor : "");
	pfgSetWidgetString(widget_list, "descriptValue", mod->info.mod->m_desc ?
		mod->info.mod->m_desc : "");


	space = calc_cluster_space(mod, UNSELECTED);
	for (i = 0; space[i] != NULL && i < maxSpace; i++) {
		tmp = (char *) xmalloc((sizeLength + SPACE_FORMAT +
			strlen(space[i]->fsp_mntpnt ?
			space[i]->fsp_mntpnt : "") + 1));
		(void) sprintf(tmp, "%*ld MB in %s", sizeLength,
			space[i]->fsp_reqd_contents_space / 1000,
			space[i]->fsp_mntpnt ? space[i]->fsp_mntpnt : "");
		xms = XmStringCreateLocalized(tmp);
		XtVaSetValues(spaceWidget[i],
			XmNlabelString, xms,
			NULL);
		XmStringFree(xms);
	}
}


/* ARGSUSED */
void
SetSelection(Widget select, XtPointer clientD, XtPointer *call_data)
{
	static int first = True;
	Module *module = NULL;
	char tmp[100];
	int size;

	if (select != NULL)
		XtVaGetValues(select,
			XmNuserData, &module,
			NULL);

	/*
	 * the first time SetSelection is called don't toggle the state of
	 * module, just create selection pixmaps and display them.
	 */
	if (first == True) {
		CreateSelectPixmap(select, &selectpixmaps.unselect,
			&selectpixmaps.select, &selectpixmaps.partial,
			&selectpixmaps.required);
		first = False;
	} else {
		if (module != NULL) {
			toggle_module(module);	/* toggle module status */
			if (pfgState & AppState_UPGRADE) {
				update_action(module);
			}
		}
		mod_status(MetaAll); /* reset status of all clusters */
	}

	/*
	 * get size of all packages/clusters selected and display in total
	 * label
	 */
	size = DiskGetContentMinimum();
	(void) sprintf(tmp, "%*d", SIZE_LENGTH, size);
	pfgSetWidgetString(widget_list, "totalSizeValue", tmp);

	pfgupdate_software();
	showDependencies();
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

	XtVaGetValues(XtParent(button),
		XmNbackground, &unselectcolor,
		XmNforeground, &foreground,
		NULL);

	values.foreground =	/* (unsigned long) unselectcolor; */
	BlackPixelOfScreen(XtScreenOfObject(button));
	values.background = unselectcolor;
	values.fill_style = FillStippled;

	select_gc = XCreateGC(XtDisplay(XtParent(button)),
		XtWindow(XtParent(software_dialog)),
		(GCForeground | GCBackground), &values);

	partial_gc = XCreateGC(XtDisplay(button),
		XtWindow(XtParent(software_dialog)),
		(GCForeground | GCBackground), &values);

	values.foreground = unselectcolor;
	values.background = unselectcolor;
	values.line_width = 2;
	required_gc = XCreateGC(XtDisplay(button),
		XtWindow(XtParent(software_dialog)),
		(GCLineWidth | GCForeground | GCBackground), &values);

	unselect_gc = XCreateGC(XtDisplay(button),
		XtWindow(XtParent(software_dialog)),
		(GCForeground | GCBackground), &values);

	XGetWindowAttributes(XtDisplay(button),
		XtWindow(XtParent(software_dialog)),
		&windowattr);

	*partialPixmap = XCreatePixmap(XtDisplay(button),
		XtWindow(XtParent(software_dialog)),
		(unsigned int) width,
		(unsigned int) height,
		windowattr.depth);

	*selectPixmap = XCreatePixmap(XtDisplay(button),
		XtWindow(XtParent(software_dialog)),
		width, height, windowattr.depth);

	*unselectPixmap = XCreatePixmap(XtDisplay(button),
		XtWindow(XtParent(software_dialog)),
		width, height, windowattr.depth);

	*requiredPixmap = XCreatePixmap(XtDisplay(button),
		XtWindow(XtParent(software_dialog)),
		width, height, windowattr.depth);

	/* Draw criss-cross lines in *partial_gc */

	points[0].x = 0;
	points[0].y = 0;
	points[1].x = width;
	points[1].y = height;
	points[2].x = 0;
	points[2].y = height;
	points[3].x = width;
	points[3].y = 0;
	XFillRectangle(XtDisplay(button), *requiredPixmap,
		select_gc, 0, 0, width, height);
	XDrawLines(XtDisplay(button), *requiredPixmap,
		required_gc, points, 4, CoordModeOrigin);

	/* Draw filled in triangle in *partial_gc */

	points[0].x = width;
	points[0].y = 0;
	points[1].x = width;
	points[1].y = height;
	points[2].x = 0;
	points[2].y = height;
	XFillRectangle(XtDisplay(button), *partialPixmap,
		unselect_gc, 0, 0, width, height);
	XFillPolygon(XtDisplay(button), *partialPixmap,
		partial_gc, points, 3, Convex, CoordModeOrigin);

	XFillRectangle(XtDisplay(button), *selectPixmap,
		select_gc, 0, 0, width, height);

	XFillRectangle(XtDisplay(button), *unselectPixmap,
		unselect_gc, 0, 0, width, height);
}


void
pfgupdate_software(void)
{
	int i;
	Module *module;
	int total;
	char tmp[10];
	XmString sizeString;
	ModStatus selectStatus;

	for (i = 0; i < Count; i++) {
		XtVaGetValues(SelectList[i].select,
			XmNuserData, &module,
			NULL);
		selectStatus = module->info.mod->m_status;
		update_selection(SelectList[i].select, selectStatus);
		/* set cluster size after selection */
		if (SelectList[i].size != NULL) {
			total = getModuleSize(module, SELECTED);
			if (total != 0 && total / 1000 < (int) 1) {
				(void) sprintf(tmp, "<1");
			} else {
				(void) sprintf(tmp, "%d", total / 1000);
			}
			sizeString = XmStringCreateLocalized(tmp);
			XtVaSetValues(SelectList[i].size,
				XmNlabelString, sizeString,
				XmNalignment, XmALIGNMENT_END,
				NULL);
			XmStringFree(sizeString);
			XmUpdateDisplay(SelectList[i].select);
		}
	}
}

void
update_selection(Widget select, ModStatus selectStatus)
{
	/*
	 * We've kludged this to make the applied pixmap logically visible
	 * to QA Partner. XmNalignment and XmNstringDirection are otherwise
	 * irrelevant.
	 */


	if (selectStatus == UNSELECTED) {
		XtVaSetValues(select,
			XmNlabelPixmap, XmUNSPECIFIED_PIXMAP,
			XmNalignment, XmALIGNMENT_BEGINNING,
			NULL);
	} else if (selectStatus == SELECTED) {
		XtVaSetValues(select,
			XmNlabelPixmap, selectpixmaps.select,
			XmNalignment, XmALIGNMENT_CENTER,
			XmNstringDirection, XmSTRING_DIRECTION_R_TO_L,
			NULL);
	} else if (selectStatus == PARTIALLY_SELECTED) {
		XtVaSetValues(select,
			XmNlabelPixmap, selectpixmaps.partial,
			XmNstringDirection, XmSTRING_DIRECTION_L_TO_R,
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
	}
}


void
showDependencies(void)
{
	Depend *dependents = NULL;
	Node *node, *node_dep;
	Module *module;
	char prev_pkgid[32];
	XmString *tmp = NULL;
	List *package_list;
	int i = 0, j;
	char *indent_name;

	if (!check_sw_depends()) {
		XmListDeleteAllItems(pfgGetNamedWidget(widget_list,
			"softwareDependenciesScrolledList"));
		return;
	}
	dependents = get_depend_pkgs();
	if (dependents == NULL) {
		return;
	}
	module = get_current_product();

	package_list = module->info.prod->p_packages;
	if (package_list == NULL) {
		return;
	}

	prev_pkgid[0] = NULL;
	while (dependents != NULL) {
		/*
		 * Show the package which has dependencies.
		 */
		if (strcmp(prev_pkgid, dependents->d_pkgid)) {
			node = findnode((List *) package_list,
				dependents->d_pkgid);
			tmp = (XmString *) xrealloc(tmp,
				((i + 1) * sizeof (XmString)));
			tmp[i] = XmStringCreateLocalized(((Modinfo *)
				node->data)->m_name);
			++i;
		}

		/*
		 * Show this packages' dependencies.
		 */
		node_dep = findnode((List *) package_list,
			dependents->d_pkgidb);
		indent_name = (char *) malloc(SPACE_INDENT_LEN +
			strlen(((Modinfo *) node_dep->data)->m_name) + 1);
		(void) sprintf(indent_name, "%*s%s",
			SPACE_INDENT_LEN, " ",
			((Modinfo *) node_dep->data)->m_name);
		tmp = (XmString *) xrealloc(tmp,
			((i + 1) * sizeof (XmString)));
		tmp[i] = XmStringCreateLocalized(indent_name);
		++i;
		free(indent_name);
		(void) strcpy(prev_pkgid, dependents->d_pkgid);
		dependents = dependents->d_next;
	}

	XmListDeleteAllItems(pfgGetNamedWidget(widget_list,
		"softwareDependenciesScrolledList"));
	XmListAddItems(
		pfgGetNamedWidget(widget_list,
			"softwareDependenciesScrolledList"),
		tmp, i, 1);

	for (j = 0; j < i; j++) {
		XmStringFree(tmp[j]);
	}
}


/* ARGSUSED */
void
softwareOkCB(Widget done, XtPointer clientD, XtPointer callD)
{
	pfSw_t *cluster_curr;
	pfSw_t *package_curr;
	pfSw_t **cluster_head = pfGetClusterListPtr();
	pfSw_t **package_head = pfGetPackageListPtr();
	Module *module;
	int i = 0, level;

	/* check for software dependencies */
	if (check_sw_depends()) {
		/*
		 * Ask user if they wish to continue, or resolve dependencies
		 */
		if (pfgQuery((Widget)software_dialog, pfQDEPENDS) == False) {
			return; /* resolve dependencies */
		}
	}

	pfNullPackClusterLists();

	while (i < Count) {
		XtVaGetValues(SelectList[i].select,
			XmNuserData, &module,
			NULL);
		if (module->info.mod->m_status != SelectList[i].orig_status) {
			if (module->type == CLUSTER) {
				if ((module->info.mod->m_status ==
					SELECTED) ||
				(module->info.mod->m_status ==
					UNSELECTED &&
					SelectList[i].orig_status ==
					SELECTED)) {
					if (pfSW_add(cluster_head,
						&cluster_curr, module) !=
					SUCCESS) {
						(void) printf("malloc error in "
						"pfSW_add\n");
						pfgCleanExit(
							EXIT_INSTALL_FAILURE,
							(void *) NULL);
					}
					level = SelectList[i].level;
					++i;
					while (i < Count &&
					SelectList[i + 1].level > level) {
						++i;
					}
				}
			}
			if (module->type == PACKAGE) {
				if (pfSW_add(package_head, &package_curr,
					module) != SUCCESS) {
					(void) printf(
						"malloc error in pfSW_add\n");
					pfgCleanExit(EXIT_INSTALL_FAILURE,
						(void *) NULL);
				}
				++i;
			} else {
				++i;
			}
		} else {
			++i;
		}
	}

	if (!(pfgState & AppState_UPGRADE)) {
		pfgUpdateMetaSize();
	}
	pfgUnbusy(pfgShell(software_dialog_parent));

	XtUnmanageChild(software_dialog);
}

void
pfgResetPackages(void)
{
	Module *module;
	int i;

	if (Created == True) {
		for (i = 0; i < Count; i++) {
			module = NULL;
			XtVaGetValues(SelectList[i].select,
				XmNuserData, &module,
				NULL);
			if (module == NULL) {
				(void) fprintf(stderr, "pfgUpdatePackages: "
					"unable to retrieve module pointer\n");
				return;
			}
			write_debug(GUI_DEBUG_L1,
				"Setting module status for %s",
				module->info.mod->m_name);
			SelectList[i].orig_status = module->info.mod->m_status;
		}
		SetSelection(NULL, NULL, NULL);
		pfNullPackClusterLists();
	}
}

void
initializeLegend(int buttonCount)
{
	int i;

	for (i = 0; i < buttonCount; i++) {
		update_selection(LegendButtons[i].select,
			LegendButtons[i].orig_status);
	}
}
utton(widget_array[WI_MESSAGEBOX], "helpButton", NULL, 0);

  /***************** button3 : XmPushButton *****************/
  widget_array[WI_BUTTON3] =
    XmCreatePushButton(widget_array[WI_MESSAGEBOX], "button3", NULL, 0);

  /***************** button4 : XmPushButton *****************/
  widget_array[WI_BUTTON4] =
    XmCreatePushButton(widget_array[WI_MESSAGEBOX], "button4", NULL, 0);

  /***************** button5 : XmPushButton *****************/
  widget_array[WI_BUTTON5] =
    XmCreatePushButton(widget_array[WI_MESSAGEBOX], "button5", NULL, 0);

  /* Terminate the widget array */
  widget_array[57] = NULL;


  /***************** panelhelpText : XmText *****************/
  n = 0;
  ttbl = XtParseTranslationTable("#override\n\
~Ctrl ~Meta<BtnDown>:\n\
~Ctrl ~Meta<BtnUp>:");
  XtOverrideTranslations(widget_array[WI_PANELHELPTEXT], ttbl);
  unregister_as_dropsite(widget_array[WI_PANELHELPTEXT],
                         NULL,
                         NULL);

  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_PANELHELPTEXT], args, n);


  /***************** softwareForm : XmForm *****************/
  n = 0;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopWidget, widget_array[WI_PANELHELPTEXT]); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNbottomWidget, widget_array[WI_MESSAGEBOX]); n++;
  XtSetValues(widget_array[WI_SOFTWAREFORM], args, n);


  /***************** clusterLabel : XmLabel *****************/
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_CLUSTERLABEL], args, n);

  XtManageChild(widget_array[WI_CLUSTERLABEL]);

  /***************** sizeLabel : XmLabel *****************/
  n = 0;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNbottomWidget, widget_array[WI_PACKAGESSCROLLEDWINDOW]); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
  XtSetArg(args[n], XmNrightWidget, widget_array[WI_PACKAGESSCROLLEDWINDOW]); n++;
  XtSetValues(widget_array[WI_SIZELABEL], args, n);

  XtManageChild(widget_array[WI_SIZELABEL]);

  /***************** packagesScrolledWindow : XmScrolledWindow *****************/
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNtopWidget, widget_array[WI_CLUSTERLABEL]); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNbottomWidget, widget_array[WI_TOTALSIZEFORM]); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
  XtSetArg(args[n], XmNrightWidget, widget_array[WI_TOTALSIZEFORM]); n++;
  XtSetValues(widget_array[WI_PACKAGESSCROLLEDWINDOW], args, n);

  XtManageChild(widget_array[WI_SOFTWARECLUSTERSROWCOLUMN]);
  XtManageChild(widget_array[WI_PACKAGESSCROLLEDWINDOW]);

  /***************** totalSizeForm : XmForm *****************/
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNbottomWidget, widget_array[WI_LEGENDFRAME]); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET); n++;
  XtSetArg(args[n], XmNrightWidget, widget_array[WI_LEGENDFRAME]); n++;
  XtSetValues(widget_array[WI_TOTALSIZEFORM], args, n);


  /***************** totalSizeLabel : XmLabel *****************/
  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNrightWidget, widget_array[WI_TOTALSIZEVALUE]); n++;
  XtSetValues(widget_array[WI_TOTALSIZELABEL], args, n);

  XtManageChild(widget_array[WI_TOTALSIZELABEL]);

  /***************** totalSizeValue : XmLabel *****************/
  n = 0;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_TOTALSIZEVALUE], args, n);

  XtManageChild(widget_array[WI_TOTALSIZEVALUE]);
  XtManageChild(widget_array[WI_TOTALSIZEFORM]);

  /***************** legendFrame : XmFrame *****************/
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_LEGENDFRAME], args, n);


  /***************** expandLegendArrow : XmArrowButton *****************/
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_EXPANDLEGENDARROW], args, n);

  XtManageChild(widget_array[WI_EXPANDLEGENDARROW]);

  /***************** expandLegendLabel : XmLabel *****************/
  n = 0;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNleftWidget, widget_array[WI_EXPANDLEGENDARROW]); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_EXPANDLEGENDLABEL], args, n);

  XtManageChild(widget_array[WI_EXPANDLEGENDLABEL]);
  XtManageChild(widget_array[WI_EXPANDFORM]);

  /***************** contractLegendArrow : XmArrowButton *****************/
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_CONTRACTLEGENDARROW], args, n);

  XtManageChild(widget_array[WI_CONTRACTLEGENDARROW]);

  /***************** contractLegendLabel : XmLabel *****************/
  n = 0;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNleftWidget, widget_array[WI_CONTRACTLEGENDARROW]); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_CONTRACTLEGENDLABEL], args, n);

  XtManageChild(widget_array[WI_CONTRACTLEGENDLABEL]);
  XtManageChild(widget_array[WI_CONTRACTFORM]);

  /***************** requiredLegendButton : XmPushButton *****************/
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftOffset, 30); n++;
  XtSetValues(widget_array[WI_REQUIREDLEGENDBUTTON], args, n);

  XtManageChild(widget_array[WI_REQUIREDLEGENDBUTTON]);

  /***************** requiredLegendLabel : XmLabel *****************/
  n = 0;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNleftWidget, widget_array[WI_REQUIREDLEGENDBUTTON]); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_REQUIREDLEGENDLABEL], args, n);

  XtManageChild(widget_array[WI_REQUIREDLEGENDLABEL]);
  XtManageChild(widget_array[WI_REQUIREDFORM]);

  /***************** partialLegendButton : XmPushButton *****************/
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftOffset, 30); n++;
  XtSetValues(widget_array[WI_PARTIALLEGENDBUTTON], args, n);

  XtManageChild(widget_array[WI_PARTIALLEGENDBUTTON]);

  /***************** partialLegendLabel : XmLabel *****************/
  n = 0;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNleftWidget, widget_array[WI_PARTIALLEGENDBUTTON]); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_PARTIALLEGENDLABEL], args, n);

  XtManageChild(widget_array[WI_PARTIALLEGENDLABEL]);
  XtManageChild(widget_array[WI_PARTIALFORM]);

  /***************** selectedLegendButton : XmPushButton *****************/
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftOffset, 30); n++;
  XtSetValues(widget_array[WI_SELECTEDLEGENDBUTTON], args, n);

  XtManageChild(widget_array[WI_SELECTEDLEGENDBUTTON]);

  /***************** selectedLegendLabel : XmLabel *****************/
  n = 0;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNleftWidget, widget_array[WI_SELECTEDLEGENDBUTTON]); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_SELECTEDLEGENDLABEL], args, n);

  XtManageChild(widget_array[WI_SELECTEDLEGENDLABEL]);
  XtManageChild(widget_array[WI_SELECTEDFORM]);

  /***************** unselectedLegendButton : XmPushButton *****************/
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftOffset, 30); n++;
  XtSetValues(widget_array[WI_UNSELECTEDLEGENDBUTTON], args, n);

  XtManageChild(widget_array[WI_UNSELECTEDLEGENDBUTTON]);

  /***************** unselectedLegendLabel : XmLabel *****************/
  n = 0;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNleftWidget, widget_array[WI_UNSELECTEDLEGENDBUTTON]); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_UNSELECTEDLEGENDLABEL], args, n);

  XtManageChild(widget_array[WI_UNSELECTEDLEGENDLABEL]);
  XtManageChild(widget_array[WI_UNSELECTEDFORM]);
  XtManageChild(widget_array[WI_LEGENDROWCOLUMN]);
  XtManageChild(widget_array[WI_LEGENDFORM]);
  XtManageChild(widget_array[WI_LEGENDFRAME]);

  /***************** softwarePanedWindow : XmPanedWindow *****************/
  n = 0;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNleftWidget, widget_array[WI_PACKAGESSCROLLEDWINDOW]); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftOffset, 20); n++;
  XtSetValues(widget_array[WI_SOFTWAREPANEDWINDOW], args, n);

  XtManageChild(widget_array[WI_SOFTWAREINFOLABEL]);

  /***************** softwareInfoRowColumn : XmRowColumn *****************/
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_SOFTWAREINFOROWCOLUMN], args, n);

  XtManageChild(widget_array[WI_PRODLABEL]);
  XtManageChild(widget_array[WI_ABBREVLABEL]);
  XtManageChild(widget_array[WI_VENDORLABEL]);
  XtManageChild(widget_array[WI_DESCRIPTLABEL]);
  XtManageChild(widget_array[WI_ESTLABEL]);
  XtManageChild(widget_array[WI_SOFTWAREINFOROWCOLUMN]);

  /***************** softwareInfoRowColumn1 : XmRowColumn *****************/
  n = 0;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftWidget, widget_array[WI_SOFTWAREINFOROWCOLUMN]); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_SOFTWAREINFOROWCOLUMN1], args, n);

  XtManageChild(widget_array[WI_PRODVALUE]);
  XtManageChild(widget_array[WI_ABBREVVALUE]);
  XtManageChild(widget_array[WI_VENDORVALUE]);
  XtManageChild(widget_array[WI_DESCRIPTVALUE]);
  XtManageChild(widget_array[WI_SIZEROWCOLUMN]);
  XtManageChild(widget_array[WI_SOFTWAREINFOROWCOLUMN1]);
  XtManageChild(widget_array[WI_SOFTWAREINFOFORM]);
  XtManageChild(widget_array[WI_SOFTWAREINFOSCROLLEDWINDOW]);
  XtManageChild(widget_array[WI_SOFTWAREINFOFRAME]);
  XtManageChild(widget_array[WI_SOFTWAREDEPENDENCIESSCROLLEDLIST]);
  XtManageChild(widget_array[WI_SOFTWAREDEPENDENCIESLABEL]);
  XtManageChild(widget_array[WI_SOFTWAREDEPENDENCIESFRAME]);
  XtManageChild(widget_array[WI_SOFTWAREPANEDWINDOW]);
  XtManageChild(widget_array[WI_SOFTWAREFORM]);

  /***************** messageBox : XmMessageBox *****************/
  n = 0;
  XtSetArg(args[n], XmNdefaultButton, widget_array[WI_OKBUTTON]); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_MESSAGEBOX], args, n);

  XtAddCallback(widget_array[WI_OKBUTTON],
                XmNactivateCallback,
                (XtCallbackProc)softwareOkCB,
                (XtPointer)NULL);

  XtManageChild(widget_array[WI_OKBUTTON]);
  XtAddCallback(widget_array[WI_HELPBUTTON],
                XmNactivateCallback,
                pfgHelp,
                (XtPointer)"spotsoftcust.r");

  XtManageChild(widget_array[WI_HELPBUTTON]);
  XtManageChild(widget_array[WI_MESSAGEBOX]);

  /*
   *  Allocate memory for the widget array to return
   */
  if (warr_ret != NULL) {
    *warr_ret = (Widget *) malloc(sizeof(Widget)*58);
    (void) memcpy((char *)*warr_ret,
                  (char *)widget_array,
           sizeof(Widget)*58);
  }

  /*
   *   Fix for SMI's X/NeWS server
   */
  tu_ol_fix_hierarchy(widget_array[WI_SOFTWARE_DIALOG]);

  /*
   *   Invoke the create callbacks
   */
  tu_widcre_invoke_hooks(widget_array[WI_SOFTWARE_DIALOG]);

  /*
   *   Return the first created widget.
   */
  return widget_array[WI_SOFTWARE_DIALOG];
}



/****************************************************************
 * create_method:
 *     This function creates a widget hierarchy using the
 *     functions generated above.
 ****************************************************************/
static Widget create_method(char               * temp,
                            char               * name,
                            Widget               parent,
                            Display            * disp,
                            Screen             * screen,
                            tu_template_descr  * retval)
{
  Widget w;

  sDisplay = disp;
  sScreen = screen;

  /* check each node against its name and call its
   * create function if appropriate */
  w = NULL;
  if (strcmp(temp, "software_dialog") == 0){
    w = tu_software_dialog_widget(name, parent, (Widget **)retval);
  }

  sDisplay = NULL;
  sScreen = NULL;
  return w;
}

