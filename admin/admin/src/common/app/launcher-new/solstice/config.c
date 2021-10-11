/*
 * Copyright (c) 1995 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma	ident	"@(#)config.c	1.24	96/08/02 SMI"


#include <Xm/ArrowB.h>
#include <Xm/DialogS.h>
#include <Xm/LabelG.h>
#include <Xm/Form.h>
#include <Xm/List.h>
#include <Xm/PushB.h>
#include <Xm/ScrolledW.h>
#include <Xm/Separator.h>
#include <Xm/TextF.h>
#include <Xm/ToggleBG.h>
#include <Xm/RowColumn.h>

#include "util.h"
#include "launcher.h"
#include "action.h"

configContext_t * configContext = NULL;
Widget configDialog = NULL;

static int		buttonWidth = 220;
static Boolean		buttonsResizable = False;
static Boolean		redisplayPalette = False;

static Widget 	build_button_form(configContext_t *);
extern void	show_propertyDialog(Widget, Boolean);

#define FORM_MARGIN 10
#define SCROLLW_WIDTH	180
#define SCROLLW_RIGHT_POS 80
#define ARROW_LABEL_WIDTH	50
#define LABEL_BOTTOM_OFFSET 10
#define HIDE_LIST_RA 2
#define BUTTONS_LA 11
#define BUTTONS_RA 15
#define SHOW_LIST_LA 17
#define SHOW_LIST_RA 28
#define ACC_LA 30


char general_msg[256];

extern int chosenNumColumns;
int newNumCols = 3;
Widget spin_list;

static Widget numColumnsfieldw;

/* Callbacks for List of apps */
extern void selectionCB(Widget, XtPointer, XmListCallbackStruct *);
extern void doubleclickCB(Widget, XtPointer, XmListCallbackStruct *);

/* extern from list.c */
extern int display_appList(Widget, visibility_t);
extern void 	delete_appList_entry(Widget, int); 
extern XmString get_appList_item(Widget, int);
extern Widget	which_list(int **, int *);

/* from apptable.c */
extern AppInfo 	* itemToAppInfo(XmString);
extern void 	resetVisibility_appTable_entry();

#define LIST_SIZE(list, cnt) XtVaGetValues(list, XmNitemCount, &cnt, NULL)

/*
 *  cancel
 */
static void
cancelCB(
	Widget		w, 
	XtPointer	cd, 
	XtPointer	cbs)
{
	resetVisibility_appTable_entry();
	if (propertyDialog && XtIsRealized(propertyDialog))
		XtPopdown(propertyDialog);
	XtPopdown(configDialog);
}

static void
applyCB(Widget w, XtPointer cd, XtPointer cbs)
{
	int val;

	commitVisibility_appTable_entry();
	if ((newNumCols != chosenNumColumns) || redisplayPalette) {
		chosenNumColumns = newNumCols;
		update_display_layout();
	}
	if (cd) {
		XtPopdown(configDialog);
	}
}

static void
showCB(Widget w, XtPointer cd, XtPointer cbs)
{
	int icnt;
	int * index_list;
	configContext_t * c = (configContext_t *)cd;
	XmString	item;
	AppInfo		* ai;

	if (XmListGetSelectedPos(c->c_hideappList, &index_list, &icnt)) {
		item = get_appList_item(c->c_hideappList, index_list[0]);
		ai = itemToAppInfo(item);
		if (ai)
			ai->a_show = SHOW_PENDING;
		else
			display_error(w, catgets(catd, 1, 3, "Invalid Application"));
		XmListAddItem(c->c_appList, item, 0);
		XmListDeleteItem(c->c_hideappList, item);
		XmListSelectPos(c->c_hideappList, index_list[0], False);
		LIST_SIZE(c->c_hideappList, icnt);
		if (icnt == 0)
			XtVaSetValues(w, XmNsensitive, False, NULL);
		redisplayPalette = True;
	}
}

