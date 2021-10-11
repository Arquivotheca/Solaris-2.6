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

#pragma	ident	"@(#)ui_timezone.c 1.7 94/05/24"

/*
 *	File:		ui_timezone.c
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
#include "sysid_ui.h"
#include "sysid_msgs.h"


static boolean_t	tz_valid(char *);
static Validate_proc	ui_valid_tz;
static int		sort_regions(const void *, const void *);
static void		tz_init(void);
static void		tz_region_init(Menu *);

/*
 * Check that a timezone name is valid (i.e. it has an entry in
 * /usr/share/lib/zoneinfo.
 */

static boolean_t
tz_valid(char *tz_name)
{
	char tzpath[MAXPATHLEN+1];
	struct stat stat_buf;

	(void) strcpy(tzpath, TZDIR);
	(void) strcat(tzpath, "/");
	(void) strcat(tzpath, tz_name);
	if (stat(tzpath, &stat_buf) == 0) {
		return (B_TRUE);
	} else {
		return (B_FALSE);
	}
}

char *
tz_from_offset(char *offset)
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

static int
ui_valid_tz(Field_desc *f)
{
	Sysid_err errcode;
	char *tz_name = (char *)f->value;

	if (tz_name == (char *)0 || tz_name[0] == '\0') {
		if (f->flags & FF_VALREQ)
			errcode = SYSID_ERR_NO_VALUE;
		else
			errcode = SYSID_SUCCESS;
	} else if (tz_valid(tz_name) == B_TRUE)
		errcode = SYSID_SUCCESS;
	else
		errcode = SYSID_ERR_BAD_TZ_FILE_NAME;

	return (errcode);
}

/*
 * ui_get_timezone:
 *
 *	This routine is the client interface routine used for
 *	retrieving the desired timezone the installing system will
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
 *	now, we gotta hardcode the regions/timezones.
 *
 *	The timezone value returned must be a pathname to the timezone file,
 *	relative to /usr/lib/locale/TZ.
 *
 *	Input:  pointer to character buffer in which to place
 *		the name of the timezone.
 *
 *		Addresses of two integers holding the default region
 *		and timezone menu selections.  These integers are
 *		updated to the user's actual selections.
 */


/*
 * List all possible timezones within given regions.
 * Note that the value argument for each timezone must be
 * the actual filename path *RELATIVE* to /usr/share/lib/zoneinfo.
 */

static	char	*TZ_africa_values[] = {
	"Egypt",
	"Libya"
};
static	Menu	TZ_africa = {
	{ NULL },			/* labels */
	{ (void *)TZ_africa_values },
	sizeof (TZ_africa_values) / sizeof (char *),
	NO_INTEGER_VALUE
};

static	char	*TZ_western_asia_values[] = {
	"Turkey",
	"W-SU",
	"Iran",
	"Israel",
	"Mideast/Riyadh89"
};
static	Menu	TZ_western_asia = {
	{ NULL },			/* labels */
	{ (void *)TZ_western_asia_values },
	sizeof (TZ_western_asia_values) / sizeof (char *),
	NO_INTEGER_VALUE
};

static	char	*TZ_eastern_asia_values[] = {
	"PRC",
	"ROC",
	"Hongkong",
	"Japan",
	"ROK",
	"Singapore"
};
static	Menu	TZ_eastern_asia = {
	{ NULL },			/* labels */
	{ (void *)TZ_eastern_asia_values },
	sizeof (TZ_eastern_asia_values) / sizeof (char *),
	NO_INTEGER_VALUE
};

static	char	*TZ_australia_newzealand_values[] = {
	"Australia/Tasmania",
	"Australia/Queensland",
	"Australia/North",
	"Australia/West",
	"Australia/South",
	"Australia/Victoria",
	"Australia/NSW",
	"Australia/Broken_Hill",
	"Australia/Yancowinna",
	"Australia/LHI",
	"NZ"
};
static	Menu	TZ_australia_newzealand = {
	{ NULL },			/* labels */
	(void *)TZ_australia_newzealand_values,
	sizeof (TZ_australia_newzealand_values) / sizeof (char *),
	NO_INTEGER_VALUE
};

static	char	*TZ_canada_values[] = {
	"Canada/Newfoundland",
	"Canada/Atlantic",
	"Canada/Eastern",
	"Canada/Central",
	"Canada/East-Saskatchewan",
	"Canada/Mountain",
	"Canada/Pacific",
	"Canada/Yukon"
};
static	Menu	TZ_canada = {
	{ NULL },			/* labels */
	(void *)TZ_canada_values,
	sizeof (TZ_canada_values) / sizeof (char *),
	NO_INTEGER_VALUE
};

