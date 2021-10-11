/*
 * Copyright (c) 1991,1992,1993 Sun Microsystems, Inc.  All Rights Reserved.
 * Sun considers its source code as an unpublished, proprietary trade secret,
 * and it is available only under strict license provisions.  This copyright
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

#pragma	ident	"@(#)xm_timezone.c 1.14 96/06/25"

/*
 *	File:		xm_timezone.c
 *
 *	Description:	This file contains the routines needed to prompt
 *			the user the system timezone.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tzfile.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <Xm/Xm.h>
#include <Xm/Form.h>
#include <Xm/ToggleBG.h>
#include <Xm/PushBG.h>
#include <Xm/LabelG.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/List.h>
#include <Xm/Scale.h>
#include <Xm/MessageB.h>
#include <Xm/SeparatoG.h>
#include <Xm/FileSB.h>
#include "xm_defs.h"
#include "sysid_msgs.h"

#define	TZ_FROM_REGION	0
#define	TZ_FROM_OFFSET	1
#define	TZ_FROM_FILE	2

/*
 * This structure is a wrapper around the Menu
 * structure used elsewhere.  In the Motif
 * GUI, we need an array of XmStrings in
 * order to populate the Timezone list.
 */
typedef struct zone Zone;
struct zone {
	XmString	*z_strings;
	Menu		*z_menu;
	int		z_size;
};

/*
 * Can't be perfect.  Oh well...
 */
static Widget	tz_region;
static Widget	tz_offset;
static Widget	tz_file;
static Widget	tz_file_select;

/*
 * "region_position" is used to hold the current region list position.
 * Granted, the use of global variables is always frowned upon. However,
 * the code has not been architected to provide any method to get the
 * current state of the region list. Storing the widget id of this
 * list is not acceptable, since this list gets created and destroyed
 * each time the timezone window is opened and closed.
 */
static int	region_position;

static Validate_proc	xm_region_valid;

static Widget	create_region_subwin(Widget, Field_desc *, int);
static Widget	create_offset_subwin(Widget, Field_desc *, int);
static Widget	create_file_subwin(Widget, Field_desc *, int);

static int	get_tz_method(Widget);
static Field_desc *	get_tzfile_field_desc(Widget);

static void	reply_tz(Widget, XtPointer, XtPointer);
static void	popup_subwin(Widget, XtPointer, XtPointer);
static void	popdown_subwin(Widget, XtPointer, XtPointer);
static void	popup_rulesfile(Widget, XtPointer, XtPointer);
static void	destroy_rulesfile(Widget, XtPointer, XtPointer);
static void	set_rulesfile(Widget, XtPointer, XtPointer);

static void	change_regions(Widget, XtPointer, XtPointer);

static Zone	*create_zones(Menu *, int);
static void	destroy_zones(Widget, XtPointer, XtPointer);
static void	change_zones(Widget, XtPointer, XtPointer);

static int
xm_region_valid(Field_desc *f)
{
	Zone	*zp = (Zone *)f->value;

	if (zp == (Zone *)0 || zp->z_menu->selected < 0)
		return (SYSID_ERR_NO_VALUE);

	if (zp->z_menu->selected > zp->z_size)
		/* this is a "can't happen" */
		return (SYSID_ERR_MAX_VALUE_EXCEEDED);

	return (SYSID_SUCCESS);
}