static void
hideCB(Widget w, XtPointer cd, XtPointer cbs)
{
	int icnt;
	int * index_list;
	configContext_t * c = (configContext_t *)cd;
	XmString	item;
	AppInfo		* ai;

	if (XmListGetSelectedPos(c->c_appList, &index_list, &icnt)) {
		item = get_appList_item(c->c_appList, index_list[0]);
		ai = itemToAppInfo(item);
		if (ai)
			ai->a_show = HIDE_PENDING;
		else
			display_error(w, catgets(catd, 1, 4, "Invalid Application"));
		XmListAddItem(c->c_hideappList, item, 0);
		XmListDeleteItem(c->c_appList, item);
		XmListSelectPos(c->c_appList, index_list[0], False);
		LIST_SIZE(c->c_appList, icnt);
		if (icnt == 0)
			XtVaSetValues(w, XmNsensitive, False, NULL);
		redisplayPalette = True;
	}
}

static void
removeCB(Widget w, XtPointer cd, XtPointer cbs)
{
	configContext_t * c;
	int 			* index_list;
	int			rc, cnt, i;
	Boolean			deleted = False;
   	AppInfo			* ai;
	XmString		item;
	Widget 			list_w;

	XtVaGetValues(w, XmNuserData, &c, NULL);

	list_w = which_list(&index_list, &cnt);
	if (list_w == NULL)
		return;

	for (i = 0; i < cnt; i++) {
		item = get_appList_item(list_w, index_list[i]);
		ai = itemToAppInfo(item);
		if (ai == NULL) {
			display_error(w, catgets(catd, 1, 5, "Invalid Application"));
			continue;
		}
sprintf(general_msg, catgets(catd, 1, 6, "Do you really want to remove %s from the Launcher registration list?"), ai->a_appName);
		if (!Confirm(launchermain, general_msg, False, catgets(catd, 1, 7, "Remove"))) 
			continue;

		if ((rc = solstice_del_app(ai->a_appName, localRegistry)) == LAUNCH_OK) 
		{
			delete_appList_entry(list_w, index_list[i]);
			if (ai->a_show == SHOW)
				unSlapIconFromPalette(ai);
			remove_appTable_entry(ai->a_appName);
			deleted = True;
		}
		else
			solstice_error(w, rc);
	}
	XtFree((char *)index_list);
	if (deleted)
		set_edit_functions(c, False, False, -1);
}

void
propertyCB(Widget w,
	XtPointer cd,
	XtPointer cbs)
{
	Boolean disp = (Boolean) cd;

	show_propertyDialog(w, disp);
}

void swap_appTable_entries(int, int);

#define MOVE_UP(l, i)	(swap_appList_entries(l, i, i-1))
#define MOVE_DOWN(l, i)	(swap_appList_entries(l, i, i+1))

static void
upCB(Widget w, XtPointer cd, XtPointer cbs)
{
	configContext_t * c = (configContext_t *) cd;
	int * index_list, cnt;

	if (XmListGetSelectedPos(c->c_appList, &index_list, &cnt))
		if (index_list[0]-1 > 0)  {
			MOVE_UP(c->c_appList, index_list[0]);
			redisplayPalette = True;
		}
}

static void
downCB(Widget w, XtPointer cd, XtPointer cbs)
{
	configContext_t * c = (configContext_t *) cd;
	int * index_list, cnt, sz;

	if (XmListGetSelectedPos(c->c_appList, &index_list, &cnt)) {
		LIST_SIZE(c->c_appList, sz);
		if (index_list[0] < sz) {
			redisplayPalette = True;
			MOVE_DOWN(c->c_appList, index_list[0]);
		}
	}
}

static void
panelButtonCB(Widget w, XtPointer cd, XtPointer cbs)
{
	int delta = (int)cd;
	char num[2];

	newNumCols += delta;
	if (newNumCols < 1) {	
		XBell(Gdisplay, 0);
		newNumCols = 1;
	} else if (newNumCols > 16) {
		XBell(Gdisplay, 0);
		newNumCols = 16;
	}
	sprintf(num, "%2.d", newNumCols);
	XmTextFieldSetString(numColumnsfieldw, num);
}

