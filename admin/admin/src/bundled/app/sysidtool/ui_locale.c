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

#pragma	ident	"@(#)ui_locale.c 1.7 94/05/24"

/*
 *	File:		ui_locale.c
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
#include "sysid_ui.h"
#include "sysid_msgs.h"


static int		sort_languages(const void *, const void *);
static void		locale_init(void);
static void		locale_lang_init(Menu *);

char *
locale_from_offset(char *offset)
{
	static char offstr[256];
	int	i;

	(void) strcpy(offstr, "GMT");
	i = atoi(offset);
	if (i != 0) {
		if (i > 0)
			(void) strcat(offstr, "+");
		(void) sprintf(&offstr[strlen(offstr)], "%d", i);
	}
	return (offstr);
}

/*
 * ui_get_locale:
 *
 *	This routine is the client interface routine used for
 *	retrieving the desired locale the installing system will
 *	be configured for.
 *
 *	Because there are over 60 possible time zone values, the user
 *	is first asked to specify a geographic region, and then is asked
 *	to specify a time zone within that region.  Instead of selecting
 * 	a specific region, we allow the user to select to enter an offset
 *	from GMT.
 *
 *	Too bad there isn't a nice logical grouping via use of the filesystem,
 *	that way we could just read the filesystem.  Maybe someday, but for
 *	now, we gotta hardcode the languages/locales.
 *
 *	The locale value returned must be a pathname to the locale file,
 *	relative to /usr/lib/locale/TZ.
 *
 *	Input:  pointer to character buffer in which to place
 *		the name of the locale.
 *
 *		Addresses of two integers holding the default region
 *		and locale menu selections.  These integers are
 *		updated to the user's actual selections.
 */

static Menu **language_values;

static	Menu	languages = {
	{ NULL },			/* labels */
	/*(void *)language_values,*/
	(void *)NULL,
	0,
	NO_INTEGER_VALUE,
	(void *)NULL
};

static	Field_desc	language_menu[] = {
	{ FIELD_EXCLUSIVE_CHOICE, (void *)ATTR_LOCALE, NULL, NULL, NULL,
	    -1, -1, -1, -1, FF_LAB_ALIGN | FF_LAB_LJUST | FF_VALREQ,
	    ui_valid_choice },
};

static void
locale_init(void)
{
	static	int been_here;

	if (been_here)
		return;

	locale_lang_init(&languages);
 
	been_here = 1;
}

typedef struct item Item;
struct item {
	char	*label;
	Menu	*submenu;
};

static int
sort_languages(const void *v1, const void *v2)
{
	Item	*item1 = (Item *)v1;
	Item	*item2 = (Item *)v2;

	return (strcoll(item1->label, item2->label));
}

static void
locale_lang_init(Menu *languages)
{
	Menu	**submenus = (Menu **)languages->values;
	Item	*items;
	int	nitems = languages->nitems;
	int	i;

	items = (Item *)xmalloc(nitems * sizeof (Item));

	for (i = 0; i < nitems; i++)
		items[i].label = languages->labels[i];

	for (i = 0; i < nitems; i++)
		items[i].submenu = submenus[i];

	qsort((void *)items, nitems, sizeof (Item), sort_languages);

	for (i = 0; i < nitems; i++) {
		/*
		 * Transfer sorted values to languages menu.
		 */
		languages->labels[i] = items[i].label;
		submenus[i] = items[i].submenu;

		if (submenus[i] == (Menu *)0)
			continue;

		/*
		 * Set default selection
		 */
		if (strcmp(languages->labels[i], "C") == 0)
			languages->selected = i;
	}
}

void
ui_get_locale(MSG *mp, int reply_to)
{
	static	int been_here = 0;
	static	Field_help help;
	int	lang_pick;
	int	locale_pick;
	int	i, j;
	int	nlangs;
	int	nlocales;
	char	**lang;
	char	ll[MAX_LOCALE+1];
	char	**locale_values;
	Menu	**locale_menu;

	(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
				(void *)&nlangs, sizeof (int));

	languages.labels = (char **)xmalloc(nlangs * sizeof (char *));
	languages.client_data = (char **)xmalloc(nlangs * sizeof (char *));

	language_values = (Menu **) calloc(nlangs,sizeof(Menu*));

	languages.values = (void*)language_values;
	languages.nitems = nlangs;

	lang = (char **) calloc(nlangs,sizeof(char *));
	locale_menu = (Menu **) calloc(nlangs, sizeof(Menu *));

	for (i = 0; i < nlangs; i++) {
		lang[i] = (char *) calloc(MAX_LOCALE+1, sizeof(char*));

		(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
				(void *)lang[i], MAX_LOCALE+1);

		(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
				(void *)&nlocales, sizeof(int));

		locale_values = (char **) calloc(nlocales, sizeof(char *));

		locale_menu[i] = (Menu *) calloc(1, sizeof(Menu));
		locale_menu[i]->client_data = (char **)xmalloc(nlocales * sizeof (char *));

		for (j = 0; j < nlocales; j++) {
			locale_values[j] =
				(char *) calloc(MAX_LOCALE+1, sizeof(char*));

			(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
				(void *)locale_values[j], MAX_LOCALE+1);
		}

		locale_menu[i]->labels = locale_values;
		locale_menu[i]->values = (char **)locale_values;
		locale_menu[i]->nitems = nlocales;
		locale_menu[i]->selected = NO_INTEGER_VALUE;

		language_values[i] = locale_menu[i];

		languages.labels[i] = lang[i];
	}

	locale_init();

	(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
				(void *)ll, MAX_LOCALE+1);
	(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
				(void *)&lang_pick, sizeof (lang_pick));
	(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
				(void *)&locale_pick, sizeof (locale_pick));
	msg_delete(mp);


	/*
	 * if first time through, then use "languages.selected" instead of
	 * using "lang_pick", because locale_lang_init() initializes what
	 * the initial region selection should be (set to United States).
	 * "lang_pick" from thsi point on will hold the correct current
	 * value from that chosen by the user
	 */
	if (!been_here)
		lang_pick = languages.selected; /* set from locale_lang_init */

	been_here = 1;

	for (i = 0; i < languages.nitems; i++) {
		if (locale_menu[i] != (Menu *)0) {
			if (i == lang_pick)
				locale_menu[i]->selected = locale_pick;
			else
				locale_menu[i]->selected = NO_INTEGER_VALUE;
		}
	}
	if (lang_pick != -1)
		languages.selected = lang_pick;
	/* else use default from locale_init */

	language_menu[0].help = dl_get_attr_help(ATTR_LOCALE, &help);
	language_menu[0].label = dl_get_attr_prompt(ATTR_LOCALE);
	language_menu[0].value = (void *)&languages;

	dl_get_locale(ll, language_menu, reply_to);
}
