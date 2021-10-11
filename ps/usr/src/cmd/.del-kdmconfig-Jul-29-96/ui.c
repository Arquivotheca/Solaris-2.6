/*
 * Copyright (c) 1995, by Sun Microsystems, Inc.
 * All rights reserved.
 *
 * ui.c: User interface control functions
 *
 * Description:
 *  This file provides the routines which build and manage the user
 *  interactions of the kdmconfig program. These routines are invoked
 *  by the controlling routine if any or all of the windows config
 *  elements cannot be automatically discovered.
 *
 * The following exported routines are found in this file
 *
 *  void get_display_interactive(void)
 *  void get_keyboard_interactive(void)
 *  void get_pointer_interactive(void)
 *  int get_config_confirm(int)
 *
 * All external interfaces to these routines can be found in ui.h
 * See that file for a complete interface description of every public
 * routine as well.
 *
 */

#pragma ident "@(#)ui.c 1.34 95/11/22 SMI"

#include <stdio.h>
#include <ctype.h>
#include <locale.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/param.h>
#include "windvc.h"
#include "exists.h"
#include "ui.h"
#include "ui_objects.h"
#include "kdmconfig_msgs.h"
#include "tty_utils.h"
#include "tty_help.h"
#include "tty_form.h"

#if __ppc
#include <sys/kbio.h>
#include <sys/kbd.h>
#include <stropts.h>
#include <fcntl.h>
#include <errno.h>
#endif


static int	get_attributes(char *, int);
static char	**get_value_list(NODE, ATTRIB, int *, int *);
static void	free_value_list(char **);
static Field_desc *add_confirm_device(char *, Field_desc *);

/*
 * This function uses the KIOCLAYOUT ioctl on PPC to obtain the layout.
 * It then searches through the layout string obtained from the at.cfinfo
 * and compares every one of them with the layout obtained from the ioctl.
 * If there is a match it returns the value of the index that is used
 * in the get_attributes function to pick the right locale from the 
 * corresponding string, as the default selection.
 */ 
#if __ppc
static int      get_ppc_layout(val_list_t *);
#endif


/*
 * tells whether a display is an xinside card or not
 */
static int	xin = 0;


/*
 * -------------------------------
 *  Public routines
 * -------------------------------
 */

int
get_display_interactive(int direction)
{
	return (get_attributes(DISPLAY_CAT, direction));
}

int
get_pointer_interactive(int direction)
{
	return (get_attributes(POINTER_CAT, direction));
}

int
get_keyboard_interactive(int direction)
{
	return (get_attributes(KEYBOARD_CAT, direction));
}

int
get_monitor_interactive(int direction)
{
	return (get_attributes(MONITOR_CAT, direction));
}

int
get_xin()
{
	return (xin);
}

int
get_config_confirm(int mask)
{
	NODE		dvi;
	ATTRIB		*alist, *a;
	UIobj		*confirm_obj;
	Field_desc	*fields, *f;
	int		nfields;
	int		ndevs, nattrs;
	int		ch;

	confirm_obj = ui_get_object(CONFIRM_CAT);

	ndevs = 0;
	nattrs = 0;
	nfields = 0;
	/*
	 * How many devices? and for each device,
	 * how many attributes?
	 */
	if (mask & CONFIRM_DIS) {
		ndevs++;
		dvi = get_selected_device(DISPLAY_CAT);
		alist = get_attrib_name_list(dvi);
		for (a = alist; *a; a++)
			nattrs++;
	}
	if (mask & CONFIRM_KBD) {
		ndevs++;
		dvi = get_selected_device(KEYBOARD_CAT);
		alist = get_attrib_name_list(dvi);
		for (a = alist; *a; a++)
			nattrs++;
	}
	if (mask & CONFIRM_PTR) {
		ndevs++;
		dvi = get_selected_device(POINTER_CAT);
		alist = get_attrib_name_list(dvi);
		for (a = alist; *a; a++)
			nattrs++;
	}
	if ((mask & CONFIRM_MON) && get_xin()) {
		ndevs++;
		dvi = get_selected_device(MONITOR_CAT);
		alist = get_attrib_name_list(dvi);
		for (a = alist; *a; a++)
			nattrs++;
	}
	nfields = ndevs + nattrs + 1;	/* +1 for confirm field */

	fields = (Field_desc *)malloc(nfields * sizeof (Field_desc));

	(void) memset((void *)fields, 0, nfields * sizeof (Field_desc));

	f = fields;

	if (mask & CONFIRM_DIS)
		f = add_confirm_device(DISPLAY_CAT, f);
	if (mask & CONFIRM_KBD)
		f = add_confirm_device(KEYBOARD_CAT, f);
	if (mask & CONFIRM_PTR)
		f = add_confirm_device(POINTER_CAT, f);
	if ((mask & CONFIRM_MON) && get_xin()) {
		f = add_confirm_device(MONITOR_CAT, f);
	}
	/*
	 * The last field is a "dummy" one whose
	 * purpose is to catch the function key.
	 */
	f->type = FIELD_NONE;
	f->label = get_object_label(confirm_obj);
	f->flags = FF_CHANGE;
	f->help = get_object_help(confirm_obj);

	ch = form_common(
		get_object_title(confirm_obj),
		get_object_text(confirm_obj),
		fields, nfields, NULL);

	free(fields);

	if (is_continue(ch)) {
		end_curses(1, 1);
		return (CONFIRM_OK);
	} else
		return (CONFIRM_NO);
}