static void
justificationCB(
	Widget w, 
	XtPointer cd, 
	XmListCallbackStruct* cbs)
{
}

static Widget
title_sep(Widget form, char * title) 
{
	Widget titlew, sep;

	titlew = XtVaCreateManagedWidget("title",
                    xmLabelGadgetClass, form,
                    XmNtopAttachment, XmATTACH_FORM,
		    XmNtopOffset, OFFSET, 
                    XmNleftAttachment, XmATTACH_FORM,
                    RSC_CVT( XmNlabelString, title),
                    NULL);
        sep = XtVaCreateManagedWidget( "sep",
                        xmSeparatorWidgetClass,
                        form,
                        XmNleftAttachment, XmATTACH_FORM,
                        XmNrightAttachment, XmATTACH_FORM,
                        XmNtopAttachment, XmATTACH_WIDGET,
                        XmNtopWidget, titlew,
                        XmNleftOffset, 0,
                        XmNrightOffset, 0,
                        NULL );
	return(sep);
}

ActionItem action_items[] = {
/* These strings are placeholders. The field values are reset with
   wrapped" strings at run-time.
*/
	{ "OK",  applyCB, (XtPointer)True, NULL},
	{ "Apply",  applyCB, NULL, NULL},
/* TMP */
	{ "Cancel",  cancelCB, NULL, NULL},
	{ "Help",  helpCB, "props.h.hlp", NULL},
};

static Widget
build_arrow_form(configContext_t * ctxt, Widget top, Widget bottom) 
{
	Widget arrowForm;
	Widget sep;
	Widget		upLabel, downLabel;

	arrowForm = XtVaCreateManagedWidget("arrowForm",
			xmFormWidgetClass, ctxt->c_workForm,
                    XmNleftAttachment, XmATTACH_POSITION,
		    XmNleftPosition, 5,
		    XmNleftOffset, OFFSET,
#if 0
                    XmNrightAttachment, XmATTACH_POSITION,
		    XmNrightPosition, 8,
                        XmNrightAttachment, XmATTACH_FORM,
			XmNrightOffset, OFFSET,
#endif
			XmNwidth, buttonWidth,
			XmNtopOffset, OFFSET,
			XmNbottomOffset, OFFSET,
			NULL);
	if (top)
		XtVaSetValues(arrowForm, 
			XmNtopAttachment, XmATTACH_WIDGET,
                        XmNtopWidget, top,
			NULL);
	if (bottom)
		XtVaSetValues(arrowForm, 
			XmNbottomAttachment, XmATTACH_WIDGET,
                        XmNbottomWidget, bottom,
			NULL);

	sep = title_sep(arrowForm, "Icon Order");

	ctxt->c_upButton =  XtVaCreateManagedWidget("upButton",
                        xmArrowButtonWidgetClass, arrowForm,
			XmNwidth, ARROW_LABEL_WIDTH,
                        XmNleftAttachment, XmATTACH_FORM,
                        XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, sep,
			XmNtopOffset, OFFSET,
                        XmNnavigationType, XmTAB_GROUP,
			XmNarrowDirection, XmARROW_UP,
			NULL);
	XtAddCallback(ctxt->c_upButton, XmNactivateCallback,
			upCB, (XtPointer) ctxt);
	upLabel	=  XtVaCreateManagedWidget( "upLabel",
			xmLabelGadgetClass,
			arrowForm,
			RSC_CVT(XmNlabelString, catgets(catd, 1, 12, "Up")),
                        XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, sep,
			XmNtopOffset, OFFSET,
			XmNrightAttachment, XmATTACH_FORM,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget,  ctxt->c_upButton,
			XmNalignment, XmALIGNMENT_BEGINNING,
			NULL);
	ctxt->c_downButton =  XtVaCreateManagedWidget("downButton",
                        xmArrowButtonWidgetClass, arrowForm,
			XmNwidth, ARROW_LABEL_WIDTH,
                        XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, ctxt->c_upButton,
                        XmNleftAttachment, XmATTACH_FORM,
                        XmNbottomAttachment, XmATTACH_FORM,
                        XmNnavigationType, XmTAB_GROUP,
			XmNarrowDirection, XmARROW_DOWN,
			NULL);
	XtAddCallback(ctxt->c_downButton, XmNactivateCallback,
			downCB, (XtPointer) ctxt);
	downLabel = XtVaCreateManagedWidget( "downLabel",
			xmLabelGadgetClass,
			arrowForm,
			RSC_CVT(XmNlabelString, catgets(catd, 1, 13, "Down")),
			XmNrightAttachment, XmATTACH_FORM,
			XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget,  ctxt->c_downButton,
			XmNbottomAttachment, XmATTACH_FORM,
			XmNalignment, XmALIGNMENT_BEGINNING,
			NULL);

	return(arrowForm);
}	

