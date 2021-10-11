/*
 * Copyright (c) 1991-1996 Sun Microsystems, Inc.  All Rights Reserved.
 */

#pragma	ident	"@(#)xm_locale.c 1.10 96/10/17"

/*
 *	File:		xm_locale.c
 *
 *	Description:	This file contains the routines needed to prompt
 *			the user the system locale.
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
typedef struct lang_locale LangLocale;
struct lang_locale {
	XmString	*ll_strings;
	Menu		*ll_menu;
	int		ll_size;
};

/*
 * Can't be perfect.  Oh well...
 */
static Widget	loc_lang;
int		lang_index;
Widget		lang_label, locale_label;
Widget		lang_dialog;

/*
 * "locale_position" is used to hold the current lang list position.
 * Granted, the use of global variables is always frowned upon. However,
 * the code has not been architected to provide any method to get the
 * current state of the lang list. Storing the widget id of this
 * list is not acceptable, since this list gets created and destroyed
 * each time the locale window is opened and closed.
 */
static int	locale_position;

/*static Validate_proc	xm_lang_valid;
 */

static Widget	create_lang_subwin(Widget, Field_desc *, int);

/*static int	get_tz_method(Widget);
 */

static void	reply_locale(Widget, XtPointer, XtPointer);

static void	change_languages(Widget, XtPointer, XtPointer);
static void	set_labels_from_lang(Widget, XtPointer, XtPointer);

static LangLocale	*create_locales(Menu *, int);
static void	get_lang_display_strings(Menu *, int);
static void	get_locale_display_string(Menu *, int);
static void	destroy_locales(Widget, XtPointer, XtPointer);
static void	change_locales(Widget, XtPointer, XtPointer);

/*static int
 *xm_lang_valid(Field_desc *f)
 *{
 *	LangLocale	*lp = (LangLocale *)f->value;
 *
 *	if (lp == (LangLocale *)0 || lp->ll_menu->selected < 0)
 *		return (SYSID_ERR_NO_VALUE);
 *
 *	if (lp->ll_menu->selected > lp->ll_size)
 *		
 *		return (SYSID_ERR_MAX_VALUE_EXCEEDED);
 *
 *	return (SYSID_SUCCESS);
 *}
 */

/*ARGSUSED*/
void
do_get_locale(
	char		*locale,
	Field_desc	*languages,
	int		reply_to)
{
	xm_destroy_working();	/* period of inactivity is over */

	loc_lang = create_lang_subwin(toplevel, languages, reply_to);
	XtManageChild(loc_lang);

}


/*ARGSUSED*/
static void
reply_locale(
	Widget		widget,		/* dialog for sub-window */
	XtPointer	client_data,	/* parent dialog */
	XtPointer	call_data)
{
	Field_desc	*f;
	Widget		main_form;
	Widget		data_form;
	MSG		*mp;
	LangLocale	*lp;
	Menu		*lmenu;
	char		locale[MAX_LOCALE+1];
	int		loc_pick = -1;
	int		reply_to;
	int		n;


	XtVaGetValues(XmMessageBoxGetChild(widget, XmDIALOG_OK_BUTTON),
			XmNuserData,		&reply_to,
			NULL);

	XtVaGetValues(widget, XmNuserData, &main_form, NULL);
	XtVaGetValues(main_form, XmNuserData, &data_form, NULL);


	XtVaGetValues(data_form, XmNuserData, &f, NULL);
	if (xm_validate_value(widget, f) != SYSID_SUCCESS)
		return;
	lp = (LangLocale *)f->value;
	lmenu = lp->ll_menu;
	(void) strcpy(locale,
			((char **)lmenu->client_data)[lmenu->selected]);
	n = locale_position;

	XtPopdown(XtParent(widget));

	mp = msg_new();
	(void) msg_set_type(mp, REPLY_OK);
	(void) msg_add_arg(mp, ATTR_LOCALEPICK, VAL_STRING,
				(void *)locale, MAX_LOCALE+1);
	(void) msg_add_arg(mp, ATTR_LOC_LANG, VAL_INTEGER,
				(void *)&n, sizeof (n));
	(void) msg_add_arg(mp, ATTR_LOC_INDEX, VAL_INTEGER,
				(void *)&loc_pick, sizeof (loc_pick));
	(void) msg_send(mp, reply_to);
	msg_delete(mp);
}

