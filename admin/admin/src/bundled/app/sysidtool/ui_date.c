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

#pragma	ident	"@(#)ui_date.c 1.7 95/11/13"

/*
 *	File:		get_date_time.c
 *
 *	Description:	This file contains the routines needed to prompt
 *			the user for the date and time.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "sysid_ui.h"
#include "sysid_msgs.h"

#define	MAX_DATE	64

static Validate_proc	ui_valid_day;
static Validate_proc	ui_valid_year;
static Validate_proc	ui_update_date;
static int ui_get_year(char *year);

static Field_desc	dateinfo[] = {
	{ FIELD_TEXT, (void *)ATTR_DATE_AND_TIME, NULL, NULL, NULL,
		-1, -1, -1, -1, FF_RDONLY | FF_SUMMARY | FF_LAB_LJUST,
		ui_update_date },
	{ FIELD_TEXT, (void *)ATTR_YEAR, NULL, NULL, NULL,
		4, MAX_YEAR, 0, 9999,
		FF_LAB_ALIGN | FF_LAB_RJUST | FF_TYPE_TO_WIPE | FF_KEYFOCUS |
		FF_VALREQ,
		ui_valid_year },
	{ FIELD_TEXT, (void *)ATTR_MONTH, NULL, NULL, NULL,
		2, MAX_MONTH, 1, 12,
		FF_LAB_ALIGN | FF_LAB_RJUST | FF_TYPE_TO_WIPE | FF_VALREQ,
		ui_valid_integer },
	{ FIELD_TEXT, (void *)ATTR_DAY, NULL, NULL, NULL,
		2, MAX_DAY, 1, 31,
		FF_LAB_ALIGN | FF_LAB_RJUST | FF_TYPE_TO_WIPE | FF_VALREQ,
		ui_valid_day },
	{ FIELD_TEXT, (void *)ATTR_HOUR, NULL, NULL, NULL,
		2, MAX_HOUR, 0, 23,
		FF_LAB_ALIGN | FF_LAB_RJUST | FF_TYPE_TO_WIPE | FF_VALREQ,
		ui_valid_integer },
	{ FIELD_TEXT, (void *)ATTR_MINUTE, NULL, NULL, NULL,
		2, MAX_MINUTE, 0, 59,
		FF_LAB_ALIGN | FF_LAB_RJUST | FF_TYPE_TO_WIPE | FF_VALREQ,
		ui_valid_integer }
};

static int
ui_valid_day(Field_desc *f)
{
	static days_per_month[] = /* Feb. handled below */
		{ 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

	char	*numbers = "0123456789";
	char	*input = (char *)f->value;
	char	*cp;
	int	year;
	int	month;	/* user's input, therefore indexed from 1 */
	int	day;	/* user's input, therefore indexed from 1 */

	for (cp = input; *cp != '\0'; cp++)
		if (strchr(numbers, *cp) == (char *)0)
			return (SYSID_ERR_BAD_DIGIT);

	day = atoi(input);
	month = atoi(dateinfo[2].value);	/* XXX */
	year = atoi(dateinfo[1].value);		/* XXX */

	if (day < 1)
		return (SYSID_ERR_MIN_VALUE_EXCEEDED);

	if (day > days_per_month[month - 1])
		return (SYSID_ERR_MAX_VALUE_EXCEEDED);

	/*
	 * Leap year check
	 */
	if (month == 2 && (year % 4 != 0) && day == days_per_month[month - 1])
		return (SYSID_ERR_MAX_VALUE_EXCEEDED);

	return (SYSID_SUCCESS);
}

static int
ui_valid_year(Field_desc *f)
{
	char *input = (char *)f->value;
	int ret;

	/* make sure it's a valid integer first */
	ret = ui_valid_integer(f);
	if (ret != SYSID_SUCCESS)
		return (ret);

	/* make sure it's a reasonable year */
	ret = ui_get_year(input);
	if (ret < 0)
		return (SYSID_ERR_BAD_YEAR);
	else
		return (SYSID_SUCCESS);
}

/*
 * ui_update_date:
 *
 *	This routine refreshes the date and time line that is displayed
 *	at the top of the date and time form.  It is used to refresh
 *	the date and time after the user leaves any field of the form.
 *
 *	Note that the values should be validated prior to being used
 *	to update the date field.  If the values are not validated,
 *	weird things can happen.
 */