static Widget
build_button_form(configContext_t * ctxt)
{
	Widget buttonForm;
	Widget button;

	buttonForm = XtVaCreateManagedWidget("buttonForm",
			xmFormWidgetClass, ctxt->c_workForm,
			XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNtopWidget, ctxt->c_scrollWin, 
                    XmNleftAttachment, XmATTACH_POSITION,
		    XmNleftPosition, 5,
		    XmNleftOffset, OFFSET,
			XmNwidth, buttonWidth,
			XmNbottomOffset, OFFSET,
			NULL);
	button = ctxt->c_addAppButton =  XtVaCreateManagedWidget("addAppButton",
                        xmPushButtonWidgetClass, buttonForm,
                        RSC_CVT( XmNlabelString, catgets(catd, 1, 14, "Add Application...") ),
			XmNtopAttachment, XmATTACH_FORM,
                        XmNrightAttachment, XmATTACH_FORM,
			XmNrightOffset, OFFSET,
			XmNleftAttachment, XmATTACH_FORM,
                        XmNnavigationType, XmTAB_GROUP,
			XmNresizable, buttonsResizable,
			XmNwidth, buttonWidth,
			NULL);

	XtVaSetValues(ctxt->c_addAppButton, 
			XmNuserData, ctxt,
			NULL);
	XtAddCallback(ctxt->c_addAppButton, 
			XmNactivateCallback,
			(XtCallbackProc) propertyCB,
			(XtPointer) NULL);

	ctxt->c_propertyButton =  XtVaCreateManagedWidget(
			catgets(catd, 1, 15, "propertyButton"),
                        xmPushButtonWidgetClass, buttonForm,
                        RSC_CVT( XmNlabelString, catgets(catd, 1, 16, "Application Properties...") ),
                        XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, button,
                        XmNrightAttachment, XmATTACH_FORM,
			XmNrightOffset, OFFSET,
			XmNleftAttachment, XmATTACH_FORM,
                        XmNnavigationType, XmTAB_GROUP,
			XmNresizable, buttonsResizable,
			XmNwidth, buttonWidth,
			XmNsensitive, False,
			NULL);
	XtVaSetValues(ctxt->c_propertyButton, 
			XmNuserData, ctxt,
			NULL);
	XtAddCallback(ctxt->c_propertyButton, 
			XmNactivateCallback,
			(XtCallbackProc) propertyCB,
			(XtPointer) True);
	return(buttonForm);
}