/*ARGSUSED*/
void
do_get_timezone(
	char		*timezone,
	Field_desc	*regions,
	Field_desc	*gmt_offset,
	Field_desc	*tz_filename,
	int		reply_to)
{
	static Field_help intro_help;
	static Field_help form_help;
	static int	init;
	static XmString	ok;
	static XmString	help;

	static Field_desc method_field = {
		FIELD_EXCLUSIVE_CHOICE, NULL, NULL, NULL, NULL,
		-1, -1, -1, -1, FF_LAB_ALIGN | FF_LAB_LJUST | FF_VALREQ,
		NULL
	};

	Widget		dialog;

	if (!init) {
		static char	*methods[3];
		static Menu	menu;

		ok = XmStringCreateLocalized(SET_BUTTON);
		help = XmStringCreateLocalized(HELP_BUTTON);

		methods[0] = TZ_BY_REGION;
		methods[1] = TZ_BY_GMT;
		methods[2] = TZ_BY_FILE;

		menu.labels = methods;
		menu.values = (void *)0;
		menu.nitems = 3;
		menu.selected = TZ_FROM_REGION;

		method_field.help = dl_get_attr_help(ATTR_TIMEZONE, &form_help);
		method_field.label = TIMEZONE_PROMPT;
		method_field.value = (void *)&menu;

		init++;
	}
	xm_destroy_working();	/* period of inactivity is over */

	regions->validate = xm_region_valid;

	dialog = form_create(toplevel, TIMEZONE_TITLE, ok, (XmString)0, help);

	form_common(dialog, TIMEZONE_TEXT, &method_field, 1);

	tz_region = create_region_subwin(dialog, regions, reply_to);
	tz_offset = create_offset_subwin(dialog, gmt_offset, reply_to);
	tz_file = create_file_subwin(dialog, tz_filename, reply_to);

	XtAddCallback(dialog, XmNokCallback, popup_subwin, (XtPointer)dialog);
	XtAddCallback(dialog, XmNhelpCallback,
			xm_help, (XtPointer)get_attr_help(
				(Sysid_attr)regions->user_data, &form_help));

	if (is_install_environment()) {
		/*
		 * Don't need to pass fields, nfields here because the
		 * timezone has popup windows and the traversal stuff is
		 * handled in there instead.
		 */
		form_intro(dialog, NULL, 0, INTRO_TITLE, INTRO_TEXT,
					get_attr_help(ATTR_NONE, &intro_help));
	} else {
		XtManageChild(dialog);
		XtPopup(XtParent(dialog), XtGrabNone);
	}

}

static int
get_tz_method(Widget dialog)
{
	Widget		main_form;
	Widget		data_form;
	Widget		button;
	XmField		*method_field;
	Menu		*menu;

	XtVaGetValues(dialog, XmNuserData, &main_form, NULL);
	XtVaGetValues(main_form, XmNuserData, &data_form, NULL);
	XtVaGetValues(data_form, XmNuserData, &method_field, NULL);

	menu = (Menu *)method_field->xf_desc->value;

	XtVaGetValues(method_field->xf_value, XmNmenuHistory, &button, NULL);
	if (button != (Widget)0)
		XtVaGetValues(button, XmNuserData, &menu->selected, NULL);
	else
		menu->selected = TZ_FROM_REGION;

	return (menu->selected);
}

/*
 * Get the Field_desc structure for the time zone filenames out of the
 * user data.
 */
static Field_desc *
get_tzfile_field_desc(Widget dialog)
{

	Widget		main_form;
	Widget		data_form;
	Widget		value;
	Field_desc	*fields;

	XtVaGetValues(dialog, XmNuserData, &main_form, NULL);
	XtVaGetValues(main_form, XmNuserData, &data_form, NULL);
	XtVaGetValues(data_form, XmNuserData, &value, NULL);
	XtVaGetValues(value, XmNuserData, &fields, NULL);

	return (fields);
}