/*
 * This function creates the motif dialog that contains the
 * scrolled language and locale lists
 */
static Widget
create_lang_subwin(
	Widget		parent,
	Field_desc	*f,
	int		reply_to)
{
	static		XmString ok;

	Menu		*language_menu;
	Widget		main_form;
	Widget		data_form;
	Widget		lang_scroll, locale_scroll;
	XmString	*lang_xms;	/* list of languages */
	LangLocale	*lang_locales;	/* list of locales by lang */
	XmString	t;
	Arg		args[20];
	int		nlanguages;
	int		n;

	if (ok == (XmString)0) {
		ok = XmStringCreateLocalized(CONTINUE_BUTTON);
	}

	language_menu = (Menu *)f->value;

	/*
	 * how many languages are there?
	 */
	nlanguages = language_menu->nitems;

	/*
	 * define the strings that will be displayed
	 * for each language
	 */
	get_lang_display_strings(language_menu, nlanguages);

	/*
	 * create the lists on compund strings for all of the
	 * languages and all the locales
	 */
	lang_xms = xm_create_list(language_menu, nlanguages);
	lang_locales = create_locales(language_menu, nlanguages);

	/*
	 * create the dialog to hold all of the data
	 */
	lang_dialog =
		form_create(parent, LOCALE_TEXT, ok,
					(XmString)0, (XmString)0);

	XtVaSetValues(XmMessageBoxGetChild(lang_dialog, XmDIALOG_OK_BUTTON),
			XmNuserData,		reply_to,
			NULL);

	XtAddCallback(lang_dialog, XmNdestroyCallback,
				xm_destroy_list, (XtPointer)lang_xms);
	XtAddCallback(lang_dialog, XmNdestroyCallback,
				destroy_locales, (XtPointer)lang_locales);

	XtAddCallback(lang_dialog, XmNokCallback,
				reply_locale, (XtPointer)parent);

	form_common(lang_dialog, SELECT_LOCALE_TEXT, (Field_desc *)0, -1);

	XtVaGetValues(lang_dialog, XmNuserData, &main_form, NULL);
	XtVaGetValues(main_form, XmNuserData, &data_form, NULL);

	XtVaSetValues(data_form,
			XmNfractionBase,	100,
			XmNuserData,		f,
			NULL);

	t = XmStringCreateLocalized(LANGUAGE_LABEL);
	lang_label = XtVaCreateManagedWidget(
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
	XtSetArg(args[n], XmNuserData, lang_locales);			n++;
	XtSetArg(args[n], XmNitemCount, nlanguages);			n++;
	XtSetArg(args[n], XmNitems, lang_xms);				n++;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET);		n++;
	XtSetArg(args[n], XmNtopWidget, lang_label);			n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_POSITION);	n++;
	XtSetArg(args[n], XmNrightPosition, 48);			n++;
	XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM);		n++;

	lang_scroll = XmCreateScrolledList(data_form, "region_list", args, n);

	t = XmStringCreateLocalized(LOCALE_LABEL);
	locale_label = XtVaCreateManagedWidget(
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
	XtSetArg(args[n], XmNtopWidget, locale_label);			n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION);	n++;
	XtSetArg(args[n], XmNleftPosition, 52);				n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM);		n++;
	XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM);		n++;

	locale_scroll = XmCreateScrolledList(data_form, "zoneList", args, n);

	XtAddCallback(lang_scroll, XmNbrowseSelectionCallback,
				change_languages, (XtPointer)locale_scroll);
	XtAddCallback(lang_scroll, XmNbrowseSelectionCallback,
				set_labels_from_lang, (XtPointer)language_menu);
	XtAddCallback(locale_scroll, XmNbrowseSelectionCallback,
				change_locales, (XtPointer)0);

	if (language_menu->selected >= 0)
		XmListSelectPos(lang_scroll, language_menu->selected + 1, True);

	/*
	 * make sure the first item (the language) is selected by
	 * default in the locale list
	 */
	XmListSelectPos(locale_scroll, 1, True);

	XtManageChild(lang_scroll);
	XtManageChild(locale_scroll);

	return (lang_dialog);
}