int
ui_update_date(Field_desc *f)
{

	Field_desc *year_field;
	Field_desc *month_field;
	Field_desc *day_field;
	Field_desc *hour_field;
	Field_desc *minute_field;
	char	*year;
	char	*month;
	char	*day;
	char	*hour;
	char	*minute;
	char	*date;
	struct tm tm;
	int ret;

	date   = f->value;

	year_field   = &dateinfo[1];
	month_field  = &dateinfo[2];
	day_field    = &dateinfo[3];
	hour_field   = &dateinfo[4];
	minute_field = &dateinfo[5];

	year   = year_field->value;
	month  = month_field->value;
	day    = day_field->value;
	hour   = hour_field->value;
	minute = minute_field->value;


	ret = ui_valid_year(year_field);
	if (ret != SYSID_SUCCESS)
		return (ret);
	tm.tm_year = ui_get_year(year);

	ret = ui_valid_integer(month_field);
	if (ret != SYSID_SUCCESS)
		return (ret);
	tm.tm_mon = atoi(month)-1;

	ret = ui_valid_day(day_field);
	if (ret != SYSID_SUCCESS)
		return (ret);
	tm.tm_mday = atoi(day);

	ret = ui_valid_integer(hour_field);
	if (ret != SYSID_SUCCESS)
		return (ret);
	tm.tm_hour = atoi(hour);

	ret = ui_valid_integer(minute_field);
	if (ret != SYSID_SUCCESS)
		return (ret);
	tm.tm_min = atoi(minute);

	(void) strftime(date, MAX_DATE, "%x %R", &tm);

	return (SYSID_SUCCESS);
}

static int
ui_get_year(char *year)
{
	int len;

	/* -1 indicates an invalid year */
	int year_int = -1;

	/*
	 * year length:
	 * 1: e.g. 3 --> 03
	 * 2: e.g. 43 --> 43
	 * 3: e.g. 343 --> nonsense!
	 * 4: e.g. 1945 --> 45
	 * 4: e.g. 2045 --> 45
	 */
	if (!year)
		return (year_int);

	len = strlen(year);
	if (len == 1 || len == 2)
		year_int = atoi(year);
	else if (len == 4)
		year_int = atoi(year+2);

	return (year_int);
}

/*
 * ui_get_date:
 *
 *	This routine is the client interface routine
 *	used for retrieving the date & time information from the user.
 *
 *	Input:	pointers to character buffers in which to place the
 *		values for the various components of date & time.
 */

void
ui_get_date(MSG *mp, int reply_to)
{
	static char	year[MAX_YEAR+1];
	static char	month[MAX_MONTH+1];
	static char	day[MAX_DAY+1];
	static char	hour[MAX_HOUR+1];
	static char	minute[MAX_MINUTE+1];
	static char	date[MAX_DATE+1];
	static int	been_here;
	static Field_help help, *phelp;

	if (!been_here) {
		phelp = dl_get_attr_help(ATTR_DATE_AND_TIME, &help);

		dateinfo[0].help = phelp;
		dateinfo[0].label = dl_get_attr_name(ATTR_DATE_AND_TIME);
		dateinfo[0].value = date;

		dateinfo[1].help = phelp;
		dateinfo[1].label = YEAR;
		dateinfo[1].value = year;

		dateinfo[2].help = phelp;
		dateinfo[2].label = MONTH;
		dateinfo[2].value = month;

		dateinfo[3].help = phelp;
		dateinfo[3].label = DAY;
		dateinfo[3].value = day;

		dateinfo[4].help = phelp;
		dateinfo[4].label = HOUR;
		dateinfo[4].value = hour;

		dateinfo[5].help = phelp;
		dateinfo[5].label = MINUTE;
		dateinfo[5].value = minute;

		been_here = 1;
	}

	(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
					(void *)year, sizeof (year));
	(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
					(void *)month, sizeof (month));
	(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
					(void *)day, sizeof (day));
	(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
					(void *)hour, sizeof (hour));
	(void) msg_get_arg(mp, (Sysid_attr *)0, (Val_t *)0,
					(void *)minute, sizeof (minute));
	msg_delete(mp);

	(void) ui_update_date(dateinfo);

	dl_do_form(
		dl_get_attr_title(ATTR_DATE_AND_TIME),
		dl_get_attr_text(ATTR_DATE_AND_TIME),
		dateinfo, sizeof (dateinfo) / sizeof (Field_desc),
		reply_to);
}
