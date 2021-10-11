/*
 * Copyright (c) 1993 Sun Microsystems, Inc.  All Rights Reserved.
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

#pragma	ident	"@(#)xm_form.c 1.16 96/08/06"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <stropts.h>
#include <sys/conf.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <Xm/Xm.h>
#include <Xm/Form.h>
#include <Xm/List.h>
#include <Xm/RowColumn.h>
#include <Xm/DialogS.h>
#include <Xm/MenuShell.h>
#include <Xm/MessageB.h>
#include <Xm/SelectioB.h>
#include <Xm/LabelG.h>
#include <Xm/ToggleBG.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/Protocols.h>
#include <Xm/AtomMgr.h>
#include "xm_defs.h"
#include "sysid_msgs.h"

#ifdef XEDITRES
#include <X11/Xmu/Editres.h>
#endif

/*
 * This file contains the implementation of generic forms for
 * the Motif GUI.  Each screen of information is implemented
 * as a popup dialog with a rowColumn form as its Work Area.
 * The first widget in a generic form is either a label, if there
 * is background text associated with the form, or the first field
 * of the form.  If a label, it is followed by the first field.
 *
 * Each field in the form is in turn composed of a simple form
 * (Field Form) containing a label gadget and a widget appropriate
 * for the type of the form.  Generally, this type will be one
 * of a textField (most common), a menu, or a radio box.
 *
 * A field that is handled specially is the form's "summary
 * field" -- a field that is intended to reflect the union of
 * the values of the form's other fields.  When sensitive text
 * fields are updated (losingFocusCallback), the validation
 * routine associated with the summary field is called and upon
 * return the summary field is redisplayed.  Currently the only
 * use for this field is in the "Date and Time" screen, for
 * displaying the concatenation of the date and time's
 * individual components.
 *
 * Association of user data with widgets:
 *
 *	Dialog:		the [rowColumn] form constituting the work area
 *	Work Area:	a pointer to the XmFields describing the form
 *	Field Form:	the initial value of the field (for Reset)
 *	Radio Box:	the initially set ToggleButton (for Reset)
 *	Text Field:	a pointer to the Field_desc describing the field
 *
 * Callback client data:
 *
 *	Ok, Yes, No buttons:
 *			the fd where replies get sent (reply_to)
 *	Text fields:	the XmField describing the form's "summary field"
 */

#ifdef USE_XPG4_WCS
/*
 * If strwidth() is not adopted in CDE/s495, convert x to wide char
 * string and use wcswidth(x).
 */
#define	MBWIDTH(x)		strwidth(x)
#else
#define	MBWIDTH(x)		eucscol(x)
#endif

static void	popup_pending(Widget, XtPointer, XtPointer);
static void	form_close(Widget, XtPointer, XtPointer);
static void	update_text(Widget, XtPointer, XtPointer);
static void	set_labels_from_locale(Widget, XtPointer, XtPointer);

static Widget	xm_label(Widget, const char *, char *);
static Widget	xm_subform(Widget, Widget, const char *, void *);

static Widget	xm_add_text(Widget, Widget, Field_desc *, XmField *);
static Widget	xm_add_confirm(Widget, Widget, Field_desc *, XmField *);
static Widget	xm_add_choice(Widget, Widget, Field_desc *, XmField *);

static Widget locale_dialog = NULL;

/*
 * structure for passing client data so that we have enough
 * information, i.e. fields data, to set up process traversal on
 * 'pending' dialogs when they are popped up.
 */
typedef struct _Popup_info {
	Widget	pending;
	Field_desc	*fields;
	int	nfields;
} Popup_info;

