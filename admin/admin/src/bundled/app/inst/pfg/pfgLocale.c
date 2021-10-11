#ifndef lint
#pragma ident "@(#)pfglocale.c 1.68 96/07/26 SMI"
#endif

/*
 * Copyright (c) 1994-1996 by Sun MicroSystems, Inc.
 * All rights reserved.
 */

/*
 * Module:	pfglocale.c
 * Group:	installtool
 * Description:
 */

#include "pf.h"
#include "pfg.h"
#include "pfglocales.h"

#include "pfgLocale_ui.h"

static void localeContinueCB(Widget, XtPointer, XtPointer);
static void localeRemoveCB(Widget, XtPointer, XtPointer);
static void localeAddCB(Widget, XtPointer, XtPointer);
static int setLocales(Widget, int);
static void localeListCB(Widget, XtPointer, XmListCallbackStruct *);

static SelLists *lists;

static WidgetList widget_list = NULL;

static Widget locale_dialog;

Widget
pfgCreateLocales(void)
{
	XmString *unselectedItems = NULL;
	XmString *selectedItems = NULL;
	Module *module;
	int sCount = 0, uCount = 0, i;

	Dimension height, width;

	if (widget_list)
		free(widget_list);

	widget_list = NULL;

	/* create list of locales */
	module = get_all_locales();

	/* walk the locales module structure inorder to setup lists */
	/* NOTE assuming the locales have no subtree */
	while (module != NULL) {

		if (module->info.locale->l_selected == SELECTED) {
			sCount++;
			selectedItems = realloc(selectedItems,
				(sizeof (XmString) * sCount));
			if (selectedItems == NULL) {
				perror("pfgCreateLocales:malloc failed");
				pfgCleanExit(EXIT_INSTALL_FAILURE,
					(void *) NULL);
			}
			selectedItems[sCount - 1] = XmStringCreateLocalized(
				module->info.locale->l_language);
		} else {
			uCount++;
			unselectedItems = realloc(unselectedItems,
				(sizeof (XmString) *uCount));
			if (unselectedItems == NULL) {
				perror("pfgCreateLocales: malloc");
				pfgCleanExit(EXIT_INSTALL_FAILURE,
					(void *) NULL);
			}
			unselectedItems[uCount - 1] = XmStringCreateLocalized(
				module->info.locale->l_language);
			write_debug(GUI_DEBUG_L1,
				"creating unselect list\nlocale = %s",
				module->info.locale->l_language);
		}
		module = get_next(module);
	}

	write_debug(GUI_DEBUG_L1,
		"uCount=%d,  sCount=%d\n", uCount, sCount);

	lists = (SelLists *) xmalloc(sizeof (SelLists));

	locale_dialog = tu_locale_dialog_widget("locale_dialog",
		pfgTopLevel, &widget_list);

	XmAddWMProtocolCallback(pfgShell(locale_dialog), pfgWMDeleteAtom,
		(XtCallbackProc) pfgExit, NULL);

	XtVaSetValues(pfgShell(locale_dialog),
		XmNdeleteResponse, XmDO_NOTHING,
		XmNtitle, TITLE_LOCALES,
		NULL);

	pfgSetWidgetString(widget_list, "panelhelpText", MSG_LOCALES);
	pfgSetWidgetString(widget_list, "availableLabel", PFG_LC_DONT);
	pfgSetWidgetString(widget_list, "selectedLabel", PFG_LC_SUPPORT);
	pfgSetWidgetString(widget_list, "addButton", PFG_LC_ADD);
	pfgSetWidgetString(widget_list, "removeButton", PFG_LC_REMOVE);

	pfgSetWidgetString(widget_list, "continueButton", PFG_CONTINUE);
	pfgSetWidgetString(widget_list, "gobackButton", PFG_GOBACK);
	pfgSetWidgetString(widget_list, "exitButton", PFG_EXIT);
	pfgSetWidgetString(widget_list, "helpButton", PFG_HELP);

	XtAddCallback(pfgGetNamedWidget(widget_list, "addButton"),
		XmNactivateCallback, localeAddCB, lists);
	XtAddCallback(pfgGetNamedWidget(widget_list, "removeButton"),
		XmNactivateCallback, localeRemoveCB, lists);

	lists->unselectList =
		pfgGetNamedWidget(widget_list, "availableScrolledList");
	XtVaSetValues(lists->unselectList,
		XmNitems, unselectedItems,
		XmNitemCount, uCount,
		NULL);

	lists->selectList =
		pfgGetNamedWidget(widget_list, "selectedScrolledList");
	XtVaSetValues(lists->selectList,
		XmNitems, selectedItems,
		XmNitemCount, sCount,
		NULL);

	XtAddCallback(pfgGetNamedWidget(widget_list, "continueButton"),
		XmNactivateCallback, localeContinueCB, lists);

	XtAddCallback(lists->unselectList, XmNbrowseSelectionCallback,
		localeListCB, (XtPointer) lists);
	XtAddCallback(lists->selectList, XmNbrowseSelectionCallback,
		localeListCB, (XtPointer) lists);

	XtAddCallback(lists->unselectList, XmNdefaultActionCallback,
		localeListCB, (XtPointer) lists);
	XtAddCallback(lists->selectList, XmNdefaultActionCallback,
		localeListCB, (XtPointer) lists);

	XtManageChild(locale_dialog);

	XtVaGetValues(pfgShell(locale_dialog),
		XmNwidth, &width,
		XmNheight, &height,
		NULL);

	XtVaSetValues(pfgShell(locale_dialog),
		XmNminWidth, width,
		XmNminHeight, height,
		NULL);

	/* free list of strings */
	if (unselectedItems != NULL) {
		for (i = 0; i < uCount; i++) {
			XmStringFree(unselectedItems[i]);
		}
		free(unselectedItems);
	}
	if (selectedItems != NULL) {
		for (i = 0; i < sCount; i++) {
			XmStringFree(selectedItems[i]);
		}
		free(selectedItems);
	}

	return (locale_dialog);
}