#define	HELPROOT	"/usr/lib/locale"
#define	HELPDIR		"kdmconfig.help"

void
ui_init(void)
{
	static char help_path[MAXPATHLEN];	/* XXX extern char *helpdir; */
	static char lastlocale[32];
	char	*locale;

	locale = setlocale(LC_MESSAGES, (char *)0);
	if (strcmp(locale, lastlocale)) {
		char	*helproot = getenv("KDMCONFIG_HELPROOT");
		/*
		 * Locale has changed or it's the
		 * first time through this code.
		 */
		(void) sprintf(help_path, "%s/%s/help/%s",
			helproot ? helproot : HELPROOT, locale, HELPDIR);
		if (access(help_path, X_OK) < 0)
			(void) sprintf(help_path, "%s/C/help/%s",
				helproot ? helproot : HELPROOT, HELPDIR);
		helpdir = help_path;
		(void) strncpy(lastlocale, locale, sizeof (lastlocale));
		lastlocale[sizeof (lastlocale) - 1] = '\0';
	}
}

void
ui_wintro(void)
{
	form_wintro(get_object_help(ui_get_object(INTRO_CAT)));
}

int
ui_intro(void)
{
	int ch;
	int direction = NAV_FORWARD;
	UIobj		*intro_obj;	/* introductory object */

	intro_obj = ui_get_object(INTRO_CAT);

	ch = form_intro(
		get_object_title(intro_obj),
		get_object_text(intro_obj),
		get_object_help(intro_obj),
		INTRO_ON_DEMAND);

	if (is_bypass(ch))
		direction = NAV_BACKWARD;

	return (direction);
}

void
ui_error(char *text, int do_exit)
{
	char	*newtext;
	size_t	len = strlen(text);
	char	*cp;

	if (!do_exit && curses_on) {
		UIobj	*error_obj = ui_get_object(ERROR_CAT);
		char	*error_text = get_object_text(error_obj);

		newtext = (char *)xmalloc(len + strlen(error_text) + 3);

		(void) strcpy(newtext, text);

		/*
		 * Strip all trailing newlines
		 */
		cp = &newtext[len];	/* *cp == terminating '\0' */
		while (--cp > newtext && *cp == '\n')
			*cp = '\0';

		(void) strcat(newtext, "\n\n");
		(void) strcat(newtext, error_text);

		form_error(
			get_object_title(error_obj),
			newtext,
			get_object_help(error_obj));
	} else {
		if (do_exit)
			end_curses(1, 1);

		newtext = (char *)xmalloc(len + 2);

		(void) strcpy(newtext, text);

		cp = &newtext[len];	/* *cp == terminating '\0' */
		if (cp > newtext && cp[-1] != '\n') {
			*cp++ = '\n';
			*cp = '\0';
		}

		if (do_exit)
			(void) fprintf(stderr, "%s", newtext);
		else
			werror(stdscr, 0, 0, 80, newtext);
	}

	free(newtext);
}