void
form_intro(Widget pending, Field_desc *fields, int nfields,
	char *title, char *text, Field_help *help)
{
	static int	first_prompt = 1;
	static Field_desc intro[] = {
	    { FIELD_NONE, NULL, NULL, NULL, NULL, -1, -1, -1, -1, 0, NULL }
	};
	Widget	dialog;
	Widget	parade_intro;
	Popup_info *popup_info;

	if (first_prompt) {
		XmString	ok_str =
			XmStringCreateLocalized(CONTINUE_BUTTON);
		XmString	help_str =
			XmStringCreateLocalized(HELP_BUTTON);

		first_prompt = 0;

		intro[0].help = help;
		/*
		 * Create tool intro screen
		 */
		dialog = form_create(
			toplevel, title, ok_str, (XmString)0, help_str);
		form_common(dialog, text, intro, 1);

		popup_info = (Popup_info *) malloc(sizeof (Popup_info));
		popup_info->pending = pending;
		popup_info->fields = fields;
		popup_info->nfields = nfields;
		XtAddCallback(dialog, XmNokCallback,
					popup_pending, (XtPointer)popup_info);
		XtAddCallback(dialog, XmNhelpCallback,
					xm_help, (XtPointer)help);

		if (unlink(PARADE_INTRO_FILE) == 0) {
			/*
			 * Create "parade" intro screen
			 */
			parade_intro = form_create(toplevel, PARADE_INTRO_TITLE,
					ok_str, (XmString)0, help_str);
			form_common(parade_intro, PARADE_INTRO_TEXT, intro, 1);

			popup_info =
				(Popup_info *) malloc(sizeof (Popup_info));
			popup_info->pending = dialog;
			popup_info->fields = NULL;
			popup_info->nfields = 0;
			XtAddCallback(parade_intro, XmNokCallback,
					popup_pending, (XtPointer)popup_info);
			XtAddCallback(parade_intro, XmNhelpCallback,
					xm_help, (XtPointer)help);
			dialog = parade_intro;
		}
	} else
		dialog = pending;

	XtManageChild(dialog);
	XtPopup(XtParent(dialog), XtGrabNone);

	if (!first_prompt)
		xm_set_traversal(dialog, fields, nfields);
}

/*ARGSUSED*/
static void
popup_pending(Widget	widget,
	XtPointer	client_data,
	XtPointer	call_data)
{
	Popup_info	* popup_info = (Popup_info *) client_data;

	form_destroy(widget);

	XtManageChild(popup_info->pending);
	XtPopup(XtParent(popup_info->pending), XtGrabNone);
	xm_set_traversal(popup_info->pending, popup_info->fields,
		popup_info->nfields);

	XtFree(popup_info);
}