/*ARGSUSED*/
static void
reply_tz(
	Widget		widget,		/* dialog for sub-window */
	XtPointer	client_data,	/* parent dialog */
	XtPointer	call_data)
{
	Field_desc	*f;
	Widget		parent = (Widget)client_data;
	Widget		value;
	Widget		main_form;
	Widget		data_form;
	MSG		*mp;
	Zone		*zp;
	Menu		*tz_menu;
	char		*old_value;
	char		timezone[MAX_TZ+1];
	int		tz_pick = -1;
	int		reply_to;
	int		method;
	int		n;

	method = get_tz_method(parent);

	XtVaGetValues(XmMessageBoxGetChild(widget, XmDIALOG_OK_BUTTON),
			XmNuserData,		&reply_to,
			NULL);

	XtVaGetValues(widget, XmNuserData, &main_form, NULL);
	XtVaGetValues(main_form, XmNuserData, &data_form, NULL);

	switch (method) {
	case TZ_FROM_REGION:
	default:	/* I don't think "default" can ever happen... */
		XtVaGetValues(data_form, XmNuserData, &f, NULL);
		if (xm_validate_value(widget, f) != SYSID_SUCCESS)
			return;
		zp = (Zone *)f->value;
		tz_menu = zp->z_menu;
		(void) strcpy(timezone,
				((char **)tz_menu->values)[tz_menu->selected]);
		n = region_position;
		break;
	case TZ_FROM_OFFSET:
		XtVaGetValues(data_form, XmNuserData, &value, NULL);
		XtVaGetValues(value, XmNuserData, &f, XmNvalue, &n, NULL);
		(void) sprintf(f->value, "%d", n);
		(void) strcpy(timezone, tz_from_offset((char *)f->value));
		n = -1;
		break;
	case TZ_FROM_FILE:
		XtVaGetValues(data_form, XmNuserData, &value, NULL);
		XtVaGetValues(value, XmNuserData, &f, NULL);
		old_value = (char *)f->value;
		XtVaGetValues(value, XmNvalue, &f->value, NULL);
		if (xm_validate_value(widget, f) != SYSID_SUCCESS) {
			f->value = (void *)old_value;
			return;
		}
		(void) strcpy(timezone, (char *)f->value);
		n = -1;
		break;
	}
	XtPopdown(XtParent(widget));
	XtPopdown(XtParent(parent));

	XtDestroyWidget(parent);

	mp = msg_new();
	(void) msg_set_type(mp, REPLY_OK);
	(void) msg_add_arg(mp, ATTR_TIMEZONE, VAL_STRING,
				(void *)timezone, MAX_TZ+1);
	(void) msg_add_arg(mp, ATTR_TZ_REGION, VAL_INTEGER,
				(void *)&n, sizeof (n));
	(void) msg_add_arg(mp, ATTR_TZ_INDEX, VAL_INTEGER,
				(void *)&tz_pick, sizeof (tz_pick));
	(void) msg_send(mp, reply_to);
	msg_delete(mp);
}

/*ARGSUSED*/
static void
popup_subwin(Widget	widget,		/* selection method dialog */
	XtPointer	client_data,
	XtPointer	call_data)
{
	Widget	parent = (Widget)client_data;
	int	method = get_tz_method(widget);

	switch (method) {
	case TZ_FROM_REGION:
	default:
		XtManageChild(tz_region);
		XtPopup(XtParent(tz_region), XtGrabNone);
		break;
	case TZ_FROM_OFFSET:
		XtManageChild(tz_offset);
		XtPopup(XtParent(tz_offset), XtGrabNone);
		break;
	case TZ_FROM_FILE:
		XtManageChild(tz_file);
		XtPopup(XtParent(tz_file), XtGrabNone);
		xm_set_traversal(tz_file, get_tzfile_field_desc(tz_file), 1);
		break;
	}
	xm_busy(parent);
}

/*ARGSUSED*/
static void
popdown_subwin(Widget	widget,
	XtPointer	client_data,
	XtPointer	call_data)
{
	Widget	parent = (Widget)client_data;

	if (tz_file_select != (Widget)0)
		XtPopdown(XtParent(tz_file_select));
	XtPopdown(XtParent(widget));
	xm_unbusy(parent);
}