/*
 * -------------------------------
 *  Private routines
 * -------------------------------
 */

/*
 * Used to alphabetize menus yet keep track of
 * the underlying library's index for each item.
 */
typedef struct menu_item MenuItem;
struct menu_item {
	char	*string;
	int	index;
};

static MenuItem	*values_to_items(char **, int);
static char	**items_to_values(MenuItem *, int);
static int	sort_items(const void *, const void *);

static int
get_attributes(char *catname, int direction)	/* also object name */
{
	Field_desc	field;
	Menu		menu;

	NODE		dvi;
	ATTRIB		*alist, *a, *b;
	UIobj		*cat_obj;	/* category object */
	UIobj		*attr_obj;	/* attribute object */
	char		*selection;
	char		*form_label;
	char		**devices, **dp;
	char		**values;
	MenuItem	*items;
	int		ndevices;
	int		ch;
	int		i;
	int 		depth = 0;
	int		btype = 0;
	long		longval;
	char		*text;
	static	char	Depth[] = "__depth__";
#if __ppc
	int 		layout_selected;
	val_list_t	*layouts;
#endif

	cat_obj = ui_get_object(catname);
	if (cat_obj == (UIobj *)0) {
		char	buf[256];

		(void) sprintf(buf, "No %s object!", catname);
		ui_notice(buf);
		return (NAV_FORWARD);
	}

select_device:
	menu.values = (void *)0;


	devices = get_category_list(catname);
	if (devices == (char **)0) {
		char	buf[256];

		(void) sprintf(buf, "No %s devices!", catname);
		ui_notice(buf);
		return (NAV_FORWARD);
	}

	dvi = get_selected_device(catname);


	ndevices = 0;
	for (dp = devices; *dp; dp++) {
		ndevices++;
	}
	menu.nitems = ndevices;

	items = values_to_items(devices, ndevices);
	menu.labels = items_to_values(items, ndevices);

	menu.selected = -1;
	for (i = 0; i < ndevices; i++) {
		if (dvi && strcmp(dvi->title, menu.labels[i]) == 0)
			menu.selected = i;
	}

	if (menu.nitems > 1) {
		(void) memset((void *)&field, 0, sizeof (field));

		field.type = FIELD_EXCLUSIVE_CHOICE;
		field.flags = FF_GOBACK | FF_CANCEL;
		field.validate = (Validate_proc *)NULL;
		field.label = get_object_label(cat_obj);
		field.value = &menu;
		field.help = get_object_help(cat_obj);
		form_label = get_object_title(cat_obj);
		text = get_object_text(cat_obj);
		ch = form_common(form_label,
			get_object_text(cat_obj), &field, 1, NULL);

		if (is_goback(ch))
			return (NAV_BACKWARD);
		else
			direction = NAV_FORWARD;
	} else
		menu.selected = 0;

	dvi = get_device_info(catname, items[menu.selected].index);
	set_selected_device(dvi);

	if (strcmp(catname, DISPLAY_CAT) == 0)
		xin = is_xinside(dvi);

	free(items);
	free(menu.labels);

	if (xin && (strcmp(catname, DISPLAY_CAT) == 0))
		set_attrib_value_typ(dvi, Depth, (VAL)selection);

/* layouts will point to the string of keyboard layouts listed in
 * the at.cfinfo file.
 */
#if __ppc
	layouts = find_attr_val(dvi->typ_alist, "__locale__");	
#endif

	alist = get_attrib_name_list(dvi);
	a = alist;
	while (*a) {
		menu.selected = -1;
		values = get_value_list(dvi, *a, &menu.nitems, &menu.selected);


		if (menu.nitems >= 1) {
			field.type = FIELD_EXCLUSIVE_CHOICE;
			field.value = &menu;
			field.validate = (Validate_proc *)NULL;
			items = values_to_items(values, menu.nitems);
			menu.labels = items_to_values(items, menu.nitems);

			for (i = 0; i < menu.nitems; i++) {

/* The KIOCLAYOUT has given us the layout so we use that instead of 
 * selecting English as the default layout.  Also, we don't want to
 * mess up the default selections for stuff like resolution and depth,
 * which is why we make this specific to layout.
 */
#if __ppc
                             if (strcmp(*a, "__layout__") == 0)
				 menu.selected = get_ppc_layout(layouts);
#endif

				if (menu.labels[i] == values[menu.selected]) {
					menu.selected = i;
					break;
				}
			}

			attr_obj = ui_get_object((const char *)*a);

			field.label = get_object_label(attr_obj);
			field.flags = FF_LAB_ALIGN | FF_LAB_LJUST | FF_GOBACK;
			field.help = get_object_help(attr_obj);
			form_label = get_object_title(attr_obj);
			text = get_object_text(attr_obj);
			ch = form_common(form_label,
				get_object_text(attr_obj), &field, 1, NULL);


			free(items);
			free(menu.labels);
		}

		if (is_goback(ch) ||
		    (menu.nitems < 2 && direction == NAV_BACKWARD)) {
			/*
			 * This is really gross.  Where we go back to
			 * depends on where we are and what preceded us.
			 * If we're past the first item in the attribute
			 * list, we go back to the previous attribute.
			 * If we're at the first attribute and got here
			 * via a device menu (more than 1 device in this
			 * category), we go back to that menu, otherwise
			 * we go back to the previous category.
			 */
			direction = NAV_BACKWARD;

			free_value_list(values);

			if (a > alist)
				--a;		/* back one and iterate */
			else if (ndevices == 1)
				break;		/* out of loop and return */
			else
				goto select_device;	/* to device menu */
		} else {
			direction = NAV_FORWARD;

			selection = menu.labels[menu.selected];

			if (get_attrib_type(dvi, *a) == VAL_STRING) {
				if (strncmp(*a, Depth,
				    sizeof (Depth)) == 0) {
					depth = 1;
				} else {
					depth = 0;
				}
				set_attrib_value(dvi, *a, (VAL)selection);
				if (is_xinside(dvi) && depth) {
					a = get_attrib_name_list(dvi);
					while (*a) {
						if (strcmp(*a, Depth) == 0)
							break;
						a++;
					}

				}
			} else {
				if (selection[0] == '0' &&
				    (selection[1] == 'x' ||
				    selection[1] == 'X')) {
					longval = strtoul(selection,
							(char **)NULL, 16);
					sprintf(selection, "%s", selection);
					set_attrib_value(dvi, *a, (VAL)longval);
				} else
					set_attrib_value(dvi, *a,
						    (VAL)atoi(selection));

			}
			free_value_list(values);

			a++;
		}
	}
	return (direction);
}