void
form_common(
	Widget		dialog,
	char		*text,
	Field_desc	*fields,
	int		nfields)
{
	Widget		main_form;
	Widget		data_form;
	XmField		*first_field;
	XmField		*last_field;
	XmField		*sum_field;
	XmField		*xmf;
	Widget		text_w;
	Widget		last_w;
	int		i;
	Dimension	height, max_h;
	Dimension	width, max_w;
	short		columns, max_c;
	/*
	 * Make panel help text widgets
	 * non-selectable so they behave
	 * like labels.
	 */
	String		translations =		/* for XmText */
		"~Ctrl ~Meta<BtnDown>:\n"
		"~Ctrl ~Meta<BtnUp>:";

	XtVaGetValues(dialog, XmNuserData, &main_form, NULL);

	if (text != (char *)0) {
		text_w = XtVaCreateManagedWidget(
		    "dialogText", xmTextWidgetClass, main_form,
			XmNautoShowCursorPosition,	False,
			XmNcursorPositionVisible,	False,
			XmNscrollVertical,		False,
			XmNeditMode,			XmMULTI_LINE_EDIT,
			XmNeditable,			False,
			XmNtraversalOn,			False,
			XmNresizeHeight,		True,
			XmNcolumns,			XM_DEFAULT_COLUMNS,
			XmNvalue,			text,
			XmNwordWrap,			True,
			XmNshadowThickness,		0,
			XmNtopAttachment,		XmATTACH_FORM,
			XmNleftAttachment,		XmATTACH_FORM,
			XmNrightAttachment,		XmATTACH_FORM,
			NULL);
		XtOverrideTranslations(text_w,
				XtParseTranslationTable(translations));
	} else
		text_w = (Widget)0;

	data_form = XtVaCreateManagedWidget(
	    "dialogData", xmFormWidgetClass, main_form,
		XmNresizePolicy,	XmRESIZE_ANY,
		XmNresizable,		True,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		text_w,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNbottomAttachment,	XmATTACH_FORM,
		NULL);

	max_h = 0;
	max_w = 0;
	max_c = 0;

	first_field = (XmField *)0;
	last_field = (XmField *)0;
	sum_field = (XmField *)0;

	last_w = text_w;

	for (i = 0; i < nfields; i++) {
		Field_desc *f = &fields[i];

		xmf = (XmField *)xmalloc(sizeof (XmField));
		xmf->xf_label = NULL;
		xmf->xf_value = NULL;
		xmf->xf_desc = f;
		xmf->xf_next = (XmField *)0;

		switch (f->type) {
		case FIELD_TEXT:
			last_w = xm_add_text(data_form, last_w, f, xmf);
			break;
		case FIELD_EXCLUSIVE_CHOICE:
			last_w = xm_add_choice(data_form, last_w, f, xmf);
			break;
		case FIELD_CONFIRM:
			last_w = xm_add_confirm(data_form, last_w, f, xmf);
			break;
		}

		if (first_field == (XmField *)0) {
			first_field = last_field = xmf;
		} else {
			last_field->xf_next = xmf;
			last_field = xmf;
		}

		if (xmf->xf_label) {
			XtVaGetValues(xmf->xf_label, XmNwidth,	&width, NULL);
			if (width > max_w)
				max_w = width;
		}

		if (xmf->xf_value) {
			XtVaGetValues(xmf->xf_value, XmNheight,	&height, NULL);
			if (height > max_h)
				max_h = height;
			if ((f->type == FIELD_TEXT) &&
			    ((f->flags & FF_RDONLY) == 0)) {
				XtVaGetValues(xmf->xf_value,
					XmNcolumns,	&columns,
					NULL);
				if (columns > max_c)
					max_c = columns;
			}
		}
		if (f->flags & FF_SUMMARY)
			sum_field = xmf;
	}

	if (last_w != (Widget)0 && nfields != -1) {
		if (last_field == (XmField *)0 ||
		    (last_field->xf_desc->type == FIELD_EXCLUSIVE_CHOICE &&
		    (last_field->xf_desc->flags & FF_RDONLY) == 0))
			XtVaSetValues(last_w,
				XmNbottomAttachment,	XmATTACH_FORM,
				NULL);
	}

	for (xmf = first_field; xmf != (XmField *)0; xmf = xmf->xf_next) {
		if (xmf->xf_desc->type != FIELD_EXCLUSIVE_CHOICE ||
		    (xmf->xf_desc->flags & FF_RDONLY)) {
			if (xmf->xf_label)
				XtVaSetValues(xmf->xf_label,
					XmNheight,	max_h,
					XmNwidth,	max_w,
					NULL);
			if (xmf->xf_value)
				XtVaSetValues(xmf->xf_value,
					XmNheight,	max_h,
					NULL);
		}
		/*
		 * All editable text fields on a form
		 * are made the same size and have a
		 * callback routine to update the
		 * summary field.
		 */
		if (xmf->xf_desc->type == FIELD_TEXT &&
		    ((xmf->xf_desc->flags & FF_RDONLY) == 0)) {
			XtVaSetValues(xmf->xf_value, XmNcolumns, max_c, NULL);
			XtAddCallback(xmf->xf_value,
				XmNlosingFocusCallback,
				update_text, (XtPointer)sum_field);
		}

	}
	XtVaSetValues(main_form, XmNuserData, data_form, NULL);
	XtVaSetValues(data_form, XmNuserData, first_field, NULL);
}

/*
 * Deny window-manager generated requests
 * to close down a form
 */
/*ARGSUSED*/
static void
form_close(Widget	widget,
	XtPointer	client_data,
	XtPointer	call_data)
{
	Widget	dialog = (Widget)client_data;

	xm_popup_error(dialog, ERROR_TITLE, DISMISS_ERROR_TEXT);
}