static Widget
create_region_subwin(
	Widget		parent,
	Field_desc	*f,
	int		reply_to)
{
	static		XmString ok;
	static		XmString cancel;
	static		XmString help;

	static		Field_help form_help;

	Menu		*region_menu;
	Widget		region_dialog;
	Widget		main_form;
	Widget		data_form;
	Widget		rg_label, zn_label;
	Widget		rg_list, zn_list;
	XmString	*rg_xms;	/* list of regions (compound strings) */
	Zone		*rg_zones;	/* list of timezones by region */
	XmString	t;
	Arg		args[20];
	int		nregions;
	int		n;

	if (ok == (XmString)0) {
		ok = XmStringCreateLocalized(CONTINUE_BUTTON);
		cancel = XmStringCreateLocalized(CANCEL_BUTTON);
		help = XmStringCreateLocalized(HELP_BUTTON);
	}

	region_menu = (Menu *)f->value;
	/*
	 * XXX
	 *
	 * This code has knowledge of the contents of the region
	 * list.  The tty interface handled TZ specification as
	 * offset from GMT or rules file via sub-menus.  This
	 * interface handles it via a radio box and thus these
	 * two menu items must be removed.  IMPORTANT:  These
	 * two items are assumed to be the last two in the menu!
	 */
	nregions = region_menu->nitems - 2;	/* gmt, file name */

	rg_xms = xm_create_list(region_menu, nregions);
	rg_zones = create_zones(region_menu, nregions);

	region_dialog = form_create(parent, TZ_REGION_TITLE, ok, cancel, help);

	XtVaSetValues(XmMessageBoxGetChild(region_dialog, XmDIALOG_OK_BUTTON),
			XmNuserData,		reply_to,
			NULL);

	XtAddCallback(region_dialog, XmNdestroyCallback,
					xm_destroy_list, (XtPointer)rg_xms);
	XtAddCallback(region_dialog, XmNdestroyCallback,
					destroy_zones, (XtPointer)rg_zones);

	XtAddCallback(region_dialog, XmNokCallback,
				reply_tz, (XtPointer)parent);
	XtAddCallback(region_dialog, XmNcancelCallback,
				popdown_subwin, (XtPointer)parent);
	XtAddCallback(region_dialog, XmNhelpCallback,
			xm_help, (XtPointer)get_attr_help(
				(Sysid_attr)f->user_data, &form_help));

	form_common(region_dialog, TZ_REGION_TEXT, (Field_desc *)0, -1);

	XtVaGetValues(region_dialog, XmNuserData, &main_form, NULL);
	XtVaGetValues(main_form, XmNuserData, &data_form, NULL);

	XtVaSetValues(data_form,
			XmNfractionBase,	100,
			XmNuserData,		f,
			NULL);

	t = XmStringCreateLocalized(TZ_REGION_PROMPT);
	rg_label = XtVaCreateManagedWidget(
		"regionLabel1", xmLabelGadgetClass, data_form,
			XmNalignment,		XmALIGNMENT_BEGINNING,
			XmNlabelString,		t,
			XmNtopAttachment,	XmATTACH_FORM,
			XmNleftAttachment,	XmATTACH_FORM,
			NULL);
	XmStringFree(t);

	n = 0;
	XtSetArg(args[n], XmNlistSizePolicy, XmRESIZE_IF_POSSIBLE);	n++;
	XtSetArg(args[n], XmNscrollBarDisplayPolicy, XmAS_NEEDED);	n++;
	XtSetArg(args[n], XmNselectionPolicy, XmBROWSE_SELECT);		n++;
	XtSetArg(args[n], XmNvisibleItemCount, 10);			n++;
	XtSetArg(args[n], XmNuserData, rg_zones);			n++;
	XtSetArg(args[n], XmNitemCount, nregions);			n++;
	XtSetArg(args[n], XmNitems, rg_xms);				n++;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET);		n++;
	XtSetArg(args[n], XmNtopWidget, rg_label);			n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION);	n++;
	XtSetArg(args[n], XmNrightPosition, 48);			n++;
	XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM);		n++;

	rg_list = XmCreateScrolledList(data_form, "region_list", args, n);

	t = XmStringCreateLocalized(TZ_INDEX_PROMPT);
	zn_label = XtVaCreateManagedWidget(
		"regionLabel2", xmLabelGadgetClass, data_form,
			XmNalignment,		XmALIGNMENT_BEGINNING,
			XmNlabelString,		t,
			XmNtopAttachment,	XmATTACH_FORM,
			XmNleftAttachment,	XmATTACH_POSITION,
			XmNleftPosition,	52,
			NULL);
	XmStringFree(t);

	n = 0;
	XtSetArg(args[n], XmNlistSizePolicy, XmRESIZE_IF_POSSIBLE);	n++;
	XtSetArg(args[n], XmNscrollBarDisplayPolicy, XmAS_NEEDED);	n++;
	XtSetArg(args[n], XmNselectionPolicy, XmBROWSE_SELECT);		n++;
	XtSetArg(args[n], XmNvisibleItemCount, 10);			n++;
	XtSetArg(args[n], XmNuserData, f);				n++;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET);		n++;
	XtSetArg(args[n], XmNtopWidget, zn_label);			n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION);	n++;
	XtSetArg(args[n], XmNleftPosition, 52);				n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM);		n++;

	zn_list = XmCreateScrolledList(data_form, "zoneList", args, n);

	XtAddCallback(rg_list, XmNbrowseSelectionCallback,
				change_regions, (XtPointer)zn_list);
	XtAddCallback(zn_list, XmNbrowseSelectionCallback,
				change_zones, (XtPointer)0);

	if (region_menu->selected >= 0)
		XmListSelectPos(rg_list, region_menu->selected + 1, True);

	/* initialize region_position to the currently selected list position */
	region_position = region_menu->selected;

	XtManageChild(rg_list);
	XtManageChild(zn_list);

	return (region_dialog);
}