#define	CHUNKSIZE	5

static char **
get_value_list(
	NODE		node,
	ATTRIB		name,
	int		*nitems_p,
	int		*selected_p)
{
	char		**list;
	VAL		val, sel;
	size_t		size = CHUNKSIZE * sizeof (char *);
	int		nitems;
	val_t		type = get_attrib_type(node, name);

	sel = get_selected_attrib_value(node, name);



	list = (char **)malloc(size);

	nitems = 0;
	while (((val = get_attrib_value(node, name)) != (VAL)0) &&
	    *(int *)val != 0) {
		if (((nitems + 1) % CHUNKSIZE) == 0) {
			size += (CHUNKSIZE * sizeof (char *));
			list = (char **)realloc(list, size);
		}
		if (type == VAL_NUMERIC) {
			char	buf[32];

			if (*(int *)sel == *(int *)val)
				*selected_p = nitems;
			print_num(buf, *(int *)val, 0);
			list[nitems] = strdup(buf);
			nitems++;
		} else if (type == VAL_UNUMERIC) {
			char    buf[32];

			if (*(int *)sel == *(int *)val)
				*selected_p = nitems;
			print_num(buf, *(unsigned int  *)val, 1);
			list[nitems] = strdup(buf);
			nitems++;
		} else if (type == VAL_STRING) {

			if (strcmp((char *)sel, (char *)val) == 0)
				*selected_p = nitems;

			list[nitems] = strdup((char *)val);
			nitems++;
		}
		if (name != (ATTRIB)0)
			name = (ATTRIB)0;	/* yuck */
	}
	list[nitems] = (char *)0;
	*nitems_p = nitems;

	if (*selected_p < 0)
		*selected_p = 0;

	return (list);
}