Widget
form_create(
	Widget		parent,
	char		*title,
	XmString	ok_label,
	XmString	cancel_label,
	XmString	help_label)
{
	Widget	dialog;
	Widget	button;
	Widget	form;
	Widget	shell;
	Arg	args[5];	/* XXX */
	int	i;
	Atom	wmdelete;

	i = 0;
	XtSetArg(args[i], XmNautoUnmanage, False);			i++;
	XtSetArg(args[i], XmNdeleteResponse, XmDO_NOTHING);		i++;
	if (ok_label) {
		XtSetArg(args[i], XmNokLabelString, ok_label);
		i++;
	}
	if (cancel_label) {
		XtSetArg(args[i], XmNcancelLabelString, cancel_label);
		i++;
	}
	if (help_label) {
		XtSetArg(args[i], XmNhelpLabelString, help_label);
		i++;
	}
	dialog = XmCreateTemplateDialog(parent, "dialog", args, i);
	locale_dialog = dialog;

	form = XtVaCreateManagedWidget(
		"dialogForm", xmFormWidgetClass, dialog,
			XmNresizePolicy,	XmRESIZE_ANY,
			NULL);

	XtVaSetValues(dialog,
		XmNuserData,		form,
		XmNresizePolicy,	XmRESIZE_ANY,
		NULL);

	XtVaSetValues(XtParent(dialog),
		XmNtitle,		title,
		XmNallowShellResize,	True,
		NULL);

	if (ok_label == (XmString)0) {
		button = XmMessageBoxGetChild(dialog, XmDIALOG_OK_BUTTON);
		if (button != (Widget)0)
			XtUnmanageChild(button);
	}

	if (cancel_label == (XmString)0) {
		button = XmMessageBoxGetChild(dialog, XmDIALOG_CANCEL_BUTTON);
		if (button != (Widget)0)
			XtUnmanageChild(button);
	}

	if (help_label == (XmString)0) {
		button = XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON);
		if (button != (Widget)0)
			XtUnmanageChild(button);
	}

	shell = XtParent(dialog);
	wmdelete = XmInternAtom(XtDisplay(parent), "WM_DELETE_WINDOW", False);
	XmAddWMProtocolCallback(shell, wmdelete, form_close,
		(XtPointer) dialog);

#ifdef XEDITRES
	XtAddEventHandler(
	    shell, (EventMask)0, True, _XEditResCheckMessages, NULL);
#endif

	return (dialog);
}

/*
 * Update summary field, if present
 */
void
update_summary(XmField *sum)
{
	XmString	t;
	int ret;

	if (sum != (XmField *)0 &&
	    sum->xf_desc->validate != (Validate_proc *)0) {
		ret = (*sum->xf_desc->validate)(sum->xf_desc);
		if (ret == SYSID_SUCCESS) {
			t = XmStringCreateLocalized(sum->xf_desc->value);
			XtVaSetValues(sum->xf_value, XmNlabelString, t, NULL);
			XmStringFree(t);
		} else {
			xm_popup_error(xm_get_shell(sum->xf_value),
				VALIDATION_ERROR_TITLE,
				get_err_string(ret, 0, NULL));
		}
	}
}

/*
 * Installed as losing focus callback routine
 * for sensitive text widgets.  Responsible for
 * updating the value in the field descriptor
 * structure and updating the summary field for
 * the form, if one is present.
 */
/*ARGSUSED*/
static void
update_text(Widget	widget,
	XtPointer	client_data,
	XtPointer	call_data)
{
	/* LINTED [alignment ok] */
	XmField	*xmf_sum = (XmField *)client_data;
	Field_desc	*f;
	char		*new;

	XtVaGetValues(widget,
		XmNuserData,	&f,
		XmNvalue,	&new,
		NULL);

	/*
	 * If we have a new value and we either
	 * didn't have an old value or the new
	 * value is different than the old, save
	 * the new value and update the summary
	 * field.
	 */
	if ((new != (char *)0) &&
	    (f->value == (char *)0 || strcmp(f->value, new) != 0)) {
		f->value = new;
		update_summary(xmf_sum);
	}
}

static Widget
xm_label(Widget parent, const char *name, char *label)
{
	XmString	t;
	Widget		w;


	if ((int)MBWIDTH(label) > XM_DEFAULT_COLUMNS)
		t = xm_format_text(label, XM_DEFAULT_COLUMNS, 1);
	else
		t = XmStringCreateLocalized(label);

	w = XtVaCreateManagedWidget(
	    name, xmLabelGadgetClass, parent,
		XmNlabelString,		t,
		XmNalignment,		XmALIGNMENT_END,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_FORM,
		NULL);

	XmStringFree(t);

	return (w);
}