static Widget
create_offset_subwin(
	Widget		parent,
	Field_desc	*f,
	int		reply_to)
{
	static		char offset[MAX_GMT_OFFSET+1];
	static		XmString ok;
	static		XmString cancel;
	static		XmString help;

	static		Field_help form_help;

	Widget		offset_dialog;
	Widget		main_form;
	Widget		data_form;
	Widget		offset_label;
	Widget		offset_value;
	XmString	t;

	if (ok == (XmString)0) {
		ok = XmStringCreateLocalized(CONTINUE_BUTTON);
		cancel = XmStringCreateLocalized(CANCEL_BUTTON);
		help = XmStringCreateLocalized(HELP_BUTTON);
	}

	f->flags |= FF_VALREQ;
	f->value = offset;

	offset_dialog = form_create(parent, TZ_GMT_TITLE, ok, cancel, help);

	XtVaSetValues(XmMessageBoxGetChild(offset_dialog, XmDIALOG_OK_BUTTON),
			XmNuserData,		reply_to,
			NULL);

	XtAddCallback(offset_dialog, XmNokCallback,
				reply_tz, (XtPointer)parent);
	XtAddCallback(offset_dialog, XmNcancelCallback,
				popdown_subwin, (XtPointer)parent);
	XtAddCallback(offset_dialog, XmNhelpCallback,
			xm_help, (XtPointer)get_attr_help(
				(Sysid_attr)f->user_data, &form_help));

	form_common(offset_dialog, TZ_GMT_TEXT, (Field_desc *)0, -1);

	XtVaGetValues(offset_dialog, XmNuserData, &main_form, NULL);
	XtVaGetValues(main_form, XmNuserData, &data_form, NULL);

	XtVaSetValues(data_form, XmNuserData, f, NULL);

	t = XmStringCreateLocalized(f->label);
	offset_label = XtVaCreateManagedWidget(
		"offsetLabel", xmLabelGadgetClass, data_form,
			XmNalignment,		XmALIGNMENT_BEGINNING,
			XmNlabelString,		t,
			XmNleftAttachment,	XmATTACH_FORM,
			XmNtopAttachment,	XmATTACH_FORM,
			NULL);
	XmStringFree(t);

	offset_value = XtVaCreateManagedWidget(
		"offsetScale", xmScaleWidgetClass, data_form,
			XmNminimum,		-12,
			XmNmaximum,		13,
			XmNscaleMultiple,	1,
			XmNshowValue,		True,
			XmNvalue,		f->value ? atoi(f->value) : 0,
			XmNuserData,		f,
			XmNorientation,		XmHORIZONTAL,
			XmNleftAttachment,	XmATTACH_WIDGET,
			XmNleftWidget,		offset_label,
			XmNleftOffset,		10,
			XmNtopAttachment,	XmATTACH_OPPOSITE_WIDGET,
			XmNtopWidget,		offset_label,
			XmNrightAttachment,	XmATTACH_FORM,
			NULL);

	XtVaSetValues(data_form, XmNuserData, offset_value, NULL);

	return (offset_dialog);
}