Widget
build_title_justification_form(configContext_t * ctxt, Widget top, Widget bottom) 
{
	XmString left, right, center;
	Widget rbox, sep;
	Widget radioForm;

	radioForm = XtVaCreateManagedWidget("radioForm",
			xmFormWidgetClass, ctxt->c_workForm,
                        XmNrightAttachment, XmATTACH_FORM,
			XmNrightOffset, OFFSET,
			XmNtopOffset, OFFSET,
			XmNbottomOffset, OFFSET,
			XmNwidth, buttonWidth,
			NULL);
	if (top)
		XtVaSetValues(radioForm, 
			XmNtopAttachment, XmATTACH_WIDGET,
                        XmNtopWidget, top,
			NULL);
	if (bottom)
		XtVaSetValues(radioForm, 
			XmNbottomAttachment, XmATTACH_WIDGET,
                        XmNbottomWidget, bottom,
			NULL);

	sep = title_sep(radioForm, catgets(catd, 1, 17, "Title Justification"));

	left = XmStringCreateLocalized(catgets(catd, 1, 18, "Left"));
	center = XmStringCreateLocalized(catgets(catd, 1, 19, "Center"));
	right = XmStringCreateLocalized(catgets(catd, 1, 20, "Right"));

	rbox = XmVaCreateSimpleRadioBox(radioForm, "radioBox",
		0,
		justificationCB,
		XmVaRADIOBUTTON, left, NULL, NULL, NULL,
		XmVaRADIOBUTTON, center, NULL, NULL, NULL,
		XmVaRADIOBUTTON, right, NULL, NULL, NULL,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, sep,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNrightOffset, OFFSET,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNorientation, XmHORIZONTAL,
		NULL);

	XmStringFree(left);
	XmStringFree(right);
	XmStringFree(center);

	XtManageChild(rbox);
	return(radioForm);
}

static void
vscrollCB(Widget w, XtPointer cd, XmScrollBarCallbackStruct * cbs)
{
	switch (cbs->reason) {
	case XmCR_TO_TOP:
	case XmCR_INCREMENT:
		newNumCols++;
		newNumCols = newNumCols > 10 ? 10 : newNumCols;
		break;
	case XmCR_TO_BOTTOM:
	case XmCR_DECREMENT:
		newNumCols--;
		newNumCols = newNumCols < 1 ? 1 : newNumCols;
		break;
	default:
		break;
	}
	XmListSetPos(spin_list, newNumCols);
}


static Widget
build_width_form(configContext_t * ctxt, Widget top, Widget bottom)
{
	Widget sep, widthForm;
	Widget vsb;
	char num[2];
	Widget bform, upButton, downButton;

	widthForm = XtVaCreateManagedWidget("widthForm",
			xmFormWidgetClass, ctxt->c_workForm,
                    XmNleftAttachment, XmATTACH_POSITION,
		    XmNleftPosition, 5,
		    XmNleftOffset, OFFSET,
#if 0
                    XmNrightAttachment, XmATTACH_POSITION,
		    XmNrightPosition, 8,
#endif
			XmNwidth, buttonWidth,
			XmNtopOffset, OFFSET,
			XmNbottomOffset, OFFSET,
			NULL);
	if (top)
		XtVaSetValues(widthForm, 
			XmNtopAttachment, XmATTACH_WIDGET,
                        XmNtopWidget, top,
			NULL);
	if (bottom)
		XtVaSetValues(widthForm, 
			XmNbottomAttachment, XmATTACH_WIDGET,
                        XmNbottomWidget, bottom,
			NULL);

	sep = title_sep(widthForm, catgets(catd, 1, 21, "Launcher Width"));

	spin_list = XmCreateScrolledList(widthForm, catgets(catd, 1, 22, "scrolledList"), NULL, NULL);
	XtVaSetValues(spin_list, 
		XmNwidth, 32,
		XmNitemCount, 10,
		XmNvisibleItemCount, 1,
		RSC_CVT(XmNitems, "1, 2, 3, 4, 5, 6, 7, 8, 9, 10"),
		NULL);
	XtVaSetValues(XtParent(spin_list),
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, sep,
		XmNtopOffset, OFFSET,
		XmNleftAttachment, XmATTACH_FORM,
                XmNresizable, False,
		XmNscrollBarDisplayPolicy, XmSTATIC,
		XmNtraversalOn, False,
		NULL);

	XmListSetPos(spin_list, chosenNumColumns);

	XtVaGetValues(XtParent(spin_list), XmNverticalScrollBar, &vsb, NULL);
	XtAddCallback(vsb, XmNvalueChangedCallback, vscrollCB, NULL);
	XtAddCallback(vsb, XmNincrementCallback, vscrollCB, NULL);
	XtAddCallback(vsb, XmNdecrementCallback, vscrollCB, NULL);
	XtAddCallback(vsb, XmNpageIncrementCallback, vscrollCB, NULL);
	XtAddCallback(vsb, XmNpageDecrementCallback, vscrollCB, NULL);
	XtAddCallback(vsb, XmNtoTopCallback, vscrollCB, NULL);
	XtAddCallback(vsb, XmNtoBottomCallback, vscrollCB, NULL);
	
	XtManageChild(spin_list);
 
	XtVaCreateManagedWidget("label",
                    xmLabelGadgetClass, widthForm,
                    XmNtopAttachment, XmATTACH_WIDGET,
		    XmNtopWidget, sep,
		    XmNtopOffset, OFFSET+4, 
                    XmNleftAttachment, XmATTACH_WIDGET,
		    XmNleftWidget, XtParent(spin_list),
                    RSC_CVT( XmNlabelString, catgets(catd, 1, 23, "Columns")),
                    NULL);

	return(widthForm);
}