static Widget
xm_subform(Widget parent, Widget last, const char *name, void *user_data)
{
	Widget		form;

	form = XtVaCreateManagedWidget(name, xmFormWidgetClass, parent,
		XmNhorizontalSpacing,	10,
		XmNresizePolicy,	XmRESIZE_ANY,
		XmNresizable,		True,
		XmNuserData,		user_data,
		XmNleftAttachment,	XmATTACH_FORM,
		NULL);

	if (last == (Widget)0)
		XtVaSetValues(form,
			XmNtopAttachment,	XmATTACH_FORM,
			NULL);
	else
		XtVaSetValues(form,
			XmNtopAttachment,	XmATTACH_WIDGET,
			XmNtopWidget,		last,
			XmNtopOffset,		0,
			NULL);

	return (form);
}

static Widget
xm_add_text(Widget parent, Widget last, Field_desc *f, XmField *xmf)
{
	Widget		form;
	XmString	t;

	form = xm_subform(parent, last, "textForm", f->value);

	xmf->xf_label = xm_label(form, "textLabel", f->label);

	if ((f->flags & FF_RDONLY) == 0) {
		/*
		 * Name the text field widget with something unique (the label
		 * name works good) so that we can do an XtNameToWidget on it
		 * later (like for setting XmProcessTraversal ).
		 */
		xmf->xf_value = XtVaCreateManagedWidget(
		    f->label ? f->label : "textField",
		    xmTextFieldWidgetClass, form,
			XmNvalue,		f->value,
			XmNcursorPosition,
				f->value ? strlen(f->value) : 0,
			XmNuserData,		f,
			XmNcolumns,		f->field_length != -1 ?
							f->field_length : 20,
			XmNmaxLength,		f->value_length != -1 ?
							f->value_length : 80,
			XmNleftAttachment,	XmATTACH_WIDGET,
			XmNleftWidget,		xmf->xf_label,
			XmNtopAttachment,	XmATTACH_FORM,
			XmNbottomAttachment,	XmATTACH_FORM,
			NULL);
	} else {
		t = XmStringCreateLocalized(f->value);
		xmf->xf_value = XtVaCreateManagedWidget(
		    "textValue", xmLabelGadgetClass, form,
			XmNalignment,		XmALIGNMENT_BEGINNING,
			XmNlabelString,		t,
			XmNuserData,		f,
			XmNleftAttachment,	XmATTACH_WIDGET,
			XmNleftWidget,		xmf->xf_label,
			XmNtopAttachment,	XmATTACH_FORM,
			XmNbottomAttachment,	XmATTACH_FORM,
			NULL);
		XmStringFree(t);
	}

	return (form);
}