static Widget
create_file_subwin(
	Widget		parent,
	Field_desc	*f,
	int		reply_to)
{
	static		char file[MAXPATHLEN+1];
	static		XmString ok;
	static		XmString cancel;
	static		XmString help;

	static		Field_help form_help;

	Widget		file_dialog;
	Widget		main_form;
	Widget		data_form;
	Widget		file_label;
	Widget		file_name;
	Widget		file_select;
	XmString	t;
	Dimension	height, max_h;

	if (ok == (XmString)0) {
		ok = XmStringCreateLocalized(CONTINUE_BUTTON);
		cancel = XmStringCreateLocalized(CANCEL_BUTTON);
		help = XmStringCreateLocalized(HELP_BUTTON);
	}

	f->flags |= FF_VALREQ;
	f->value = file;

	file_dialog = form_create(parent, TZ_FILE_TITLE, ok, cancel, help);

	XtVaSetValues(XmMessageBoxGetChild(file_dialog, XmDIALOG_OK_BUTTON),
			XmNuserData,		reply_to,
			NULL);

	XtAddCallback(file_dialog, XmNokCallback,
				reply_tz, (XtPointer)parent);
	XtAddCallback(file_dialog, XmNcancelCallback,
				popdown_subwin, (XtPointer)parent);
	XtAddCallback(file_dialog, XmNhelpCallback,
			xm_help, (XtPointer)get_attr_help(
				(Sysid_attr)f->user_data, &form_help));

	form_common(file_dialog, TZ_FILE_TEXT, (Field_desc *)0, -1);

	XtVaGetValues(file_dialog, XmNuserData, &main_form, NULL);
	XtVaGetValues(main_form, XmNuserData, &data_form, NULL);

	XtVaSetValues(data_form, XmNuserData, f, NULL);

	t = XmStringCreateLocalized(f->label);
	file_label = XtVaCreateManagedWidget(
		"fileLabel", xmLabelGadgetClass, data_form,
			XmNalignment,		XmALIGNMENT_END,
			XmNlabelString,		t,
			XmNleftAttachment,	XmATTACH_FORM,
			XmNtopAttachment,	XmATTACH_FORM,
			NULL);
	XmStringFree(t);

	file_select = XtVaCreateManagedWidget(
		"fileSelect", xmPushButtonGadgetClass, data_form,
			XmNlabelString,
					XmStringCreateLocalized(SELECT_BUTTON),
			XmNtopAttachment,	XmATTACH_OPPOSITE_WIDGET,
			XmNtopWidget,		file_label,
			XmNrightAttachment,	XmATTACH_FORM,
			NULL);

	/*
	 * Name the text field widget with something unique (the label
	 * name works good) so that we can do an XtNameToWidget on it
	 * later (like for setting XmProcessTraversal).
	 */
	file_name = XtVaCreateManagedWidget(
		f->label ? f->label : "fileName",
		xmTextFieldWidgetClass, data_form,
			XmNvalue,		f->value ? f->value : "",
			XmNuserData,		f,
			XmNcolumns,		20,
			XmNmaxLength,		f->value_length != -1 ?
							f->value_length : 80,
			XmNtopAttachment,	XmATTACH_OPPOSITE_WIDGET,
			XmNtopWidget,		file_label,
			XmNleftAttachment,	XmATTACH_WIDGET,
			XmNleftWidget,		file_label,
			XmNleftOffset,		10,
			XmNrightAttachment,	XmATTACH_WIDGET,
			XmNrightWidget,		file_select,
			XmNrightOffset,		10,
			NULL);

	XtVaGetValues(file_label, XmNheight, &height, NULL);
	max_h = height;

	XtVaGetValues(file_name, XmNheight, &height, NULL);
	if (height > max_h)
		max_h = height;

	XtVaGetValues(file_select, XmNheight, &height, NULL);
	if (height > max_h)
		max_h = height;

	XtVaSetValues(file_label, XmNheight, max_h, NULL);
	XtVaSetValues(file_name, XmNheight, max_h, NULL);
	XtVaSetValues(file_select, XmNheight, max_h, NULL);

	XtVaSetValues(data_form, XmNuserData, file_name, NULL);

	XtAddCallback(file_select, XmNactivateCallback,
				popup_rulesfile, (XtPointer)file_name);
	return (file_dialog);
}