void
build_hideList(configContext_t * ctxt)
{
	ctxt->c_hidetitle =  XtVaCreateManagedWidget("title",
                    xmLabelGadgetClass, ctxt->c_workForm,
                    XmNtopAttachment, XmATTACH_FORM,
		    XmNtopOffset, OFFSET, 
                    XmNleftAttachment, XmATTACH_FORM,
		    XmNleftOffset, OFFSET,
                    RSC_CVT( XmNlabelString, catgets(catd, 1, 24, "Hide")),
                    NULL);
	ctxt->c_hidescrollWin = XtVaCreateWidget( "scrollWin",
		xmScrolledWindowWidgetClass,
		ctxt->c_workForm,
		XmNwidth, SCROLLW_WIDTH,
		XmNresizable, False,
                XmNscrollingPolicy, XmAPPLICATION_DEFINED,
                XmNvisualPolicy, XmVARIABLE,
                XmNscrollBarDisplayPolicy, XmSTATIC,
                XmNshadowThickness, 0,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, ctxt->c_hidetitle,
                    XmNleftAttachment, XmATTACH_FORM,
		    XmNleftOffset, OFFSET,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNrightPosition, HIDE_LIST_RA,
		NULL );
	ctxt->c_hideappList = XtVaCreateManagedWidget( "appList",
                xmListWidgetClass,
                ctxt->c_hidescrollWin,
                XmNvisibleItemCount, 12,
                XmNlistSizePolicy, XmCONSTANT,
                XmNresizable, False,
		XmNtraversalOn, False,
                NULL);
	XtAddCallback(ctxt->c_hideappList, XmNbrowseSelectionCallback,
		(XtCallbackProc) selectionCB,
                (XtPointer) ctxt );
}