static	char	*TZ_europe_values[] = {
	"GB",
	"Eire",
	"Iceland",
	"Poland",
	"WET",
	"MET",
	"EET"
};
static	Menu	TZ_europe = {
	{ NULL },			/* labels */
	(void *)TZ_europe_values,
	sizeof (TZ_europe_values) / sizeof (char *),
	NO_INTEGER_VALUE
};

static	char	*TZ_mexico_values[] = {
	"Mexico/BajaNorte",
	"Mexico/BajaSur",
	"Mexico/General"
};
static	Menu	TZ_mexico = {
	{ NULL },			/* labels */
	(void *)TZ_mexico_values,
	sizeof (TZ_mexico_values) / sizeof (char *),
	NO_INTEGER_VALUE
};

static	char	*TZ_south_america_values[] = {
	"Brazil/East",
	"Brazil/West",
	"Brazil/Acre",
	"Brazil/DeNoronha",
	"Chile/Continental",
	"Chile/EasterIsland"
};
static	Menu	TZ_south_america = {
	{ NULL },			/* labels */
	(void *)TZ_south_america_values,
	sizeof (TZ_south_america_values) / sizeof (char *),
	NO_INTEGER_VALUE
};

static	char	*TZ_united_states_values[] = {
	"US/Eastern",
	"US/Central",
	"US/Mountain",
	"US/Pacific",
	"US/East-Indiana",
	"US/Arizona",
	"US/Michigan",
	"US/Samoa",
	"US/Alaska",
	"US/Aleutian",
	"US/Hawaii"
};
static	Menu	TZ_united_states = {
	{ NULL },			/* labels */
	(void *)TZ_united_states_values,
	sizeof (TZ_united_states_values) / sizeof (char *),
	NO_INTEGER_VALUE
};

static	Menu	*region_values[] = {
	&TZ_africa,
	&TZ_western_asia,
	&TZ_eastern_asia,
	&TZ_australia_newzealand,
	&TZ_canada,
	&TZ_europe,
	&TZ_mexico,
	&TZ_south_america,
	&TZ_united_states,
	NULL,			/* offset from GMT */
	NULL,			/* timezone rules file name */
};
static	Menu	regions = {
	{ NULL },			/* labels */
	(void *)region_values,
	sizeof (region_values) / sizeof (void *),
	NO_INTEGER_VALUE
};

static	Field_desc	region_menu[] = {
	{ FIELD_EXCLUSIVE_CHOICE, (void *)ATTR_TZ_REGION, NULL, NULL, NULL,
	    -1, -1, -1, -1, FF_LAB_ALIGN | FF_LAB_LJUST | FF_VALREQ,
	    ui_valid_choice },
};

static	Field_desc	gmt_offsets[] = {
	{ FIELD_TEXT, (void *)ATTR_TZ_GMT, NULL, NULL, NULL,
	    MAX_GMT_OFFSET, MAX_GMT_OFFSET, -12, 13,
	    FF_LAB_ALIGN | FF_LAB_LJUST | FF_CANCEL | FF_KEYFOCUS,
	    ui_valid_integer },
};

static	Field_desc	tz_filename[] = {
#define	TZ_NAME_SIZE	(MAXPATHLEN - sizeof (TZDIR) - 1)
	{ FIELD_TEXT, (void *)ATTR_TZ_FILE, NULL, NULL, NULL,
	    32, TZ_NAME_SIZE, -1, -1,
	    FF_LAB_ALIGN | FF_LAB_LJUST | FF_CANCEL | FF_KEYFOCUS,
	    ui_valid_tz },
};