/* ARGSUSED */
void
localeContinueCB(Widget w, XtPointer clientD, XtPointer callD)
{
	SelLists *lists;

	/* LINTED [pointer cast] */
	lists = (SelLists *) clientD;
	free(lists);

	if (widget_list)
		free(widget_list);

	widget_list = NULL;

	pfgSetAction(parAContinue);
}


int
setLocales(Widget list, int action)
{
	XmString *items;
	char *string;
	int count;
	int err = FAILURE;
	Module *mod, *prod;

	prod = get_current_product();
	XtVaGetValues(list,
		XmNselectedItems, &items,
		XmNselectedItemCount, &count,
		NULL);
	if (count == 0)
		return (SUCCESS);

	XmStringGetLtoR(*items, XmSTRING_DEFAULT_CHARSET, &string);
	if (string == NULL)
		return (SUCCESS);

	/*
	 * this is an N squared algorithm, however performance should not be
	 * a problem since most installations will have less than 5 locales
	 */
	for (mod = get_all_locales(); mod; mod = get_next(mod)) {
		if (strcmp(mod->info.locale->l_language, string) == 0) {
			if (action == SELECTED) {
				write_debug(GUI_DEBUG_L1,
					"mark %s as selected",
					mod->info.locale->l_locale);
				if (pfgState & AppState_UPGRADE) {
					upg_select_locale(prod,
						mod->info.locale->l_locale);
					err = SUCCESS;
				} else {
					err = select_locale(prod, mod->
						info.locale->l_locale);
				}
			} else if (action == UNSELECTED) {
				write_debug(GUI_DEBUG_L1,
					"mark %s as unselected",
					mod->info.locale->l_locale);
				if (pfgState & AppState_UPGRADE) {
					upg_select_locale(prod,
						mod->info.locale->l_locale);
					err = SUCCESS;
				} else {
					err = deselect_locale(prod, mod->
						info.locale->l_locale);
				}
			}
		break;
		}
	}
	return (err);
}

/* ARGSUSED */
void
localeRemoveCB(Widget remove, XtPointer clientD, XtPointer callD)
{
	XmString *selectItems;
	int sCount;
	SelLists *lists;
	int ret;

	/* LINTED [pointer cast] */
	lists = (SelLists *) clientD;
	XtVaGetValues(lists->selectList,
		XmNselectedItems, &selectItems,
		XmNselectedItemCount, &sCount,
		NULL);

	ret = setLocales(lists->selectList, UNSELECTED);
	if (ret != SUCCESS) {
		pfgWarning(locale_dialog, pfErLANG);
	}

	XmListAddItems(lists->unselectList, selectItems, sCount, 0);

	XmListDeleteItems(lists->selectList, selectItems, sCount);

	XtSetSensitive(pfgGetNamedWidget(widget_list, "addButton"), False);
	XtSetSensitive(pfgGetNamedWidget(widget_list, "removeButton"), False);
	XmListDeselectAllItems(lists->unselectList);
	XmListDeselectAllItems(lists->selectList);
}