void
build_listButtons(configContext_t * ctxt)
{
	Widget button;
	Widget buttonForm;
	buttonForm = ctxt->c_listButtonForm = XtVaCreateManagedWidget("buttonForm",
			xmFormWidgetClass, ctxt->c_workForm,
			XmNtopAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_FORM,
                        XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, ctxt->c_hidescrollWin,
		        XmNleftOffset, OFFSET,
			XmNfractionBase, 12,
/*
                    XmNrightAttachment, XmATTACH_POSITION,
		    XmNrightPosition, 3,
*/
			NULL);
	button = ctxt->c_hideButton = XtVaCreateManagedWidget("hideAppButton",
                        xmPushButtonWidgetClass, buttonForm,
                        RSC_CVT( XmNlabelString, catgets(catd, 1, 25, "<< Hide") ),
                        XmNtopAttachment, XmATTACH_POSITION,
			XmNtopPosition, 3,
                        XmNbottomAttachment, XmATTACH_POSITION,
			XmNbottomPosition, 4,
                        XmNnavigationType, XmTAB_GROUP,
			XmNresizable, True,
			XmNwidth, buttonWidth/2,
			XmNsensitive, False,
			NULL);
	button = ctxt->c_showButton = XtVaCreateManagedWidget("showAppButton",
                        xmPushButtonWidgetClass, buttonForm,
                        RSC_CVT( XmNlabelString, catgets(catd, 1, 26, "Show >>") ),
                        XmNtopAttachment, XmATTACH_POSITION,
			XmNtopPosition, 4,
                        XmNbottomAttachment, XmATTACH_POSITION,
			XmNbottomPosition, 5,
                        XmNnavigationType, XmTAB_GROUP,
			XmNresizable, True,
			XmNwidth, buttonWidth/2,
			XmNsensitive, False,
			NULL);
	button = ctxt->c_rmAppButton =  XtVaCreateManagedWidget("rmAppButton",
                        xmPushButtonWidgetClass, buttonForm,
                        RSC_CVT( XmNlabelString, catgets(catd, 1, 27, "Remove") ),
                        XmNtopAttachment, XmATTACH_POSITION,
			XmNtopPosition, 7,
                        XmNbottomAttachment, XmATTACH_POSITION,
			XmNbottomPosition, 8,
                        XmNnavigationType, XmTAB_GROUP, 
			XmNresizable, True,
			XmNwidth, buttonWidth/2,
			XmNsensitive, False,
			NULL);
	XtVaSetValues(ctxt->c_rmAppButton, 
			XmNuserData, ctxt,
			NULL);
	XtAddCallback(ctxt->c_rmAppButton, 
			XmNactivateCallback,
			(XtCallbackProc) removeCB,
			(XtPointer) True);
	XtAddCallback(ctxt->c_showButton, 
			XmNactivateCallback,
			(XtCallbackProc) showCB,
			(XtPointer) ctxt);
	XtAddCallback(ctxt->c_hideButton, 
			XmNactivateCallback,
			(XtCallbackProc) hideCB,
			(XtPointer) ctxt);
}