static void
tz_init(void)
{
	static	Field_help gmt_help, file_help;
	static	int been_here;
	int	i;

	if (been_here)
		return;

	i = 0;
	TZ_africa.labels = (char **)xmalloc(TZ_africa.nitems * sizeof (char *));
	TZ_africa.labels[i++] = EGYPT;
	TZ_africa.labels[i++] = LIBYA;

	i = 0;
	TZ_western_asia.labels = (char **)
		xmalloc(TZ_western_asia.nitems * sizeof (char *));
	TZ_western_asia.labels[i++] = TURKEY;
	TZ_western_asia.labels[i++] = WESTERN_USSR;
	TZ_western_asia.labels[i++] = IRAN;
	TZ_western_asia.labels[i++] = ISRAEL;
	TZ_western_asia.labels[i++] = SAUDI_ARABIA;

	i = 0;
	TZ_eastern_asia.labels = (char **)
		xmalloc(TZ_eastern_asia.nitems * sizeof (char *));
	TZ_eastern_asia.labels[i++] = CHINA;
	TZ_eastern_asia.labels[i++] = TAIWAN;
	TZ_eastern_asia.labels[i++] = HONGKONG;
	TZ_eastern_asia.labels[i++] = JAPAN;
	TZ_eastern_asia.labels[i++] = KOREA;
	TZ_eastern_asia.labels[i++] = SINGAPORE;

	i = 0;
	TZ_australia_newzealand.labels = (char **)
		xmalloc(TZ_australia_newzealand.nitems * sizeof (char *));
	TZ_australia_newzealand.labels[i++] = TASMANIA;
	TZ_australia_newzealand.labels[i++] = QUEENSLAND;
	TZ_australia_newzealand.labels[i++] = NORTH;
	TZ_australia_newzealand.labels[i++] = WEST;
	TZ_australia_newzealand.labels[i++] = SOUTH;
	TZ_australia_newzealand.labels[i++] = VICTORIA;
	TZ_australia_newzealand.labels[i++] = NEW_SOUTH_WALES;
	TZ_australia_newzealand.labels[i++] = BROKEN_HILL;
	TZ_australia_newzealand.labels[i++] = YANCOWINNA;
	TZ_australia_newzealand.labels[i++] = LHI;
	TZ_australia_newzealand.labels[i++] = NEW_ZEALAND;

	i = 0;
	TZ_canada.labels = (char **)xmalloc(TZ_canada.nitems * sizeof (char *));
	TZ_canada.labels[i++] = NEWFOUNDLAND;
	TZ_canada.labels[i++] = ATLANTIC;
	TZ_canada.labels[i++] = EASTERN;
	TZ_canada.labels[i++] = CENTRAL;
	TZ_canada.labels[i++] = EAST_SASKATCHEWAN;
	TZ_canada.labels[i++] = MOUNTAIN;
	TZ_canada.labels[i++] = PACIFIC;
	TZ_canada.labels[i++] = YUKON;

	i = 0;
	TZ_europe.labels = (char **)xmalloc(TZ_europe.nitems * sizeof (char *));
	TZ_europe.labels[i++] = BRITAIN;
	TZ_europe.labels[i++] = EIRE;
	TZ_europe.labels[i++] = ICELAND;
	TZ_europe.labels[i++] = POLAND;
	TZ_europe.labels[i++] = WESTERN_EUROPE;
	TZ_europe.labels[i++] = MIDDLE_EUROPE;
	TZ_europe.labels[i++] = EASTERN_EUROPE;

	i = 0;
	TZ_mexico.labels = (char **)xmalloc(TZ_mexico.nitems * sizeof (char *));
	TZ_mexico.labels[i++] = MEXICO_BAJA_NORTE;
	TZ_mexico.labels[i++] = MEXICO_BAJA_SUR;
	TZ_mexico.labels[i++] = MEXICO_GENERAL;

	i = 0;
	TZ_south_america.labels = (char **)
		xmalloc(TZ_south_america.nitems * sizeof (char *));
	TZ_south_america.labels[i++] = BRAZIL_EAST;
	TZ_south_america.labels[i++] = BRAZIL_WEST;
	TZ_south_america.labels[i++] = BRAZIL_ACRE;
	TZ_south_america.labels[i++] = BRAZIL_DE_NORONHA;
	TZ_south_america.labels[i++] = CHILE_CONTINENTAL;
	TZ_south_america.labels[i++] = CHILE_EASTER_ISLAND;

	i = 0;
	TZ_united_states.labels = (char **)
		xmalloc(TZ_united_states.nitems * sizeof (char *));
	TZ_united_states.labels[i++] = USA_EASTERN;
	TZ_united_states.labels[i++] = USA_CENTRAL;
	TZ_united_states.labels[i++] = USA_MOUNTAIN;
	TZ_united_states.labels[i++] = USA_PACIFIC;
	TZ_united_states.labels[i++] = USA_EAST_INDIANA;
	TZ_united_states.labels[i++] = USA_ARIZONA;
	TZ_united_states.labels[i++] = USA_MICHIGAN;
	TZ_united_states.labels[i++] = USA_SAMOA;
	TZ_united_states.labels[i++] = USA_ALASKA;
	TZ_united_states.labels[i++] = USA_ALEUTIAN;
	TZ_united_states.labels[i++] = USA_HAWAII;

	tz_region_init(&regions);

	gmt_offsets[0].help = dl_get_attr_help(ATTR_TZ_GMT, &gmt_help);
	gmt_offsets[0].label = dl_get_attr_prompt(ATTR_TZ_GMT);

	tz_filename[0].help = dl_get_attr_help(ATTR_TZ_FILE, &file_help);
	tz_filename[0].label = dl_get_attr_prompt(ATTR_TZ_FILE);

	been_here = 1;
}

typedef struct item Item;
struct item {
	char	*label;
	Menu	*submenu;
};

static int
sort_regions(const void *v1, const void *v2)
{
	Item	*item1 = (Item *)v1;
	Item	*item2 = (Item *)v2;

	return (strcoll(item1->label, item2->label));
}