/*ARGSUSED*/
static void
popup_rulesfile(
	Widget		widget,		/* Select... button */
	XtPointer	client_data,
	XtPointer	call_data)
{
	Widget	text = (Widget)client_data;

	if (tz_file_select == (Widget)0) {
		tz_file_select = XmCreateFileSelectionDialog(
			XtParent(widget), "fileSelector", NULL, 0);
		XtVaSetValues(tz_file_select,
			XmNfilterLabelString,
				XmStringCreateLocalized(FILTER_LABEL),
			XmNdirListLabelString,
				XmStringCreateLocalized(DIR_LABEL),
			XmNfileListLabelString,
				XmStringCreateLocalized(FILES_LABEL),
			XmNselectionLabelString,
				XmStringCreateLocalized(SELECTION_LABEL),
			XmNokLabelString,
				XmStringCreateLocalized(OK_BUTTON),
			XmNapplyLabelString,
				XmStringCreateLocalized(FILTER_BUTTON),
			XmNcancelLabelString,
				XmStringCreateLocalized(CANCEL_BUTTON),
			XmNhelpLabelString,
				XmStringCreateLocalized(HELP_BUTTON),
			XmNdialogTitle,	XmStringCreateLocalized(TZ_FILE_TITLE),
			XmNdirectory,	XmStringCreateLocalized(TZDIR),
			NULL);
		XtAddCallback(tz_file_select, XmNokCallback,
				set_rulesfile, (XtPointer)0);
		XtAddCallback(tz_file_select, XmNcancelCallback,
				XtUnmanageChild, (XtPointer)0);
		XtAddCallback(tz_file_select, XmNdestroyCallback,
				destroy_rulesfile, (XtPointer)0);
	}

	XtVaSetValues(tz_file_select,
		XmNdirectory,		XmStringCreateLocalized(TZDIR),
		XmNuserData,		text,
		NULL);

	XtManageChild(tz_file_select);
	XtPopup(XtParent(tz_file_select), XtGrabNone);
}

/*ARGSUSED*/
static void
destroy_rulesfile(
	Widget		widget,
	XtPointer	client_data,
	XtPointer	call_data)
{
	tz_file_select = (Widget)0;
}