Widget
build_config_dialog(Widget parent)
{
	configContext_t	*ctxt;
	Widget		w, shell;
	Widget		headerForm;
	Widget		addPulldown;
	Widget		arrowForm;
	Widget		buttonForm;
	Widget		titleForm;
	Widget		widthForm;
	Widget		ancestor;

	ctxt = (configContext_t *) malloc(sizeof(configContext_t));
	memset((void *)ctxt, (int) 0, (size_t)sizeof(configContext_t));
	
	ancestor = (parent == NULL ? GtopLevel : get_shell_ancestor(parent));

	shell = XtVaCreatePopupShell( "LauncherMain",
			xmDialogShellWidgetClass, ancestor,
			XmNtitle, catgets(catd, 1, 28, "Launcher: Properties"),
                	XmNinitialState, NormalState,
			/* XmNallowShellResize, False, */
			XmNminWidth, 600,
			XmNminHeight, 300,
			XmNx, 300,
			XmNy, 200,
			NULL );

	ctxt->c_workForm = XtVaCreateWidget( "workForm",
		xmFormWidgetClass,
		shell,
		XmNfractionBase, 8,
/*	
		XmNverticalSpacing, FORM_MARGIN,
		XmNhorizontalSpacing, FORM_MARGIN,
*/
		NULL );
	
	XtVaSetValues(ctxt->c_workForm, XmNuserData, ctxt, NULL);

	build_hideList(ctxt);
	build_listButtons(ctxt);

	ctxt->c_title =  XtVaCreateManagedWidget("title",
                    xmLabelGadgetClass, ctxt->c_workForm,
                    XmNtopAttachment, XmATTACH_FORM,
		    XmNtopOffset, OFFSET, 
                    XmNleftAttachment, XmATTACH_WIDGET,
		    XmNleftWidget, ctxt->c_listButtonForm,
		    XmNleftOffset, OFFSET,
                    RSC_CVT( XmNlabelString, catgets(catd, 1, 29, "Show")),
                    NULL);
	ctxt->c_scrollWin = XtVaCreateWidget( "scrollWin",
		xmScrolledWindowWidgetClass,
		ctxt->c_workForm,
		XmNwidth, SCROLLW_WIDTH,
		XmNresizable, False,
                XmNscrollingPolicy, XmAPPLICATION_DEFINED,
                XmNvisualPolicy, XmVARIABLE,
                XmNscrollBarDisplayPolicy, XmSTATIC,
                XmNshadowThickness, 0,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, ctxt->c_title,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, ctxt->c_listButtonForm,
		XmNleftOffset, OFFSET,
                   XmNrightAttachment, XmATTACH_POSITION,
		    XmNrightPosition, 5,
		NULL );
	ctxt->c_appList = XtVaCreateManagedWidget( "appList",
                xmListWidgetClass,
                ctxt->c_scrollWin,
                XmNvisibleItemCount, 12,
                XmNlistSizePolicy, XmCONSTANT,
                XmNresizable, False,
		XmNtraversalOn, False,
                NULL);
	XtAddCallback(ctxt->c_appList, XmNbrowseSelectionCallback,
		(XtCallbackProc) selectionCB,
                (XtPointer) ctxt );


	buttonForm = build_button_form(ctxt);
	arrowForm = build_arrow_form(ctxt, buttonForm, NULL);
#if 0
        titleForm = build_title_justification_form(ctxt, arrowForm, NULL);
        widthForm = build_width_form(ctxt, titleForm, NULL);
#else
        widthForm = build_width_form(ctxt, arrowForm, NULL);
#endif

	action_items[0].label = catgets(catd, 1, 8, "OK");
        action_items[1].label = catgets(catd, 1, 9, "Apply");
        action_items[2].label = catgets(catd, 1, 10, "Cancel");
        action_items[3].label = catgets(catd, 1, 11, "Help");

        ctxt->c_buttonBox = CreateActionArea(ctxt->c_workForm, action_items, 4);

	XtVaSetValues(widthForm,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, ctxt->c_buttonBox,
		XmNbottomOffset, OFFSET,
		NULL );
	XtVaSetValues(ctxt->c_scrollWin,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, ctxt->c_buttonBox,
		XmNbottomOffset, OFFSET,
		NULL );
	XtVaSetValues(ctxt->c_hidescrollWin,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, ctxt->c_buttonBox,
		XmNbottomOffset, OFFSET,
		NULL );
	XtVaSetValues(ctxt->c_listButtonForm,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, ctxt->c_buttonBox,
		XmNbottomOffset, OFFSET,
		NULL );

	XtManageChild(ctxt->c_buttonBox);
	XtManageChild(ctxt->c_scrollWin);
	XtManageChild(ctxt->c_hidescrollWin);

/*
	XtAddCallback( shell, XmNdestroyCallback,
		(XtCallbackProc) exitCB,
		(XtPointer) ctxt);
*/

	configContext = ctxt;
	return (shell);
}

void
show_configDialog(Widget parent) 
{
	if (configDialog == NULL)
		configDialog = build_config_dialog(launchermain);

/*
	saveNumColumns = chosenNumColumns;
*/
	redisplayPalette = False;
	set_edit_functions(configContext, False, False, -1);
	display_appList(configContext->c_hideappList, HIDE);
	display_appList(configContext->c_appList, SHOW);
	XtManageChild(configContext->c_workForm);
	XtPopup(configDialog, XtGrabNone);
}