static Widget
xm_add_choice(Widget parent, Widget last, Field_desc *f, XmField *xmf)
{
	Menu		*menu = (Menu *)f->value;
	Widget		form;
	XmString	*list_items;
	XmString	t;
	Arg		args[20];
	int		n;
	int		i;

	if (menu->selected == -1)
		menu->selected = 0;

	form = xm_subform(parent, last, "choiceForm", f->value);

	xmf->xf_label = xm_label(form, "choiceLabel", f->label);

	if (f->flags & FF_RDONLY) {

		t = XmStringCreateLocalized(menu->labels[menu->selected]);

		xmf->xf_value = XtVaCreateManagedWidget(
		    "choiceValue", xmLabelGadgetClass, form,
			XmNlabelString,		t,
			XmNuserData,		f,
			XmNleftAttachment,	XmATTACH_WIDGET,
			XmNleftWidget,		xmf->xf_label,
			XmNtopAttachment,	XmATTACH_FORM,
			XmNbottomAttachment,	XmATTACH_FORM,
			NULL);
		XmStringFree(t);

		return (form);
	}

	XtVaSetValues(form, XmNrightAttachment, XmATTACH_FORM, NULL);

	if (f->flags & FF_FORCE_SCROLLABLE || menu->nitems > XM_MAX_ITEMS) {

		f->flags |= FF_FORCE_SCROLLABLE;	/* XXX */

		list_items = xm_create_list(menu, menu->nitems);

		n = 0;
		XtSetArg(args[n],
		    XmNlistSizePolicy, XmRESIZE_IF_POSSIBLE);		n++;
		XtSetArg(args[n],
		    XmNscrollBarDisplayPolicy, XmAS_NEEDED);		n++;
		XtSetArg(args[n], XmNselectionPolicy, XmBROWSE_SELECT);	n++;
		XtSetArg(args[n], XmNvisibleItemCount, XM_MAX_ITEMS);	n++;
		XtSetArg(args[n], XmNuserData, f);			n++;
		XtSetArg(args[n], XmNitemCount, menu->nitems);		n++;
		XtSetArg(args[n], XmNitems, list_items);		n++;
		XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET);	n++;
		XtSetArg(args[n], XmNtopWidget, xmf->xf_label);		n++;
		XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);	n++;
		XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM);	n++;
		XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM);	n++;

		xmf->xf_value = XmCreateScrolledList(
					form, "choiceList", args, n);

		if (menu->selected >= 0)
			XmListSelectPos(
				xmf->xf_value, menu->selected + 1, False);

		XtAddCallback(xmf->xf_value, XmNdestroyCallback,
					xm_destroy_list, (XtPointer)list_items);

		/*
		 * setup the callback for the list of locales - the callback
		 * that'll be called will dynamically change the labels and
		 * title of the locale window based on the locale chosen.
		 */
		if ((Sysid_attr)f->user_data == ATTR_LOCALE) {
			XtAddCallback(xmf->xf_value, XmNdefaultActionCallback,
					set_labels_from_locale, NULL);
			XtAddCallback(xmf->xf_value, XmNbrowseSelectionCallback,
					set_labels_from_locale, NULL);
		}

		XtManageChild(xmf->xf_value);

	} else {
		xmf->xf_value = XtVaCreateManagedWidget(
		    "choiceRadio", xmRowColumnWidgetClass, form,
			XmNrowColumnType,	XmWORK_AREA,
			XmNorientation,		XmVERTICAL,
			XmNradioAlwaysOne,	True,
			XmNradioBehavior,	True,
			XmNpacking,		XmPACK_COLUMN,
			XmNtopAttachment,	XmATTACH_FORM,
			XmNleftAttachment,	XmATTACH_WIDGET,
			XmNleftWidget,		xmf->xf_label,
			XmNbottomAttachment,	XmATTACH_FORM,
			NULL);

		for (i = 0; i < menu->nitems; i++) {
			char	*cp = menu->labels[i];
			Widget	button;

			t = XmStringCreateLocalized(cp);
			button = XtVaCreateManagedWidget(
			    "choiceButton",
			    xmToggleButtonGadgetClass,
			    xmf->xf_value,
				XmNlabelString,		t,
				XmNuserData,		i,
				XmNset,			i == menu->selected,
				NULL);
			XmStringFree(t);

			if (i == menu->selected) {
				XtVaSetValues(xmf->xf_value,
				    XmNmenuHistory,	button,
				    XmNuserData,	button,
				    NULL);
			}
		}
	}

	return (form);
}