/*ARGSUSED*/
static void
set_rulesfile(
	Widget		widget,		/* selection dialog */
	XtPointer	client_data,
	XtPointer	call_data)
{
	XmFileSelectionBoxCallbackStruct *cbs =
			/*LINTED [alignment ok]*/
			(XmFileSelectionBoxCallbackStruct *)call_data;
	Widget	file_text;
	char	*text, *cp;

	XtVaGetValues(widget, XmNuserData, &file_text, NULL);
	if (!XmStringGetLtoR(cbs->value, XmSTRING_DEFAULT_CHARSET, &text)) {
		Widget	dialog_text;

		dialog_text = XmFileSelectionBoxGetChild(widget, XmDIALOG_TEXT);
		XtVaGetValues(dialog_text, XmNvalue, &text, NULL);
	}

	if (strstr(text, TZDIR))
		cp = &text[strlen(TZDIR) + 1];
	else
		/*
		 * If we get here TZDIR isn't
		 * in the specified path, so
		 * the user will probably get
		 * a validation error
		 */
		cp = text;

	XtVaSetValues(file_text, XmNvalue, cp, NULL);

	XtUnmanageChild(widget);
	XtPopdown(XtParent(widget));
}

/*
 * Selection callback for Region list
 *	The region list user data points to
 *	the array of timezone choices.
 */
static void
change_regions(Widget	widget,		/* region list */
	XtPointer	client_data,	/* zone list */
	XtPointer	call_data)
{
	static int	pos;
	/* LINTED [alignment ok] */
	XmListCallbackStruct *cbs = (XmListCallbackStruct *)call_data;
	Widget zn_list = (Widget)client_data;
	Field_desc	*f;
	Zone	*zones, *zp;
	Menu	*tz_menu;

	XtVaGetValues(widget, XmNuserData, &zones, NULL);

	pos = cbs->item_position;

	/* set region_position to the new selected list position */
	region_position = pos - 1;

	zp = &zones[pos - 1];
	tz_menu = zp->z_menu;

	XtVaGetValues(zn_list, XmNuserData, &f, NULL);
	f->value = (void *)zp;

	XmListDeleteAllItems(zn_list);
	XmListAddItems(zn_list, zp->z_strings, zp->z_size, 0);

	if (tz_menu->selected >= 0 && tz_menu->selected < zp->z_size)
		XmListSelectPos(zn_list, tz_menu->selected + 1, True);
}

static Zone *
create_zones(Menu *region_menu, int nregions)
{
	Zone	*zones, *zp;
	Menu	**menus = (Menu **)region_menu->values;
	Menu	*tz_menu;
	int	i, j;

	zones = (Zone *)xmalloc(sizeof (Zone) * (nregions + 1));
	for (i = 0; i < nregions; i++) {
		zp = &zones[i];
		tz_menu = menus[i];

		zp->z_strings = (XmString *)
				xmalloc(sizeof (XmString) * tz_menu->nitems);
		zp->z_menu = tz_menu;
		zp->z_size = 0;

		for (j = 0; j < tz_menu->nitems; j++) {
			if (((char **)tz_menu->values)[j] != (char *)0) {
				zp->z_strings[j] =
				    XmStringCreateLocalized(tz_menu->labels[j]);
				zp->z_size++;
			}
		}
	}
	zones[nregions].z_menu = (Menu *)0;	/* flag end of array */
	return (zones);
}

/*ARGSUSED*/
static void
destroy_zones(Widget	widget,
	XtPointer	client_data,
	XtPointer	call_data)
{
	/* LINTED [alignment ok] */
	Zone	*zones = (Zone *)client_data;
	Zone	*zp;
	int	i;

	for (zp = zones; zp->z_menu != (Menu *)0; zp++) {
		for (i = 0; i < zp->z_size; i++)
			XmStringFree(zp->z_strings[i]);
		free(zp->z_strings);
	}
	free(zones);
}

/*
 * Selection callback for Timezone list
 *	The timezone list user data points to
 *	the region field descriptor.
 */
/*ARGSUSED*/
static void
change_zones(Widget	widget,
	XtPointer	client_data,
	XtPointer	call_data)
{
	/* LINTED [alignment ok] */
	XmListCallbackStruct *cbs = (XmListCallbackStruct *)call_data;
	Field_desc	*f;
	Zone		*zp;

	XtVaGetValues(widget, XmNuserData, &f, NULL);
	zp = (Zone *)f->value;
	zp->z_menu->selected = cbs->item_position - 1;
}