/*
 * this function changes the on screen text and window title
 * and labels to be displayed in the language selected by the
 * user
 */
static void
set_labels_from_lang(Widget widget,
	XtPointer	client_data,
	XtPointer	call_data)
{
	int		*pos;
	int		npos;
	char		*lang;
	XmString	xms;
	Menu		*menu = (Menu *)client_data;

	if (!XmListGetSelectedPos(widget, &pos, &npos))
		return;

	lang_index = pos[0] - 1;
	lang = ((char **) (menu->client_data))[pos[0]-1];
	XtFree(pos);

	/*
	 * set the locale to the currently selected locale from the list
	 */
	 (void) setlocale(LC_MESSAGES, lang);

	 /*
	  * localize the dialog title
	  */
	  XtVaSetValues(XtParent(lang_dialog),
		XmNtitle, LOCALE_TEXT,
		NULL);

	/*
	 * localize the language list label
	 */
	xms = XmStringCreateLocalized(LANGUAGE_LABEL);
	XtVaSetValues(lang_label,
		XmNlabelString, xms,
		NULL);
	XmStringFree(xms);

	/*
	 * localize the locale list label
	 */
	xms = XmStringCreateLocalized(LOCALE_LABEL);
	XtVaSetValues(locale_label,
		XmNlabelString, xms,
		NULL);
	XmStringFree(xms);

	/*
	 * localize the on screen text
	 */
	XtVaSetValues(XtNameToWidget(lang_dialog, "*dialogText"),
		XmNvalue, SELECT_LOCALE_TEXT,
		NULL);

	/*
	 * localize the continue button label
	 */
	xms = XmStringCreateLocalized(CONTINUE_BUTTON);
	XtVaSetValues(lang_dialog,
		XmNokLabelString, xms,
		NULL);
	XmStringFree(xms);

}

/*
 * Selection callback for the Language list
 * The lang list user data points to
 * the array of locale choices.
 */
static void
change_languages(Widget	widget,		/* lang list */
	XtPointer	client_data,	/* locale list */
	XtPointer	call_data)
{
	static int	pos;
	/* LINTED [alignment ok] */
	XmListCallbackStruct *cbs = (XmListCallbackStruct *)call_data;
	Widget locale_scroll = (Widget)client_data;
	Field_desc	*f;
	LangLocale	*locales, *lp;

	XtVaGetValues(widget, XmNuserData, &locales, NULL);

	pos = cbs->item_position;

	lp = &locales[pos - 1];

	XtVaGetValues(locale_scroll, XmNuserData, &f, NULL);
	f->value = (void *)lp;

	XmListDeleteAllItems(locale_scroll);
	XmListAddItems(locale_scroll, lp->ll_strings, lp->ll_size, 0);

	XmListSelectPos(locale_scroll, 1, True);
}

/*
 * this function sets the strings that will be displayed for each
 * language
 */