static Widget
xm_add_confirm(Widget parent, Widget last, Field_desc *f, XmField *xmf)
{
	static XmString	yes;
	static XmString	no;
	Widget		form;
	int		confirm = (int)f->value;

	if (yes == (XmString)0) {
		yes = XmStringCreateLocalized(YES);
		no = XmStringCreateLocalized(NO);
	}

	form = xm_subform(parent, last, "confirmForm", f->value);

	xmf->xf_label = xm_label(form, "choiceLabel", f->label);

	if ((f->flags & FF_RDONLY) == 0) {
		Widget	yes_button;
		Widget	no_button;
		Widget	button;

		xmf->xf_value = XtVaCreateManagedWidget(
		    "confirmRadio", xmRowColumnWidgetClass, form,
			XmNrowColumnType,	XmWORK_AREA,
			XmNorientation,		XmVERTICAL,
			XmNradioAlwaysOne,	True,
			XmNradioBehavior,	True,
			XmNpacking,		XmPACK_COLUMN,
			XmNtopAttachment,	XmATTACH_FORM,
			XmNleftAttachment,	XmATTACH_WIDGET,
			XmNleftWidget,		xmf->xf_label,
			XmNbottomAttachment,	XmATTACH_FORM,
			NULL);

		yes_button = XtVaCreateManagedWidget(
		    "confirmYes", xmToggleButtonGadgetClass, xmf->xf_value,
			XmNlabelString,		yes,
			XmNuserData,		TRUE,
			NULL);

		no_button = XtVaCreateManagedWidget(
		    "confirmNo", xmToggleButtonGadgetClass, xmf->xf_value,
			XmNlabelString,		no,
			XmNuserData,		FALSE,
			NULL);

		if (confirm == TRUE)
			button = yes_button;
		else
			button = no_button;

		XmToggleButtonGadgetSetState(button, True, True);
		XtVaSetValues(xmf->xf_value, XmNuserData, button, NULL);
	} else {
		xmf->xf_value = XtVaCreateManagedWidget(
		    "confirmValue", xmLabelGadgetClass, form,
			XmNalignment,		XmALIGNMENT_BEGINNING,
			XmNlabelString,		confirm == TRUE ?
								    yes : no,
			XmNleftAttachment,	XmATTACH_WIDGET,
			XmNleftWidget,		xmf->xf_label,
			XmNtopAttachment,	XmATTACH_FORM,
			XmNbottomAttachment,	XmATTACH_FORM,
			NULL);
	}

	return (form);
}

void
form_destroy(Widget dialog)
{
	XmField *fields, *xmf, *next;
	Widget	main_form;
	Widget	data_form;

	XtVaGetValues(dialog, XmNuserData, &main_form, NULL);
	if (main_form != (Widget)0) {
		XtVaGetValues(main_form, XmNuserData, &data_form, NULL);
		if (data_form != (Widget)0) {
			XtVaGetValues(data_form, XmNuserData, &fields, NULL);
		} else
			fields = (XmField *)0;
	} else
		fields = (XmField *)0;

#ifdef lint
	next = fields;
#endif
	for (xmf = fields; xmf != (XmField *)0; xmf = next) {
		next = xmf->xf_next;
		free(xmf);
	}
	XtPopdown(XtParent(dialog));
	XtDestroyWidget(dialog);
}

/*
 * set_labels_from_locale is called whenever an item is chosen from
 * the list of locales. the purpose of this function is to dynamically
 * change the labels and title of locale window based on the locale
 * chosen. this is kind of a hack. putting this in really does break
 * the design of the UI, since form & function are now dependent for
 * this window. the usefullness of this functionality, though, is
 * such that I'm willing to make a special case.
 */
/*ARGSUSED*/
static void
set_labels_from_locale(Widget	widget,
	XtPointer		client_data,
	XtPointer		call_data)
{
	Field_desc *f;

	XtVaGetValues(widget,
		XmNuserData, &f,
		NULL);

	if (f != NULL) {
		Menu *menu;
		menu = (Menu *)f->value;

		if (menu != NULL) {
			/*
			 * set locale to the selected list item, and
			 * redraw all labels under this locale
			 */
			int *pos;
			int npos;
			char *locale;
			XmString xms;
			Widget label;

			/*
			 * get the selected item from the list of locales
			 */
			if (!XmListGetSelectedPos(widget, &pos, &npos))
				return;
			locale = ((char **)(menu->values))[pos[0] - 1];
			XtFree(pos);

			/*
			 * set the locale to the currently selected
			 * locale from the list
			 */
			(void) setlocale(LC_MESSAGES, locale);

			/*
			 * localize the dialog title
			 */
			XtVaSetValues(XtParent(locale_dialog),
				XmNtitle, LOCALE_TEXT,
				NULL);

			/*
			 * topWidget contains the label for the
			 * scrolled list
			 */
			XtVaGetValues(XtParent(widget),
				XmNtopWidget, &label,
				NULL);

			/*
			 * localize the scrolled list label
			 */
			xms = XmStringCreateLocalized(f->label);
			XtVaSetValues(label,
				XmNlabelString, xms,
				NULL);
			XmStringFree(xms);

			/*
			 * localize the continue button
			 */
			xms = XmStringCreateLocalized(CONTINUE_BUTTON);
			XtVaSetValues(locale_dialog,
				XmNokLabelString, xms,
				NULL);
			XmStringFree(xms);
		}
	}
}