/* ARGSUSED */
void
localeAddCB(Widget add, XtPointer clientD, XtPointer callD)
{
	XmString *unselectItems;
	int uCount;
	SelLists *lists;
	int ret;

	/* LINTED [pointer cast] */
	lists = (SelLists *) clientD;
	XtVaGetValues(lists->unselectList,
		XmNselectedItems, &unselectItems,
		XmNselectedItemCount, &uCount,
		NULL);

	ret = setLocales(lists->unselectList, SELECTED);
	if (ret != SUCCESS) {
		pfgWarning(locale_dialog, pfErLANG);
	}

	XmListAddItems(lists->selectList, unselectItems, uCount, 0);
	XmListDeleteItems(lists->unselectList, unselectItems, uCount);

	XtSetSensitive(pfgGetNamedWidget(widget_list, "addButton"), False);
	XtSetSensitive(pfgGetNamedWidget(widget_list, "removeButton"), False);
	XmListDeselectAllItems(lists->unselectList);
	XmListDeselectAllItems(lists->selectList);
}

/* ARGSUSED */
static void
localeListCB(Widget list, XtPointer client, XmListCallbackStruct *cbs)
{
	Widget left, right;
	SelLists *lll;

	/* LINTED [pointer cast] */
	lll = (SelLists *) client;
	left = lll->unselectList;
	right = lll->selectList;

	if (cbs->reason == XmCR_DEFAULT_ACTION) {	/* double click */
		if (list == left)
			localeAddCB(list, client, (XtPointer) NULL);
		else
			localeRemoveCB(list, client, (XtPointer) NULL);

	} else {		/* single click */
		if (list == left) {
			XtSetSensitive(
				pfgGetNamedWidget(widget_list, "addButton"),
				True);
			XtSetSensitive(
				pfgGetNamedWidget(widget_list, "removeButton"),
				False);
			XmListDeselectAllItems(right);
		} else {
			XtSetSensitive(
				pfgGetNamedWidget(widget_list, "addButton"),
				False);
			XtSetSensitive(
				pfgGetNamedWidget(widget_list, "removeButton"),
				True);
			XmListDeselectAllItems(left);
		}
	}
}