static void
get_lang_display_strings(Menu *menu,
			int nitems)
{
	int	i;
	int	status;
	char	label_string[256], buff[512], buff2[512];

	for (i = 0; i < nitems; i++) {
		(void) setlocale(LC_MESSAGES, menu->labels[i]);
		status = get_l10n_string(1, menu->labels[i], buff, buff2, 512);
		switch (status) {
		case 1: /* locales without translation */
			(void) sprintf(label_string, "%s %s",
				SELECT_THIS_ITEM, buff2);
			break;
		case 0: /* locales with translated string */
			(void) sprintf(label_string, "%s %s (%s)",
				gettext(SELECT_THIS_ITEM),
				buff, buff2);
			break;
		case -1: /* no locale description */
			(void) sprintf(label_string, "%s %s",
				gettext(SELECT_THIS_ITEM),
				menu->labels[i]);
			break;
		}

		((char **)menu->client_data)[i] = xstrdup(menu->labels[i]);
		menu->labels[i] = xstrdup(label_string);
	}
}

/*
 * this function sets the string that will be displayed for the
 * locale
 */

static void
get_locale_display_string(Menu *menu, int i)
{
	int	status;
	char	label_string[256], buff[512], buff2[512];

	status = get_l10n_string(0, menu->labels[i], buff, buff2, 512);
	switch (status) {
	case 1: /* locales without translation */
		(void) sprintf(label_string, "%s", buff2);
		break;
	case 0: /* translated string */
		(void) sprintf(label_string, "%s (%s)", buff, buff2);
		break;
	case -1: /* no locale description */
		(void) sprintf(label_string, "%s", menu->labels[i]);
		break;
	}

	((char **)menu->client_data)[i] = xstrdup(menu->labels[i]);
	menu->labels[i] = xstrdup(label_string);
}

/*
 * create the list of languages and locales that go with
 * each language
 */
static LangLocale *
create_locales(Menu *language_menu, int nlanguages)
{
	LangLocale	*locales, *lp;
	Menu	**menus = (Menu **)language_menu->values;
	Menu	*lmenu;
	int	i, j;

	locales = (LangLocale *)xmalloc(sizeof (LangLocale) * (nlanguages + 1));
	for (i = 0; i < nlanguages; i++) {
		char* lang_string;

		lp = &locales[i];
		lmenu = menus[i];

		lp->ll_strings = (XmString *)
				xmalloc(sizeof (XmString) * lmenu->nitems);
		lp->ll_menu = lmenu;
		lp->ll_size = 0;

		/* call setlocale with the language string */
		lang_string = ((char **)language_menu->client_data)[i];
		(void) setlocale(LC_MESSAGES, lang_string);

		for (j = 0; j < lmenu->nitems; j++) {
			if (((char **)lmenu->values)[j] != (char *)0) {
				get_locale_display_string(lmenu, j);
				lp->ll_strings[j] =
				    XmStringCreateLocalized(lmenu->labels[j]);
				lp->ll_size++;
			}
		}
	}
	locales[nlanguages].ll_menu = (Menu *)0;	/* flag end of array */
	return (locales);
}

/*ARGSUSED*/
static void
destroy_locales(Widget	widget,
	XtPointer	client_data,
	XtPointer	call_data)
{
	/* LINTED [alignment ok] */
	LangLocale	*locales = (LangLocale *)client_data;
	LangLocale	*lp;
	int	i;

	for (lp = locales; lp->ll_menu != (Menu *)0; lp++) {
		for (i = 0; i < lp->ll_size; i++)
			XmStringFree(lp->ll_strings[i]);
		free(lp->ll_strings);
	}
	free(locales);
}

/*
 * Selection callback for the Locale list
 * The locale list user data points to
 * the lang field descriptor.
 */
/*ARGSUSED*/
static void
change_locales(Widget	widget,
	XtPointer	client_data,
	XtPointer	call_data)
{
	/* LINTED [alignment ok] */
	XmListCallbackStruct *cbs = (XmListCallbackStruct *)call_data;
	Field_desc	*f;
	LangLocale		*lp;

	XtVaGetValues(widget, XmNuserData, &f, NULL);
	lp = (LangLocale *)f->value;
	lp->ll_menu->selected = cbs->item_position - 1;
	locale_position = cbs->item_position - 1;
}