static void
tz_region_init(Menu *regions)
{
	Menu	**submenus = (Menu **)regions->values;
	Item	*items;
	int	nitems = regions->nitems;
	int	i, j, k;

	items = (Item *)xmalloc(nitems * sizeof (Item));

	i = 0;
	items[i++].label = AFRICA;
	items[i++].label = WESTERN_ASIA;
	items[i++].label = EASTERN_ASIA;
	items[i++].label = AUSTRALIA_NEWZEALAND;
	items[i++].label = CANADA;
	items[i++].label = EUROPE;
	items[i++].label = MEXICO;
	items[i++].label = SOUTH_AMERICA;
	items[i++].label = UNITED_STATES;
	items[i++].label = GMT_OFFSET;		/* XXX not sorted */
	items[i++].label = TZ_FILE_NAME;	/* XXX not sorted */

	for (i = 0; i < nitems; i++)
		items[i].submenu = submenus[i];

	qsort((void *)items, nitems - 2, sizeof (Item), sort_regions);

	regions->labels = (char **)xmalloc(nitems * sizeof (char *));

	for (i = 0; i < nitems; i++) {
		Menu	*menu;
		char	**filenames;
		int	valid_menu_item_count;
		/*
		 * Transfer sorted values to regions menu.
		 */
		regions->labels[i] = items[i].label;
		submenus[i] = items[i].submenu;

		if (submenus[i] == (Menu *)0)
			continue;

		/*
		 * Set default selection
		 */
		if (strcmp(regions->labels[i], UNITED_STATES) == 0)
			regions->selected = i;

		menu = submenus[i];
		filenames = (char **)menu->values;

		/*
		 * menu->nitems represents the original number of items
		 * on the list. valid_menu_item_count will represent the
		 * number of validated items on the list, after each is
		 * validiated below.
		 */
		valid_menu_item_count = menu->nitems;

		j = 0;
		while (j < menu->nitems) {
			/*
			 * If the timezone in "filenames[j]" is non-NULL and
			 * is not in the TZDIR directory, then remove it from
			 * the list of choices by bumping up all subsequent
			 * entries in the list, and nulling out the last entry.
			 */
			if (filenames[j] != (char *)0 &&
			    tz_valid(filenames[j]) == B_FALSE) {

				/* bump up by one all subsequent entries */
				for (k = j + 1;  k < menu->nitems; k++) {
					menu->labels[k - 1] = menu->labels[k];
					filenames[k - 1] = filenames[k];
				}

				/*
				 * zero out the last non-NULL entry on the list,
				 * since it has been bumped up one entry in the
				 * list.
				 */
				menu->labels[k - 1] = (char *)NULL;
				filenames[k - 1] = (char *)NULL;

				/*
				 * decrement total number of valid entries
				 * remaining
				 */
				valid_menu_item_count--;
			} else {
				/*
				 * increment "j" ONLY if the entry is valid.
				 * Otherwise, the next entry could be invalid,
				 * and getting bumped up would skip checking
				 * its validity.
				 */
				j++;
			}
		}

		/*
		 * update menu->nitems with the number of VALID list entries
		 */
		menu->nitems = valid_menu_item_count;
	}
}

void
ui_get_timezone(MSG *mp, int reply_to)
{
	static	int been_here = 0;
	static	Field_help help;
	Menu	**tz_menus;
	char	timezone[MAX_TZ+1];
	int	region_pick;
	int	tz_pick;
	int	i;

	tz_init();

	(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
				(void *)timezone, MAX_TZ+1);
	(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
				(void *)&region_pick, sizeof (region_pick));
	(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
				(void *)&tz_pick, sizeof (tz_pick));
	msg_delete(mp);

	/*
	 * if first time through, then use "regions.selected" instead of
	 * using "region_pick", because tz_region_init() initializes what
	 * the initial region selection should be (set to United States).
	 * "region_pick" from thsi point on will hold the correct current
	 * value from that chosen by the user
	 */
	if (!been_here)
		region_pick = regions.selected; /* set from tz_region_init */

	been_here = 1;

	tz_menus = (Menu **)regions.values;
	for (i = 0; i < regions.nitems; i++) {
		if (tz_menus[i] != (Menu *)0) {
			if (i == region_pick)
				tz_menus[i]->selected = tz_pick;
			else
				tz_menus[i]->selected = NO_INTEGER_VALUE;
		}
	}
	if (region_pick != -1)
		regions.selected = region_pick;
	/* else use default from tz_init */

	region_menu[0].help = dl_get_attr_help(ATTR_TZ_REGION, &help);
	region_menu[0].label = dl_get_attr_prompt(ATTR_TZ_REGION);
	region_menu[0].value = (void *)&regions;

	dl_get_timezone(
	    timezone, region_menu, gmt_offsets, tz_filename, reply_to);
}