static void
free_value_list(char **list)
{
	char	**cp;

	for (cp = list; *cp; cp++)
		free(*cp);
	free(list);
}

static Field_desc *
add_confirm_device(char *catname, Field_desc *f)
{
	NODE		dvi;
	ATTRIB		*alist, *a;
	UIobj		*cat_obj;
	UIobj		*attr_obj;
	Field_desc	*last = f;

	cat_obj = ui_get_object(catname);
	dvi = get_selected_device(catname);

	f->type = FIELD_TEXT;
	f->label = get_object_confirm(cat_obj);
	f->value = dvi->title;
	f->field_length = -1;
	f->value_length = -1;
	f->flags = FF_RDONLY | FF_LAB_RJUST | FF_LAB_ALIGN | FF_GOBACK;
	f++;

	alist = get_attrib_name_list(dvi);
	for (a = alist; *a; a++) {
		val_t	type = get_attrib_type(dvi, *a);

		attr_obj = ui_get_object((const char *)*a);

		f->type = FIELD_TEXT;
		f->label = get_object_confirm(attr_obj);
		if (type == VAL_STRING)
			f->value = get_selected_attrib_value(dvi, *a);
		else {
			char	buf[32];

			if (type == VAL_NUMERIC)
				print_num(buf, *(int *)
				    get_selected_attrib_value(dvi, *a), 0);
			else if (type == VAL_UNUMERIC)
				print_num(buf, *(unsigned int *)
					get_selected_attrib_value(dvi, *a), 1);
			f->value = strdup(buf);
		}
		f->field_length = -1;
		f->value_length = -1;
		f->flags = FF_RDONLY | FF_LAB_RJUST | FF_LAB_ALIGN | FF_GOBACK;

		last = f++;
	}

	last->flags |= FF_ENDGROUP;

	return (f);
}

static int
sort_items(const void *v1, const void *v2)
{
	MenuItem	*item1 = (MenuItem *)v1;
	MenuItem	*item2 = (MenuItem *)v2;

	return (strcoll(item1->string, item2->string));
}

static MenuItem *
values_to_items(char **values, int nvalues)
{
	MenuItem	*items;
	int		i;

	items = (MenuItem *)malloc(sizeof (MenuItem) * nvalues);

	for (i = 0; i < nvalues; i++) {
		items[i].string = values[i];
		items[i].index = i;
	}

	qsort((void *)items, nvalues, sizeof (MenuItem), sort_items);


	return (items);
}

static char **
items_to_values(MenuItem *items, int nvalues)
{
	char	**values;
	int	i;

	values = (char **)malloc(sizeof (char *) * nvalues);

	for (i = 0; i < nvalues; i++) {
		values[i] = items[i].string;
	}

	return (values);
}

/* Please see the explanation for this function in the beginning of the
 * file near its prototype declaration.
 */
#if __ppc
static int
get_ppc_layout(val_list_t *layout_val)
{
        int             fd;
        int             ppc_layout;
        int             layout_selected = 0;
	char 		*layout_ptr;
	int		kbd_layout;

        if ((fd = open("/dev/kbd", O_WRONLY)) == -1)
		verb_msg("%s\n", KDMCONFIG_MSGS(KDMCONFIG_CANTOPEN_KBD));
        if (ioctl(fd, KIOCLAYOUT, &ppc_layout) == -1)
		verb_msg("%s\n", KDMCONFIG_MSGS(KDMCONFIG_CANTOPEN_KBD));
	close(fd);	
	layout_ptr = layout_val->val.string;
	while ((layout_ptr = strchr(layout_ptr, ',')) != NULL)
	{
		kbd_layout = atoi(++layout_ptr);
		if (kbd_layout == ppc_layout)
		 	return (layout_selected);
		layout_selected++;
	}
	
}
#endif __ppc