/* ARGSUSED */
void
localeGobackCB(Widget w, XtPointer clientD, XtPointer callD)
{
	pfgSetAction(parAGoback);
}
hment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNtopWidget, widget_array[WI_PANELHELPTEXT]); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNbottomWidget, widget_array[WI_MESSAGEBOX]); n++;
  XtSetValues(widget_array[WI_FORM], args, n);


  /***************** availableLabel : XmLabel *****************/
  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION); n++;
  XtSetArg(args[n], XmNrightPosition, 4); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_AVAILABLELABEL], args, n);

  XtManageChild(widget_array[WI_AVAILABLELABEL]);

  /***************** availableScrolledList : XmScrolledList *****************/
  pn = 0;
  XtSetArg(pargs[pn], XmNleftAttachment, XmATTACH_FORM); pn++;
  XtSetArg(pargs[pn], XmNtopAttachment, XmATTACH_WIDGET); pn++;
  XtSetArg(pargs[pn], XmNtopWidget, widget_array[WI_SELECTEDLABEL]); pn++;
  XtSetArg(pargs[pn], XmNbottomAttachment, XmATTACH_FORM); pn++;
  XtSetArg(pargs[pn], XmNrightAttachment, XmATTACH_POSITION); pn++;
  XtSetArg(pargs[pn], XmNrightPosition, 4); pn++;
  tmpw = get_constraint_widget(widget_array[WI_AVAILABLESCROLLEDLIST], widget_array[WI_FORM]);
  if (tmpw)
    XtSetValues(tmpw, pargs, pn);

  XtManageChild(widget_array[WI_AVAILABLESCROLLEDLIST]);

  /***************** buttonForm : XmRowColumn *****************/
  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNtopWidget, widget_array[WI_AVAILABLELABEL]); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION); n++;
  XtSetArg(args[n], XmNleftPosition, 4); n++;
  XtSetValues(widget_array[WI_BUTTONFORM], args, n);

  XtSetSensitive(widget_array[WI_ADDBUTTON], False);
  XtManageChild(widget_array[WI_ADDBUTTON]);
  XtSetSensitive(widget_array[WI_REMOVEBUTTON], False);
  XtManageChild(widget_array[WI_REMOVEBUTTON]);
  XtManageChild(widget_array[WI_BUTTONFORM]);

  /***************** selectedLabel : XmLabel *****************/
  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n], XmNleftPosition, 6); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftWidget, widget_array[WI_BUTTONFORM]); n++;
  XtSetValues(widget_array[WI_SELECTEDLABEL], args, n);

  XtManageChild(widget_array[WI_SELECTEDLABEL]);

  /***************** selectedScrolledList : XmScrolledList *****************/
  pn = 0;
  XtSetArg(pargs[pn], XmNleftAttachment, XmATTACH_WIDGET); pn++;
  XtSetArg(pargs[pn], XmNleftWidget, widget_array[WI_BUTTONFORM]); pn++;
  XtSetArg(pargs[pn], XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET); pn++;
  tmpw1 = get_constraint_widget(
widget_array[WI_AVAILABLESCROLLEDLIST], widget_array[WI_FORM]);
  XtSetArg(pargs[pn], XmNtopWidget, tmpw1); pn++;
  XtSetArg(pargs[pn], XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET); pn++;
  tmpw1 = get_constraint_widget(
widget_array[WI_AVAILABLESCROLLEDLIST], widget_array[WI_FORM]);
  XtSetArg(pargs[pn], XmNbottomWidget, tmpw1); pn++;
  XtSetArg(pargs[pn], XmNrightAttachment, XmATTACH_FORM); pn++;
  tmpw = get_constraint_widget(widget_array[WI_SELECTEDSCROLLEDLIST], widget_array[WI_FORM]);
  if (tmpw)
    XtSetValues(tmpw, pargs, pn);

  XtManageChild(widget_array[WI_SELECTEDSCROLLEDLIST]);
  XtManageChild(widget_array[WI_FORM]);

  /***************** messageBox : XmMessageBox *****************/
  n = 0;
  XtSetArg(args[n], XmNdefaultButton, widget_array[WI_CONTINUEBUTTON]); n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
  XtSetValues(widget_array[WI_MESSAGEBOX], args, n);

  XtManageChild(widget_array[WI_CONTINUEBUTTON]);
  XtAddCallback(widget_array[WI_GOBACKBUTTON],
                XmNactivateCallback,
                (XtCallbackProc)localeGobackCB,
                (XtPointer)NULL);

  XtManageChild(widget_array[WI_GOBACKBUTTON]);
  XtAddCallback(widget_array[WI_EXITBUTTON],
                XmNactivateCallback,
                (XtCallbackProc)pfgExit,
                (XtPointer)NULL);

  XtManageChild(widget_array[WI_EXITBUTTON]);
  XtAddCallback(widget_array[WI_HELPBUTTON],
                XmNactivateCallback,
                pfgHelp,
                (XtPointer)"language.t");

  XtManageChild(widget_array[WI_HELPBUTTON]);
  XtManageChild(widget_array[WI_MESSAGEBOX]);

  /*
   *  Allocate memory for the widget array to return
   */
  if (warr_ret != NULL) {
    *warr_ret = (Widget *) malloc(sizeof(Widget)*17);
    (void) memcpy((char *)*warr_ret,
                  (char *)widget_array,
           sizeof(Widget)*17);
  }

  /*
   *   Fix for SMI's X/NeWS server
   */
  tu_ol_fix_hierarchy(widget_array[WI_LOCALE_DIALOG]);

  /*
   *   Invoke the create callbacks
   */
  tu_widcre_invoke_hooks(widget_array[WI_LOCALE_DIALOG]);

  /*
   *   Return the first created widget.
   */
  return widget_array[WI_LOCALE_DIALOG];
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
  if (strcmp(temp, "locale_dialog") == 0){
    w = tu_locale_dialog_widget(name, parent, (Widget **)retval);
  }

  sDisplay = NULL;
  sScreen = NULL;
  return w;
}

