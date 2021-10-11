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

#pragma	ident	"@(#)tty_timezone.c 1.5 93/11/18"

/*
 *	File:		timezone.c
 *
 *	Description:	This file contains the routines needed to prompt
 *			the user the system timezone.
 */

#include <stdlib.h>
#include <string.h>
#include <tzfile.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include "tty_defs.h"
#include "tty_msgs.h"

#define	MAX_SIZE_TZ	50

static Field_desc tz[] = {
	{ FIELD_EXCLUSIVE_CHOICE, (void *)ATTR_TZ_INDEX, NULL, NULL, NULL,
	    -1, -1, -1, -1, FF_LAB_LJUST | FF_LAB_ALIGN | FF_VALREQ | FF_CANCEL,
	    ui_valid_choice }
};

/*ARGSUSED*/
void
do_get_timezone(
	char		*timezone,
	Field_desc	*regions,
	Field_desc	*gmt_offset,
	Field_desc	*tz_filename,
	int		reply_to)
{
	Field_help help;
	char	tzname[MAXPATHLEN+1];
	char	offset[MAX_GMT_OFFSET+1];
	int	region_pick;
	int	tz_pick;
	Menu	*region_menu;
	Menu	**tz_menus;
	MSG	*mp;
	int	ch;

	if (is_install_environment()) {
		form_intro(INTRO_TITLE, INTRO_TEXT, 
			get_attr_help(ATTR_NONE, &help), INTRO_ONE_TIME_ONLY);
	}

	region_menu = (Menu *)regions->value;
	tz_menus = (Menu **)region_menu->values;

	tzname[0] = NULL;
	offset[0] = NULL;

	/* Keep looping until a timezone is selected */
	timezone[0] = NULL;
	while (timezone[0] == NULL) {
		/*
		 * Display regions menu
		 */
		(void) form_common(
				TIMEZONE_TITLE, TIMEZONE_TEXT,
				regions, 1, _get_err_string);

		region_pick = region_menu->selected;
		/*
		 * Check what was selected
		 */
		if (strcmp(region_menu->labels[region_pick], GMT_OFFSET) == 0) {
			/*
			 * Prompt for user to enter offset from GMT
			 */
			gmt_offset->value = (void *)offset;

			ch = form_common(
					TZ_GMT_TITLE, TZ_GMT_TEXT,
					gmt_offset, 1, _get_err_string);

			/* Check if anything entered */
			if (is_continue(ch) && offset[0] != NULL)
				(void) strcpy(timezone, tz_from_offset(offset));
		} else if (strcmp(
		    region_menu->labels[region_pick], TZ_FILE_NAME) == 0) {
			/*
			 * Prompt for the rules file name
			 */
			tz_filename->value = (void *)tzname;

			if (tz_filename->field_length >
			    tz_filename->value_length) {
				tz_filename->field_length =
				tz_filename->value_length;
			}
			ch = form_common(
					TZ_FILE_TITLE, TZ_FILE_TEXT,
					tz_filename, 1, _get_err_string);

			if (is_continue(ch) && tzname[0] != NULL)
				(void) strcpy(timezone, tzname);
		} else {
			/*
			 * A region was selected.  Display the
			 * timezones for that region.
			 */
			Menu	*tz_menu = tz_menus[region_pick];

			tz_pick = tz_menu->selected;
			if (tz_pick < 0 || tz_pick >= tz_menu->nitems)
				tz_menu->selected = tz_pick = NO_INTEGER_VALUE;

			tz[0].help = get_attr_help(ATTR_TZ_INDEX, &help);
			tz[0].label = TZ_INDEX_PROMPT;
			tz[0].value = (void *)tz_menu;

			ch = form_common(
				TZ_INDEX_TITLE, TZ_INDEX_TEXT,
				tz, 1, _get_err_string);

			if (is_continue(ch)) {
				/*
				 * If valid timezone was selected, copy
				 * the token into the timezone buffer.
				 * This value will be returned.
				 */
				tz_pick = tz_menu->selected;
				(void) strcpy(timezone,
				    ((char **)tz_menu->values)[tz_pick]);
			}
		}
	}
	mp = msg_new();
	(void) msg_set_type(mp, REPLY_OK);
	(void) msg_add_arg(mp, ATTR_TIMEZONE, VAL_STRING,
				(void *)timezone, MAX_TZ+1);
	(void) msg_add_arg(mp, ATTR_TZ_REGION, VAL_INTEGER,
				(void *)&region_pick, sizeof (region_pick));
	(void) msg_add_arg(mp, ATTR_TZ_INDEX, VAL_INTEGER,
				(void *)&tz_pick, sizeof (tz_pick));
	(void) msg_send(mp, reply_to);
	msg_delete(mp);
}
